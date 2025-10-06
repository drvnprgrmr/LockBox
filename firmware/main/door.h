#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_timer.h>

#include "common.h"

extern const gpio_num_t doorStatePin;
extern const gpio_num_t doorLockStatePin;

typedef enum {
  DOOR_OPENED,
  DOOR_CLOSED
} DoorState;

typedef enum {
  DOOR_LOCKED,
  DOOR_UNLOCKED
} DoorLockState;

extern DoorState doorState;
extern DoorLockState doorLockState;

void initDoor();
void lockDoor();
void unlockDoor();

