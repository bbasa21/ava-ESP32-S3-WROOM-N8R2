#include "EyeVariation.h"

// Default animation: 0 -> 1 -> 0 over a 1000 ms cycle.
EyeVariation::EyeVariation()
    : Animation(0, 1000, 0, 1000, 0)
{
    Clear();
}

// ==================================================
// Clear variation values
// ==================================================

void EyeVariation::Clear()
{
    Values.OffsetX = 0;
    Values.OffsetY = 0;
    Values.Height = 0;
    Values.Width = 0;

    Values.Slope_Top = 0.0f;
    Values.Slope_Bottom = 0.0f;

    Values.Radius_Top = 0;
    Values.Radius_Bottom = 0;

    Values.Inverse_Radius_Top = 0;
    Values.Inverse_Radius_Bottom = 0;

    Values.Inverse_Offset_Top = 0;
    Values.Inverse_Offset_Bottom = 0;

    // Keep Output deterministic after Clear().
    if (Input != nullptr) {
        Output = *Input;
    } else {
        Output = EyeConfig{};
    }
}

// ==================================================
// Configure animation timing
// ==================================================

void EyeVariation::SetInterval(
    uint16_t t0,
    uint16_t t1,
    uint16_t t2,
    uint16_t t3,
    uint16_t t4
)
{
    Animation.SetInterval(t0, t1, t2, t3, t4);
    Animation.Restart();
}

// ==================================================
// Update from animation
// ==================================================

void EyeVariation::Update()
{
    if (Input == nullptr) {
        return;
    }

    const float t = 2.0f * Animation.GetValue() - 1.0f;
    Apply(t);
}

// ==================================================
// Apply variation
// ==================================================

void EyeVariation::Apply(float t)
{
    if (Input == nullptr) {
        return;
    }

    Output.OffsetX = Input->OffsetX + static_cast<int16_t>(Values.OffsetX * t);
    Output.OffsetY = Input->OffsetY + static_cast<int16_t>(Values.OffsetY * t);

    Output.Height = Input->Height + static_cast<int16_t>(Values.Height * t);
    Output.Width = Input->Width + static_cast<int16_t>(Values.Width * t);

    Output.Slope_Top = Input->Slope_Top + Values.Slope_Top * t;
    Output.Slope_Bottom = Input->Slope_Bottom + Values.Slope_Bottom * t;

    Output.Radius_Top =
        Input->Radius_Top + static_cast<int16_t>(Values.Radius_Top * t);

    Output.Radius_Bottom =
        Input->Radius_Bottom + static_cast<int16_t>(Values.Radius_Bottom * t);

    Output.Inverse_Radius_Top =
        Input->Inverse_Radius_Top +
        static_cast<int16_t>(Values.Inverse_Radius_Top * t);

    Output.Inverse_Radius_Bottom =
        Input->Inverse_Radius_Bottom +
        static_cast<int16_t>(Values.Inverse_Radius_Bottom * t);

    Output.Inverse_Offset_Top =
        Input->Inverse_Offset_Top +
        static_cast<int16_t>(Values.Inverse_Offset_Top * t);

    Output.Inverse_Offset_Bottom =
        Input->Inverse_Offset_Bottom +
        static_cast<int16_t>(Values.Inverse_Offset_Bottom * t);
}
