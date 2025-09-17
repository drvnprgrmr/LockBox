#include "passcode.h"

static char const *const TAG = "passcode";

static char const popChar = '*';
static char const validateChar = '#';

char inputPasscode[MAX_PASSCODE_LENGTH]{'\0'};
int inputPasscodeLength = 0;

nvs_handle_t my_handle;

char const *const secretPasscodeKey = "secretPasscode";

// reset the passcode stored in nvs
esp_err_t setSecretPasscode(char newSecretPasscode[MAX_PASSCODE_LENGTH])
{
  esp_err_t err;

  // store passcode
  ESP_LOGI(TAG, "Writing string to NVS...");

  // write secret passcode hash to nvs
  err = nvs_set_str(my_handle, secretPasscodeKey, newSecretPasscode);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to write secret passcode's hash!");
    return ESP_FAIL;
  }

  return ESP_OK;
}

void handleInput(char chr)
{
  esp_err_t err;
  if (chr == popChar)
  {
    popPasscode();
  }
  else if (chr == validateChar)
  {
    err = validatePasscode();
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
    appendPasscode(chr);
  }
}

void displayPasscode()
{
  esp_rom_printf("input(%d): ", inputPasscodeLength);
  for (size_t i = 0; i < inputPasscodeLength; i++)
  {
    esp_rom_printf("%c", inputPasscode[i]);
  }
  esp_rom_printf("\n");
}

void popPasscode()
{
  // check that the inputPasscode isn't empty
  if (inputPasscodeLength)
  {
    // go back one character
    --inputPasscodeLength;

    // set this position's value to a null char
    inputPasscode[inputPasscodeLength] = '\0';
  }

  // print passcode
  displayPasscode();
}

esp_err_t validatePasscode()
{
  esp_err_t err;

  char secretPasscode[MAX_PASSCODE_LENGTH];

  size_t secretPasscodeLength = 0;

  // get length stored
  err = nvs_get_str(my_handle, secretPasscodeKey, NULL, &secretPasscodeLength);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Error getting length of secret passcode: %s", esp_err_to_name(err));
    return ESP_FAIL;
  }

  // get secret passcode
  ESP_LOGI(TAG, "Reading string from NVS...");
  err = nvs_get_str(my_handle, secretPasscodeKey, secretPasscode, &secretPasscodeLength);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Error reading secret passcode: %s", esp_err_to_name(err));
    return ESP_FAIL;
  }

  ESP_LOGD(TAG, "secret, input: %s (%d), %s", secretPasscode, secretPasscodeLength, inputPasscode);

  for (int i = 0; i < secretPasscodeLength; i++)
  {
    if (inputPasscode[i] != secretPasscode[i])
    {
      return ESP_FAIL;
    }
  }

  return ESP_OK;
}

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
  ESP_LOGI(TAG, "Opening Non-Volatile Storage (NVS) handle...");
  err = nvs_open(TAG, NVS_READWRITE, &m_nvsHandle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
  }
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