#include "Ava_WiFi.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#include "Ava_Config.h"
#include "Ava_OLED.h"


// ==================================================
// AVA WIFI SYSTEM
// ==================================================
//
// مسئولیت این فایل:
//
//   - WiFi connection
//   - WiFi reconnect
//   - Internet test
//   - Tehran weather
//   - Weather state
//   - Weather retry
//   - HTTPS/TLS recovery
//   - OLED weather trigger
//   - Debug information
//
// بدون ArduinoJson
//
// ==================================================


// ==================================================
// WIFI CONFIGURATION
// ==================================================
//
// مقدارهای فعلی SSID و PASSWORD خودت را اینجا نگه دار.
// به دلیل امنیتی در این پاسخ دوباره نمایش داده نشده‌اند.
//
// ==================================================

static const char* WIFI_SSID =
    "mano pesaram shoma hame";

static const char* WIFI_PASSWORD =
    "0912403597609126336751";


// ==================================================
// WIFI RECONNECT
// ==================================================

static unsigned long lastReconnectAttempt =
    0;

static constexpr unsigned long RECONNECT_INTERVAL =
    10000UL;


// ==================================================
// INTERNET TEST
// ==================================================

static bool internetTestDone =
    false;

static bool internetAvailable =
    false;

static constexpr unsigned long INTERNET_TEST_DELAY =
    1500UL;

static unsigned long wifiConnectedSince =
    0;


// ==================================================
// WEATHER CONFIGURATION
// ==================================================
//
// Tehran, Iran
//
// Latitude:
//   35.6892
//
// Longitude:
//   51.3890
//
// ==================================================

static constexpr float TEHRAN_LATITUDE =
    35.6892f;

static constexpr float TEHRAN_LONGITUDE =
    51.3890f;


// --------------------------------------------------
// Weather update interval
//
// هر 10 دقیقه
// --------------------------------------------------

static constexpr unsigned long WEATHER_UPDATE_INTERVAL =
    600000UL;


// --------------------------------------------------
// Weather retry interval
//
// در صورت شکست کامل درخواست، آوا تا 30 ثانیه
// دوباره درخواست خودکار نمی‌فرستد.
//
// توجه:
// retry داخلی HTTPS پایین‌تر از این interval
// جداست و بلافاصله یک بار دیگر تلاش می‌کند.
//
// --------------------------------------------------

static constexpr unsigned long WEATHER_RETRY_INTERVAL =
    30000UL;

static unsigned long lastWeatherAttempt =
    0;


// ==================================================
// HTTPS RECOVERY CONFIGURATION
// ==================================================
//
// اگر TLS/HTTPS در اولین تلاش شکست بخورد:
//
//   1. HTTP session بسته می‌شود
//   2. TLS client بسته می‌شود
//   3. 300ms صبر
//   4. یک HTTPS session کاملاً جدید ساخته می‌شود
//   5. دوباره GET انجام می‌شود
//
// ==================================================

static constexpr unsigned long WEATHER_HTTPS_RETRY_DELAY =
    300UL;


// ==================================================
// WEATHER STATE
// ==================================================

static bool weatherAvailable =
    false;

static float weatherTemperature =
    0.0f;

static int weatherHumidity =
    0;

static int weatherCode =
    -1;

static float weatherWindSpeed =
    0.0f;

static unsigned long lastWeatherUpdate =
    0;


// ==================================================
// PRINT WIFI INFORMATION
// ==================================================

static void avaPrintWiFiInfo()
{
    Serial.print(
        "[WiFi] Hostname: "
    );

    Serial.println(
        WiFi.getHostname()
    );


    Serial.print(
        "[WiFi] MAC: "
    );

    Serial.println(
        WiFi.macAddress()
    );


    Serial.print(
        "[WiFi] IP: "
    );

    if (
        WiFi.status() ==
        WL_CONNECTED
    )
    {
        Serial.println(
            WiFi.localIP()
        );
    }
    else
    {
        Serial.println(
            "NOT_CONNECTED"
        );
    }
}


// ==================================================
// BEGIN
// ==================================================

