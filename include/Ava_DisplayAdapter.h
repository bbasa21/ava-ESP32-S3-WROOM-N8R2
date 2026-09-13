#ifndef AVA_DISPLAY_ADAPTER_H
#define AVA_DISPLAY_ADAPTER_H

#include <Arduino.h>
#include <U8g2lib.h>

// ==================================================
// AVA DISPLAY ADAPTER
//
// Hardware abstraction layer between:
//   Ava Eye Engine / EyeDrawer
//              ↓
//      AvaDisplayAdapter
//              ↓
//              U8g2
//              ↓
//        SSD1309 SPI
//
// U8g2-compatible drawing semantics:
//
//   color = 0 → erase / black
//   color = 1 → draw  / white
//
// EyeDrawer نباید مستقیماً U8g2 یا OLED را بشناسد.
// ==================================================

namespace AvaDisplayAdapter {

// ==================================================
// Internal state
// ==================================================

inline U8G2* &displayInstance()
{
    static U8G2* instance = nullptr;
    return instance;
}

inline uint8_t &currentDrawColor()
{
    static uint8_t color = 1;
    return color;
}

inline bool &initializedState()
{
    static bool state = false;
    return state;
}


// ==================================================
// Bind OLED
// ==================================================

inline void begin(U8G2 &oled)
{
    displayInstance() = &oled;
    currentDrawColor() = 1;
    initializedState() = true;
}


// ==================================================
// State
// ==================================================

inline bool isInitialized()
{
    return initializedState();
}


inline bool isReady()
{
    return initializedState() &&
           displayInstance() != nullptr;
}


// ==================================================
// Get display
//
// فقط برای لایه‌های پایین‌تر.
// Eye Engine نباید مستقیماً از این استفاده کند.
// ==================================================

inline U8G2* getDisplay()
{
    return displayInstance();
}


// ==================================================
// Display dimensions
// ==================================================

inline uint16_t width()
{
    if (!isReady()) {
        return 0;
    }

    return displayInstance()->getDisplayWidth();
}


inline uint16_t height()
{
    if (!isReady()) {
        return 0;
    }

    return displayInstance()->getDisplayHeight();
}


// ==================================================
// Draw color
//
// U8g2 semantics:
//
//   0 = erase
//   1 = draw
// ==================================================

inline void setDrawColor(uint8_t color)
{
    currentDrawColor() =
        (color == 0) ? 0 : 1;

    if (!isReady()) {
        return;
    }

    if (currentDrawColor() == 0) {
        displayInstance()->setDrawColor(0);
    }
    else {
        displayInstance()->setDrawColor(1);
    }
}


// ==================================================
// Get draw color
// ==================================================

inline uint8_t getDrawColor()
{
    return currentDrawColor();
}


// ==================================================
// Horizontal line
// ==================================================

inline void drawHLine(
    int16_t x,
    int16_t y,
    int16_t lineWidth
)
{
    if (!isReady() || lineWidth <= 0) {
        return;
    }

    displayInstance()->drawHLine(
        x,
        y,
        lineWidth
    );
}


// ==================================================
// Filled rectangle
// ==================================================

inline void drawBox(
    int16_t x,
    int16_t y,
    int16_t boxWidth,
    int16_t boxHeight
)
{
    if (!isReady() ||
        boxWidth <= 0 ||
        boxHeight <= 0) {
        return;
    }

    displayInstance()->drawBox(
        x,
        y,
        boxWidth,
        boxHeight
    );
}


// ==================================================
// Filled triangle
// ==================================================

inline void drawTriangle(
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1,
    int16_t x2,
    int16_t y2
)
{
    if (!isReady()) {
        return;
    }

    displayInstance()->drawTriangle(
        x0,
        y0,
        x1,
        y1,
        x2,
        y2
    );
}


// ==================================================
// Clear framebuffer
//
// فقط framebuffer را پاک می‌کند.
// ارسال واقعی با displayRefresh()
// ==================================================

inline void clear()
{
    if (!isReady()) {
        return;
    }

    displayInstance()->clearBuffer();
}


// ==================================================
// Push framebuffer to OLED
// ==================================================

inline void displayRefresh()
{
    if (!isReady()) {
        return;
    }

    displayInstance()->sendBuffer();
}


// ==================================================
// Frame helpers
// ==================================================

inline void beginFrame()
{
    if (!isReady()) {
        return;
    }

    displayInstance()->clearBuffer();

    // هر فریم با حالت رسم سفید شروع شود.
    currentDrawColor() = 1;
    displayInstance()->setDrawColor(1);
}


inline void endFrame()
{
    displayRefresh();
}


} // namespace AvaDisplayAdapter

#endif