#include "Ava_AI.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Preferences.h>

#include "Ava_WiFi.h"
#include "Ava_NetworkLock.h"

#ifndef AVA_OPENROUTER_MODEL
#define AVA_OPENROUTER_MODEL "openrouter/free"
#endif

static const char* AVA_PREFS_NAMESPACE = "ava_ai";
static const char* AVA_PREFS_API_KEY = "or_key";

static String avaAIGetApiKey()
{
    Preferences prefs;

    if (!prefs.begin(AVA_PREFS_NAMESPACE, true))
    {
        return "";
    }

    String key = prefs.getString(AVA_PREFS_API_KEY, "");
    prefs.end();

    return key;
}

bool avaAISetApiKey(const String& apiKey)
{
    String key = apiKey;
    key.trim();

    if (key.length() == 0)
    {
        return false;
    }

    Preferences prefs;

    if (!prefs.begin(AVA_PREFS_NAMESPACE, false))
    {
        return false;
    }

    const size_t written = prefs.putString(AVA_PREFS_API_KEY, key);
    prefs.end();

    return written > 0;
}

bool avaAIHasApiKey()
{
    return avaAIGetApiKey().length() > 0;
}

static bool aiReady = false;

static String avaAIJsonEscape(const String& input)
{
    String output;
    output.reserve(input.length() + 16);

    for (size_t i = 0; i < input.length(); ++i)
    {
        const char c = input[i];

        switch (c)
        {
            case '\\': output += "\\\\"; break;
            case '"':  output += "\\\""; break;
            case '\n': output += "\\n"; break;
            case '\r': output += "\\r"; break;
            case '\t': output += "\\t"; break;
            default:    output += c; break;
        }
    }

    return output;
}

static bool avaAIExtractText(const String& payload, String& answer)
{
    answer = "";

    int contentKey = payload.indexOf("\"content\"");

    while (contentKey >= 0)
    {
        int colon = payload.indexOf(':', contentKey + 9);

        if (colon < 0)
        {
            return false;
        }

        int valueStart = colon + 1;

        while (valueStart < payload.length() &&
               (payload[valueStart] == ' ' ||
                payload[valueStart] == '\\t' ||
                payload[valueStart] == '\\r' ||
                payload[valueStart] == '\\n'))
        {
            ++valueStart;
        }

        if (valueStart < payload.length() && payload[valueStart] == '"')
        {
            String decoded;
            decoded.reserve(512);

            bool escaped = false;

            for (int i = valueStart + 1; i < payload.length(); ++i)
            {
                const char c = payload[i];

                if (escaped)
                {
                    switch (c)
                    {
                        case '"':  decoded += '"';  break;
                        case '\\': decoded += '\\'; break;
                        case 'n':  decoded += '\n'; break;
                        case 'r':  decoded += '\r'; break;
                        case 't':  decoded += '\t'; break;
                        case '/': decoded += '/'; break;
                        case 'b': decoded += '\\b'; break;
                        case 'f': decoded += '\\f'; break;
                        default:   decoded += c; break;
                    }

                    escaped = false;
                    continue;
                }

                if (c == '\\')
                {
                    escaped = true;
                    continue;
                }

                if (c == '"')
                {
                    answer = decoded;
                    answer.trim();
                    return answer.length() > 0;
                }

                decoded += c;
            }

            return false;
        }

        contentKey = payload.indexOf("\"content\"", contentKey + 9);
    }

    return false;
}

static String avaAIExtractError(const String& payload)
{
    const String marker = "\"message\":\"";
    int start = payload.indexOf(marker);

    if (start < 0)
    {
        return "";
    }

    start += marker.length();

    String message;

    for (int i = start; i < payload.length(); ++i)
    {
        const char c = payload[i];

        if (c == '"' && (i == start || payload[i - 1] != '\\'))
        {
            break;
        }

        if (c == '\\' && i + 1 < payload.length())
        {
            const char next = payload[i + 1];

            if (next == '"')
            {
                message += '"';
                ++i;
                continue;
            }

            if (next == 'n')
            {
                message += '\n';
                ++i;
                continue;
            }
        }

        message += c;
    }

    return message;
}

