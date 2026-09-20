#ifndef AVA_EYES_H
#define AVA_EYES_H

#include <Arduino.h>

#include "Ava_DisplayAdapter.h"
#include "Ava_OLED.h"
#include "Eye.h"
#include "BlinkAssistant.h"
#include "EyePresets.h"

// ==================================================
// AVA EYES
// --------------------------------------------------
// مسئولیت:
//   - Eye State
//   - Eye Expression
//   - Gaze Direction
//   - Manual Gaze Lock
//   - Natural Gaze
//   - Eye Rendering
// ==================================================


// ==================================================
// EYE STATES
// ==================================================

enum EyeState {
  EYES_CALM,
  EYES_HAPPY,
  EYES_SAD,
  EYES_SLEEPY,
  EYES_THINKING,
  EYES_LISTENING,
  EYES_SURPRISED,
  EYES_BLINK
};

static EyeState avaCurrentEyeState = EYES_CALM;


// ==================================================
// AVA REFERENCE EMOTIONS
// --------------------------------------------------
// Based on playfultechnology/esp32-eyes
// ==================================================

enum AvaEyeEmotion {

    AVA_EMOTION_NORMAL = 0,
    AVA_EMOTION_ANGRY,
    AVA_EMOTION_GLEE,
    AVA_EMOTION_HAPPY,

    AVA_EMOTION_SAD_REFERENCE,

    AVA_EMOTION_WORRIED,
    AVA_EMOTION_FOCUSED,
    AVA_EMOTION_ANNOYED,
    AVA_EMOTION_SURPRISED,
    AVA_EMOTION_SKEPTIC,
    AVA_EMOTION_FRUSTRATED,
    AVA_EMOTION_UNIMPRESSED,
    AVA_EMOTION_SLEEPY,
    AVA_EMOTION_SUSPICIOUS,
    AVA_EMOTION_SQUINT,
    AVA_EMOTION_FURIOUS,
    AVA_EMOTION_SCARED,
    AVA_EMOTION_AWE,

    AVA_EMOTION_COUNT
};


// ==================================================
// EYE FRAME NAMES
// --------------------------------------------------
// API قدیمی پروژه حفظ شده.
// ==================================================

inline const char* getEyesFrame(EyeState state)
{
  switch (state)
  {
    case EYES_CALM:
      return "•     •";

    case EYES_HAPPY:
      return "^     ^";

    case EYES_SAD:
      return "-     -";

    case EYES_SLEEPY:
      return "-*-   -*-";

    case EYES_THINKING:
      return "o     o";

    case EYES_LISTENING:
      return "◉     ◉";

    case EYES_SURPRISED:
      return "O     O";

    case EYES_BLINK:
      return "-     -";

    default:
      return "•     •";
  }
}


// ==================================================
// GAZE DIRECTIONS
// ==================================================

enum AvaGazeDirection {
  GAZE_CENTER,
  GAZE_LEFT,
  GAZE_RIGHT,
  GAZE_UP,
  GAZE_DOWN,
  GAZE_UP_LEFT,
  GAZE_UP_RIGHT,
  GAZE_DOWN_LEFT,
  GAZE_DOWN_RIGHT
};

static AvaGazeDirection avaCurrentGaze = GAZE_CENTER;


// ==================================================
// MANUAL GAZE LOCK
// ==================================================

static bool avaGazeManualLock = false;


// ==================================================
// GAZE OFFSET
// --------------------------------------------------
// API قدیمی برای compatibility حفظ شده.
// ==================================================

struct AvaGazeOffset {
  int16_t x;
  int16_t y;
};


// ==================================================
// GAZE OFFSET MAP
// ==================================================

inline AvaGazeOffset getGazeOffset(AvaGazeDirection gaze)
{
  switch (gaze)
  {
    case GAZE_LEFT:
      return { -6, 0 };

    case GAZE_RIGHT:
      return { 6, 0 };

    case GAZE_UP:
      return { 0, -4 };

    case GAZE_DOWN:
      return { 0, 4 };

    case GAZE_UP_LEFT:
      return { -6, -4 };

    case GAZE_UP_RIGHT:
      return { 6, -4 };

    case GAZE_DOWN_LEFT:
      return { -6, 4 };

    case GAZE_DOWN_RIGHT:
      return { 6, 4 };

    case GAZE_CENTER:
    default:
      return { 0, 0 };
  }
}


