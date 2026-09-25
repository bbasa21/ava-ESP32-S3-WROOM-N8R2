#include "Ava_Eyes.h"
#include "BlinkAssistant.h"
#include "Ava_OLED.h"
#include "Ava_OTA_Display.h"
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
// DEBUG EYES / GAME GAZE LOCK
// ==================================================

static bool avaGameGazeLockActive = false;
static bool avaGameSavedRandomLook = true;

void avaEnableDebugEyes()
{
    // Give the Face behavior engine real emotion weights. Without this,
    // the stock engine has only Normal weighted and debug eyes appear frozen.
    avaFace.Behavior.Clear();

    for (int i = 0; i < eEmotions::EMOTIONS_COUNT; ++i)
    {
        avaFace.Behavior.SetEmotion(
            static_cast<eEmotions>(i),
            1.0f
        );
    }

    avaFace.RandomBehavior = true;
    avaFace.RandomLook = true;
    avaFace.RandomBlink = true;

    avaEnableNaturalGaze();

    Serial.println("[EYES] DEBUG EYES ENABLED.");
}

void avaEnterGameGazeLock()
{
    if (!avaGameGazeLockActive)
    {
        avaGameSavedRandomLook = avaFace.RandomLook;
        avaGameGazeLockActive = true;
    }

    avaFace.RandomLook = false;

    setGaze(GAZE_DOWN);

    Serial.println("[EYES] GAME GAZE LOCK -> GAZE_DOWN.");
}

void avaExitGameGazeLock()
{
    if (!avaGameGazeLockActive)
    {
        return;
    }

    avaFace.RandomLook = avaGameSavedRandomLook;
    avaGameGazeLockActive = false;

    avaEnableNaturalGaze();

    Serial.println("[EYES] GAME GAZE LOCK RELEASED.");
}


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
    // ==================================================
    // OTA UI OWNS THE OLED
    // --------------------------------------------------
    // While OTA is rendering its progress screen, the
    // eye renderer must not overwrite that frame.
    // ==================================================

    if (avaOTAUIIsActive())
    {
        return;
    }


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

// ==================================================
// MY GAMES ENTRY ANIMATION
// --------------------------------------------------
// HAPPY -> forced blink -> forced blink ->
// NORMAL + GAZE_DOWN
// --------------------------------------------------
// The Face behavior/look randomizers are temporarily
// paused so the HAPPY expression remains intact while
// the blink engine animates over the existing eyes.
// ==================================================

static bool avaMyGamesIntroActive = false;
static uint32_t avaMyGamesIntroStart = 0;
static bool avaMyGamesSavedRandomBehavior = true;
static bool avaMyGamesSavedRandomLook = true;
static bool avaMyGamesSecondBlinkDone = false;

void avaMyGamesEnter()
{
    // MY GAMES entry is a one-shot HAPPY reaction + blink.
    // After the intro, the gaze remains locked to GAZE_DOWN
    // until MY_GAMES_EXIT or GAME_END releases the lock.
    avaMyGamesSavedRandomBehavior = avaFace.RandomBehavior;
    avaMyGamesSavedRandomLook = avaFace.RandomLook;

    avaEnterGameGazeLock();

    avaMyGamesIntroActive = true;
    avaMyGamesIntroStart = millis();
    avaMyGamesSecondBlinkDone = false;

    avaFace.RandomBehavior = false;
    avaFace.RandomLook = false;

    avaApplyEmotion(AVA_EMOTION_HAPPY);

    avaFace.DoBlink();

    Serial.println("[MY GAMES] HAPPY + BLINK INTRO STARTED.");
}

void avaMyGamesIntroUpdate()
{
    if (!avaMyGamesIntroActive)
    {
        return;
    }

    const uint32_t elapsed =
        static_cast<uint32_t>(millis() - avaMyGamesIntroStart);

    if (!avaMyGamesSecondBlinkDone && elapsed >= 800UL)
    {
        avaFace.DoBlink();
        avaMyGamesSecondBlinkDone = true;
    }

    if (elapsed >= 1700UL)
    {
        avaApplyEmotion(AVA_EMOTION_NORMAL);
        setGaze(GAZE_DOWN);

        avaFace.RandomBehavior = avaMyGamesSavedRandomBehavior;
        // MY GAMES must keep its gaze locked down. RandomLook is
        // intentionally left disabled until MY_GAMES_EXIT.
        avaFace.RandomLook = false;

        avaMyGamesIntroActive = false;

        Serial.println("[MY GAMES] INTRO FINISHED -> NORMAL + GAZE_DOWN.");
    }
}

void avaBlinkAssistantUpdate()
{
    avaMyGamesIntroUpdate();
    avaEyesBlinkTick();
}