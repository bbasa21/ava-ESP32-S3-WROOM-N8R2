#ifndef AVA_OTA_H
#define AVA_OTA_H

#include <Arduino.h>

bool avaOTAUpdate();
bool avaOTACheckForUpdate();
bool avaOTAInstallUpdate();

void avaOTAStartTask();
bool avaOTAIsRunning();

#endif
