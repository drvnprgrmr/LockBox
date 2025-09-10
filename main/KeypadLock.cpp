#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_timer.h>

// my includes
#include "keypad.h"
#include "door.h"
#include "passcode.h"

static char const *const TAG = "APP_MAIN";

const char popChar = '*';
const char validateChar = '#';

extern "C" void app_main(void)
{
  // debug
  esp_log_level_set("*", ESP_LOG_INFO);

  int const cols = 4;
  int const rows = 4;
  std::array<gpio_num_t, rows> rowPins = {GPIO_NUM_13, GPIO_NUM_12, GPIO_NUM_14, GPIO_NUM_27};
  std::array<gpio_num_t, cols> columnPins = {GPIO_NUM_26, GPIO_NUM_25, GPIO_NUM_33, GPIO_NUM_32};
  std::array<std::array<char, cols>, rows> keymap = {{{'1', '2', '3', 'A'},
                                                      {'4', '5', '6', 'B'},
                                                      {'7', '8', '9', 'C'},
                                                      {'*', '0', '#', 'D'}}};

  Keypad<rows, cols> keypad{keymap, rowPins, columnPins};
  keypad.beginScanTask();

  while (true)
  {
    if (!keypad.pressedKeyBuffer.empty())
    {
      char keyChar = keypad.pressedKeyBuffer.front();
      keypad.pressedKeyBuffer.pop();

      ESP_LOGI(TAG, "Pressed key: %c", keyChar);
    };
    vTaskDelay(1);
  }
}
