#ifndef AVA_OTA_V2_H
#define AVA_OTA_V2_H

#include <Arduino.h>

// Experimental OTA engine. Not wired into AVA's startup flow yet.
// Call manually only after reviewing/testing the feature branch.
bool avaOTA2CheckAndInstall(const char* manifestUrl);

#endif
