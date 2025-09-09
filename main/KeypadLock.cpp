#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_timer.h>

// my includes
#include "keypad.h"
#include "door.h"
#include "passcode.h"

const char popChar = '*';
const char validateChar = '#';

extern "C" void app_main(void)
{
  // initPins();

  // initDoor();

  // while (true)
  // {
  //   // add delay to please the watchdog
  //   vTaskDelay(50 / portTICK_PERIOD_MS);

  //   // check the keypad for keypresses
  //   scanPins();

  //   if (!onHold)
  //   {

  //     if (curKey == popChar)
  //     {
  //       popPasscode();
  //       continue;
  //     }
  //     else if (curKey == validateChar)
  //     {
  //       if (validatePasscode())
  //       {
  //         unlockDoor();
  //         continue;
  //       };
  //     }

  //     // add key to input passcode
  //     appendPasscode(curKey);
  //   }

  // }

  int const cols = 4;
  int const rows = 4;
  std::array<gpio_num_t, rows> rowPins = {GPIO_NUM_13, GPIO_NUM_12, GPIO_NUM_14, GPIO_NUM_27};
  std::array<gpio_num_t, cols> columnPins = {GPIO_NUM_26, GPIO_NUM_25, GPIO_NUM_33, GPIO_NUM_32};
  std::array<std::array<char, cols>, rows> keymap = {{{'1', '2', '3', 'A'},
                                                      {'4', '5', '6', 'B'},
                                                      {'7', '8', '9', 'C'},
                                                      {'*', '0', '#', 'D'}}};

  Keypad<rows, cols> keypad{keymap, rowPins, columnPins};
  while (true)
  {
    keypad.scanKeys();
    vTaskDelay(1);
  }
}
