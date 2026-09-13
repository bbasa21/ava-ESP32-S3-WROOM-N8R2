#ifndef AVA_EYEBLINK_H
#define AVA_EYEBLINK_H

#include <Arduino.h>
#include "EyeConfig.h"

// ==================================================
// AVA EYE BLINK ENGINE - STAGE 3
// Non-blocking, independent of Animations / AsyncTimer.
// ==================================================

class EyeBlink {
public:
    EyeBlink();

    // Base eye configuration.
    void SetInput(EyeConfig* input);
    EyeConfig* GetInput() const;

    // Transformed configuration consumed by EyeDrawer.
    EyeConfig Output{};

    // Call from loop() continuously.
    void Update();

    // Apply animation progress [0..1].
    void Apply(float t);

    // Restart the blink immediately.
    void Restart();

    // True while the blink is active.
    bool IsBlinking() const;

    // Blink geometry.
    int32_t BlinkWidth = 38;
    int32_t BlinkHeight = 3;

    uint32_t CloseDuration = 70;
    uint32_t HoldDuration  = 45;
    uint32_t OpenDuration  = 90;
private:
    EyeConfig* _input = nullptr;
    bool _blinking = false;
    uint32_t _startTime = 0;
};

#endif
