#ifndef AVA_TIME_H
#define AVA_TIME_H

#include <Arduino.h>
#include "Ava_OLED.h"

// ==================================================
// AVA TIME SYSTEM
//
// مسئولیت:
//   - نگهداری ساعت فعلی AVA
//   - اعتبارسنجی ساعت
//   - دریافت HH:MM یا HH:MM:SS
//   - آماده‌سازی برای دریافت از AVA PET → BLE
//   - ارسال ساعت به OLED
//
// معماری:
//
//   AVA PET
//      ↓
//   BLE DATA
//      ↓
//   AvaTime
//      ↓
//   Ava OLED
//
// نمونه داده:
//
//   TIME_RESPONSE|22:41
//   TIME_RESPONSE|22:41:30
//
// ==================================================

namespace AvaTime {

// ==================================================
// INTERNAL STATE
// ==================================================

inline uint8_t& hour()
{
    static uint8_t value = 0;
    return value;
}

inline uint8_t& minute()
{
    static uint8_t value = 0;
    return value;
}

inline uint8_t& second()
{
    static uint8_t value = 0;
    return value;
}

inline bool& valid()
{
    static bool value = false;
    return value;
}


// ==================================================
// VALIDATE TIME
// ==================================================

inline bool isValid(
    int h,
    int m,
    int s = 0
)
{
    return
        h >= 0 &&
        h <= 23 &&
        m >= 0 &&
        m <= 59 &&
        s >= 0 &&
        s <= 59;
}


// ==================================================
// SET TIME
// ==================================================

inline bool setTime(
    int h,
    int m,
    int s = 0
)
{
    if (!isValid(h, m, s))
    {
        Serial.println(
            "[TIME] ERROR: Invalid time."
        );

        return false;
    }

    hour() =
        static_cast<uint8_t>(h);

    minute() =
        static_cast<uint8_t>(m);

    second() =
        static_cast<uint8_t>(s);

    valid() = true;

    Serial.printf(
        "[TIME] Time set: %02d:%02d:%02d\n",
        hour(),
        minute(),
        second()
    );

    return true;
}


// ==================================================
// PARSE TIME STRING
//
// قبول:
//
//   HH:MM
//   HH:MM:SS
//
// مثال:
//
//   22:41
//   22:41:30
// ==================================================

inline bool parseTime(
    const String& input
)
{
    String value = input;

    value.trim();

    if (value.length() == 0)
    {
        return false;
    }

    int firstColon =
        value.indexOf(':');

    if (firstColon <= 0)
    {
        Serial.println(
            "[TIME] ERROR: Invalid time format."
        );

        return false;
    }

    int secondColon =
        value.indexOf(
            ':',
            firstColon + 1
        );

    int h = 0;
    int m = 0;
    int s = 0;

    // --------------------------------------------------
    // HH:MM
    // --------------------------------------------------

    if (secondColon < 0)
    {
        String hourText =
            value.substring(
                0,
                firstColon
            );

        String minuteText =
            value.substring(
                firstColon + 1
            );

        if (
            hourText.length() == 0 ||
            minuteText.length() == 0
        )
        {
            return false;
        }

        h = hourText.toInt();
        m = minuteText.toInt();
        s = 0;
    }

    // --------------------------------------------------
    // HH:MM:SS
    // --------------------------------------------------

    else
    {
        String hourText =
            value.substring(
                0,
                firstColon
            );

        String minuteText =
            value.substring(
                firstColon + 1,
                secondColon
            );

        String secondText =
            value.substring(
                secondColon + 1
            );

        if (
            hourText.length() == 0 ||
            minuteText.length() == 0 ||
            secondText.length() == 0
        )
        {
            return false;
        }

        h = hourText.toInt();
        m = minuteText.toInt();
        s = secondText.toInt();
    }

    return setTime(
        h,
        m,
        s
    );
}


// ==================================================
// PARSE BLE TIME RESPONSE
//
// ورودی مورد انتظار:
//
//   TIME_RESPONSE|22:41
//
// یا:
//
//   TIME_RESPONSE|22:41:30
//
// ==================================================

inline bool parseResponse(
    const String& response
)
{
    String value = response;

    value.trim();

    const String prefix =
        "TIME_RESPONSE|";

    if (
        !value.startsWith(prefix)
    )
    {
        return false;
    }

    String timeValue =
        value.substring(
            prefix.length()
        );

    timeValue.trim();

    Serial.print(
        "[TIME] Response received: "
    );

    Serial.println(
        timeValue
    );

    return parseTime(
        timeValue
    );
}


// ==================================================
// GETTERS
// ==================================================

inline uint8_t getHour()
{
    return hour();
}


inline uint8_t getMinute()
{
    return minute();
}


inline uint8_t getSecond()
{
    return second();
}


inline bool isSet()
{
    return valid();
}


// ==================================================
// FORMAT TIME
//
// خروجی:
//
//   HH:MM:SS
// ==================================================

inline String getFormattedTime()
{
    char buffer[9];

    snprintf(
        buffer,
        sizeof(buffer),
        "%02u:%02u:%02u",
        hour(),
        minute(),
        second()
    );

    return String(buffer);
}


// ==================================================
// FORMAT SHORT TIME
//
// خروجی:
//
//   HH:MM
// ==================================================

inline String getFormattedShortTime()
{
    char buffer[6];

    snprintf(
        buffer,
        sizeof(buffer),
        "%02u:%02u",
        hour(),
        minute()
    );

    return String(buffer);
}


// ==================================================
// SHOW TIME ON OLED
// ==================================================

inline void showOnOLED()
{
    if (!valid())
    {
        Serial.println(
            "[TIME] Cannot display: time not set."
        );

        return;
    }

    // --------------------------------------------------
    // Send formatted time to OLED layer.
    // --------------------------------------------------

    avaOLEDSetTime(
        getFormattedShortTime()
    );

    avaOLEDSetMode(
        AVA_OLED_TIME
    );

    avaOLEDRenderTime();

    Serial.print(
        "[TIME] OLED display: "
    );

    Serial.println(
        getFormattedShortTime()
    );
}


// ==================================================
// HIDE TIME / RETURN TO NORMAL
// ==================================================

inline void hideFromOLED()
{
    avaOLEDSetMode(
        AVA_OLED_NORMAL
    );

    Serial.println(
        "[TIME] OLED returned to NORMAL mode."
    );
}


// ==================================================
// DEBUG
// ==================================================

inline void debugPrint()
{
    Serial.println();
    Serial.println(
        "=========== AVA TIME ==========="
    );

    Serial.print("Valid: ");

    Serial.println(
        valid()
            ? "YES"
            : "NO"
    );

    Serial.print("Hour: ");
    Serial.println(hour());

    Serial.print("Minute: ");
    Serial.println(minute());

    Serial.print("Second: ");
    Serial.println(second());

    Serial.print("Formatted: ");

    Serial.println(
        getFormattedTime()
    );

    Serial.println(
        "================================"
    );

    Serial.println();
}

} // namespace AvaTime

#endif
