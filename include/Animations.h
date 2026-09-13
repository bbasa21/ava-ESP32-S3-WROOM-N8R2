#ifndef AVA_ANIMATIONS_H
#define AVA_ANIMATIONS_H

#include <Arduino.h>

// ==================================================
// AVA ANIMATION ENGINE - PHASE 1
//
// Generic, display-independent animation primitives.
//
// Architecture:
//   Animation -> EyeConfig -> EyeDrawer
//                         -> AvaDisplayAdapter
//                         -> SSD1306
//
// This layer intentionally knows nothing about U8g2,
// Adafruit GFX, SSD1306, or any other display driver.
//
// Animation output range:
//   0.0f ... 1.0f
//
// The animation can be sampled either from its own
// internal clock (GetValue()) or from an explicit
// elapsed time (GetValue(elapsedMillis)).
// ==================================================

class IAnimation {
public:
    virtual ~IAnimation() = default;

    virtual float GetValue() = 0;
    virtual float GetValue(unsigned long elapsedMillis) = 0;
    virtual unsigned long GetElapsed() = 0;
    virtual void Restart() = 0;
};

// ==================================================
// Animation base
// ==================================================

class AnimationBase : public IAnimation {
public:
    explicit AnimationBase(unsigned long interval)
        : Interval(interval), StartTime(millis()) {}

    unsigned long Interval = 0;
    unsigned long StartTime = 0;

    void Restart() override {
        StartTime = millis();
    }

    float GetValue() override final {
        return Calculate(GetElapsed());
    }

    float GetValue(unsigned long elapsedMillis) override final {
        return Calculate(elapsedMillis);
    }

    unsigned long GetElapsed() override {
        return static_cast<unsigned long>(millis() - StartTime);
    }

protected:
    virtual float Calculate(unsigned long elapsedMillis) = 0;
};

// ==================================================
// Delta animation
//
// Holds at 0 until the interval expires, then becomes 1.
// ==================================================

class DeltaAnimation : public AnimationBase {
public:
    explicit DeltaAnimation(unsigned long interval)
        : AnimationBase(interval) {}

protected:
    float Calculate(unsigned long elapsedMillis) override {
        return (elapsedMillis < Interval) ? 0.0f : 1.0f;
    }
};

// ==================================================
// Step animation
//
// Same basic timing behavior as DeltaAnimation, kept as
// a separate semantic primitive for future behavior code.
// ==================================================

class StepAnimation : public AnimationBase {
public:
    explicit StepAnimation(unsigned long interval)
        : AnimationBase(interval) {}

protected:
    float Calculate(unsigned long elapsedMillis) override {
        return (elapsedMillis < Interval) ? 0.0f : 1.0f;
    }
};

// ==================================================
// Ramp animation
//
// Linear 0 -> 1, then stays at 1.
// ==================================================

class RampAnimation : public AnimationBase {
public:
    explicit RampAnimation(unsigned long interval)
        : AnimationBase(interval) {}

protected:
    float Calculate(unsigned long elapsedMillis) override {
        if (Interval == 0) {
            return 1.0f;
        }

        if (elapsedMillis < Interval) {
            return static_cast<float>(elapsedMillis) /
                   static_cast<float>(Interval);
        }

        return 1.0f;
    }
};

// ==================================================
// Triangle animation
//
// Repeating 0 -> 1 -> 0 waveform.
// ==================================================

class TriangleAnimation : public AnimationBase {
public:
    explicit TriangleAnimation(unsigned long interval)
        : AnimationBase(interval) {
        _t0 = interval / 2;
        _t1 = interval - _t0;
    }

    TriangleAnimation(unsigned long t0, unsigned long t1)
        : AnimationBase(t0 + t1), _t0(t0), _t1(t1) {}

protected:
    float Calculate(unsigned long elapsedMillis) override {
        if (Interval == 0) {
            return 1.0f;
        }

        const unsigned long elapsed = elapsedMillis % Interval;

        if (_t0 == 0) {
            return 1.0f;
        }

        if (elapsed < _t0) {
            return static_cast<float>(elapsed) /
                   static_cast<float>(_t0);
        }

        if (_t1 == 0) {
            return 0.0f;
        }

        return 1.0f -
               (static_cast<float>(elapsed - _t0) /
                static_cast<float>(_t1));
    }

private:
    unsigned long _t0 = 0;
    unsigned long _t1 = 0;
};

// ==================================================
// Trapezium animation
//
// Repeating 0 -> 1 -> 0 waveform with configurable
// rise, hold and fall sections.
// ==================================================

class TrapeziumAnimation : public AnimationBase {
public:
    explicit TrapeziumAnimation(unsigned long t)
        : AnimationBase(t) {
        _t0 = t / 3;
        _t1 = t / 3;
        _t2 = t - _t0 - _t1;
    }

