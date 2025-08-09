#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_timer.h>

#include "common.h"

#define DOOR_STATE_PIN 2
#define DOOR_LOCK_STATE_PIN 5

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

