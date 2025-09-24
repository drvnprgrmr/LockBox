/* ----------------------------------- ESP ---------------------------------- */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_timer.h>

/* --------------------------------- MANAGED -------------------------------- */
#include "keypad.hpp"

/* ---------------------------------- LOCAL --------------------------------- */
#include "door.h"
#include "passcode.h"

static char const *const TAG = "APP_MAIN";

// create new keypad to handle key presses
Keypad<4, 4> keypad{
    {{{'1', '2', '3', 'A'},
      {'4', '5', '6', 'B'},
      {'7', '8', '9', 'C'},
      {'*', '0', '#', 'D'}}},
    {GPIO_NUM_13, GPIO_NUM_12, GPIO_NUM_14, GPIO_NUM_27},
    {GPIO_NUM_26, GPIO_NUM_25, GPIO_NUM_33, GPIO_NUM_32}};

// Instantiate class instance for handling passcode
Passcode passcode{{GPIO_NUM_5, GPIO_NUM_18, GPIO_NUM_19, GPIO_NUM_21}, GPIO_NUM_23, GPIO_NUM_4};

extern "C" void app_main(void)
{
  // debug
  esp_log_level_set("*", ESP_LOG_DEBUG);

  // increase debounce time
  keypad.setDebounceTime(50 * 1000);

  // begin scanning keys
  keypad.beginScanTask();

  char keyChar{};
  while (true)
  {
    if (keypad.getPressed(keyChar, portMAX_DELAY))
    {
      ESP_LOGD(TAG, "Pressed key: %c", keyChar);
      passcode.handleInput(keyChar);
    };
    vTaskDelay(1);
  }
}
