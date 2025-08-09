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

void app_main(void)
{
  initPins();

  initDoor();

  while (true)
  {
    // add delay to please the watchdog
    vTaskDelay(50 / portTICK_PERIOD_MS);
    
    // check the keypad for keypresses
    scanPins();

    if (!onHold)
    {

      if (curKey == popChar)
      {
        popPasscode();
        continue;
      }
      else if (curKey == validateChar)
      {
        if (validatePasscode())
        {
          unlockDoor();
          continue;
        };
      }

      // add key to input passcode
      appendPasscode(curKey);
    }

    
  }
}
