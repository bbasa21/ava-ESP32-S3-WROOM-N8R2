#include "Ava_OTA_V3.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <esp_arduino_version.h>
#include <esp_crt_bundle.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <mbedtls/sha256.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <stdio.h>
#include <string.h>

#include "Ava_BuildInfo.h"
#include "Ava_NetworkLock.h"
#include "Ava_OTA_V3_Manifest.h"
#include "Ava_OTA_V3_BootGuard.h"

#if ESP_ARDUINO_VERSION_MAJOR < 3
extern "C" const uint8_t x509_crt_bundle[];
#endif

namespace {
#if defined(AVA_OTA_V3_TEST_CHANNEL)
constexpr char MANIFEST_URL[] =
    "https://raw.githubusercontent.com/bbasa21/ava-ESP32-S3-WROOM-N8R2/feature/ota-v3/ota/v3-test-manifest.json";
constexpr char SIGNATURE_URL[] =
    "https://raw.githubusercontent.com/bbasa21/ava-ESP32-S3-WROOM-N8R2/feature/ota-v3/ota/v3-test-manifest.sig";
constexpr char RELEASE_TAG_PREFIX[] = "ota-v3-test-";
#else
constexpr char MANIFEST_URL[] =
    "https://raw.githubusercontent.com/bbasa21/ava-ESP32-S3-WROOM-N8R2/main/ota/manifest.json";
constexpr char SIGNATURE_URL[] =
    "https://raw.githubusercontent.com/bbasa21/ava-ESP32-S3-WROOM-N8R2/main/ota/manifest.sig";
constexpr char RELEASE_TAG_PREFIX[] = "firmware-";
#endif
constexpr size_t OTA_CHUNK_SIZE = 4096;
constexpr uint32_t NETWORK_LOCK_TIMEOUT_MS = 12000;
constexpr uint32_t HTTP_CONNECT_TIMEOUT_MS = 12000;
constexpr uint32_t HTTP_READ_TIMEOUT_MS = 20000;
constexpr uint32_t AUTO_CHECK_INTERVAL_MS = 300000UL;
constexpr UBaseType_t COMMAND_QUEUE_LENGTH = 4;
constexpr uint32_t OTA_WORKER_STACK_SIZE = 16384;

class AvaOTASecureClient : public WiFiClientSecure {
public:
    AvaOTASecureClient() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
        useBuiltinCACertBundle();
#else
        setCACertBundle(x509_crt_bundle);
#endif
    }
};

enum WorkerCommand : uint8_t { WORKER_CHECK = 1, WORKER_INSTALL = 2 };

StaticQueue_t commandQueueStorage;
uint8_t commandQueueBytes[COMMAND_QUEUE_LENGTH * sizeof(WorkerCommand)];
QueueHandle_t commandQueue = nullptr;
TaskHandle_t workerTaskHandle = nullptr;
bool managerInitialized = false;
bool autoClockStarted = false;
uint32_t lastAutoCheckMs = 0;
bool hasVerifiedManifest = false;
AvaOTAV3Manifest lastVerifiedManifest = {};

portMUX_TYPE statusMux = portMUX_INITIALIZER_UNLOCKED;
AvaOTAV3Status statusSnapshot = {
    AVA_OTA_V3_IDLE, AVA_OTA_V3_ERR_NONE, 0, AVA_BUILD_NUMBER, 0, 0, 0, "READY"
};

void setState(AvaOTAV3State state, const char* detail,
              AvaOTAV3Error error = AVA_OTA_V3_ERR_NONE) {
    portENTER_CRITICAL(&statusMux);
    statusSnapshot.state = state;
    statusSnapshot.error = error;
    if (detail) {
        strncpy(statusSnapshot.detail, detail, sizeof(statusSnapshot.detail) - 1);
        statusSnapshot.detail[sizeof(statusSnapshot.detail) - 1] = '\0';
    }
    if (state == AVA_OTA_V3_CHECK_QUEUED || state == AVA_OTA_V3_CHECKING ||
        state == AVA_OTA_V3_INSTALL_QUEUED || state == AVA_OTA_V3_DOWNLOADING ||
        state == AVA_OTA_V3_VERIFYING_IMAGE) {
        statusSnapshot.progress = 0;
    }
    portEXIT_CRITICAL(&statusMux);
}

