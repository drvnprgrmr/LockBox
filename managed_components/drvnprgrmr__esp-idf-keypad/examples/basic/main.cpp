#include "keypad.hpp"

Keypad<4, 4> keypad{
    {{{'1', '2', '3', 'A'},
      {'4', '5', '6', 'B'},
      {'7', '8', '9', 'C'},
      {'*', '0', '#', 'D'}}},
    {GPIO_NUM_13, GPIO_NUM_12, GPIO_NUM_14, GPIO_NUM_27},
    {GPIO_NUM_26, GPIO_NUM_25, GPIO_NUM_33, GPIO_NUM_32}};

extern "C" void app_main(void)
{
  keypad.beginScanTask();

  char key;
  while (true)
  {
    if (keypad.getPressed(key, portMAX_DELAY))
    {
      ESP_LOGI("app", "Pressed: %c", key);
    }
    if (keypad.getHeld(key, 0))
    {
      ESP_LOGI("app", "Held: %c", key);
    }
  }
}