void avaWiFiBegin()
{
    Serial.println();

    Serial.println(
        "=========== AVA WIFI ==========="
    );


    // --------------------------------------------------
    // STA MODE
    // --------------------------------------------------

    WiFi.mode(
        WIFI_STA
    );


    // --------------------------------------------------
    // HOSTNAME
    // --------------------------------------------------

    WiFi.setHostname(
        "AVA"
    );


    WiFi.setAutoReconnect(
        true
    );


    WiFi.persistent(
        false
    );


    // --------------------------------------------------
    // DEVICE INFORMATION
    // --------------------------------------------------

    Serial.print(
        "[WiFi] Device ID: "
    );

    Serial.println(
        AVA_DEVICE_ID
    );


    Serial.print(
        "[WiFi] SSID: "
    );

    Serial.println(
        WIFI_SSID
    );


    Serial.println(
        "[WiFi] Starting connection..."
    );


    // --------------------------------------------------
    // START CONNECTION
    // --------------------------------------------------

    WiFi.begin(
        WIFI_SSID,
        WIFI_PASSWORD
    );


    // --------------------------------------------------
    // RESET STATES
    // --------------------------------------------------

    wifiConnectedSince =
        0;

    internetTestDone =
        false;

    internetAvailable =
        false;

    weatherAvailable =
        false;

    weatherTemperature =
        0.0f;

    weatherHumidity =
        0;

    weatherCode =
        -1;

    weatherWindSpeed =
        0.0f;

    lastWeatherAttempt =
        0;

    lastWeatherUpdate =
        0;

    lastReconnectAttempt =
        0;


    // --------------------------------------------------
    // Initial information
    // --------------------------------------------------

    avaPrintWiFiInfo();


    Serial.println(
        "================================"
    );
}


// ==================================================
// INTERNET TEST
// ==================================================

bool avaWiFiInternetTest()
{
    if (
        WiFi.status() !=
        WL_CONNECTED
    )
    {
        Serial.println(
            "[NET] Internet test skipped: WiFi not connected."
        );

        return false;
    }


    Serial.println();

    Serial.println(
        "=========== AVA INTERNET TEST ==========="
    );


    Serial.println(
        "[NET] WiFi connection is active."
    );


    Serial.print(
        "[NET] IP: "
    );

    Serial.println(
        WiFi.localIP()
    );


    Serial.println(
        "[NET] Sending HTTPS request..."
    );


    // ==================================================
    // HTTPS CLIENT
    // ==================================================

    WiFiClientSecure client;

    // فعلاً certificate validation خاموش است.
    // بعداً می‌توانیم CA واقعی اضافه کنیم.

    client.setInsecure();


    HTTPClient http;


    const char* testURL =
        "https://httpbin.org/status/200";


    Serial.print(
        "[NET] URL: "
    );

    Serial.println(
        testURL
    );


    // ==================================================
    // HTTP BEGIN
    // ==================================================

    if (
        !http.begin(
            client,
            testURL
        )
    )
    {
        Serial.println(
            "[NET] ERROR: HTTP begin failed."
        );


        client.stop();


        Serial.println(
            "=========================================="
        );


        return false;
    }


    // ==================================================
    // TIMEOUT
    // ==================================================

    http.setConnectTimeout(
        5000
    );

    http.setTimeout(
        5000
    );


    // ==================================================
    // GET
    // ==================================================

    int httpCode =
        http.GET();


    Serial.print(
        "[NET] HTTP response code: "
    );

    Serial.println(
        httpCode
    );


    // ==================================================
    // RESULT
    // ==================================================

    bool success =
        (
            httpCode ==
            HTTP_CODE_OK
        );


    if (
        success
    )
    {
        Serial.println(
            "[NET] INTERNET OK"
        );

        Serial.println(
            "[NET] AVA successfully reached the Internet."
        );
    }
    else
    {
        Serial.println(
            "[NET] INTERNET FAILED"
        );


        if (
            httpCode > 0
        )
        {
            Serial.print(
                "[NET] Server responded with HTTP code: "
            );

            Serial.println(
                httpCode
            );
        }
        else
        {
            Serial.print(
                "[NET] HTTP client error: "
            );

            Serial.println(
                http.errorToString(
                    httpCode
                )
            );
        }
    }


    // ==================================================
    // CLEANUP
    // ==================================================

    http.end();

    client.stop();


    Serial.println(
        "=========================================="
    );

    Serial.println();


    return success;
}


