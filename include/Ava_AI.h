#ifndef AVA_AI_H
#define AVA_AI_H

#include <Arduino.h>

bool avaAIPrepare();
bool avaAIReady();
bool avaAIAsk(const String& question, String& answer);
bool avaAISetApiKey(const String& apiKey);
bool avaAIHasApiKey();

#endif
