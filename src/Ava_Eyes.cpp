#include "Ava_Eyes.h"
#include "BlinkAssistant.h"
#include "Ava_OLED.h"
#include "Face.h"

// ==================================================
// AVA FACE ENGINE
// --------------------------------------------------
// The real ESP32-EYES reference engine instance.
// All AVA eye APIs operate on this Face.
// ==================================================

Face avaFace(
    128,
    64,
    44
);


// ==================================================
// AVA EYE API ALIASES
// --------------------------------------------------
// Keep the existing AVA API alive while using
// Face-owned Eye instances underneath.
// ==================================================

Eye& avaLeftEye  = avaFace.LeftEye;
Eye& avaRightEye = avaFace.RightEye;


// ==================================================
// AVA BLINK ASSISTANT
// --------------------------------------------------
// This is the BlinkAssistant owned by avaFace.
// ==================================================

BlinkAssistant& avaBlinkAssistant = avaFace.Blink;


// ==================================================
// APPLY CURRENT GAZE
// --------------------------------------------------
// The old gaze system used SetCenter() offsets.
//
// Now gaze is handled by the reference
// LookAssistant / EyeTransformation system.
//
// IMPORTANT:
// This function is called only when the gaze
// actually changes. It must NOT be called from
// avaRenderEyeFrame(), otherwise LookAt()
// would restart the animation every frame.
// ==================================================

void avaApplyCurrentGaze()
{
    switch (avaCurrentGaze)
    {
        case GAZE_LEFT:

            avaFace.LookLeft();

            break;


        case GAZE_RIGHT:

            avaFace.LookRight();

            break;


        case GAZE_UP:

            avaFace.LookTop();

            break;


        case GAZE_DOWN:

            avaFace.LookBottom();

            break;


        case GAZE_UP_LEFT:

            avaFace.Look.LookAt(
                1.0f,
                -1.0f
            );

            break;


        case GAZE_UP_RIGHT:

            avaFace.Look.LookAt(
                -1.0f,
                -1.0f
            );

            break;


        case GAZE_DOWN_LEFT:

            avaFace.Look.LookAt(
                1.0f,
                1.0f
            );

            break;


        case GAZE_DOWN_RIGHT:

            avaFace.Look.LookAt(
                -1.0f,
                1.0f
            );

            break;


        case GAZE_CENTER:

        default:

            avaFace.LookFront();

            break;
    }
}


// ==================================================
// AVA EYES BLINK / FACE ENGINE / RENDER TICK
// ==================================================

void avaEyesBlinkTick()
{
    // ==================================================
    // OLED TIME MODE
    // ==================================================
    //
    // When the clock owns the OLED,
    // the Eye Engine must not overwrite it.
    //
    // ==================================================

    if (
        avaOLEDGetMode() == AVA_OLED_TIME
    )
    {
        return;
    }


    // ==================================================
    // OLED WEATHER MODE
    // ==================================================

    if (
        avaOLEDGetMode() == AVA_OLED_WEATHER
    )
    {
        return;
    }


    // ==================================================
    // UPDATE FULL REFERENCE FACE ENGINE
    // ==================================================
    //
    // This updates:
    //
    //   FaceBehavior
    //   LookAssistant
    //   BlinkAssistant
    //
    // It does NOT own the OLED.
    //
    // ==================================================

    avaFace.Update();


    // ==================================================
    // SINGLE AVA RENDER PATH
    // ==================================================
    //
    // The OLED is still owned by Ava's display layer.
    //
    // ==================================================

    avaRenderEyeFrame();
}


// ==================================================
// PUBLIC BLINK / FACE ENGINE UPDATE
// ==================================================

void avaBlinkAssistantUpdate()
{
    avaEyesBlinkTick();
}