/*
  AVA Robot - Core Firmware v0.1
  Creator: Ali

  Notes:
  - فعلاً بدون سخت‌افزار واقعی
  - تست از طریق Serial Monitor
  - بعداً همین ساختار می‌ره روی ESP32-S3
*/

#include <Arduino.h>

#include "Ava_Config.h"
#include "Ava_Types.h"

#include "Ava_Eyes.h"
#include "Ava_Voice.h"
#include "Ava_Behavior.h"

#include "Ava_Personality.h"
#include "Ava_Movement.h"

#include "Ava_Dance.h"
#include "Ava_Memory.h"
#include "Ava_Achievements.h"

#include "Ava_Senses.h"
#include "Ava_Touch.h"

#include "Ava_Decision.h"
#include "Ava_OLED.h"
#include "EyeBlink.h"
#include "Ava_DisplaySettings.h"

#include "BlinkAssistant.h"
#include "Ava_Communication.h"
#include "Ava_WiFi.h"
#include "Ava_OTA.h"
#include "Ava_Bluetooth.h"

#include <esp_mac.h>

// ================================
// Function Prototypes
// ================================
void bootAva();
void handleSerialInput();
void processCommand(String cmd);
void updateBehavior();
void moveAva(MoveState move);
void setMood(Mood newMood);
void speak(const String& text);


void avaPettingTick(
    unsigned long now,
    unsigned long& lastInteractionTime,
    RobotState& currentState,
    Mood& currentMood
);
// ==================================================
// AVA NETWORK IDENTITY
// ==================================================

void avaSetNetworkIdentity()
{
  esp_err_t result =
      esp_base_mac_addr_set(
          AVA_BASE_MAC
      );

  if (result == ESP_OK) {

    Serial.println(
        "[NET] Fixed Base MAC applied."
    );

    Serial.printf(
        "[NET] Base MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
        AVA_BASE_MAC[0],
        AVA_BASE_MAC[1],
        AVA_BASE_MAC[2],
        AVA_BASE_MAC[3],
        AVA_BASE_MAC[4],
        AVA_BASE_MAC[5]
    );

  } else {

    Serial.print(
        "[NET] Failed to set Base MAC. Error: "
    );

    Serial.println(
        static_cast<int>(result)
    );
  }

  Serial.print("[NET] AVA ID: ");
  Serial.println(AVA_DEVICE_ID);
}
// ================================
// Global State
// ================================
RobotState currentState = STATE_BOOTING;
Mood currentMood = MOOD_CALM;

unsigned long lastInteractionTime = 0;
unsigned long bootStartTime = 0;

String incomingCommand = "";

// ================================
// Setup / Loop
// ================================

void setup() {

  Serial.begin(115200);

  delay(500);

  randomSeed(millis());

  bootStartTime = millis();

  // ==================================================
  // AVA NETWORK IDENTITY
  // Must be set before Wi-Fi and BLE initialization.
  // ==================================================

  avaSetNetworkIdentity();

  // ==================================================
  // OLED must be initialized before bootAva()
  // ==================================================

  avaOLEDBegin();
  avaDisplaySettingsBegin();
  bootAva();

  avaMemoryOnBoot(millis());

  avaTouchBegin();

  avaCommunicationBegin();

  // ==================================================
  // Network
  // ==================================================

  avaWiFiBegin();

  AvaBluetooth::begin();

  lastInteractionTime = millis();
}

void loop() {
  handleSerialInput();
  AvaBluetooth::update();
  avaCommunicationUpdate();
  avaWiFiUpdate();

  // OTA checks only after Wi-Fi is connected.
  // The updater prevents repeated checks during the same boot.
  if (avaWiFiConnected()) {
    avaOTAUpdate();
  }

  // Update raw touch state first so all higher-level logic sees
  // the current physical touch state.
  avaTouchUpdate();

  if (!avaDecision().targetAvailable) {
    avaNaturalGazeTick();
  }

  avaDecisionUpdate();

  updateBehavior();

  avaOLEDUpdate();
  avaBlinkAssistantUpdate();

  delay(20);
}
// ================================
// Boot
// ================================
void bootAva() {
  currentState = STATE_BOOTING;
  currentMood = MOOD_CALM;

  Serial.println();
  Serial.println("==================================");
  Serial.println("Booting Ava...");
  Serial.println("==================================");

  Serial.print("Hi ");
  Serial.print(OWNER_NAME);
  Serial.print(". I am ");
  Serial.print(AVA_NAME);
  Serial.println(".");

  Serial.println("Starting personality system...");
  Serial.println("Starting emotion system...");
  Serial.println("Starting command system...");
  Serial.println("Ready.");
  Serial.println();

  renderEyes(EYES_CALM);
  setGaze(GAZE_CENTER);

  currentState = STATE_IDLE;
  setMood(MOOD_CALM);
}

