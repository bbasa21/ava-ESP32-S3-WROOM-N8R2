#ifndef AVA_EYE_VARIATION_H
#define AVA_EYE_VARIATION_H

#include <Arduino.h>
#include "Animations.h"
#include "EyeConfig.h"

// ==================================================
// AVA EYE VARIATION
// ==================================================
// Applies configurable shape/position variations to
// an EyeConfig using the existing Ava animation engine.
//
// Flow:
// EyeVariation -> EyeConfig -> EyeDrawer
//
// No U8g2 dependency.
// ==================================================

class EyeVariation {
public:
    EyeVariation();

    // Input configuration supplied by the caller.
    EyeConfig* Input = nullptr;

    // Resulting configuration after variation is applied.
    EyeConfig Output{};

    // Animation controlling the variation amount.
    TrapeziumPulseAnimation Animation;

    // Variation values. Each field is a delta applied to Input.
    EyeConfig Values{};

    // Reset all variation values to zero.
    void Clear();

    // Configure the five-part trapezium pulse.
    void SetInterval(
        uint16_t t0,
        uint16_t t1,
        uint16_t t2,
        uint16_t t3,
        uint16_t t4
    );

    // Update Output from the current animation position.
    void Update();

    // Apply an explicit variation amount in the range -1.0 .. +1.0.
    void Apply(float t);
};

#endif