// ==================================================
// FIND CURRENT OBJECT
// ==================================================

static bool avaFindCurrentObject(
    const String& json,
    int& objectStart,
    int& objectEnd
)
{
    const String currentKey =
        "\"current\"";


    int currentPosition =
        json.indexOf(
            currentKey
        );


    if (
        currentPosition < 0
    )
    {
        return false;
    }


    // --------------------------------------------------
    // Find opening {
    // --------------------------------------------------

    objectStart =
        json.indexOf(
            '{',
            currentPosition +
            currentKey.length()
        );


    if (
        objectStart < 0
    )
    {
        return false;
    }


    // --------------------------------------------------
    // Find matching closing }
    // --------------------------------------------------

    int depth =
        0;

    bool insideString =
        false;

    bool escaped =
        false;


    for (
        int i = objectStart;
        i < json.length();
        i++
    )
    {
        char c =
            json[i];


        // ------------------------------------------------
        // JSON string handling
        // ------------------------------------------------

        if (
            escaped
        )
        {
            escaped =
                false;

            continue;
        }


        if (
            c == '\\' &&
            insideString
        )
        {
            escaped =
                true;

            continue;
        }


        if (
            c == '"'
        )
        {
            insideString =
                !insideString;

            continue;
        }


        if (
            insideString
        )
        {
            continue;
        }


        // ------------------------------------------------
        // Object depth
        // ------------------------------------------------

        if (
            c == '{'
        )
        {
            depth++;
        }
        else if (
            c == '}'
        )
        {
            depth--;


            if (
                depth == 0
            )
            {
                objectEnd =
                    i;

                return true;
            }
        }
    }


    return false;
}


// ==================================================
// FIND KEY VALUE POSITION
// ==================================================

static bool avaFindCurrentJsonValue(
    const String& json,
    const char* key,
    int& valueStart,
    int& valueEnd
)
{
    int objectStart =
        -1;

    int objectEnd =
        -1;


    if (
        !avaFindCurrentObject(
            json,
            objectStart,
            objectEnd
        )
    )
    {
        return false;
    }


    String currentObject =
        json.substring(
            objectStart,
            objectEnd + 1
        );


    String searchKey =
        String("\"") +
        key +
        "\"";


    int keyPosition =
        currentObject.indexOf(
            searchKey
        );


    if (
        keyPosition < 0
    )
    {
        return false;
    }


    // --------------------------------------------------
    // Find :
    // --------------------------------------------------

    int colonPosition =
        currentObject.indexOf(
            ':',
            keyPosition +
            searchKey.length()
        );


    if (
        colonPosition < 0
    )
    {
        return false;
    }


    // --------------------------------------------------
    // Start value
    // --------------------------------------------------

    int position =
        colonPosition + 1;


    // --------------------------------------------------
    // Skip whitespace
    // --------------------------------------------------

    while (
        position <
        currentObject.length()
    )
    {
        char c =
            currentObject[position];


        if (
            c == ' ' ||
            c == '\t' ||
            c == '\r' ||
            c == '\n'
        )
        {
            position++;
        }
        else
        {
            break;
        }
    }


    if (
        position >=
        currentObject.length()
    )
    {
        return false;
    }


    // --------------------------------------------------
    // JSON null
    // --------------------------------------------------

    if (
        currentObject.startsWith(
            "null",
            position
        )
    )
    {
        return false;
    }


    // --------------------------------------------------
    // Find numeric end
    // --------------------------------------------------

    int end =
        position;


    while (
        end <
        currentObject.length()
    )
    {
        char c =
            currentObject[end];


        if (
            (c >= '0' && c <= '9') ||
            c == '-' ||
            c == '+' ||
            c == '.' ||
            c == 'e' ||
            c == 'E'
        )
        {
            end++;
        }
        else
        {
            break;
        }
    }


    if (
        end ==
        position
    )
    {
        return false;
    }


    valueStart =
        objectStart +
        position;

    valueEnd =
        objectStart +
        end;


    return true;
}


