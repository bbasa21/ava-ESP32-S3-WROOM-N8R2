#include "Ava_OLED.h"

#include <Arduino.h>
#include <U8g2lib.h>

#include "Ava_DisplayAdapter.h"
#include "EyeDrawer.h"
#include "Ava_Font.h"
#include "Ava_Eyes.h"
#include "Ava_WiFi.h"

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
    if (!avaOLEDReady)
    {
        return;
    }

    // --------------------------------------------------
    // Weather data must already exist
    // --------------------------------------------------

    if (!avaWeatherAvailable())
    {
        Serial.println(
            "[OLED] Weather display skipped: no valid weather data."
        );

        return;
    }

    // --------------------------------------------------
    // Start / restart timer
    // --------------------------------------------------

    avaWeatherDisplayStartedMs =
        millis();

    // --------------------------------------------------
    // Weather becomes OLED owner
    // --------------------------------------------------

    avaOLEDSetMode(
        AVA_OLED_WEATHER
    );

    Serial.println(
        "[OLED] Weather display started (15 seconds)."
    );
}

// ==================================================
// WEATHER ACTIVE
// ==================================================

bool avaOLEDWeatherActive()
{
    return (
        avaOLED().mode ==
        AVA_OLED_WEATHER
    );
}

// ==================================================
// RENDER WEATHER
// ==================================================

void avaOLEDRenderWeather()
{
    if (!avaOLEDReady)
    {
        return;
    }

    AvaDisplayAdapter::beginFrame();

    // --------------------------------------------------
    // Font
    // --------------------------------------------------

    avaDisplay.setFont(
        u8g2_font_6x10_tf
    );

    avaDisplay.setDrawColor(1);

    // ==================================================
    // TITLE
    // ==================================================

    const char* title =
        "TEHRAN WEATHER";

    int16_t titleWidth =
        avaDisplay.getStrWidth(
            title
        );

    avaDisplay.drawStr(
        (
            AVA_OLED_WIDTH -
            titleWidth
        ) / 2,
        9,
        title
    );

    // ==================================================
    // SEPARATOR
    // ==================================================

    avaDisplay.drawHLine(
        0,
        11,
        128
    );

    // ==================================================
    // TEMPERATURE
    // ==================================================

    String temperature =
        String(
            avaWeatherTemperature(),
            1
        );

    temperature +=
        " C";

    avaDisplay.setFont(
        u8g2_font_7x14B_tf
    );

    int16_t temperatureWidth =
        avaDisplay.getStrWidth(
            temperature.c_str()
        );

    avaDisplay.drawStr(
        (
            AVA_OLED_WIDTH -
            temperatureWidth
        ) / 2,
        27,
        temperature.c_str()
    );

    // ==================================================
    // CONDITION
    // ==================================================

    avaDisplay.setFont(
        u8g2_font_6x10_tf
    );

    String condition =
        avaWeatherCodeName(
            avaWeatherCode()
        );

    int16_t conditionWidth =
        avaDisplay.getStrWidth(
            condition.c_str()
        );

    avaDisplay.drawStr(
        (
            AVA_OLED_WIDTH -
            conditionWidth
        ) / 2,
        39,
        condition.c_str()
    );

    // ==================================================
    // HUMIDITY + WIND
    // ==================================================

    String bottom =
        "H:" +
        String(
            avaWeatherHumidity()
        ) +
        "%  W:" +
        String(
            avaWeatherWindSpeed(),
            1
        );

    avaDisplay.drawStr(
        18,
        51,
        bottom.c_str()
    );

    // ==================================================
    // REMAINING TIMER
    // ==================================================

    unsigned long elapsed =
        millis() -
        avaWeatherDisplayStartedMs;

    unsigned long remaining =
        0;

    if (
        elapsed <
        AVA_WEATHER_DISPLAY_DURATION_MS
    )
    {
        remaining =
            (
                AVA_WEATHER_DISPLAY_DURATION_MS -
                elapsed +
                999
            ) / 1000;
    }

    String timer =
        String(
            remaining
        ) +
        "s";

    int16_t timerWidth =
        avaDisplay.getStrWidth(
            timer.c_str()
        );

    avaDisplay.drawStr(
        AVA_OLED_WIDTH -
        timerWidth -
        2,
        51,
        timer.c_str()
    );

    // ==================================================
    // PUSH COMPLETE FRAME
    // ==================================================

    AvaDisplayAdapter::endFrame();
}