void setProgress(uint32_t done, uint32_t total) {
    portENTER_CRITICAL(&statusMux);
    statusSnapshot.bytesTransferred = done;
    statusSnapshot.totalBytes = total;
    statusSnapshot.progress = total
        ? static_cast<uint8_t>((static_cast<uint64_t>(done) * 100ULL) / total)
        : 0;
    portEXIT_CRITICAL(&statusMux);
}

void fail(AvaOTAV3Error error, const char* detail) {
    Serial.printf("[OTA3] FAILED (%s): %s\n", avaOTAV3ErrorName(error), detail ? detail : "");
    setState(AVA_OTA_V3_FAILED, detail, error);
}

bool fetchText(const char* url, size_t maximumLength,
               AvaOTAV3Error httpError, AvaOTAV3Error sizeError,
               String& output, AvaOTAV3Error& error) {
    AvaOTASecureClient client;
    HTTPClient http;
    http.setConnectTimeout(HTTP_CONNECT_TIMEOUT_MS);
    http.setTimeout(HTTP_READ_TIMEOUT_MS);
    http.setReuse(false);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    if (!http.begin(client, url)) {
        error = httpError;
        return false;
    }
    // HTTPClient::begin() resets request headers; add cache policy afterwards.
    http.addHeader("Cache-Control", "no-cache");
    const int code = http.GET();
    const int length = http.getSize();
    if (code != HTTP_CODE_OK) {
        http.end();
        error = httpError;
        return false;
    }
    if (length <= 0 || static_cast<size_t>(length) > maximumLength) {
        http.end();
        error = sizeError;
        return false;
    }

    output = http.getString();
    http.end();
    if (output.length() != static_cast<unsigned int>(length) ||
        output.length() > maximumLength) {
        error = sizeError;
        return false;
    }
    error = AVA_OTA_V3_ERR_NONE;
    return true;
}

bool fetchVerifiedManifest(AvaOTAV3Manifest& manifest, AvaOTAV3Error& error) {
    String body;
    if (!fetchText(MANIFEST_URL, 2048,
                   AVA_OTA_V3_ERR_MANIFEST_HTTP, AVA_OTA_V3_ERR_MANIFEST_SIZE,
                   body, error)) return false;

    String signature;
    if (!fetchText(SIGNATURE_URL, 256,
                   AVA_OTA_V3_ERR_SIGNATURE_HTTP, AVA_OTA_V3_ERR_SIGNATURE_HTTP,
                   signature, error)) return false;
    signature.trim();

    if (!avaOTAV3VerifyAndParseManifest(
            reinterpret_cast<const uint8_t*>(body.c_str()),
            body.length(), signature.c_str(), manifest, error)) return false;

    const String allowed =
        String("https://github.com/bbasa21/ava-ESP32-S3-WROOM-N8R2/releases/download/") +
        RELEASE_TAG_PREFIX + String(manifest.build) + "/ava-firmware.bin";
    if (allowed != manifest.firmwareUrl) {
        error = AVA_OTA_V3_ERR_URL_REJECTED;
        return false;
    }
    error = AVA_OTA_V3_ERR_NONE;
    return true;
}

String digestToHex(const uint8_t* digest) {
    static const char hex[] = "0123456789abcdef";
    String result;
    result.reserve(64);
    for (size_t i = 0; i < 32; ++i) {
        result += hex[digest[i] >> 4];
        result += hex[digest[i] & 0x0f];
    }
    return result;
}

