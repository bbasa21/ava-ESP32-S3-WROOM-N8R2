#include "Eye.h"

Eye::Eye()
{
    ChainOperators();

    // Preserve the repository's natural idle variation timing.
    // Use EyeVariation's public API instead of touching the
    // private timing fields of TrapeziumPulseAnimation.
    Variation1.SetInterval(200, 200, 200, 200, 0);
    Variation2.SetInterval(0, 200, 200, 200, 200);
}

void Eye::ChainOperators()
{
    Transition.Origin = &Config;

    Transformation.Input = &Config;

    Variation1.Input = &Transformation.Output;

    Variation2.Input = &Variation1.Output;

    BlinkTransformation.SetInput(&Variation2.Output);

    FinalConfig = &BlinkTransformation.Output;
}

EyeConfig Eye::TransformPresetForMirror(
    const EyeConfig& preset,
    bool mirrored
)
{
    EyeConfig result = preset;

    // Match the repository's coordinate convention:
    // Y is inverted and horizontal slope changes sign
    // depending on which eye is mirrored.
    result.OffsetX = mirrored ? -preset.OffsetX : preset.OffsetX;
    result.OffsetY = -preset.OffsetY;

    result.Slope_Top =
        mirrored ? preset.Slope_Top : -preset.Slope_Top;

    result.Slope_Bottom =
        mirrored ? preset.Slope_Bottom : -preset.Slope_Bottom;

    return result;
}

void Eye::Update()
{
    Transition.Update();
    Transformation.Update();
    Variation1.Update();
    Variation2.Update();
    BlinkTransformation.Update();
}

void Eye::Draw()
{
    Update();

    if (FinalConfig == nullptr) {
        return;
    }

    EyeDrawer::Draw(
        CenterX,
        CenterY,
        FinalConfig
    );
}

void Eye::ApplyPreset(const EyeConfig& preset)
{
    Config = TransformPresetForMirror(
        preset,
        IsMirrored
    );

    // Apply immediately.
    Transition.Destin = Config;
    Transition.Animation.Restart();

    // Keep the transformation chain synchronized.
    Transformation.Current = Transformation.Origin;
    Transformation.Destin = Transformation.Origin;
    Transformation.Animation.Restart();

    Variation1.Clear();
    Variation2.Clear();

    BlinkTransformation.SetInput(&Variation2.Output);
    BlinkTransformation.Update();
}

void Eye::TransitionTo(const EyeConfig& preset)
{
    Transition.Destin = TransformPresetForMirror(
        preset,
        IsMirrored
    );

    Transition.Animation.Restart();
}

void Eye::SetCenter(int16_t x, int16_t y)
{
    CenterX = x;
    CenterY = y;
}

void Eye::SetMirrored(bool mirrored)
{
    if (IsMirrored == mirrored) {
        return;
    }

    IsMirrored = mirrored;
}

void Eye::Blink()
{
    BlinkTransformation.Restart();
}

bool Eye::IsBlinking() const
{
    return BlinkTransformation.IsBlinking();
}