// ==================================================
// OLED UPDATE
// ==================================================

void avaOLEDUpdate()
{
    if (!avaOLEDReady)
    {
        return;
    }

    const unsigned long now =
        millis();

    avaOLED().lastUpdateMs =
        now;

    // ==================================================
    // WEATHER MODE
    // ==================================================

    if (
        avaOLED().mode ==
        AVA_OLED_WEATHER
    )
    {
        const unsigned long elapsed =
            now -
            avaWeatherDisplayStartedMs;

        if (
            elapsed >=
            AVA_WEATHER_DISPLAY_DURATION_MS
        )
        {
            Serial.println(
                "[OLED] Weather display finished."
            );

            avaOLEDSetMode(
                AVA_OLED_NORMAL
            );

            avaRenderEyeFrame();

            Serial.println(
                "[OLED] Display control returned to Eye Engine."
            );

            return;
        }

        avaOLEDRenderWeather();

        return;
    }

    // ==================================================
    // TIME MODE
    // ==================================================

    if (
        avaOLED().mode ==
        AVA_OLED_TIME
    )
    {
        const unsigned long elapsed =
            now -
            avaTimeDisplayStartedMs;

        if (
            elapsed >=
            AVA_TIME_DISPLAY_DURATION_MS
        )
        {
            Serial.println(
                "[OLED] Time display finished."
            );

            avaOLEDSetMode(
                AVA_OLED_NORMAL
            );

            avaOLEDRestoreNormalEyes();

            avaRenderEyeFrame();

            Serial.println(
                "[OLED] Display control returned to Eye Engine."
            );

            return;
        }

        avaOLEDRenderTime();

        return;
    }

    // ==================================================
    // OTHER MODES
    // ==================================================

    switch (
        avaOLED().mode
    )
    {
        case AVA_OLED_BATTERY:

            // Reserved for battery UI.
            break;

        case AVA_OLED_DEBUG:

            // Reserved for OLED debug UI.
            break;

        case AVA_OLED_NORMAL:

        default:

            // NORMAL is owned by Eye Engine.
            break;
    }
}

// ==================================================
// RESTORE NORMAL EYES
// ==================================================

void avaOLEDRestoreNormalEyes()
{
    avaLeftEye.SetCenter(
        38,
        AVA_NORMAL_EYE_CENTER_Y
    );

    avaRightEye.SetCenter(
        90,
        AVA_NORMAL_EYE_CENTER_Y
    );

    Serial.println(
        "[OLED] Eye position restored to NORMAL."
    );
}

// ==================================================
// CLEAR
// ==================================================

void avaOLEDClear()
{
    if (!avaOLEDReady)
    {
        return;
    }

    AvaDisplayAdapter::clear();

    AvaDisplayAdapter::displayRefresh();
}

// ==================================================
// RENDER
// ==================================================

void avaOLEDRender()
{
    if (!avaOLEDReady)
    {
        return;
    }

    switch (
        avaOLED().mode
    )
    {
        case AVA_OLED_TIME:

            avaOLEDRenderTime();

            return;

        case AVA_OLED_WEATHER:

            avaOLEDRenderWeather();

            return;

        case AVA_OLED_BATTERY:

            // Reserved.
            return;

        case AVA_OLED_DEBUG:

            // Reserved.
            return;

        case AVA_OLED_NORMAL:

        default:

            // NORMAL is owned by Eye Engine.
            return;
    }
}

// ==================================================
// DEBUG
// ==================================================

