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

static constexpr char const *const TAG = "KEYPAD";

size_t constexpr updateListSize = 4;

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

// use interrupt instead
// char const NULL_KEY = '\0'; // represents a key that's not used

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
      for (size_t j = 0; j < cols; j++)
      {
        m_keys[i][j] = Key{keymap[i][j], KeyState::IDLE};
      }
    }

    // init pins
    initPins();

    ESP_LOGI(TAG, "Keypad initialized!");
  }

public:
  char getKey();
  char getKeys();

  void setDebounceTime(uint64_t); // time in xx
  void setHoldTime(uint64_t);

  // Scan the pins to detect pressed ones.
  void scanKeys()
  {
    // ESP_LOGI(TAG, "Cur: %llu last: %llu debounce: %llu\n", esp_timer_get_time(), m_lastScanTime, m_debounceTime);

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

          // print level
          // ESP_LOGI(TAG, "Level: %i\n", level);

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

  /* times in microseconds (us) */
  uint64_t m_lastScanTime{0};
  uint64_t m_debounceTime{50 * 1000}; // minimum time between scans
  uint64_t m_holdTime{700 * 1000};    // how long a key should be pressed down to be considered held

  // list to hold updates
  std::array<Key const *const, updateListSize> pressedList{}, heldList{};

private:
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

  void updateKey(size_t r, size_t c, KeyLevel level)
  {
    // get a reference of the key to be updated
    Key& key = m_keys[r][c];

    // current key level is high
    if (level == KeyLevel::HIGH)
    {
      // transitioning from idle/released to high
      if ((key.state == KeyState::IDLE || key.state == KeyState::RELEASED))
      {
        // change the key to a pressed state
        key.state = KeyState::PRESSED;

        // log a key press
        ESP_LOGI(TAG, "Key pressed: %c\n", key.chr);

        // set the time of transition
        key.holdTimer = esp_timer_get_time();
      }

      else if (key.state == KeyState::PRESSED && (esp_timer_get_time() - key.holdTimer > m_holdTime))
      {
        // remained high
        key.state = KeyState::HELD;

        // log a held key
        ESP_LOGI(TAG, "Key held: %c\n", key.chr);
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

  // todo: add other list but create config to select which to update
  // todo: for better memory efficiency
  void updatePressed(char) {}
  void updateHold() {}
};
