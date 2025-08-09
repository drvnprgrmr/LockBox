#pragma once

#include <freertos/FreeRTOS.h>
#include <esp_timer.h>
#include "common.h"

#define PASSCODE_LENGTH 4

extern const char secretPasscode[PASSCODE_LENGTH];

extern char inputPasscode[PASSCODE_LENGTH];

void displayPasscode();

void appendPasscode(char chr);

void popPasscode();

bool validatePasscode();
