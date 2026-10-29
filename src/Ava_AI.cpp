#include "Ava_AI.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Preferences.h>

#include "Ava_WiFi.h"
#include "Ava_NetworkLock.h"

#ifndef AVA_GAPGPT_MODEL
#define AVA_GAPGPT_MODEL "gpt-4o"
#endif

static const char* AVA_PREFS_NAMESPACE = "ava_ai";
static const char* AVA_PREFS_API_KEY = "or_key";

static String avaAIGetApiKey()
{
    Preferences prefs;
    if (!prefs.begin(AVA_PREFS_NAMESPACE, true))
        return "";

    String key = prefs.getString(AVA_PREFS_API_KEY, "");
    prefs.end();
    return key;
}

bool avaAISetApiKey(const String& apiKey)
{
    String key = apiKey;
    key.trim();
    if (key.length() == 0)
        return false;

    Preferences prefs;
    if (!prefs.begin(AVA_PREFS_NAMESPACE, false))
        return false;

    const size_t written = prefs.putString(AVA_PREFS_API_KEY, key);
    prefs.end();
    return written > 0;
}

bool avaAIHasApiKey()
{
    return avaAIGetApiKey().length() > 0;
}

static const char* AVA_PREFS_MEMORY = "user_memory";
static const size_t AVA_MEMORY_MAX_LENGTH = 180;

String avaAIGetMemory()
{
    Preferences prefs;
    if (!prefs.begin(AVA_PREFS_NAMESPACE, true))
        return "";
    String memory = prefs.getString(AVA_PREFS_MEMORY, "");
    prefs.end();
    return memory;
}

bool avaAISetMemory(const String& note)
{
    String value = note;
    value.trim();
    if (value.length() == 0 || value.length() > AVA_MEMORY_MAX_LENGTH)
        return false;
    Preferences prefs;
    if (!prefs.begin(AVA_PREFS_NAMESPACE, false))
        return false;
    const size_t written = prefs.putString(AVA_PREFS_MEMORY, value);
    prefs.end();
    return written > 0;
}

bool avaAIClearMemory()
{
    Preferences prefs;
    if (!prefs.begin(AVA_PREFS_NAMESPACE, false))
        return false;
    const bool removed = prefs.remove(AVA_PREFS_MEMORY);
    prefs.end();
    return removed;
}

