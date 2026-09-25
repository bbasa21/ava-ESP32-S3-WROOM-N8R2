#include "Ava_OTA.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <mbedtls/sha256.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "Ava_BuildInfo.h"

// Stable manifest location. The release workflow updates this file
// after publishing each firmware release.
static const char* AVA_OTA_MANIFEST_URL =
    "https://raw.githubusercontent.com/bbasa21/ava-ESP32-S3-WROOM-N8R2/main/ota/manifest.json";

static bool avaOTAAlreadyChecked = false;
static bool avaOTATaskStarted = false;
static TaskHandle_t avaOTATaskHandle = nullptr;

static void avaOTATask(void* parameter)
{
    (void)parameter;

    Serial.println("[OTA] Dedicated OTA task started.");

    avaOTAUpdate();

    avaOTATaskHandle = nullptr;
    vTaskDelete(nullptr);
}

void avaOTAStartTask()
{
    if (avaOTATaskStarted)
    {
        return;
    }

    avaOTATaskStarted = true;

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
    String searchKey = String("\"") + key + "\":\"";
    int start = json.indexOf(searchKey);

    if (start < 0)
    {
        return false;
    }

    start += searchKey.length();

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
    if (avaOTAAlreadyChecked)
    {
        return false;
    }

    avaOTAAlreadyChecked = true;

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

    if (!http.begin(client, AVA_OTA_MANIFEST_URL))
    {
        Serial.println("[OTA] ERROR: Manifest HTTP begin failed.");
        client.stop();
        return false;
    }

    http.setConnectTimeout(5000);
    http.setTimeout(10000);

    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK)
    {
        Serial.print("[OTA] ERROR: Manifest HTTP code: ");
        Serial.println(httpCode);
        http.end();
        client.stop();
        return false;
    }

    String manifest = http.getString();

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
        firmwareClient.stop();
        return false;
    }

    firmwareHttp.setConnectTimeout(5000);
    firmwareHttp.setTimeout(15000);

    int firmwareCode = firmwareHttp.GET();

    if (firmwareCode != HTTP_CODE_OK)
    {
        Serial.print("[OTA] ERROR: Firmware HTTP code: ");
        Serial.println(firmwareCode);
        firmwareHttp.end();
        firmwareClient.stop();
        return false;
    }

    int contentLength = firmwareHttp.getSize();

    if (contentLength <= 0)
    {
        Serial.println("[OTA] ERROR: Invalid firmware size.");
        firmwareHttp.end();
        firmwareClient.stop();
        return false;
    }

    Serial.print("[OTA] Firmware size: ");
    Serial.print(contentLength);
    Serial.println(" bytes");

    if (!Update.begin(static_cast<size_t>(contentLength)))
    {
        Serial.print("[OTA] ERROR: Update.begin failed. Error: ");
        Serial.println(Update.getError());
        firmwareHttp.end();
        firmwareClient.stop();
        return false;
    }

    mbedtls_sha256_context sha256;
    mbedtls_sha256_init(&sha256);

    mbedtls_sha256_starts(&sha256, 0);

    uint8_t buffer[4096];
    size_t written = 0;
    bool transferOK = true;

    WiFiClient* stream = firmwareHttp.getStreamPtr();

    while (
        stream->connected() &&
        written < static_cast<size_t>(contentLength)
    )
    {
        size_t available = stream->available();

        if (available == 0)
        {
            delay(1);
            continue;
        }

        size_t toRead = available;

        if (toRead > sizeof(buffer))
        {
            toRead = sizeof(buffer);
        }

        size_t remaining =
            static_cast<size_t>(contentLength) - written;

        if (toRead > remaining)
        {
            toRead = remaining;
        }

        size_t readBytes =
            stream->readBytes(
                buffer,
                toRead
            );

        if (readBytes == 0)
        {
            transferOK = false;
            break;
        }

        size_t updateBytes =
            Update.write(
                buffer,
                readBytes
            );

        if (updateBytes != readBytes)
        {
            Serial.println("[OTA] ERROR: Flash write failed.");
            transferOK = false;
            break;
        }

        mbedtls_sha256_update(
            &sha256,
            buffer,
            readBytes
        );

        written += readBytes;

        avaOTAReportProgress(
            written,
            static_cast<size_t>(contentLength)
        );
    }

    uint8_t digest[32];

    mbedtls_sha256_finish(
        &sha256,
        digest
    );

    bool hashOK = true;

    mbedtls_sha256_free(&sha256);

    firmwareHttp.end();
    firmwareClient.stop();

    if (
        !transferOK ||
        written != static_cast<size_t>(contentLength) ||
        !hashOK
    )
    {
        Serial.println("[OTA] ERROR: Firmware transfer failed.");
        Update.abort();
        return false;
    }

    String actualSha256;

    for (uint8_t i = 0; i < sizeof(digest); ++i)
    {
        if (digest[i] < 0x10)
        {
            actualSha256 += "0";
        }

        actualSha256 += String(
            digest[i],
            HEX
        );
    }

    actualSha256.toLowerCase();

    Serial.print("[OTA] Actual SHA-256: ");
    Serial.println(actualSha256);

    if (actualSha256 != expectedSha256)
    {
        Serial.println("[OTA] ERROR: SHA-256 mismatch.");
        Update.abort();
        return false;
    }

    if (!Update.end(true))
    {
        Serial.print("[OTA] ERROR: Update.end failed. Error: ");
        Serial.println(Update.getError());
        return false;
    }

    if (!Update.isFinished())
    {
        Serial.println("[OTA] ERROR: Update is not finished.");
        return false;
    }

    Serial.println("[OTA] Firmware verified and installed.");
    Serial.println("[OTA] Rebooting into the new build...");
    Serial.println("=====================================");

    delay(1000);
    ESP.restart();

    return true;
}
