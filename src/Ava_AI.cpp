#include "Ava_AI.h"

#include <WiFi.h>

#include "Ava_WiFi.h"
#include "Ava_NetworkLock.h"

static bool aiReady = false;

bool avaAIPrepare()
{
    aiReady = false;

    Serial.println();
    Serial.println("=========== AVA AI ===========");
    Serial.println("[I HAVE QUESTION] AI request received.");
    Serial.println("[AI] Preparing AVA AI...");

    Serial.println("[AI] Checking WiFi connection...");

    if (!avaWiFiConnected())
    {
        Serial.println("[AI] WiFi is not connected.");
        Serial.println("[AI] AVA AI unavailable.");
        Serial.println("==============================");
        return false;
    }

    Serial.println("[AI] WiFi connected.");
    Serial.print("[AI] SSID: ");
    Serial.println(avaWiFiSSID());
    Serial.print("[AI] IP: ");
    Serial.println(avaWiFiIp());
    Serial.print("[AI] RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");

    Serial.println("[AI] Checking Internet access...");

    {
        AvaNetworkLockGuard networkLock;

        if (!networkLock.isLocked())
        {
            Serial.println("[AI] ERROR: Could not acquire network lock.");
            Serial.println("[AI] AVA AI unavailable.");
            Serial.println("==============================");
            return false;
        }

        if (!avaWiFiInternetTest())
        {
            Serial.println("[AI] Internet is unavailable.");
            Serial.println("[AI] AVA AI unavailable.");
            Serial.println("==============================");
            return false;
        }
    }

    aiReady = true;

    Serial.println("[AI] Internet connection OK.");
    Serial.println("[AI] AVA AI is ready.");
    Serial.println("==============================");

    return true;
}

bool avaAIReady()
{
    return aiReady;
}

bool avaAIAsk(const String& question, String& answer)
{
    answer = "";

    if (question.length() == 0)
    {
        Serial.println("[AI] ERROR: Empty question.");
        return false;
    }

    if (!avaAIPrepare())
    {
        return false;
    }

    Serial.println("[AI] Question accepted.");
    Serial.print("[AI] Question: ");
    Serial.println(question);

    Serial.println("[AI] AVA AI API is not connected yet.");
    Serial.println("[AI] Waiting for AVA AI API + Gemini integration.");

    aiReady = false;
    return false;
}
