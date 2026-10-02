#include "Ava_WiFi.h"

#include <WiFi.h>

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
        avaNetworkRequestedSSID,
        avaNetworkRequestedPassword
    );

    Serial.println(
        "[WiFi] New network connection started."
    );

    return true;
}
