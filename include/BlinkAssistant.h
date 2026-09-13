#ifndef AVA_BLINK_ASSISTANT_H
#define AVA_BLINK_ASSISTANT_H

#include <Arduino.h>
#include "AsyncTimer.h"

class Face;

// ==================================================
// AVA BLINK ASSISTANT
//
// Reference esp32-eyes architecture:
//
//     BlinkAssistant
//          ↓
//         Face
//       ↙     ↘
//   LeftEye  RightEye
//
// The actual EyeBlink engine remains Ava's own
// implementation.
// ==================================================

class BlinkAssistant {
public:
    explicit BlinkAssistant(Face& face);

    AsyncTimer Timer;

    // Call continuously from Ava's main loop.
    void Update();

    // Force an immediate blink.
    void Blink();

private:
    Face& _face;
};

// Compatibility bridge for Ava's existing main loop.
void avaBlinkAssistantUpdate();

#endif