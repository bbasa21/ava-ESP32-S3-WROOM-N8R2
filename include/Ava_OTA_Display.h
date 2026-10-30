#pragma once
#include <Arduino.h>
enum AvaOTAUIStatus : uint8_t { AVA_OTA_UI_IDLE=0, AVA_OTA_UI_UPDATING, AVA_OTA_UI_VERIFYING, AVA_OTA_UI_INSTALLING, AVA_OTA_UI_RESTARTING, AVA_OTA_UI_ERROR };
void avaOTAUIBegin();
void avaOTAUISetProgress(int percent);
void avaOTAUISetStatus(AvaOTAUIStatus status);
void avaOTAUIEnd();
bool avaOTAUIIsActive();
void avaOTAUIUpdate();