bool hashPartition(const esp_partition_t* partition, size_t length, String& output) {
    if (!partition || length == 0 || length > partition->size) return false;
    uint8_t buffer[OTA_CHUNK_SIZE];
    uint8_t digest[32];
    mbedtls_sha256_context sha;
    mbedtls_sha256_init(&sha);
    if (mbedtls_sha256_starts(&sha, 0) != 0) {
        mbedtls_sha256_free(&sha);
        return false;
    }

    bool ok = true;
    for (size_t offset = 0; offset < length;) {
        const size_t chunk = min(sizeof(buffer), length - offset);
        if (esp_partition_read(partition, offset, buffer, chunk) != ESP_OK ||
            mbedtls_sha256_update(&sha, buffer, chunk) != 0) {
            ok = false;
            break;
        }
        offset += chunk;
        vTaskDelay(1);
    }
    if (ok) ok = mbedtls_sha256_finish(&sha, digest) == 0;
    mbedtls_sha256_free(&sha);
    if (!ok) return false;
    output = digestToHex(digest);
    return true;
}

bool sameManifest(const AvaOTAV3Manifest& a, const AvaOTAV3Manifest& b) {
    return a.build == b.build && a.size == b.size &&
        strcmp(a.version, b.version) == 0 &&
        strcmp(a.commit, b.commit) == 0 &&
        strcmp(a.firmwareUrl, b.firmwareUrl) == 0 &&
        strcmp(a.sha256, b.sha256) == 0;
}

void performCheck() {
    setState(AVA_OTA_V3_CHECKING, "VERIFYING RELEASE");
    if (WiFi.status() != WL_CONNECTED) {
        fail(AVA_OTA_V3_ERR_WIFI, "WI-FI DISCONNECTED");
        return;
    }

    AvaNetworkLockGuard networkLock(pdMS_TO_TICKS(NETWORK_LOCK_TIMEOUT_MS));
    if (!networkLock.isLocked()) {
        fail(AVA_OTA_V3_ERR_NETWORK_LOCK, "NETWORK BUSY");
        return;
    }

    AvaOTAV3Manifest manifest = {};
    AvaOTAV3Error error = AVA_OTA_V3_ERR_NONE;
    if (!fetchVerifiedManifest(manifest, error)) {
        const char* detail =
            error == AVA_OTA_V3_ERR_SIGNING_KEY_NOT_CONFIGURED ? "SIGNING KEY NOT CONFIGURED" :
            error == AVA_OTA_V3_ERR_SIGNATURE_INVALID ? "SIGNATURE REJECTED" :
            error == AVA_OTA_V3_ERR_URL_REJECTED ? "FIRMWARE URL REJECTED" :
            "MANIFEST CHECK FAILED";
        fail(error, detail);
        return;
    }

    lastVerifiedManifest = manifest;
    hasVerifiedManifest = true;

    portENTER_CRITICAL(&statusMux);
    statusSnapshot.availableBuild = manifest.build;
    statusSnapshot.totalBytes = manifest.size;
    statusSnapshot.bytesTransferred = 0;
    statusSnapshot.progress = manifest.build > AVA_BUILD_NUMBER ? 0 : 100;
    portEXIT_CRITICAL(&statusMux);

    if (manifest.build <= AVA_BUILD_NUMBER) {
        setState(AVA_OTA_V3_UP_TO_DATE, "FIRMWARE IS CURRENT");
        Serial.printf("[OTA3] Up to date. Local=%u Remote=%lu\n",
            static_cast<unsigned>(AVA_BUILD_NUMBER),
            static_cast<unsigned long>(manifest.build));
        return;
    }

    setState(AVA_OTA_V3_UPDATE_AVAILABLE, "SIGNED UPDATE READY");
    Serial.printf("[OTA3] Signed update ready: build %lu, %lu bytes.\n",
        static_cast<unsigned long>(manifest.build),
        static_cast<unsigned long>(manifest.size));
}

