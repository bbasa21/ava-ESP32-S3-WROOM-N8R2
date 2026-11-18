#include "Ava_OTA_V3_BootGuard.h"

#include <Arduino.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include "Ava_OTA_V3.h"

namespace {
bool pendingVerification = false;
uint32_t verificationStartedAt = 0;
constexpr uint32_t HEALTH_CONFIRM_DELAY_MS = 60000UL;
constexpr uint32_t HEALTH_FAILSAFE_DELAY_MS = 180000UL;
constexpr uint32_t MIN_HEAP_TO_CONFIRM = 20U * 1024U;
}

void avaOTAV3BootGuardBegin() {
    pendingVerification = false;
    verificationStartedAt = 0;

    const esp_partition_t* running = esp_ota_get_running_partition();
    if (!running) {
        Serial.println("[OTA3][BOOT] Running partition unavailable.");
        return;
    }

    esp_ota_img_states_t imageState;
    const esp_err_t result = esp_ota_get_state_partition(running, &imageState);
    if (result != ESP_OK) {
        Serial.printf("[OTA3][BOOT] No pending-verify state (err=%d).\n",
                      static_cast<int>(result));
        return;
    }

    if (imageState == ESP_OTA_IMG_PENDING_VERIFY) {
        pendingVerification = true;
        verificationStartedAt = millis();
        avaOTAV3BootVerificationStarted();
    } else {
        Serial.printf("[OTA3][BOOT] Running image state=%d; no confirmation required.\n",
                      static_cast<int>(imageState));
    }
}

void avaOTAV3BootGuardTick() {
    if (!pendingVerification) return;

    const uint32_t elapsed = static_cast<uint32_t>(millis() - verificationStartedAt);
    const uint32_t freeHeap = ESP.getFreeHeap();

    if (elapsed >= HEALTH_CONFIRM_DELAY_MS && freeHeap >= MIN_HEAP_TO_CONFIRM) {
        const esp_err_t result = esp_ota_mark_app_valid_cancel_rollback();
        if (result == ESP_OK) {
            pendingVerification = false;
            avaOTAV3BootVerificationConfirmed();
            return;
        }
        Serial.printf("[OTA3][BOOT] Mark-valid failed (err=%d); still pending.\n",
                      static_cast<int>(result));
    }

    if (elapsed >= HEALTH_FAILSAFE_DELAY_MS && freeHeap < MIN_HEAP_TO_CONFIRM) {
        Serial.printf("[OTA3][BOOT] Health check failed: free heap=%lu. Reverting.\n",
                      static_cast<unsigned long>(freeHeap));
        avaOTAV3BootVerificationFailed();
        // Bootloader performs the actual rollback to the previous OTA slot.
        esp_ota_mark_app_invalid_rollback_and_reboot();
    }
}
