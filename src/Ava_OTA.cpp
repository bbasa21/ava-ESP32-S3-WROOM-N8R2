#include "Ava_OTA.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <mbedtls/sha256.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_crt_bundle.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "Ava_BuildInfo.h"
#include "Ava_NetworkLock.h"
#include "Ava_OTA_Display.h"

namespace {
constexpr char MANIFEST[]="https://raw.githubusercontent.com/bbasa21/ava-ESP32-S3-WROOM-N8R2/main/ota/manifest.json";
constexpr size_t CHUNK=4096;
volatile bool busy=false;
bool ready=false;
uint32_t newBuild=0;
String imageUrl, expected;

bool getString(const String& j,const char* key,String& out){
 String k=String("\"")+key+"\"";int p=j.indexOf(k);if(p<0)return false;
 p=j.indexOf(':',p+k.length());if(p<0)return false;++p;
 while(p<(int)j.length()&&isspace((unsigned char)j[p]))++p;
 if(p>=(int)j.length()||j[p++]!='"')return false;
 int e=j.indexOf('"',p);if(e<0)return false;out=j.substring(p,e);return true;
}
bool getBuild(const String& j,uint32_t& out){
 int p=j.indexOf("\"build\"");if(p<0)return false;p=j.indexOf(':',p+7);if(p<0)return false;++p;
 while(p<(int)j.length()&&isspace((unsigned char)j[p]))++p;
 if(p>=(int)j.length()||!isdigit((unsigned char)j[p]))return false;
 uint64_t n=0;while(p<(int)j.length()&&isdigit((unsigned char)j[p])){n=n*10+j[p++]-'0';if(n>UINT32_MAX)return false;}out=(uint32_t)n;return true;
}
bool validHash(const String& s){if(s.length()!=64)return false;for(char c:s)if(!isxdigit((unsigned char)c))return false;return true;}
String hexHash(const uint8_t* d){const char* h="0123456789abcdef";String s;s.reserve(64);for(int i=0;i<32;i++){s+=h[d[i]>>4];s+=h[d[i]&15];}return s;}
bool partitionHash(const esp_partition_t* p,size_t n,String& out){
 if(!p||!n||n>p->size)return false;uint8_t b[CHUNK],d[32];mbedtls_sha256_context c;mbedtls_sha256_init(&c);mbedtls_sha256_starts(&c,0);
 for(size_t off=0;off<n;){size_t z=min(sizeof(b),n-off);if(esp_partition_read(p,off,b,z)!=ESP_OK){mbedtls_sha256_free(&c);return false;}mbedtls_sha256_update(&c,b,z);off+=z;}
 mbedtls_sha256_finish(&c,d);mbedtls_sha256_free(&c);out=hexHash(d);return true;
}
void fail(const char* why){Serial.printf("[OTA2] ERROR: %s\n",why);avaOTAUISetStatus(AVA_OTA_UI_ERROR);delay(1500);avaOTAUIEnd();}
void installer(void*){
 busy=true;avaOTAUIBegin();avaOTAUISetStatus(AVA_OTA_UI_UPDATING);
 AvaNetworkLockGuard lock(pdMS_TO_TICKS(10000));
 if(!lock.isLocked()||WiFi.status()!=WL_CONNECTED||!ready){fail("Network unavailable or run OTA CHECK first.");busy=false;vTaskDelete(nullptr);return;}
 WiFiClientSecure client;
 client.useBuiltinCACertBundle();
 HTTPClient http;http.setConnectTimeout(12000);http.setTimeout(20000);http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
 if(!http.begin(client,imageUrl)){fail("Firmware request could not start.");busy=false;vTaskDelete(nullptr);return;}
 int code=http.GET(),length=http.getSize();if(code!=HTTP_CODE_OK||length<=0){http.end();fail("Invalid firmware HTTP response/length.");busy=false;vTaskDelete(nullptr);return;}
 const esp_partition_t* target=esp_ota_get_next_update_partition(nullptr);
 if(!target||(size_t)length>target->size){http.end();fail("OTA partition unavailable or image too large.");busy=false;vTaskDelete(nullptr);return;}
 esp_ota_handle_t handle=0;esp_err_t err=esp_ota_begin(target,(size_t)length,&handle);
 if(err!=ESP_OK){http.end();fail("esp_ota_begin failed.");busy=false;vTaskDelete(nullptr);return;}
 WiFiClient* stream=http.getStreamPtr();uint8_t b[CHUNK],digest[32];size_t done=0;uint32_t last=millis();bool ok=stream!=nullptr;
 mbedtls_sha256_context sha;mbedtls_sha256_init(&sha);mbedtls_sha256_starts(&sha,0);
 while(ok&&done<(size_t)length){
  size_t a=stream->available();if(!a){if(!stream->connected()||millis()-last>20000){ok=false;break;}vTaskDelay(1);continue;}
  size_t want=min(sizeof(b),(size_t)length-done);want=min(want,a);int got=stream->read(b,want);if(got<=0)continue;last=millis();
  if(esp_ota_write(handle,b,(size_t)got)!=ESP_OK){ok=false;break;}mbedtls_sha256_update(&sha,b,(size_t)got);done+=(size_t)got;avaOTAUISetProgress((int)(done*100ULL/(size_t)length));
 }
 mbedtls_sha256_finish(&sha,digest);mbedtls_sha256_free(&sha);http.end();
 if(!ok||done!=(size_t)length||hexHash(digest)!=expected){esp_ota_abort(handle);fail("Download incomplete or SHA-256 mismatch.");busy=false;vTaskDelete(nullptr);return;}
 avaOTAUISetStatus(AVA_OTA_UI_VERIFYING);err=esp_ota_end(handle);
 if(err!=ESP_OK){fail("ESP image validation failed.");busy=false;vTaskDelete(nullptr);return;}
 String flash;if(!partitionHash(target,done,flash)||flash!=expected){fail("Flash readback verification failed.");busy=false;vTaskDelete(nullptr);return;}
 if(esp_ota_set_boot_partition(target)!=ESP_OK){fail("Could not activate verified image.");busy=false;vTaskDelete(nullptr);return;}
 ready=false;avaOTAUISetProgress(100);avaOTAUISetStatus(AVA_OTA_UI_RESTARTING);Serial.printf("[OTA2] Build %lu verified. Restarting.\n",(unsigned long)newBuild);delay(1200);ESP.restart();vTaskDelete(nullptr);
}
}

