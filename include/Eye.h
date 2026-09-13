#ifndef AVA_EYE_H
#define AVA_EYE_H

#include <Arduino.h>
#include "EyeConfig.h"
#include "EyeDrawer.h"
#include "EyeTransition.h"
#include "EyeTransformation.h"
#include "EyeVariation.h"
#include "EyeBlink.h"

// ============================================================
// AVA EYE - STAGE 4
//
// Public eye API for Ava.
//
// Flow:
//   Eye
//    -> EyeConfig
//    -> EyeTransition
//    -> EyeTransformation
//    -> EyeVariation
//    -> EyeBlink
//    -> EyeDrawer
//    -> AvaDisplayAdapter
//    -> SSD1306
//
// No Face dependency.
// No Common.h dependency.
// No U8g2 dependency.
// ============================================================

class Eye {
public:
    Eye();

    // Position of this eye on the Ava OLED.
    int16_t CenterX = 0;
    int16_t CenterY = 0;

    // Mirror the horizontal geometry for the opposite eye.
    bool IsMirrored = false;

    // Base configuration.
    EyeConfig Config{};

    // Final configuration consumed by EyeDrawer.
    EyeConfig* FinalConfig = nullptr;

    // Animation/operator chain.
    EyeTransition Transition;
    EyeTransformation Transformation;
    EyeVariation Variation1;
    EyeVariation Variation2;
    EyeBlink BlinkTransformation;

    // Apply a preset immediately as the new base configuration.
    void ApplyPreset(const EyeConfig& preset);

    // Animate from the current configuration to a new preset.
    void TransitionTo(const EyeConfig& preset);

    // Update all eye operators without drawing.
    void Update();

    // Update the eye and send the final configuration to EyeDrawer.
    void Draw();

    // Convenience controls.
    void SetCenter(int16_t x, int16_t y);
    void SetMirrored(bool mirrored);

    // Trigger a blink on this eye.
    void Blink();

    bool IsBlinking() const;

private:
    void ChainOperators();

    static EyeConfig TransformPresetForMirror(
        const EyeConfig& preset,
        bool mirrored
    );
};

#endif
