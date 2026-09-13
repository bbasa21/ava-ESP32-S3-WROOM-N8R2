#include "EyeTransition.h"

// ============================================================
// Constructor
// ============================================================

EyeTransition::EyeTransition()
    : Animation(500) {
}

// ============================================================
// Update
// ============================================================

void EyeTransition::Update() {

    if (Origin == nullptr) {
        return;
    }

    float t = Animation.GetValue();

    // Keep the repository's original transition behavior:
    // RampAnimation supplies the 0..1 interpolation value.
    Apply(t);
}

// ============================================================
// Apply transition
// ============================================================

void EyeTransition::Apply(float t) {

    if (Origin == nullptr) {
        return;
    }

    // Safety clamp.
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    Origin->OffsetX =
        Origin->OffsetX * (1.0f - t) +
        Destin.OffsetX * t;

    Origin->OffsetY =
        Origin->OffsetY * (1.0f - t) +
        Destin.OffsetY * t;

    Origin->Height =
        Origin->Height * (1.0f - t) +
        Destin.Height * t;

    Origin->Width =
        Origin->Width * (1.0f - t) +
        Destin.Width * t;

    Origin->Slope_Top =
        Origin->Slope_Top * (1.0f - t) +
        Destin.Slope_Top * t;

    Origin->Slope_Bottom =
        Origin->Slope_Bottom * (1.0f - t) +
        Destin.Slope_Bottom * t;

    Origin->Radius_Top =
        Origin->Radius_Top * (1.0f - t) +
        Destin.Radius_Top * t;

    Origin->Radius_Bottom =
        Origin->Radius_Bottom * (1.0f - t) +
        Destin.Radius_Bottom * t;

    Origin->Inverse_Radius_Top =
        Origin->Inverse_Radius_Top * (1.0f - t) +
        Destin.Inverse_Radius_Top * t;

    Origin->Inverse_Radius_Bottom =
        Origin->Inverse_Radius_Bottom * (1.0f - t) +
        Destin.Inverse_Radius_Bottom * t;

    Origin->Inverse_Offset_Top =
        Origin->Inverse_Offset_Top * (1.0f - t) +
        Destin.Inverse_Offset_Top * t;

    Origin->Inverse_Offset_Bottom =
        Origin->Inverse_Offset_Bottom * (1.0f - t) +
        Destin.Inverse_Offset_Bottom * t;
}
