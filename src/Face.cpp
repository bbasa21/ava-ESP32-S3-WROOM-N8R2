#include "Face.h"

Face::Face(
    uint16_t screenWidth,
    uint16_t screenHeight,
    uint16_t eyeSize
)
    : LeftEye(),
      RightEye(),
      Blink(*this),
      Look(*this),
      Expression(*this),
      Behavior(*this)
{
    Width = screenWidth;
    Height = screenHeight;
    EyeSize = eyeSize;

    CenterX = Width / 2;
    CenterY = Height / 2;

    LeftEye.SetMirrored(true);
    RightEye.SetMirrored(false);

    // Start with the reference engine's Normal emotion.
    Behavior.Clear();
    Behavior.SetEmotion(eEmotions::Normal, 1.0f);
    Behavior.GoToEmotion(eEmotions::Normal);
}

void Face::Update()
{
    if (RandomBehavior) Behavior.Update();
    if (RandomLook) Look.Update();
    if (RandomBlink) Blink.Update();

    Draw();
}

void Face::Draw()
{
    LeftEye.SetCenter(
        static_cast<int16_t>(
            CenterX - EyeSize / 2 - EyeInterDistance
        ),
        static_cast<int16_t>(CenterY)
    );

    RightEye.SetCenter(
        static_cast<int16_t>(
            CenterX + EyeSize / 2 + EyeInterDistance
        ),
        static_cast<int16_t>(CenterY)
    );

    LeftEye.Draw();
    RightEye.Draw();
}

void Face::DoBlink()
{
    Blink.Blink();
}

void Face::LookFront()
{
    Look.LookAt(0.0f, 0.0f);
}

void Face::LookRight()
{
    Look.LookAt(-1.0f, 0.0f);
}

void Face::LookLeft()
{
    Look.LookAt(1.0f, 0.0f);
}

void Face::LookTop()
{
    Look.LookAt(0.0f, -1.0f);
}

void Face::LookBottom()
{
    Look.LookAt(0.0f, 1.0f);
}

void Face::Wait(unsigned long milliseconds)
{
    // Kept for API compatibility.
    // Ava remains fully non-blocking.
    (void)milliseconds;
}
