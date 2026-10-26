#include "Ava_OTA_Display.h"

#include <U8g2lib.h>

#include "Ava_DisplayAdapter.h"
#include "Ava_OLED.h"
#include "Ava_Eyes.h"

extern U8G2_SSD1309_128X64_NONAME0_F_4W_SW_SPI avaDisplay;

// ==================================================
// OTA UI STATE
// ==================================================

static volatile bool avaOTAUIActive = false;
static volatile int avaOTAUIProgress = 0;
static volatile AvaOTAUIStatus avaOTAUIState = AVA_OTA_UI_IDLE;

void avaOTAUIBegin()
{
    avaOTAUIProgress = 0;
    avaOTAUIState = AVA_OTA_UI_UPDATING;
    avaOTAUIActive = true;
}

void avaOTAUISetProgress(int percent)
{
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    avaOTAUIProgress = percent;
}

void avaOTAUISetStatus(AvaOTAUIStatus status)
{
    avaOTAUIState = status;
}

bool avaOTAUIIsActive()
{
    return avaOTAUIActive;
}

void avaOTAUIEnd()
{
    avaOTAUIActive = false;
    avaOTAUIState = AVA_OTA_UI_IDLE;
    avaOTAUIProgress = 0;
}

// ==================================================
// OTA RENDER — AVA'S REAL ANIMATED EYES + UPDATE PANEL
// 128x64 SSD1309:
//   - Existing animated eyes at the top, without a face frame.
//   - Update status and a slim progress line below.
// ==================================================

static void avaOTAUIRender()
{
    if (!AvaDisplayAdapter::isReady() ||
        !avaOLEDCanRender(AVA_OLED_OWNER_OTA))
    {
        return;
    }

    const int progress = avaOTAUIProgress;
    const AvaOTAUIStatus status = avaOTAUIState;

    const char* statusText = "DOWNLOADING";
    const char* footerText = "PLEASE WAIT";

    switch (status)
    {
        case AVA_OTA_UI_VERIFYING:
            statusText = "VERIFYING";
            footerText = "CHECKING IMAGE";
            break;

        case AVA_OTA_UI_INSTALLING:
            statusText = "INSTALLING";
            footerText = "WRITING FIRMWARE";
            break;

        case AVA_OTA_UI_RESTARTING:
            statusText = "RESTARTING";
            footerText = "AVA WILL REBOOT";
            break;

        case AVA_OTA_UI_ERROR:
            statusText = "UPDATE ERROR";
            footerText = "CHECK SERIAL LOG";
            break;

        case AVA_OTA_UI_UPDATING:
        case AVA_OTA_UI_IDLE:
        default:
            statusText = "DOWNLOADING";
            footerText = "PLEASE WAIT";
            break;
    }

    AvaDisplayAdapter::beginFrame();
    avaDisplay.clearBuffer();
    avaDisplay.setDrawColor(1);

    // Use the actual animated eyes used by the normal AVA face.
    // Move their centers up temporarily to make room for OTA information.
    const int16_t leftPreviousY = avaLeftEye.CenterY;
    const int16_t rightPreviousY = avaRightEye.CenterY;
    avaLeftEye.CenterY = 15;
    avaRightEye.CenterY = 15;

    avaLeftEye.Draw();
    avaRightEye.Draw();

    // Restore the normal face geometry immediately after rendering.
    avaLeftEye.CenterY = leftPreviousY;
    avaRightEye.CenterY = rightPreviousY;

    // A single quiet divider; no boxes or segmented tiles.
    avaDisplay.drawHLine(5, 32, 118);

    avaDisplay.setFont(u8g2_font_6x10_tf);
    avaDisplay.drawStr(5, 44, statusText);

    char percentText[8];
    snprintf(percentText, sizeof(percentText), "%d%%", progress);
    const int16_t percentWidth = avaDisplay.getStrWidth(percentText);
    avaDisplay.drawStr(123 - percentWidth, 44, percentText);

    // Thin continuous progress line.
    const int16_t barX = 5;
    const int16_t barWidth = 118;
    avaDisplay.drawHLine(barX, 51, barWidth);
    const int16_t progressWidth =
        static_cast<int16_t>((barWidth * progress) / 100);
    if (progressWidth > 0)
    {
        avaDisplay.drawHLine(barX, 51, progressWidth);
    }

    avaDisplay.setFont(u8g2_font_5x8_tf);
    avaDisplay.drawStr(5, 62, footerText);

    AvaDisplayAdapter::endFrame();
}

void avaOTAUIUpdate()
{
    if (!avaOTAUIActive)
    {
        return;
    }

    avaOTAUIRender();
}
