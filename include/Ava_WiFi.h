#ifndef AVA_WIFI_H
#define AVA_WIFI_H

#include <Arduino.h>

// ==================================================
// AVA WIFI SYSTEM
//
// مسئولیت:
//
//   WiFi
//      ↓
//   Internet
//      ↓
//   Weather API
//      ↓
//   Tehran Weather Data
//
// فعلاً خروجی فقط روی Serial Monitor است.
//
// ==================================================

// ==================================================
// WIFI
// ==================================================

void avaWiFiBegin();

void avaWiFiUpdate();

bool avaWiFiConnected();

String avaWiFiStatus();

String avaWiFiMac();

String avaWiFiIp();

// ==================================================
// INTERNET
// ==================================================

// تست واقعی اتصال به اینترنت
//
// خروجی:
//
//   true  = Internet OK
//   false = Internet FAILED
//
bool avaWiFiInternetTest();

// ==================================================
// WEATHER
// ==================================================
//
// دریافت وضعیت آب‌وهوای تهران.
//
// خروجی:
//
//   true  = دریافت موفق
//   false = دریافت ناموفق
//
// فعلاً اطلاعات روی Serial چاپ می‌شود.
//
// ==================================================

bool avaWiFiGetTehranWeather();

// ==================================================
// WEATHER STATE
// ==================================================

bool avaWeatherAvailable();

float avaWeatherTemperature();

int avaWeatherHumidity();

int avaWeatherCode();

float avaWeatherWindSpeed();

// ==================================================
// WEATHER TEXT
// ==================================================
//
// تبدیل WMO Weather Code به متن ساده.
//
// مثال:
//
//   0  → Clear sky
//   1  → Mainly clear
//   2  → Partly cloudy
//   3  → Overcast
//
// ==================================================

String avaWeatherCodeName(int code);

// ==================================================
// DEBUG
// ==================================================

void avaWiFiDebugPrint();

#endif