// Conservative text-based tone matching for the existing OLED emotion API.
const char* avaAIEmotionHint(const String& answer)
{
    String text = answer;
    text.toLowerCase();

    if (text.indexOf("متأسفم") >= 0 || text.indexOf("متاسفم") >= 0 ||
        text.indexOf("ناراحتی") >= 0 || text.indexOf("غمگین") >= 0 ||
        text.indexOf("sorry") >= 0 || text.indexOf("sad") >= 0)
        return "SAD";

    if (text.indexOf("تبریک") >= 0 || text.indexOf("خوشحالم") >= 0 ||
        text.indexOf("عالیه") >= 0 || text.indexOf("افتخار") >= 0 ||
        text.indexOf("آفرین") >= 0 || text.indexOf("خوشحال") >= 0 ||
        text.indexOf("wonderful") >= 0 || text.indexOf("congrat") >= 0 ||
        text.indexOf("happy") >= 0 || text.indexOf("great") >= 0)
        return "HAPPY";

    if (text.indexOf("؟") >= 0 || text.indexOf("چطور") >= 0 ||
        text.indexOf("شاید") >= 0 || text.indexOf("نمی‌دانم") >= 0 ||
        text.indexOf("wonder") >= 0 || text.indexOf("perhaps") >= 0)
        return "THINKING";

    return "NEUTRAL";
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
            return false;

        int valueStart = colon + 1;

        while (valueStart < payload.length() &&
               (payload[valueStart] == ' ' ||
                payload[valueStart] == '\t' ||
                payload[valueStart] == '\r' ||
                payload[valueStart] == '\n'))
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
                        case '"':  decoded += '"'; break;
                        case '\\': decoded += '\\'; break;
                        case 'n': decoded += '\n'; break;
                        case 'r': decoded += '\r'; break;
                        case 't': decoded += '\t'; break;
                        case '/': decoded += '/'; break;
                        case 'b': decoded += '\b'; break;
                        case 'f': decoded += '\f'; break;
                        default: decoded += c; break;
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
        return "";

    start += marker.length();
    String message;

    for (int i = start; i < payload.length(); ++i)
    {
        const char c = payload[i];

        if (c == '"' && (i == start || payload[i - 1] != '\\'))
            break;

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
    Serial.println("[AI] GapGPT request received.");

    const String apiKey = avaAIGetApiKey();

    if (apiKey.length() == 0)
    {
        Serial.println("[AI] ERROR: GapGPT API key is not configured.");
        Serial.println("[AI] Set the key once with: apikey|YOUR_GAPGPT_KEY");
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
    Serial.println(AVA_GAPGPT_MODEL);
    Serial.println("[AI] GapGPT is ready.");
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
        return false;

    const String apiKey = avaAIGetApiKey();

    if (apiKey.length() == 0)
    {
        Serial.println("[AI] ERROR: GapGPT API key is not configured.");
        aiReady = false;
        return false;
    }

    AvaNetworkLockGuard networkLock;

    if (!networkLock.isLocked())
    {
        Serial.println("[AI] ERROR: Could not acquire network lock for GapGPT.");
        aiReady = false;
        return false;
    }

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    const String url = "https://api.gapgpt.app/v1/chat/completions";

    if (!http.begin(client, url))
    {
        Serial.println("[AI] ERROR: HTTP begin failed.");
        aiReady = false;
        return false;
    }

    http.setTimeout(30000);

    // GapGPT is returning Transfer-Encoding: chunked to the ESP32.
    // Force HTTP/1.0 so the response uses a normal body instead of chunked
    // transfer encoding, which avoids an empty/whitespace payload on ESP32.
    http.useHTTP10(true);

    const char* responseHeaders[] = {
        "Content-Type",
        "Content-Length",
        "Transfer-Encoding",
        "Content-Encoding"
    };
    http.collectHeaders(responseHeaders, 4);

    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", String("Bearer ") + apiKey);

    const String savedMemory = avaAIGetMemory();
    const String systemInstruction =
        "You are AVA, a warm, curious, gentle and witty robot companion created by Ali. "
        "Use a consistent personality: natural, kind, lightly playful when appropriate, never stiff or excessively dramatic. "
        "When the user is upset, validate the feeling first, then offer one practical and honest response. "
        "Match the user's language. For Persian, use natural conversational Persian. "
        "Keep replies OLED-friendly: usually one to three short sentences unless more detail is requested. "
        "Never flatter mechanically; be sincere, specific and grounded in the conversation. "
        "Do not claim human feelings, consciousness, sensor access, hardware control, or memories that are not available. "
        "A short user-approved note may be supplied as persistent memory. Use it only when relevant and never invent additional personal facts. "
        "Current hardware can display text on OLED and print text on Serial."
        + (savedMemory.length() > 0 ? String("\\nUser-approved memory note: ") + savedMemory : "");

    String body;
    body.reserve(question.length() + systemInstruction.length() + 256);

    body =
        "{"
        "\"model\":\"" + avaAIJsonEscape(AVA_GAPGPT_MODEL) + "\","
        "\"messages\":["
            "{"
                "\"role\":\"system\","
                "\"content\":\"" + avaAIJsonEscape(systemInstruction) + "\""
            "},"
            "{"
                "\"role\":\"user\","
                "\"content\":\"" + avaAIJsonEscape(question) + "\""
            "}"
        "],"
        "\"stream\":false"
        "}";

    Serial.println("[AI] Sending request to GapGPT...");
    Serial.print("[AI] Question: ");
    Serial.println(question);

    const int httpCode = http.POST(body);
    const String payload = http.getString();

    Serial.print("[AI] Content-Type: ");
    Serial.println(http.header("Content-Type"));

    Serial.print("[AI] Content-Length: ");
    Serial.println(http.header("Content-Length"));

    Serial.print("[AI] Transfer-Encoding: ");
    Serial.println(http.header("Transfer-Encoding"));

    Serial.print("[AI] Content-Encoding: ");
    Serial.println(http.header("Content-Encoding"));

    Serial.print("[AI] HTTP status: ");
    Serial.println(httpCode);

    if (httpCode <= 0)
    {
        Serial.print("[AI] HTTP error: ");
        Serial.println(http.errorToString(httpCode));
        http.end();
        aiReady = false;
        return false;
    }

    if (httpCode < 200 || httpCode >= 300)
    {
        Serial.println("[AI] GapGPT request failed.");

        const String errorMessage = avaAIExtractError(payload);

        if (errorMessage.length() > 0)
        {
            Serial.print("[AI] GapGPT error: ");
            Serial.println(errorMessage);
        }
        else
        {
            Serial.println("[AI] GapGPT error: No message field found.");
        }

        Serial.print("[AI] GapGPT response body length: ");
        Serial.println(payload.length());
        Serial.println("[AI] GapGPT response body:");
        Serial.println(payload);

        http.end();
        aiReady = false;
        return false;
    }

    Serial.print("[AI] GapGPT response body length: ");
    Serial.println(payload.length());

    if (!avaAIExtractText(payload, answer))
    {
        Serial.println("[AI] ERROR: Could not extract GapGPT response text.");
        Serial.println("[AI] Raw response:");
        Serial.println(payload);
        http.end();
        aiReady = false;
        return false;
    }

    http.end();

    Serial.println("[AI] GapGPT response received.");
    Serial.print("[AI] Answer: ");
    Serial.println(answer);

    aiReady = true;
    return true;
}
