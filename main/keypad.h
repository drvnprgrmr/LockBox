#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_timer.h>
#include <esp_log.h>

#ifdef __cplusplus
} // extern "C"
#endif

#include <array>
#include <queue>

// todo: fix
static char const *const KeypadTAG = "KEYPAD";

static size_t const MAX_KEY_BUFFER_SIZE = 10; // max keys to hold in buffer

// current level of the key
enum class KeyLevel
{
  LOW,
  HIGH
};

// state of the key based on current level and previous level
enum class KeyState
{
  IDLE,     // key is low and was not active previously
  RELEASED, // key is low and was active previously
  PRESSED,  // key is high and was not active previously
  HELD,     // key is high and was active previously (for a defined time)
};

struct Key
{
  char chr;            // the character this key represents
  KeyState state;      // the state of the key
  int64_t holdTimer{}; // the timer to keep track of when this key was held
};

template <size_t rows, size_t cols>
class Keypad
{
public:
  Keypad(std::array<std::array<char, cols>, rows> keymap, std::array<gpio_num_t, rows> rowPins, std::array<gpio_num_t, cols> columnPins)
      : m_rowPins(rowPins),
        m_columnPins(columnPins)
  {
    // create the key matrix
    for (size_t i = 0; i < rows; i++)
    {
      for (size_t c = 0; c < cols; c++)
      {
        m_keys[i][c] = Key{keymap[i][c], KeyState::IDLE};
      }
    }

    // init pins
    initPins();

    ESP_LOGI(KeypadTAG, "Keypad initialized!");
  }

public:
  // public key buffers that will be updated whenever a key changes
  std::queue<char> pressedKeyBuffer, heldKeyBuffer;

public:
  /* === Background tasks approach === */

  void beginScanTask()
  {
    xTaskCreate(
        Keypad::foreverScanTask,
        "Scan Keypad",
        1024,
        this,
        1,
        NULL);
  }

  void stopScanTask() {}

public:
  /* === Direct access approach ===  */

  esp_err_t setDebounceTime(uint64_t debounceTime)
  {
    if (debounceTime > 1 * 1000 && debounceTime < m_holdTime - 100 * 1000)
    {
      m_debounceTime = debounceTime;
      return ESP_OK;
    }
    return ESP_ERR_INVALID_ARG;
  }

  esp_err_t setHoldTime(uint64_t holdTime)
  {
    if (holdTime > m_debounceTime - 100 * 1000)
    {
      m_holdTime = holdTime;
      return ESP_OK;
    }
    return ESP_ERR_INVALID_ARG;
  }

  // scan the keys forever
  void foreverScan()
  {
    while (true)
    {
      scanKeys();
      vTaskDelay(1);
    }
  }

  void initPins()
  {
    // initialize the row pins
    for (size_t r = 0; r < rows; r++)
    {
      // set the pin as input
      gpio_set_direction(m_rowPins[r], GPIO_MODE_INPUT);

      // set it to be low by default
      gpio_set_pull_mode(m_rowPins[r], GPIO_PULLDOWN_ONLY);
    }

    // initialize the column pins
    for (size_t c = 0; c < cols; c++)
    {
      // set the pin as output
      gpio_set_direction(m_columnPins[c], GPIO_MODE_OUTPUT);

      // ensure the pin's output is low
      gpio_set_level(m_columnPins[c], 0);
    }
  }

  void scanKeys()
  {
    if (esp_timer_get_time() - m_lastScanTime > m_debounceTime)
    {
      // keeps track of the logic level of a key
      KeyLevel level{};

      for (size_t r = 0; r < rows; r++)
      {
        for (size_t c = 0; c < cols; c++)
        {
          // set the column pin high
          gpio_set_level(m_columnPins[c], 1);

          // read the row pin to get the state of the key
          level = gpio_get_level(m_rowPins[r]) ? KeyLevel::HIGH : KeyLevel::LOW;

          // update the key
          updateKey(r, c, level);

          // reset the column pin low
          gpio_set_level(m_columnPins[c], 0);
        }
      }

      m_lastScanTime = esp_timer_get_time();
    }
  }

private:
  // a matrix to manage each key of the keypad
  std::array<std::array<Key, cols>, rows> m_keys{};

  std::array<gpio_num_t, rows> m_rowPins;
  std::array<gpio_num_t, cols> m_columnPins;

  /* times are in microseconds (us) */
  uint64_t m_lastScanTime{0};
  uint64_t m_debounceTime{10 * 1000}; // minimum time between scans
  uint64_t m_holdTime{500 * 1000};    // how long a key should be pressed down to be considered held

private:
  static void foreverScanTask(void *pvParameters)
  {
    Keypad *instance = static_cast<Keypad *>(pvParameters);

    instance->foreverScan();

    // delete task if foreverScan ever returns
    vTaskDelete(NULL);
  }

  void updateKey(size_t r, size_t c, KeyLevel level)
  {
    // get a reference of the key to be updated
    Key &key = m_keys[r][c];

    // current key level is high
    if (level == KeyLevel::HIGH)
    {
      // transitioning from idle/released to high
      if ((key.state == KeyState::IDLE || key.state == KeyState::RELEASED))
      {
        // change the key to a pressed state
        key.state = KeyState::PRESSED;

        if (pressedKeyBuffer.size() <= MAX_KEY_BUFFER_SIZE)
        {
          // push the pressed key to the queue
          pressedKeyBuffer.push(key.chr);
        }

        // log a key press
        ESP_LOGD(KeypadTAG, "Key pressed: %c\n", key.chr);

        // set the time of transition
        key.holdTimer = esp_timer_get_time();
      }

      else if (key.state == KeyState::PRESSED && (esp_timer_get_time() - key.holdTimer > m_holdTime))
      {
        // remained high
        key.state = KeyState::HELD;

        if (heldKeyBuffer.size() <= MAX_KEY_BUFFER_SIZE)
        {
          // push the held key into a queue
          heldKeyBuffer.push(key.chr);
        }

        // log a held key
        ESP_LOGD(KeypadTAG, "Key held: %c\n", key.chr);
      }
    }

    // current key level is low
    else if (level == KeyLevel::LOW)
    {
      if ((key.state == KeyState::PRESSED || key.state == KeyState::HELD))
      {
        // transitioning from pressed/held down to low
        key.state = KeyState::RELEASED;
      }

      else if (key.state == KeyState::RELEASED)
      {
        // becomes idle if left low
        key.state = KeyState::IDLE;
      }
    }
  }
};