void performInstall() {
    if (!hasVerifiedManifest) {
        fail(AVA_OTA_V3_ERR_MANIFEST_CHANGED, "RUN OTA CHECK FIRST");
        return;
    }
    if (WiFi.status() != WL_CONNECTED) {
        fail(AVA_OTA_V3_ERR_WIFI, "WI-FI DISCONNECTED");
        return;
    }

    AvaNetworkLockGuard networkLock(pdMS_TO_TICKS(NETWORK_LOCK_TIMEOUT_MS));
    if (!networkLock.isLocked()) {
        fail(AVA_OTA_V3_ERR_NETWORK_LOCK, "NETWORK BUSY");
        return;
    }

    // Re-fetch and re-verify immediately before writing flash.
    AvaOTAV3Manifest manifest = {};
    AvaOTAV3Error error = AVA_OTA_V3_ERR_NONE;
    if (!fetchVerifiedManifest(manifest, error)) {
        fail(error, "RE-VALIDATION FAILED");
        return;
    }
    if (!sameManifest(manifest, lastVerifiedManifest) ||
        manifest.build <= AVA_BUILD_NUMBER) {
        fail(AVA_OTA_V3_ERR_MANIFEST_CHANGED, "RE-CHECK REQUIRED");
        hasVerifiedManifest = false;
        return;
    }

    const esp_partition_t* target = esp_ota_get_next_update_partition(nullptr);
    const esp_partition_t* running = esp_ota_get_running_partition();
    if (!target || !running || target->address == running->address ||
        target->subtype == running->subtype || manifest.size > target->size) {
        fail(AVA_OTA_V3_ERR_PARTITION, "INACTIVE SLOT NOT AVAILABLE");
        return;
    }

    AvaOTASecureClient client;
    HTTPClient http;
    http.setConnectTimeout(HTTP_CONNECT_TIMEOUT_MS);
    http.setTimeout(HTTP_READ_TIMEOUT_MS);
    http.setReuse(false);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    if (!http.begin(client, manifest.firmwareUrl)) {
        fail(AVA_OTA_V3_ERR_IMAGE_HTTP, "FIRMWARE REQUEST FAILED");
        return;
    }

    const int code = http.GET();
    const int contentLength = http.getSize();
    if (code != HTTP_CODE_OK) {
        http.end();
        fail(AVA_OTA_V3_ERR_IMAGE_HTTP, "FIRMWARE HTTP ERROR");
        return;
    }
    if (contentLength <= 0 ||
        static_cast<uint32_t>(contentLength) != manifest.size ||
        static_cast<size_t>(contentLength) > target->size) {
        http.end();
        fail(AVA_OTA_V3_ERR_IMAGE_SIZE, "FIRMWARE SIZE MISMATCH");
        return;
    }

    esp_ota_handle_t otaHandle = 0;
    if (esp_ota_begin(target, manifest.size, &otaHandle) != ESP_OK) {
        http.end();
        fail(AVA_OTA_V3_ERR_OTA_BEGIN, "FLASH WRITE SESSION FAILED");
        return;
    }

    setState(AVA_OTA_V3_DOWNLOADING, "DOWNLOADING SIGNED IMAGE");
    setProgress(0, manifest.size);
    WiFiClient* stream = http.getStreamPtr();
    uint8_t buffer[OTA_CHUNK_SIZE];
    uint8_t digest[32];
    size_t written = 0;
    uint32_t lastDataAt = millis();
    bool ok = stream != nullptr;

    mbedtls_sha256_context sha;
    mbedtls_sha256_init(&sha);
    if (mbedtls_sha256_starts(&sha, 0) != 0) ok = false;

    while (ok && written < manifest.size) {
        const int available = stream->available();
        if (available <= 0) {
            if (!stream->connected() ||
                static_cast<uint32_t>(millis() - lastDataAt) > HTTP_READ_TIMEOUT_MS) {
                ok = false;
                error = AVA_OTA_V3_ERR_DOWNLOAD_TIMEOUT;
                break;
            }
            vTaskDelay(1);
            continue;
        }

        size_t wanted = sizeof(buffer);
        const size_t remaining = static_cast<size_t>(manifest.size) - written;
        if (wanted > remaining) wanted = remaining;
        const size_t availableBytes = static_cast<size_t>(available);
        if (wanted > availableBytes) wanted = availableBytes;
        const int received = stream->read(buffer, wanted);
        if (received <= 0) {
            vTaskDelay(1);
            continue;
        }

        lastDataAt = millis();
        if (esp_ota_write(otaHandle, buffer, static_cast<size_t>(received)) != ESP_OK) {
            ok = false;
            error = AVA_OTA_V3_ERR_OTA_WRITE;
            break;
        }
        if (mbedtls_sha256_update(&sha, buffer, static_cast<size_t>(received)) != 0) {
            ok = false;
            error = AVA_OTA_V3_ERR_HASH_MISMATCH;
            break;
        }

        written += static_cast<size_t>(received);
        setProgress(static_cast<uint32_t>(written), manifest.size);
        vTaskDelay(1);
    }

    if (ok && written == manifest.size) {
        ok = mbedtls_sha256_finish(&sha, digest) == 0;
    } else {
        ok = false;
    }
    mbedtls_sha256_free(&sha);
    http.end();

    if (!ok || written != manifest.size) {
        esp_ota_abort(otaHandle);
        if (error == AVA_OTA_V3_ERR_NONE) error = AVA_OTA_V3_ERR_DOWNLOAD_TIMEOUT;
        fail(error, "DOWNLOAD ABORTED; ACTIVE SLOT PRESERVED");
        return;
    }

    setState(AVA_OTA_V3_VERIFYING_IMAGE, "CHECKING SHA-256");
    if (digestToHex(digest) != manifest.sha256) {
        esp_ota_abort(otaHandle);
        fail(AVA_OTA_V3_ERR_HASH_MISMATCH, "IMAGE HASH DOES NOT MATCH");
        return;
    }

    // esp_ota_end validates the ESP image header and image structure.
    if (esp_ota_end(otaHandle) != ESP_OK) {
        fail(AVA_OTA_V3_ERR_IMAGE_INVALID, "ESP IMAGE VALIDATION FAILED");
        return;
    }

    String readbackHash;
    if (!hashPartition(target, manifest.size, readbackHash) ||
        readbackHash != manifest.sha256) {
        fail(AVA_OTA_V3_ERR_READBACK, "FLASH READBACK FAILED");
        return;
    }

    setState(AVA_OTA_V3_READY_TO_BOOT, "IMAGE VERIFIED");
    if (esp_ota_set_boot_partition(target) != ESP_OK) {
        fail(AVA_OTA_V3_ERR_SET_BOOT_PARTITION, "BOOT SLOT NOT CHANGED");
        return;
    }

    hasVerifiedManifest = false;
    setProgress(manifest.size, manifest.size);
    setState(AVA_OTA_V3_REBOOT_PENDING, "RESTARTING SAFELY");
    Serial.printf("[OTA3] Build %lu verified; selecting inactive slot and restarting.\n",
        static_cast<unsigned long>(manifest.build));
    vTaskDelay(pdMS_TO_TICKS(1200));
    ESP.restart();
}