// ==================================================
// GAZE NAME
// ==================================================

inline const char* getGazeName(AvaGazeDirection gaze)
{
  switch (gaze)
  {
    case GAZE_CENTER:
      return "CENTER";

    case GAZE_LEFT:
      return "LEFT";

    case GAZE_RIGHT:
      return "RIGHT";

    case GAZE_UP:
      return "UP";

    case GAZE_DOWN:
      return "DOWN";

    case GAZE_UP_LEFT:
      return "UP_LEFT";

    case GAZE_UP_RIGHT:
      return "UP_RIGHT";

    case GAZE_DOWN_LEFT:
      return "DOWN_LEFT";

    case GAZE_DOWN_RIGHT:
      return "DOWN_RIGHT";

    default:
      return "CENTER";
  }
}


// ==================================================
// EXTERNAL EYE OBJECTS
// ==================================================

extern Eye& avaLeftEye;
extern Eye& avaRightEye;
extern BlinkAssistant& avaBlinkAssistant;


// ==================================================
// EYE CENTERS
// ==================================================

static constexpr int16_t AVA_LEFT_EYE_CENTER_X  = 38;
static constexpr int16_t AVA_LEFT_EYE_CENTER_Y  = 32;

static constexpr int16_t AVA_RIGHT_EYE_CENTER_X = 90;
static constexpr int16_t AVA_RIGHT_EYE_CENTER_Y = 32;


// ==================================================
// APPLY CURRENT GAZE
// --------------------------------------------------
// پیاده‌سازی واقعی در Ava_Eyes.cpp
//
// این تابع نباید داخل avaRenderEyeFrame()
// صدا زده شود.
// ==================================================

void avaApplyCurrentGaze();


// ==================================================
// RENDER CURRENT EYE FRAME
// --------------------------------------------------
// فقط Render.
// هیچ Gaze یا Animation را Restart نمی‌کند.
// ==================================================

inline void avaRenderEyeFrame()
{
  // TIME / WEATHER own the OLED.
  if (
      avaOLED().mode == AVA_OLED_TIME ||
      avaOLED().mode == AVA_OLED_WEATHER
  )
  {
    return;
  }

  if (!AvaDisplayAdapter::isReady())
  {
    return;
  }

  AvaDisplayAdapter::beginFrame();

  avaLeftEye.Draw();
  avaRightEye.Draw();

  AvaDisplayAdapter::endFrame();
}


// ==================================================
// MANUAL GAZE
// ==================================================

inline void setGaze(AvaGazeDirection gaze)
{
  avaCurrentGaze = gaze;

  // Manual gaze owns the gaze.
  avaGazeManualLock = true;

  Serial.print("Gaze: ");
  Serial.println(getGazeName(gaze));

  // اعمال نگاه توسط LookAssistant.
  avaApplyCurrentGaze();

  // نمایش فوری.
  avaRenderEyeFrame();
}


// ==================================================
// NATURAL GAZE
// ==================================================

inline void setNaturalGaze(AvaGazeDirection gaze)
{
  if (avaGazeManualLock)
  {
    return;
  }

  // اگر همان جهت فعلی است، کاری نکن.
  if (avaCurrentGaze == gaze)
  {
    return;
  }

  avaCurrentGaze = gaze;

  Serial.print("Natural Gaze: ");
  Serial.println(getGazeName(gaze));

  // اعمال نگاه توسط LookAssistant.
  avaApplyCurrentGaze();

  avaRenderEyeFrame();
}


// ==================================================
// ENABLE NATURAL GAZE
// ==================================================

inline void avaEnableNaturalGaze()
{
  avaGazeManualLock = false;

  Serial.println("Gaze: AUTO");
}


// ==================================================
// MANUAL LOCK STATUS
// ==================================================

