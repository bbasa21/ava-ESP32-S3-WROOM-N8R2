#ifndef AVA_SNAKE_OLED_H
#define AVA_SNAKE_OLED_H

#include <Arduino.h>

namespace AvaSnakeOLED {

void begin();
void update();
void showRoll(int player);
void showDice(int player, int dice);
void showMove(int player, int oldPosition, int finalPosition, int moveType);
void showFinished(int player);

} // namespace AvaSnakeOLED

#endif
