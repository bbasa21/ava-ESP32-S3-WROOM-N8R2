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
    const uint32_t now = millis();
    Timer.Update();

    // Occasionally add a quick second blink, as people naturally do.
    if (_doubleBlinkPending &&
        static_cast<int32_t>(now - _doubleBlinkAtMs) >= 0)
    {
        _doubleBlinkPending = false;
        Blink();
        return;
    }

    if (Timer.IsExpired()) {
        Blink();

        // Most blinks are single; a small fraction become a double blink.
        if (random(0, 100) < 12) {
            _doubleBlinkPending = true;
            _doubleBlinkAtMs = millis() + 180UL;
        }
    }
}

// ==================================================
// Force Blink
// ==================================================

void BlinkAssistant::Blink()
{
    _face.LeftEye.BlinkTransformation.Restart();
    _face.RightEye.BlinkTransformation.Restart();

    // Vary the pause between blinks instead of repeating on a fixed beat.
    Timer.SetIntervalMillis(static_cast<unsigned long>(random(2800, 6501)));
    Timer.Reset();
}
