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
  ESP_LOGI(TAG, "NVS handle closed.");
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
  if (m_inputPos == MAX_PASSCODE_LENGTH)
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

esp_err_t Passcode::validate()
{
  esp_err_t err;

  char secret[MAX_PASSCODE_LENGTH + 1]; // account for the null character

  size_t secretLength = 0;

  // get length stored
  err = nvs_get_str(m_nvsHandle, m_secretKey, NULL, &secretLength);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Error getting length of secret passcode: %s", esp_err_to_name(err));
    return ESP_FAIL;
  }

  // get secret passcode
  ESP_LOGI(TAG, "Reading string from NVS...");
  err = nvs_get_str(m_nvsHandle, m_secretKey, secret, &secretLength);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Error reading secret passcode: %s", esp_err_to_name(err));
    return ESP_FAIL;
  }

  ESP_LOGD(TAG, "secret, input: %s (%d), %s", secret, secretLength, m_input);

  // clear input
  for (size_t i = 0; i < m_inputPos; i++)
  {
  }
  for (int i = 0; i < secretLength; i++)
  {
    if (m_input[i] != secret[i])
    {
      // clear current input
      clear();
      return ESP_FAIL;
    }
  }

  // clear current input
  clear();
  return ESP_OK;
}

void Passcode::handleInput(char inputChar)
{
  esp_err_t err;

  if (inputChar == m_popChar)
  {
    pop();
  }
  else if (inputChar == m_validateChar)
  {
    err = validate();
    if (err == ESP_FAIL)
    {
      ESP_LOGI(TAG, "Passcode wrong.");
    }
    else if (err == ESP_OK)
    {
      ESP_LOGI(TAG, "Passcode correct.");
    }
  }
  else
  {
    append(inputChar);
  }
}

esp_err_t Passcode::setSecret(char const *newSecret)
{
  esp_err_t err;

  size_t secretLen = strlen(newSecret);

  // ensure secret passcode is the right size
  if (secretLen < MIN_PASSCODE_LENGTH && secretLen > MAX_PASSCODE_LENGTH)
  {
    ESP_LOGE(TAG, "Invalid size of secret passcode.");
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
  ESP_LOGI(TAG, "Writing string to NVS...");

  // write secret passcode hash to nvs
  err = nvs_set_str(m_nvsHandle, m_secretKey, newSecret);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to write secret passcode's hash!");
    return ESP_FAIL;
  }

  // Commit changes
  // After setting any values, nvs_commit() must be called to ensure changes are written
  // to flash storage. Implementations may write to storage at other times,
  // but this is not guaranteed.
  ESP_LOGI(TAG, "\nCommitting updates in NVS...");
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
