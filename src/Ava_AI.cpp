#include "Ava_AI.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#include "Ava_WiFi.h"
#include "Ava_NetworkLock.h"

#if __has_include("Ava_Secrets.h")
#include "Ava_Secrets.h"
#endif

#ifndef AVA_OPENROUTER_API_KEY
#define AVA_OPENROUTER_API_KEY ""
#endif

#ifndef AVA_OPENROUTER_MODEL
#define AVA_OPENROUTER_MODEL "openrouter/free"
#endif

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
    const String marker = "\"content\":\"";
    int start = payload.indexOf(marker);

    if (start < 0)
    {
        return false;
    }

    start += marker.length();

    String decoded;
    decoded.reserve(512);

    bool escaped = false;

    for (int i = start; i < payload.length(); ++i)
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

    if (String(AVA_OPENROUTER_API_KEY).length() == 0)
    {
        Serial.println("[AI] ERROR: OpenRouter API key is not configured.");
        Serial.println("[AI] Create include/Ava_Secrets.h locally.");
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
    http.addHeader("Authorization", String("Bearer ") + AVA_OPENROUTER_API_KEY);

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

        http.end();
        aiReady = false;
        return false;
    }

    http.end();

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