// ==================================================
// JSON FLOAT
// ==================================================

static bool avaExtractCurrentJsonFloat(
    const String& json,
    const char* key,
    float& result
)
{
    int valueStart =
        -1;

    int valueEnd =
        -1;


    if (
        !avaFindCurrentJsonValue(
            json,
            key,
            valueStart,
            valueEnd
        )
    )
    {
        return false;
    }


    String value =
        json.substring(
            valueStart,
            valueEnd
        );


    result =
        value.toFloat();


    return true;
}


// ==================================================
// JSON INT
// ==================================================

static bool avaExtractCurrentJsonInt(
    const String& json,
    const char* key,
    int& result
)
{
    float temporary =
        0.0f;


    if (
        !avaExtractCurrentJsonFloat(
            json,
            key,
            temporary
        )
    )
    {
        return false;
    }


    result =
        static_cast<int>(
            temporary
        );


    return true;
}


// ==================================================
// WEATHER CODE NAME
// ==================================================

String avaWeatherCodeName(
    int code
)
{
    switch (
        code
    )
    {
        case 0:
            return "Clear sky";

        case 1:
            return "Mainly clear";

        case 2:
            return "Partly cloudy";

        case 3:
            return "Overcast";

        case 45:
        case 48:
            return "Fog";

        case 51:
        case 53:
        case 55:
            return "Drizzle";

        case 56:
        case 57:
            return "Freezing drizzle";

        case 61:
        case 63:
        case 65:
            return "Rain";

        case 66:
        case 67:
            return "Freezing rain";

        case 71:
        case 73:
        case 75:
            return "Snow";

        case 77:
            return "Snow grains";

        case 80:
        case 81:
        case 82:
            return "Rain showers";

        case 85:
        case 86:
            return "Snow showers";

        case 95:
            return "Thunderstorm";

        case 96:
        case 99:
            return "Thunderstorm with hail";

        default:
            return "Unknown";
    }
}


// ==================================================
// GET TEHRAN WEATHER
// ==================================================

