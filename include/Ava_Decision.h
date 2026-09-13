#ifndef AVA_DECISION_H
#define AVA_DECISION_H

#include <Arduino.h>
#include "Ava_Senses.h"
#include "Ava_Eyes.h"

// ==================================================
// AVA DECISION ENGINE
//
// Sense -> Decision
//
// این فایل فقط تصمیم می‌گیرد.
// اجرای حرکت، صدا و رفتار اصلی بعداً در
// Ava_Behavior.h انجام می‌شود.
//
// هیچ وابستگی به Ava_Types.h ندارد.
// ==================================================


// ==================================================
// Decision IDs
// ==================================================

enum AvaDecision : uint8_t {
  AVA_DECISION_IDLE = 0,

  AVA_DECISION_HEIGHT_FEAR,
  AVA_DECISION_OBSTACLE_DANGER,
  AVA_DECISION_CRITICAL_BATTERY,

  AVA_DECISION_LOW_BATTERY,
  AVA_DECISION_CAUTION,

  AVA_DECISION_LOUD_SOUND,
  AVA_DECISION_TOUCH,
  AVA_DECISION_TARGET,

  AVA_DECISION_DARK
};


// ==================================================
// Decision State
// ==================================================

struct AvaDecisionState {

  AvaDecision current = AVA_DECISION_IDLE;

  bool danger = false;
  bool caution = false;

  bool shouldBackAway = false;
  bool shouldLookDown = false;

  bool targetAvailable = false;

  unsigned long lastDecisionMs = 0;
};


// ==================================================
// Singleton
// ==================================================

inline AvaDecisionState& avaDecision() {
  static AvaDecisionState state;
  return state;
}


// ==================================================
// Decision Name
// ==================================================

inline const char* avaDecisionName(AvaDecision decision) {

  switch (decision) {

    case AVA_DECISION_IDLE:
      return "IDLE";

    case AVA_DECISION_HEIGHT_FEAR:
      return "HEIGHT_FEAR";

    case AVA_DECISION_OBSTACLE_DANGER:
      return "OBSTACLE_DANGER";

    case AVA_DECISION_CRITICAL_BATTERY:
      return "CRITICAL_BATTERY";

    case AVA_DECISION_LOW_BATTERY:
      return "LOW_BATTERY";

    case AVA_DECISION_CAUTION:
      return "CAUTION";

    case AVA_DECISION_LOUD_SOUND:
      return "LOUD_SOUND";

    case AVA_DECISION_TOUCH:
      return "TOUCH";

    case AVA_DECISION_TARGET:
      return "TARGET";

    case AVA_DECISION_DARK:
      return "DARK";

    default:
      return "UNKNOWN";
  }
}


// ==================================================
// Calculate Decision
// ==================================================

inline AvaDecision avaCalculateDecision() {

  const AvaSenseState& s = avaSenses();

  // ------------------------------------------------
  // Priority 1: Critical battery
  // ------------------------------------------------

  if (s.power.criticalBattery) {
    return AVA_DECISION_CRITICAL_BATTERY;
  }


  // ------------------------------------------------
  // Priority 2: Height danger
  // ------------------------------------------------

  if (s.distance.heightDanger) {
    return AVA_DECISION_HEIGHT_FEAR;
  }


  // ------------------------------------------------
  // Priority 3: Dangerous obstacle
  // ------------------------------------------------

  if (s.distance.obstacleDanger) {
    return AVA_DECISION_OBSTACLE_DANGER;
  }


  // ------------------------------------------------
  // Priority 4: Low battery
  // ------------------------------------------------

  if (s.power.lowBattery) {
    return AVA_DECISION_LOW_BATTERY;
  }


  // ------------------------------------------------
  // Priority 5: General caution
  // ------------------------------------------------

  if (s.distance.heightWarning ||
      s.distance.obstacleWarning) {

    return AVA_DECISION_CAUTION;
  }


  // ------------------------------------------------
  // Priority 6: Touch
  // ------------------------------------------------

  if (s.touch.touched) {
    return AVA_DECISION_TOUCH;
  }


  // ------------------------------------------------
  // Priority 7: Loud sound
  // ------------------------------------------------

  if (s.sound.loudSound) {
    return AVA_DECISION_LOUD_SOUND;
  }


  // ------------------------------------------------
  // Priority 8: Camera target
  // ------------------------------------------------

  if (s.vision.targetDetected) {
    return AVA_DECISION_TARGET;
  }


  // ------------------------------------------------
  // Priority 9: Darkness
  // ------------------------------------------------

  if (s.light.dark) {
    return AVA_DECISION_DARK;
  }


  // ------------------------------------------------
  // Nothing special
  // ------------------------------------------------

  return AVA_DECISION_IDLE;
}


