#pragma once

#include <freertos/FreeRTOS.h>
#include <esp_timer.h>
#include <esp_log.h>
#include <esp_random.h>

#include <nvs.h>
#include <nvs_flash.h>

#define MAX_PASSCODE_LENGTH 5

extern char inputPasscode[MAX_PASSCODE_LENGTH];

void initPasscode();

void handleInput(char chr);

esp_err_t setSecretPasscode(char *newSecretPasscode);

void displayPasscode();

void appendPasscode(char chr);

void popPasscode();

esp_err_t validatePasscode();

class Passcode
{
private:
  nvs_handle_t m_nvsHandle;

  char m_input[MAX_PASSCODE_LENGTH]{'\0'};
  size_t m_inputPos{0};

    /* cooloff period after max incorrect attempts */
  uint64_t m_cooloff{60 * 1000 * 1000}; // 1 minute

  uint8_t m_curIncorrectAttempts{0};
  uint8_t m_maxIncorrectAttempts{3};

  /* character that triggers a pop */
  char m_popChar{'*'};

  // character that triggers a validate
  char m_validateChar{'#'};

private:
  void append(char inputChar);
  void pop();
  esp_err_t validate();

public:
  Passcode();

  void handleInput(char inputChar);

  esp_err_t setSecret(char *newSecret);

  void print();
};
