#ifndef AVA_STATUS_UI_H
#define AVA_STATUS_UI_H

#include <Arduino.h>
#include "Ava_DisplayAdapter.h"
#include "EyeConfig.h"

// ==================================================
// AVA STATUS UI
//
// مسئول:
//
//   - Digital Clock
//   - Battery percentage
//   - Status-mode eye layout
//
// این لایه به Voice وابسته نیست.
// این لایه به RTC وابسته نیست.
//
// Voice / RTC فقط مقدار ساعت یا باتری را
// به این لایه تحویل می‌دهند.
//
// Renderer سخت‌افزار فقط:
//     AvaDisplayAdapter
//
// ==================================================

namespace AvaStatusUI {

// ==================================================
// Display geometry
// ==================================================

static constexpr int16_t SCREEN_WIDTH  = 128;
static constexpr int16_t SCREEN_HEIGHT = 64;

// محدوده پایین برای Status UI
static constexpr int16_t STATUS_TOP    = 45;
static constexpr int16_t STATUS_BOTTOM = 63;

// ==================================================
// Seven-segment digit geometry
// ==================================================

static constexpr int16_t DIGIT_WIDTH  = 10;
static constexpr int16_t DIGIT_HEIGHT = 18;
static constexpr int16_t SEG_THICKNESS = 3;

static constexpr int16_t DIGIT_GAP = 4;

// ==================================================
// Basic adapter helpers
// ==================================================

inline void setWhite() {
    AvaDisplayAdapter::setDrawColor(1);
}

inline void setBlack() {
    AvaDisplayAdapter::setDrawColor(0);
}

// ==================================================
// Draw one 7-segment digit
//
// Segment layout:
//
//      AAA
//     F   B
//      GGG
//     E   C
//      DDD
// ==================================================

inline void drawDigit(
    int16_t x,
    int16_t y,
    uint8_t digit
) {
    if (digit > 9) {
        return;
    }

    static constexpr bool segments[10][7] = {
        // A     B      C      D      E      F      G
        { true,  true,  true,  true,  true,  true,  false }, // 0
        { false, true,  true, false, false, false, false }, // 1
        { true,  true,  false, true,  true, false, true  }, // 2
        { true,  true,  true,  true, false, false, true  }, // 3
        { false, true,  true, false, false, true,  true  }, // 4
        { true,  false, true,  true, false, true,  true  }, // 5
        { true,  false, true,  true, true,  true,  true  }, // 6
        { true,  true,  true, false, false, false, false }, // 7
        { true,  true,  true, true, true, true,  true  }, // 8
        { true,  true,  true, true, false, true,  true  }  // 9
    };

    const bool* s = segments[digit];

    setWhite();

    // A
    if (s[0]) {
        AvaDisplayAdapter::drawBox(
            x + SEG_THICKNESS,
            y,
            DIGIT_WIDTH - (SEG_THICKNESS * 2),
            SEG_THICKNESS
        );
    }

    // B
    if (s[1]) {
        AvaDisplayAdapter::drawBox(
            x + DIGIT_WIDTH - SEG_THICKNESS,
            y + SEG_THICKNESS,
            SEG_THICKNESS,
            (DIGIT_HEIGHT / 2) - SEG_THICKNESS
        );
    }

    // C
    if (s[2]) {
        AvaDisplayAdapter::drawBox(
            x + DIGIT_WIDTH - SEG_THICKNESS,
            y + (DIGIT_HEIGHT / 2),
            SEG_THICKNESS,
            (DIGIT_HEIGHT / 2) - SEG_THICKNESS
        );
    }

    // D
    if (s[3]) {
        AvaDisplayAdapter::drawBox(
            x + SEG_THICKNESS,
            y + DIGIT_HEIGHT - SEG_THICKNESS,
            DIGIT_WIDTH - (SEG_THICKNESS * 2),
            SEG_THICKNESS
        );
    }

    // E
    if (s[4]) {
        AvaDisplayAdapter::drawBox(
            x,
            y + (DIGIT_HEIGHT / 2),
            SEG_THICKNESS,
            (DIGIT_HEIGHT / 2) - SEG_THICKNESS
        );
    }

    // F
    if (s[5]) {
        AvaDisplayAdapter::drawBox(
            x,
            y + SEG_THICKNESS,
            SEG_THICKNESS,
            (DIGIT_HEIGHT / 2) - SEG_THICKNESS
        );
    }

    // G
    if (s[6]) {
        AvaDisplayAdapter::drawBox(
            x + SEG_THICKNESS,
            y + (DIGIT_HEIGHT / 2) - (SEG_THICKNESS / 2),
            DIGIT_WIDTH - (SEG_THICKNESS * 2),
            SEG_THICKNESS
        );
    }
}

// ==================================================
// Draw colon
// ==================================================

inline void drawColon(
    int16_t x,
    int16_t y,
    bool visible = true
) {
    if (!visible) {
        return;
    }

    setWhite();

    AvaDisplayAdapter::drawBox(
        x,
        y + 5,
        3,
        3
    );

    AvaDisplayAdapter::drawBox(
        x,
        y + 13,
        3,
        3
    );
}

// ==================================================
// Draw percent symbol
// ==================================================

inline void drawPercent(
    int16_t x,
    int16_t y
) {
    setWhite();

    // top-left dot
    AvaDisplayAdapter::drawBox(
        x,
        y,
        3,
        3
    );

    // bottom-right dot
    AvaDisplayAdapter::drawBox(
        x + 9,
        y + 15,
        3,
        3
    );

    // diagonal
    for (int16_t i = 0; i < 12; ++i) {
        AvaDisplayAdapter::drawBox(
            x + 2 + i,
            y + 3 + (i * 12 / 12),
            2,
            2
        );
    }
}

// ==================================================
// Draw clock
//
// Example:
//     12:37
//
// Style:
//     Digital / pixel
// ==================================================

inline void drawTime(
    uint8_t hour,
    uint8_t minute,
    bool colonVisible = true
) {
    if (!AvaDisplayAdapter::isReady()) {
        return;
    }

    hour   %= 24;
    minute %= 60;

    const uint8_t h1 = hour / 10;
    const uint8_t h2 = hour % 10;
    const uint8_t m1 = minute / 10;
    const uint8_t m2 = minute % 10;

    const int16_t totalWidth =
        (DIGIT_WIDTH * 4) +
        (DIGIT_GAP * 3) +
        7;

    int16_t startX =
        (SCREEN_WIDTH - totalWidth) / 2;

    const int16_t y = 27;

    AvaDisplayAdapter::beginFrame();

    drawDigit(startX, y, h1);
    startX += DIGIT_WIDTH + DIGIT_GAP;

    drawDigit(startX, y, h2);
    startX += DIGIT_WIDTH + DIGIT_GAP;

    drawColon(startX, y, colonVisible);
    startX += 7 + DIGIT_GAP;

    drawDigit(startX, y, m1);
    startX += DIGIT_WIDTH + DIGIT_GAP;

    drawDigit(startX, y, m2);

    AvaDisplayAdapter::endFrame();
}


// ==================================================
// Draw battery
//
// Example:
//
//   ┌──────────────┐
//   │█████████     │ 87%
//   └──────────────┘
//
// ==================================================

inline void drawBattery(
    uint8_t percent
) {
    if (!AvaDisplayAdapter::isReady()) {
        return;
    }

    percent = constrain(percent, 0, 100);

    const int16_t boxX = 13;
    const int16_t boxY = 48;

    const int16_t boxW = 75;
    const int16_t boxH = 12;

    const int16_t terminalW = 4;
    const int16_t terminalH = 4;

    const int16_t innerPadding = 2;

    AvaDisplayAdapter::beginFrame();

    // Outer battery
    setWhite();

    // Top
    AvaDisplayAdapter::drawHLine(
        boxX,
        boxY,
        boxW
    );

    // Bottom
    AvaDisplayAdapter::drawHLine(
        boxX,
        boxY + boxH,
        boxW
    );

    // Left
    AvaDisplayAdapter::drawHLine(
        boxX,
        boxY,
        boxH
    );

    // Right
    AvaDisplayAdapter::drawHLine(
        boxX + boxW,
        boxY,
        boxH
    );

    // Battery terminal
    AvaDisplayAdapter::drawBox(
        boxX + boxW + 1,
        boxY + 4,
        terminalW,
        terminalH
    );

    // Fill area
    const int16_t innerX =
        boxX + innerPadding;

    const int16_t innerY =
        boxY + innerPadding;

    const int16_t innerW =
        boxW - (innerPadding * 2);

    const int16_t innerH =
        boxH - (innerPadding * 2);

    int16_t fillW =
        (innerW * percent) / 100;

    if (fillW > 0) {
        AvaDisplayAdapter::drawBox(
            innerX,
            innerY,
            fillW,
            innerH
        );
    }

    // Percentage digits
    const uint8_t p1 = percent / 10;
    const uint8_t p2 = percent % 10;

    const int16_t textX = 94;
    const int16_t textY = 46;

    if (percent >= 100) {
        drawDigit(textX, textY, 1);
        drawDigit(
            textX + DIGIT_WIDTH + 1,
            textY,
            0
        );
    } else {
        drawDigit(textX, textY, p1);
        drawDigit(
            textX + DIGIT_WIDTH + 1,
            textY,
            p2
        );
    }

    drawPercent(
        textX + 22,
        textY + 2
    );

    AvaDisplayAdapter::endFrame();
}


// ==================================================
// Configure eyes for status mode
//
// چشم‌ها در Status Mode کم‌ارتفاع‌تر می‌شوند.
// خود Animation Engine بعداً همین Config را
// transition می‌دهد.
// ==================================================

inline void applyStatusEyeConfig(
    EyeConfig& leftEye,
    EyeConfig& rightEye
) {
    leftEye.Height = 9;
    rightEye.Height = 9;

    leftEye.Width = 36;
    rightEye.Width = 36;

    leftEye.Radius_Top = 4;
    rightEye.Radius_Top = 4;

    leftEye.Radius_Bottom = 4;
    rightEye.Radius_Bottom = 4;

    leftEye.Slope_Top = 0;
    rightEye.Slope_Top = 0;

    leftEye.Slope_Bottom = 0;
    rightEye.Slope_Bottom = 0;
}


// ==================================================
// Restore normal eye dimensions
// ==================================================

inline void applyNormalEyeConfig(
    EyeConfig& leftEye,
    EyeConfig& rightEye
) {
    leftEye.Height = 28;
    rightEye.Height = 28;

    leftEye.Width = 36;
    rightEye.Width = 36;

    leftEye.Radius_Top = 8;
    rightEye.Radius_Top = 8;

    leftEye.Radius_Bottom = 8;
    rightEye.Radius_Bottom = 8;
}

} // namespace AvaStatusUI

#endif