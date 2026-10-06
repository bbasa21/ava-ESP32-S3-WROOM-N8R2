#include "Ava_OTA.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <mbedtls/sha256.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_heap_caps.h>
#include <esp_partition.h>
#include <esp_ota_ops.h>

#include "Ava_NetworkLock.h"

#include "Ava_BuildInfo.h"
#include "Ava_OTA_Display.h"

// Stable manifest location. The release workflow updates this file
// after publishing each firmware release.
static const char* AVA_OTA_MANIFEST_URL =
    "https://raw.githubusercontent.com/bbasa21/ava-ESP32-S3-WROOM-N8R2/main/ota/manifest.json";

static bool avaOTAAlreadyChecked = false;
static bool avaOTATaskStarted = false;
static TaskHandle_t avaOTATaskHandle = nullptr;
static volatile bool avaOTARunning = false;

static void avaOTATask(void* parameter)
{
    (void)parameter;

    Serial.println("[OTA] Dedicated OTA task started.");

    while (true)
    {
        avaOTARunning = true;
        avaOTAUpdate();
        avaOTARunning = false;

        vTaskDelay(pdMS_TO_TICKS(300000));
    }
}

bool avaOTAIsRunning()
{
    return avaOTARunning;
}

void avaOTAStartTask()
{
    if (avaOTATaskStarted)
    {
        return;
    }

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

        Serial.println("[OTA] ERROR: Failed to create OTA task.");
        return;
    }

    Serial.println("[OTA] OTA task scheduled on Core 0.");
}

static bool avaOTAExtractString(
    const String& json,
    const char* key,
    String& value
)
{
    String searchKey = String("\"") + key + "\":";
    int start = json.indexOf(searchKey);

    if (start < 0)
    {
        return false;
    }

    start += searchKey.length();

    while (
        start < json.length() &&
        (
            json[start] == ' ' ||
            json[start] == '\t' ||
            json[start] == '\r' ||
            json[start] == '\n'
        )
    )
    {
        start++;
    }

    if (
        start >= json.length() ||
        json[start] != '"'
    )
    {
        return false;
    }

    start++;

    int end = json.indexOf("\"", start);

    if (end < 0)
    {
        return false;
    }

    value = json.substring(start, end);
    return true;
}

static bool avaOTAExtractInt(
    const String& json,
    const char* key,
    uint32_t& value
)
{
    String searchKey = String("\"") + key + "\":";
    int start = json.indexOf(searchKey);

    if (start < 0)
    {
        return false;
    }

    start += searchKey.length();

    while (
        start < json.length() &&
        (
            json[start] == ' ' ||
            json[start] == '\t' ||
            json[start] == '\r' ||
            json[start] == '\n'
        )
    )
    {
        start++;
    }

    int end = start;

    while (
        end < json.length() &&
        json[end] >= '0' &&
        json[end] <= '9'
    )
    {
        end++;
    }

    if (end == start)
    {
        return false;
    }

    value = static_cast<uint32_t>(
        json.substring(start, end).toInt()
    );

    return true;
}

static void avaOTAReportProgress(
    size_t written,
    size_t total
)
{
    if (total == 0)
    {
        return;
    }

    static int lastPercent = -1;

    int percent = static_cast<int>(
        (written * 100ULL) / total
    );

    if (percent > 100)
    {
        percent = 100;
    }

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

    if (percent == 100)
    {
        lastPercent = -1;
    }
}

