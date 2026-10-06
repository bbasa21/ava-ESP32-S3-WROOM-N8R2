#include "Ava_OTA.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <mbedtls/sha256.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>

#include "Ava_NetworkLock.h"
#include "Ava_BuildInfo.h"
#include "Ava_OTA_Display.h"

static const char* AVA_OTA_MANIFEST_URL =
    "https://raw.githubusercontent.com/bbasa21/ava-ESP32-S3-WROOM-N8R2/main/ota/manifest.json";

static constexpr uint32_t OTA_CONNECT_TIMEOUT_MS = 10000;
static constexpr uint32_t OTA_HTTP_TIMEOUT_MS = 15000;
static constexpr uint32_t OTA_STREAM_TIMEOUT_MS = 20000;
static constexpr size_t OTA_BUFFER_SIZE = 4096;

static bool avaOTATaskStarted = false;
static TaskHandle_t avaOTATaskHandle = nullptr;
static volatile bool avaOTARunning = false;
static bool avaOTAUpdateAvailable = false;
static uint32_t avaOTARemoteBuild = 0;
static String avaOTAFirmwareURL;
static String avaOTAExpectedSHA256;

static bool avaOTAIsValidSHA256(const String& value)
{
    if (value.length() != 64) return false;
    for (size_t i = 0; i < value.length(); i++)
    {
        char c = value[i];
        if (!((c >= '0' && c <= '9') ||
              (c >= 'a' && c <= 'f') ||
              (c >= 'A' && c <= 'F')))
            return false;
    }
    return true;
}

static bool avaOTAExtractString(const String& json, const char* key, String& value)
{
    String searchKey = String("\"") + key + "\":";
    int start = json.indexOf(searchKey);
    if (start < 0) return false;

    start += searchKey.length();
    while (start < json.length() &&
           (json[start] == ' ' || json[start] == '\t' ||
            json[start] == '\r' || json[start] == '\n'))
        start++;

    if (start >= json.length() || json[start] != '\"') return false;
    start++;

    int end = json.indexOf("\"", start);
    if (end < 0) return false;

    value = json.substring(start, end);
    return true;
}

static bool avaOTAExtractInt(const String& json, const char* key, uint32_t& value)
{
    String searchKey = String("\"") + key + "\":";
    int start = json.indexOf(searchKey);
    if (start < 0) return false;

    start += searchKey.length();
    while (start < json.length() &&
           (json[start] == ' ' || json[start] == '\t' ||
            json[start] == '\r' || json[start] == '\n'))
        start++;

    int end = start;
    while (end < json.length() && json[end] >= '0' && json[end] <= '9')
        end++;

    if (end == start) return false;

    value = static_cast<uint32_t>(json.substring(start, end).toInt());
    return true;
}

static String avaOTASHA256ToString(const uint8_t* digest)
{
    static const char* hex = "0123456789abcdef";
    String result;
    result.reserve(64);

    for (size_t i = 0; i < 32; i++)
    {
        result += hex[(digest[i] >> 4) & 0x0F];
        result += hex[digest[i] & 0x0F];
    }
    return result;
}

static void avaOTAReportProgress(size_t written, size_t total)
{
    if (total == 0) return;

    static int lastPercent = -1;
    int percent = static_cast<int>((written * 100ULL) / total);
    if (percent > 100) percent = 100;

    avaOTAUISetProgress(percent);

    if (percent != lastPercent)
    {
        lastPercent = percent;
        Serial.print("[OTA] Progress: ");
        Serial.print(percent);
        Serial.print("% (");
        Serial.print(written);
        Serial.print("/");
        Serial.print(total);
        Serial.println(")");
    }

    if (percent == 100) lastPercent = -1;
}