// ==================================================
// Apply Decision
//
// فقط Eyes/Gaze مربوط به Decision را اینجا اعمال می‌کنیم.
// حرکت واقعی و Voice بعداً در Behavior انجام می‌شوند.
// ==================================================

inline void avaApplyDecision(AvaDecision decision) {

  AvaDecisionState& d = avaDecision();

  d.current = decision;

  d.danger = false;
  d.caution = false;

  d.shouldBackAway = false;
  d.shouldLookDown = false;

  d.targetAvailable = false;


  // ------------------------------------------------
  // IDLE
  // ------------------------------------------------

  if (decision == AVA_DECISION_IDLE) {

    return;
  }


  // ------------------------------------------------
  // HEIGHT FEAR
  // ------------------------------------------------

  if (decision == AVA_DECISION_HEIGHT_FEAR) {

    d.danger = true;
    d.shouldBackAway = true;
    d.shouldLookDown = true;

    setGaze(GAZE_DOWN);

    Serial.println("AVA DECISION: HEIGHT FEAR");
    Serial.println("AVA DECISION: BACK AWAY");

    return;
  }


  // ------------------------------------------------
  // OBSTACLE
  // ------------------------------------------------

  if (decision == AVA_DECISION_OBSTACLE_DANGER) {

    d.danger = true;
    d.shouldBackAway = true;

    Serial.println("AVA DECISION: OBSTACLE DANGER");
    Serial.println("AVA DECISION: BACK AWAY");

    return;
  }


  // ------------------------------------------------
  // CRITICAL BATTERY
  // ------------------------------------------------

  if (decision == AVA_DECISION_CRITICAL_BATTERY) {

    d.danger = true;

    Serial.println("AVA DECISION: CRITICAL BATTERY");

    return;
  }


  // ------------------------------------------------
  // LOW BATTERY
  // ------------------------------------------------

  if (decision == AVA_DECISION_LOW_BATTERY) {

    d.caution = true;

    Serial.println("AVA DECISION: LOW BATTERY");

    return;
  }


  // ------------------------------------------------
  // CAUTION
  // ------------------------------------------------

  if (decision == AVA_DECISION_CAUTION) {

    d.caution = true;

    Serial.println("AVA DECISION: CAUTION");

    return;
  }


  // ------------------------------------------------
  // LOUD SOUND
  // ------------------------------------------------

  if (decision == AVA_DECISION_LOUD_SOUND) {

    d.caution = true;

    Serial.println("AVA DECISION: LOUD SOUND");

    return;
  }


  // ------------------------------------------------
  // TOUCH
  // ------------------------------------------------

  if (decision == AVA_DECISION_TOUCH) {

    Serial.println("AVA DECISION: TOUCH");

    return;
  }


  // ------------------------------------------------
  // TARGET
  // ------------------------------------------------

  if (decision == AVA_DECISION_TARGET) {

    d.targetAvailable = true;

    // خود Ava_Senses قبلاً target را به Gaze وصل می‌کند.
    // اینجا فقط تصمیم ثبت می‌شود.

    Serial.println("AVA DECISION: TARGET PRESENT");

    return;
  }


  // ------------------------------------------------
  // DARK
  // ------------------------------------------------

  if (decision == AVA_DECISION_DARK) {

    d.caution = true;

    Serial.println("AVA DECISION: DARK");

    return;
  }
}


