#pragma once
#include "keypad_types.hpp"
#include "keypad_config.h"

#include <array>
#include <queue>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_timer.h>
#include <esp_log.h>

/**
 * @brief Generic keypad driver (template-based)
 *
 * This class manages scanning and debouncing of a keypad matrix.
 *
 * @tparam rows Number of rows
 * @tparam cols Number of columns
 */
template <size_t rows, size_t cols>
class Keypad
{
public:
  /**
   * @brief Construct a new Keypad object
   *
   * Initializes GPIO pins and creates a key state matrix.
   *
   * @param keymap 2D array mapping each row/col to a character
   * @param rowPins Array of GPIO pins connected to rows
   * @param colPins Array of GPIO pins connected to columns
   */
  Keypad(std::array<std::array<char, cols>, rows> keymap,
         std::array<gpio_num_t, rows> rowPins,
         std::array<gpio_num_t, cols> colPins)
      : m_rowPins(rowPins), m_colPins(colPins)
  {
    for (size_t r = 0; r < rows; r++)
    {
      for (size_t c = 0; c < cols; c++)
      {
        m_keys[r][c] = Key{keymap[r][c], KeyState::IDLE};
      }
    }

    // create FreeRTOS queues
    m_pressedKeyQueue = xQueueCreate(CONFIG_KEYPAD_MAX_BUFFER_SIZE, sizeof(char));
    m_heldKeyQueue = xQueueCreate(CONFIG_KEYPAD_MAX_BUFFER_SIZE, sizeof(char));

    // initialize keypad pins
    initPins();

    ESP_LOGI(KEYPAD_TAG, "Keypad initialized (%zux%zu)", rows, cols);
  }

  ~Keypad()
  {
    if (m_pressedKeyQueue)
      vQueueDelete(m_pressedKeyQueue);
    if (m_heldKeyQueue)
      vQueueDelete(m_heldKeyQueue);
  }

public:
  /** @brief Start FreeRTOS task to continuously scan the keypad */
  void beginScanTask()
  {
    xTaskCreate(
        Keypad::foreverScanTask,
        "ScanKeypad",
        2048,
        this,
        1,
        &m_taskHandle);
  }

  /** @brief Stop the background keypad scan task */
  void stopScanTask()
  {
    if (m_taskHandle)
    {
      vTaskDelete(m_taskHandle);
      m_taskHandle = nullptr;
    }
  }

  /** @brief Blocking read for pressed key */
  bool getPressed(char &c, TickType_t timeout = 0)
  {
    return xQueueReceive(m_pressedKeyQueue, &c, timeout) == pdTRUE;
  }

  /** @brief Blocking read for held key */
  bool getHeld(char &c, TickType_t timeout = 0)
  {
    return xQueueReceive(m_heldKeyQueue, &c, timeout) == pdTRUE;
  }

  /** @brief Set debounce time in microseconds */
  esp_err_t setDebounceTime(uint64_t debounceTime)
  {
    if (debounceTime > 1000 && debounceTime < m_holdTime - 100000)
    {
      m_debounceTime = debounceTime;
      return ESP_OK;
    }
    return ESP_ERR_INVALID_ARG;
  }

  /** @brief Set hold time in microseconds */
  esp_err_t setHoldTime(uint64_t holdTime)
  {
    if (holdTime > m_debounceTime + 100000)
    {
      m_holdTime = holdTime;
      return ESP_OK;
    }
    return ESP_ERR_INVALID_ARG;
  }

  /** @brief Scan the keypad forever (blocking, not in a task) */
  void foreverScan()
  {
    while (true)
    {
      scanKeys();
      vTaskDelay(1);
    }
  }

  /** @brief Perform one keypad scan and update states */
  void scanKeys()
  {
    if (esp_timer_get_time() - m_lastScanTime > m_debounceTime)
    {
      for (size_t r = 0; r < rows; r++)
      {
        for (size_t c = 0; c < cols; c++)
        {
          gpio_set_level(m_colPins[c], 1);
          KeyLevel level = gpio_get_level(m_rowPins[r]) ? KeyLevel::HIGH : KeyLevel::LOW;
          updateKey(r, c, level);
          gpio_set_level(m_colPins[c], 0);
        }
      }
      m_lastScanTime = esp_timer_get_time();
    }
  }

private:
  TaskHandle_t m_taskHandle{nullptr};

  QueueHandle_t m_pressedKeyQueue = nullptr;
  QueueHandle_t m_heldKeyQueue = nullptr;

  std::array<std::array<Key, cols>, rows> m_keys{};

  std::array<gpio_num_t, rows> m_rowPins;
  std::array<gpio_num_t, cols> m_colPins;

  uint64_t m_lastScanTime{0};
  uint64_t m_debounceTime{KEYPAD_DEFAULT_DEBOUNCE};
  uint64_t m_holdTime{KEYPAD_DEFAULT_HOLD};

private:
  void initPins()
  {
    for (auto pin : m_rowPins)
    {
      gpio_set_direction(pin, GPIO_MODE_INPUT);
      gpio_set_pull_mode(pin, GPIO_PULLDOWN_ONLY);
    }
    for (auto pin : m_colPins)
    {
      gpio_set_direction(pin, GPIO_MODE_OUTPUT);
      gpio_set_level(pin, 0);
    }
  }

  static void foreverScanTask(void *pvParameters)
  {
    auto *instance = static_cast<Keypad *>(pvParameters);
    instance->foreverScan();
    vTaskDelete(nullptr);
  }

  void updateKey(size_t r, size_t c, KeyLevel level)
  {
    Key &key = m_keys[r][c];
    if (level == KeyLevel::HIGH)
    {
      if (key.state == KeyState::IDLE || key.state == KeyState::RELEASED)
      {
        key.state = KeyState::PRESSED;

        // update the queue for pressed keys
        char chr = key.chr;
        xQueueSend(m_pressedKeyQueue, &chr, 0);

        ESP_LOGD(KEYPAD_TAG, "Key pressed: %c", key.chr);
        key.holdTimer = esp_timer_get_time();
      }
      else if (key.state == KeyState::PRESSED &&
               (esp_timer_get_time() - key.holdTimer > m_holdTime))
      {
        key.state = KeyState::HELD;

        // update the queue for held keys
        char chr = key.chr;
        xQueueSend(m_heldKeyQueue, &chr, 0);

        ESP_LOGD(KEYPAD_TAG, "Key held: %c", key.chr);
      }
    }
    else
    {
      if (key.state == KeyState::PRESSED || key.state == KeyState::HELD)
      {
        key.state = KeyState::RELEASED;
      }
      else if (key.state == KeyState::RELEASED)
      {
        key.state = KeyState::IDLE;
      }
    }
  }
};