bool avaWiFiGetTehranWeather()
{
    if (
        WiFi.status() !=
        WL_CONNECTED
    )
    {
        Serial.println(
            "[WEATHER] ERROR: WiFi not connected."
        );

        return false;
    }


    // --------------------------------------------------
    // Record attempt time
    // --------------------------------------------------

    lastWeatherAttempt =
        millis();


    Serial.println();

    Serial.println(
        "=========== AVA TEHRAN WEATHER ==========="
    );


    Serial.println(
        "[WEATHER] Location: Tehran, Iran"
    );


    Serial.print(
        "[WEATHER] Latitude: "
    );

    Serial.println(
        TEHRAN_LATITUDE,
        4
    );


    Serial.print(
        "[WEATHER] Longitude: "
    );

    Serial.println(
        TEHRAN_LONGITUDE,
        4
    );


    // ==================================================
    // OPEN-METEO URL
    // ==================================================

    String url =
        "https://api.open-meteo.com/v1/forecast"
        "?latitude=35.6892"
        "&longitude=51.3890"
        "&current=temperature_2m"
        ",relative_humidity_2m"
        ",weather_code"
        ",wind_speed_10m"
        "&timezone=Asia%2FTehran";


    Serial.println(
        "[WEATHER] Sending HTTPS request..."
    );


    Serial.print(
        "[WEATHER] URL: "
    );

    Serial.println(
        url
    );


    // ==================================================
    // HTTP/TLS REQUEST
    // ==================================================
    //
    // نکته مهم:
    //
    // برای هر تلاش یک WiFiClientSecure جدید ساخته می‌شود.
    //
    // این باعث می‌شود اگر TLS session قبلی خراب شده باشد،
    // retry از یک connection کاملاً تازه شروع شود.
    //
    // ==================================================

    String payload =
        "";

    int httpCode =
        -1;

    bool requestSucceeded =
        false;


    for (
        int attempt = 1;
        attempt <= 2;
        attempt++
    )
    {
        if (
            attempt == 2
        )
        {
            Serial.println();
            Serial.println(
                "[WEATHER] First HTTPS attempt failed."
            );

            Serial.println(
                "[WEATHER] Closing failed TLS session..."
            );


            delay(
                WEATHER_HTTPS_RETRY_DELAY
            );


            Serial.println(
                "[WEATHER] Starting fresh HTTPS retry..."
            );
        }


        // ------------------------------------------------
        // Fresh TLS client
        // ------------------------------------------------

        WiFiClientSecure client;

        client.setInsecure();


        // ------------------------------------------------
        // Fresh HTTP session
        // ------------------------------------------------

        HTTPClient http;


        if (
            !http.begin(
                client,
                url
            )
        )
        {
            Serial.println(
                "[WEATHER] ERROR: HTTP begin failed."
            );


            client.stop();


            if (
                attempt == 1
            )
            {
                continue;
            }


            Serial.println(
                "[WEATHER] HTTPS retry HTTP begin failed."
            );

            Serial.println(
                "=========================================="
            );


            return false;
        }


        // ------------------------------------------------
        // TIMEOUT
        // ------------------------------------------------

        if (
            attempt == 1
        )
        {
            http.setConnectTimeout(
                8000
            );

            http.setTimeout(
                8000
            );
        }
        else
        {
            http.setConnectTimeout(
                10000
            );

            http.setTimeout(
                10000
            );
        }


        // ------------------------------------------------
        // GET
        // ------------------------------------------------

        httpCode =
            http.GET();


        if (
            attempt == 1
        )
        {
            Serial.print(
                "[WEATHER] HTTP response code: "
            );
        }
        else
        {
            Serial.print(
                "[WEATHER] Retry HTTP response code: "
            );
        }

        Serial.println(
            httpCode
        );


        // ------------------------------------------------
        // SUCCESS
        // ------------------------------------------------

        if (
            httpCode ==
            HTTP_CODE_OK
        )
        {
            payload =
                http.getString();


            http.end();

            client.stop();


            if (
                payload.length() == 0
            )
            {
                Serial.println(
                    "[WEATHER] ERROR: Empty response."
                );


                if (
                    attempt == 1
                )
                {
                    continue;
                }


                Serial.println(
                    "[WEATHER] Retry returned empty response."
                );

                Serial.println(
                    "=========================================="
                );


                return false;
            }


            requestSucceeded =
                true;


            if (
                attempt == 2
            )
            {
                Serial.println(
                    "[WEATHER] HTTPS retry succeeded."
                );
            }


            break;
        }


        // ------------------------------------------------
        // FAILED REQUEST
        // ------------------------------------------------

        if (
            attempt == 1
        )
        {
            Serial.println(
                "[WEATHER] First HTTPS request failed."
            );
        }
        else
        {
            Serial.println(
                "[WEATHER] HTTPS retry failed."
            );
        }


        if (
            httpCode > 0
        )
        {
            if (
                attempt == 1
            )
            {
                Serial.print(
                    "[WEATHER] Server response: "
                );
            }
            else
            {
                Serial.print(
                    "[WEATHER] Retry server response: "
                );
            }


            Serial.println(
                httpCode
            );
        }
        else
        {
            if (
                attempt == 1
            )
            {
                Serial.print(
                    "[WEATHER] HTTP error: "
                );
            }
            else
            {
                Serial.print(
                    "[WEATHER] Retry HTTPS error: "
                );
            }


            Serial.println(
                http.errorToString(
                    httpCode
                )
            );
        }


        // ------------------------------------------------
        // Cleanup failed session
        // ------------------------------------------------

        http.end();

        client.stop();


        // ------------------------------------------------
        // If this was second attempt, give up
        // ------------------------------------------------

        if (
            attempt == 2
        )
        {
            Serial.println(
                "[WEATHER] ERROR: Weather request failed."
            );


            Serial.println(
                "=========================================="
            );


            return false;
        }
    }


    // ==================================================
    // REQUEST RESULT
    // ==================================================

    if (
        !requestSucceeded
    )
    {
        Serial.println(
            "[WEATHER] ERROR: HTTPS request did not succeed."
        );


        Serial.println(
            "=========================================="
        );


        return false;
    }


    // ==================================================
    // READ RESPONSE
    // ==================================================

    Serial.print(
        "[WEATHER] Response size: "
    );

    Serial.print(
        payload.length()
    );

    Serial.println(
        " bytes"
    );


    // ==================================================
    // PARSE
    // ==================================================

    float temperature =
        0.0f;

    int humidity =
        0;

    int code =
        -1;

    float windSpeed =
        0.0f;


    bool temperatureOK =
        avaExtractCurrentJsonFloat(
            payload,
            "temperature_2m",
            temperature
        );


    bool humidityOK =
        avaExtractCurrentJsonInt(
            payload,
            "relative_humidity_2m",
            humidity
        );


    bool codeOK =
        avaExtractCurrentJsonInt(
            payload,
            "weather_code",
            code
        );


    bool windOK =
        avaExtractCurrentJsonFloat(
            payload,
            "wind_speed_10m",
            windSpeed
        );


    // ==================================================
    // PARSER DIAGNOSTIC
    // ==================================================

    Serial.println();

    Serial.println(
        "[WEATHER] Parser diagnostic:"
    );


    int currentStart =
        -1;

    int currentEnd =
        -1;


    bool currentFound =
        avaFindCurrentObject(
            payload,
            currentStart,
            currentEnd
        );


    Serial.print(
        "  current object: "
    );

    Serial.println(
        currentFound
            ? "FOUND"
            : "FAILED"
    );


    Serial.print(
        "  temperature_2m: "
    );

    Serial.println(
        temperatureOK
            ? "OK"
            : "FAILED"
    );


    Serial.print(
        "  relative_humidity_2m: "
    );

    Serial.println(
        humidityOK
            ? "OK"
            : "FAILED"
    );


    Serial.print(
        "  weather_code: "
    );

    Serial.println(
        codeOK
            ? "OK"
            : "FAILED"
    );


    Serial.print(
        "  wind_speed_10m: "
    );

    Serial.println(
        windOK
            ? "OK"
            : "FAILED"
    );


    // ==================================================
    // VALIDATION
    // ==================================================

    if (
        !temperatureOK ||
        !humidityOK ||
        !codeOK ||
        !windOK
    )
    {
        Serial.println();

        Serial.println(
            "[WEATHER] ERROR: Could not parse weather data."
        );


        Serial.println(
            "[WEATHER] Raw response:"
        );


        Serial.println(
            payload
        );


        Serial.println(
            "=========================================="
        );


        // داده قبلی پاک نمی‌شود.

        return false;
    }


    // ==================================================
    // VALID RANGE CHECK
    // ==================================================

    bool valid =
        true;


    if (
        temperature < -100.0f ||
        temperature > 100.0f
    )
    {
        valid =
            false;
    }


    if (
        humidity < 0 ||
        humidity > 100
    )
    {
        valid =
            false;
    }


    if (
        code < 0 ||
        code > 99
    )
    {
        valid =
            false;
    }


    if (
        windSpeed < 0.0f ||
        windSpeed > 500.0f
    )
    {
        valid =
            false;
    }


    if (
        !valid
    )
    {
        Serial.println(
            "[WEATHER] ERROR: Parsed values are outside valid range."
        );


        Serial.println(
            "=========================================="
        );


        return false;
    }


    // ==================================================
    // STORE VALID DATA
    // ==================================================

    weatherTemperature =
        temperature;


    weatherHumidity =
        humidity;


    weatherCode =
        code;


    weatherWindSpeed =
        windSpeed;


    weatherAvailable =
        true;


    lastWeatherUpdate =
        millis();


    // ==================================================
    // RESULT
    // ==================================================

    Serial.println();

    Serial.println(
        "[WEATHER] ===== CURRENT TEHRAN WEATHER ====="
    );


    Serial.print(
        "[WEATHER] Temperature: "
    );

    Serial.print(
        weatherTemperature,
        1
    );

    Serial.println(
        " C"
    );


    Serial.print(
        "[WEATHER] Humidity: "
    );

    Serial.print(
        weatherHumidity
    );

    Serial.println(
        " %"
    );


    Serial.print(
        "[WEATHER] Weather code: "
    );

    Serial.println(
        weatherCode
    );


    Serial.print(
        "[WEATHER] Condition: "
    );

    Serial.println(
        avaWeatherCodeName(
            weatherCode
        )
    );


    Serial.print(
        "[WEATHER] Wind speed: "
    );

    Serial.print(
        weatherWindSpeed,
        1
    );

    Serial.println(
        " km/h"
    );


    Serial.println(
        "[WEATHER] Weather data stored successfully."
    );


    Serial.println(
        "[WEATHER] ====================================="
    );


    Serial.println();


    // ==================================================
    // WEATHER → OLED
    // ==================================================

    avaOLEDShowWeather();


    return true;
}


