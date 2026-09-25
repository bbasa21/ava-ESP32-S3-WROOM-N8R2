#ifndef AVA_OTA_DISPLAY_H
#define AVA_OTA_DISPLAY_H

#include <Arduino.h>

// ==================================================
// AVA OTA OLED UI
// ==================================================

enum AvaOTAUIStatus : uint8_t
{
    AVA_OTA_UI_IDLE = 0,
    AVA_OTA_UI_UPDATING,
    AVA_OTA_UI_VERIFYING,
    AVA_OTA_UI_INSTALLING,
    AVA_OTA_UI_RESTARTING,
    AVA_OTA_UI_ERROR
};

void avaOTAUIBegin();
void avaOTAUISetProgress(int percent);
void avaOTAUISetStatus(AvaOTAUIStatus status);
void avaOTAUIEnd();
void avaOTAUIUpdate();

#endif
