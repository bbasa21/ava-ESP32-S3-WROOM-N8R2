#include "BlinkAssistant.h"
#include "Face.h"

BlinkAssistant::BlinkAssistant(Face& face)
    : _face(face),
      Timer(3500)
{
    Timer.Start();
}

// ==================================================
// Update
// ==================================================

void BlinkAssistant::Update()
{
    Timer.Update();

    if (Timer.IsExpired()) {
        Blink();
    }
}

// ==================================================
// Force Blink
// ==================================================

void BlinkAssistant::Blink()
{
    _face.LeftEye.BlinkTransformation.Restart();
    _face.RightEye.BlinkTransformation.Restart();

    Timer.Reset();
}
