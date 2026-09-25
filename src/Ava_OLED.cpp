#include "Ava_OLED.h"

#include <Arduino.h>
#include <U8g2lib.h>

#include "Ava_DisplayAdapter.h"
#include "EyeDrawer.h"
#include "Ava_Font.h"
#include "Ava_Eyes.h"
#include "Ava_WiFi.h"
#include "Ava_OTA_Display.h"

// ==================================================
// FORWARD DECLARATIONS
// ==================================================

void avaOLEDRestoreNormalEyes();

// ==================================================
// OLED CONFIGURATION
// ==================================================

#define AVA_OLED_WIDTH   128
#define AVA_OLED_HEIGHT  64

// ==================================================
// SSD1309 SPI PINS
// ==================================================

#define AVA_OLED_SCK     12
#define AVA_OLED_MOSI    11
#define AVA_OLED_RES     10
#define AVA_OLED_DC      13
#define AVA_OLED_CS      14

// ==================================================
// TIME CONFIGURATION
// ==================================================

#define AVA_TIME_DISPLAY_DURATION_MS 6000UL

#define AVA_TIME_EYE_CENTER_Y   22
#define AVA_NORMAL_EYE_CENTER_Y 32

#define AVA_TIME_TEXT_Y 43

// ==================================================
// WEATHER CONFIGURATION
// ==================================================

#define AVA_WEATHER_DISPLAY_DURATION_MS 15000UL

// ==================================================
// OLED OBJECT
//
// SSD1309 128x64
// 4-Wire Software SPI
//
// SCK  -> GPIO12
// MOSI -> GPIO11
// CS   -> GPIO14
// DC   -> GPIO13
// RES  -> GPIO10
// ==================================================

U8G2_SSD1309_128X64_NONAME0_F_4W_SW_SPI avaDisplay(
    U8G2_R0,
    AVA_OLED_SCK,
    AVA_OLED_MOSI,
    AVA_OLED_CS,
    AVA_OLED_DC,
    AVA_OLED_RES
);

// ==================================================
// INTERNAL HARDWARE STATE
// ==================================================

bool avaOLEDReady = false;

// ==================================================
// TIME STATE
// ==================================================

String avaCurrentTime = "--:--";

bool avaTimeAvailable = false;

unsigned long avaTimeDisplayStartedMs = 0;

// ==================================================
// WEATHER DISPLAY STATE
// ==================================================

unsigned long avaWeatherDisplayStartedMs = 0;

// ==================================================
// OLED BEGIN
// ==================================================

void avaOLEDBegin()
{
    Serial.println("[OLED] Initializing SSD1309 SPI...");

    // --------------------------------------------------
    // Initialize U8g2 / SSD1309
    // --------------------------------------------------

    avaDisplay.begin();
    avaDisplay.setContrast(255);

    // --------------------------------------------------
    // Bind physical OLED to display adapter
    // --------------------------------------------------

    AvaDisplayAdapter::begin(
        avaDisplay
    );

    // --------------------------------------------------
    // Initialize AVA font system
    // --------------------------------------------------

    avaFontBegin();

    // --------------------------------------------------
    // Update OLED state
    // --------------------------------------------------

    avaOLEDReady = true;

    avaOLED().initialized = true;
    avaOLED().available = true;

    // --------------------------------------------------
    // Clear framebuffer
    // --------------------------------------------------

    AvaDisplayAdapter::clear();

    AvaDisplayAdapter::displayRefresh();

    // --------------------------------------------------
    // Initial diagnostic screen
    // --------------------------------------------------

    avaDisplay.clearBuffer();

    avaDisplay.setFont(
        u8g2_font_6x10_tf
    );

    avaDisplay.setDrawColor(1);

    avaDisplay.drawStr(
        0,
        10,
        "AVA OLED"
    );

    avaDisplay.drawStr(
        0,
        23,
        "SSD1309"
    );

    avaDisplay.drawStr(
        0,
        36,
        "128x64"
    );

    avaDisplay.drawStr(
        0,
        49,
        "SPI READY"
    );

    avaDisplay.sendBuffer();

    Serial.println("[OLED] READY");
}

// ==================================================
// APPLY CONTRAST
// ==================================================

void avaOLEDApplyContrast(
    uint8_t contrast
)
{
    if (!avaOLEDReady)
    {
        return;
    }

    avaDisplay.setContrast(contrast);
}

// ==================================================
// SET TIME
// ==================================================

void avaOLEDSetTime(
    const String& time
)
{
    String value = time;

    value.trim();

    if (
        value.length() == 0
    )
    {
        return;
    }

    avaCurrentTime = value;

    avaTimeAvailable = true;

    avaTimeDisplayStartedMs =
        millis();

    Serial.print(
        "[OLED] Time received: "
    );

    Serial.println(
        avaCurrentTime
    );

    Serial.println(
        "[OLED] Time display timer started (6 seconds)."
    );
}

// ==================================================
// GET TIME
// ==================================================

String avaOLEDGetTime()
{
    return avaCurrentTime;
}

// ==================================================
// CHECK TIME
// ==================================================

bool avaOLEDHasTime()
{
    return avaTimeAvailable;
}

// ==================================================
// RENDER TIME
// ==================================================

void avaOLEDRenderTime()
{
    if (!avaOLEDReady)
    {
        return;
    }

    AvaDisplayAdapter::beginFrame();

    // --------------------------------------------------
    // Time font
    // --------------------------------------------------

    avaDisplay.setFont(
        u8g2_font_logisoso24_tn
    );

    avaDisplay.setDrawColor(1);

    // --------------------------------------------------
    // Calculate text width
    // --------------------------------------------------

    int16_t textWidth =
        avaDisplay.getStrWidth(
            avaCurrentTime.c_str()
        );

    int16_t x =
        (
            AVA_OLED_WIDTH -
            textWidth
        ) / 2;

    // --------------------------------------------------
    // U8g2 text Y coordinate is the baseline
    // --------------------------------------------------

    avaDisplay.drawStr(
        x,
        AVA_TIME_TEXT_Y,
        avaCurrentTime.c_str()
    );

    AvaDisplayAdapter::endFrame();
}

// ==================================================
// SHOW WEATHER
// ==================================================

void avaOLEDShowWeather()
{
    if (avaOTAUIIsActive())
    {
        Serial.println("[OLED] Weather display blocked: OTA owns OLED.");
        return;
    }

    if (!avaOLEDReady)
    {
        return;
    }

    // --------------------------------------------------
    // Weather data must already exist
    // --------------------------------------------------