#include "Ava_Communication.h"

#include <Arduino.h>

#include "Ava_Bluetooth.h"
#include "Ava_OLED.h"
#include "Ava_Eyes.h"
#include "Ava_DisplaySettings.h"
#include "Ava_WiFi.h"

// ============================================================
// AVA COMMUNICATION
//
// مسئولیت:
//
//   BLE DATA
//        ↓
//   AvaCommunication
//        ↓
//   Command / Response
//        ↓
//   Application / OLED
//
// فرمان‌های فعلی:
//
//   TIME_RESPONSE|22:41
//   TIME_RESPONSE|22:41:30
//   TIME_RESPONSE|22:41:30|2026-09-07
//   HOW'S THE WEATHER?
//
// مسیر Weather:
//
//   Phone / Serial
//        ↓
//   BLE DATA
//        ↓
//   avaCommunicationUpdate()
//        ↓
//   avaWiFiGetTehranWeather()
//        ↓
//   Open-Meteo
//        ↓
//   Weather State
//        ↓
//   avaOLEDShowWeather()
//
// ============================================================


// ============================================================
// INTERNAL STATE
// ============================================================

static String communicationLastMessage = "";

static bool communicationMessagePending = false;


// ============================================================
// BEGIN
// ============================================================

void avaCommunicationBegin()
{
    communicationLastMessage = "";

    communicationMessagePending = false;

    Serial.println(
        "[COMM] Communication system initialized."
    );
}


// ============================================================
// WEATHER COMMAND NORMALIZATION
//
// هدف:
//
// اجازه دادن به چند شکل طبیعی برای درخواست هوا:
//
//   HOW'S THE WEATHER?
//   HOWS THE WEATHER?
//   HOW'S WEATHER?
//   HOWS WEATHER?
//
// حروف کوچک / بزرگ مهم نیستند.
//
// ============================================================

static bool avaIsWeatherCommand(String command)
{
    command.trim();

    command.toUpperCase();

    command.replace(
        " ",
        ""
    );

    command.replace(
        "\t",
        ""
    );


    // HOW'S THE WEATHER?
    if (
        command == "HOW'STHEWEATHER?"
    )
    {
        return true;
    }


    // HOWS THE WEATHER?
    if (
        command == "HOWSTHEWEATHER?"
    )
    {
        return true;
    }


    // HOW'S WEATHER?
    if (
        command == "HOW'SWEATHER?"
    )
    {
        return true;
    }


    // HOWS WEATHER?
    if (
        command == "HOWSWEATHER?"
    )
    {
        return true;
    }


    // نسخه بدون ?
    if (
        command == "HOWSTHEWEATHER"
    )
    {
        return true;
    }


    if (
        command == "HOWSWEATHER"
    )
    {
        return true;
    }


    return false;
}


// ============================================================
// UPDATE
//
// این تابع باید در loop() اجرا شود.
//
// وظیفه:
//
//   1. بررسی DATA دریافتی از BLE
//   2. خواندن آخرین پیام
//   3. تشخیص TIME_RESPONSE
//   4. تشخیص Weather Command
//   5. اجرای Weather Request
//
// ============================================================