bool avaOTAUpdate()
{
    AvaNetworkLockGuard networkLock;

    if (!networkLock.isLocked())
    {
        Serial.println("[OTA] ERROR: Could not acquire network lock.");
        return false;
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[OTA] Skipped: WiFi is not connected.");
        return false;
    }

    Serial.println();
    Serial.println("=========== AVA OTA CHECK ===========");
    Serial.print("[OTA] Local build: ");
    Serial.println(AVA_BUILD_NUMBER);
    Serial.print("[OTA] Manifest: ");
    Serial.println(AVA_OTA_MANIFEST_URL);

    WiFiClientSecure client;
    // Temporary TLS mode matching the existing WiFi HTTPS implementation.
    // Certificate validation can be hardened after OTA is proven stable.
    client.setInsecure();

    HTTPClient http;

    Serial.println("[OTA] Manifest HTTP begin...");

    if (!http.begin(client, AVA_OTA_MANIFEST_URL))
    {
        Serial.println("[OTA] ERROR: Manifest HTTP begin failed.");
        client.stop();
        return false;
    }

    http.setConnectTimeout(5000);
    http.setTimeout(10000);

    Serial.println("[OTA] Manifest HTTP GET...");
    int httpCode = http.GET();

    Serial.print("[OTA] Manifest HTTP code: ");
    Serial.println(httpCode);

    if (httpCode != HTTP_CODE_OK)
    {
        Serial.print("[OTA] ERROR: Manifest HTTP code: ");
        Serial.println(httpCode);
        http.end();
        client.stop();
        return false;
    }

    Serial.println("[OTA] Reading manifest...");
    String manifest = http.getString();
    Serial.print("[OTA] Manifest received: ");
    Serial.print(manifest.length());
    Serial.println(" bytes");

    http.end();
    client.stop();

    uint32_t remoteBuild = 0;
    String firmwareUrl;
    String expectedSha256;

    if (
        !avaOTAExtractInt(
            manifest,
            "build",
            remoteBuild
        ) ||
        !avaOTAExtractString(
            manifest,
            "firmware_url",
            firmwareUrl
        ) ||
        !avaOTAExtractString(
            manifest,
            "sha256",
            expectedSha256
        )
    )
    {
        Serial.println("[OTA] ERROR: Invalid manifest.");
        return false;
    }

    expectedSha256.toLowerCase();

    Serial.print("[OTA] Remote build: ");
    Serial.println(remoteBuild);

    if (remoteBuild <= AVA_BUILD_NUMBER)
    {
        Serial.println("[OTA] No newer firmware.");
        Serial.println("=====================================");
        return false;
    }

    Serial.println("[OTA] New firmware found.");
    avaOTAUIBegin();
    Serial.print("[OTA] Firmware URL: ");
    Serial.println(firmwareUrl);
    Serial.print("[OTA] Expected SHA-256: ");
    Serial.println(expectedSha256);

    WiFiClientSecure firmwareClient;
    firmwareClient.setInsecure();

    HTTPClient firmwareHttp;

    if (!firmwareHttp.begin(firmwareClient, firmwareUrl))
    {
        Serial.println("[OTA] ERROR: Firmware HTTP begin failed.");
        avaOTAUISetStatus(AVA_OTA_UI_ERROR);
        delay(2500);
        avaOTAUIEnd();
        firmwareClient.stop();
        return false;
    }

    firmwareHttp.setConnectTimeout(5000);
    firmwareHttp.setTimeout(15000);
    firmwareHttp.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    // GitHub Releases may need a moment after the HTTPS connection is
    // established before the redirected response stream becomes readable.
    // Keep the exact firmware URL/path; only retry the same GET connection.
    int firmwareCode = -1;
    int firmwareAttempts = 0;

    while (firmwareAttempts < 3)
    {
        firmwareAttempts++;

        Serial.print("[OTA] Firmware HTTP GET attempt ");
        Serial.print(firmwareAttempts);
        Serial.println("/3...");

        firmwareCode = firmwareHttp.GET();

        if (firmwareCode == HTTP_CODE_OK)
        {
            break;
        }

        Serial.print("[OTA] Firmware GET failed: ");
        Serial.println(firmwareCode);

        firmwareHttp.end();
        firmwareClient.stop();

        if (firmwareAttempts < 3)
        {
            delay(1500);

            if (!firmwareHttp.begin(firmwareClient, firmwareUrl))
            {
                Serial.println("[OTA] Firmware HTTP reconnect failed.");
                continue;
            }

            firmwareHttp.setConnectTimeout(5000);
            firmwareHttp.setTimeout(15000);
            firmwareHttp.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        }
    }

    if (firmwareCode != HTTP_CODE_OK)
    {
        Serial.print("[OTA] ERROR: Firmware HTTP code: ");
        Serial.println(firmwareCode);
        firmwareHttp.end();
        firmwareClient.stop();
        avaOTAUISetStatus(AVA_OTA_UI_ERROR);
        delay(2500);
        avaOTAUIEnd();
        return false;
    }

    int contentLength = firmwareHttp.getSize();

    if (contentLength <= 0)
    {
        Serial.println("[OTA] ERROR: Invalid firmware size.");
        avaOTAUISetStatus(AVA_OTA_UI_ERROR);
        delay(2500);
        avaOTAUIEnd();
        firmwareHttp.end();
        firmwareClient.stop();
        return false;
    }

    Serial.print("[OTA] Firmware size: ");
    Serial.print(contentLength);
    Serial.println(" bytes");

    // Stream the firmware directly into the OTA partition in small chunks.
    // Keep SHA-256 verification in our code because the ESP32 UpdateClass
    // available in this build does not provide setSHA256().
    // Register the native UpdateClass progress callback so the OLED
    // progress bar tracks the same writeStream() operation that downloads
    // and flashes the firmware. This does not change the firmware URL or
    // download path.
    Update.onProgress(avaOTAReportProgress);

    if (!Update.begin(static_cast<size_t>(contentLength)))
    {
        Serial.print("[OTA] ERROR: Update.begin failed. Error: ");
        Serial.println(Update.getError());
        avaOTAUISetStatus(AVA_OTA_UI_ERROR);
        delay(2500);
        avaOTAUIEnd();
        firmwareHttp.end();
        firmwareClient.stop();
        return false;
    }

    // IMPORTANT:
    // Capture the exact OTA partition selected by Update.begin().
    // Update.end(true) activates this partition, so querying
    // esp_ota_get_next_update_partition() after end() would return the
    // other OTA slot. Read-back verification must use this saved target.
    const esp_partition_t* otaPartition =
        esp_ota_get_next_update_partition(nullptr);

    // Save the target address/size before Update.end(). After finalization,
    // the OTA boot slot changes, so esp_ota_get_next_update_partition() no
    // longer identifies the image we just wrote.
    const uint32_t otaPartitionAddress =
        otaPartition ? otaPartition->address : 0;
    const size_t otaPartitionSize =
        otaPartition ? otaPartition->size : 0;

    if (otaPartition == nullptr)
    {
        Serial.println("[OTA] ERROR: OTA target partition not found.");
        Update.abort();
        avaOTAUISetStatus(AVA_OTA_UI_ERROR);
        delay(2500);
        avaOTAUIEnd();
        firmwareHttp.end();
        firmwareClient.stop();
        return false;
    }

    if (otaPartition == esp_ota_get_running_partition())
    {
        Serial.println("[OTA] ERROR: OTA target is the running partition.");
        Update.abort();
        avaOTAUISetStatus(AVA_OTA_UI_ERROR);
        delay(2500);
        avaOTAUIEnd();
        firmwareHttp.end();
        firmwareClient.stop();
        return false;
    }

    Serial.print("[OTA] Target OTA partition: ");
    Serial.print(otaPartition->label);
    Serial.print(" @ 0x");
    Serial.println(otaPartitionAddress, HEX);

    // Use Arduino-ESP32's native OTA stream path. It owns the internal
    // buffering and flash-write alignment; do not duplicate that buffering here.
    WiFiClient* stream = firmwareHttp.getStreamPtr();

    if (stream == nullptr)
    {
        Serial.println("[OTA] ERROR: Firmware stream is unavailable.");
        Update.abort();
        avaOTAUISetStatus(AVA_OTA_UI_ERROR);
        delay(2500);
        avaOTAUIEnd();
        firmwareHttp.end();
        firmwareClient.stop();
        return false;
    }

    size_t streamed = Update.writeStream(*stream);

    Serial.print("[OTA] Update.writeStream() wrote: ");
    Serial.print(streamed);
    Serial.print("/");
    Serial.println(contentLength);

    firmwareHttp.end();
    firmwareClient.stop();

    avaOTAUISetStatus(AVA_OTA_UI_VERIFYING);
    avaOTAUISetProgress(100);

    if (streamed != static_cast<size_t>(contentLength))
    {
        Serial.println("[OTA] ERROR: Firmware stream/write failed.");
        Update.abort();
        avaOTAUISetStatus(AVA_OTA_UI_ERROR);
        delay(2500);
        avaOTAUIEnd();
        return false;
    }

    // Finalize UpdateClass before read-back verification so all buffered
    // bytes are physically committed to the OTA partition.
    //
    // IMPORTANT:
    // writeStream() already knows the exact firmware size. Do NOT use
    // Update.end(true) here because "true" allows finalization even when
    // Update's internal progress is incomplete. That can mask a flash-write
    // failure and let us reach the custom SHA-256 read-back with a corrupted
    // image. end(false) forces the Update library to reject an incomplete
    // internal write before the new slot can be activated.
    avaOTAUISetStatus(AVA_OTA_UI_INSTALLING);

    Serial.print("[OTA] Update progress before finalize: ");
    Serial.print(Update.progress());
    Serial.print("/");
    Serial.println(Update.size());

    if (!Update.isFinished())
    {
        Serial.print("[OTA] ERROR: Update internal progress is incomplete: ");
        Serial.print(Update.progress());
        Serial.print("/");
        Serial.println(Update.size());
        Update.abort();
        avaOTAUISetStatus(AVA_OTA_UI_ERROR);
        delay(2500);
        avaOTAUIEnd();
        return false;
    }

    if (!Update.end(false))
    {
        Serial.print("[OTA] ERROR: Update.end failed. Error: ");
        Serial.print(Update.getError());
        Serial.print(" (");
        Update.printError(Serial);
        Serial.println(")");
        avaOTAUISetStatus(AVA_OTA_UI_ERROR);
        delay(2500);
        avaOTAUIEnd();
        return false;
    }

    // Update.end(true) has finalized and activated the target slot.
    // Resolve the boot partition again and verify its address. This avoids
    // relying on a partition pointer across Update.end(), and guarantees
    // that the SHA-256 read-back is performed on the image that will boot.
    const esp_partition_t* verifiedPartition = esp_ota_get_boot_partition();

    if (verifiedPartition == nullptr)
    {
        Serial.println("[OTA] ERROR: Boot partition could not be resolved.");
        esp_ota_set_boot_partition(esp_ota_get_running_partition());
        avaOTAUISetStatus(AVA_OTA_UI_ERROR);
        delay(2500);
        avaOTAUIEnd();
        return false;
    }

    Serial.print("[OTA] OTA partition offset: 0x");
    Serial.println(verifiedPartition->address, HEX);
    Serial.print("[OTA] OTA partition size: ");
    Serial.print(verifiedPartition->size);
    Serial.println(" bytes");

    Serial.print("[OTA] Expected target offset: 0x");
    Serial.println(otaPartitionAddress, HEX);

    if (verifiedPartition->address != otaPartitionAddress)
    {
        Serial.println("[OTA] ERROR: Boot partition is not the partition written by Update.");
        esp_ota_set_boot_partition(esp_ota_get_running_partition());
        avaOTAUISetStatus(AVA_OTA_UI_ERROR);
        delay(2500);
        avaOTAUIEnd();
        return false;
    }

    if (static_cast<size_t>(contentLength) > otaPartitionSize ||
        static_cast<size_t>(contentLength) > verifiedPartition->size)
    {
        Serial.println("[OTA] ERROR: Firmware exceeds OTA partition.");
        esp_ota_set_boot_partition(esp_ota_get_running_partition());
        avaOTAUISetStatus(AVA_OTA_UI_ERROR);
        delay(2500);
        avaOTAUIEnd();
        return false;
    }

    mbedtls_sha256_context flashSha256;
    mbedtls_sha256_init(&flashSha256);
    mbedtls_sha256_starts(&flashSha256, 0);

    uint8_t flashBuffer[4096];
    size_t flashReadOffset = 0;
    bool flashReadOK = true;

    while (flashReadOffset < static_cast<size_t>(contentLength))
    {
        size_t toRead =
            static_cast<size_t>(contentLength) - flashReadOffset;

        if (toRead > sizeof(flashBuffer))
        {
            toRead = sizeof(flashBuffer);
        }

        esp_err_t readResult = esp_partition_read(
            verifiedPartition,
            flashReadOffset,
            flashBuffer,
            toRead
        );

        if (readResult != ESP_OK)
        {
            Serial.print("[OTA] ERROR: Flash read failed: ");
            Serial.println(esp_err_to_name(readResult));
            flashReadOK = false;
            break;
        }

        mbedtls_sha256_update(
            &flashSha256,
            flashBuffer,
            toRead
        );

        flashReadOffset += toRead;
    }

    uint8_t flashDigest[32];

    mbedtls_sha256_finish(
        &flashSha256,
        flashDigest
    );

    mbedtls_sha256_free(&flashSha256);

    const char* hex = "0123456789abcdef";

    String flashSha256Hex;
    flashSha256Hex.reserve(64);

    for (size_t i = 0; i < sizeof(flashDigest); ++i)
    {
        flashSha256Hex += hex[(flashDigest[i] >> 4) & 0x0F];
        flashSha256Hex += hex[flashDigest[i] & 0x0F];
    }

    Serial.print("[OTA] Flash SHA-256: ");
    Serial.println(flashSha256Hex);

    if (!flashReadOK || flashReadOffset != static_cast<size_t>(contentLength))
    {
        Serial.println("[OTA] ERROR: Could not verify OTA partition contents.");
        esp_ota_set_boot_partition(esp_ota_get_running_partition());
        avaOTAUISetStatus(AVA_OTA_UI_ERROR);
        delay(2500);
        avaOTAUIEnd();
        return false;
    }

    if (flashSha256Hex != expectedSha256)
    {
        Serial.println("[OTA] ERROR: Flash SHA-256 mismatch.");
        Serial.println("[OTA] Downloaded image and stored OTA image differ.");
        esp_ota_set_boot_partition(esp_ota_get_running_partition());
        avaOTAUISetStatus(AVA_OTA_UI_ERROR);
        delay(2500);
        avaOTAUIEnd();
        return false;
    }

    Serial.println("[OTA] Flash contents match downloaded firmware.");

    avaOTAUISetStatus(AVA_OTA_UI_RESTARTING);
    avaOTAUISetProgress(100);

    Serial.println("[OTA] Firmware verified and installed.");
    Serial.println("[OTA] Rebooting into the new build...");
    Serial.println("=====================================");

    delay(1000);
    ESP.restart();

    return true;
}