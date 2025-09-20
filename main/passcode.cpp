#include "passcode.h"

static char const *const TAG = "passcode";

/* -------------------------------------------------------------------------- */

Passcode::Passcode()
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

Passcode::~Passcode()
{
  // Close NVS Handle
  nvs_close(m_nvsHandle);
  ESP_LOGV(TAG, "NVS handle closed.");
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

    // check if the passcode is locked from further tries
    if (m_isLocked)
    {
      ESP_LOGI(TAG, "Passcode has to be reset before any more tries.");
      clear();
      return PasscodeError::REQUIRE_RESET;
    }

    // check if the cooldown was started
    if (m_cooldownTimer > 0)
    {
      // cooldown isn't over yet
      if (esp_timer_get_time() - m_cooldownTimer < m_cooldown)
      {
        uint8_t secondsLeft = (m_cooldown + m_cooldownTimer - esp_timer_get_time()) / (1 * 1000 * 1000);
        ESP_LOGI(TAG, "Try again after %u seconds.", secondsLeft);
        clear();
        return PasscodeError::COOLDOWN;
      }

      // cooldown is over. allow one last try
      else
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
}

void Passcode::onInvalid()
{
  // increment failed attempts count and check for max incorrect
  if (++m_incorrectAttempts == PASSCODE_MAX_INCORRECT_ATTEMPTS)
  {
    // start cooldown timer
    m_cooldownTimer = esp_timer_get_time();
  };

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