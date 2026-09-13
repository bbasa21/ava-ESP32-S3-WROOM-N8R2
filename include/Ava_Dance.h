#ifndef AVA_DANCE_H
#define AVA_DANCE_H

#include <Arduino.h>
#include "Ava_Eyes.h"
#include "Ava_Types.h"

// ==================================================
// AVA Dance Styles
// ==================================================
enum AvaDanceStyle : uint8_t {
  DANCE_CUTE = 0,
  DANCE_HAPPY,
  DANCE_BANDARI,
  DANCE_FAST,
  DANCE_GENTLE
};

// ==================================================
// Helpers
// ==================================================
inline const char* avaMoodName(Mood mood) {
  switch (mood) {
    case MOOD_CALM:      return "CALM";
    case MOOD_HAPPY:     return "HAPPY";
    case MOOD_SAD:       return "SAD";
    case MOOD_SLEEPY:    return "SLEEPY";
    case MOOD_THINKING:  return "THINKING";
    case MOOD_LISTENING: return "LISTENING";
    default:             return "UNKNOWN";
  }
}

inline const char* avaDanceStyleName(AvaDanceStyle style) {
  switch (style) {
    case DANCE_CUTE:    return "CUTE";
    case DANCE_HAPPY:   return "HAPPY";
    case DANCE_BANDARI: return "BANDARI";
    case DANCE_FAST:    return "FAST";
    case DANCE_GENTLE:  return "GENTLE";
    default:            return "UNKNOWN";
  }
}

// ==================================================
// Auto detect style from music + mood
// ==================================================
inline AvaDanceStyle avaDanceAutoStyle(uint16_t bpm, uint8_t energyLevel, Mood mood) {
  if (mood == MOOD_SLEEPY || energyLevel < 20) {
    return DANCE_GENTLE;
  }

  if (bpm >= 135 || energyLevel >= 85) {
    return DANCE_FAST;
  }

  if (bpm >= 110 || energyLevel >= 65) {
    return DANCE_BANDARI;
  }

  if (mood == MOOD_HAPPY) {
    return DANCE_HAPPY;
  }

  return DANCE_CUTE;
}

// ==================================================
// Eyes choreography
// ==================================================
inline void avaDanceRenderEyes(AvaDanceStyle style, uint8_t beatIndex) {
  switch (style) {
    case DANCE_CUTE:
      if (beatIndex % 4 == 0)      renderEyes(EYES_HAPPY);
      else if (beatIndex % 4 == 1) renderEyes(EYES_THINKING);
      else if (beatIndex % 4 == 2) renderEyes(EYES_LISTENING);
      else                         renderEyes(EYES_CALM);
      break;

    case DANCE_HAPPY:
      if (beatIndex % 2 == 0)      renderEyes(EYES_HAPPY);
      else                         renderEyes(EYES_CALM);
      break;

    case DANCE_BANDARI:
      if (beatIndex % 3 == 0)      renderEyes(EYES_HAPPY);
      else if (beatIndex % 3 == 1) renderEyes(EYES_LISTENING);
      else                         renderEyes(EYES_HAPPY);
      break;

    case DANCE_FAST:
      if (beatIndex % 2 == 0)      renderEyes(EYES_LISTENING);
      else                         renderEyes(EYES_HAPPY);
      break;

    case DANCE_GENTLE:
      if (beatIndex % 2 == 0)      renderEyes(EYES_THINKING);
      else                         renderEyes(EYES_CALM);
      break;

    default:
      renderEyes(EYES_CALM);
      break;
  }
}

// ==================================================
// Wheel choreography preview
// ==================================================
inline const char* avaDanceWheelMotion(AvaDanceStyle style, uint8_t beatIndex) {
  switch (style) {
    case DANCE_CUTE:
      switch (beatIndex % 4) {
        case 0: return "Small sway left";
        case 1: return "Small sway right";
        case 2: return "Tiny forward bounce";
        default: return "Center sway";
      }

    case DANCE_HAPPY:
      return (beatIndex % 2 == 0) ? "Sway left" : "Sway right";

    case DANCE_BANDARI:
      switch (beatIndex % 4) {
        case 0: return "Quick left roll";
        case 1: return "Quick right roll";
        case 2: return "Left-right bounce";
        default: return "Fast center spin";
      }

    case DANCE_FAST:
      return (beatIndex % 2 == 0) ? "Fast left twitch" : "Fast right twitch";

    case DANCE_GENTLE:
      return (beatIndex % 2 == 0) ? "Soft left sway" : "Soft right sway";

    default:
      return "No motion";
  }
}

