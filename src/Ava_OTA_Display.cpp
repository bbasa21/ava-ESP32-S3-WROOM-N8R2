#include "Ava_OTA_Display.h"

#include <U8g2lib.h>

#include "Ava_DisplayAdapter.h"

extern U8G2_SSD1309_128X64_NONAME0_F_4W_SW_SPI avaDisplay;

// ==================================================
// OTA UI STATE
// ==================================================

static volatile bool avaOTAUIActive = false;
static volatile int avaOTAUIProgress = 0;
static volatile AvaOTAUIStatus avaOTAUIState = AVA_OTA_UI_IDLE;

// ==================================================
// OTA UI CONTROL
// These functions are called from the dedicated OTA task.
// They only update small scalar state; OLED drawing stays
// inside the normal Arduino loopTask.
// ==================================================

void avaOTAUIBegin()
{
    avaOTAUIProgress = 0;
    avaOTAUIState = AVA_OTA_UI_UPDATING;
    avaOTAUIActive = true;
}

void avaOTAUISetProgress(int percent)
{
    if (percent < 0)
    {
        percent = 0;
    }

    if (percent > 100)
    {
        percent = 100;
    }

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
// RENDER
// ==================================================

static void avaOTAUIRender()
{
    if (!AvaDisplayAdapter::isReady())
    {
        return;
    }

    int progress = avaOTAUIProgress;
    AvaOTAUIStatus status = avaOTAUIState;

    const char* statusText = "UPDATING...";
    
    switch (status)
    {
        case AVA_OTA_UI_VERIFYING:
            statusText = "VERIFYING...";
            break;

        case AVA_OTA_UI_INSTALLING:
            statusText = "INSTALLING...";
            break;

        case AVA_OTA_UI_RESTARTING:
            statusText = "RESTARTING...";
            break;

        case AVA_OTA_UI_ERROR:
            statusText = "UPDATE ERROR";
            break;

        case AVA_OTA_UI_UPDATING:
        default:
            statusText = "UPDATING...";
            break;
    }

    AvaDisplayAdapter::beginFrame();

    avaDisplay.setDrawColor(1);

    // --------------------------------------------------
    // AVA
    // --------------------------------------------------

    avaDisplay.setFont(u8g2_font_7x14B_tf);

    const char* title = "AVA";
    int16_t titleWidth = avaDisplay.getStrWidth(title);

    avaDisplay.drawStr(
        (128 - titleWidth) / 2,
        13,
        title
    );

    // --------------------------------------------------
    // STATUS
    // --------------------------------------------------

    avaDisplay.setFont(u8g2_font_6x10_tf);

    int16_t statusWidth =
        avaDisplay.getStrWidth(statusText);

    avaDisplay.drawStr(
        (128 - statusWidth) / 2,
        27,
        statusText
    );

    // --------------------------------------------------
    // PROGRESS BAR
    // --------------------------------------------------

    const int16_t barX = 12;
    const int16_t barY = 35;
    const int16_t barWidth = 104;
    const int16_t barHeight = 9;

    avaDisplay.drawFrame(
        barX,
        barY,
        barWidth,
        barHeight
    );

    int16_t innerWidth =
        static_cast<int16_t>(
            ((barWidth - 2) * progress) / 100
        );

    if (innerWidth > 0)
    {
        avaDisplay.drawBox(
            barX + 1,
            barY + 1,
            innerWidth,
            barHeight - 2
        );
    }

    // --------------------------------------------------
    // PERCENT
    // --------------------------------------------------

    char percentText[8];

    snprintf(
        percentText,
        sizeof(percentText),
        "%d%%",
        progress
    );

    avaDisplay.setFont(u8g2_font_7x14B_tf);

    int16_t percentWidth =
        avaDisplay.getStrWidth(percentText);

    avaDisplay.drawStr(
        (128 - percentWidth) / 2,
        57,
        percentText
    );

    AvaDisplayAdapter::endFrame();
}

// ==================================================
// OTA UI TICK
// ==================================================

void avaOTAUIUpdate()
{
    if (!avaOTAUIActive)
    {
        return;
    }

    avaOTAUIRender();
}