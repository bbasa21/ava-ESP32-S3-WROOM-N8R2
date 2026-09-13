#ifndef AVA_MOVEMENT_H
#define AVA_MOVEMENT_H

// AVA Movement System

enum MoveState {
  MOVE_STOP,
  MOVE_FORWARD,
  MOVE_BACKWARD,
  MOVE_LEFT,
  MOVE_RIGHT,
  MOVE_TURN_LEFT,
  MOVE_TURN_RIGHT,
  MOVE_DANCE,
  MOVE_NOD
};

const char* getMoveName(MoveState state) {
  switch (state) {
    case MOVE_STOP:       return "STOP";
    case MOVE_FORWARD:    return "FORWARD";
    case MOVE_BACKWARD:   return "BACKWARD";
    case MOVE_LEFT:       return "LEFT";
    case MOVE_RIGHT:      return "RIGHT";
    case MOVE_TURN_LEFT:  return "TURN_LEFT";
    case MOVE_TURN_RIGHT: return "TURN_RIGHT";
    case MOVE_DANCE:      return "DANCE";
    case MOVE_NOD:        return "NOD";
    default:              return "STOP";
  }
}

#endif