void avaCommunicationUpdate()
{
    // --------------------------------------------------------
    // هیچ DATA جدیدی از BLE نداریم
    // --------------------------------------------------------

    if (!AvaBluetooth::hasData())
    {
        return;
    }


    // --------------------------------------------------------
    // دریافت DATA
    // --------------------------------------------------------

    String data =
        AvaBluetooth::readData();

    data.trim();


    if (
        data.length() == 0
    )
    {
        return;
    }


    // --------------------------------------------------------
    // ذخیره آخرین پیام
    // --------------------------------------------------------

    communicationLastMessage =
        data;

    communicationMessagePending =
        true;


    Serial.print(
        "[COMM RX] "
    );

    Serial.println(
        data
    );


    // ========================================================
    // TIME RESPONSE
    //
    // فرمت:
    //
    // TIME_RESPONSE|22:41
    //
    // یا:
    //
    // TIME_RESPONSE|22:41:30
    //
    // یا:
    //
    // TIME_RESPONSE|22:41:30|2026-09-07
    //
    // ========================================================

    const String timePrefix =
        "TIME_RESPONSE|";


    if (
        data.startsWith(
            timePrefix
        )
    )
    {
        String time =
            data.substring(
                timePrefix.length()
            );

        time.trim();


        // ----------------------------------------------------
        // اگر تاریخ هم همراه زمان ارسال شده باشد،
        // فقط بخش اول یعنی TIME را نگه می‌داریم.
        //
        // مثال:
        //
        // 22:41:30|2026-09-07
        //
        // تبدیل می‌شود به:
        //
        // 22:41:30
        // ----------------------------------------------------

        int separator =
            time.indexOf('|');


        if (
            separator >= 0
        )
        {
            time =
                time.substring(
                    0,
                    separator
                );

            time.trim();
        }


        // ----------------------------------------------------
        // Validate
        // ----------------------------------------------------

        if (
            time.length() == 0
        )
        {
            Serial.println(
                "[COMM] ERROR: Empty time response."
            );

            return;
        }


        // ----------------------------------------------------
        // Log
        // ----------------------------------------------------

        Serial.print(
            "[COMM] Time response received: "
        );

        Serial.println(
            time
        );


        // ----------------------------------------------------
        // Store time
        // ----------------------------------------------------

        avaOLEDSetTime(
            time
        );


        // ----------------------------------------------------
        // Switch OLED to TIME mode
        // ----------------------------------------------------

        avaOLEDSetMode(
            AVA_OLED_TIME
        );


        Serial.println(
            "[COMM] Time delivered to OLED."
        );


        return;
    }


    // ========================================================
    // ========================================================
    // TIC TAC TOE LOSS REACTION
    // ========================================================

    if (data == "TTT_GLEE")
    {
        avaApplyEmotion(AVA_EMOTION_GLEE);
        Serial.println("[COMM] TTT loss reaction: GLEE.");
        return;
    }

    if (data == "TTT_GLEE_END")
    {
        avaApplyEmotion(AVA_EMOTION_NORMAL);
        Serial.println("[COMM] TTT loss reaction ended.");
        return;
    }

    if (data == "CALM")
    {
        avaApplyEmotion(AVA_EMOTION_NORMAL);
        Serial.println("[COMM] CALM command: normal emotion.");
        return;
    }

    // ========================================================
    // DISPLAY SETTINGS
    // ========================================================

    if (data == "SETTINGS_ENTER")
    {
        avaDisplaySettingsEnter();
        avaCommunicationSend(
            "SETTINGS_READY|" +
            String(avaDisplaySettingsBrightness()) + "|" +
            String(avaDisplaySettingsContrast()) + "|" +
            avaDisplaySettingsMode()
        );
        return;
    }

    if (data == "SETTINGS_EXIT")
    {
        avaDisplaySettingsExit();
        return;
    }

    if (data == "DISPLAY_APPLY")
    {
        if (!avaDisplaySettingsActive())
        {
            avaCommunicationSend("DISPLAY_APPLY_REJECTED|NO_SESSION");
            return;
        }
        avaDisplaySettingsApply();
        avaCommunicationSend("DISPLAY_APPLIED|" + String(avaDisplaySettingsBrightness()) + "|" + String(avaDisplaySettingsContrast()) + "|" + avaDisplaySettingsMode());
        return;
    }

    if (data.startsWith("DISPLAY_SET|"))
    {
        if (!avaDisplaySettingsActive())
        {
            avaCommunicationSend("DISPLAY_SET_REJECTED|NO_SESSION");
            return;
        }
        int first = data.indexOf('|');
        int second = data.indexOf('|', first + 1);
        if (first < 0 || second < 0)
        {
            avaCommunicationSend("DISPLAY_SET_REJECTED|FORMAT");
            return;
        }
        String key = data.substring(first + 1, second);
        String value = data.substring(second + 1);
        key.trim();
        value.trim();
        key.toUpperCase();

        if (key == "BRIGHTNESS")
        {
            int number = value.toInt();
            if (number < 0 || number > 255) { avaCommunicationSend("DISPLAY_SET_REJECTED|BRIGHTNESS"); return; }
            avaDisplaySettingsSetBrightness((uint8_t)number);
        }
        else if (key == "CONTRAST")
        {
            int number = value.toInt();
            if (number < 0 || number > 255) { avaCommunicationSend("DISPLAY_SET_REJECTED|CONTRAST"); return; }
            avaDisplaySettingsSetContrast((uint8_t)number);
        }
        else if (key == "MODE")
        {
            avaDisplaySettingsSetMode(value);
        }
        else
        {
            avaCommunicationSend("DISPLAY_SET_REJECTED|UNKNOWN");
            return;
        }

        avaCommunicationSend("DISPLAY_PREVIEW|" + String(avaDisplaySettingsBrightness()) + "|" + String(avaDisplaySettingsContrast()) + "|" + avaDisplaySettingsMode());
        return;
    }

    // WEATHER COMMAND
    //
    // مثال:
    //
    // HOW'S THE WEATHER?
    //
    // ========================================================

    if (
        avaIsWeatherCommand(
            data
        )
    )
    {
        Serial.println();

        Serial.println(
            "[COMM] Weather command detected."
        );

        Serial.println(
            "[COMM] Requesting Tehran weather..."
        );


        // ----------------------------------------------------
        // درخواست مستقیم Weather
        // ----------------------------------------------------

        bool success =
            avaWiFiGetTehranWeather();


        // ----------------------------------------------------
        // نتیجه
        // ----------------------------------------------------

        if (
            success
        )
        {
            Serial.println(
                "[COMM] Weather request completed successfully."
            );
        }
        else
        {
            Serial.println(
                "[COMM] Weather request FAILED."
            );
        }


        Serial.println();

        return;
    }


    // ========================================================
    // UNKNOWN DATA
    // ========================================================

    Serial.print(
        "[COMM] Unknown DATA: "
    );

    Serial.println(
        data
    );
}


