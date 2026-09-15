#ifndef AVA_TIC_TAC_TOE_H
#define AVA_TIC_TAC_TOE_H

#include <Arduino.h>

// ============================================================
// AVA TIC TAC TOE ENGINE
//
// AVA = O
// ALI = X
//
// The ESP32 is the sole game-brain / authority.
// The phone only sends ALI's selected cell and displays state.
//
// Cell indexes:
//   0 1 2
//   3 4 5
//   6 7 8
//
// Wire messages used by the BLE layer:
//   TTT_STATE|<board>|<turn>
//   TTT_MOVE|ALI|<cell>
//   TTT_MOVE|AVA|<cell>
//   TTT_MOVE_ACCEPTED|ALI|<cell>
//   TTT_MOVE_REJECTED|<reason>
//   TTT_RESULT|ALI / AVA / DRAW
//   TTT_SCORE|<ali>|<ava>
//   TTT_FINISHED|ALI / AVA / DRAW
// ============================================================

namespace AvaTicTacToe {

static constexpr char EMPTY = '-';
static constexpr char ALI = 'X';
static constexpr char AVA = 'O';

inline char (&board())[9]
{
    static char value[9] = {
        EMPTY, EMPTY, EMPTY,
        EMPTY, EMPTY, EMPTY,
        EMPTY, EMPTY, EMPTY
    };
    return value;
}

inline bool& running()
{
    static bool value = false;
    return value;
}

inline bool& finished()
{
    static bool value = false;
    return value;
}

inline char& turn()
{
    static char value = ALI;
    return value;
}

inline char& winner()
{
    static char value = EMPTY;
    return value;
}

inline int& aliScore()
{
    static int value = 0;
    return value;
}

inline int& avaScore()
{
    static int value = 0;
    return value;
}

inline void clearBoard()
{
    for (int i = 0; i < 9; ++i)
        board()[i] = EMPTY;
}

inline void resetRound()
{
    clearBoard();
    running() = true;
    finished() = false;
    winner() = EMPTY;
    turn() = ALI;
}

inline void resetGame()
{
    aliScore() = 0;
    avaScore() = 0;
    resetRound();
}

inline bool isFull()
{
    for (int i = 0; i < 9; ++i)
        if (board()[i] == EMPTY)
            return false;
    return true;
}

inline char checkWinner()
{
    static const uint8_t lines[8][3] = {
        {0,1,2}, {3,4,5}, {6,7,8},
        {0,3,6}, {1,4,7}, {2,5,8},
        {0,4,8}, {2,4,6}
    };

    for (int i = 0; i < 8; ++i)
    {
        char a = board()[lines[i][0]];
        char b = board()[lines[i][1]];
        char c = board()[lines[i][2]];

        if (a != EMPTY && a == b && b == c)
            return a;
    }

    if (isFull())
        return 'D';

    return EMPTY;
}

inline int findWinningMove(char player)
{
    for (int i = 0; i < 9; ++i)
    {
        if (board()[i] != EMPTY)
            continue;

        board()[i] = player;
        char result = checkWinner();
        board()[i] = EMPTY;

        if (result == player)
            return i;
    }

    return -1;
}

inline int chooseAvaMove()
{
    int move = findWinningMove(AVA);
    if (move >= 0)
        return move;

    move = findWinningMove(ALI);
    if (move >= 0)
        return move;

    if (board()[4] == EMPTY)
        return 4;

    const int corners[4] = {0, 2, 6, 8};
    int freeCorners[4];
    int count = 0;

    for (int i = 0; i < 4; ++i)
    {
        if (board()[corners[i]] == EMPTY)
            freeCorners[count++] = corners[i];
    }

    if (count > 0)
        return freeCorners[random(0, count)];

    int freeCells[9];
    count = 0;

    for (int i = 0; i < 9; ++i)
    {
        if (board()[i] == EMPTY)
            freeCells[count++] = i;
    }

    if (count > 0)
        return freeCells[random(0, count)];

    return -1;
}

inline bool placeMove(char player, int cell)
{
    if (!running() || finished())
        return false;

    if (player != ALI && player != AVA)
        return false;

    if (cell < 0 || cell > 8)
        return false;

    if (turn() != player)
        return false;

    if (board()[cell] != EMPTY)
        return false;

    board()[cell] = player;

    char result = checkWinner();

    if (result == ALI || result == AVA || result == 'D')
    {
        finished() = true;
        running() = false;
        winner() = result;

        if (result == ALI)
            ++aliScore();
        else if (result == AVA)
            ++avaScore();

        return true;
    }

    turn() = (player == ALI) ? AVA : ALI;
    return true;
}

inline bool aliMove(int cell)
{
    return placeMove(ALI, cell);
}

inline bool avaMove(int cell)
{
    return placeMove(AVA, cell);
}

inline int makeAvaMove()
{
    if (!running() || finished() || turn() != AVA)
        return -1;

    int cell = chooseAvaMove();
    if (cell < 0)
        return -1;

    if (!avaMove(cell))
        return -1;

    return cell;
}

inline bool rematch()
{
    resetRound();
    return true;
}

inline bool isRunning()
{
    return running();
}

inline bool isFinished()
{
    return finished();
}

inline char getTurn()
{
    return turn();
}

inline char getWinner()
{
    return winner();
}

inline char getCell(int index)
{
    if (index < 0 || index > 8)
        return EMPTY;
    return board()[index];
}

inline String getBoardString()
{
    String result;
    result.reserve(9);

    for (int i = 0; i < 9; ++i)
        result += board()[i];

    return result;
}

inline int getAliScore()
{
    return aliScore();
}

inline int getAvaScore()
{
    return avaScore();
}

} // namespace AvaTicTacToe

#endif
