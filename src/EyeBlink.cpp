#include "EyeBlink.h"

EyeBlink::EyeBlink()
{
}

void EyeBlink::SetInput(EyeConfig* input)
{
    _input = input;

    if (_input != nullptr && !_blinking) {
        Apply(0.0f);
    }
}

EyeConfig* EyeBlink::GetInput() const
{
    return _input;
}

void EyeBlink::Restart()
{
    if (_input == nullptr) {
        _blinking = false;
        return;
    }

    _startTime = millis();
    _blinking = true;

    // Start from the normal/open configuration.
    Apply(0.0f);
}

void EyeBlink::Update()
{
    if (_input == nullptr) {
        _blinking = false;
        return;
    }

    if (!_blinking) {
        Apply(0.0f);
        return;
    }

    const uint32_t elapsed =
        static_cast<uint32_t>(millis() - _startTime);

    const uint32_t totalDuration =
        static_cast<uint32_t>(
            CloseDuration +
            HoldDuration +
            OpenDuration
        );

    // Blink finished.
    if (elapsed >= totalDuration) {
        _blinking = false;
        Apply(0.0f);
        return;
    }

    float t = 0.0f;

    // ------------------------------------------
    // Closing: 0 -> 1
    // ------------------------------------------
    if (elapsed < CloseDuration) {

        t =
            static_cast<float>(elapsed) /
            static_cast<float>(CloseDuration);
    }

    // ------------------------------------------
    // Fully closed: 1
    // ------------------------------------------
    else if (
        elapsed <
        static_cast<uint32_t>(CloseDuration + HoldDuration)
    ) {

        t = 1.0f;
    }

    // ------------------------------------------
    // Opening: 1 -> 0
    // ------------------------------------------
    else {

        const uint32_t openElapsed =
            elapsed -
            static_cast<uint32_t>(
                CloseDuration + HoldDuration
            );

        t =
            1.0f -
            static_cast<float>(openElapsed) /
            static_cast<float>(OpenDuration);
    }

    // Smooth acceleration/deceleration.
    t = t * t * (3.0f - 2.0f * t);

    Apply(t);
}

void EyeBlink::Apply(float t)
{
    if (_input == nullptr) {
        return;
    }

    t = constrain(t, 0.0f, 1.0f);

    Output.OffsetX = _input->OffsetX;
    Output.OffsetY = _input->OffsetY;

    Output.Width =
        static_cast<int16_t>(
            _input->Width +
            (BlinkWidth - _input->Width) * t
        );

    Output.Height =
        static_cast<int16_t>(
            _input->Height +
            (BlinkHeight - _input->Height) * t
        );

    Output.Slope_Top =
        _input->Slope_Top * (1.0f - t);

    Output.Slope_Bottom =
        _input->Slope_Bottom * (1.0f - t);

    Output.Radius_Top =
        static_cast<int16_t>(
            _input->Radius_Top * (1.0f - t)
        );

    Output.Radius_Bottom =
        static_cast<int16_t>(
            _input->Radius_Bottom * (1.0f - t)
        );

    Output.Inverse_Radius_Top =
        static_cast<int16_t>(
            _input->Inverse_Radius_Top * (1.0f - t)
        );

    Output.Inverse_Radius_Bottom =
        static_cast<int16_t>(
            _input->Inverse_Radius_Bottom * (1.0f - t)
        );

    Output.Inverse_Offset_Top =
        static_cast<int16_t>(
            _input->Inverse_Offset_Top * (1.0f - t)
        );

    Output.Inverse_Offset_Bottom =
        static_cast<int16_t>(
            _input->Inverse_Offset_Bottom * (1.0f - t)
        );
}

bool EyeBlink::IsBlinking() const
{
    return _blinking;
}