#pragma once
#include <Arduino.h>

void avaDisplaySettingsBegin();
void avaDisplaySettingsEnter();
void avaDisplaySettingsExit();
bool avaDisplaySettingsActive();

void avaDisplaySettingsSetBrightness(uint8_t value);
void avaDisplaySettingsSetContrast(uint8_t value);
void avaDisplaySettingsSetMode(const String& mode);

void avaDisplaySettingsApply();
void avaDisplaySettingsLoad();

uint8_t avaDisplaySettingsBrightness();
uint8_t avaDisplaySettingsContrast();
String avaDisplaySettingsMode();
