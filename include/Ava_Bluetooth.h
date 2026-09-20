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
#include "Ava_TicTacToe.h"
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
// COMMAND RX FIFO BUFFER
// ==================================================

static constexpr uint8_t COMMAND_QUEUE_SIZE = 4;

inline String* commandQueue()
{
    static String value[COMMAND_QUEUE_SIZE];
    return value;
}

inline uint8_t& commandQueueHead()
{
    static uint8_t value = 0;
    return value;
}

inline uint8_t& commandQueueTail()
{
    static uint8_t value = 0;
    return value;
}

inline uint8_t& commandQueueCount()
{
    static uint8_t value = 0;
    return value;
}

inline bool& commandPending()
{
    static bool value = false;
    return value;
}


// ==================================================
// COMMAND QUEUE API
// ==================================================

inline bool hasCommand()
{
    return commandQueueCount() > 0;
}


inline String readCommand()
{
    if (commandQueueCount() == 0)
    {
        commandPending() = false;
        return "";
    }


    String command =
        commandQueue()[commandQueueHead()];


    commandQueue()[commandQueueHead()] =
        "";


    commandQueueHead() =
        (commandQueueHead() + 1) %
        COMMAND_QUEUE_SIZE;


    commandQueueCount()--;


    commandPending() =
        (commandQueueCount() > 0);


    return command;
}


inline bool queueCommand(
    const String& command
)
{
    if (commandQueueCount() >=
        COMMAND_QUEUE_SIZE)
    {
        return false;
    }


    commandQueue()[commandQueueTail()] =
        command;


    commandQueueTail() =
        (commandQueueTail() + 1) %
        COMMAND_QUEUE_SIZE;


    commandQueueCount()++;


    commandPending() = true;


    return true;
}


