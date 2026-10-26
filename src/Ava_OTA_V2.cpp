#include "Ava_OTA_V2.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <mbedtls/sha256.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <cJSON.h>

#include "Ava_BuildInfo.h"
#include "Ava_NetworkLock.h"

namespace {
constexpr uint32_t CONNECT_TIMEOUT_MS = 10000;
constexpr uint32_t HTTP_TIMEOUT_MS = 15000;
constexpr uint32_t STREAM_IDLE_TIMEOUT_MS = 20000;
constexpr size_t BUFFER_SIZE = 4096;
constexpr size_t MAX_MANIFEST_BYTES = 2048;

struct Manifest {
  uint32_t build = 0;
  String version;
  String url;
  String sha256;
};

bool isHexSha256(const String& s) {
  if (s.length() != 64) return false;
  for (size_t i = 0; i < s.length(); ++i) {
    const char c = s[i];
    if (!((c >= '0' && c <= '9') ||
          (c >= 'a' && c <= 'f') ||
          (c >= 'A' && c <= 'F'))) return false;
  }
  return true;
}

String digestToHex(const uint8_t digest[32]) {
  static const char kHexDigits[] = "0123456789abcdef";
  String out;
  out.reserve(64);
  for (size_t i = 0; i < 32; ++i) {
    out += kHexDigits[digest[i] >> 4];
    out += kHexDigits[digest[i] & 0x0f];
  }
  return out;
}

bool parseManifest(const String& body, Manifest& out) {
  if (body.isEmpty() || body.length() > MAX_MANIFEST_BYTES) {
    Serial.println("[OTA2] Invalid manifest size.");
    return false;
  }

  cJSON* root = cJSON_ParseWithLength(body.c_str(), body.length());
  if (!root || !cJSON_IsObject(root)) {
    if (root) cJSON_Delete(root);
    Serial.println("[OTA2] Manifest JSON is invalid.");
    return false;
  }

  const cJSON* build = cJSON_GetObjectItemCaseSensitive(root, "build");
  const cJSON* version = cJSON_GetObjectItemCaseSensitive(root, "version");
  const cJSON* url = cJSON_GetObjectItemCaseSensitive(root, "firmware_url");
  const cJSON* sha = cJSON_GetObjectItemCaseSensitive(root, "sha256");

  bool ok = cJSON_IsNumber(build) &&
            build->valuedouble >= 0 &&
            build->valuedouble <= 4294967295.0 &&
            build->valuedouble == (double)(uint32_t)build->valuedouble &&
            cJSON_IsString(version) && version->valuestring &&
            cJSON_IsString(url) && url->valuestring &&
            cJSON_IsString(sha) && sha->valuestring;

  if (ok) {
    out.build = (uint32_t)build->valuedouble;
    out.version = version->valuestring;
    out.url = url->valuestring;
    out.sha256 = sha->valuestring;
    out.sha256.toLowerCase();
    ok = !out.version.isEmpty() &&
         out.url.startsWith("https://") &&
         out.url.length() < 512 &&
         isHexSha256(out.sha256);
  }

  cJSON_Delete(root);
  if (!ok) Serial.println("[OTA2] Manifest fields failed validation.");
  return ok;
}

bool getHttps(const String& url, String& body, size_t maxBytes) {
  if (!url.startsWith("https://")) {
    Serial.println("[OTA2] Refusing non-HTTPS URL.");
    return false;
  }

  WiFiClientSecure client;
  // Kept compatible with the existing firmware for initial feature-branch
  // bring-up. Before production use, replace with CA/certificate validation.
  client.setInsecure();

  HTTPClient http;
  http.setConnectTimeout(CONNECT_TIMEOUT_MS);
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  if (!http.begin(client, url)) {
    Serial.println("[OTA2] HTTP begin failed.");
    return false;
  }

  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("[OTA2] HTTP GET failed: %d\n", code);
    http.end();
    client.stop();
    return false;
  }

