#ifndef AVA_EYEDRAWER_H
#define AVA_EYEDRAWER_H

#include <Arduino.h>
#include "Ava_DisplayAdapter.h"
#include "EyeConfig.h"

// ==================================================
// AVA EYE DRAWER
//
// Renderer مستقل از سخت‌افزار.
//
// Eye Engine
//     ↓
// EyeDrawer
//     ↓
// AvaDisplayAdapter
//     ↓
// Adafruit SSD1306
//
// این کلاس نباید مستقیماً با U8g2 یا SSD1306 کار کند.
// تمام عملیات نمایش از AvaDisplayAdapter عبور می‌کند.
// ==================================================

enum CornerType {
    T_R,
    T_L,
    B_L,
    B_R
};

class EyeDrawer {

public:

    // ==================================================
    // Draw eye
    // ==================================================

    static void Draw(
        int16_t centerX,
        int16_t centerY,
        EyeConfig* config
    ) {

        if (config == nullptr) {
            return;
        }

        if (!AvaDisplayAdapter::isReady()) {
            return;
        }

        // --------------------------------------------------
        // Local geometry values
        //
        // مهم:
        // هرگز config را مستقیماً تغییر نمی‌دهیم.
        // --------------------------------------------------

        int32_t radiusTop =
            config->Radius_Top;

        int32_t radiusBottom =
            config->Radius_Bottom;

        // --------------------------------------------------
        // Calculate slopes
        // --------------------------------------------------

        int32_t deltaYTop =
            config->Height * config->Slope_Top / 2.0f;

        int32_t deltaYBottom =
            config->Height * config->Slope_Bottom / 2.0f;

        int32_t totalHeight =
            config->Height +
            deltaYTop -
            deltaYBottom;

        // --------------------------------------------------
        // Prevent radius overflow
        // --------------------------------------------------

        if (
            radiusTop > 0 &&
            radiusBottom > 0 &&
            totalHeight - 1 <
                radiusTop + radiusBottom
        ) {

            int32_t radiusSum =
                radiusTop + radiusBottom;

            radiusTop =
                radiusTop * (totalHeight - 1)
                / radiusSum;

            radiusBottom =
                radiusBottom * (totalHeight - 1)
                / radiusSum;
        }

        // --------------------------------------------------
        // Calculate inner corners
        // --------------------------------------------------

        int32_t TLcY =
            centerY +
            config->OffsetY -
            config->Height / 2 +
            radiusTop -
            deltaYTop;

        int32_t TLcX =
            centerX +
            config->OffsetX -
            config->Width / 2 +
            radiusTop;

        int32_t TRcY =
            centerY +
            config->OffsetY -
            config->Height / 2 +
            radiusTop +
            deltaYTop;

        int32_t TRcX =
            centerX +
            config->OffsetX +
            config->Width / 2 -
            radiusTop;

        int32_t BLcY =
            centerY +
            config->OffsetY +
            config->Height / 2 -
            radiusBottom -
            deltaYBottom;

        int32_t BLcX =
            centerX +
            config->OffsetX -
            config->Width / 2 +
            radiusBottom;

        int32_t BRcY =
            centerY +
            config->OffsetY +
            config->Height / 2 -
            radiusBottom +
            deltaYBottom;

        int32_t BRcX =
            centerX +
            config->OffsetX +
            config->Width / 2 -
            radiusBottom;

        // --------------------------------------------------
        // Interior extents
        // --------------------------------------------------

        int32_t minCX =
            min(TLcX, BLcX);

        int32_t maxCX =
            max(TRcX, BRcX);

        int32_t minCY =
            min(TLcY, TRcY);

        int32_t maxCY =
            max(BLcY, BRcY);

        // --------------------------------------------------
        // Main eye body
        // --------------------------------------------------

        FillRectangle(
            minCX,
            minCY,
            maxCX,
            maxCY,
            1
        );

        // --------------------------------------------------
        // Extend body toward rounded corners
        // --------------------------------------------------

        FillRectangle(
            TRcX,
            TRcY,
            BRcX + radiusBottom,
            BRcY,
            1
        );

        FillRectangle(
            TLcX - radiusTop,
            TLcY,
            BLcX,
            BLcY,
            1
        );

        FillRectangle(
            TLcX,
            TLcY - radiusTop,
            TRcX,
            TRcY,
            1
        );

        FillRectangle(
            BLcX,
            BLcY,
            BRcX,
            BRcY + radiusBottom,
            1
        );

        // --------------------------------------------------
        // Top slope
        // --------------------------------------------------

        if (config->Slope_Top > 0) {

            FillRectangularTriangle(
                TLcX,
                TLcY - radiusTop,
                TRcX,
                TRcY - radiusTop,
                0
            );

            FillRectangularTriangle(
                TRcX,
                TRcY - radiusTop,
                TLcX,
                TLcY - radiusTop,
                1
            );
        }

        else if (config->Slope_Top < 0) {

            FillRectangularTriangle(
                TRcX,
                TRcY - radiusTop,
                TLcX,
                TLcY - radiusTop,
                0
            );

            FillRectangularTriangle(
                TLcX,
                TLcY - radiusTop,
                TRcX,
                TRcY - radiusTop,
                1
            );
        }

        // --------------------------------------------------
        // Bottom slope
        // --------------------------------------------------

        if (config->Slope_Bottom > 0) {

            FillRectangularTriangle(
                BRcX + radiusBottom,
                BRcY + radiusBottom,
                BLcX - radiusBottom,
                BLcY + radiusBottom,
                0
            );

            FillRectangularTriangle(
                BLcX - radiusBottom,
                BLcY + radiusBottom,
                BRcX + radiusBottom,
                BRcY + radiusBottom,
                1
            );
        }

        else if (config->Slope_Bottom < 0) {

            FillRectangularTriangle(
                BLcX - radiusBottom,
                BLcY + radiusBottom,
                BRcX + radiusBottom,
                BRcY + radiusBottom,
                0
            );

            FillRectangularTriangle(
                BRcX + radiusBottom,
                BRcY + radiusBottom,
                BLcX - radiusBottom,
                BLcY + radiusBottom,
                1
            );
        }

        // --------------------------------------------------
        // Rounded corners
        // --------------------------------------------------

        if (radiusTop > 0) {

            FillEllipseCorner(
                T_L,
                TLcX,
                TLcY,
                radiusTop,
                radiusTop,
                1
            );

            FillEllipseCorner(
                T_R,
                TRcX,
                TRcY,
                radiusTop,
                radiusTop,
                1
            );
        }

        if (radiusBottom > 0) {

            FillEllipseCorner(
                B_L,
                BLcX,
                BLcY,
                radiusBottom,
                radiusBottom,
                1
            );

            FillEllipseCorner(
                B_R,
                BRcX,
                BRcY,
                radiusBottom,
                radiusBottom,
                1
            );
        }
    }

