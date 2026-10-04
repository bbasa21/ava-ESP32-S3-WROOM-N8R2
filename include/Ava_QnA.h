#pragma once

#include <Arduino.h>

// Returns true when AVA has a built-in offline answer.
bool avaQnAAnswer(const String& question, String& answer);
