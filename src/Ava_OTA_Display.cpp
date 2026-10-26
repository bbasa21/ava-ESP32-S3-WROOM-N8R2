#include "Ava_OTA_Display.h"

#include <U8g2lib.h>

#include "Ava_DisplayAdapter.h"
#include "Ava_OLED.h"

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
// CYBERPUNK HUD RENDER
// 128x64 monochrome SSD1309. AVA's normal face is
// represented by a small custom eye icon in the header.
// No full-screen animation or extra heap allocation.
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
    switch (status)
    {
        case AVA_OTA_UI_VERIFYING:  statusText = "VERIFYING";  break;
        case AVA_OTA_UI_INSTALLING: statusText = "INSTALLING"; break;
        case AVA_OTA_UI_RESTARTING: statusText = "RESTARTING"; break;
        case AVA_OTA_UI_ERROR:      statusText = "UPDATE ERROR"; break;
        case AVA_OTA_UI_UPDATING:
        case AVA_OTA_UI_IDLE:
        default:                   statusText = "DOWNLOADING"; break;
    }

    AvaDisplayAdapter::beginFrame();
    avaDisplay.setDrawColor(1);

    // Header frame and angled cyber-HUD corners.
    avaDisplay.drawHLine(2, 1, 124);
    avaDisplay.drawHLine(2, 15, 124);
    avaDisplay.drawVLine(2, 1, 5);
    avaDisplay.drawVLine(125, 1, 5);
    avaDisplay.drawVLine(2, 11, 5);
    avaDisplay.drawVLine(125, 11, 5);

    // Tiny AVA face: two eyes inside a compact HUD frame.
    avaDisplay.drawFrame(6, 3, 19, 10);
    avaDisplay.drawBox(10, 6, 3, 4);
    avaDisplay.drawBox(18, 6, 3, 4);
    avaDisplay.drawHLine(14, 8, 3);

    // Title and a tiny moving scan marker.
    avaDisplay.setFont(u8g2_font_6x10_tf);
    avaDisplay.drawStr(31, 10, "AVA // OTA");
    const int scanX = 91 + (static_cast<int>((millis() / 90UL) % 6UL) * 5);
    avaDisplay.drawVLine(scanX, 4, 7);
    avaDisplay.drawPixel(scanX + 1, 3);

    // Status line with a small technical indicator.
    if (status == AVA_OTA_UI_ERROR)
    {
        avaDisplay.drawFrame(7, 20, 7, 7);
        avaDisplay.drawLine(8, 21, 12, 25);
        avaDisplay.drawLine(12, 21, 8, 25);
    }
    else
    {
        avaDisplay.drawFrame(7, 20, 7, 7);
        avaDisplay.drawBox(9, 22, 3, 3);
    }

    avaDisplay.setFont(u8g2_font_6x10_tf);
    avaDisplay.drawStr(19, 27, statusText);

    // Progress percentage at the right edge.
    char percentText[8];
    snprintf(percentText, sizeof(percentText), "%d%%", progress);
    const int16_t percentWidth = avaDisplay.getStrWidth(percentText);
    avaDisplay.drawStr(121 - percentWidth, 27, percentText);

    // Segmented progress bar with a crisp outline.
    const int16_t barX = 7;
    const int16_t barY = 34;
    const int16_t barWidth = 114;
    const int16_t barHeight = 10;
    avaDisplay.drawFrame(barX, barY, barWidth, barHeight);

    const int16_t innerWidth =
        static_cast<int16_t>(((barWidth - 2) * progress) / 100);
    if (innerWidth > 0)
    {
        avaDisplay.drawBox(barX + 1, barY + 1, innerWidth, barHeight - 2);
    }

    // Small calibration ticks under the bar.
    for (int16_t x = barX; x <= barX + barWidth; x += 8)
    {
        avaDisplay.drawVLine(x, 45, 2);
    }

    // Footer is intentionally short and readable.
    avaDisplay.setFont(u8g2_font_5x8_tf);
    if (status == AVA_OTA_UI_ERROR)
    {
        avaDisplay.drawStr(7, 60, "FAILED - CHECK LOG");
    }
    else if (status == AVA_OTA_UI_RESTARTING)
    {
        avaDisplay.drawStr(7, 60, "DONE - REBOOTING");
    }
    else if (status == AVA_OTA_UI_VERIFYING)
    {
        avaDisplay.drawStr(7, 60, "CHECKING IMAGE");
    }
    else if (status == AVA_OTA_UI_INSTALLING)
    {
        avaDisplay.drawStr(7, 60, "WRITING FLASH");
    }
    else
    {
        avaDisplay.drawStr(7, 60, "SECURE LINK  /  OTA");
    }

    // A subtle lower HUD rail.
    avaDisplay.drawHLine(7, 63, 114);

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