  const int length = http.getSize();
  if (length <= 0 || (size_t)length > maxBytes) {
    Serial.println("[OTA2] Response size invalid or too large.");
    http.end();
    client.stop();
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();
  body.reserve((size_t)length);
  uint32_t lastData = millis();

  while (body.length() < (size_t)length) {
    const int available = stream ? stream->available() : 0;
    if (available > 0) {
      char buf[256];
      size_t want = (size_t)available;
      if (want > sizeof(buf)) want = sizeof(buf);
      const int n = stream->readBytes(buf, want);
      if (n > 0) {
        body.concat(buf, (size_t)n);
        lastData = millis();
        continue;
      }
    }

    if (!stream || (!stream->connected() && stream->available() == 0) ||
        millis() - lastData > STREAM_IDLE_TIMEOUT_MS) {
      Serial.println("[OTA2] Response interrupted or timed out.");
      http.end();
      client.stop();
      return false;
    }
    delay(1);
  }

  http.end();
  client.stop();
  return body.length() == (size_t)length;
}

bool installImage(const Manifest& manifest) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[OTA2] Wi-Fi is not connected.");
    return false;
  }

  String unused;
  WiFiClientSecure client;
  client.setInsecure(); // See TLS note in getHttps().
  HTTPClient http;
  http.setConnectTimeout(CONNECT_TIMEOUT_MS);
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  if (!http.begin(client, manifest.url)) {
    Serial.println("[OTA2] Could not open firmware URL.");
    return false;
  }

  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("[OTA2] Firmware HTTP error: %d\n", code);
    http.end();
    client.stop();
    return false;
  }

  const int contentLength = http.getSize();
  const esp_partition_t* running = esp_ota_get_running_partition();
  const esp_partition_t* target = esp_ota_get_next_update_partition(nullptr);
  if (contentLength <= 0 || !target || !running ||
      target->address == running->address ||
      (size_t)contentLength > target->size) {
    Serial.println("[OTA2] Invalid image length or OTA partition.");
    http.end();
    client.stop();
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();
  if (!stream || !Update.begin((size_t)contentLength)) {
    Serial.printf("[OTA2] Update.begin failed: %u\n", Update.getError());
    http.end();
    client.stop();
    return false;
  }

  uint8_t buffer[BUFFER_SIZE];
  size_t total = 0;
  uint32_t lastData = millis();
  bool ok = true;
  mbedtls_sha256_context hash;
  mbedtls_sha256_init(&hash);
  mbedtls_sha256_starts(&hash, 0);

  Serial.printf("[OTA2] Downloading build %lu (%d bytes).\n",
                (unsigned long)manifest.build, contentLength);

  while (total < (size_t)contentLength) {
    int available = stream->available();
    if (available <= 0) {
      if ((!stream->connected() && stream->available() == 0) ||
          millis() - lastData > STREAM_IDLE_TIMEOUT_MS) {
        Serial.println("[OTA2] Download interrupted or timed out.");
        ok = false;
        break;
      }
      delay(1);
      continue;
    }

    size_t want = (size_t)available;
    const size_t remaining = (size_t)contentLength - total;
    if (want > sizeof(buffer)) want = sizeof(buffer);
    if (want > remaining) want = remaining;

    const int n = stream->read(buffer, want);
    if (n <= 0) continue;
    lastData = millis();

    if (Update.write(buffer, (size_t)n) != (size_t)n) {
      Serial.printf("[OTA2] Flash write failed: %u\n", Update.getError());
      ok = false;
      break;
    }

    mbedtls_sha256_update(&hash, buffer, (size_t)n);
    total += (size_t)n;
    if ((total % (32 * 1024)) < (size_t)n) {
      Serial.printf("[OTA2] Downloaded %u / %u bytes.\n",
                    (unsigned)total, (unsigned)contentLength);
    }
  }

  uint8_t digest[32];
  mbedtls_sha256_finish(&hash, digest);
  mbedtls_sha256_free(&hash);
  const String actualSha = digestToHex(digest);

  http.end();
  client.stop();

  if (!ok || total != (size_t)contentLength) {
    Serial.println("[OTA2] Incomplete download; aborting.");
    Update.abort();
    return false;
  }

  if (actualSha != manifest.sha256) {
    Serial.println("[OTA2] SHA-256 mismatch; aborting.");
    Update.abort();
    return false;
  }

  if (!Update.isFinished() || !Update.end(false)) {
    Serial.printf("[OTA2] Finalization failed: %u\n", Update.getError());
    Update.abort();
    return false;
  }

  const esp_err_t setBoot = esp_ota_set_boot_partition(target);
  if (setBoot != ESP_OK) {
    Serial.printf("[OTA2] Boot partition selection failed: %s\n",
                  esp_err_to_name(setBoot));
    return false;
  }

  const esp_partition_t* selected = esp_ota_get_boot_partition();
  if (!selected || selected->address != target->address) {
    Serial.println("[OTA2] Boot partition verification failed.");
    esp_ota_set_boot_partition(running);
    return false;
  }

  Serial.println("[OTA2] Image verified and boot partition selected. Restarting.");
  delay(500);
  ESP.restart();
  return true;
}
} // namespace

bool avaOTA2CheckAndInstall(const char* manifestUrl) {
  if (!manifestUrl || !String(manifestUrl).startsWith("https://")) {
    Serial.println("[OTA2] Invalid manifest URL.");
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[OTA2] Wi-Fi is not connected.");
    return false;
  }

  AvaNetworkLockGuard lock(pdMS_TO_TICKS(15000));
  if (!lock.isLocked()) {
    Serial.println("[OTA2] Could not acquire network lock.");
    return false;
  }

  Serial.println("[OTA2] Checking experimental manifest.");
  String body;
  if (!getHttps(String(manifestUrl), body, MAX_MANIFEST_BYTES)) return false;

  Manifest manifest;
  if (!parseManifest(body, manifest)) return false;

  Serial.printf("[OTA2] Local build: %lu; remote build: %lu; version: %s\n",
                (unsigned long)AVA_BUILD_NUMBER,
                (unsigned long)manifest.build,
                manifest.version.c_str());

  if (manifest.build <= AVA_BUILD_NUMBER) {
    Serial.println("[OTA2] No newer firmware.");
    return true;
  }

  return installImage(manifest);
}