    // ==================================================
    // Rounded corner renderer
    // ==================================================

    static void FillEllipseCorner(
        CornerType corner,
        int16_t x0,
        int16_t y0,
        int32_t rx,
        int32_t ry,
        uint16_t color
    ) {

        if (rx < 2 || ry < 2) {
            return;
        }

        int32_t x;
        int32_t y;

        int32_t rx2 = rx * rx;
        int32_t ry2 = ry * ry;

        int32_t fx2 = 4 * rx2;
        int32_t fy2 = 4 * ry2;

        int32_t s;

        // --------------------------------------------------
        // Top Right
        // --------------------------------------------------

        if (corner == T_R) {

            for (
                x = 0,
                y = ry,
                s = 2 * ry2 +
                    rx2 * (1 - 2 * ry);

                ry2 * x <= rx2 * y;

                x++
            ) {

                AvaDisplayAdapter::setDrawColor(color);

                AvaDisplayAdapter::drawHLine(
                    x0,
                    y0 - y,
                    x
                );

                if (s >= 0) {

                    s += fx2 * (1 - y);
                    y--;
                }

                s += ry2 * ((4 * x) + 6);
            }

            for (
                x = rx,
                y = 0,
                s = 2 * rx2 +
                    ry2 * (1 - 2 * rx);

                rx2 * y <= ry2 * x;

                y++
            ) {

                AvaDisplayAdapter::setDrawColor(color);

                AvaDisplayAdapter::drawHLine(
                    x0,
                    y0 - y,
                    x
                );

                if (s >= 0) {

                    s += fy2 * (1 - x);
                    x--;
                }

                s += rx2 * ((4 * y) + 6);
            }
        }

        // --------------------------------------------------
        // Bottom Right
        // --------------------------------------------------

        else if (corner == B_R) {

            for (
                x = 0,
                y = ry,
                s = 2 * ry2 +
                    rx2 * (1 - 2 * ry);

                ry2 * x <= rx2 * y;

                x++
            ) {

                AvaDisplayAdapter::setDrawColor(color);

                AvaDisplayAdapter::drawHLine(
                    x0,
                    y0 + y - 1,
                    x
                );

                if (s >= 0) {

                    s += fx2 * (1 - y);
                    y--;
                }

                s += ry2 * ((4 * x) + 6);
            }

            for (
                x = rx,
                y = 0,
                s = 2 * rx2 +
                    ry2 * (1 - 2 * rx);

                rx2 * y <= ry2 * x;

                y++
            ) {

                AvaDisplayAdapter::setDrawColor(color);

                AvaDisplayAdapter::drawHLine(
                    x0,
                    y0 + y - 1,
                    x
                );

                if (s >= 0) {

                    s += fy2 * (1 - x);
                    x--;
                }

                s += rx2 * ((4 * y) + 6);
            }
        }

        // --------------------------------------------------
        // Top Left
        // --------------------------------------------------

        else if (corner == T_L) {

            for (
                x = 0,
                y = ry,
                s = 2 * ry2 +
                    rx2 * (1 - 2 * ry);

                ry2 * x <= rx2 * y;

                x++
            ) {

                AvaDisplayAdapter::setDrawColor(color);

                AvaDisplayAdapter::drawHLine(
                    x0 - x,
                    y0 - y,
                    x
                );

                if (s >= 0) {

                    s += fx2 * (1 - y);
                    y--;
                }

                s += ry2 * ((4 * x) + 6);
            }

            for (
                x = rx,
                y = 0,
                s = 2 * rx2 +
                    ry2 * (1 - 2 * rx);

                rx2 * y <= ry2 * x;

                y++
            ) {

                AvaDisplayAdapter::setDrawColor(color);

                AvaDisplayAdapter::drawHLine(
                    x0 - x,
                    y0 - y,
                    x
                );

                if (s >= 0) {

                    s += fy2 * (1 - x);
                    x--;
                }

                s += rx2 * ((4 * y) + 6);
            }
        }

        // --------------------------------------------------
        // Bottom Left
        // --------------------------------------------------

        else if (corner == B_L) {

            for (
                x = 0,
                y = ry,
                s = 2 * ry2 +
                    rx2 * (1 - 2 * ry);

                ry2 * x <= rx2 * y;

                x++
            ) {

                AvaDisplayAdapter::setDrawColor(color);

                AvaDisplayAdapter::drawHLine(
                    x0 - x,
                    y0 + y - 1,
                    x
                );

                if (s >= 0) {

                    s += fx2 * (1 - y);
                    y--;
                }

                s += ry2 * ((4 * x) + 6);
            }

            for (
                x = rx,
                y = 0,
                s = 2 * rx2 +
                    ry2 * (1 - 2 * rx);

                rx2 * y <= ry2 * x;

                y++
            ) {

                AvaDisplayAdapter::setDrawColor(color);

                AvaDisplayAdapter::drawHLine(
                    x0 - x,
                    y0 + y,
                    x
                );

                if (s >= 0) {

                    s += fy2 * (1 - x);
                    x--;
                }

                s += rx2 * ((4 * y) + 6);
            }
        }
    }

