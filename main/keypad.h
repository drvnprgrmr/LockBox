#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_timer.h>

#include "common.h"


typedef enum {
  PRESSED,
  RELEASED,
} KeyState;

typedef struct {
  byte r;
  byte c;
} KeyPos;

#define ROW_SIZE 4
#define COL_SIZE 4


// keypad on hold or not
extern bool onHold;

// current key
extern char curKey;

/**
 * FUNCTIONS
 */
void initPins();
void scanPins();