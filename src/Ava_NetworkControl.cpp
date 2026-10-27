#include "Ava_WiFi.h"

#include <WiFi.h>
#include <Preferences.h>

// ==================================================
// AVA NETWORK CONTROL
// ==================================================
// کنترل شبکه از طریق AVA PET.
// تنظیمات WiFi فعلی پروژه دست‌نخورده می‌ماند.
// ==================================================

static String avaNetworkRequestedSSID = "";

static String avaNetworkRequestedPassword = "";

String avaWiFiSSID()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        return WiFi.SSID();
    }

    return avaNetworkRequestedSSID;
}

bool avaWiFiConnect(
    const String& ssid,
    const String& password
)
{
    String newSSID = ssid;
    String newPassword = password;

    newSSID.trim();

    if (newSSID.length() == 0)
    {
        Serial.println(
            "[WiFi] Network connect rejected: empty SSID."
        );

        return false;
    }

    // Persist credentials before starting the connection so they survive reboot.
    Preferences wifiPreferences;
    if (!wifiPreferences.begin("ava_wifi", false))
    {
        Serial.println("[WiFi] ERROR: Could not open WiFi NVS namespace.");
        return false;
    }

    const size_t ssidBytes = wifiPreferences.putString("ssid", newSSID);
    const size_t passwordBytes = wifiPreferences.putString("password", newPassword);
    wifiPreferences.end();

    if (ssidBytes == 0 || (newPassword.length() > 0 && passwordBytes == 0))
    {
        Serial.println("[WiFi] ERROR: Could not persist WiFi credentials to NVS.");
        return false;
    }

    avaNetworkRequestedSSID = newSSID;
    avaNetworkRequestedPassword = newPassword;

    Serial.print(
        "[WiFi] Network requested from AVA PET: "
    );

    Serial.println(
        avaNetworkRequestedSSID
    );

    WiFi.disconnect(
        false,
        false
    );

    delay(100);

    WiFi.begin(
        avaNetworkRequestedSSID.c_str(),
        avaNetworkRequestedPassword.c_str()
    );

    Serial.println(
        "[WiFi] Credentials saved to NVS; connection started."
    );

    return true;
}
