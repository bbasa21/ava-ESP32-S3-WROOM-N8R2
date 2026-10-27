#include "Ava_WiFi.h"

#include <WiFi.h>
#include <Preferences.h>

// ==================================================
// AVA NETWORK CONTROL
// ==================================================
// کنترل شبکه از طریق AVA PET.
// تنظیمات WiFi فعلی پروژه دست‌نخورده می‌ماند.
// ==================================================

static const char* AVA_WIFI_PREFS_NAMESPACE = "ava_wifi";
static const char* AVA_WIFI_PREFS_SSID = "ssid";
static const char* AVA_WIFI_PREFS_PASSWORD = "password";

static String avaNetworkRequestedSSID = "";

String avaWiFiSSID()
{
    if (WiFi.status() == WL_CONNECTED)
        return WiFi.SSID();
    return avaNetworkRequestedSSID;
}

bool avaWiFiLoadSavedCredentials(String& ssid, String& password)
{
    Preferences prefs;
    if (!prefs.begin(AVA_WIFI_PREFS_NAMESPACE, true))
        return false;

    ssid = prefs.getString(AVA_WIFI_PREFS_SSID, "");
    password = prefs.getString(AVA_WIFI_PREFS_PASSWORD, "");
    prefs.end();

    ssid.trim();
    if (ssid.length() == 0)
        return false;

    avaNetworkRequestedSSID = ssid;
    return true;
}

bool avaWiFiConnect(const String& ssid, const String& password)
{
    String newSSID = ssid;
    newSSID.trim();
    if (newSSID.length() == 0)
    {
        Serial.println("[WiFi] Network connect rejected: empty SSID.");
        return false;
    }

    // Persist before disconnecting so the selected network survives reboot.
    Preferences prefs;
    if (!prefs.begin(AVA_WIFI_PREFS_NAMESPACE, false))
    {
        Serial.println("[WiFi] ERROR: Could not open NVS for Wi-Fi settings.");
        return false;
    }

    const size_t ssidWritten = prefs.putString(AVA_WIFI_PREFS_SSID, newSSID);
    const size_t passwordWritten = prefs.putString(AVA_WIFI_PREFS_PASSWORD, password);
    prefs.end();

    if (ssidWritten == 0 || passwordWritten == 0)
    {
        Serial.println("[WiFi] ERROR: Could not persist Wi-Fi settings.");
        return false;
    }

    avaNetworkRequestedSSID = newSSID;
    Serial.print("[WiFi] Network requested from AVA PET: ");
    Serial.println(avaNetworkRequestedSSID);

    WiFi.disconnect(false, false);
    delay(100);
    WiFi.begin(newSSID.c_str(), password.c_str());
    Serial.println("[WiFi] New network connection started.");
    return true;
}