bool avaOTACheckForUpdate()
{
    AvaNetworkLockGuard networkLock;

    if (!networkLock.isLocked())
    {
        Serial.println("[OTA] ERROR: Could not acquire network lock.");
        return false;
    }

    avaOTAUpdateAvailable = false;
    avaOTARemoteBuild = 0;
    avaOTAFirmwareURL = "";
    avaOTAExpectedSHA256 = "";

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[OTA] WiFi is not connected.");
        return false;
    }

    Serial.println();
    Serial.println("=========== AVA OTA CHECK ===========");
    Serial.print("[OTA] Local build: ");
    Serial.println(AVA_BUILD_NUMBER);
    Serial.print("[OTA] Manifest: ");
    Serial.println(AVA_OTA_MANIFEST_URL);

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.setConnectTimeout(OTA_CONNECT_TIMEOUT_MS);
    http.setTimeout(OTA_HTTP_TIMEOUT_MS);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    if (!http.begin(client, AVA_OTA_MANIFEST_URL))
    {
        Serial.println("[OTA] ERROR: Manifest HTTP begin failed.");
        client.stop();
        return false;
    }

    Serial.println("[OTA] GET manifest...");
    int httpCode = http.GET();

    Serial.print("[OTA] Manifest HTTP code: ");
    Serial.println(httpCode);

    if (httpCode != HTTP_CODE_OK)
    {
        http.end();
        client.stop();
        return false;
    }

    String manifest = http.getString();
    http.end();
    client.stop();

    Serial.print("[OTA] Manifest size: ");
    Serial.print(manifest.length());
    Serial.println(" bytes");

    uint32_t remoteBuild = 0;
    String firmwareURL;
    String expectedSHA256;

    if (!avaOTAExtractInt(manifest, "build", remoteBuild) ||
        !avaOTAExtractString(manifest, "firmware_url", firmwareURL) ||
        !avaOTAExtractString(manifest, "sha256", expectedSHA256))
    {
        Serial.println("[OTA] ERROR: Invalid manifest.");
        return false;
    }

    expectedSHA256.toLowerCase();

    if (!avaOTAIsValidSHA256(expectedSHA256))
    {
        Serial.println("[OTA] ERROR: Invalid SHA-256.");
        Serial.print("[OTA] SHA length: ");
        Serial.println(expectedSHA256.length());
        return false;
    }

    Serial.print("[OTA] Remote build: ");
    Serial.println(remoteBuild);
    Serial.print("[OTA] Remote SHA-256: ");
    Serial.println(expectedSHA256);

    if (remoteBuild <= AVA_BUILD_NUMBER)
    {
        Serial.println("[OTA] No newer firmware.");
        Serial.println("=====================================");
        return true;
    }

    avaOTARemoteBuild = remoteBuild;
    avaOTAFirmwareURL = firmwareURL;
    avaOTAExpectedSHA256 = expectedSHA256;
    avaOTAUpdateAvailable = true;

    Serial.println("[OTA] NEW FIRMWARE AVAILABLE.");
    Serial.print("[OTA] Firmware URL: ");
    Serial.println(avaOTAFirmwareURL);
    Serial.print("[OTA] Build: ");
    Serial.println(avaOTARemoteBuild);
    Serial.println("=====================================");

    return true;
}

