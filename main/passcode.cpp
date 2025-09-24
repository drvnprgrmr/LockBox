#include "passcode.h"

static char const *const TAG = "passcode";

// TODO: Save passcode lock state to nvs too

/* -------------------------------------------------------------------------- */

Passcode::Passcode()
{
  initNvs();
}

Passcode::Passcode(std::array<gpio_num_t, PASSCODE_LENGTH> inputIndicatorPins, gpio_num_t lockIndicatorPin, gpio_num_t buzzerPin)
    : m_inputIndicatorPins{inputIndicatorPins},
      m_lockIndicatorPin{lockIndicatorPin},
      m_buzzerPin{buzzerPin},
      m_pinsEnabled{true}
{
  initNvs();
  initPins();
}

Passcode::~Passcode()
{
  // Close NVS Handle
  nvs_close(m_nvsHandle);
  ESP_LOGV(TAG, "NVS handle closed.");

  if (m_pinsEnabled)
  {
    ledc_stop(LOCK_SPEED_MODE, LOCK_CHANNEL, 0);

    ledc_stop(BUZZER_SPEED_MODE, BUZZER_CHANNEL, 0);

    if (m_blinkTaskHandle)
    {
      vTaskDelete(m_blinkTaskHandle);
      m_blinkTaskHandle = nullptr;
    }
  }
}

void Passcode::initNvs()
{
  // initialize the nvs handle
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    // NVS partition was truncated and needs to be erased
    // Retry nvs_flash_init
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  // Open NVS handle
  ESP_LOGV(TAG, "Opening Non-Volatile Storage (NVS) handle...");
  err = nvs_open(TAG, NVS_READWRITE, &m_nvsHandle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
  }
}

void Passcode::initPins()
{
  // install fade function
  ledc_fade_func_install(0);

  // register blink task
  xTaskCreate(blinkTask, "BlinkAlarm", 1024, this, 0, &m_blinkTaskHandle);

  // init led pins
  for (gpio_num_t ledInputPin : m_inputIndicatorPins)
  {
    gpio_set_direction(ledInputPin, GPIO_MODE_OUTPUT);
    gpio_set_level(ledInputPin, 0);
  }

  // init lock pin
  ledc_timer_config_t lockTimerConfig = {
      .speed_mode = LOCK_SPEED_MODE,
      .duty_resolution = LOCK_DUTY_RESOLUTION,
      .timer_num = LOCK_TIMER,
      .freq_hz = LOCK_FREQUENCY, // 2kHz
      .clk_cfg = LOCK_CLK_CFG,
  };
  ledc_timer_config(&lockTimerConfig);

  ledc_channel_config_t lockChannelConfig = {
      .gpio_num = m_lockIndicatorPin,
      .speed_mode = LOCK_SPEED_MODE,
      .channel = LOCK_CHANNEL,
      .intr_type = LOCK_INTR_TYPE,
      .timer_sel = LOCK_TIMER,
      .duty = 0,
      .hpoint = 0,
  };
  ledc_channel_config(&lockChannelConfig);

  // init buzzer pin
  ledc_timer_config_t buzzerTimerConfig = {
      .speed_mode = BUZZER_SPEED_MODE,
      .duty_resolution = BUZZER_DUTY_RESOLUTION,
      .timer_num = BUZZER_TIMER,
      .freq_hz = BUZZER_FREQUENCY, // 2kHz
      .clk_cfg = BUZZER_CLK_CFG,
  };
  ledc_timer_config(&buzzerTimerConfig);

  ledc_channel_config_t buzzerChannelConfig = {
      .gpio_num = m_buzzerPin,
      .speed_mode = BUZZER_SPEED_MODE,
      .channel = BUZZER_CHANNEL,
      .intr_type = BUZZER_INTR_TYPE,
      .timer_sel = BUZZER_TIMER,
      .duty = 0,
      .hpoint = 0,
  };
  ledc_channel_config(&buzzerChannelConfig);
}

void Passcode::append(char inputChar)
{
  // check if character is a digit
  if (!(inputChar >= '0' && inputChar <= '9'))
  {
    ESP_LOGI(TAG, "Invalid char, '%c'.", inputChar);
    return;
  }

  // check if input passcode is full
  if (m_inputPos == PASSCODE_LENGTH)
  {
    ESP_LOGI(TAG, "Input full.");
    return;
  }

  if (m_pinsEnabled)
  {
    // turn on led at this position
    gpio_num_t ledPin = m_inputIndicatorPins[m_inputPos];
    gpio_set_level(ledPin, 1);

    inputBeep();
  }

  // add character to passcode and increase it's length
  m_input[m_inputPos++] = inputChar;

  // print passcode
  print();
}

