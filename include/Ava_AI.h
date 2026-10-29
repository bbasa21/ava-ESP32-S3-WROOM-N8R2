#ifndef AVA_AI_H
#define AVA_AI_H

#include <Arduino.h>

bool avaAIPrepare();
bool avaAIReady();
bool avaAIAsk(const String& question, String& answer);
bool avaAISetApiKey(const String& apiKey);
bool avaAIHasApiKey();

// User-controlled persistent memory stored locally in ESP32 NVS.
String avaAIGetMemory();
bool avaAISetMemory(const String& note);
bool avaAIClearMemory();

// Conservative text hint used to select an existing OLED expression.
const char* avaAIEmotionHint(const String& answer);

#endif
