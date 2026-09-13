#ifndef AVA_ACHIEVEMENTS_H
#define AVA_ACHIEVEMENTS_H

#include <Arduino.h>
#include "Ava_Memory.h"
#include "Ava_Dance.h"
#include "Ava_Types.h"

// ==================================================
// AVA Achievements
// Source of truth: Firmware
// App only displays synced state later
// ==================================================
enum AvaAchievementId : uint8_t {
  ACH_FIRST_COMMAND = 0,
  ACH_FIRST_DANCE,
  ACH_TEN_COMMANDS,
  ACH_TEN_DANCES,
  ACH_DANCE_VARIETY,
  ACH_BANDARI_QUEEN,
  ACH_HAPPY_AVA,
  ACH_ONE_YEAR_TOGETHER,
  ACHIEVEMENT_COUNT
};

struct AvaAchievementInfo {
  const char* title;
  const char* description;
};

struct AvaAchievementState {
  bool unlocked[ACHIEVEMENT_COUNT] = { false };
  uint8_t uniqueDanceStyleMask = 0;
  uint32_t unlockedCount = 0;
  uint32_t happyMoodCount = 0;
  unsigned long lastUnlockMs = 0;
};

inline AvaAchievementState& avaAchievementState() {
  static AvaAchievementState state;
  return state;
}

inline const AvaAchievementInfo& avaAchievementInfo(AvaAchievementId id) {
  static const AvaAchievementInfo infos[ACHIEVEMENT_COUNT] = {
    { "First Command",      "Unlocked after AVA receives the first command." },
    { "First Dance",        "Unlocked after AVA performs the first dance." },
    { "10 Commands",        "Unlocked after AVA receives 10 commands." },
    { "10 Dances",          "Unlocked after AVA performs 10 dances." },
    { "Dance Variety",      "Unlocked after AVA uses 3 different dance styles." },
    { "Bandari Queen",      "Unlocked after AVA performs Bandari dance." },
    { "Happy Ava",          "Unlocked after AVA reaches happy mood enough times." },
    { "One Year Together",  "Unlocked on AVA's first year milestone." }
  };

  return infos[id];
}

inline const char* avaAchievementIdName(AvaAchievementId id) {
  switch (id) {
    case ACH_FIRST_COMMAND:      return "ACH_FIRST_COMMAND";
    case ACH_FIRST_DANCE:        return "ACH_FIRST_DANCE";
    case ACH_TEN_COMMANDS:       return "ACH_TEN_COMMANDS";
    case ACH_TEN_DANCES:         return "ACH_TEN_DANCES";
    case ACH_DANCE_VARIETY:      return "ACH_DANCE_VARIETY";
    case ACH_BANDARI_QUEEN:      return "ACH_BANDARI_QUEEN";
    case ACH_HAPPY_AVA:          return "ACH_HAPPY_AVA";
    case ACH_ONE_YEAR_TOGETHER:  return "ACH_ONE_YEAR_TOGETHER";
    default:                     return "ACH_UNKNOWN";
  }
}

inline uint8_t avaCountBits8(uint8_t value) {
  uint8_t count = 0;
  while (value) {
    count += (value & 0x01);
    value >>= 1;
  }
  return count;
}

inline bool avaAchievementIsUnlocked(AvaAchievementId id) {
  return avaAchievementState().unlocked[id];
}

inline uint32_t avaAchievementUnlockedCount() {
  return avaAchievementState().unlockedCount;
}

inline uint32_t avaAchievementUnlockedMask() {
  uint32_t mask = 0;
  for (uint8_t i = 0; i < ACHIEVEMENT_COUNT; ++i) {
    if (avaAchievementState().unlocked[i]) {
      mask |= (1UL << i);
    }
  }
  return mask;
}