void avaOLEDDebugPrint()
{
    Serial.println();

    Serial.println(
        "=========== AVA OLED ==========="
    );

    Serial.print(
        "Ready: "
    );

    Serial.println(
        avaOLEDReady
            ? "YES"
            : "NO"
    );

    Serial.print(
        "Initialized: "
    );

    Serial.println(
        avaOLED().initialized
            ? "YES"
            : "NO"
    );

    Serial.print(
        "Available: "
    );

    Serial.println(
        avaOLED().available
            ? "YES"
            : "NO"
    );

    Serial.print(
        "Adapter: "
    );

    Serial.println(
        AvaDisplayAdapter::isReady()
            ? "READY"
            : "NOT READY"
    );

    Serial.print(
        "Controller: "
    );

    Serial.println(
        "SSD1309"
    );

    Serial.print(
        "Resolution: "
    );

    Serial.println(
        "128x64"
    );

    Serial.print(
        "Protocol: "
    );

    Serial.println(
        "SPI"
    );

    Serial.print(
        "SCK: "
    );

    Serial.println(
        AVA_OLED_SCK
    );

    Serial.print(
        "MOSI: "
    );

    Serial.println(
        AVA_OLED_MOSI
    );

    Serial.print(
        "RES: "
    );

    Serial.println(
        AVA_OLED_RES
    );

    Serial.print(
        "DC: "
    );

    Serial.println(
        AVA_OLED_DC
    );

    Serial.print(
        "CS: "
    );

    Serial.println(
        AVA_OLED_CS
    );

    Serial.print(
        "Mode: "
    );

    Serial.println(
        avaOLEDModeName(
            avaOLED().mode
        )
    );

    Serial.print(
        "Eye Style: "
    );

    Serial.println(
        avaOLEDStyleName(
            avaOLED().eyeStyle
        )
    );

    Serial.print(
        "Gaze: "
    );

    Serial.print(
        avaOLED().gaze.x
    );

    Serial.print(
        ","
    );

    Serial.println(
        avaOLED().gaze.y
    );

    Serial.print(
        "Battery: "
    );

    Serial.print(
        avaOLED().batteryPercent
    );

    Serial.println(
        "%"
    );

    Serial.print(
        "Brightness: "
    );

    Serial.println(
        avaOLED().brightness
    );

    Serial.print(
        "Time: "
    );

    Serial.println(
        avaCurrentTime
    );

    Serial.print(
        "Time available: "
    );

    Serial.println(
        avaTimeAvailable
            ? "YES"
            : "NO"
    );

    Serial.print(
        "Weather active: "
    );

    Serial.println(
        avaOLEDWeatherActive()
            ? "YES"
            : "NO"
    );

    if (
        avaWeatherAvailable()
    )
    {
        Serial.print(
            "Weather temperature: "
        );

        Serial.print(
            avaWeatherTemperature(),
            1
        );

        Serial.println(
            " C"
        );

        Serial.print(
            "Weather humidity: "
        );

        Serial.print(
            avaWeatherHumidity()
        );

        Serial.println(
            " %"
        );

        Serial.print(
            "Weather code: "
        );

        Serial.println(
            avaWeatherCode()
        );

        Serial.print(
            "Weather condition: "
        );

        Serial.println(
            avaWeatherCodeName(
                avaWeatherCode()
            )
        );

        Serial.print(
            "Weather wind: "
        );

        Serial.print(
            avaWeatherWindSpeed(),
            1
        );

        Serial.println(
            " km/h"
        );
    }

    if (
        avaOLED().mode ==
        AVA_OLED_TIME
    )
    {
        Serial.print(
            "Time display elapsed: "
        );

        Serial.print(
            millis() -
            avaTimeDisplayStartedMs
        );

        Serial.println(
            " ms"
        );
    }

    if (
        avaOLED().mode ==
        AVA_OLED_WEATHER
    )
    {
        Serial.print(
            "Weather display elapsed: "
        );

        Serial.print(
            millis() -
            avaWeatherDisplayStartedMs
        );

        Serial.println(
            " ms"
        );
    }

    Serial.println(
        "================================"
    );

    Serial.println();
}