// ============================================================
// SEND MESSAGE
//
// ارسال عمومی Communication از طریق BLE.
//
// مثال:
//
//   avaCommunicationSend(
//       "TIME_REQUEST"
//   );
//
// ============================================================

void avaCommunicationSend(
    const String& message
)
{
    String value =
        message;

    value.trim();


    if (
        value.length() == 0
    )
    {
        return;
    }


    Serial.print(
        "[COMM TX] "
    );

    Serial.println(
        value
    );


    // --------------------------------------------------------
    // TIME REQUEST
    //
    // این پیام باید از EVENT_UUID برای Android ارسال شود،
    // چون Android برای TIME_REQUEST روی EVENT گوش می‌دهد.
    //
    // بقیه پیام‌های Communication همچنان از DATA_UUID
    // ارسال می‌شوند.
    // --------------------------------------------------------

    if (
        value == "TIME_REQUEST"
    )
    {
        AvaBluetooth::sendEvent(
            value
        );
    }
    else
    {
        AvaBluetooth::sendData(
            value
        );
    }
}


// ============================================================
// CHECK COMMAND / MESSAGE
// ============================================================

bool avaCommunicationHasCommand()
{
    return communicationMessagePending;
}


// ============================================================
// READ COMMAND / MESSAGE
// ============================================================

String avaCommunicationReadCommand()
{
    if (
        !communicationMessagePending
    )
    {
        return "";
    }


    String message =
        communicationLastMessage;


    communicationLastMessage =
        "";

    communicationMessagePending =
        false;


    return message;
}