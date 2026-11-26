#include "Ava_OTA_BootGuard.h"

#include <Arduino.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include "sdkconfig.h"

#if !defined(CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE) || CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE != 1
#error "AVA OTA requires CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=1."
#endif

#include "Ava_OLED.h"
#include "Ava_DisplayAdapter.h"

namespace {
bool pendingVerification = false;
uint32_t verificationStartedAt = 0;

constexpr uint32_t HEALTH_CONFIRM_DELAY_MS = 60000UL;
constexpr uint32_t HEALTH_FAILSAFE_DELAY_MS = 180000UL;
constexpr uint32_t MIN_HEAP_TO_CONFIRM = 20U * 1024U;
}

void avaOTABootGuardBegin() {
    pendingVerification = false;
    verificationStartedAt = 0;

    const esp_partition_t* running = esp_ota_get_running_partition();
    if (!running) {
        Serial.println("[OTA][BOOT] Running partition unavailable.");
        return;
    }

    esp_ota_img_states_t imageState;
    const esp_err_t result =
        esp_ota_get_state_partition(running, &imageState);

    if (result != ESP_OK) {
        Serial.printf("[OTA][BOOT] No pending-verify state (err=%d).\n",
                      static_cast<int>(result));
        return;
    }

    if (imageState == ESP_OTA_IMG_PENDING_VERIFY) {
        pendingVerification = true;
        verificationStartedAt = millis();
        Serial.println("[OTA][BOOT] New image pending health verification.");
    } else {
        Serial.printf("[OTA][BOOT] Image state=%d; confirmation not required.\n",
                      static_cast<int>(imageState));
    }
}

void avaOTABootGuardTick() {
    if (!pendingVerification) return;

    const uint32_t elapsed =
        static_cast<uint32_t>(millis() - verificationStartedAt);

    const uint32_t freeHeap = ESP.getFreeHeap();

    const bool displayHealthy =
        avaOLEDIsInitialized() &&
        avaOLEDIsAvailable() &&
        AvaDisplayAdapter::isReady();

    const bool coreHealthy =
        freeHeap >= MIN_HEAP_TO_CONFIRM &&
        displayHealthy;

    if (elapsed >= HEALTH_CONFIRM_DELAY_MS && coreHealthy) {
        const esp_err_t result =
            esp_ota_mark_app_valid_cancel_rollback();

        if (result == ESP_OK) {
            pendingVerification = false;
            Serial.println("[OTA][BOOT] Image confirmed healthy; rollback cancelled.");
            return;
        }

        Serial.printf("[OTA][BOOT] Mark-valid failed (err=%d); still pending.\n",
                      static_cast<int>(result));
    }

    if (elapsed >= HEALTH_FAILSAFE_DELAY_MS) {
        Serial.printf(
            "[OTA][BOOT] Health timeout: heap=%lu, OLED=%s. Rolling back.\n",
            static_cast<unsigned long>(freeHeap),
            displayHealthy ? "OK" : "NOT READY"
        );

        // The ESP32 bootloader performs the actual rollback.
        esp_ota_mark_app_invalid_rollback_and_reboot();
    }
}