void Passcode::pop()
{
  // check that the input isn't empty
  if (m_inputPos)
  {
    // go back one character
    --m_inputPos;

    // set this position's value to a null char
    m_input[m_inputPos] = '\0';

    if (m_pinsEnabled)
    {
      // turn off the led at this position
      gpio_num_t ledPin = m_inputIndicatorPins[m_inputPos];
      gpio_set_level(ledPin, 0);

      inputBeep();
    }

    // print passcode
    print();
  }
}

void Passcode::clear()
{
  // check that the input isn't empty
  while (m_inputPos)
  {
    {
      // go back one character
      --m_inputPos;

      // set this position's value to a null char
      m_input[m_inputPos] = '\0';

      if (m_pinsEnabled)
      {
        // turn off the led at this position
        gpio_num_t ledPin = m_inputIndicatorPins[m_inputPos];
        gpio_set_level(ledPin, 0);
      }
    }
  }
}

PasscodeError Passcode::validate()
{
  // first ensure the passcode is the right length
  if (m_inputPos != PASSCODE_LENGTH)
  {
    ESP_LOGI(TAG, "Input not complete.");
    return PasscodeError::INCOMPLETE;
  }

  size_t secretLength = PASSCODE_LENGTH + 1; // account for the null character
  char secret[secretLength];

  // get secret passcode while ensuring no errors
  ESP_ERROR_CHECK(nvs_get_str(m_nvsHandle, PASSCODE_SECRET_KEY, secret, &secretLength));

  // log the secret passcode for verification
  ESP_LOGD(TAG, "secret = %s", secret);

  // validate input
  for (int i = 0; i < PASSCODE_LENGTH; i++)
  {
    if (m_input[i] != secret[i])
    {
      // clear input
      clear();

      return PasscodeError::INVALID;
    }
  }

  // clear input
  clear();
  return PasscodeError::VALID;
}

PasscodeError Passcode::handleInput(char inputChar)
{
  // check if the passcode is locked from further tries
  if (m_isLocked)
  {
    ESP_LOGI(TAG, "Passcode has to be reset before any more tries.");
    return PasscodeError::REQUIRE_RESET;
  }

  // cooldown started but isn't over yet
  else if (m_cooldownTimer > 0 && esp_timer_get_time() - m_cooldownTimer < m_cooldown)
  {
    uint8_t secondsLeft = (m_cooldown + m_cooldownTimer - esp_timer_get_time()) / (1 * 1000 * 1000);
    ESP_LOGI(TAG, "Try again after %u seconds.", secondsLeft);
    clear();
    return PasscodeError::COOLDOWN;
  }

  PasscodeError err;

  // pop from the passcode
  if (inputChar == m_popChar)
  {
    pop();
    return PasscodeError::OK;
  }

  // validate the passcode
  else if (inputChar == m_validateChar)
  {

    // cooldown is over. allow one last try
    if (m_cooldownTimer > 0)
    {
      err = validate();
      if (err == PasscodeError::INVALID)
      {
        m_isLocked = true;
        ESP_LOGI(TAG, "All tries have been exhausted. The passcode is now locked from further input.");
        return err;
      }
      else if (err == PasscodeError::VALID)
      {
        onValid();
        return err;
      }
      return err;
    }

    err = validate();
    if (err == PasscodeError::INVALID)
    {
      onInvalid();
      return err;
    }

    else if (err == PasscodeError::VALID)
    {
      onValid();
      return err;
    }

    return err; // when incomplete
  }

  // append to the passcode
  else
  {
    append(inputChar);
    return PasscodeError::OK;
  }

  //? never reaches here but the compiler complains with [-Werror=return-type]
  return PasscodeError::OK;
}

void Passcode::onValid()
{
  ESP_LOGI(TAG, "Passcode valid.");

  // reset the countdown
  m_cooldownTimer = 0;

  // reset the number of incorrect attempts
  m_incorrectAttempts = 0;

  if (m_pinsEnabled)
  {
    validBeep();
  }
}

