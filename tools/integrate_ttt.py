from pathlib import Path

path = Path("include/Ava_Bluetooth.h")
text = path.read_text(encoding="utf-8")

include_anchor = '#include "Ava_Config.h"\n'
include_line = '#include "Ava_TicTacToe.h"\n'
if include_line not in text:
    if include_anchor not in text:
        raise SystemExit("Could not find BLE include anchor")
    text = text.replace(include_anchor, include_anchor + include_line, 1)

helper_anchor = "// ==================================================\n// Process incoming DATA\n"
helpers = """// ==================================================
// TIC TAC TOE BLE HELPERS
//
// ESP32 is the sole game brain and score authority.
// Android only sends ALI moves and renders the state.
// ==================================================

inline String ticTacToeWinnerText()
{
    char winner = AvaTicTacToe::getWinner();

    if (winner == AvaTicTacToe::ALI)
        return "ALI";

    if (winner == AvaTicTacToe::AVA)
        return "AVA";

    return "DRAW";
}

inline void sendTicTacToeState()
{
    String message =
        "TTT_STATE|" +
        AvaTicTacToe::getBoardString() +
        "|" +
        String(AvaTicTacToe::getTurn());

    sendData(message);
}

inline void sendTicTacToeScore()
{
    String message =
        "TTT_SCORE|" +
        String(AvaTicTacToe::getAliScore()) +
        "|" +
        String(AvaTicTacToe::getAvaScore());

    sendData(message);
}

inline void sendTicTacToeFinished()
{
    String winner = ticTacToeWinnerText();

    sendData("TTT_RESULT|" + winner);
    sendTicTacToeScore();
    sendData("TTT_FINISHED|" + winner);
}

inline void startTicTacToe()
{
    AvaTicTacToe::resetGame();

    sendEvent("GAME_STARTED");
    sendTicTacToeScore();
    sendTicTacToeState();

    Serial.println("[TTT BLE] TIC TAC TOE STARTED.");
    Serial.println("[TTT BLE] ALI = X | AVA = O");
}

inline void processTicTacToeMove(const String& data)
{
    const String prefix = "TTT_MOVE|";

    if (!data.startsWith(prefix))
        return;

    String payload = data.substring(prefix.length());
    int separator = payload.indexOf('|');

    if (separator < 0)
    {
        sendData("TTT_MOVE_REJECTED|INVALID_FORMAT");
        return;
    }

    String player = payload.substring(0, separator);
    String cellText = payload.substring(separator + 1);

    player.trim();
    cellText.trim();
    player.toUpperCase();

    if (player != "ALI")
    {
        sendData("TTT_MOVE_REJECTED|ALI_ONLY");
        return;
    }

    int cell = cellText.toInt();

    if (cell < 0 || cell > 8)
    {
        sendData("TTT_MOVE_REJECTED|INVALID_CELL");
        return;
    }

    if (!AvaTicTacToe::isRunning())
    {
        sendData("TTT_MOVE_REJECTED|GAME_NOT_RUNNING");
        return;
    }

    if (AvaTicTacToe::getTurn() != AvaTicTacToe::ALI)
    {
        sendData("TTT_MOVE_REJECTED|NOT_YOUR_TURN");
        return;
    }

    if (!AvaTicTacToe::aliMove(cell))
    {
        sendData("TTT_MOVE_REJECTED|INVALID_MOVE");
        return;
    }

    sendData("TTT_MOVE_ACCEPTED|ALI|" + String(cell));
    sendTicTacToeState();

    Serial.print("[TTT BLE] ALI -> ");
    Serial.println(cell);

    if (AvaTicTacToe::isFinished())
    {
        sendTicTacToeFinished();
        return;
    }

    int avaCell = AvaTicTacToe::makeAvaMove();

    if (avaCell < 0)
    {
        sendData("TTT_MOVE_REJECTED|AVA_MOVE_FAILED");
        return;
    }

    sendData("TTT_MOVE|AVA|" + String(avaCell));
    sendTicTacToeState();

    Serial.print("[TTT BLE] AVA -> ");
    Serial.println(avaCell);

    if (AvaTicTacToe::isFinished())
        sendTicTacToeFinished();
}


"""