bool avaOTAInstallUpdate()
{
    if (!avaOTAUpdateAvailable)
    {
        Serial.println("[OTA] No update is ready to install.");
        return false;
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[OTA] ERROR: WiFi disconnected.");
        return false;
    }

    AvaNetworkLockGuard networkLock;
    if (!networkLock.isLocked())
    {
        Serial.println("[OTA] ERROR: Could not acquire network lock.");
        return false;
    }

    Serial.println();
    Serial.println("=========== AVA OTA INSTALL ==========");
    Serial.print("[OTA] Installing build: ");
    Serial.println(avaOTARemoteBuild);

    avaOTAUIBegin();

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.setConnectTimeout(OTA_CONNECT_TIMEOUT_MS);
    http.setTimeout(OTA_HTTP_TIMEOUT_MS);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    if (!http.begin(client, avaOTAFirmwareURL))
    {
        Serial.println("[OTA] ERROR: Firmware HTTP begin failed.");
        avaOTAUISetStatus(AVA_OTA_UI_ERROR);
        delay(2000);
        avaOTAUIEnd();
        client.stop();
        return false;
    }

    Serial.println("[OTA] Downloading firmware...");
    int httpCode = http.GET();

    Serial.print("[OTA] Firmware HTTP code: ");
    Serial.println(httpCode);

    if (httpCode != HTTP_CODE_OK)
    {
        Serial.println("[OTA] ERROR: Firmware download failed.");
        http.end();
        client.stop();
        avaOTAUISetStatus(AVA_OTA_UI_ERROR);
        delay(2000);
        avaOTAUIEnd();
        return false;
    }

    int contentLength = http.getSize();

    Serial.print("[OTA] Firmware size: ");
    Serial.print(contentLength);
    Serial.println(" bytes");

    if (contentLength <= 0)
    {
        Serial.println("[OTA] ERROR: Invalid firmware size.");
        http.end();
        client.stop();
        avaOTAUISetStatus(AVA_OTA_UI_ERROR);
        delay(2000);
        avaOTAUIEnd();
        return false;
    }

    const esp_partition_t* targetPartition =
        esp_ota_get_next_update_partition(nullptr);
    const esp_partition_t* runningPartition =
        esp_ota_get_running_partition();

    if (targetPartition == nullptr || targetPartition == runningPartition)
    {
        Serial.println("[OTA] ERROR: Invalid OTA target partition.");
        http.end();
        client.stop();
        avaOTAUISetStatus(AVA_OTA_UI_ERROR);
        delay(2000);
        avaOTAUIEnd();
        return false;
    }

    Serial.print("[OTA] Target partition: ");
    Serial.println(targetPartition->label);
    Serial.print("[OTA] Target address: 0x");
    Serial.println(targetPartition->address, HEX);
    Serial.print("[OTA] Target size: ");
    Serial.print(targetPartition->size);
    Serial.println(" bytes");

    if (static_cast<size_t>(contentLength) > targetPartition->size)
    {
        Serial.println("[OTA] ERROR: Firmware is too large.");
        http.end();
        client.stop();
        avaOTAUISetStatus(AVA_OTA_UI_ERROR);
        delay(2000);
        avaOTAUIEnd();
        return false;
    }

    if (!Update.begin(static_cast<size_t>(contentLength)))
    {
        Serial.print("[OTA] ERROR: Update.begin failed: ");
        Serial.println(Update.getError());
        Update.printError(Serial);
        http.end();
        client.stop();
        avaOTAUISetStatus(AVA_OTA_UI_ERROR);
        delay(2000);
        avaOTAUIEnd();
        return false;
    }

    WiFiClient* stream = http.getStreamPtr();

    if (stream == nullptr)
    {
        Serial.println("[OTA] ERROR: Firmware stream unavailable.");
        Update.abort();
        http.end();
        client.stop();
        avaOTAUISetStatus(AVA_OTA_UI_ERROR);
        delay(2000);
        avaOTAUIEnd();
        return false;
    }

    uint8_t buffer[OTA_BUFFER_SIZE];
    size_t totalWritten = 0;
    uint32_t lastDataTime = millis();

    mbedtls_sha256_context sha256;
    mbedtls_sha256_init(&sha256);
    mbedtls_sha256_starts(&sha256, 0);

    bool downloadOK = true;

    Serial.println("[OTA] Firmware stream started.");

    while (totalWritten < static_cast<size_t>(contentLength))
    {
        size_t remaining =
            static_cast<size_t>(contentLength) - totalWritten;
        size_t requested = remaining;

        if (requested > OTA_BUFFER_SIZE)
            requested = OTA_BUFFER_SIZE;

        size_t available = stream->available();

        if (available == 0)
        {
            if (!stream->connected())
            {
                Serial.println("[OTA] ERROR: Stream disconnected.");
                downloadOK = false;
                break;
            }

            if (millis() - lastDataTime > OTA_STREAM_TIMEOUT_MS)
            {
                Serial.println("[OTA] ERROR: Stream timeout.");
                downloadOK = false;
                break;
            }

            delay(1);
            continue;
        }

        if (available < requested)
            requested = available;

        int bytesRead = stream->read(buffer, requested);

        if (bytesRead <= 0)
            continue;

        lastDataTime = millis();

        size_t written =
            Update.write(buffer, static_cast<size_t>(bytesRead));

        if (written != static_cast<size_t>(bytesRead))
        {
            Serial.println("[OTA] ERROR: Flash write failed.");
            Serial.print("[OTA] Expected write: ");
            Serial.println(bytesRead);
            Serial.print("[OTA] Actual write: ");
            Serial.println(written);
            Serial.print("[OTA] Update error: ");
            Serial.println(Update.getError());
            Update.printError(Serial);
            downloadOK = false;
            break;
        }

        mbedtls_sha256_update(
            &sha256,
            buffer,
            static_cast<size_t>(bytesRead)
        );

        totalWritten += static_cast<size_t>(bytesRead);

        avaOTAReportProgress(
            totalWritten,
            static_cast<size_t>(contentLength)
        );
    }

    uint8_t digest[32];
    mbedtls_sha256_finish(&sha256, digest);
    mbedtls_sha256_free(&sha256);

    String downloadedSHA256 =
        avaOTASHA256ToString(digest);

    Serial.print("[OTA] Downloaded SHA-256: ");
    Serial.println(downloadedSHA256);
    Serial.print("[OTA] Expected SHA-256:   ");
    Serial.println(avaOTAExpectedSHA256);

    if (!downloadOK ||
        totalWritten != static_cast<size_t>(contentLength))
    {
        Serial.println("[OTA] ERROR: Firmware download incomplete.");
        Update.abort();
        http.end();
        client.stop();
        avaOTAUISetStatus(AVA_OTA_UI_ERROR);
        delay(2000);
        avaOTAUIEnd();
        return false;
    }

    if (downloadedSHA256 != avaOTAExpectedSHA256)
    {
        Serial.println("[OTA] ERROR: SHA-256 mismatch.");
        Update.abort();
        http.end();
        client.stop();
        avaOTAUISetStatus(AVA_OTA_UI_ERROR);
        delay(2000);
        avaOTAUIEnd();
        return false;
    }

    Serial.println("[OTA] SHA-256 verified.");
    avaOTAUISetStatus(AVA_OTA_UI_VERIFYING);

    if (!Update.isFinished())
    {
        Serial.print("[OTA] ERROR: Update incomplete: ");
        Serial.print(Update.progress());
        Serial.print("/");
        Serial.println(Update.size());

        Update.abort();
        http.end();
        client.stop();
        avaOTAUISetStatus(AVA_OTA_UI_ERROR);
        delay(2000);
        avaOTAUIEnd();
        return false;
    }

    if (!Update.end(false))
    {
        Serial.print("[OTA] ERROR: Update.end failed: ");
        Serial.println(Update.getError());
        Update.printError(Serial);

        http.end();
        client.stop();
        avaOTAUISetStatus(AVA_OTA_UI_ERROR);
        delay(2000);
        avaOTAUIEnd();
        return false;
    }

    http.end();
    client.stop();

    Serial.println("[OTA] Firmware written successfully.");

    esp_err_t bootResult =
        esp_ota_set_boot_partition(targetPartition);

    if (bootResult != ESP_OK)
    {
        Serial.print("[OTA] ERROR: Could not set boot partition: ");
        Serial.println(esp_err_to_name(bootResult));

        avaOTAUISetStatus(AVA_OTA_UI_ERROR);
        delay(2000);
        avaOTAUIEnd();
        return false;
    }

    const esp_partition_t* bootPartition =
        esp_ota_get_boot_partition();

    if (bootPartition == nullptr ||
        bootPartition->address != targetPartition->address)
    {
        Serial.println("[OTA] ERROR: Boot partition verification failed.");
        esp_ota_set_boot_partition(runningPartition);

        avaOTAUISetStatus(AVA_OTA_UI_ERROR);
        delay(2000);
        avaOTAUIEnd();
        return false;
    }

    avaOTAUpdateAvailable = false;

    avaOTAUISetProgress(100);
    avaOTAUISetStatus(AVA_OTA_UI_RESTARTING);

    Serial.println();
    Serial.println("=========== AVA OTA SUCCESS ==========");
    Serial.print("[OTA] Installed build: ");
    Serial.println(avaOTARemoteBuild);
    Serial.println("[OTA] SHA-256 verified.");
    Serial.println("[OTA] Boot partition activated.");
    Serial.println("[OTA] Restarting AVA...");
    Serial.println("=======================================");

    delay(1000);
    ESP.restart();

    return true;
}

bool avaOTAUpdate()
{
    if (!avaOTACheckForUpdate())
        return false;

    if (!avaOTAUpdateAvailable)
        return true;

    return avaOTAInstallUpdate();
}

static void avaOTATask(void* parameter)
{
    (void)parameter;

    Serial.println("[OTA] OTA task started.");

    while (true)
    {
        avaOTARunning = true;
        avaOTAUpdate();
        avaOTARunning = false;

        vTaskDelay(pdMS_TO_TICKS(300000));
    }
}

void avaOTAStartTask()
{
    if (avaOTATaskStarted)
        return;

    avaOTATaskStarted = true;
    avaOTARunning = true;

    BaseType_t result = xTaskCreatePinnedToCore(
        avaOTATask,
        "AVA_OTA",
        16384,
        nullptr,
        1,
        &avaOTATaskHandle,
        0
    );

    if (result != pdPASS)
    {
        avaOTATaskStarted = false;
        avaOTARunning = false;
        avaOTATaskHandle = nullptr;

        Serial.println(
            "[OTA] ERROR: Failed to create OTA task."
        );
        return;
    }

    Serial.println("[OTA] OTA task scheduled on Core 0.");
}

bool avaOTAIsRunning()
{
    return avaOTARunning;
}
