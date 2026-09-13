#ifndef AVA_BEHAVIOR_H
#define AVA_BEHAVIOR_H

#include <Arduino.h>
#include "Ava_Types.h"
#include "Ava_Eyes.h"
#include "Ava_Touch.h"

// ================================
// AVA Behavior System
// ================================

// زمان‌های رفتار خودکار
#define AVA_IDLE_THINK_AFTER_MS   20000UL
#define AVA_IDLE_SLEEP_AFTER_MS   40000UL
#define AVA_IDLE_PROMPT_AFTER_MS  90000UL

// ================================
// رفتار خودکار آوا
// ================================
inline void avaAutoBehaviorTick(unsigned long now,
                                unsigned long &lastInteractionTime,
                                RobotState &currentState,
                                Mood &currentMood) {

  // فقط وقتی آوا در حالت Idle است
  if (currentState != STATE_IDLE) {
    return;
  }

  unsigned long idleTime = now - lastInteractionTime;

  // ================================
  // بعد از 20 ثانیه: THINKING
  // ================================
  if (idleTime > AVA_IDLE_THINK_AFTER_MS &&
      idleTime <= AVA_IDLE_SLEEP_AFTER_MS) {

    // فقط یک بار هنگام ورود به حالت THINKING اجرا شود
    if (currentMood != MOOD_THINKING) {
      currentMood = MOOD_THINKING;
      renderEyes(EYES_THINKING);
    }
  }

  // ================================
  // بعد از 40 ثانیه: SLEEPY
  // ================================
  else if (idleTime > AVA_IDLE_SLEEP_AFTER_MS &&
           idleTime <= AVA_IDLE_PROMPT_AFTER_MS) {

    // فقط یک بار هنگام ورود به حالت SLEEPY اجرا شود
    if (currentMood != MOOD_SLEEPY) {
      currentMood = MOOD_SLEEPY;
      renderEyes(EYES_SLEEPY);
    }
  }

  // ================================
  // بعد از 90 ثانیه: PROMPT
  // ================================
  else if (idleTime > AVA_IDLE_PROMPT_AFTER_MS) {

    Serial.print("Ava: ");
    Serial.print(OWNER_NAME);
    Serial.println(", are you still there?");

    renderEyes(EYES_THINKING);

    // تایمر Idle از نو شروع شود
    lastInteractionTime = now;
  }
}

#endif