inline bool avaIsGazeManualLocked()
{
  return avaGazeManualLock;
}


// ==================================================
// GET CURRENT GAZE
// ==================================================

inline AvaGazeDirection getCurrentGaze()
{
  return avaCurrentGaze;
}


// ==================================================
// EXPLICIT GETTER
// ==================================================

inline AvaGazeDirection avaGetCurrentGaze()
{
  return avaCurrentGaze;
}


// ==================================================
// GAZE TARGET
// ==================================================

inline void setGazeTarget(uint8_t x, uint8_t y)
{
  AvaGazeDirection direction = GAZE_CENTER;


  // ==================================================
  // LEFT
  // ==================================================

  if (x < 35)
  {
    if (y < 35)
    {
      direction = GAZE_UP_LEFT;
    }
    else if (y > 65)
    {
      direction = GAZE_DOWN_LEFT;
    }
    else
    {
      direction = GAZE_LEFT;
    }
  }


  // ==================================================
  // RIGHT
  // ==================================================

  else if (x > 65)
  {
    if (y < 35)
    {
      direction = GAZE_UP_RIGHT;
    }
    else if (y > 65)
    {
      direction = GAZE_DOWN_RIGHT;
    }
    else
    {
      direction = GAZE_RIGHT;
    }
  }


  // ==================================================
  // CENTER
  // ==================================================

  else
  {
    if (y < 35)
    {
      direction = GAZE_UP;
    }
    else if (y > 65)
    {
      direction = GAZE_DOWN;
    }
    else
    {
      direction = GAZE_CENTER;
    }
  }

  setGaze(direction);
}


// ==================================================
// NATURAL GAZE TICK
// ==================================================

inline void avaNaturalGazeTick()
{
  static unsigned long lastGazeChange = 0;

  const unsigned long now = millis();


  // ==================================================
  // MANUAL PRIORITY
  // ==================================================

  if (avaGazeManualLock)
  {
    return;
  }


  // ==================================================
  // TIMING
  // ==================================================

  if (now - lastGazeChange < 2000UL)
  {
    return;
  }


  // ==================================================
  // RANDOM DECISION
  // ==================================================

  if (random(0, 100) >= 25)
  {
    return;
  }


  // ==================================================
  // PICK NATURAL DIRECTION
  // ==================================================

  AvaGazeDirection newGaze =
      static_cast<AvaGazeDirection>(random(0, 9));


  // ==================================================
  // AVOID SAME DIRECTION
  // ==================================================

  if (newGaze == avaCurrentGaze)
  {
    lastGazeChange = now;
    return;
  }


  // ==================================================
  // APPLY NATURAL GAZE
  // ==================================================

  setNaturalGaze(newGaze);

  lastGazeChange = now;
}


// ==================================================
// EYE EXPRESSION
// --------------------------------------------------
// این بخش مخصوص API قدیمی AVA است.
//
// مهم:
// EYES_SAD = HELL
// و عمداً تغییر نکرده.
// ==================================================

inline void renderEyes(EyeState state)
{
  avaCurrentEyeState = state;

  Serial.print("Eyes: ");
  Serial.println(getEyesFrame(state));

  if (!AvaDisplayAdapter::isReady())
  {
    return;
  }


  EyeConfig preset{};

  preset.OffsetX = 0;
  preset.OffsetY = 0;

  preset.Width = 38;
  preset.Height = 28;

  preset.Slope_Top = 0.0f;
  preset.Slope_Bottom = 0.0f;

  preset.Radius_Top = 8;
  preset.Radius_Bottom = 8;

  preset.Inverse_Radius_Top = 0;
  preset.Inverse_Radius_Bottom = 0;

  preset.Inverse_Offset_Top = 0;
  preset.Inverse_Offset_Bottom = 0;


  // ==================================================
  // EXPRESSION PRESETS
  // ==================================================

  switch (state)
  {
    case EYES_HAPPY:

      preset.Height = 18;
      preset.Radius_Top = 8;
      preset.Radius_Bottom = 8;
      preset.Slope_Top = 0.25f;

      break;


    case EYES_SAD:

      // ==================================================
      // HELL MODE
      // ==================================================

      preset.Slope_Top = -0.25f;

      break;


    case EYES_SLEEPY:

      preset.Height = 8;
      preset.Radius_Top = 3;
      preset.Radius_Bottom = 3;

      break;


    case EYES_SURPRISED:

      preset.Width = 30;
      preset.Height = 38;
      preset.Radius_Top = 15;
      preset.Radius_Bottom = 15;

      break;


    case EYES_BLINK:

      break;


    case EYES_CALM:
    case EYES_THINKING:
    case EYES_LISTENING:

    default:

      break;
  }


  // ==================================================
  // MIRRORING
  // --------------------------------------------------
  // API قدیمی AVA حفظ شده.
  // ==================================================

  avaLeftEye.SetMirrored(false);
  avaRightEye.SetMirrored(true);


  // ==================================================
  // APPLY EXPRESSION
  // ==================================================

  avaLeftEye.ApplyPreset(preset);
  avaRightEye.ApplyPreset(preset);


  // ==================================================
  // RENDER
  // ==================================================

  avaRenderEyeFrame();
}