bool avaOTACheckForUpdate(){
 if(busy)return false;AvaNetworkLockGuard lock(pdMS_TO_TICKS(10000));if(!lock.isLocked()||WiFi.status()!=WL_CONNECTED){Serial.println("[OTA2] CHECK FAILED: Wi-Fi unavailable/busy.");return false;}
 ready=false;WiFiClientSecure client;client.useBuiltinCACertBundle();HTTPClient http;http.setConnectTimeout(12000);http.setTimeout(15000);http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
 if(!http.begin(client,MANIFEST))return false;int code=http.GET();if(code!=HTTP_CODE_OK){http.end();Serial.printf("[OTA2] Manifest HTTP %d\n",code);return false;}
 String json=http.getString();http.end();
 if(!getBuild(json,newBuild)||!getString(json,"firmware_url",imageUrl)||!getString(json,"sha256",expected)||!validHash(expected)||!imageUrl.startsWith("https://")){Serial.println("[OTA2] Invalid manifest.");return false;}
 expected.toLowerCase();if(newBuild<=AVA_BUILD_NUMBER){Serial.printf("[OTA2] Up to date: local=%u remote=%lu\n",(unsigned)AVA_BUILD_NUMBER,(unsigned long)newBuild);return true;}
 ready=true;Serial.printf("[OTA2] Update available: build %lu. Run OTA INSTALL.\n",(unsigned long)newBuild);return true;
}
bool avaOTAStartInstall(){if(busy||!ready){Serial.println("[OTA2] Run OTA CHECK first.");return false;}TaskHandle_t task=nullptr;if(xTaskCreatePinnedToCore(installer,"AVA_OTA2",12288,nullptr,1,&task,0)!=pdPASS){Serial.println("[OTA2] Installer task creation failed.");return false;}return true;}
bool avaOTAIsRunning(){return busy;}
