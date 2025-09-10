#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_timer.h>

// my includes
#include "keypad.hpp"
#include "door.h"
#include "passcode.h"

static char const *const TAG = "APP_MAIN";

const char popChar = '*';
const char validateChar = '#';

Keypad<4, 4> keypad{
    {{{'1', '2', '3', 'A'},
      {'4', '5', '6', 'B'},
      {'7', '8', '9', 'C'},
      {'*', '0', '#', 'D'}}},
    {GPIO_NUM_13, GPIO_NUM_12, GPIO_NUM_14, GPIO_NUM_27},
    {GPIO_NUM_26, GPIO_NUM_25, GPIO_NUM_33, GPIO_NUM_32}};

extern "C" void app_main(void)
{
  // debug
  // esp_log_level_set("*", ESP_LOG_INFO);

  keypad.beginScanTask();

  char keyChar{};
  while (true)
  {
    if (keypad.getPressed(keyChar, portMAX_DELAY))
    {
      ESP_LOGI(TAG, "Pressed key: %c", keyChar);
    };
    vTaskDelay(1);
  }
}
