#ifndef AVA_VOICE_H
#define AVA_VOICE_H

#include <Arduino.h>

// ================================
// AVA Voice System
// ================================

// Voice settings
#define VOICE_LANGUAGE "English"
#define VOICE_STYLE "Soft Robot Girl"
#define VOICE_SPEED 1.0


// Ava voice messages
const char* VOICE_BOOT =
  "Hello Ali. I am Ava.";

const char* VOICE_GREETING =
  "Hi Ali. Nice to see you.";

const char* VOICE_HAPPY =
  "I am happy.";

const char* VOICE_THINKING =
  "Let me think about it.";

const char* VOICE_SLEEP =
  "Okay. I will rest now.";

const char* VOICE_WAKE =
  "I am awake now.";

const char* VOICE_UNKNOWN =
  "I do not understand yet, but I am learning.";


// Voice output
// فعلاً خروجی فقط روی Serial است
// بعداً به اسپیکر واقعی وصل می‌شود
inline void playVoice(const char* text) {
  Serial.print("Ava Voice: ");
  Serial.println(text);
}

#endif