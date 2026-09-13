#ifndef AVA_OLED_H
#define AVA_OLED_H

#include <Arduino.h>

// ==================================================
// AVA OLED INTERFACE
//
// مسئولیت:
//
//   - OLED State
//   - Display Mode
//   - Eye Style
//   - Gaze
//   - Layout
//   - Battery
//   - Brightness
//   - Time
//   - Weather
//   - Public OLED API
//
// سخت‌افزار واقعی OLED در:
//     Ava_OLED.cpp
//
// مدیریت framebuffer در:
//     Ava_DisplayAdapter
//
// موتور چشم و انیمیشن در:
//     Eye / EyeDrawer / Animation Engine
//
// ==================================================

// ==================================================
// DISPLAY MODES
// ==================================================

enum AvaOLEDMode : uint8_t {


AVA_OLED_NORMAL = 0,
AVA_OLED_BATTERY,
AVA_OLED_TIME,
AVA_OLED_WEATHER,
AVA_OLED_DEBUG


};

// ==================================================
// EYE STYLES
// ==================================================

enum AvaOledEyeStyle : uint8_t {


AVA_OLED_EYES_DEFAULT = 0,
AVA_OLED_EYES_HAPPY,
AVA_OLED_EYES_SAD,
AVA_OLED_EYES_SLEEPY,
AVA_OLED_EYES_THINKING,
AVA_OLED_EYES_LISTENING,
AVA_OLED_EYES_SURPRISED


};

// ==================================================
// GAZE
//
// Range:
//   X = 0..100
//   Y = 0..100
// ==================================================

struct AvaOLEDGaze {


int16_t x = 50;
int16_t y = 50;


};

// ==================================================
// EYE / SCREEN LAYOUT
// ==================================================

struct AvaOLEDLayout {


// ------------------------------------------------
// Screen
// ------------------------------------------------

int16_t screenWidth  = 128;
int16_t screenHeight = 64;


// ------------------------------------------------
// Left eye
// ------------------------------------------------

int16_t leftX = 18;
int16_t leftY = 18;


// ------------------------------------------------
// Right eye
// ------------------------------------------------

int16_t rightX = 74;
int16_t rightY = 18;


// ------------------------------------------------
// Base eye dimensions
// ------------------------------------------------

int16_t eyeWidth  = 36;
int16_t eyeHeight = 28;


// ------------------------------------------------
// Base corner radius
// ------------------------------------------------

int16_t cornerRadius = 8;


// ------------------------------------------------
// Status UI area
// ------------------------------------------------

int16_t statusY = 52;


};

// ==================================================
// OLED STATE
// ==================================================

struct AvaOLEDState {


// ------------------------------------------------
// Hardware
// ------------------------------------------------

bool initialized = false;
bool available   = false;


// ------------------------------------------------
// Display mode
// ------------------------------------------------

AvaOLEDMode mode = AVA_OLED_NORMAL;


// ------------------------------------------------
// Eye style
// ------------------------------------------------

AvaOledEyeStyle eyeStyle =
    AVA_OLED_EYES_DEFAULT;


// ------------------------------------------------
// Gaze
// ------------------------------------------------

AvaOLEDGaze gaze;


// ------------------------------------------------
// Layout
// ------------------------------------------------

AvaOLEDLayout layout;


// ------------------------------------------------
// Battery
// ------------------------------------------------

uint8_t batteryPercent = 100;


// ------------------------------------------------
// Brightness
// ------------------------------------------------

uint8_t brightness = 255;


// ------------------------------------------------
// Last update timestamp
// ------------------------------------------------

uint32_t lastUpdateMs = 0;


};

// ==================================================
// SINGLETON STATE ACCESS
// ==================================================

inline AvaOLEDState& avaOLED()
{
static AvaOLEDState state;


return state;


}

// ==================================================
// INITIALIZATION
// ==================================================

void avaOLEDBegin();

// ==================================================
// INITIALIZATION / AVAILABILITY
// ==================================================

inline bool avaOLEDIsInitialized()
{
return avaOLED().initialized;
}

inline bool avaOLEDIsAvailable()
{
return avaOLED().available;
}

inline void avaOledSetAvailable(
bool available
)
{
avaOLED().available = available;
}

