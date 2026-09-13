#ifndef AVA_SENSES_H
#define AVA_SENSES_H

#include <Arduino.h>
#include "Ava_Eyes.h"

// ==================================================
// AVA SENSES SYSTEM
// Hardware-independent sensor layer
//
// فعلاً:
// - بدون سخت‌افزار واقعی
// - قابل شبیه‌سازی
// - آماده برای اتصال به سنسورهای واقعی
// ==================================================


// ==================================================
// Limits / thresholds
// ==================================================

static constexpr float AVA_GROUND_HEIGHT_WARNING_CM = 25.0f;
static constexpr float AVA_GROUND_HEIGHT_DANGER_CM  = 40.0f;

static constexpr float AVA_FRONT_OBSTACLE_WARNING_CM = 35.0f;
static constexpr float AVA_FRONT_OBSTACLE_DANGER_CM  = 15.0f;

static constexpr uint8_t AVA_LOW_BATTERY_PERCENT  = 20;
static constexpr uint8_t AVA_CRITICAL_BATTERY_PERCENT = 10;


// ==================================================
// Camera / Vision
// ==================================================

struct AvaVisionTarget {
  bool valid = false;

  // Position in normalized 0..100 coordinates
  int x = 50;
  int y = 50;

  // Approximate size / confidence
  uint8_t confidence = 0;
  uint16_t size = 0;

  // شناسه ساده برای آینده
  int id = -1;
};


struct AvaVisionState {
  bool cameraAvailable = false;
  bool targetDetected = false;

  uint8_t targetCount = 0;

  AvaVisionTarget selectedTarget;

  // حدوداً مرکز سوژه منتخب
  int targetCenterX = 50;
  int targetCenterY = 50;
};


// ==================================================
// Distance / Ultrasonic
// ==================================================

struct AvaDistanceState {
  bool ultrasonicAvailable = false;

  // فاصله سنسور جلویی
  float frontDistanceCm = -1.0f;

  // فاصله تا زمین
  float groundDistanceCm = -1.0f;

  bool frontObjectDetected = false;

  bool groundDetected = false;

  bool obstacleWarning = false;
  bool obstacleDanger = false;

  bool heightWarning = false;
  bool heightDanger = false;
};


// ==================================================
// Battery / Dock
// ==================================================

struct AvaPowerState {
  bool batterySensorAvailable = false;

  uint8_t batteryPercent = 0;

  bool charging = false;
  bool docked = false;
  bool fullyCharged = false;

  bool lowBattery = false;
  bool criticalBattery = false;

  unsigned long lastBatteryUpdateMs = 0;
};


// ==================================================
// Touch / Buttons
// ==================================================

struct AvaTouchState {
  bool touchSensorAvailable = false;

  bool touched = false;

  uint8_t touchCount = 0;

  unsigned long lastTouchMs = 0;
};


// ==================================================
// Light Sensor
// ==================================================

struct AvaLightState {
  bool lightSensorAvailable = false;

  uint16_t lightLevel = 0;

  bool dark = false;
  bool bright = false;
};


// ==================================================
// Sound / Microphone
// ==================================================

struct AvaSoundState {
  bool microphoneAvailable = false;

  uint16_t soundLevel = 0;

  bool soundDetected = false;
  bool loudSound = false;
};


// ==================================================
// General sensor status
// ==================================================

struct AvaSensorAvailability {
  bool camera = false;
  bool ultrasonic = false;
  bool battery = false;
  bool touch = false;
  bool light = false;
  bool microphone = false;
};


// ==================================================
// Complete AVA Sense State
// ==================================================

struct AvaSenseState {

  AvaVisionState vision;

  AvaDistanceState distance;

  AvaPowerState power;

  AvaTouchState touch;

  AvaLightState light;

  AvaSoundState sound;

  AvaSensorAvailability availability;

  // سیستم مرکزی
  bool anySensorAvailable = false;

  // خطر کلی
  bool dangerDetected = false;

  // آوا باید احتیاط کند
  bool cautionRequired = false;

  // آوا بهتر است عقب برود
  bool shouldBackAway = false;

  // آخرین زمان تغییر Sense
  unsigned long lastSenseUpdateMs = 0;
};


// ==================================================
// Singleton state
// ==================================================