inline bool avaAchievementUnlock(AvaAchievementId id, unsigned long nowMs) {
  AvaAchievementState& s = avaAchievementState();

  if (s.unlocked[id]) {
    return false;
  }

  s.unlocked[id] = true;
  s.unlockedCount++;
  s.lastUnlockMs = nowMs;

  Serial.println();
  Serial.println("🏆 Achievement Unlocked!");
  Serial.print("Title: ");
  Serial.println(avaAchievementInfo(id).title);
  Serial.print("ID: ");
  Serial.println(avaAchievementIdName(id));
  Serial.print("Description: ");
  Serial.println(avaAchievementInfo(id).description);
  Serial.println();

  return true;
}

inline void avaAchievementOnCommand(const String& cmd, unsigned long nowMs) {
  AvaMemoryState& mem = avaMemory();
  AvaAchievementState& s = avaAchievementState();

  // First command
  if (mem.commandCount == 1) {
    avaAchievementUnlock(ACH_FIRST_COMMAND, nowMs);
  }

  // 10 commands
  if (mem.commandCount >= 10) {
    avaAchievementUnlock(ACH_TEN_COMMANDS, nowMs);
  }

  // Optional creator moment
  if (cmd == "hi" || cmd == "hello" || cmd == "hey") {
    // No special unlock here yet, but keeping the hook for future expansion.
  }

  // Keep memory link alive for future app sync
  (void)s;
}

inline void avaAchievementOnDance(AvaDanceStyle style, unsigned long nowMs) {
    AvaMemoryState& mem = avaMemory();
  AvaAchievementState& s = avaAchievementState();

  // First dance
  if (mem.danceCount == 1) {
    avaAchievementUnlock(ACH_FIRST_DANCE, nowMs);
  }

  // 10 dances
  if (mem.danceCount >= 10) {
    avaAchievementUnlock(ACH_TEN_DANCES, nowMs);
  }

  // Dance variety
  uint8_t bit = 0;
  switch (style) {
    case DANCE_CUTE:    bit = 1 << 0; break;
    case DANCE_HAPPY:   bit = 1 << 1; break;
    case DANCE_BANDARI: bit = 1 << 2; break;
    case DANCE_FAST:    bit = 1 << 3; break;
    case DANCE_GENTLE:  bit = 1 << 4; break;
    default:            bit = 0;      break;
  }

  s.uniqueDanceStyleMask |= bit;

  if (avaCountBits8(s.uniqueDanceStyleMask) >= 3) {
    avaAchievementUnlock(ACH_DANCE_VARIETY, nowMs);
  }

  // Style-specific unlocks
  if (style == DANCE_BANDARI) {
    avaAchievementUnlock(ACH_BANDARI_QUEEN, nowMs);
  }

  if (style == DANCE_HAPPY) {
    avaAchievementUnlock(ACH_HAPPY_AVA, nowMs);
  }
}

inline void avaAchievementOnMood(Mood mood, unsigned long nowMs) {
  AvaAchievementState& s = avaAchievementState();

  if (mood == MOOD_HAPPY) {
    s.happyMoodCount++;
    if (s.happyMoodCount >= 5) {
      avaAchievementUnlock(ACH_HAPPY_AVA, nowMs);
    }
  }
}

inline void avaAchievementOnYearMilestone(unsigned long nowMs) {
  avaAchievementUnlock(ACH_ONE_YEAR_TOGETHER, nowMs);
}

inline void avaAchievementDebugPrint() {
  Serial.println();
  Serial.println("=========== AVA ACHIEVEMENTS ===========");

  for (uint8_t i = 0; i < ACHIEVEMENT_COUNT; ++i) {
    Serial.print(avaAchievementIsUnlocked((AvaAchievementId)i) ? "✅ " : "🔒 ");
    Serial.print(avaAchievementInfo((AvaAchievementId)i).title);
    Serial.print(" — ");
    Serial.println(avaAchievementInfo((AvaAchievementId)i).description);
  }

  Serial.print("Unlocked count: ");
  Serial.println(avaAchievementUnlockedCount());

  Serial.print("Unlocked mask: 0x");
  Serial.println(avaAchievementUnlockedMask(), HEX);

  Serial.println("========================================");
  Serial.println();
}

#endif