#ifndef AVA_FONT_H
#define AVA_FONT_H

#include <Arduino.h>

// ==================================================
// AVA FONT
//
// هدف فعلی:
//   Orbitron Bold 16px
//   مخصوص Time UI روی OLED 128x64
//
// مسیر:
//
//   Time UI
//      ↓
//   Ava_Font
//      ↓
//   AvaDisplayAdapter
//      ↓
//   SSD1306
//
// فونت به صورت bitmap در:
//   Orbitron_Bold_16.h
//
// ذخیره شده است.
// ==================================================

// ==================================================
// FONT CONSTANTS
// ==================================================

#define AVA_FONT_WIDTH       16
#define AVA_FONT_HEIGHT      20

#define AVA_FONT_DEFAULT_GAP 2

// ==================================================
// FONT STATE
// ==================================================

struct AvaFontState {

bool initialized = false;

int16_t cursorX = 0;
int16_t cursorY = 0;

uint8_t gap = AVA_FONT_DEFAULT_GAP;

bool enabled = true;

};

// ==================================================
// STATE ACCESS
// ==================================================

AvaFontState& avaFont();

// ==================================================
// INITIALIZATION
// ==================================================

void avaFontBegin();

bool avaFontIsInitialized();

// ==================================================
// CONFIGURATION
// ==================================================

void avaFontSetGap(
uint8_t gap
);

uint8_t avaFontGetGap();

// ==================================================
// DRAW SINGLE GLYPH
// ==================================================

void avaFontDrawGlyph(
char character,
int16_t x,
int16_t y
);

// ==================================================
// DRAW TEXT
//
// فعلاً برای ASCII مناسب است.
// ==================================================

void avaFontDrawText(
const char* text,
int16_t x,
int16_t y
);

// ==================================================
// TEXT MEASUREMENT
// ==================================================

uint16_t avaFontTextWidth(
const char* text
);

// ==================================================
// CENTERED TEXT
// ==================================================

void avaFontDrawCentered(
const char* text,
int16_t y
);

// ==================================================
// TIME
//
// مثال:
//
//   12:45
//   08:03
//   23:59
//
// مخصوص Time UI آوا
// ==================================================

void avaFontDrawTime(
const char* timeText,
int16_t y
);

// ==================================================
// DEBUG
// ==================================================

void avaFontDebugPrint();

#endif