// ================================
// Mood / Eyes
// ================================
void setMood(Mood newMood) {
  currentMood = newMood;

  switch (currentMood) {
    case MOOD_CALM:
      renderEyes(EYES_CALM);
      break;

    case MOOD_HAPPY:
      renderEyes(EYES_HAPPY);
      break;

    case MOOD_SAD:
      renderEyes(EYES_SAD);
      break;

    case MOOD_SLEEPY:
      renderEyes(EYES_SLEEPY);
      break;

    case MOOD_THINKING:
      renderEyes(EYES_THINKING);
      break;

    case MOOD_LISTENING:
      renderEyes(EYES_LISTENING);
      break;

    default:
      renderEyes(EYES_CALM);
      break;
  }
}

// ================================
// Voice
// ================================
void speak(const String& text) {
  // فعلاً فقط روی Serial چاپ می‌کنیم
  Serial.print("Ava: ");
  Serial.println(text);
}

// ================================
// Commands
// ================================
void handleSerialInput() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();

    if (c == '\n' || c == '\r') {
      if (incomingCommand.length() > 0) {
        incomingCommand.trim();
        processCommand(incomingCommand);
        incomingCommand = "";
      }
    } else {
      incomingCommand += c;
    }
  }
}

void processCommand(String cmd) {
  cmd.trim();
  cmd.toLowerCase();

  lastInteractionTime = millis();
  currentState = STATE_LISTENING;
  setMood(MOOD_LISTENING);

  Serial.print("Command received: ");
  Serial.println(cmd);

  avaMemoryOnCommandReceived(cmd, millis());
  avaAchievementOnCommand(cmd, millis());
  // ================================
  // Direct Eye Commands
  // ================================
  if (cmd == "time" ||
    cmd == "what time is it") {

  Serial.println("[TIME] Requesting time from application...");

  avaCommunicationSend("TIME_REQUEST");

  currentState = STATE_IDLE;
  return;
}
  // ================================
  // Reference Eye Emotion Commands
  // ================================
  if (cmd == "emotion normal") {
    avaApplyEmotion(AVA_EMOTION_NORMAL);
    currentState = STATE_IDLE;
    return;
  }
  else if (cmd == "emotion angry") {
    avaApplyEmotion(AVA_EMOTION_ANGRY);
    currentState = STATE_IDLE;
    return;
  }
  else if (cmd == "emotion glee") {
    avaApplyEmotion(AVA_EMOTION_GLEE);
    currentState = STATE_IDLE;
    return;
  }
  else if (cmd == "emotion happy") {
    avaApplyEmotion(AVA_EMOTION_HAPPY);
    currentState = STATE_IDLE;
    return;
  }
  else if (cmd == "emotion sad_ref") {
    avaApplyEmotion(AVA_EMOTION_SAD_REFERENCE);
    currentState = STATE_IDLE;
    return;
  }
  else if (cmd == "emotion worried") {
    avaApplyEmotion(AVA_EMOTION_WORRIED);
    currentState = STATE_IDLE;
    return;
}
  else if (cmd == "emotion focused") {
    avaApplyEmotion(AVA_EMOTION_FOCUSED);
    currentState = STATE_IDLE;
    return;
}
  else if (cmd == "emotion annoyed") {
    avaApplyEmotion(AVA_EMOTION_ANNOYED);
    currentState = STATE_IDLE;
    return;
}
  else if (cmd == "emotion surprised") {
    avaApplyEmotion(AVA_EMOTION_SURPRISED);
    currentState = STATE_IDLE;
    return;
}
  else if (cmd == "emotion skeptic") {
    avaApplyEmotion(AVA_EMOTION_SKEPTIC);
    currentState = STATE_IDLE;
    return;
}
  else if (cmd == "emotion frustrated") {
    avaApplyEmotion(AVA_EMOTION_FRUSTRATED);
    currentState = STATE_IDLE;
    return;
}
  else if (cmd == "emotion unimpressed") {
    avaApplyEmotion(AVA_EMOTION_UNIMPRESSED);
    currentState = STATE_IDLE;
    return;
}
  else if (cmd == "emotion sleepy") {
    avaApplyEmotion(AVA_EMOTION_SLEEPY);
    currentState = STATE_IDLE;
    return;
}
  else if (cmd == "emotion suspicious") {
    avaApplyEmotion(AVA_EMOTION_SUSPICIOUS);
    currentState = STATE_IDLE;
    return;
}
  else if (cmd == "emotion squint") {
    avaApplyEmotion(AVA_EMOTION_SQUINT);
    currentState = STATE_IDLE;
    return;
}
  else if (cmd == "emotion furious") {
    avaApplyEmotion(AVA_EMOTION_FURIOUS);
    currentState = STATE_IDLE;
    return;
}
  else if (cmd == "emotion scared") {
    avaApplyEmotion(AVA_EMOTION_SCARED);
    currentState = STATE_IDLE;
    return;
}
  else if (cmd == "emotion awe") {
    avaApplyEmotion(AVA_EMOTION_AWE);
    currentState = STATE_IDLE;
    return;
}

  if (cmd == "happy") {
    setMood(MOOD_HAPPY);
    currentState = STATE_IDLE;
    return;
  }
  else if (cmd == "sad") {
    setMood(MOOD_SAD);
    currentState = STATE_IDLE;
    return;
  }
  else if (cmd == "sleepy") {
    setMood(MOOD_SLEEPY);
    currentState = STATE_IDLE;
    return;
  }
  else if (cmd == "thinking") {
    setMood(MOOD_THINKING);
    currentState = STATE_IDLE;
    return;
  }
  else if (cmd == "listening") {
    setMood(MOOD_LISTENING);
    currentState = STATE_IDLE;
    return;
  }
  else if (cmd == "calm") {
    setMood(MOOD_CALM);
    currentState = STATE_IDLE;
    return;
  }

  if (cmd == "debug memory") {
    avaMemoryDebugPrint();
    currentState = STATE_IDLE;
    setMood(MOOD_THINKING);
    return;
  }

  if (cmd == "debug help") {
    Serial.println();
    Serial.println("=========== AVA DEBUG CONSOLE ===========");
    Serial.println("debug help         - Show debug commands");
    Serial.println("debug memory       - Show memory state");
    Serial.println("debug achievements - Show achievement state");
    Serial.println("debug mood         - Show current mood");
    Serial.println("debug state        - Show current robot state");
    Serial.println("debug all          - Show all debug information");
    Serial.println("debug gaze         - Show current gaze");
    Serial.println("debug decision     - Show decision engine state");
    Serial.println("debug oled         - Show OLED state");
    Serial.println("==========================================");
    Serial.println();

    currentState = STATE_IDLE;
    setMood(MOOD_THINKING);
    return;
  }

  if (cmd == "debug mood") {
    Serial.println();
    Serial.println("=========== AVA MOOD ===========");
    Serial.print("Current mood ID: ");
    Serial.println((int)currentMood);
    Serial.println("================================");
    Serial.println();

    currentState = STATE_IDLE;
    setMood(MOOD_THINKING);
    return;
  }

  if (cmd == "debug state") {
    Serial.println();
    Serial.println("=========== AVA STATE ===========");
    Serial.print("Current state ID: ");
    Serial.println((int)currentState);
    Serial.println("=================================");
    Serial.println();

    currentState = STATE_IDLE;
    setMood(MOOD_THINKING);
    return;
  }

  if (cmd == "debug all") {
    avaMemoryDebugPrint();
    avaAchievementDebugPrint();

    Serial.println();
    Serial.println("=========== AVA CURRENT ===========");
    Serial.print("Mood ID: ");
    Serial.println((int)currentMood);
    Serial.print("State ID: ");
    Serial.println((int)currentState);
    Serial.println("===================================");
    Serial.println();

    currentState = STATE_IDLE;
    setMood(MOOD_THINKING);
    return;
  }

  if (cmd == "debug achievements") {
    avaAchievementDebugPrint();
    currentState = STATE_IDLE;
    setMood(MOOD_THINKING);
    return;
  }

  if (cmd == "debug gaze") {
    Serial.println();
    Serial.println("=========== AVA GAZE ===========");
    Serial.print("Current gaze: ");
    Serial.println(getGazeName(getCurrentGaze()));
    Serial.println("================================");
    Serial.println();

    currentState = STATE_IDLE;
    setMood(MOOD_THINKING);
    return;
  }

  if (cmd == "debug decision") {
    avaDecisionDebugPrint();

    currentState = STATE_IDLE;
    setMood(MOOD_THINKING);
    return;
  }

  if (cmd == "sense ground 50") {
    avaSetGroundDistance(50);
    currentState = STATE_IDLE;
    return;
  }
  else if (cmd == "sense ground 20") {
    avaSetGroundDistance(20);
    currentState = STATE_IDLE;
    return;
  }
  else if (cmd == "sense front 10") {
    avaSetFrontDistance(10);
    currentState = STATE_IDLE;
    return;
  }
  else if (cmd == "sense light 5") {
    avaSetLightLevel(5);
    currentState = STATE_IDLE;
    return;
  }
  else if (cmd == "sense sound 90") {
    avaSetSoundLevel(90);
    currentState = STATE_IDLE;
    return;
  }
  else if (cmd == "sense target 20 50") {
    setGazeTarget(20, 50);
    currentState = STATE_IDLE;
    return;
  }

  if (cmd == "debug eyes") {
    Serial.println();
    Serial.println("=========== AVA EYES ===========");
    Serial.print("Current eye frame: ");
    Serial.println(getEyesFrame(avaCurrentEyeState));
    Serial.print("Current gaze: ");
    Serial.println(getGazeName(getCurrentGaze()));
    Serial.println("================================");
    Serial.println();

    currentState = STATE_IDLE;
    setMood(MOOD_THINKING);
    return;
  }

  if (cmd == "debug dance") {
    Serial.println();
    Serial.println("=========== AVA DANCE ===========");

    Serial.println("Available dance styles:");
    Serial.println("0 - CUTE");
    Serial.println("1 - HAPPY");
    Serial.println("2 - BANDARI");
    Serial.println("3 - FAST");
    Serial.println("4 - GENTLE");

    Serial.print("Dance count: ");
    Serial.println(avaMemory().danceCount);

    if (avaMemory().hasLastDanceStyle) {
      Serial.print("Last dance: ");
      Serial.println(
        avaDanceStyleName(
          avaMemory().lastDanceStyle
        )
      );
    }
    else {
      Serial.println("Last dance: N/A");
    }

    Serial.println("=================================");
    Serial.println();

    currentState = STATE_IDLE;
    setMood(MOOD_THINKING);
    return;
  }

  if (cmd == "debug oled") {
    avaOLEDDebugPrint();

    currentState = STATE_IDLE;
    setMood(MOOD_THINKING);
    return;
  }

  if (cmd == "gaze auto") {
    avaEnableNaturalGaze();
    currentState = STATE_IDLE;
    return;
  }

  if (cmd == "gaze center") {
    setGaze(GAZE_CENTER);
    currentState = STATE_IDLE;
    return;
  }
  else if (cmd == "gaze left") {
    setGaze(GAZE_LEFT);
    currentState = STATE_IDLE;
    return;
  }
  else if (cmd == "gaze right") {
    setGaze(GAZE_RIGHT);
    currentState = STATE_IDLE;
    return;
  }
  else if (cmd == "gaze up") {
    setGaze(GAZE_UP);
    currentState = STATE_IDLE;
    return;
  }
  else if (cmd == "gaze down") {
    setGaze(GAZE_DOWN);
    currentState = STATE_IDLE;
    return;
  }
  else if (cmd == "gaze random") {
    setGaze((AvaGazeDirection)random(0, 9));
    currentState = STATE_IDLE;
    return;
  }

  else if (
    cmd == "weather" ||
    cmd == "how's the weather?" ||
    cmd == "how's the weather" ||
    cmd == "hows the weather?" ||
    cmd == "hows the weather"
  )
  {
    Serial.println();
    Serial.println(
      "[WEATHER] Weather requested by owner."
    );

    Serial.println(
      "[WEATHER] Requesting Tehran weather..."
    );

    bool success =
      avaWiFiGetTehranWeather();

    if (success)
    {
      Serial.println(
        "[WEATHER] Weather request completed successfully."
      );
    }
    else
    {
      Serial.println(
        "[WEATHER] Weather request FAILED."
      );
    }

    Serial.println();

    setMood(MOOD_THINKING);

    return;
  }

  else if (cmd == "dance") {
    speak("Okay. Let's dance.");
    setMood(MOOD_HAPPY);
    avaDanceAutoPreview(
      128,
      90,
      currentMood,
      8
    );
    avaMemoryOnDancePerformed(
      DANCE_HAPPY,
      millis()
    );
    avaAchievementOnDance(
      DANCE_HAPPY,
      millis()
    );
  }
  else if (cmd == "dance cute") {
    speak("Cute dance mode.");
    setMood(MOOD_HAPPY);
    avaDancePreview(
      DANCE_CUTE,
      8,
      90
    );
    avaMemoryOnDancePerformed(
      DANCE_CUTE,
      millis()
    );
    avaAchievementOnDance(
      DANCE_CUTE,
      millis()
    );
  }
  else if (cmd == "dance happy") {
    speak("Happy dance mode.");
    setMood(MOOD_HAPPY);
    avaDancePreview(
      DANCE_HAPPY,
      8,
      95
    );
    avaMemoryOnDancePerformed(
      DANCE_HAPPY,
      millis()
    );
    avaAchievementOnDance(
      DANCE_HAPPY,
      millis()
    );
  }
  else if (cmd == "dance bandari") {
    speak("Bandari mode.");
    setMood(MOOD_HAPPY);
    avaDancePreview(
      DANCE_BANDARI,
      8,
      100
    );
    avaMemoryOnDancePerformed(
      DANCE_BANDARI,
      millis()
    );
    avaAchievementOnDance(
      DANCE_BANDARI,
      millis()
    );
  }
  else if (cmd == "dance fast") {
    speak("Fast dance mode.");
    setMood(MOOD_HAPPY);
    avaDancePreview(
      DANCE_FAST,
      8,
      100
    );
    avaMemoryOnDancePerformed(
      DANCE_FAST,
      millis()
    );
    avaAchievementOnDance(
      DANCE_FAST,
      millis()
    );
  }
  else if (cmd == "dance gentle") {
    speak("Gentle dance mode.");
    setMood(MOOD_CALM);
    avaDancePreview(
      DANCE_GENTLE,
      8,
      40
    );
    avaMemoryOnDancePerformed(
      DANCE_GENTLE,
      millis()
    );
    avaAchievementOnDance(
      DANCE_GENTLE,
      millis()
    );
  }
  else if (cmd == "nod") {
    speak("Yes.");
    moveAva(MOVE_NOD);
  }
  else if (cmd == "forward") {
    speak("Moving forward.");
    moveAva(MOVE_FORWARD);
  }
  else if (cmd == "stop") {
    speak("Stopping.");
    moveAva(MOVE_STOP);
  }
  else {
    playVoice(VOICE_UNKNOWN);
    setMood(MOOD_SAD);
  }

  if (currentState != STATE_SLEEPING) {
    currentState = STATE_IDLE;
  }
}
// ================================
// Movement
// ================================
void moveAva(MoveState move) {
  Serial.print("Movement: ");
  Serial.println(getMoveName(move));
}

// ================================
// Behavior
// ================================
void updateBehavior() {
  avaAutoBehaviorTick(millis(), lastInteractionTime, currentState, currentMood);
}