    TrapeziumAnimation(unsigned long t0,
                       unsigned long t1,
                       unsigned long t2)
        : AnimationBase(t0 + t1 + t2),
          _t0(t0), _t1(t1), _t2(t2) {}

protected:
    float Calculate(unsigned long elapsedMillis) override {
        if (Interval == 0) {
            return 0.0f;
        }

        const unsigned long elapsed =
            (elapsedMillis > Interval) ? Interval : elapsedMillis;

        if (_t0 > 0 && elapsed < _t0) {
            return static_cast<float>(elapsed) /
                   static_cast<float>(_t0);
        }

        const unsigned long holdEnd = _t0 + _t1;

        if (elapsed < holdEnd) {
            return 1.0f;
        }

        if (_t2 == 0) {
            return 0.0f;
        }

        return 1.0f -
               (static_cast<float>(elapsed - holdEnd) /
                static_cast<float>(_t2));
    }

private:
    unsigned long _t0 = 0;
    unsigned long _t1 = 0;
    unsigned long _t2 = 0;
};

// ==================================================
// Trapezium pulse animation
//
// Configurable:
//   delay-before-rise
//   rise
//   hold
//   fall
//   delay-after-fall
//
// The active pulse itself is always 0 -> 1 -> 0.
// ==================================================

class TrapeziumPulseAnimation : public AnimationBase {
public:
    explicit TrapeziumPulseAnimation(unsigned long t)
        : AnimationBase(t) {
        SetInterval(static_cast<uint16_t>(t));
    }

    TrapeziumPulseAnimation(unsigned long t0,
                            unsigned long t1,
                            unsigned long t2)
        : AnimationBase(t0 + t1 + t2) {
        _t0 = 0;
        _t1 = t0;
        _t2 = t1;
        _t3 = t2;
        _t4 = 0;
        Interval = _t0 + _t1 + _t2 + _t3 + _t4;
    }

    TrapeziumPulseAnimation(unsigned long t0,
                            unsigned long t1,
                            unsigned long t2,
                            unsigned long t3,
                            unsigned long t4)
        : AnimationBase(t0 + t1 + t2 + t3 + t4),
          _t0(t0), _t1(t1), _t2(t2), _t3(t3), _t4(t4) {}

protected:
    float Calculate(unsigned long elapsedMillis) override {
        if (Interval == 0) {
            return 0.0f;
        }

        const unsigned long elapsed = elapsedMillis % Interval;
        const unsigned long riseEnd = _t0 + _t1;
        const unsigned long holdEnd = riseEnd + _t2;
        const unsigned long fallEnd = holdEnd + _t3;

        if (elapsed < _t0) {
            return 0.0f;
        }

        if (elapsed < riseEnd) {
            if (_t1 == 0) return 1.0f;
            return static_cast<float>(elapsed - _t0) /
                   static_cast<float>(_t1);
        }

        if (elapsed < holdEnd) {
            return 1.0f;
        }

        if (elapsed < fallEnd) {
            if (_t3 == 0) return 0.0f;
            return 1.0f -
                   (static_cast<float>(elapsed - holdEnd) /
                    static_cast<float>(_t3));
        }

        return 0.0f;
    }

public:
    // Set a symmetric pulse whose total active time is t.
    void SetInterval(uint16_t t) {
        _t0 = 0;
        _t1 = t / 3;
        _t2 = t - _t1 - _t1;
        _t3 = _t1;
        _t4 = 0;
        Interval = _t0 + _t1 + _t2 + _t3 + _t4;
    }

    void SetTriangle(uint16_t t, uint16_t delay) {
        _t0 = 0;
        _t1 = t / 2;
        _t2 = 0;
        _t3 = _t1;
        _t4 = delay;
        Interval = _t0 + _t1 + _t2 + _t3 + _t4;
    }

    void SetTriangleCuadrature(uint16_t t, uint16_t delay) {
        _t0 = delay;
        _t1 = t / 2;
        _t2 = 0;
        _t3 = _t1;
        _t4 = 0;
        Interval = _t0 + _t1 + _t2 + _t3 + _t4;
    }

    void SetPulse(uint16_t t, uint16_t delay) {
        _t0 = 0;
        _t1 = t / 3;
        _t2 = t - _t1 - _t1;
        _t3 = _t1;
        _t4 = delay;
        Interval = _t0 + _t1 + _t2 + _t3 + _t4;
    }

    void SetPulseCuadrature(uint16_t t, uint16_t delay) {
        _t0 = delay;
        _t1 = t / 3;
        _t2 = t - _t1 - _t1;
        _t3 = _t1;
        _t4 = 0;
        Interval = _t0 + _t1 + _t2 + _t3 + _t4;
    }

    void SetInterval(uint16_t t0,
                     uint16_t t1,
                     uint16_t t2,
                     uint16_t t3,
                     uint16_t t4) {
        _t0 = t0;
        _t1 = t1;
        _t2 = t2;
        _t3 = t3;
        _t4 = t4;
        Interval = _t0 + _t1 + _t2 + _t3 + _t4;
    }

private:
    unsigned long _t0 = 0;
    unsigned long _t1 = 0;
    unsigned long _t2 = 0;
    unsigned long _t3 = 0;
    unsigned long _t4 = 0;
};

#endif // AVA_ANIMATIONS_H
