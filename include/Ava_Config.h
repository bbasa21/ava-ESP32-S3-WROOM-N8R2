#ifndef AVA_CONFIG_H
#define AVA_CONFIG_H

#include <Arduino.h>

// ==================================================
// AVA BASIC IDENTITY
// ==================================================

#define AVA_NAME "AVA"
#define OWNER_NAME "Ali"

// ==================================================
// AVA LOGICAL DEVICE ID
// ==================================================

#define AVA_DEVICE_ID "AVA-001"

// ==================================================
// AVA FIXED BASE MAC
// ==================================================
//
// Wi-Fi and Bluetooth addresses are derived from
// this Base MAC by the ESP32 networking stack.
//
// A6 = locally administered + unicast.
// This MAC is assigned only to Ava.
//

static constexpr uint8_t AVA_BASE_MAC[6] = {
    0xA6,
    0x4B,
    0x2D,
    0x76,
    0x31,
    0x90
};

#endif