// ==================================================
// CURRENT EXPRESSION
// ==================================================

inline EyeState avaGetCurrentEyeState()
{
  return avaCurrentEyeState;
}


// ==================================================
// APPLY REFERENCE EMOTION
// --------------------------------------------------
// 18 emotions from reference engine.
// ==================================================

inline void avaApplyEmotion(AvaEyeEmotion emotion)
{
  // --------------------------------------------------
  // Clear previous emotion variations
  // --------------------------------------------------

  avaLeftEye.Variation1.Clear();
  avaLeftEye.Variation2.Clear();

  avaRightEye.Variation1.Clear();
  avaRightEye.Variation2.Clear();


  // --------------------------------------------------
  // REFERENCE EMOTIONS
  // --------------------------------------------------

  switch (emotion)
  {

    // ==================================================
    // NORMAL
    // ==================================================

    case AVA_EMOTION_NORMAL:

      avaRightEye.Variation1.Values.Height = 3;
      avaRightEye.Variation2.Values.Width = 1;

      avaLeftEye.Variation1.Values.Height = 2;
      avaLeftEye.Variation2.Values.Width = 2;

      avaRightEye.Variation1.Animation.SetTriangle(
        1000,
        0
      );

      avaLeftEye.Variation1.Animation.SetTriangle(
        1000,
        0
      );

      avaRightEye.TransitionTo(Preset_Normal);
      avaLeftEye.TransitionTo(Preset_Normal);

      break;


    // ==================================================
    // ANGRY
    // ==================================================

    case AVA_EMOTION_ANGRY:

      avaRightEye.Variation1.Values.OffsetY = 2;
      avaLeftEye.Variation1.Values.OffsetY = 2;

      avaRightEye.Variation1.Animation.SetTriangle(
        300,
        0
      );

      avaLeftEye.Variation1.Animation.SetTriangle(
        300,
        0
      );

      avaRightEye.TransitionTo(Preset_Angry);
      avaLeftEye.TransitionTo(Preset_Angry);

      break;


    // ==================================================
    // GLEE
    // ==================================================

    case AVA_EMOTION_GLEE:

      avaRightEye.Variation1.Values.OffsetY = 5;
      avaLeftEye.Variation1.Values.OffsetY = 5;

      avaRightEye.Variation1.Animation.SetTriangle(
        300,
        0
      );

      avaLeftEye.Variation1.Animation.SetTriangle(
        300,
        0
      );

      avaRightEye.TransitionTo(Preset_Glee);
      avaLeftEye.TransitionTo(Preset_Glee);

      break;


    // ==================================================
    // HAPPY
    // ==================================================

    case AVA_EMOTION_HAPPY:

      avaRightEye.TransitionTo(Preset_Happy);
      avaLeftEye.TransitionTo(Preset_Happy);

      break;


    // ==================================================
    // SAD - REFERENCE
    // --------------------------------------------------
    // این با EYES_SAD فرق دارد.
    // EYES_SAD همچنان HELL است.
    // ==================================================

    case AVA_EMOTION_SAD_REFERENCE:

      avaRightEye.TransitionTo(Preset_Sad);
      avaLeftEye.TransitionTo(Preset_Sad);

      break;


    // ==================================================
    // WORRIED
    // ==================================================

    case AVA_EMOTION_WORRIED:

      avaRightEye.TransitionTo(Preset_Worried);
      avaLeftEye.TransitionTo(Preset_Worried_Alt);

      break;


    // ==================================================
    // FOCUSED
    // ==================================================

    case AVA_EMOTION_FOCUSED:

      avaRightEye.TransitionTo(Preset_Focused);
      avaLeftEye.TransitionTo(Preset_Focused);

      break;


    // ==================================================
    // ANNOYED
    // ==================================================

    case AVA_EMOTION_ANNOYED:

      avaRightEye.TransitionTo(Preset_Annoyed);
      avaLeftEye.TransitionTo(Preset_Annoyed_Alt);

      break;


    // ==================================================
    // SURPRISED
    // ==================================================

    case AVA_EMOTION_SURPRISED:

      avaRightEye.TransitionTo(Preset_Surprised);
      avaLeftEye.TransitionTo(Preset_Surprised);

      break;


    // ==================================================
    // SKEPTIC
    // ==================================================

    case AVA_EMOTION_SKEPTIC:

      avaRightEye.TransitionTo(Preset_Skeptic);
      avaLeftEye.TransitionTo(Preset_Skeptic_Alt);

      break;


    // ==================================================
    // FRUSTRATED
    // ==================================================

    case AVA_EMOTION_FRUSTRATED:

      avaRightEye.TransitionTo(Preset_Frustrated);
      avaLeftEye.TransitionTo(Preset_Frustrated);

      break;


    // ==================================================
    // UNIMPRESSED
    // ==================================================

    case AVA_EMOTION_UNIMPRESSED:

      avaRightEye.TransitionTo(Preset_Unimpressed);
      avaLeftEye.TransitionTo(Preset_Unimpressed_Alt);

      break;


    // ==================================================
    // SLEEPY
    // ==================================================

    case AVA_EMOTION_SLEEPY:

      avaRightEye.TransitionTo(Preset_Sleepy);
      avaLeftEye.TransitionTo(Preset_Sleepy_Alt);

      break;


    // ==================================================
    // SUSPICIOUS
    // ==================================================

    case AVA_EMOTION_SUSPICIOUS:

      avaRightEye.TransitionTo(Preset_Suspicious);
      avaLeftEye.TransitionTo(Preset_Suspicious_Alt);

      break;


    // ==================================================
    // SQUINT
    // ==================================================

    case AVA_EMOTION_SQUINT:

      avaLeftEye.Variation1.Values.OffsetX = 6;
      avaLeftEye.Variation2.Values.OffsetY = 6;

      avaRightEye.TransitionTo(Preset_Squint);
      avaLeftEye.TransitionTo(Preset_Squint_Alt);

      break;


    // ==================================================
    // FURIOUS
    // ==================================================

    case AVA_EMOTION_FURIOUS:

      avaRightEye.TransitionTo(Preset_Furious);
      avaLeftEye.TransitionTo(Preset_Furious);

      break;


    // ==================================================
    // SCARED
    // ==================================================

    case AVA_EMOTION_SCARED:

      avaRightEye.TransitionTo(Preset_Scared);
      avaLeftEye.TransitionTo(Preset_Scared);

      break;


    // ==================================================
    // AWE
    // ==================================================

    case AVA_EMOTION_AWE:

      avaRightEye.TransitionTo(Preset_Awe);
      avaLeftEye.TransitionTo(Preset_Awe);

      break;


    // ==================================================
    // FALLBACK
    // ==================================================

    default:

      avaRightEye.TransitionTo(Preset_Normal);
      avaLeftEye.TransitionTo(Preset_Normal);

      break;
  }
}



// MY GAMES entry animation: HAPPY -> blink -> NORMAL + GAZE_DOWN.
void avaMyGamesEnter();
void avaMyGamesIntroUpdate();

#endif
