#ifndef AVA_FACE_H
#define AVA_FACE_H

#include <Arduino.h>
#include "Eye.h"
#include "BlinkAssistant.h"
#include "LookAssistant.h"
#include "FaceExpression.h"
#include "FaceBehavior.h"

// ============================================================
// AVA FACE - PHASE 6
//
// Hardware-independent face coordinator.
// The actual OLED remains behind EyeDrawer/AvaDisplayAdapter.
// ============================================================

class Face {
public:
    Face(uint16_t screenWidth, uint16_t screenHeight, uint16_t eyeSize);

    uint16_t Width = 0;
    uint16_t Height = 0;
    uint16_t CenterX = 0;
    uint16_t CenterY = 0;
    uint16_t EyeSize = 0;
    uint16_t EyeInterDistance = 4;

    Eye LeftEye;
    Eye RightEye;
    BlinkAssistant Blink;
    LookAssistant Look;
    FaceExpression Expression;
    FaceBehavior Behavior;

    bool RandomBehavior = true;
    bool RandomLook = true;
    bool RandomBlink = true;

    void Update();
    void Draw();
    void DoBlink();

    void LookLeft();
    void LookRight();
    void LookFront();
    void LookTop();
    void LookBottom();

    // Non-blocking replacement for the repository's old Wait().
    // It intentionally does nothing here; Ava's main loop remains free.
    void Wait(unsigned long milliseconds);
};

#endif
