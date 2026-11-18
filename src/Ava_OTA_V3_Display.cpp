#include "Ava_OTA_V3_Display.h"

#include <U8g2lib.h>
#include <stdio.h>

#include "Ava_OTA_V3.h"
#include "Ava_DisplayAdapter.h"
#include "Ava_OLED.h"

extern U8G2_SSD1309_128X64_NONAME0_F_4W_SW_SPI avaDisplay;

namespace {
AvaOTAV3State lastDisplayedState = AVA_OTA_V3_IDLE;
uint32_t terminalHoldUntil = 0;
constexpr uint32_t TERMINAL_SCREEN_HOLD_MS = 6500UL;

bool isTerminalState(AvaOTAV3State state) {
    return state == AVA_OTA_V3_UPDATE_AVAILABLE ||
           state == AVA_OTA_V3_UP_TO_DATE ||
           state == AVA_OTA_V3_BOOT_VALIDATED ||
           state == AVA_OTA_V3_FAILED;
}
bool isActiveState(AvaOTAV3State state) {
    return state == AVA_OTA_V3_CHECK_QUEUED ||
           state == AVA_OTA_V3_CHECKING ||
           state == AVA_OTA_V3_INSTALL_QUEUED ||
           state == AVA_OTA_V3_DOWNLOADING ||
           state == AVA_OTA_V3_VERIFYING_IMAGE ||
           state == AVA_OTA_V3_READY_TO_BOOT ||
           state == AVA_OTA_V3_REBOOT_PENDING ||
           state == AVA_OTA_V3_BOOT_VERIFYING;
}
const char* labelFor(AvaOTAV3State state) {
    switch (state) {
        case AVA_OTA_V3_CHECK_QUEUED: return "CHECK QUEUED";
        case AVA_OTA_V3_CHECKING: return "CHECKING RELEASE";
        case AVA_OTA_V3_UPDATE_AVAILABLE: return "UPDATE AVAILABLE";
        case AVA_OTA_V3_UP_TO_DATE: return "SYSTEM UP TO DATE";
        case AVA_OTA_V3_INSTALL_QUEUED: return "PREPARING INSTALL";
        case AVA_OTA_V3_DOWNLOADING: return "DOWNLOADING";
        case AVA_OTA_V3_VERIFYING_IMAGE: return "VERIFYING IMAGE";
        case AVA_OTA_V3_READY_TO_BOOT: return "IMAGE VERIFIED";
        case AVA_OTA_V3_REBOOT_PENDING: return "RESTARTING";
        case AVA_OTA_V3_BOOT_VERIFYING: return "BOOT HEALTH CHECK";
        case AVA_OTA_V3_BOOT_VALIDATED: return "UPDATE CONFIRMED";
        case AVA_OTA_V3_FAILED: return "UPDATE FAILED";
        default: return "OTA V3";
    }
}
}

bool avaOTAV3DisplayIsActive() {
    const AvaOTAV3Status status = avaOTAV3GetStatus();
    const uint32_t now = millis();

    if (status.state != lastDisplayedState) {
        lastDisplayedState = status.state;
        terminalHoldUntil = isTerminalState(status.state)
            ? now + TERMINAL_SCREEN_HOLD_MS : 0;
    }
    if (isActiveState(status.state)) return true;
    return isTerminalState(status.state) &&
           static_cast<int32_t>(terminalHoldUntil - now) > 0;
}

void avaOTAV3DisplayUpdate() {
    if (!avaOTAV3DisplayIsActive() ||
        !AvaDisplayAdapter::isReady() ||
        !avaOLEDCanRender(AVA_OLED_OWNER_OTA)) return;

    const AvaOTAV3Status status = avaOTAV3GetStatus();
    AvaDisplayAdapter::beginFrame();
    avaDisplay.setDrawColor(1);

    avaDisplay.setFont(u8g2_font_7x14B_tf);
    const char* title = "AVA OTA  //  V3";
    avaDisplay.drawStr((128 - avaDisplay.getStrWidth(title)) / 2, 12, title);

    avaDisplay.setFont(u8g2_font_6x10_tf);
    const char* label = labelFor(status.state);
    avaDisplay.drawStr((128 - avaDisplay.getStrWidth(label)) / 2, 25, label);

    char buildLine[32];
    if (status.availableBuild) {
        snprintf(buildLine, sizeof(buildLine), "BUILD %lu -> %lu",
                 static_cast<unsigned long>(status.currentBuild),
                 static_cast<unsigned long>(status.availableBuild));
    } else {
        snprintf(buildLine, sizeof(buildLine), "CURRENT BUILD %lu",
                 static_cast<unsigned long>(status.currentBuild));
    }
    avaDisplay.drawStr(8, 37, buildLine);
    avaDisplay.drawFrame(8, 42, 112, 9);

    const int fillWidth = (108 * status.progress) / 100;
    if (fillWidth > 0) avaDisplay.drawBox(10, 44, fillWidth, 5);

    char footer[32];
    if (status.state == AVA_OTA_V3_DOWNLOADING && status.totalBytes) {
        snprintf(footer, sizeof(footer), "%u%% %lu/%lu KB",
                 status.progress,
                 static_cast<unsigned long>(status.bytesTransferred / 1024U),
                 static_cast<unsigned long>(status.totalBytes / 1024U));
    } else if (status.error != AVA_OTA_V3_ERR_NONE) {
        snprintf(footer, sizeof(footer), "ERR: %s",
                 avaOTAV3ErrorName(status.error));
    } else {
        snprintf(footer, sizeof(footer), "%s", status.detail);
    }
    avaDisplay.setFont(u8g2_font_5x8_tf);
    avaDisplay.drawStr(2, 62, footer);
    AvaDisplayAdapter::endFrame();
}
