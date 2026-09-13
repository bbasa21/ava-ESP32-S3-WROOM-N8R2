#ifndef AVA_EYE_TRANSITION_H
#define AVA_EYE_TRANSITION_H

#include <Arduino.h>
#include "Animations.h"
#include "EyeConfig.h"

// ============================================================
// AVA EYE TRANSITION
//
// Repository logic preserved:
//   Origin  -> current EyeConfig
//   Destin  -> target EyeConfig
//   Animation -> 500 ms RampAnimation
//
// Hardware/display independent.
// Flow:
//   EyeTransition -> EyeConfig -> EyeDrawer -> AvaDisplayAdapter
// ============================================================

class EyeTransition {
public:
    EyeTransition();

    EyeConfig* Origin = nullptr;
    EyeConfig Destin{};

    RampAnimation Animation;

    void Update();
    void Apply(float t);
};

#endif