inline void clearCommandQueue()
{
    for (uint8_t i = 0;
         i < COMMAND_QUEUE_SIZE;
         i++)
    {
        commandQueue()[i] = "";
    }


    commandQueueHead() = 0;
    commandQueueTail() = 0;
    commandQueueCount() = 0;
    commandPending() = false;
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

inline void sendGameQuestion()
{
    String message =
        "GAME_QUESTION|";


    message +=
        String(
            AvaGames::getCurrentRound()
        );


    // ==================================================
    // LOGIC BATTLE
    // ==================================================

    if (AvaGames::getCurrentGame() ==
        AvaGames::GAME_LOGIC_BATTLE)
    {
        message += "|";

        message +=
            AvaGames::getLogicQuestionText();


        for (int i = 0; i < 4; i++)
        {
            message += "|";

            message +=
                String(
                    AvaGames::getOption(i)
                );
        }


        sendData(message);

        return;
    }


    // ==================================================
    // MATH BATTLE
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


    for (int i = 0; i < 4; i++)
    {
        message += "|";

        message +=
            String(
                AvaGames::getOption(i)
            );
    }


    sendData(message);
}


// ==================================================
// Send game score
// ==================================================

inline void sendGameScore()
{
    String message =
        "GAME_SCORE|";


    message +=
        String(
            AvaGames::getAliScore()
        );


    message += "|";


    message +=
        String(
            AvaGames::getAvaScore()
        );


    sendData(message);
}


// ==================================================
// Send round result
// ==================================================

inline void sendGameResult()
{
    String message =
        "GAME_RESULT|";


    message +=
        String(
            AvaGames::getCurrentRound()
        );


    message += "|";


    message +=
        String(
            AvaGames::getAliAnswer()
        );


    message += "|";


    message +=
        String(
            AvaGames::getAvaAnswer()
        );


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
// ==================================================

inline void sendAvaGameAnswer()
{
    String message =
        "GAME_ANSWER|AVA|";


    message +=
        String(
            AvaGames::getAvaAnswer()
        );


    sendData(message);
}


// ==================================================
// Send final game result
// ==================================================

inline void sendGameFinished()
{
    String message =
        "GAME_FINISHED|";


    message +=
        AvaGames::getWinner();


    message += "|";


    message +=
        String(
            AvaGames::getAliScore()
        );


    message += "|";


    message +=
        String(
            AvaGames::getAvaScore()
        );


    sendData(message);
}


// ==================================================
// Process GAME_ANSWER
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
        data.substring(
            prefix.length()
        );


    int separator =
        payload.indexOf('|');


    if (separator < 0)
    {
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
        if (!AvaGames::isRunning())
        {
            sendData(
                "GAME_ANSWER_FAILED|GAME_NOT_RUNNING"
            );

            return;
        }


        if (AvaGames::hasAliAnswered())
        {
            sendData(
                "GAME_ANSWER_FAILED|ALREADY_ANSWERED"
            );

            return;
        }


        bool correct =
            AvaGames::submitAnswer(
                AvaGames::PLAYER_ALI,
                answer
            );


        String response =
            "GAME_ANSWER_ACCEPTED|ALI|";


        response +=
            correct ? "1" : "0";


        sendData(response);


        // ==================================================
        // AVA ANSWER
        // ==================================================

        int generatedAvaAnswer =
            AvaGames::generateAvaAnswer();


        AvaGames::submitAnswer(
            AvaGames::PLAYER_AVA,
            generatedAvaAnswer
        );
    }


    // ==================================================
    // AVA ANSWER
    // ==================================================

    else if (player == "AVA")
    {
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
        sendData(
            "GAME_ANSWER_FAILED|UNKNOWN_PLAYER"
        );

        return;
    }


    // ==================================================
    // BOTH PLAYERS?
    // ==================================================

    if (!AvaGames::bothPlayersAnswered())
    {
        return;
    }


    // ==================================================
    // APPLY SCORE
    // ==================================================

    AvaGames::applyRoundScore();


    // ==================================================
    // SEND AVA ANSWER
    // ==================================================

    sendAvaGameAnswer();


    // ==================================================
    // SEND RESULT
    // ==================================================

    sendGameResult();


    // ==================================================
    // SEND SCORE
    // ==================================================

    sendGameScore();


    // ==================================================
    // NEXT ROUND
    // ==================================================

    if (AvaGames::nextRound())
    {
        sendGameQuestion();
    }
    else
    {
        sendGameFinished();
    }
}


// ==================================================
// TIC TAC TOE
// ==================================================

inline String ticTacToeWinnerText()
{
    char winner =
        AvaTicTacToe::getWinner();


    if (winner == AvaTicTacToe::ALI)
    {
        return "ALI";
    }


    if (winner == AvaTicTacToe::AVA)
    {
        return "AVA";
    }


    return "DRAW";
}


// ==================================================
// Send TTT state
// ==================================================

inline void sendTicTacToeState()
{
    String message =
        "TTT_STATE|";


    message +=
        AvaTicTacToe::getBoardString();


    message += "|";


    message +=
        String(
            AvaTicTacToe::getTurn()
        );


    sendData(message);
}


// ==================================================
// Send TTT score
// ==================================================

inline void sendTicTacToeScore()
{
    String message =
        "TTT_SCORE|";


    message +=
        String(
            AvaTicTacToe::getAliScore()
        );


    message += "|";


    message +=
        String(
            AvaTicTacToe::getAvaScore()
        );


    sendData(message);
}


// ==================================================
// Send TTT finished
// ==================================================

inline void sendTicTacToeFinished()
{
    String winner =
        ticTacToeWinnerText();


    sendData(
        "TTT_RESULT|" + winner
    );


    sendTicTacToeScore();


    sendData(
        "TTT_FINISHED|" + winner
    );
}


// ==================================================
// START TIC TAC TOE
// ==================================================

inline void startTicTacToe()
{
    AvaTicTacToe::resetGame();


    // ==================================================
    // AVA LOOKS DOWN AT PHONE
    // ==================================================

    setGaze(
        GAZE_DOWN
    );


    sendEvent(
        "GAME_STARTED"
    );


    sendTicTacToeScore();

    sendTicTacToeState();


    Serial.println(
        "[TTT BLE] TIC TAC TOE STARTED."
    );


    Serial.println(
        "[TTT BLE] ALI = X | AVA = O"
    );
}


// ==================================================
// Process TTT move
// ==================================================

inline void processTicTacToeMove(
    const String& data
)
{
    const String prefix =
        "TTT_MOVE|";


    if (!data.startsWith(prefix))
    {
        return;
    }


    String payload =
        data.substring(
            prefix.length()
        );


    int separator =
        payload.indexOf('|');


    if (separator < 0)
    {
        sendData(
            "TTT_MOVE_REJECTED|INVALID_FORMAT"
        );

        return;
    }


    String player =
        payload.substring(
            0,
            separator
        );


    String cellText =
        payload.substring(
            separator + 1
        );


    player.trim();
    cellText.trim();

    player.toUpperCase();


    // ==================================================
    // ONLY ALI
    // ==================================================

    if (player != "ALI")
    {
        sendData(
            "TTT_MOVE_REJECTED|ALI_ONLY"
        );

        return;
    }


    // ==================================================
    // CELL
    // ==================================================

    int cell =
        cellText.toInt();


    if (cell < 0 || cell > 8)
    {
        sendData(
            "TTT_MOVE_REJECTED|INVALID_CELL"
        );

        return;
    }


    // ==================================================
    // GAME RUNNING
    // ==================================================

    if (!AvaTicTacToe::isRunning())
    {
        sendData(
            "TTT_MOVE_REJECTED|GAME_NOT_RUNNING"
        );

        return;
    }


    // ==================================================
    // TURN
    // ==================================================

    if (AvaTicTacToe::getTurn() !=
        AvaTicTacToe::ALI)
    {
        sendData(
            "TTT_MOVE_REJECTED|NOT_YOUR_TURN"
        );

        return;
    }


    // ==================================================
    // APPLY ALI MOVE
    // ==================================================

    if (!AvaTicTacToe::aliMove(cell))
    {
        sendData(
            "TTT_MOVE_REJECTED|INVALID_MOVE"
        );

        return;
    }


    // ==================================================
    // ACCEPT
    // ==================================================

    sendData(
        "TTT_MOVE_ACCEPTED|ALI|" +
        String(cell)
    );


    sendTicTacToeState();


    Serial.print(
        "[TTT BLE] ALI -> "
    );


    Serial.println(cell);


    // ==================================================
    // ALI FINISHED GAME?
    // ==================================================

    if (AvaTicTacToe::isFinished())
    {
        sendTicTacToeFinished();

        return;
    }


    // ==================================================
    // AVA MOVE
    // ==================================================

    int avaCell =
        AvaTicTacToe::makeAvaMove();


    if (avaCell < 0)
    {
        sendData(
            "TTT_MOVE_REJECTED|AVA_MOVE_FAILED"
        );

        return;
    }


    sendData(
        "TTT_MOVE|AVA|" +
        String(avaCell)
    );


    sendTicTacToeState();


    Serial.print(
        "[TTT BLE] AVA -> "
    );


    Serial.println(
        avaCell
    );


    // ==================================================
    // FINAL STATE
    // ==================================================

    if (AvaTicTacToe::isFinished())
    {
        sendTicTacToeFinished();
    }
}


// ==================================================
// Process incoming DATA
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


    // ==================================================
    // GAME ANSWER
    // ==================================================

    if (value.startsWith(
        "GAME_ANSWER|"
    ))
    {
        processGameAnswer(
            value
        );

        return;
    }


    // ==================================================
    // TTT MOVE
    // ==================================================

    if (value.startsWith(
        "TTT_MOVE|"
    ))
    {
        processTicTacToeMove(
            value
        );

        return;
    }


    // ==================================================
    // TTT REMATCH
    // ==================================================

    if (value == "TTT_REMATCH")
    {
        Serial.println(
            "[TTT BLE] REMATCH requested."
        );


        // --------------------------------------------------
        // IMPORTANT:
        // Do NOT call avaApplyEmotion(AVA_EMOTION_NORMAL)
        // here.
        //
        // The app sends:
        //
        // DEBUG OLED
        // TTT_REMATCH
        //
        // DEBUG OLED must be allowed to perform its own
        // OLED / eye debug behavior without being reset
        // by the rematch handler.
        // --------------------------------------------------


        // --------------------------------------------------
        // Keep gaze down
        // --------------------------------------------------

        setGaze(
            GAZE_DOWN
        );


        // --------------------------------------------------
        // Start new round
        // --------------------------------------------------

        if (AvaTicTacToe::rematch())
        {
            Serial.println(
                "[TTT BLE] REMATCH accepted."
            );


            sendEvent(
                "GAME_STARTED"
            );


            sendTicTacToeScore();

            sendTicTacToeState();


            sendData(
                "TTT_REMATCHED"
            );
        }
        else
        {
            Serial.println(
                "[TTT BLE] REMATCH rejected."
            );


            sendData(
                "TTT_REMATCH_REJECTED"
            );
        }


        return;
    }


    // ==================================================
    // OTHER DATA
    // ==================================================

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

    // "debug eyes" is intentionally lowercase and case-sensitive.
    if (cmd == "debug eyes")
    {
        avaEnableDebugEyes();
        sendEvent("DEBUG_EYES");
        return;
    }

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
        sendEvent(
            "TIME_REQUEST"
        );


        return;
    }


    // ==================================================
    // DEBUG OLED
    // ==================================================

    if (cmd == "DEBUG OLED")
    {
        avaApplyEmotion(
            AVA_EMOTION_NORMAL
        );


        sendEvent(
            "DEBUG_OLED"
        );


        return;
    }


    // ==================================================
    // MY GAMES ENTER
    // ==================================================

    if (cmd == "MY_GAMES_ENTER")
    {
        avaMyGamesEnter();


        sendEvent(
            "MY_GAMES_READY"
        );


        return;
    }


    // ==================================================
    // MY GAMES EXIT
    // ==================================================

    if (cmd == "MY_GAMES_EXIT")
    {
        avaExitGameGazeLock();
        avaApplyEmotion(AVA_EMOTION_NORMAL);
        sendEvent("MY_GAMES_EXIT");
        return;
    }


    // ==================================================
    // GAME LOAD - TIC TAC TOE
    // ==================================================

    if (cmd ==
        "GAME_LOAD|TIC_TAC_TOE")
    {
        avaEnterGameGazeLock();
        startTicTacToe();

        return;
    }


    // ==================================================
    // TTT REMATCH COMMAND
    // ==================================================

    if (cmd == "TTT_REMATCH")
    {
        Serial.println(
            "[TTT BLE] COMMAND REMATCH requested."
        );

        avaEnterGameGazeLock();


        // --------------------------------------------------
        // IMPORTANT:
        // Do NOT reset AVA emotion here.
        //
        // DEBUG OLED is sent separately by the app
        // before TTT_REMATCH.
        // --------------------------------------------------


        setGaze(
            GAZE_DOWN
        );


        if (AvaTicTacToe::rematch())
        {
            Serial.println(
                "[TTT BLE] COMMAND REMATCH accepted."
            );


            sendEvent(
                "GAME_STARTED"
            );


            sendTicTacToeScore();

            sendTicTacToeState();


            sendData(
                "TTT_REMATCHED"
            );
        }
        else
        {
            Serial.println(
                "[TTT BLE] COMMAND REMATCH rejected."
            );


            sendData(
                "TTT_REMATCH_REJECTED"
            );
        }


        return;
    }


    // ==================================================
    // GAME LOAD - OTHER GAMES
    // ==================================================

    if (cmd.startsWith(
        "GAME_LOAD|"
    ))
    {
        avaEnterGameGazeLock();

        String gameId =
            cmd.substring(
                String("GAME_LOAD|").length()
            );


        gameId.trim();


        if (AvaGames::loadGame(gameId))
        {
            sendEvent(
                "GAME_LOADED|" +
                gameId
            );
        }
        else
        {
            sendEvent(
                "GAME_LOAD_FAILED|" +
                gameId
            );
        }


        return;
    }


    // ==================================================
    // GAME START
    // ==================================================

    if (cmd == "GAME_START")
    {
        avaEnterGameGazeLock();

        if (AvaTicTacToe::isRunning())
        {
            startTicTacToe();

            return;
        }


        if (AvaGames::startGame())
        {
            sendEvent(
                "GAME_STARTED"
            );


            sendGameQuestion();
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
        if (AvaTicTacToe::isRunning() ||
            AvaTicTacToe::isFinished())
        {
            AvaTicTacToe::resetGame();
            avaExitGameGazeLock();


            sendEvent(
                "GAME_ENDED"
            );


            return;
        }


        AvaGames::endGame();
        avaExitGameGazeLock();


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
    // UNKNOWN COMMAND
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


        // --------------------------------------------------
        // Clear ALL pending BLE commands from this session.
        // --------------------------------------------------

        clearCommandQueue();


        receivedData() =
            "";


        dataPending() =
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
            return;
        }


        std::string value =
            characteristic->getValue();


        if (value.empty())
        {
            return;
        }


        String command =
            String(
                value.c_str()
            );


        command.trim();


        if (command.length() == 0)
        {
            return;
        }


        Serial.print(
            "[BLE RX] COMMAND: "
        );


        Serial.println(command);


        // --------------------------------------------------
        // Queue command instead of using a single-slot
        // command buffer.
        // --------------------------------------------------

        if (queueCommand(command))
        {
            Serial.print(
                "[BLE] COMMAND queued for main loop. Queue="
            );


            Serial.println(
                commandQueueCount()
            );
        }
        else
        {
            Serial.println(
                "[BLE] WARNING: COMMAND queue full."
            );
        }
    }
};