// ==================================================
// WIFI UPDATE
// ==================================================

void avaWiFiUpdate()
{
    // ==================================================
    // CONNECTED
    // ==================================================

    if (
        WiFi.status() ==
        WL_CONNECTED
    )
    {
        // ------------------------------------------------
        // FIRST SUCCESSFUL CONNECTION
        // ------------------------------------------------

        if (
            wifiConnectedSince == 0
        )
        {
            wifiConnectedSince =
                millis();


            Serial.println();

            Serial.println(
                "[WiFi] CONNECTED."
            );


            avaPrintWiFiInfo();


            // ------------------------------------------------
            // Reset internet state
            // ------------------------------------------------

            internetTestDone =
                false;


            internetAvailable =
                false;


            // ------------------------------------------------
            // Weather state
            // ------------------------------------------------

            weatherAvailable =
                false;


            weatherTemperature =
                0.0f;


            weatherHumidity =
                0;


            weatherCode =
                -1;


            weatherWindSpeed =
                0.0f;


            lastWeatherUpdate =
                0;


            lastWeatherAttempt =
                0;
        }


        // ==================================================
        // INTERNET TEST
        // ==================================================

        if (
            !internetTestDone &&
            millis() -
            wifiConnectedSince >=
                INTERNET_TEST_DELAY
        )
        {
            internetAvailable =
                avaWiFiInternetTest();


            internetTestDone =
                true;


            if (
                internetAvailable
            )
            {
                Serial.println(
                    "[WiFi] Internet is available."
                );
            }
            else
            {
                Serial.println(
                    "[WiFi] Internet test failed."
                );
            }
        }


        // ==================================================
        // WEATHER
        // ==================================================

        if (
            internetTestDone &&
            internetAvailable
        )
        {
            const unsigned long now =
                millis();


            // ------------------------------------------------
            // First request
            // ------------------------------------------------

            bool firstWeatherRequest =
                !weatherAvailable &&
                lastWeatherAttempt == 0;


            // ------------------------------------------------
            // Normal scheduled update
            // ------------------------------------------------

            bool scheduledUpdate =
                weatherAvailable &&
                (
                    now -
                    lastWeatherUpdate >=
                    WEATHER_UPDATE_INTERVAL
                );


            // ------------------------------------------------
            // Retry after failed request
            // ------------------------------------------------

            bool retryAfterFailure =
                !weatherAvailable &&
                lastWeatherAttempt != 0 &&
                (
                    now -
                    lastWeatherAttempt >=
                    WEATHER_RETRY_INTERVAL
                );


            if (
                firstWeatherRequest ||
                scheduledUpdate ||
                retryAfterFailure
            )
            {
                avaWiFiGetTehranWeather();
            }
        }


        return;
    }


    // ==================================================
    // NOT CONNECTED
    // ==================================================

    wifiConnectedSince =
        0;


    internetTestDone =
        false;


    internetAvailable =
        false;


    // --------------------------------------------------
    // آخرین Weather معتبر را پاک نمی‌کنیم.
    // --------------------------------------------------


    const unsigned long now =
        millis();


    // ==================================================
    // RECONNECT
    // ==================================================

    if (
        now -
        lastReconnectAttempt >=
        RECONNECT_INTERVAL
    )
    {
        lastReconnectAttempt =
            now;


        Serial.println(
            "[WiFi] Reconnecting..."
        );


        WiFi.reconnect();
    }
}


