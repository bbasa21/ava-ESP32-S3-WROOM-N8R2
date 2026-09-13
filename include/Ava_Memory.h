#ifndef AVA_MEMORY_H
#define AVA_MEMORY_H

#include <Arduino.h>
#include <string.h>
#include "Ava_Dance.h"

// ==================================================
// AVA Memory State
// ==================================================
struct AvaMemoryState {
  uint32_t bootCount = 0;
  uint32_t commandCount = 0;
  uint32_t danceCount = 0;

  unsigned long bootMs = 0;
  unsigned long lastInteractionMs = 0;
  unsigned long lastFullChargeMs = 0;
  unsigned long lastDockExitMs = 0;

  uint8_t lastPostChargeBatteryPercent = 0;
  bool hasLastPostChargeBatteryPercent = false;

  bool postChargeCapturePending = false;  // true after full charge, until first post-dock battery reading is saved
  bool docked = false;
  bool charging = false;

  AvaDanceStyle lastDanceStyle = DANCE_CUTE;
  bool hasLastDanceStyle = false;

  char lastCommand[48] = {0};
};

// Single shared instance (header-only, no .cpp needed)
inline AvaMemoryState& avaMemory() {
  static AvaMemoryState state;
  return state;
}

// ==================================================
// Boot / interaction tracking
// ==================================================
inline void avaMemoryOnBoot(unsigned long nowMs) {
  AvaMemoryState& m = avaMemory();
  m.bootCount++;
  m.bootMs = nowMs;
  m.lastInteractionMs = nowMs;
}

inline void avaMemoryOnInteraction(unsigned long nowMs) {
  avaMemory().lastInteractionMs = nowMs;
}

inline void avaMemoryOnCommandReceived(const String& cmd, unsigned long nowMs) {
  AvaMemoryState& m = avaMemory();
  m.commandCount++;
  m.lastInteractionMs = nowMs;
  snprintf(m.lastCommand, sizeof(m.lastCommand), "%s", cmd.c_str());
}

inline void avaMemoryOnDancePerformed(AvaDanceStyle style, unsigned long nowMs) {
  AvaMemoryState& m = avaMemory();
  m.danceCount++;
  m.lastDanceStyle = style;
  m.hasLastDanceStyle = true;
  m.lastInteractionMs = nowMs;
}

// ==================================================
// Charging / dock memory
// ==================================================
inline void avaMemoryMarkDocked(unsigned long nowMs) {
  AvaMemoryState& m = avaMemory();
  m.docked = true;
  m.charging = true;
  m.lastInteractionMs = nowMs;
  // New charging cycle starts, but we only save battery once after full charge + exit dock
  m.postChargeCapturePending = false;
}

inline void avaMemoryMarkFullyCharged(unsigned long nowMs) {
  AvaMemoryState& m = avaMemory();
  m.lastFullChargeMs = nowMs;
  m.charging = false;
  m.postChargeCapturePending = true;   // allow exactly one post-charge battery save
  m.lastInteractionMs = nowMs;
}

inline void avaMemoryMarkUndocked(unsigned long nowMs) {
  AvaMemoryState& m = avaMemory();
  m.docked = false;
  m.charging = false;
  m.lastDockExitMs = nowMs;
  m.lastInteractionMs = nowMs;
}

// Save battery percent only once after leaving dock / charge cycle
inline bool avaMemoryTryRecordPostChargeBattery(uint8_t batteryPercent, unsigned long nowMs) {
  AvaMemoryState& m = avaMemory();

  if (!m.postChargeCapturePending) {
    return false;
  }

  if (m.docked) {
    return false;
  }

  m.lastPostChargeBatteryPercent = batteryPercent;
  m.hasLastPostChargeBatteryPercent = true;
  m.postChargeCapturePending = false;  // lock until next full-charge cycle
  m.lastDockExitMs = nowMs;
  m.lastInteractionMs = nowMs;

  return true;
}

// Optional: reset the "one shot" post-charge capture manually
inline void avaMemoryResetPostChargeCapture() {
  avaMemory().postChargeCapturePending = false;
}

// ==================================================
// Debug print
// ==================================================
inline void avaMemoryDebugPrint() {
  const AvaMemoryState& m = avaMemory();

  Serial.println();
  Serial.println("=========== AVA MEMORY ===========");
  Serial.print("Boot count: ");
  Serial.println(m.bootCount);

  Serial.print("Command count: ");
  Serial.println(m.commandCount);

  Serial.print("Dance count: ");
  Serial.println(m.danceCount);

  Serial.print("Last command: ");
  Serial.println(m.lastCommand);

  Serial.print("Last dance style: ");
  Serial.println(m.hasLastDanceStyle ? avaDanceStyleName(m.lastDanceStyle) : "N/A");

  Serial.print("Last full charge (ms): ");
  Serial.println(m.lastFullChargeMs);

  Serial.print("Last dock exit (ms): ");
  Serial.println(m.lastDockExitMs);

  Serial.print("Last post-charge battery: ");
  if (m.hasLastPostChargeBatteryPercent) {
    Serial.print(m.lastPostChargeBatteryPercent);
    Serial.println("%");
  } else {
    Serial.println("N/A");
  }

  Serial.print("Docked: ");
  Serial.println(m.docked ? "YES" : "NO");

  Serial.print("Charging: ");
  Serial.println(m.charging ? "YES" : "NO");

  Serial.print("Post-charge capture pending: ");
  Serial.println(m.postChargeCapturePending ? "YES" : "NO");

  Serial.println("==================================");
  Serial.println();
}

#endif