// ==================================================
// DATA CALLBACK
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
            return;
        }


        std::string value =
            characteristic->getValue();


        if (value.empty())
        {
            return;
        }


        String data =
            String(
                value.c_str()
            );


        data.trim();


        if (data.length() == 0)
        {
            return;
        }


        Serial.print(
            "[BLE RX] DATA: "
        );


        Serial.println(data);


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
        }
    }
};


// ==================================================
// BLE BEGIN
// ==================================================

inline bool begin()
{
    BLEDevice::init(
        "AVA"
    );


    server() =
        BLEDevice::createServer();


    if (server() == nullptr)
    {
        return false;
    }


    server()->setCallbacks(
        new ServerCallbacks()
    );


    service() =
        server()->createService(
            SERVICE_UUID
        );


    if (service() == nullptr)
    {
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
        return false;
    }


    stateCharacteristic()->addDescriptor(
        new BLE2902()
    );


    // ==================================================
    // DATA
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
        return false;
    }


    dataCharacteristic()->addDescriptor(
        new BLE2902()
    );


    dataCharacteristic()->setCallbacks(
        new DataCallbacks()
    );


    // ==================================================
    // START SERVICE
    // ==================================================

    service()->start();


    // ==================================================
    // ADVERTISING
    // ==================================================

    BLEAdvertising* advertising =
        BLEDevice::getAdvertising();


    if (advertising == nullptr)
    {
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


    Serial.println(
        "[BLE] BLE initialized."
    );


    Serial.println(
        "[BLE] Device name: AVA"
    );


    Serial.println(
        "[BLE] Ready for connection."
    );


    return true;
}


// ==================================================
// BLE UPDATE
// ==================================================

inline void update()
{
    // ==================================================
    // CONNECTION EVENT
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
    // COMMAND
    // ==================================================

    if (hasCommand())
    {
        String command =
            readCommand();


        if (command.length() > 0)
        {
            Serial.print(
                "[BLE] Processing queued COMMAND from main loop. Remaining="
            );


            Serial.println(
                commandQueueCount()
            );


            executeCommand(
                command
            );
        }
    }


    // ==================================================
    // DATA
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
// SEND EVENT
// ==================================================

inline void sendEvent(
    const String& message
)
{
    BLECharacteristic* characteristic =
        eventCharacteristic();


    if (characteristic == nullptr)
    {
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
// SEND STATE
// ==================================================

inline void sendState(
    const String& state
)
{
    BLECharacteristic* characteristic =
        stateCharacteristic();


    if (characteristic == nullptr)
    {
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
// SEND DATA
// ==================================================

inline void sendData(
    const String& data
)
{
    BLECharacteristic* characteristic =
        dataCharacteristic();


    if (characteristic == nullptr)
    {
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
