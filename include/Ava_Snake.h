#ifndef AVA_SNAKE_H
#define AVA_SNAKE_H

#include <Arduino.h>

namespace AvaSnake {

static constexpr int START_POSITION = 0;
static constexpr int FINISH_POSITION = 100;

enum Turn {
    TURN_ALI,
    TURN_AVA
};

enum MoveType {
    MOVE_NORMAL,
    MOVE_LADDER,
    MOVE_SNAKE,
    MOVE_FINISHED
};

struct MoveResult {
    int player;
    int dice;
    int oldPosition;
    int landedPosition;
    int finalPosition;
    MoveType moveType;
    bool finished;
};

inline int &aliPosition() { static int value = START_POSITION; return value; }
inline int &avaPosition() { static int value = START_POSITION; return value; }
inline Turn &turn() { static Turn value = TURN_ALI; return value; }
inline int &lastDice() { static int value = 0; return value; }
inline int &winner() { static int value = -1; return value; }
inline bool &running() { static bool value = false; return value; }
inline bool &finished() { static bool value = false; return value; }

inline int snakeLadderTarget(int position)
{
    switch (position)
    {
        case 2:  return 38;
        case 7:  return 14;
        case 8:  return 31;
        case 15: return 26;
        case 21: return 42;
        case 28: return 84;
        case 36: return 44;
        case 51: return 67;
        case 71: return 91;
        case 78: return 98;
        case 16: return 6;
        case 47: return 26;
        case 49: return 11;
        case 62: return 19;
        case 64: return 36;
        case 74: return 53;
        case 89: return 68;
        case 92: return 88;
        case 95: return 75;
        case 99: return 54;
    }
    return position;
}

inline bool isLadder(int from, int to)
{
    return to > from;
}

inline bool isSnake(int from, int to)
{
    return to < from;
}

inline void reset()
{
    aliPosition() = START_POSITION;
    avaPosition() = START_POSITION;
    turn() = TURN_ALI;
    lastDice() = 0;
    winner() = -1;
    running() = false;
    finished() = false;
}

inline void start()
{
    reset();
    running() = true;
}

inline bool isAliTurn()
{
    return running() && !finished() && turn() == TURN_ALI;
}

inline bool isAvaTurn()
{
    return running() && !finished() && turn() == TURN_AVA;
}

inline MoveResult roll(int player)
{
    MoveResult result{};
    result.player = player;
    result.dice = 0;
    result.moveType = MOVE_NORMAL;
    result.finished = false;

    if (!running() || finished()) return result;
    if ((player == 0 && turn() != TURN_ALI) || (player == 1 && turn() != TURN_AVA)) return result;

    const int oldPosition = (player == 0) ? aliPosition() : avaPosition();
    const int dice = random(1, 7);
    const int landed = (oldPosition + dice <= FINISH_POSITION) ? oldPosition + dice : oldPosition;
    const int target = snakeLadderTarget(landed);

    if (player == 0) aliPosition() = target;
    else avaPosition() = target;

    lastDice() = dice;

    result.dice = dice;
    result.oldPosition = oldPosition;
    result.landedPosition = landed;
    result.finalPosition = target;

    if (target == FINISH_POSITION) {
        result.moveType = MOVE_FINISHED;
        result.finished = true;
        finished() = true;
        winner() = player;
        running() = false;
        return result;
    }

    if (target != landed)
        result.moveType = isLadder(landed, target) ? MOVE_LADDER : MOVE_SNAKE;

    turn() = (turn() == TURN_ALI) ? TURN_AVA : TURN_ALI;
    return result;
}

inline int getPosition(int player)
{
    return player == 0 ? aliPosition() : avaPosition();
}

inline int getWinner() { return winner(); }
inline Turn getTurn() { return turn(); }
inline bool isRunning() { return running(); }
inline bool isFinished() { return finished(); }

} // namespace AvaSnake

#endif
