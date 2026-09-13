#ifndef AVA_TOUCH_H
#define AVA_TOUCH_H

#include <Arduino.h>
#include "Ava_Senses.h"

// ==================================================
// AVA TOUCH SYSTEM
//
// Hardware:
//   5 × TTP223 Capacitive Touch Sensor
//
// فعلاً:
//   - GPIOهای واقعی مشخص نشده‌اند
//   - هیچ سخت‌افزار واقعی لازم نیست
//   - Simulation Mode برای تست نرم‌افزاری وجود دارد
//
// مسئولیت این فایل:
//   - خواندن 5 سنسور
//   - مدیریت Press / Hold / Release
//   - ارائه وضعیت هر سنسور
//   - تولید Event خام Touch
//
// تشخیص "نوازش" در لایه Behavior انجام می‌شود.
// ==================================================


// ==================================================
// CONFIGURATION
// ==================================================

static constexpr uint8_t AVA_TOUCH_SENSOR_COUNT = 5;


// --------------------------------------------------
// GPIO
//
// فعلاً همه -1 هستند.
// بعداً فقط این آرایه را تغییر می‌دهیم.
// --------------------------------------------------

static constexpr int AVA_TOUCH_PINS[
    AVA_TOUCH_SENSOR_COUNT
] = {
    4,   // Sensor 1
    5,   // Sensor 2
    6,   // Sensor 3
    7,   // Sensor 4
    15   // Sensor 5
};


// --------------------------------------------------
// Active level
// --------------------------------------------------

static constexpr uint8_t AVA_TOUCH_ACTIVE_LEVEL = HIGH;


// --------------------------------------------------
// Hold threshold
// --------------------------------------------------

static constexpr uint32_t AVA_TOUCH_HELD_THRESHOLD_MS = 500;


// ==================================================
// RAW TOUCH EVENTS
// ==================================================

enum AvaTouchEvent : uint8_t {

    AVA_TOUCH_NONE = 0,

    AVA_TOUCH_PRESSED,
    AVA_TOUCH_HELD,
    AVA_TOUCH_RELEASED
};


// ==================================================
// SENSOR STATE
// ==================================================

struct AvaTouchSensorState {

    bool configured = false;
    bool touched = false;
    bool previousTouched = false;

    bool heldReported = false;

    uint32_t pressStartMs = 0;
    uint32_t lastEventMs = 0;

    AvaTouchEvent lastEvent =
        AVA_TOUCH_NONE;
};


// ==================================================
// DRIVER STATE
// ==================================================

struct AvaTouchDriverState {

    bool initialized = false;

    AvaTouchSensorState sensors[
        AVA_TOUCH_SENSOR_COUNT
    ];

    AvaTouchEvent lastEvent =
        AVA_TOUCH_NONE;

    int8_t lastSensor = -1;

    uint32_t lastEventMs = 0;
};


// ==================================================
// SINGLETON
// ==================================================

inline AvaTouchDriverState& avaTouchDriver()
{
    static AvaTouchDriverState state;
    return state;
}


// ==================================================
// EVENT NAME
// ==================================================

inline const char* avaTouchEventName(
    AvaTouchEvent event
)
{
    switch (event) {

        case AVA_TOUCH_NONE:
            return "NONE";

        case AVA_TOUCH_PRESSED:
            return "PRESSED";

        case AVA_TOUCH_HELD:
            return "HELD";

        case AVA_TOUCH_RELEASED:
            return "RELEASED";

        default:
            return "UNKNOWN";
    }
}


// ==================================================
// SENSOR NAME
// ==================================================

inline const char* avaTouchSensorName(
    uint8_t sensorIndex
)
{
    switch (sensorIndex) {

        case 0:
            return "TOUCH_1";

        case 1:
            return "TOUCH_2";

        case 2:
            return "TOUCH_3";

        case 3:
            return "TOUCH_4";

        case 4:
            return "TOUCH_5";

        default:
            return "UNKNOWN";
    }
}


// ==================================================
// INITIALIZATION
// ==================================================

inline bool avaTouchBegin()
{
    AvaTouchDriverState& t =
        avaTouchDriver();

    t.initialized = false;
    t.lastEvent = AVA_TOUCH_NONE;
    t.lastSensor = -1;
    t.lastEventMs = 0;

    Serial.println();
    Serial.println(
        "=========== AVA TOUCH ==========="
    );

    Serial.print("Sensor count: ");
    Serial.println(
        AVA_TOUCH_SENSOR_COUNT
    );

    bool anyConfigured = false;

    for (uint8_t i = 0;
         i < AVA_TOUCH_SENSOR_COUNT;
         ++i)
    {
        AvaTouchSensorState& s =
            t.sensors[i];

        s.configured =
            AVA_TOUCH_PINS[i] >= 0;

        s.touched = false;
        s.previousTouched = false;
        s.heldReported = false;
        s.pressStartMs = 0;
        s.lastEventMs = 0;
        s.lastEvent = AVA_TOUCH_NONE;

        Serial.print(
            avaTouchSensorName(i)
        );
        Serial.print(" GPIO: ");
        Serial.println(
            AVA_TOUCH_PINS[i]
        );

        if (!s.configured) {
            continue;
        }

        anyConfigured = true;

        pinMode(
            AVA_TOUCH_PINS[i],
            INPUT
        );
    }

    if (!anyConfigured) {

        Serial.println(
            "[TOUCH] No GPIO configured."
        );

        Serial.println(
            "[TOUCH] Simulation mode available."
        );

        Serial.println(
            "================================="
        );
        Serial.println();

        return false;
    }

    t.initialized = true;

    Serial.println(
        "[TOUCH] Touch system initialized."
    );

    Serial.println(
        "================================="
    );
    Serial.println();

    return true;
}