    // ==================================================
    // Rectangle
    // ==================================================

    static void FillRectangle(
        int32_t x0,
        int32_t y0,
        int32_t x1,
        int32_t y1,
        int32_t color
    ) {

        int32_t left =
            min(x0, x1);

        int32_t right =
            max(x0, x1);

        int32_t top =
            min(y0, y1);

        int32_t bottom =
            max(y0, y1);

        int32_t width =
            right - left;

        int32_t height =
            bottom - top;

        if (width <= 0 || height <= 0) {
            return;
        }

        AvaDisplayAdapter::setDrawColor(color);

        AvaDisplayAdapter::drawBox(
            left,
            top,
            width,
            height
        );
    }

    // ==================================================
    // Rectangular triangle
    // ==================================================

    static void FillRectangularTriangle(
        int32_t x0,
        int32_t y0,
        int32_t x1,
        int32_t y1,
        int32_t color
    ) {

        AvaDisplayAdapter::setDrawColor(color);

        AvaDisplayAdapter::drawTriangle(
            x0,
            y0,
            x1,
            y1,
            x1,
            y0
        );
    }

    // ==================================================
    // Generic triangle
    // ==================================================

    static void FillTriangle(
        int32_t x0,
        int32_t y0,
        int32_t x1,
        int32_t y1,
        int32_t x2,
        int32_t y2,
        int32_t color
    ) {

        AvaDisplayAdapter::setDrawColor(color);

        AvaDisplayAdapter::drawTriangle(
            x0,
            y0,
            x1,
            y1,
            x2,
            y2
        );
    }
};

#endif