if "inline void processTicTacToeMove" not in text:
    if helper_anchor not in text:
        raise SystemExit("Could not find processData anchor")
    text = text.replace(helper_anchor, helpers + helper_anchor, 1)

data_anchor = '''    if (value.startsWith("GAME_ANSWER|"))
    {
        processGameAnswer(value);
        return;
    }
'''
data_insert = data_anchor + '''
    // --------------------------------------------------
    // TIC TAC TOE MOVE
    // --------------------------------------------------

    if (value.startsWith("TTT_MOVE|"))
    {
        processTicTacToeMove(value);
        return;
    }

    if (value == "TTT_REMATCH")
    {
        if (AvaTicTacToe::rematch())
        {
            sendEvent("GAME_STARTED");
            sendTicTacToeScore();
            sendTicTacToeState();
        }
        return;
    }
'''
if "processTicTacToeMove(value);" not in text:
    if data_anchor not in text:
        raise SystemExit("Could not find GAME_ANSWER data anchor")
    text = text.replace(data_anchor, data_insert, 1)

load_anchor = '''    if (cmd.startsWith("GAME_LOAD|"))
    {
        String gameId =
'''
load_insert = '''    if (cmd == "GAME_LOAD|TIC_TAC_TOE")
    {
        sendEvent("GAME_LOADED|TIC_TAC_TOE");
        return;
    }

''' + load_anchor
if 'cmd == "GAME_LOAD|TIC_TAC_TOE"' not in text:
    if load_anchor not in text:
        raise SystemExit("Could not find GAME_LOAD anchor")
    text = text.replace(load_anchor, load_insert, 1)

start_anchor = '''    if (cmd == "GAME_START")
    {
        if (AvaGames::startGame())
'''
start_insert = '''    if (cmd == "GAME_START")
    {
        if (AvaTicTacToe::isRunning())
        {
            startTicTacToe();
            return;
        }

        if (AvaGames::startGame())
'''
if 'startTicTacToe();' not in text:
    if start_anchor not in text:
        raise SystemExit("Could not find GAME_START anchor")
    text = text.replace(start_anchor, start_insert, 1)

end_anchor = '''    if (cmd == "GAME_END")
    {
        AvaGames::endGame();
'''
end_insert = '''    if (cmd == "GAME_END")
    {
        if (AvaTicTacToe::isRunning() || AvaTicTacToe::isFinished())
        {
            AvaTicTacToe::resetGame();
            sendEvent("GAME_ENDED");
            return;
        }

        AvaGames::endGame();
'''
if 'AvaTicTacToe::resetGame();\n            sendEvent("GAME_ENDED")' not in text:
    if end_anchor not in text:
        raise SystemExit("Could not find GAME_END anchor")
    text = text.replace(end_anchor, end_insert, 1)

reset_anchor = '''    if (cmd == "GAME_RESET")
    {
        AvaGames::reset();
'''
reset_insert = '''    if (cmd == "GAME_RESET")
    {
        if (AvaTicTacToe::isRunning() || AvaTicTacToe::isFinished())
        {
            AvaTicTacToe::resetGame();
            sendEvent("GAME_RESET");
            sendTicTacToeScore();
            sendTicTacToeState();
            return;
        }

        AvaGames::reset();
'''
if 'sendTicTacToeScore();\n            sendTicTacToeState();' not in text:
    if reset_anchor not in text:
        raise SystemExit("Could not find GAME_RESET anchor")
    text = text.replace(reset_anchor, reset_insert, 1)

path.write_text(text, encoding="utf-8")
print("Tic-Tac-Toe BLE integration applied.")