inline AvaSenseState& avaSenses() {
  static AvaSenseState state;
  return state;
}


// ==================================================
// Internal state refresh
// ==================================================

inline void avaRefreshSenseFlags() {

  AvaSenseState& s = avaSenses();

  // ------------------------------
  // Front obstacle
  // ------------------------------

  s.distance.frontObjectDetected =
      (s.distance.frontDistanceCm >= 0.0f &&
       s.distance.frontDistanceCm <= AVA_FRONT_OBSTACLE_WARNING_CM);

  s.distance.obstacleWarning =
      (s.distance.frontDistanceCm >= 0.0f &&
       s.distance.frontDistanceCm <= AVA_FRONT_OBSTACLE_WARNING_CM);

  s.distance.obstacleDanger =
      (s.distance.frontDistanceCm >= 0.0f &&
       s.distance.frontDistanceCm <= AVA_FRONT_OBSTACLE_DANGER_CM);


  // ------------------------------
  // Ground / height
  // ------------------------------

  s.distance.groundDetected =
      (s.distance.groundDistanceCm >= 0.0f);

  s.distance.heightWarning =
      (s.distance.groundDistanceCm >= AVA_GROUND_HEIGHT_WARNING_CM);

  s.distance.heightDanger =
      (s.distance.groundDistanceCm >= AVA_GROUND_HEIGHT_DANGER_CM);


  // ------------------------------
  // Battery
  // ------------------------------

 s.power.lowBattery =
    s.power.batterySensorAvailable &&
    (s.power.batteryPercent <= AVA_LOW_BATTERY_PERCENT);

s.power.criticalBattery =
    s.power.batterySensorAvailable &&
    (s.power.batteryPercent <= AVA_CRITICAL_BATTERY_PERCENT);

  // ------------------------------
  // Light
  // ------------------------------

  s.light.dark =
      (s.light.lightLevel < 20);

  s.light.bright =
      (s.light.lightLevel > 80);


  // ------------------------------
  // Sound
  // ------------------------------

  s.sound.soundDetected =
      (s.sound.soundLevel > 15);

  s.sound.loudSound =
      (s.sound.soundLevel > 80);


  // ------------------------------
  // Overall danger
  // ------------------------------

  s.dangerDetected =
      s.distance.obstacleDanger ||
      s.distance.heightDanger ||
      s.power.criticalBattery;


  s.cautionRequired =
      s.distance.obstacleWarning ||
      s.distance.heightWarning ||
      s.power.lowBattery;


  // ------------------------------
  // Back away
  // ------------------------------

  // فعلاً فقط ارتفاع خطر
  // بعداً می‌توانیم مانع جلو را هم اضافه کنیم.
  s.shouldBackAway =
      s.distance.heightDanger ||
      s.distance.obstacleDanger;


  // ------------------------------
  // Sensor availability
  // ------------------------------

  s.availability.camera =
      s.vision.cameraAvailable;

  s.availability.ultrasonic =
      s.distance.ultrasonicAvailable;

  s.availability.battery =
      s.power.batterySensorAvailable;

  s.availability.touch =
      s.touch.touchSensorAvailable;

  s.availability.light =
      s.light.lightSensorAvailable;

  s.availability.microphone =
      s.sound.microphoneAvailable;


  s.anySensorAvailable =
      s.availability.camera ||
      s.availability.ultrasonic ||
      s.availability.battery ||
      s.availability.touch ||
      s.availability.light ||
      s.availability.microphone;


  s.lastSenseUpdateMs = millis();
}


// ==================================================
// Camera availability
// ==================================================

inline void avaSetCameraAvailable(bool available) {

  AvaSenseState& s = avaSenses();

  s.vision.cameraAvailable = available;

  avaRefreshSenseFlags();
}


// ==================================================
// Camera target count
// ==================================================

inline void avaSetCameraTargetCount(uint8_t count) {

  AvaSenseState& s = avaSenses();

  s.vision.targetCount = count;

  s.vision.targetDetected = (count > 0);

  avaRefreshSenseFlags();
}


// ==================================================
// Select one camera target
// ==================================================
//
// برای چند سوژه، فقط یک target انتخاب می‌کنیم.
// هر دو چشم همیشه به یک target نگاه می‌کنند.
//