// ==================================================
// DISPLAY MODE
// ==================================================

inline void avaOLEDSetMode(
AvaOLEDMode mode
)
{
avaOLED().mode = mode;
}

inline AvaOLEDMode avaOLEDGetMode()
{
return avaOLED().mode;
}

// ==================================================
// EYE STYLE
// ==================================================

inline void avaOLEDSetEyeStyle(
AvaOledEyeStyle style
)
{
avaOLED().eyeStyle = style;
}

inline AvaOledEyeStyle avaOLEDGetEyeStyle()
{
return avaOLED().eyeStyle;
}

// ==================================================
// GAZE
// ==================================================

inline void avaOLEDSetGaze(
int x,
int y
)
{
AvaOLEDState& o = avaOLED();


o.gaze.x = constrain(
    x,
    0,
    100
);

o.gaze.y = constrain(
    y,
    0,
    100
);


}

inline AvaOLEDGaze avaOLEDGetGaze()
{
return avaOLED().gaze;
}

// ==================================================
// BATTERY
// ==================================================

inline void avaOLEDSetBattery(
uint8_t percent
)
{
avaOLED().batteryPercent =
constrain(
percent,
0,
100
);
}

inline uint8_t avaOLEDGetBattery()
{
return avaOLED().batteryPercent;
}

// ==================================================
// BRIGHTNESS
// ==================================================

inline void avaOLEDSetBrightness(
uint8_t brightness
)
{
avaOLED().brightness =
brightness;
}

inline uint8_t avaOLEDGetBrightness()
{
return avaOLED().brightness;
}

// ==================================================
// LAYOUT
// ==================================================

inline AvaOLEDLayout& avaOLEDLayout()
{
return avaOLED().layout;
}

// ==================================================
// TIME
//
// فرمت پیشنهادی:
//
//   HH:MM
//
// یا:
//
//   HH:MM:SS
//
// ==================================================

void avaOLEDSetTime(
const String& time
);

String avaOLEDGetTime();

bool avaOLEDHasTime();

void avaOLEDRenderTime();

// ==================================================
// WEATHER
// ==================================================
//
// Weather از Ava_WiFi می‌آید.
//
// هنگام Weather Mode:
//
//   - چشم‌ها کاملاً حذف می‌شوند
//   - فقط Weather UI نمایش داده می‌شود
//   - مدت نمایش: 10 ثانیه
//   - سپس کنترل به Eye Engine برمی‌گردد
//
// ==================================================

void avaOLEDShowWeather();

void avaOLEDRenderWeather();

bool avaOLEDWeatherActive();

// ==================================================
// UPDATE
// ==================================================

void avaOLEDUpdate();

// ==================================================
// CLEAR
// ==================================================

void avaOLEDClear();

// ==================================================
// RENDER
// ==================================================

void avaOLEDRender();

// ==================================================
// DEBUG NAMES
// ==================================================

inline const char* avaOLEDModeName(
AvaOLEDMode mode
)
{
switch (mode)
{
case AVA_OLED_NORMAL:
return "NORMAL";


    case AVA_OLED_BATTERY:
        return "BATTERY";

    case AVA_OLED_TIME:
        return "TIME";

    case AVA_OLED_WEATHER:
        return "WEATHER";

    case AVA_OLED_DEBUG:
        return "DEBUG";

    default:
        return "UNKNOWN";
}


}

inline const char* avaOLEDStyleName(
AvaOledEyeStyle style
)
{
switch (style)
{
case AVA_OLED_EYES_DEFAULT:
return "DEFAULT";


    case AVA_OLED_EYES_HAPPY:
        return "HAPPY";

    case AVA_OLED_EYES_SAD:
        return "SAD";

    case AVA_OLED_EYES_SLEEPY:
        return "SLEEPY";

    case AVA_OLED_EYES_THINKING:
        return "THINKING";

    case AVA_OLED_EYES_LISTENING:
        return "LISTENING";

    case AVA_OLED_EYES_SURPRISED:
        return "SURPRISED";

    default:
        return "UNKNOWN";
}


}

// ==================================================
// DEBUG
// ==================================================

void avaOLEDDebugPrint();

#endif