void otaWorker(void*) {
    WorkerCommand command;
    for (;;) {
        if (xQueueReceive(commandQueue, &command, portMAX_DELAY) != pdTRUE) continue;
        if (command == WORKER_CHECK) performCheck();
        else if (command == WORKER_INSTALL) performInstall();
    }
}

bool isBusyState(AvaOTAV3State state) {
    return state == AVA_OTA_V3_CHECK_QUEUED ||
           state == AVA_OTA_V3_CHECKING ||
           state == AVA_OTA_V3_INSTALL_QUEUED ||
           state == AVA_OTA_V3_DOWNLOADING ||
           state == AVA_OTA_V3_VERIFYING_IMAGE ||
           state == AVA_OTA_V3_READY_TO_BOOT ||
           state == AVA_OTA_V3_REBOOT_PENDING ||
           state == AVA_OTA_V3_BOOT_VERIFYING;
}
} // namespace

void avaOTAV3Begin() {
    if (managerInitialized) return;
    commandQueue = xQueueCreateStatic(
        COMMAND_QUEUE_LENGTH, sizeof(WorkerCommand),
        commandQueueBytes, &commandQueueStorage
    );
    if (!commandQueue) {
        fail(AVA_OTA_V3_ERR_WORKER_CREATE, "COMMAND QUEUE FAILED");
        return;
    }
    if (xTaskCreate(
            otaWorker, "AVA_OTA_V3", OTA_WORKER_STACK_SIZE,
            nullptr, 1, &workerTaskHandle
        ) != pdPASS) {
        workerTaskHandle = nullptr;
        fail(AVA_OTA_V3_ERR_WORKER_CREATE, "WORKER TASK FAILED");
        return;
    }
    managerInitialized = true;
    setState(AVA_OTA_V3_IDLE, "OTA V3 READY");
    Serial.println("[OTA3] Single-owner worker ready.");
}

