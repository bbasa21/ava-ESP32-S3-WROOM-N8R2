#ifndef AVA_TYPES_H
#define AVA_TYPES_H


// ================================
// AVA Common Types
// ================================


// حالت احساسی آوا
enum Mood {
  MOOD_CALM,
  MOOD_HAPPY,
  MOOD_SAD,
  MOOD_SLEEPY,
  MOOD_THINKING,
  MOOD_LISTENING
};


// وضعیت کلی آوا
enum RobotState {
  STATE_BOOTING,
  STATE_IDLE,
  STATE_LISTENING,
  STATE_THINKING,
  STATE_SLEEPING,
  STATE_CHARGING,
  STATE_PLAYING
};


#endif