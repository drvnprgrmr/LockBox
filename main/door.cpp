#include "door.h"

const gpio_num_t doorStatePin = GPIO_NUM_2;
const gpio_num_t doorLockStatePin = GPIO_NUM_5;

DoorState doorState = DOOR_CLOSED;
DoorLockState doorLockState = DOOR_LOCKED;

void initDoor()
{
  // configure pin for the door
  gpio_set_direction(doorStatePin, GPIO_MODE_INPUT);
  gpio_set_pull_mode(doorStatePin, GPIO_PULLUP_ONLY);

  // configure pin for the door lock
  gpio_set_direction(doorLockStatePin, GPIO_MODE_OUTPUT);
}

void lockDoor()
{
  gpio_set_level(doorLockStatePin, 1);
  doorLockState = DOOR_LOCKED;

  esp_rom_printf("Door locked!");
};

void unlockDoor()
{
  gpio_set_level(doorLockStatePin, 0);
  doorLockState = DOOR_UNLOCKED;

  esp_rom_printf("Door unlocked!");
};