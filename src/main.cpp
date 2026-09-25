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
#include "Ava_OTA_Display.h"
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
  // OTA owns the network while its dedicated task is running.
  // Keep Weather, Bluetooth-triggered network commands, and other
  // loopTask network users from competing with OTA/TLS.
  if (!avaOTAIsRunning()) {
    handleSerialInput();
    AvaBluetooth::update();
    avaCommunicationUpdate();
    avaWiFiUpdate();
  }

  // OTA runs in a dedicated FreeRTOS task so HTTPS/TLS does not
  // consume the Arduino loopTask stack.
  if (avaWiFiConnected()) {
    avaOTAStartTask();
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
  avaOTAUIUpdate();
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