// ==================================================
// CONNECTION STATUS
// ==================================================

bool avaWiFiConnected()
{
    return (
        WiFi.status() ==
        WL_CONNECTED
    );
}


// ==================================================
// WIFI STATUS
// ==================================================

String avaWiFiStatus()
{
    switch (
        WiFi.status()
    )
    {
        case WL_CONNECTED:
            return "CONNECTED";

        case WL_NO_SSID_AVAIL:
            return "NO_SSID";

        case WL_CONNECT_FAILED:
            return "FAILED";

        case WL_CONNECTION_LOST:
            return "LOST";

        case WL_DISCONNECTED:
            return "DISCONNECTED";

        default:
            return "UNKNOWN";
    }
}


// ==================================================
// MAC
// ==================================================

String avaWiFiMac()
{
    return WiFi.macAddress();
}


// ==================================================
// IP
// ==================================================

String avaWiFiIp()
{
    if (
        WiFi.status() !=
        WL_CONNECTED
    )
    {
        return "NOT_CONNECTED";
    }


    return WiFi.localIP().toString();
}


// ==================================================
// WEATHER AVAILABLE
// ==================================================

bool avaWeatherAvailable()
{
    return weatherAvailable;
}


// ==================================================
// WEATHER TEMPERATURE
// ==================================================

