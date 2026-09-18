#include "Ava_DisplaySettings.h"

#include <Preferences.h>
#include "Ava_OLED.h"

static Preferences displayPrefs;
static bool settingsActive = false;

static uint8_t pendingBrightness = 255;
static uint8_t pendingContrast = 255;
static String pendingMode = "NORMAL";

static uint8_t clampValue(int value)
{
    return (uint8_t)constrain(value, 0, 255);
}

static uint8_t effectiveContrast()
{
    uint16_t result =
        ((uint16_t)pendingBrightness * (uint16_t)pendingContrast) / 255U;

    return (uint8_t)constrain((int)result, 0, 255);
}

static void applyHardware()
{
    avaOLED().brightness = pendingBrightness;

    // SSD1309/U8g2 exposes hardware intensity through setContrast().
    // Brightness + contrast are combined so both controls have an effect.
    avaOLEDApplyContrast(effectiveContrast());

    if (pendingMode == "ECO")
    {
        // ECO is represented by the selected display intensity.
        // Keep NORMAL OLED mode so Eye Engine continues rendering normally.
        avaOLEDSetMode(AVA_OLED_NORMAL);
    }
    else
    {
        avaOLEDSetMode(AVA_OLED_NORMAL);
    }
}

void avaDisplaySettingsBegin()
{
    displayPrefs.begin("ava-display", false);

    pendingBrightness =
        displayPrefs.getUChar("brightness", 255);

    pendingContrast =
        displayPrefs.getUChar("contrast", 255);

    pendingMode =
        displayPrefs.getString("mode", "NORMAL");

    pendingMode.trim();
    pendingMode.toUpperCase();

    applyHardware();

    Serial.println("[DISPLAY] Settings loaded from NVS.");
}

void avaDisplaySettingsLoad()
{
    if (!displayPrefs.begin("ava-display", false))
    {
        Serial.println("[DISPLAY] NVS open failed.");
        return;
    }

    pendingBrightness =
        displayPrefs.getUChar("brightness", 255);

    pendingContrast =
        displayPrefs.getUChar("contrast", 255);

    pendingMode =
        displayPrefs.getString("mode", "NORMAL");

    pendingMode.trim();
    pendingMode.toUpperCase();

    applyHardware();

    Serial.println("[DISPLAY] NVS settings applied.");
}

void avaDisplaySettingsEnter()
{
    settingsActive = true;

    // Start a temporary editing session from the currently saved values.
    pendingBrightness =
        displayPrefs.getUChar("brightness", 255);

    pendingContrast =
        displayPrefs.getUChar("contrast", 255);

    pendingMode =
        displayPrefs.getString("mode", "NORMAL");

    pendingMode.trim();
    pendingMode.toUpperCase();

    Serial.println("[DISPLAY] SETTINGS SESSION ENTERED.");
}

void avaDisplaySettingsExit()
{
    settingsActive = false;
    Serial.println("[DISPLAY] SETTINGS SESSION EXITED.");
}

bool avaDisplaySettingsActive()
{
    return settingsActive;
}

void avaDisplaySettingsSetBrightness(uint8_t value)
{
    pendingBrightness = clampValue(value);
    applyHardware();

    Serial.print("[DISPLAY] Brightness preview: ");
    Serial.println(pendingBrightness);
}

void avaDisplaySettingsSetContrast(uint8_t value)
{
    pendingContrast = clampValue(value);
    applyHardware();

    Serial.print("[DISPLAY] Contrast preview: ");
    Serial.println(pendingContrast);
}

void avaDisplaySettingsSetMode(const String& mode)
{
    pendingMode = mode;
    pendingMode.trim();
    pendingMode.toUpperCase();

    if (pendingMode != "NORMAL" && pendingMode != "ECO")
    {
        pendingMode = "NORMAL";
    }

    applyHardware();

    Serial.print("[DISPLAY] Mode preview: ");
    Serial.println(pendingMode);
}

void avaDisplaySettingsApply()
{
    displayPrefs.putUChar("brightness", pendingBrightness);
    displayPrefs.putUChar("contrast", pendingContrast);
    displayPrefs.putString("mode", pendingMode);

    applyHardware();

    Serial.println("[DISPLAY] SETTINGS APPLIED AND SAVED TO NVS.");
}

uint8_t avaDisplaySettingsBrightness()
{
    return pendingBrightness;
}

uint8_t avaDisplaySettingsContrast()
{
    return pendingContrast;
}

String avaDisplaySettingsMode()
{
    return pendingMode;
}