inline void avaSetCameraTarget(
    int x,
    int y,
    uint8_t confidence = 100,
    int id = -1,
    uint16_t size = 0
) {

  AvaSenseState& s = avaSenses();

  s.vision.targetDetected = true;

  s.vision.selectedTarget.valid = true;
  s.vision.selectedTarget.x = constrain(x, 0, 100);
  s.vision.selectedTarget.y = constrain(y, 0, 100);
  s.vision.selectedTarget.confidence = confidence;
  s.vision.selectedTarget.id = id;
  s.vision.selectedTarget.size = size;

  s.vision.targetCenterX = s.vision.selectedTarget.x;
  s.vision.targetCenterY = s.vision.selectedTarget.y;

  // اتصال به سیستم چشم
  setGazeTarget(
      s.vision.targetCenterX,
      s.vision.targetCenterY
  );

  avaRefreshSenseFlags();
}


// ==================================================
// No camera target
// ==================================================

inline void avaClearCameraTarget() {

  AvaSenseState& s = avaSenses();

  s.vision.targetDetected = false;

  s.vision.selectedTarget.valid = false;

  s.vision.targetCount = 0;

  s.vision.targetCenterX = 50;
  s.vision.targetCenterY = 50;

  setGaze(GAZE_CENTER);

  avaRefreshSenseFlags();
}


// ==================================================
// Front ultrasonic sensor
// ==================================================

inline void avaSetFrontDistance(float distanceCm) {

  AvaSenseState& s = avaSenses();

  s.distance.ultrasonicAvailable = true;
  s.distance.frontDistanceCm = distanceCm;

  avaRefreshSenseFlags();
}


// ==================================================
// Ground ultrasonic sensor
// ==================================================

inline void avaSetGroundDistance(float distanceCm) {

  AvaSenseState& s = avaSenses();

  s.distance.ultrasonicAvailable = true;
  s.distance.groundDistanceCm = distanceCm;

  avaRefreshSenseFlags();
}


// ==================================================
// Height safety
// ==================================================

inline bool avaHeightDangerDetected() {

  return avaSenses().distance.heightDanger;
}


inline bool avaShouldBackAway() {

  return avaSenses().shouldBackAway;
}


// ==================================================
// Battery
// ==================================================

inline void avaSetBatteryPercent(uint8_t percent) {

  AvaSenseState& s = avaSenses();

  s.power.batterySensorAvailable = true;

  s.power.batteryPercent = constrain(percent, 0, 100);

  s.power.lastBatteryUpdateMs = millis();

  avaRefreshSenseFlags();
}


inline void avaSetCharging(bool charging) {

  AvaSenseState& s = avaSenses();

  s.power.charging = charging;

  avaRefreshSenseFlags();
}


inline void avaSetDocked(bool docked) {

  AvaSenseState& s = avaSenses();

  s.power.docked = docked;

  avaRefreshSenseFlags();
}


inline void avaSetFullyCharged(bool full) {

  AvaSenseState& s = avaSenses();

  s.power.fullyCharged = full;

  if (full) {
    s.power.batteryPercent = 100;
  }

  avaRefreshSenseFlags();
}


// ==================================================
// Touch
// ==================================================

inline void avaSetTouchSensorAvailable(bool available) {

  AvaSenseState& s = avaSenses();

  s.touch.touchSensorAvailable = available;

  avaRefreshSenseFlags();
}


inline void avaRegisterTouch() {

  AvaSenseState& s = avaSenses();

  s.touch.touched = true;
  s.touch.touchCount++;
  s.touch.lastTouchMs = millis();

  avaRefreshSenseFlags();
}


inline void avaClearTouch() {

  avaSenses().touch.touched = false;
}


// ==================================================
// Light
// ==================================================

inline void avaSetLightLevel(uint16_t level) {

  AvaSenseState& s = avaSenses();

  s.light.lightSensorAvailable = true;
  s.light.lightLevel = constrain(level, 0, 100);

  avaRefreshSenseFlags();
}


// ==================================================
// Microphone / Sound
// ==================================================

inline void avaSetSoundLevel(uint16_t level) {

  AvaSenseState& s = avaSenses();

  s.sound.microphoneAvailable = true;
  s.sound.soundLevel = constrain(level, 0, 100);

  avaRefreshSenseFlags();
}


