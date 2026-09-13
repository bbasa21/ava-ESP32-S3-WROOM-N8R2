#ifndef AVA_EYE_TRANSFORMATION_H
#define AVA_EYE_TRANSFORMATION_H

#include <Arduino.h>
#include "Animations.h"
#include "EyeConfig.h"

// ============================================================
// AVA EYE TRANSFORMATION
//
// Repository behavior adapted for Ava.
//
// Flow:
//   EyeTransformation
//          ↓
//      EyeConfig
//          ↓
//      EyeDrawer
//          ↓
//   AvaDisplayAdapter
//
// No U8g2 / display dependency here.
// ============================================================

struct Transformation {
    float MoveX  = 0.0f;
    float MoveY  = 0.0f;
    float ScaleX = 1.0f;
    float ScaleY = 1.0f;
};

class EyeTransformation {
public:
    EyeTransformation();

    // Source eye configuration.
    EyeConfig* Input = nullptr;

    // Transformed output configuration.
    EyeConfig Output{};

    // Transformation states.
    Transformation Origin{};
    Transformation Current{};
    Transformation Destin{};

    // Repository-compatible 200 ms ramp.
    RampAnimation Animation;

    // Advance transformation and update Output.
    void Update();

    // Apply Current transformation to Input.
    void Apply();

    // Set a new target transformation.
    void SetDestin(Transformation transformation);
};

#endif
