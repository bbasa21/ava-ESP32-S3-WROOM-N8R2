#pragma once

#include <Arduino.h>

void avaCommunicationBegin();
void avaCommunicationUpdate();

void avaCommunicationSend(const String& message);

bool avaCommunicationHasCommand();
String avaCommunicationReadCommand();