// ==================================================
// RAW READ
// ==================================================

inline bool avaTouchReadRaw(
    uint8_t sensorIndex
)
{
    if (sensorIndex >=
        AVA_TOUCH_SENSOR_COUNT) {
        return false;
    }

    AvaTouchSensorState& s =
        avaTouchDriver()
            .sensors[sensorIndex];

    if (!s.configured) {
        return false;
    }

    return digitalRead(
        AVA_TOUCH_PINS[sensorIndex]
    ) == AVA_TOUCH_ACTIVE_LEVEL;
}


// ==================================================
// ACTIVE SENSOR COUNT
// ==================================================

inline uint8_t avaTouchActiveSensorCount()
{
    AvaTouchDriverState& t =
        avaTouchDriver();

    uint8_t count = 0;

    for (uint8_t i = 0;
         i < AVA_TOUCH_SENSOR_COUNT;
         ++i)
    {
        if (t.sensors[i].touched) {
            ++count;
        }
    }

    return count;
}


// ==================================================
// UPDATE ONE SENSOR
// ==================================================

inline AvaTouchEvent avaTouchUpdateSensor(
    uint8_t sensorIndex
)
{
    if (sensorIndex >=
        AVA_TOUCH_SENSOR_COUNT) {
        return AVA_TOUCH_NONE;
    }

    AvaTouchDriverState& t =
        avaTouchDriver();

    AvaTouchSensorState& s =
        t.sensors[sensorIndex];

    if (!s.configured) {
        return AVA_TOUCH_NONE;
    }

    const bool currentTouched =
        avaTouchReadRaw(sensorIndex);

    AvaTouchEvent event =
        AVA_TOUCH_NONE;


    // ----------------------------------------------
    // PRESSED
    // ----------------------------------------------

    if (currentTouched &&
        !s.previousTouched)
    {
        s.pressStartMs = millis();
        s.heldReported = false;

        event = AVA_TOUCH_PRESSED;
    }


    // ----------------------------------------------
    // HELD
    // ----------------------------------------------

    else if (currentTouched &&
             s.previousTouched)
    {
        const uint32_t heldTime =
            millis() - s.pressStartMs;

        if (heldTime >=
            AVA_TOUCH_HELD_THRESHOLD_MS)
        {
            if (!s.heldReported) {

                s.heldReported = true;

                event = AVA_TOUCH_HELD;
            }
        }
    }


    // ----------------------------------------------
    // RELEASED
    // ----------------------------------------------

    else if (!currentTouched &&
             s.previousTouched)
    {
        event = AVA_TOUCH_RELEASED;
    }


    // ----------------------------------------------
    // SAVE STATE
    // ----------------------------------------------

    s.touched = currentTouched;
    s.previousTouched = currentTouched;


    // ----------------------------------------------
    // EVENT PROCESSING
    // ----------------------------------------------

    if (event != AVA_TOUCH_NONE)
    {
        s.lastEvent = event;
        s.lastEventMs = millis();

        t.lastEvent = event;
        t.lastSensor =
            static_cast<int8_t>(
                sensorIndex
            );
        t.lastEventMs = millis();


        // ------------------------------------------
        // Ava Senses
        // ------------------------------------------

        if (event == AVA_TOUCH_PRESSED ||
            event == AVA_TOUCH_HELD)
        {
            avaRegisterTouch();
        }
        else if (
            event == AVA_TOUCH_RELEASED)
        {
            avaClearTouch();
        }


        // ------------------------------------------
        // Debug
        // ------------------------------------------

        Serial.print("[TOUCH] ");
        Serial.print(
            avaTouchSensorName(sensorIndex)
        );
        Serial.print(" -> ");
        Serial.println(
            avaTouchEventName(event)
        );
    }

    return event;
}


// ==================================================
// UPDATE ALL 5 SENSORS
// ==================================================

inline AvaTouchEvent avaTouchUpdate()
{
    AvaTouchEvent latestEvent =
        AVA_TOUCH_NONE;

    for (uint8_t i = 0;
         i < AVA_TOUCH_SENSOR_COUNT;
         ++i)
    {
        AvaTouchEvent event =
            avaTouchUpdateSensor(i);

        if (event != AVA_TOUCH_NONE) {
            latestEvent = event;
        }
    }

    return latestEvent;
}


