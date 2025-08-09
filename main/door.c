#include "door.h"

DoorState doorState = DOOR_CLOSED;
DoorLockState doorLockState = DOOR_LOCKED;

void initDoor()
{
  // configure pin for the door
  gpio_set_direction(DOOR_STATE_PIN, GPIO_MODE_INPUT);
  gpio_set_pull_mode(DOOR_STATE_PIN, GPIO_PULLUP_ONLY);

  // configure pin for the door lock
  gpio_set_direction(DOOR_LOCK_STATE_PIN, GPIO_MODE_OUTPUT);
}

void lockDoor()
{
  gpio_set_level(DOOR_LOCK_STATE_PIN, 1);
  doorLockState = DOOR_LOCKED;

  esp_rom_printf("Door locked!");
};

void unlockDoor()
{
  gpio_set_level(DOOR_LOCK_STATE_PIN, 0);
  doorLockState = DOOR_UNLOCKED;

  esp_rom_printf("Door unlocked!");
};