bool avaAIPrepare()
{
    aiReady = false;

    Serial.println();
    Serial.println("=========== AVA AI ===========");
    Serial.println("[AI] OpenRouter request received.");

    const String apiKey = avaAIGetApiKey();

    if (apiKey.length() == 0)
    {
        Serial.println("[AI] ERROR: OpenRouter API key is not configured.");
        Serial.println("[AI] Set the key once with: apikey|YOUR_OPENROUTER_KEY");
        Serial.println("==============================");
        return false;
    }

    if (!avaWiFiConnected())
    {
        Serial.println("[AI] WiFi is not connected.");
        Serial.println("==============================");
        return false;
    }

    {
        AvaNetworkLockGuard networkLock;

        if (!networkLock.isLocked())
        {
            Serial.println("[AI] ERROR: Could not acquire network lock.");
            Serial.println("==============================");
            return false;
        }

        if (!avaWiFiInternetTest())
        {
            Serial.println("[AI] Internet is unavailable.");
            Serial.println("==============================");
            return false;
        }
    }

    aiReady = true;

    Serial.print("[AI] Model: ");
    Serial.println(AVA_OPENROUTER_MODEL);
    Serial.println("[AI] OpenRouter is ready.");
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

    const String apiKey = avaAIGetApiKey();

    if (apiKey.length() == 0)
    {
        Serial.println("[AI] ERROR: OpenRouter API key is not configured.");
        aiReady = false;
        return false;
    }

    AvaNetworkLockGuard networkLock;

    if (!networkLock.isLocked())
    {
        Serial.println("[AI] ERROR: Could not acquire network lock for OpenRouter.");
        aiReady = false;
        return false;
    }

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;

    const String url = "https://openrouter.ai/api/v1/chat/completions";

    if (!http.begin(client, url))
    {
        Serial.println("[AI] ERROR: HTTP begin failed.");
        aiReady = false;
        return false;
    }

    http.setTimeout(30000);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", String("Bearer ") + apiKey);
    http.addHeader("HTTP-Referer", "https://github.com/bbasa21/ava-ESP32-S3-WROOM-N8R2");
    http.addHeader("X-Title", "AVA Robot");
    http.addHeader("User-Agent", "AVA-ESP32-S3");

    const String systemInstruction =
        "You are AVA, a friendly personal robot created by Ali. "
        "Answer naturally and concisely. "
        "Do not claim to control hardware unless the firmware explicitly gives you that ability. "
        "The current AVA hardware can display text on an OLED and print text on Serial.";

    String body;
    body.reserve(question.length() + systemInstruction.length() + 256);

    body =
        "{"
        "\"model\":\"" + avaAIJsonEscape(AVA_OPENROUTER_MODEL) + "\","
        "\"messages\":["
            "{"
                "\"role\":\"system\","
                "\"content\":\"" + avaAIJsonEscape(systemInstruction) + "\""
            "},"
            "{"
                "\"role\":\"user\","
                "\"content\":\"" + avaAIJsonEscape(question) + "\""
            "}"
        "]"
        "}";

    Serial.println("[AI] Sending request to OpenRouter...");
    Serial.print("[AI] Question: ");
    Serial.println(question);

    const int httpCode = http.POST(body);

    if (httpCode <= 0)
    {
        Serial.print("[AI] HTTP error: ");
        Serial.println(http.errorToString(httpCode));
        http.end();
        aiReady = false;
        return false;
    }

    const String payload = http.getString();

    Serial.print("[AI] Content-Type: ");
    Serial.println(http.header("Content-Type"));

    Serial.print("[AI] HTTP status: ");
    Serial.println(httpCode);

    if (httpCode < 200 || httpCode >= 300)
    {
        Serial.println("[AI] OpenRouter request failed.");

        const String errorMessage = avaAIExtractError(payload);

        if (errorMessage.length() > 0)
        {
            Serial.print("[AI] OpenRouter error: ");
            Serial.println(errorMessage);
        }
        else
        {
            Serial.println("[AI] OpenRouter error: No message field found.");
        }

        Serial.print("[AI] OpenRouter response body length: ");
        Serial.println(payload.length());
        Serial.println("[AI] OpenRouter response body:");
        Serial.println(payload);

        http.end();
        aiReady = false;
        return false;
    }

    http.end();

    Serial.print("[AI] OpenRouter response body length: ");
    Serial.println(payload.length());

    if (!avaAIExtractText(payload, answer))
    {
        Serial.println("[AI] ERROR: Could not extract OpenRouter response text.");
        Serial.println("[AI] Raw response:");
        Serial.println(payload);
        aiReady = false;
        return false;
    }

    Serial.println("[AI] OpenRouter response received.");
    Serial.print("[AI] Answer: ");
    Serial.println(answer);

    aiReady = true;
    return true;
}
