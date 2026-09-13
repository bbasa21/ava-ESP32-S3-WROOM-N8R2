#ifndef AVA_BLUETOOTH_H
#define AVA_BLUETOOTH_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLEAdvertising.h>

#include <esp_mac.h>
#include <esp_bt_device.h>

#include "Ava_Eyes.h"
#include "Ava_Config.h"
#include "AVA_GAMES.h"

namespace AvaBluetooth {

// ==================================================
// AVA BLE UUIDs
// ==================================================

static constexpr const char* SERVICE_UUID =
    "7b7a0001-6a76-4156-9a76-415641000001";

static constexpr const char* COMMAND_UUID =
    "7b7a0002-6a76-4156-9a76-415641000001";

static constexpr const char* EVENT_UUID =
    "7b7a0003-6a76-4156-9a76-415641000001";

static constexpr const char* STATE_UUID =
    "7b7a0004-6a76-4156-9a76-415641000001";

static constexpr const char* DATA_UUID =
    "7b7a0005-6a76-4156-9a76-415641000001";


// ==================================================
// Internal BLE objects
// ==================================================

inline BLEServer*& server()
{
    static BLEServer* value = nullptr;
    return value;
}

inline BLEService*& service()
{
    static BLEService* value = nullptr;
    return value;
}

inline BLECharacteristic*& commandCharacteristic()
{
    static BLECharacteristic* value = nullptr;
    return value;
}

inline BLECharacteristic*& eventCharacteristic()
{
    static BLECharacteristic* value = nullptr;
    return value;
}

inline BLECharacteristic*& stateCharacteristic()
{
    static BLECharacteristic* value = nullptr;
    return value;
}

inline BLECharacteristic*& dataCharacteristic()
{
    static BLECharacteristic* value = nullptr;
    return value;
}


// ==================================================
// DATA RX BUFFER
//
// IMPORTANT:
//
// BLE callback فقط دیتا را داخل این بافر می‌گذارد.
// پردازش واقعی در update() انجام می‌شود.
//
// این باعث می‌شود منطق بازی داخل BTC_TASK اجرا نشود.
// ==================================================

inline String& receivedData()
{
    static String value = "";
    return value;
}

inline bool& dataPending()
{
    static bool value = false;
    return value;
}


// ==================================================
// Connection state
// ==================================================

inline bool& connected()
{
    static bool value = false;
    return value;
}

inline bool& connectionEventPending()
{
    static bool value = false;
    return value;
}


// ==================================================
// Forward declarations
// ==================================================

inline void sendEvent(const String& message);
inline void sendState(const String& state);
inline void sendData(const String& data);

inline void executeCommand(const String& command);
inline void processData(const String& data);
inline void processGameAnswer(const String& data);


// ==================================================
// DATA RX API
// ==================================================

inline bool hasData()
{
    return dataPending();
}

inline String readData()
{
    if (!dataPending())
    {
        return "";
    }

    String data =
        receivedData();

    receivedData() = "";
    dataPending() = false;

    return data;
}


// ==================================================
// GAME HELPERS
// ==================================================


// ==================================================
// Send current game question
//
// Format:
//
// GAME_QUESTION|ROUND|A|B|OP|OPT1|OPT2|OPT3|OPT4
// ==================================================

inline void sendGameQuestion()
{
    String message =
        "GAME_QUESTION|";

    // --------------------------------------------------
    // Round
    // --------------------------------------------------

    message +=
        String(AvaGames::getCurrentRound());

    // ==================================================
    // LOGIC BATTLE
    // ==================================================

    if (AvaGames::getCurrentGame() ==
        AvaGames::GAME_LOGIC_BATTLE)
    {
        message += "|";

        message +=
            AvaGames::getLogicQuestionText();

        // --------------------------------------------------
        // 4 options
        // --------------------------------------------------

        for (int i = 0; i < 4; i++)
        {
            message += "|";

            message +=
                String(
                    AvaGames::getOption(i)
                );
        }

        sendData(message);

        Serial.println(
            "[GAME BLE] Logic question sent."
        );

        return;
    }

    // ==================================================
    // MATH BATTLE
    //
    // Existing format remains unchanged.
    // ==================================================

    message += "|";

    message +=
        String(
            AvaGames::getQuestionA()
        );

    message += "|";

    message +=
        String(
            AvaGames::getQuestionB()
        );

    message += "|";

    message +=
        String(
            AvaGames::getQuestionOperator()
        );

    // --------------------------------------------------
    // 4 options
    // --------------------------------------------------

    for (int i = 0; i < 4; i++)
    {
        message += "|";

        message +=
            String(
                AvaGames::getOption(i)
            );
    }

    sendData(message);

    Serial.println(
        "[GAME BLE] Math question sent."
    );
}
// ==================================================
// Send game score
//
// Format:
//
// GAME_SCORE|ALI_SCORE|AVA_SCORE
//
// IMPORTANT:
//
// امتیاز همیشه از Game Engine خوانده می‌شود.
//
// Android نباید خودش امتیاز را محاسبه کند.
// ==================================================

inline void sendGameScore()
{
    String message =
        "GAME_SCORE|";

    message +=
        String(AvaGames::getAliScore());

    message += "|";

    message +=
        String(AvaGames::getAvaScore());

    sendData(message);
}


// ==================================================
// Send round result
//
// Format:
//
// GAME_RESULT|ROUND|ALI_ANSWER|AVA_ANSWER|ALI_CORRECT|AVA_CORRECT
//
// IMPORTANT:
//
// این پیام وضعیت جواب‌های راند را ارسال می‌کند.
// امتیاز واقعی از GAME_SCORE دریافت می‌شود.
// ==================================================

inline void sendGameResult()
{
    String message =
        "GAME_RESULT|";

    message +=
        String(AvaGames::getCurrentRound());

    message += "|";

    message +=
        String(AvaGames::getAliAnswer());

    message += "|";

    message +=
        String(AvaGames::getAvaAnswer());

    message += "|";

    message +=
        AvaGames::isAliCorrect()
        ? "1"
        : "0";

    message += "|";

    message +=
        AvaGames::isAvaCorrect()
        ? "1"
        : "0";

    sendData(message);
}


// ==================================================
// Send AVA answer
//
// Format:
//
// GAME_ANSWER|AVA|VALUE
// ==================================================

inline void sendAvaGameAnswer()
{
    String message =
        "GAME_ANSWER|AVA|";

    message +=
        String(AvaGames::getAvaAnswer());

    sendData(message);
}


// ==================================================
// Send final game result
//
// Format:
//
// GAME_FINISHED|WINNER|ALI_SCORE|AVA_SCORE
// ==================================================

inline void sendGameFinished()
{
    String winner =
        AvaGames::getWinner();

    String message =
        "GAME_FINISHED|";

    message += winner;

    message += "|";

    message +=
        String(AvaGames::getAliScore());

    message += "|";

    message +=
        String(AvaGames::getAvaScore());

    sendData(message);
}


// ==================================================
// Process GAME_ANSWER
//
// Flow:
//
// ALI ANSWER
//      ↓
// AVA THINKS
//      ↓
// AVA ANSWER
//      ↓
// APPLY SCORE
//      ↓
// AVA ANSWER MESSAGE
//      ↓
// ROUND RESULT
//      ↓
// SCORE
//      ↓
// NEXT ROUND
//
// ==================================================

inline void processGameAnswer(
    const String& data
)
{
    const String prefix =
        "GAME_ANSWER|";

    if (!data.startsWith(prefix))
    {
        return;
    }

    String payload =
        data.substring(prefix.length());

    int separator =
        payload.indexOf('|');

    if (separator < 0)
    {
        Serial.println(
            "[GAME BLE] Invalid GAME_ANSWER."
        );

        sendData(
            "GAME_ANSWER_FAILED"
        );

        return;
    }

    String player =
        payload.substring(
            0,
            separator
        );

    String answerText =
        payload.substring(
            separator + 1
        );

    player.trim();
    answerText.trim();

    player.toUpperCase();

    if (answerText.length() == 0)
    {
        Serial.println(
            "[GAME BLE] Empty answer."
        );

        sendData(
            "GAME_ANSWER_FAILED"
        );

        return;
    }

    int answer =
        answerText.toInt();


    // ==================================================
    // ALI ANSWER
    // ==================================================

    if (player == "ALI")
    {
        // --------------------------------------------------
        // Game must be running.
        // --------------------------------------------------

        if (!AvaGames::isRunning())
        {
            Serial.println(
                "[GAME BLE] ALI answer rejected: "
                "game is not running."
            );

            sendData(
                "GAME_ANSWER_FAILED|GAME_NOT_RUNNING"
            );

            return;
        }


        // --------------------------------------------------
        // ALI can answer only once per round.
        // --------------------------------------------------

        if (AvaGames::hasAliAnswered())
        {
            Serial.println(
                "[GAME BLE] ALI already answered."
            );

            sendData(
                "GAME_ANSWER_FAILED|ALREADY_ANSWERED"
            );

            return;
        }


        // --------------------------------------------------
        // Register ALI answer.
        // --------------------------------------------------

        bool correct =
            AvaGames::submitAnswer(
                AvaGames::PLAYER_ALI,
                answer
            );


        // --------------------------------------------------
        // Tell Android that ALI answer was accepted.
        // --------------------------------------------------

        String response =
            "GAME_ANSWER_ACCEPTED|ALI|";

        response +=
            correct ? "1" : "0";

        sendData(response);


        Serial.println(
            "[GAME BLE] ============================="
        );

        Serial.print(
            "[GAME BLE] ALI ANSWER: "
        );

        Serial.println(answer);

        Serial.print(
            "[GAME BLE] ALI CORRECT: "
        );

        Serial.println(
            correct ? "YES" : "NO"
        );


        // ==================================================
        // AVA NOW ANSWERS
        // ==================================================

        Serial.println(
            "[GAME BLE] ALI answered."
        );

        Serial.println(
            "[GAME BLE] AVA is calculating..."
        );


        // --------------------------------------------------
        // Generate AVA answer.
        //
        // EASY   ≈ 10% wrong
        // MEDIUM ≈ 20% wrong
        // HARD   ≈ 35% wrong
        // --------------------------------------------------

        int generatedAvaAnswer =
            AvaGames::generateAvaAnswer();


        Serial.print(
            "[GAME BLE] AVA GENERATED ANSWER: "
        );

        Serial.println(
            generatedAvaAnswer
        );


        // --------------------------------------------------
        // Register AVA answer.
        // --------------------------------------------------

        bool avaCorrect =
            AvaGames::submitAnswer(
                AvaGames::PLAYER_AVA,
                generatedAvaAnswer
            );


        Serial.print(
            "[GAME BLE] AVA CORRECT: "
        );

        Serial.println(
            avaCorrect ? "YES" : "NO"
        );

        Serial.println(
            "[GAME BLE] Both answers should now be ready."
        );
    }


    // ==================================================
    // AVA ANSWER
    //
    // Phone must NOT control AVA.
    // ==================================================

    else if (player == "AVA")
    {
        Serial.println(
            "[GAME BLE] External AVA answer rejected."
        );

        sendData(
            "GAME_ANSWER_FAILED|AVA_INTERNAL_ONLY"
        );

        return;
    }


    // ==================================================
    // UNKNOWN PLAYER
    // ==================================================

    else
    {
        Serial.print(
            "[GAME BLE] Unknown player: "
        );

        Serial.println(player);

        sendData(
            "GAME_ANSWER_FAILED|UNKNOWN_PLAYER"
        );

        return;
    }


    // ==================================================
    // WAIT UNTIL BOTH PLAYERS HAVE ANSWERED
    // ==================================================

    if (!AvaGames::bothPlayersAnswered())
    {
        Serial.println(
            "[GAME BLE] ERROR: Both answers are not ready."
        );

        return;
    }


    // ==================================================
    // BOTH ANSWERS READY
    // ==================================================

    Serial.println(
        "[GAME BLE] BOTH PLAYERS ANSWERED."
    );

    Serial.println(
        "[GAME BLE] Processing round result..."
    );


    // ==================================================
    // STEP 1
    // Apply score FIRST
    //
    // قانون جدید:
    //
    // ALI correct -> ALI +1
    // AVA correct -> AVA +1
    //
    // هر دو می‌توانند همزمان امتیاز بگیرند.
    // ==================================================

    Serial.println(
        "[GAME BLE] STEP 1: Applying round score..."
    );

    AvaGames::applyRoundScore();


    Serial.print(
        "[GAME BLE] SCORE NOW: ALI="
    );

    Serial.print(
        AvaGames::getAliScore()
    );

    Serial.print(
        " AVA="
    );

    Serial.println(
        AvaGames::getAvaScore()
    );


    // ==================================================
    // STEP 2
    // Send AVA answer
    // ==================================================

    Serial.println(
        "[GAME BLE] STEP 2: Sending AVA answer..."
    );

    sendAvaGameAnswer();


    // ==================================================
    // STEP 3
    // Send complete round result
    //
    // IMPORTANT:
    //
    // Score has already been applied.
    // ==================================================

    Serial.println(
        "[GAME BLE] STEP 3: Sending round result..."
    );

    sendGameResult();


    // ==================================================
    // STEP 4
    // Send updated score
    //
    // Android should ONLY DISPLAY this score.
    //
    // Android must NOT calculate or increment score
    // by itself.
    // ==================================================

    Serial.println(
        "[GAME BLE] STEP 4: Sending updated score..."
    );

    sendGameScore();


    // ==================================================
    // STEP 5
    // Advance game
    //
    // nextRound():
    //
    // - score is already applied
    // - score guard prevents double scoring
    // - checks final round
    // - generates next question
    // - resets answer state
    // ==================================================

    Serial.println(
        "[GAME BLE] STEP 5: Advancing round..."
    );

    bool hasNextRound =
        AvaGames::nextRound();


    // ==================================================
    // STEP 6
    // Next question OR finish
    // ==================================================

    if (hasNextRound)
    {
        Serial.print(
            "[GAME BLE] NEXT ROUND: "
        );

        Serial.println(
            AvaGames::getCurrentRound()
        );

        Serial.println(
            "[GAME BLE] Waiting for ALI..."
        );

        Serial.println(
            "[GAME BLE] STEP 6: Sending next question..."
        );

        sendGameQuestion();
    }
    else
    {
        Serial.println(
            "[GAME BLE] GAME FINISHED."
        );

        Serial.println(
            "[GAME BLE] STEP 6: Sending finished..."
        );

        sendGameFinished();
    }


    Serial.println(
        "[GAME BLE] ============================="
    );
}


// ==================================================
// Process incoming DATA
//
// IMPORTANT:
//
// GAME_ANSWER is processed from update(),
// NOT directly from BLE callback.
//
// ==================================================

inline void processData(
    const String& data
)
{
    String value =
        data;

    value.trim();

    if (value.length() == 0)
    {
        return;
    }


    // --------------------------------------------------
    // GAME ANSWER
    // --------------------------------------------------

    if (value.startsWith("GAME_ANSWER|"))
    {
        processGameAnswer(value);
        return;
    }


    // --------------------------------------------------
    // Other DATA
    // --------------------------------------------------

    receivedData() =
        value;

    dataPending() =
        true;
}


// ==================================================
// Execute BLE command
// ==================================================

inline void executeCommand(
    const String& command
)
{
    String cmd =
        command;

    cmd.trim();
    cmd.toUpperCase();

    Serial.print(
        "[BLE] Executing: "
    );

    Serial.println(cmd);


    // ==================================================
    // HELLO
    // ==================================================

    if (cmd == "HELLO_AVA")
    {
        Serial.println(
            "[BLE] Hello received."
        );

        renderEyes(
            EYES_LISTENING
        );

        sendEvent(
            "HELLO_AVA_ACK"
        );

        return;
    }


    // ==================================================
    // CALM
    // ==================================================

    if (cmd == "EYES_CALM")
    {
        renderEyes(
            EYES_CALM
        );

        sendEvent(
            "EYES_CALM"
        );

        return;
    }


    // ==================================================
    // HAPPY
    // ==================================================

    if (cmd == "EYES_HAPPY")
    {
        renderEyes(
            EYES_HAPPY
        );

        sendEvent(
            "EYES_HAPPY"
        );

        return;
    }


    // ==================================================
    // SAD
    // ==================================================

    if (cmd == "EYES_SAD")
    {
        renderEyes(
            EYES_SAD
        );

        sendEvent(
            "EYES_SAD"
        );

        return;
    }


    // ==================================================
    // SLEEPY
    // ==================================================

    if (cmd == "EYES_SLEEPY")
    {
        renderEyes(
            EYES_SLEEPY
        );

        sendEvent(
            "EYES_SLEEPY"
        );

        return;
    }


    // ==================================================
    // THINKING
    // ==================================================

    if (cmd == "EYES_THINKING")
    {
        renderEyes(
            EYES_THINKING
        );

        sendEvent(
            "EYES_THINKING"
        );

        return;
    }


    // ==================================================
    // LISTENING
    // ==================================================

    if (cmd == "EYES_LISTENING")
    {
        renderEyes(
            EYES_LISTENING
        );

        sendEvent(
            "EYES_LISTENING"
        );

        return;
    }


    // ==================================================
    // SURPRISED
    // ==================================================

    if (cmd == "EYES_SURPRISED")
    {
        renderEyes(
            EYES_SURPRISED
        );

        sendEvent(
            "EYES_SURPRISED"
        );

        return;
    }


    // ==================================================
    // BLINK
    // ==================================================

    if (cmd == "BLINK")
    {
        EyeState previousState =
            avaCurrentEyeState;

        renderEyes(
            EYES_BLINK
        );

        delay(120);

        renderEyes(
            previousState
        );

        sendEvent(
            "BLINK"
        );

        return;
    }


    // ==================================================
    // TIME REQUEST
    // ==================================================

    if (cmd == "TIME_REQUEST")
    {
        Serial.println(
            "[BLE] Time requested."
        );

        sendEvent(
            "TIME_REQUEST"
        );

        return;
    }


    // ==================================================
    // GAME LOAD
    // ==================================================

    if (cmd.startsWith("GAME_LOAD|"))
    {
        String gameId =
            cmd.substring(
                String("GAME_LOAD|").length()
            );

        gameId.trim();

        if (AvaGames::loadGame(gameId))
        {
            sendEvent(
                "GAME_LOADED|" + gameId
            );
        }
        else
        {
            sendEvent(
                "GAME_LOAD_FAILED|" + gameId
            );
        }

        return;
    }


    // ==================================================
    // GAME START
    // ==================================================

    if (cmd == "GAME_START")
    {
        if (AvaGames::startGame())
        {
            sendEvent(
                "GAME_STARTED"
            );

            // --------------------------------------------------
            // First shared question.
            //
            // AVA DOES NOT ANSWER HERE.
            // --------------------------------------------------

            sendGameQuestion();

            Serial.println(
                "[GAME BLE] GAME STARTED."
            );

            Serial.println(
                "[GAME BLE] Waiting for ALI answer."
            );
        }
        else
        {
            sendEvent(
                "GAME_START_FAILED"
            );
        }

        return;
    }


    // ==================================================
    // GAME END
    // ==================================================

    if (cmd == "GAME_END")
    {
        AvaGames::endGame();

        sendEvent(
            "GAME_ENDED"
        );

        sendGameFinished();

        return;
    }


    // ==================================================
    // GAME RESET
    // ==================================================

    if (cmd == "GAME_RESET")
    {
        AvaGames::reset();

        sendEvent(
            "GAME_RESET"
        );

        return;
    }


    // ==================================================
    // Unknown command
    // ==================================================

    Serial.print(
        "[BLE] Unknown command: "
    );

    Serial.println(cmd);

    sendEvent(
        "UNKNOWN_COMMAND"
    );
}


// ==================================================
// BLE SERVER CALLBACKS
// ==================================================

class ServerCallbacks :
    public BLEServerCallbacks
{
public:

    void onConnect(
        BLEServer* pServer
    ) override
    {
        connected() =
            true;

        connectionEventPending() =
            true;

        Serial.println(
            "[BLE] Phone connected."
        );
    }


    void onDisconnect(
        BLEServer* pServer
    ) override
    {
        connected() =
            false;

        connectionEventPending() =
            false;

        Serial.println(
            "[BLE] Phone disconnected."
        );

        BLEDevice::startAdvertising();

        Serial.println(
            "[BLE] Advertising restarted."
        );
    }
};


// ==================================================
// COMMAND CALLBACK
// ==================================================

class CommandCallbacks :
    public BLECharacteristicCallbacks
{
public:

    void onWrite(
        BLECharacteristic* characteristic
    ) override
    {
        if (characteristic == nullptr)
        {
            Serial.println(
                "[BLE] ERROR: Null command characteristic."
            );

            return;
        }

        std::string value =
            characteristic->getValue();

        if (value.empty())
        {
            return;
        }

        String command =
            String(value.c_str());

        command.trim();

        Serial.print(
            "[BLE RX] COMMAND: "
        );

        Serial.println(command);

        executeCommand(
            command
        );
    }
};


// ==================================================
// DATA CALLBACK
//
// IMPORTANT:
//
// این callback دیگر processData() را اجرا نمی‌کند.
//
// فقط دیتا را در receivedData ذخیره می‌کند و سریع
// از BTC_TASK خارج می‌شود.
//
// پردازش واقعی در update() انجام می‌شود.
// ==================================================

class DataCallbacks :
    public BLECharacteristicCallbacks
{
public:

    void onWrite(
        BLECharacteristic* characteristic
    ) override
    {
        if (characteristic == nullptr)
        {
            Serial.println(
                "[BLE] ERROR: Null data characteristic."
            );

            return;
        }

        std::string value =
            characteristic->getValue();

        if (value.empty())
        {
            return;
        }

        String data =
            String(value.c_str());

        data.trim();

        if (data.length() == 0)
        {
            return;
        }

        Serial.print(
            "[BLE RX] DATA: "
        );

        Serial.println(data);


        // ==================================================
        // IMPORTANT:
        //
        // Do NOT process GAME_ANSWER here.
        //
        // Just queue it for update().
        // ==================================================

        if (!dataPending())
        {
            receivedData() =
                data;

            dataPending() =
                true;

            Serial.println(
                "[BLE] DATA queued for main loop."
            );
        }
        else
        {
            Serial.println(
                "[BLE] WARNING: DATA buffer busy."
            );

            // --------------------------------------------------
            // Do not overwrite an active packet.
            // --------------------------------------------------

            return;
        }
    }
};


// ==================================================
// BLE BEGIN
// ==================================================

inline bool begin()
{
    Serial.println();

    Serial.println(
        "=========== AVA BLUETOOTH ==========="
    );


    // --------------------------------------------------
    // Initialize BLE
    // --------------------------------------------------

    BLEDevice::init(
        "AVA"
    );

    const uint8_t* bleMac =
        esp_bt_dev_get_address();

    if (bleMac != nullptr)
    {
        Serial.printf(
            "[BLE] MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
            bleMac[0],
            bleMac[1],
            bleMac[2],
            bleMac[3],
            bleMac[4],
            bleMac[5]
        );
    }

    Serial.print(
        "[BLE] Device ID: "
    );

    Serial.println(
        AVA_DEVICE_ID
    );


    // --------------------------------------------------
    // Create BLE server
    // --------------------------------------------------

    server() =
        BLEDevice::createServer();

    if (server() == nullptr)
    {
        Serial.println(
            "[BLE] ERROR: Failed to create server."
        );

        return false;
    }

    server()->setCallbacks(
        new ServerCallbacks()
    );


    // --------------------------------------------------
    // Create AVA service
    // --------------------------------------------------

    service() =
        server()->createService(
            SERVICE_UUID
        );

    if (service() == nullptr)
    {
        Serial.println(
            "[BLE] ERROR: Failed to create service."
        );

        return false;
    }


    // ==================================================
    // COMMAND
    // ==================================================

    commandCharacteristic() =
        service()->createCharacteristic(
            COMMAND_UUID,
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_WRITE_NR
        );

    if (commandCharacteristic() == nullptr)
    {
        Serial.println(
            "[BLE] ERROR: Failed to create COMMAND."
        );

        return false;
    }

    commandCharacteristic()->setCallbacks(
        new CommandCallbacks()
    );


    // ==================================================
    // EVENT
    // ==================================================

    eventCharacteristic() =
        service()->createCharacteristic(
            EVENT_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_NOTIFY
        );

    if (eventCharacteristic() == nullptr)
    {
        Serial.println(
            "[BLE] ERROR: Failed to create EVENT."
        );

        return false;
    }

    eventCharacteristic()->addDescriptor(
        new BLE2902()
    );


    // ==================================================
    // STATE
    // ==================================================

    stateCharacteristic() =
        service()->createCharacteristic(
            STATE_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_NOTIFY
        );

    if (stateCharacteristic() == nullptr)
    {
        Serial.println(
            "[BLE] ERROR: Failed to create STATE."
        );

        return false;
    }

    stateCharacteristic()->addDescriptor(
        new BLE2902()
    );


    // ==================================================
    // DATA
    // Bidirectional
    // ==================================================

    dataCharacteristic() =
        service()->createCharacteristic(
            DATA_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_WRITE_NR |
            BLECharacteristic::PROPERTY_NOTIFY
        );

    if (dataCharacteristic() == nullptr)
    {
        Serial.println(
            "[BLE] ERROR: Failed to create DATA."
        );

        return false;
    }

    dataCharacteristic()->addDescriptor(
        new BLE2902()
    );

    dataCharacteristic()->setCallbacks(
        new DataCallbacks()
    );


    // ==================================================
    // Start service
    // ==================================================

    service()->start();


    // ==================================================
    // Advertising
    // ==================================================

    BLEAdvertising* advertising =
        BLEDevice::getAdvertising();

    if (advertising == nullptr)
    {
        Serial.println(
            "[BLE] ERROR: Advertising unavailable."
        );

        return false;
    }

    advertising->addServiceUUID(
        SERVICE_UUID
    );

    advertising->setScanResponse(
        true
    );

    advertising->setMinPreferred(
        0x06
    );

    advertising->setMinPreferred(
        0x12
    );

    BLEDevice::startAdvertising();


    // ==================================================
    // Debug information
    // ==================================================

    Serial.println(
        "[BLE] BLE initialized."
    );

    Serial.println(
        "[BLE] Device name: AVA"
    );

    Serial.println(
        "[BLE] Service created."
    );

    Serial.println(
        "[BLE] COMMAND characteristic: RX"
    );

    Serial.println(
        "[BLE] EVENT characteristic: TX"
    );

    Serial.println(
        "[BLE] STATE characteristic: TX"
    );

    Serial.println(
        "[BLE] DATA characteristic: RX/TX"
    );

    Serial.println(
        "[BLE] DATA RX callback: READY"
    );

    Serial.println(
        "[BLE] DATA GAME processing: MAIN LOOP"
    );

    Serial.println(
        "[BLE] Game score authority: ESP32"
    );

    Serial.println(
        "[BLE] Advertising started."
    );

    Serial.println(
        "[BLE] Ready for connection."
    );

    Serial.println(
        "======================================"
    );

    Serial.println();

    return true;
}


// ==================================================
// BLE UPDATE
//
// MUST be called from main loop()
//
// اینجا محل امن پردازش DATA و GAME است.
//
// BLE callback فقط packet را queue می‌کند.
// ==================================================

inline void update()
{
    // ==================================================
    // Connection event
    // ==================================================

    if (connectionEventPending())
    {
        if (!connected())
        {
            connectionEventPending() =
                false;

            return;
        }

        connectionEventPending() =
            false;

        sendEvent(
            "AVA_CONNECTED"
        );
    }


    // ==================================================
    // Process queued DATA
    //
    // IMPORTANT:
    //
    // This is now outside BTC_TASK.
    // ==================================================

    if (dataPending())
    {
        String data =
            readData();

        if (data.length() > 0)
        {
            Serial.println(
                "[BLE] Processing queued DATA from main loop..."
            );

            processData(
                data
            );
        }
    }
}


// ==================================================
// Connection status
// ==================================================

inline bool isConnected()
{
    return connected();
}


// ==================================================
// Send EVENT
// ==================================================

inline void sendEvent(
    const String& message
)
{
    BLECharacteristic* characteristic =
        eventCharacteristic();

    if (characteristic == nullptr)
    {
        Serial.println(
            "[BLE] EVENT unavailable."
        );

        return;
    }

    characteristic->setValue(
        message.c_str()
    );

    if (connected())
    {
        characteristic->notify();
    }

    Serial.print(
        "[BLE TX] EVENT: "
    );

    Serial.println(
        message
    );
}


// ==================================================
// Send STATE
// ==================================================

inline void sendState(
    const String& state
)
{
    BLECharacteristic* characteristic =
        stateCharacteristic();

    if (characteristic == nullptr)
    {
        Serial.println(
            "[BLE] STATE unavailable."
        );

        return;
    }

    characteristic->setValue(
        state.c_str()
    );

    if (connected())
    {
        characteristic->notify();
    }

    Serial.print(
        "[BLE TX] STATE: "
    );

    Serial.println(
        state
    );
}


// ==================================================
// Send DATA
// ==================================================

inline void sendData(
    const String& data
)
{
    BLECharacteristic* characteristic =
        dataCharacteristic();

    if (characteristic == nullptr)
    {
        Serial.println(
            "[BLE] DATA unavailable."
        );

        return;
    }

    characteristic->setValue(
        data.c_str()
    );

    if (connected())
    {
        characteristic->notify();
    }

    Serial.print(
        "[BLE TX] DATA: "
    );

    Serial.println(
        data
    );
}


} // namespace AvaBluetooth

#endif