bool avaOTAV3RequestCheck() {
    if (!managerInitialized || !commandQueue) {
        fail(AVA_OTA_V3_ERR_NOT_INITIALIZED, "OTA MANAGER NOT READY");
        return false;
    }
    if (isBusyState(avaOTAV3GetStatus().state)) return false;

    setState(AVA_OTA_V3_CHECK_QUEUED, "CHECK QUEUED");
    const WorkerCommand command = WORKER_CHECK;
    if (xQueueSend(commandQueue, &command, 0) != pdTRUE) {
        fail(AVA_OTA_V3_ERR_BUSY, "COMMAND QUEUE FULL");
        return false;
    }
    Serial.println("[OTA3] Check queued.");
    return true;
}

bool avaOTAV3RequestInstall() {
    if (!managerInitialized || !commandQueue) {
        fail(AVA_OTA_V3_ERR_NOT_INITIALIZED, "OTA MANAGER NOT READY");
        return false;
    }
    if (avaOTAV3GetStatus().state != AVA_OTA_V3_UPDATE_AVAILABLE) return false;

    setState(AVA_OTA_V3_INSTALL_QUEUED, "INSTALL QUEUED");
    const WorkerCommand command = WORKER_INSTALL;
    if (xQueueSend(commandQueue, &command, 0) != pdTRUE) {
        fail(AVA_OTA_V3_ERR_BUSY, "COMMAND QUEUE FULL");
        return false;
    }
    Serial.println("[OTA3] Install queued.");
    return true;
}

bool avaOTAV3IsBusy() {
    return isBusyState(avaOTAV3GetStatus().state);
}

AvaOTAV3Status avaOTAV3GetStatus() {
    AvaOTAV3Status copy;
    portENTER_CRITICAL(&statusMux);
    copy = statusSnapshot;
    portEXIT_CRITICAL(&statusMux);
    return copy;
}

void avaOTAV3Tick() {
    avaOTAV3BootGuardTick();
    const uint32_t now = millis();
    if (!autoClockStarted) {
        lastAutoCheckMs = now;
        autoClockStarted = true;
        return;
    }
    if (static_cast<uint32_t>(now - lastAutoCheckMs) < AUTO_CHECK_INTERVAL_MS) return;

    const AvaOTAV3Status current = avaOTAV3GetStatus();
    if (WiFi.status() != WL_CONNECTED || isBusyState(current.state) ||
        current.state == AVA_OTA_V3_UPDATE_AVAILABLE) return;

    if (avaOTAV3RequestCheck()) {
        lastAutoCheckMs = now;
        Serial.println("[OTA3] Low-priority scheduled check queued.");
    }
}

void avaOTAV3BootVerificationStarted() {
    setState(AVA_OTA_V3_BOOT_VERIFYING, "FIRST BOOT HEALTH CHECK");
    Serial.println("[OTA3] New image is pending verification.");
}
void avaOTAV3BootVerificationConfirmed() {
    setState(AVA_OTA_V3_BOOT_VALIDATED, "NEW FIRMWARE CONFIRMED");
    Serial.println("[OTA3] Boot health confirmed; rollback protection released.");
}
void avaOTAV3BootVerificationFailed() {
    setState(AVA_OTA_V3_FAILED, "BOOT HEALTH CHECK FAILED",
             AVA_OTA_V3_ERR_BOOT_VALIDATION);
    Serial.println("[OTA3] Boot health validation failed.");
}