void Passcode::onInvalid()
{
  // increment failed attempts count and check for max incorrect
  if (++m_incorrectAttempts == PASSCODE_MAX_INCORRECT_ATTEMPTS)
  {
    // start cooldown timer
    m_cooldownTimer = esp_timer_get_time();

    // fade locked led
    ledc_set_duty(LOCK_SPEED_MODE, LOCK_CHANNEL, 1000);
    ledc_update_duty(LOCK_SPEED_MODE, LOCK_CHANNEL);

    ledc_set_fade_time_and_start(LOCK_SPEED_MODE, LOCK_CHANNEL, 0, m_cooldown / 1000, LEDC_FADE_NO_WAIT);
  };

  if (m_pinsEnabled)
  {
    invalidBeep();
  }

  ESP_LOGI(TAG, "Passcode wrong. You have %d tries left.", PASSCODE_MAX_INCORRECT_ATTEMPTS - m_incorrectAttempts);
}

esp_err_t Passcode::setSecret(char const *newSecret)
{
  esp_err_t err;

  size_t secretLen = strlen(newSecret);

  // ensure secret passcode is the right size
  if (secretLen != PASSCODE_LENGTH)
  {
    ESP_LOGE(TAG, "Passcode must be %d digits.");
    return ESP_FAIL;
  }

  // ensure secret passcode consists of only numbers
  for (size_t i = 0; i < secretLen; i++)
  {
    if (newSecret[i] < '0' || newSecret[i] > '9')
    {
      ESP_LOGE(TAG, "Passcode can only contain digits.");
      return ESP_FAIL;
    }
  }

  // store passcode
  ESP_LOGV(TAG, "Writing string to NVS...");

  // write new secret passcode to nvs
  err = nvs_set_str(m_nvsHandle, PASSCODE_SECRET_KEY, newSecret);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to write passcode's!");
    return ESP_FAIL;
  }

  // Commit changes
  // After setting any values, nvs_commit() must be called to ensure changes are written
  // to flash storage. Implementations may write to storage at other times,
  // but this is not guaranteed.
  ESP_LOGV(TAG, "Committing updates in NVS...");
  err = nvs_commit(m_nvsHandle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to commit NVS changes!");
  }
  ESP_LOGV(TAG, "NVS handle closed.");

  return ESP_OK;
}

void Passcode::print()
{
  ESP_LOGD(TAG, "Input(%d): %s", m_inputPos, m_input);
}

void _beep(uint32_t freq, uint32_t duration)
{
  uint32_t duty = (1 << (BUZZER_DUTY_RESOLUTION - 1)); // set to the maximum duty cycle

  // start the buzzer
  ledc_set_freq(BUZZER_SPEED_MODE, BUZZER_TIMER, freq);
  ledc_set_duty(BUZZER_SPEED_MODE, BUZZER_CHANNEL, duty);
  ledc_update_duty(BUZZER_SPEED_MODE, BUZZER_CHANNEL);

  vTaskDelay(duration / portTICK_PERIOD_MS);

  // bring it back low
  ledc_set_duty(BUZZER_SPEED_MODE, BUZZER_CHANNEL, 0);
  ledc_update_duty(BUZZER_SPEED_MODE, BUZZER_CHANNEL);
}

void Passcode::inputBeep()
{
  // 800Hz for 100ms
  _beep(800, 100);
}

void Passcode::validBeep()
{
  // 2kHz for 500ms
  _beep(2000, 500);
}

void Passcode::invalidBeep()
{
  // 400Hz for 100ms
  _beep(440, 200);

  // 350Hz for 100ms
  _beep(200, 300);
}

void Passcode::blink()
{
  uint32_t lockDuty = 1 << (LOCK_DUTY_RESOLUTION - 1);
  uint32_t buzzerDuty = 1 << (BUZZER_DUTY_RESOLUTION - 1);

  while (true)
  {
    if (m_isLocked)
    {
      // ring an alarm
      ledc_set_freq(BUZZER_SPEED_MODE, BUZZER_TIMER, BUZZER_FREQUENCY);
      ledc_set_duty_and_update(BUZZER_SPEED_MODE, BUZZER_CHANNEL, buzzerDuty, 0);

      ledc_set_duty(LOCK_SPEED_MODE, LOCK_CHANNEL, lockDuty);
      ledc_update_duty(LOCK_SPEED_MODE, LOCK_CHANNEL);

      vTaskDelay(500 / portTICK_PERIOD_MS);

      ledc_set_duty(LOCK_SPEED_MODE, LOCK_CHANNEL, 0);
      ledc_update_duty(LOCK_SPEED_MODE, LOCK_CHANNEL);

      vTaskDelay(500 / portTICK_PERIOD_MS);
    }
  }
}

void Passcode::blinkTask(void *pvParameters)
{
  auto *instance = static_cast<Passcode *>(pvParameters);
  instance->blink();

  // safeguard
  vTaskDelete(nullptr);
}