// ==================================================
// Decision Update
// ==================================================

inline AvaDecision avaDecisionUpdate() {

  // اول Senseها دوباره محاسبه شوند
  avaRefreshSenseFlags();

  // تصمیم جدید
  AvaDecision decision = avaCalculateDecision();

  // فقط وقتی تصمیم تغییر کرده، خروجی را اعمال کن
  if (decision != avaDecision().current) {
    avaApplyDecision(decision);
  }

  avaDecision().lastDecisionMs = millis();

  return decision;
}


// ==================================================
// Helpers
// ==================================================

inline AvaDecision avaCurrentDecision() {
  return avaDecision().current;
}


inline bool avaDecisionDanger() {
  return avaDecision().danger;
}


inline bool avaDecisionCaution() {
  return avaDecision().caution;
}


inline bool avaDecisionBackAway() {
  return avaDecision().shouldBackAway;
}


inline bool avaDecisionLookDown() {
  return avaDecision().shouldLookDown;
}


inline bool avaDecisionHasTarget() {
  return avaDecision().targetAvailable;
}


// ==================================================
// Debug
// ==================================================

inline void avaDecisionDebugPrint() {

  const AvaDecisionState& d = avaDecision();
  const AvaSenseState& s = avaSenses();

  Serial.println();
  Serial.println("=========== AVA DECISION ===========");

  Serial.print("Decision: ");
  Serial.println(avaDecisionName(d.current));

  Serial.print("Danger: ");
  Serial.println(d.danger ? "YES" : "NO");

  Serial.print("Caution: ");
  Serial.println(d.caution ? "YES" : "NO");

  Serial.print("Back away: ");
  Serial.println(d.shouldBackAway ? "YES" : "NO");

  Serial.print("Look down: ");
  Serial.println(d.shouldLookDown ? "YES" : "NO");

  Serial.print("Target available: ");
  Serial.println(d.targetAvailable ? "YES" : "NO");

  Serial.println();

  Serial.print("Ground distance: ");
  Serial.print(s.distance.groundDistanceCm);
  Serial.println(" cm");

  Serial.print("Front distance: ");
  Serial.print(s.distance.frontDistanceCm);
  Serial.println(" cm");

  Serial.print("Height warning: ");
  Serial.println(s.distance.heightWarning ? "YES" : "NO");

  Serial.print("Height danger: ");
  Serial.println(s.distance.heightDanger ? "YES" : "NO");

  Serial.print("Obstacle warning: ");
  Serial.println(s.distance.obstacleWarning ? "YES" : "NO");

  Serial.print("Obstacle danger: ");
  Serial.println(s.distance.obstacleDanger ? "YES" : "NO");

  Serial.print("Battery: ");
  Serial.print(s.power.batteryPercent);
  Serial.println("%");

  Serial.print("Low battery: ");
  Serial.println(s.power.lowBattery ? "YES" : "NO");

  Serial.print("Critical battery: ");
  Serial.println(s.power.criticalBattery ? "YES" : "NO");

  Serial.print("Sound level: ");
  Serial.println(s.sound.soundLevel);

  Serial.print("Loud sound: ");
  Serial.println(s.sound.loudSound ? "YES" : "NO");

  Serial.print("Touch: ");
  Serial.println(s.touch.touched ? "YES" : "NO");

  Serial.print("Dark: ");
  Serial.println(s.light.dark ? "YES" : "NO");

  Serial.print("Target detected: ");
  Serial.println(s.vision.targetDetected ? "YES" : "NO");

  Serial.println("====================================");
  Serial.println();
}


#endif