float avaWeatherTemperature()
{
    return weatherTemperature;
}


// ==================================================
// WEATHER HUMIDITY
// ==================================================

int avaWeatherHumidity()
{
    return weatherHumidity;
}


// ==================================================
// WEATHER CODE
// ==================================================

int avaWeatherCode()
{
    return weatherCode;
}


// ==================================================
// WEATHER WIND
// ==================================================

float avaWeatherWindSpeed()
{
    return weatherWindSpeed;
}


// ==================================================
// WIFI DEBUG
// ==================================================

void avaWiFiDebugPrint()
{
    Serial.println();

    Serial.println(
        "=========== AVA WIFI DEBUG ==========="
    );


    // ==================================================
    // WIFI
    // ==================================================

    Serial.print(
        "WiFi status: "
    );

    Serial.println(
        avaWiFiStatus()
    );


    Serial.print(
        "Connected: "
    );

    Serial.println(
        avaWiFiConnected()
            ? "YES"
            : "NO"
    );


    Serial.print(
        "MAC: "
    );

    Serial.println(
        avaWiFiMac()
    );


    Serial.print(
        "IP: "
    );

    Serial.println(
        avaWiFiIp()
    );


    Serial.print(
        "Internet: "
    );

    Serial.println(
        internetAvailable
            ? "AVAILABLE"
            : "UNAVAILABLE"
    );


    // ==================================================
    // WEATHER
    // ==================================================

    Serial.println();

    Serial.println(
        "----------- WEATHER -----------"
    );


    Serial.print(
        "Available: "
    );

    Serial.println(
        avaWeatherAvailable()
            ? "YES"
            : "NO"
    );


    if (
        avaWeatherAvailable()
    )
    {
        Serial.print(
            "Temperature: "
        );

        Serial.print(
            avaWeatherTemperature(),
            1
        );

        Serial.println(
            " C"
        );


        Serial.print(
            "Humidity: "
        );

        Serial.print(
            avaWeatherHumidity()
        );

        Serial.println(
            " %"
        );


        Serial.print(
            "Weather code: "
        );

        Serial.println(
            avaWeatherCode()
        );


        Serial.print(
            "Condition: "
        );

        Serial.println(
            avaWeatherCodeName(
                avaWeatherCode()
            )
        );


        Serial.print(
            "Wind: "
        );

        Serial.print(
            avaWeatherWindSpeed(),
            1
        );

        Serial.println(
            " km/h"
        );


        Serial.print(
            "Last update: "
        );

        Serial.print(
            lastWeatherUpdate
        );

        Serial.println(
            " ms since boot"
        );
    }


    Serial.println(
        "======================================"
    );

    Serial.println();
}