// ==================================================
// LAST EVENT
// ==================================================

inline AvaTouchEvent avaTouchGetLastEvent()
{
    return avaTouchDriver().lastEvent;
}


// ==================================================
// LAST SENSOR
// ==================================================

inline int8_t avaTouchGetLastSensor()
{
    return avaTouchDriver().lastSensor;
}


// ==================================================
// SIMULATION
//
// برای تست بدون هیچ سخت‌افزار.
// ==================================================

inline AvaTouchEvent avaTouchSimulate(
    uint8_t sensorIndex,
    AvaTouchEvent simulatedEvent
)
{
    if (sensorIndex >=
        AVA_TOUCH_SENSOR_COUNT)
    {
        return AVA_TOUCH_NONE;
    }

    AvaTouchDriverState& t =
        avaTouchDriver();

    AvaTouchSensorState& s =
        t.sensors[sensorIndex];

    switch (simulatedEvent)
    {
        case AVA_TOUCH_PRESSED:

            s.touched = true;
            s.previousTouched = false;
            s.heldReported = false;
            s.pressStartMs = millis();
            s.lastEvent =
                AVA_TOUCH_PRESSED;

            t.lastEvent =
                AVA_TOUCH_PRESSED;

            t.lastSensor =
                static_cast<int8_t>(
                    sensorIndex
                );

            avaRegisterTouch();

            Serial.print(
                "[TOUCH SIM] "
            );
            Serial.print(
                avaTouchSensorName(sensorIndex)
            );
            Serial.println(
                " -> PRESSED"
            );

            break;


        case AVA_TOUCH_HELD:

            s.touched = true;
            s.previousTouched = true;
            s.heldReported = true;

            s.lastEvent =
                AVA_TOUCH_HELD;

            t.lastEvent =
                AVA_TOUCH_HELD;

            t.lastSensor =
                static_cast<int8_t>(
                    sensorIndex
                );

            avaRegisterTouch();

            Serial.print(
                "[TOUCH SIM] "
            );
            Serial.print(
                avaTouchSensorName(sensorIndex)
            );
            Serial.println(
                " -> HELD"
            );

            break;


        case AVA_TOUCH_RELEASED:

            s.touched = false;
            s.previousTouched = true;
            s.heldReported = false;

            s.lastEvent =
                AVA_TOUCH_RELEASED;

            t.lastEvent =
                AVA_TOUCH_RELEASED;

            t.lastSensor =
                static_cast<int8_t>(
                    sensorIndex
                );

            avaClearTouch();

            Serial.print(
                "[TOUCH SIM] "
            );
            Serial.print(
                avaTouchSensorName(sensorIndex)
            );
            Serial.println(
                " -> RELEASED"
            );

            break;


        default:
            break;
    }

    t.lastEventMs = millis();
    s.lastEventMs = millis();

    return simulatedEvent;
}


// ==================================================
// FULL SERIAL TEST
//
// تمام 5 سنسور تست می‌شوند.
// ==================================================

inline void avaTouchRunSerialTest()
{
    Serial.println();
    Serial.println(
        "======= AVA TOUCH TEST ======="
    );

    for (uint8_t i = 0;
         i < AVA_TOUCH_SENSOR_COUNT;
         ++i)
    {
        avaTouchSimulate(
            i,
            AVA_TOUCH_PRESSED
        );

        delay(150);

        avaTouchSimulate(
            i,
            AVA_TOUCH_HELD
        );

        delay(150);

        avaTouchSimulate(
            i,
            AVA_TOUCH_RELEASED
        );

        delay(150);
    }

    Serial.println(
        "=============================="
    );
    Serial.println();
}


// ==================================================
// DEBUG
// ==================================================

inline void avaTouchDebugPrint()
{
    const AvaTouchDriverState& t =
        avaTouchDriver();

    Serial.println();
    Serial.println(
        "=========== AVA TOUCH ==========="
    );

    Serial.print("Sensor count: ");
    Serial.println(
        AVA_TOUCH_SENSOR_COUNT
    );

    Serial.print("Driver initialized: ");
    Serial.println(
        t.initialized
            ? "YES"
            : "NO"
    );

    Serial.print("Active sensors: ");
    Serial.println(
        avaTouchActiveSensorCount()
    );

    Serial.print("Last sensor: ");

    if (t.lastSensor >= 0) {
        Serial.println(
            t.lastSensor + 1
        );
    }
    else {
        Serial.println("NONE");
    }

    Serial.print("Last event: ");
    Serial.println(
        avaTouchEventName(
            t.lastEvent
        )
    );

    Serial.print(
        "Held threshold: "
    );
    Serial.print(
        AVA_TOUCH_HELD_THRESHOLD_MS
    );
    Serial.println(" ms");

    Serial.println(
        "================================="
    );
    Serial.println();
}


#endif