const char* avaOTAV3StateName(AvaOTAV3State state) {
    switch (state) {
        case AVA_OTA_V3_IDLE: return "IDLE";
        case AVA_OTA_V3_CHECK_QUEUED: return "CHECK_QUEUED";
        case AVA_OTA_V3_CHECKING: return "CHECKING";
        case AVA_OTA_V3_UPDATE_AVAILABLE: return "UPDATE_AVAILABLE";
        case AVA_OTA_V3_UP_TO_DATE: return "UP_TO_DATE";
        case AVA_OTA_V3_INSTALL_QUEUED: return "INSTALL_QUEUED";
        case AVA_OTA_V3_DOWNLOADING: return "DOWNLOADING";
        case AVA_OTA_V3_VERIFYING_IMAGE: return "VERIFYING_IMAGE";
        case AVA_OTA_V3_READY_TO_BOOT: return "READY_TO_BOOT";
        case AVA_OTA_V3_REBOOT_PENDING: return "REBOOT_PENDING";
        case AVA_OTA_V3_BOOT_VERIFYING: return "BOOT_VERIFYING";
        case AVA_OTA_V3_BOOT_VALIDATED: return "BOOT_VALIDATED";
        case AVA_OTA_V3_FAILED: return "FAILED";
        default: return "UNKNOWN";
    }
}

const char* avaOTAV3ErrorName(AvaOTAV3Error error) {
    switch (error) {
        case AVA_OTA_V3_ERR_NONE: return "NONE";
        case AVA_OTA_V3_ERR_NOT_INITIALIZED: return "NOT_INITIALIZED";
        case AVA_OTA_V3_ERR_WORKER_CREATE: return "WORKER_CREATE";
        case AVA_OTA_V3_ERR_BUSY: return "BUSY";
        case AVA_OTA_V3_ERR_WIFI: return "WIFI";
        case AVA_OTA_V3_ERR_NETWORK_LOCK: return "NETWORK_LOCK";
        case AVA_OTA_V3_ERR_MANIFEST_HTTP: return "MANIFEST_HTTP";
        case AVA_OTA_V3_ERR_MANIFEST_SIZE: return "MANIFEST_SIZE";
        case AVA_OTA_V3_ERR_SIGNATURE_HTTP: return "SIGNATURE_HTTP";
        case AVA_OTA_V3_ERR_SIGNING_KEY_NOT_CONFIGURED: return "SIGNING_KEY_NOT_CONFIGURED";
        case AVA_OTA_V3_ERR_SIGNATURE_INVALID: return "SIGNATURE_INVALID";
        case AVA_OTA_V3_ERR_MANIFEST_SCHEMA: return "MANIFEST_SCHEMA";
        case AVA_OTA_V3_ERR_MANIFEST_CHANGED: return "MANIFEST_CHANGED";
        case AVA_OTA_V3_ERR_URL_REJECTED: return "URL_REJECTED";
        case AVA_OTA_V3_ERR_IMAGE_HTTP: return "IMAGE_HTTP";
        case AVA_OTA_V3_ERR_IMAGE_SIZE: return "IMAGE_SIZE";
        case AVA_OTA_V3_ERR_PARTITION: return "PARTITION";
        case AVA_OTA_V3_ERR_OTA_BEGIN: return "OTA_BEGIN";
        case AVA_OTA_V3_ERR_DOWNLOAD_TIMEOUT: return "DOWNLOAD_TIMEOUT";
        case AVA_OTA_V3_ERR_OTA_WRITE: return "OTA_WRITE";
        case AVA_OTA_V3_ERR_HASH_MISMATCH: return "HASH_MISMATCH";
        case AVA_OTA_V3_ERR_IMAGE_INVALID: return "IMAGE_INVALID";
        case AVA_OTA_V3_ERR_READBACK: return "READBACK";
        case AVA_OTA_V3_ERR_SET_BOOT_PARTITION: return "SET_BOOT_PARTITION";
        case AVA_OTA_V3_ERR_BOOT_VALIDATION: return "BOOT_VALIDATION";
        default: return "UNKNOWN";
    }
}