inline uint16_t avaDanceBeatDelay(AvaDanceStyle style) {
  switch (style) {
    case DANCE_CUTE:    return 220;
    case DANCE_HAPPY:   return 180;
    case DANCE_BANDARI: return 120;
    case DANCE_FAST:    return 80;
    case DANCE_GENTLE:  return 240;
    default:            return 200;
  }
}

// ==================================================
// Manual preview (test mode)
// ==================================================
inline void avaDancePreview(AvaDanceStyle style, uint8_t beats = 8, uint8_t energyLevel = 100) {
  if (beats == 0) return;

  Serial.println();
  Serial.println("=========== AVA DANCE MODE ===========");
  Serial.print("Mode: MANUAL");
  Serial.println();
  Serial.print("Style: ");
  Serial.println(avaDanceStyleName(style));
  Serial.print("Energy: ");
  Serial.print(energyLevel);
  Serial.println("%");
  Serial.println("Dance started...");
  Serial.println("======================================");

  if (energyLevel < 20) {
    Serial.println("Low energy detected. Dance will be softer.");
  }

  for (uint8_t i = 0; i < beats; i++) {
    Serial.print("Beat #");
    Serial.println(i + 1);

    Serial.print("Eyes: ");
    switch (style) {
      case DANCE_CUTE:
        Serial.println((i % 4 == 0) ? "HAPPY" : (i % 4 == 1) ? "THINKING" : (i % 4 == 2) ? "LISTENING" : "CALM");
        break;
      case DANCE_HAPPY:
        Serial.println((i % 2 == 0) ? "HAPPY" : "CALM");
        break;
      case DANCE_BANDARI:
        Serial.println((i % 3 == 0) ? "HAPPY" : "LISTENING");
        break;
      case DANCE_FAST:
        Serial.println((i % 2 == 0) ? "LISTENING" : "HAPPY");
        break;
      case DANCE_GENTLE:
        Serial.println((i % 2 == 0) ? "THINKING" : "CALM");
        break;
      default:
        Serial.println("CALM");
        break;
    }

    Serial.print("Wheels: ");
    Serial.println(avaDanceWheelMotion(style, i));

    avaDanceRenderEyes(style, i);

    delay(avaDanceBeatDelay(style));
  }

  renderEyes(EYES_CALM);
  Serial.println("Dance finished.");
  Serial.println("======================================");
  Serial.println();
}

// ==================================================
// Auto preview (music detected)
// ==================================================
inline void avaDanceAutoPreview(uint16_t bpm, uint8_t energyLevel, Mood mood, uint8_t beats = 8) {
  if (beats == 0) return;

  AvaDanceStyle style = avaDanceAutoStyle(bpm, energyLevel, mood);

  Serial.println();
  Serial.println("=========== AVA AUTO DANCE ===========");
  Serial.print("Mode: AUTO");
  Serial.println();
  Serial.print("BPM: ");
  Serial.println(bpm);
  Serial.print("Energy: ");
  Serial.print(energyLevel);
  Serial.println("%");
  Serial.print("Mood: ");
  Serial.println(avaMoodName(mood));
  Serial.print("Selected style: ");
  Serial.println(avaDanceStyleName(style));
  Serial.println("Music detected. Dance started...");
  Serial.println("======================================");

  for (uint8_t i = 0; i < beats; i++) {
    Serial.print("Beat #");
    Serial.println(i + 1);

    Serial.print("Eyes: ");
    switch (style) {
      case DANCE_CUTE:
        Serial.println((i % 4 == 0) ? "HAPPY" : (i % 4 == 1) ? "THINKING" : (i % 4 == 2) ? "LISTENING" : "CALM");
        break;
      case DANCE_HAPPY:
        Serial.println((i % 2 == 0) ? "HAPPY" : "CALM");
        break;
      case DANCE_BANDARI:
        Serial.println((i % 3 == 0) ? "HAPPY" : "LISTENING");
        break;
      case DANCE_FAST:
        Serial.println((i % 2 == 0) ? "LISTENING" : "HAPPY");
        break;
      case DANCE_GENTLE:
        Serial.println((i % 2 == 0) ? "THINKING" : "CALM");
        break;
      default:
        Serial.println("CALM");
        break;
    }

    Serial.print("Wheels: ");
    Serial.println(avaDanceWheelMotion(style, i));

    avaDanceRenderEyes(style, i);

    delay(avaDanceBeatDelay(style));
  }

  renderEyes(EYES_CALM);
  Serial.println("Auto dance finished.");
  Serial.println("======================================");
  Serial.println();
}

#endif