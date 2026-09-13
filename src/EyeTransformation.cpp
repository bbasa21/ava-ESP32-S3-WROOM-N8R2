#include "EyeTransformation.h"

// ============================================================
// Constructor
// ============================================================

EyeTransformation::EyeTransformation()
    : Animation(200) {
}

// ============================================================
// Update
// ============================================================

void EyeTransformation::Update() {

    if (Input == nullptr) {
        return;
    }

    float t = Animation.GetValue();

    // Safety clamp.
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    Current.MoveX =
        (Destin.MoveX - Origin.MoveX) * t +
        Origin.MoveX;

    Current.MoveY =
        (Destin.MoveY - Origin.MoveY) * t +
        Origin.MoveY;

    Current.ScaleX =
        (Destin.ScaleX - Origin.ScaleX) * t +
        Origin.ScaleX;

    Current.ScaleY =
        (Destin.ScaleY - Origin.ScaleY) * t +
        Origin.ScaleY;

    Apply();
}

// ============================================================
// Apply
// ============================================================

void EyeTransformation::Apply() {

    if (Input == nullptr) {
        return;
    }

    // Position transformation.
    Output.OffsetX =
        Input->OffsetX + Current.MoveX;

    // Repository uses inverted Y for screen coordinates.
    Output.OffsetY =
        Input->OffsetY - Current.MoveY;

    // Scale transformation.
    Output.Width =
        Input->Width * Current.ScaleX;

    Output.Height =
        Input->Height * Current.ScaleY;

    // Preserve the remaining EyeConfig geometry.
    Output.Slope_Top =
        Input->Slope_Top;

    Output.Slope_Bottom =
        Input->Slope_Bottom;

    Output.Radius_Top =
        Input->Radius_Top;

    Output.Radius_Bottom =
        Input->Radius_Bottom;

    Output.Inverse_Radius_Top =
        Input->Inverse_Radius_Top;

    Output.Inverse_Radius_Bottom =
        Input->Inverse_Radius_Bottom;

    Output.Inverse_Offset_Top =
        Input->Inverse_Offset_Top;

    Output.Inverse_Offset_Bottom =
        Input->Inverse_Offset_Bottom;
}

// ============================================================
// Set destination
// ============================================================

void EyeTransformation::SetDestin(
    Transformation transformation
) {

    // Current state becomes the origin of
    // the next transition.
    Origin.MoveX =
        Current.MoveX;

    Origin.MoveY =
        Current.MoveY;

    Origin.ScaleX =
        Current.ScaleX;

    Origin.ScaleY =
        Current.ScaleY;

    // Store target.
    Destin.MoveX =
        transformation.MoveX;

    Destin.MoveY =
        transformation.MoveY;

    Destin.ScaleX =
        transformation.ScaleX;

    Destin.ScaleY =
        transformation.ScaleY;
}
