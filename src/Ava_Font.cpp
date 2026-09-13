#include "Ava_Font.h"

#include "Ava_DisplayAdapter.h"
#include "Orbitron_Bold_16.h"

// ==================================================
// INTERNAL STATE
// ==================================================

AvaFontState& avaFont()
{
static AvaFontState state;
return state;
}

// ==================================================
// INITIALIZATION
// ==================================================

void avaFontBegin()
{
Serial.println("[FONT] Initializing...");


if (!AvaDisplayAdapter::isReady()) {

    Serial.println(
        "[FONT] ERROR: Display adapter not ready"
    );

    avaFont().initialized = false;
    return;
}

avaFont().initialized = true;

Serial.println(
    "[FONT] Orbitron Bold 16px READY"
);


}

bool avaFontIsInitialized()
{
return avaFont().initialized;
}

// ==================================================
// GAP
// ==================================================

void avaFontSetGap(
uint8_t gap
)
{
avaFont().gap = gap;
}

uint8_t avaFontGetGap()
{
return avaFont().gap;
}

// ==================================================
// GET GLYPH
//
// glyph_30 = '0'
// glyph_31 = '1'
// ...
// glyph_39 = '9'
// glyph_3A = ':'
//
// سایر کاراکترها فعلاً پشتیبانی نمی‌شوند.
// ==================================================

static const uint16_t* getGlyph(
char character
)
{
switch (character) {


    case '0':
        return glyph_30;

    case '1':
        return glyph_31;

    case '2':
        return glyph_32;

    case '3':
        return glyph_33;

    case '4':
        return glyph_34;

    case '5':
        return glyph_35;

    case '6':
        return glyph_36;

    case '7':
        return glyph_37;

    case '8':
        return glyph_38;

    case '9':
        return glyph_39;

    case ':':
        return glyph_3A;

    default:
        return nullptr;
}


}

// ==================================================
// DRAW SINGLE GLYPH
// ==================================================

void avaFontDrawGlyph(
char character,
int16_t x,
int16_t y
)
{
if (!avaFontIsInitialized()) {
return;
}


if (!AvaDisplayAdapter::isReady()) {
    return;
}

const uint16_t* glyph =
    getGlyph(character);

if (glyph == nullptr) {
    return;
}


// ------------------------------------------------
// هر uint16_t یک ردیف 16 بیتی است.
//
// MSB → سمت چپ
// LSB → سمت راست
// ------------------------------------------------

AvaDisplayAdapter::setDrawColor(1);

for (uint8_t row = 0;
     row < AVA_FONT_HEIGHT;
     row++) {

    uint16_t bits =
        pgm_read_word(&glyph[row]);


    for (uint8_t col = 0;
         col < AVA_FONT_WIDTH;
         col++) {

        if (bits & (0x8000 >> col)) {

            AvaDisplayAdapter::drawBox(
                x + col,
                y + row,
                1,
                1
            );
        }
    }
}


}

// ==================================================
// TEXT WIDTH
// ==================================================

uint16_t avaFontTextWidth(
const char* text
)
{
if (text == nullptr) {
return 0;
}


uint16_t width = 0;

for (size_t i = 0;
     text[i] != '\0';
     i++) {

    char c = text[i];

    if (c == ' ') {

        width +=
            AVA_FONT_WIDTH +
            avaFont().gap;

        continue;
    }

    if (getGlyph(c) != nullptr) {

        width +=
            AVA_FONT_WIDTH;

        if (text[i + 1] != '\0') {
            width += avaFont().gap;
        }
    }
}

return width;


}

// ==================================================
// DRAW TEXT
// ==================================================

void avaFontDrawText(
const char* text,
int16_t x,
int16_t y
)
{
if (!avaFontIsInitialized()) {
return;
}


if (text == nullptr) {
    return;
}

int16_t cursorX = x;

for (size_t i = 0;
     text[i] != '\0';
     i++) {

    char c = text[i];

    const uint16_t* glyph =
        getGlyph(c);

    if (glyph == nullptr) {

        // فاصله
        if (c == ' ') {
            cursorX +=
                AVA_FONT_WIDTH +
                avaFont().gap;
        }

        continue;
    }

    avaFontDrawGlyph(
        c,
        cursorX,
        y
    );

    cursorX +=
        AVA_FONT_WIDTH +
        avaFont().gap;
}

}

// ==================================================
// CENTERED TEXT
// ==================================================

void avaFontDrawCentered(
const char* text,
int16_t y
)
{
if (text == nullptr) {
return;
}


uint16_t textWidth =
    avaFontTextWidth(text);

int16_t x =
    (int16_t)(
        (AvaDisplayAdapter::width() -
         textWidth) / 2
    );

if (x < 0) {
    x = 0;
}

avaFontDrawText(
    text,
    x,
    y
);


}

// ==================================================
// DRAW TIME
//
// y = مختصات بالای bitmap
//
// مثال:
//
//   avaFontDrawTime("12:45", 22);
//
// ==================================================

void avaFontDrawTime(
const char* timeText,
int16_t y
)
{
if (timeText == nullptr) {
return;
}


avaFontDrawCentered(
    timeText,
    y
);


}

// ==================================================
// DEBUG
// ==================================================

void avaFontDebugPrint()
{
Serial.println();
Serial.println("=========== AVA FONT ===========");


Serial.print("Initialized: ");
Serial.println(
    avaFontIsInitialized()
        ? "YES"
        : "NO"
);

Serial.println(
    "Font: Orbitron Bold"
);

Serial.println(
    "Size: 16x20 bitmap"
);

Serial.print(
    "Gap: "
);

Serial.println(
    avaFont().gap
);

Serial.print(
    "Display adapter: "
);

Serial.println(
    AvaDisplayAdapter::isReady()
        ? "READY"
        : "NOT READY"
);

Serial.println(
    "Characters: 0-9 :"
);

Serial.println(
    "================================"
);

Serial.println();

}
