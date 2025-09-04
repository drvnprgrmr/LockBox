#include "keypad.h"

const gpio_num_t rowPins[ROW_SIZE] = {GPIO_NUM_13, GPIO_NUM_12, GPIO_NUM_14, GPIO_NUM_27};
const gpio_num_t colPins[COL_SIZE] = {GPIO_NUM_26, GPIO_NUM_25, GPIO_NUM_33, GPIO_NUM_32};

const char keys[ROW_SIZE][COL_SIZE] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};

// initialize key state matrix
KeyState keyStateMatrix[ROW_SIZE][COL_SIZE];

KeyState oldKeyState;

// keypad accepting new presses
bool onHold = true;

// store current key pointer
char curKey;

// store the timestamp of the last scan
int64_t lastScanTime = 0;

// debounce time for the keypresses
int64_t debounceTime = 10000; // 10 milliseconds

void initPins()
{
  // initialize the row pins
  for (byte r = 0; r < ROW_SIZE; r++)
  {
    // set the pin as input
    gpio_set_direction(rowPins[r], GPIO_MODE_INPUT);

    // set it to be low by default
    gpio_set_pull_mode(rowPins[r], GPIO_PULLDOWN_ONLY);
  }

  // initialize the column pins
  for (byte c = 0; c < COL_SIZE; c++)
  {
    // set the pin as output
    gpio_set_direction(colPins[c], GPIO_MODE_OUTPUT);

    // ensure the pin's output is low
    gpio_set_level(colPins[c], 0);
  }
}

void scanPins()
{
  if (esp_timer_get_time() - lastScanTime > debounceTime)
  {
    bool level = 0;

    for (byte r = 0; r < ROW_SIZE; r++)
    {
      for (byte c = 0; c < COL_SIZE; c++)
      {
        // set the column pin high
        gpio_set_level(colPins[c], 1);

        // add a 1 millisecond delay before reading pin
        vTaskDelay(1 / portTICK_PERIOD_MS);

        // read the row pin
        level = gpio_get_level(rowPins[r]);

        if (level)
        {
          // check the current state
          onHold = (keyStateMatrix[r][c] == PRESSED) ? true : false;

          // update current key
          curKey = keys[r][c];
        }

        // set the current state of the key
        keyStateMatrix[r][c] = level ? PRESSED : RELEASED;

        // reset the column pin low
        gpio_set_level(colPins[c], 0);
      }
    }
  }
  
  // reset the lastScanTime
  lastScanTime = esp_timer_get_time();
}