// ==================================================
// Sensor status helpers
// ==================================================

inline bool avaCameraAvailable() {
  return avaSenses().vision.cameraAvailable;
}


inline bool avaUltrasonicAvailable() {
  return avaSenses().distance.ultrasonicAvailable;
}


inline bool avaBatteryAvailable() {
  return avaSenses().power.batterySensorAvailable;
}


// ==================================================
// Debug
// ==================================================

inline void avaSensesDebugPrint() {

  const AvaSenseState& s = avaSenses();

  Serial.println();
  Serial.println("=========== AVA SENSES ===========");

  // Camera
  Serial.print("Camera: ");
  Serial.println(
      s.vision.cameraAvailable ? "AVAILABLE" : "NOT AVAILABLE"
  );

  Serial.print("Target detected: ");
  Serial.println(
      s.vision.targetDetected ? "YES" : "NO"
  );

  Serial.print("Target count: ");
  Serial.println(s.vision.targetCount);

  if (s.vision.selectedTarget.valid) {
    Serial.print("Target X: ");
    Serial.println(s.vision.targetCenterX);

    Serial.print("Target Y: ");
    Serial.println(s.vision.targetCenterY);

    Serial.print("Target confidence: ");
    Serial.print(s.vision.selectedTarget.confidence);
    Serial.println("%");
  }

  // Distance
  Serial.println();

  Serial.print("Ultrasonic: ");
  Serial.println(
      s.distance.ultrasonicAvailable ? "AVAILABLE" : "NOT AVAILABLE"
  );

  Serial.print("Front distance: ");
  Serial.print(s.distance.frontDistanceCm);
  Serial.println(" cm");

  Serial.print("Ground distance: ");
  Serial.print(s.distance.groundDistanceCm);
  Serial.println(" cm");

  Serial.print("Obstacle warning: ");
  Serial.println(
      s.distance.obstacleWarning ? "YES" : "NO"
  );

  Serial.print("Obstacle danger: ");
  Serial.println(
      s.distance.obstacleDanger ? "YES" : "NO"
  );

  Serial.print("Height warning: ");
  Serial.println(
      s.distance.heightWarning ? "YES" : "NO"
  );

  Serial.print("Height danger: ");
  Serial.println(
      s.distance.heightDanger ? "YES" : "NO"
  );

  Serial.print("Should back away: ");
  Serial.println(
      s.shouldBackAway ? "YES" : "NO"
  );

  // Battery
  Serial.println();

  Serial.print("Battery: ");
  Serial.print(s.power.batteryPercent);
  Serial.println("%");

  Serial.print("Charging: ");
  Serial.println(s.power.charging ? "YES" : "NO");

  Serial.print("Docked: ");
  Serial.println(s.power.docked ? "YES" : "NO");

  Serial.print("Fully charged: ");
  Serial.println(s.power.fullyCharged ? "YES" : "NO");

  Serial.print("Low battery: ");
  Serial.println(s.power.lowBattery ? "YES" : "NO");

  Serial.print("Critical battery: ");
  Serial.println(s.power.criticalBattery ? "YES" : "NO");

  // Touch
  Serial.println();

  Serial.print("Touch available: ");
  Serial.println(
      s.touch.touchSensorAvailable ? "YES" : "NO"
  );

  Serial.print("Touch count: ");
  Serial.println(s.touch.touchCount);

  // Light
  Serial.print("Light level: ");
  Serial.println(s.light.lightLevel);

  Serial.print("Dark: ");
  Serial.println(s.light.dark ? "YES" : "NO");

  Serial.print("Bright: ");
  Serial.println(s.light.bright ? "YES" : "NO");

  // Sound
  Serial.print("Sound level: ");
  Serial.println(s.sound.soundLevel);

  Serial.print("Sound detected: ");
  Serial.println(s.sound.soundDetected ? "YES" : "NO");

  // Overall
  Serial.println();

  Serial.print("Any sensor available: ");
  Serial.println(s.anySensorAvailable ? "YES" : "NO");

  Serial.print("Caution required: ");
  Serial.println(s.cautionRequired ? "YES" : "NO");

  Serial.print("Danger detected: ");
  Serial.println(s.dangerDetected ? "YES" : "NO");

  Serial.println("==================================");
  Serial.println();
}


#endif