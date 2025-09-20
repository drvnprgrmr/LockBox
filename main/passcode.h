#pragma once

/* -------------------------------- INCLUDES -------------------------------- */
#include <freertos/FreeRTOS.h>
#include <esp_timer.h>
#include <esp_log.h>
#include <nvs.h>
#include <nvs_flash.h>

/* --------------------------------- DEFINES -------------------------------- */
#define PASSCODE_LENGTH 4
#define PASSCODE_SECRET_KEY "secretPasscode"
#define PASSCODE_MAX_INCORRECT_ATTEMPTS 3

/* -------------------------------------------------------------------------- */
enum class PasscodeError
{
  OK,            // Input added/removed successfully
  FAIL,          // Failed to validate passcode due to internal error. Should be reported.
  INCOMPLETE,    // Passcode input incomplete
  VALID,         // Passcode entered is valid
  INVALID,       // Passcode entered is invalid
  COOLDOWN,      // Too many wrong attempts, try again after cooldown
  REQUIRE_RESET, // Maximum number of failed attempts, needs to be reset by admin
};

/* -------------------------------------------------------------------------- */
class Passcode
{
public:
  Passcode();
  ~Passcode();

  PasscodeError handleInput(char inputChar);

  esp_err_t setSecret(char const *newSecret);

  void print();

private:
  nvs_handle_t m_nvsHandle;

  char m_input[PASSCODE_LENGTH]{'\0'};
  size_t m_inputPos{0};

  char const *m_secretKey{"secretPasscode"};

  /* cooloff period after max incorrect attempts */
  uint64_t m_cooldown{60 * 1000 * 1000}; // 1 minute
  uint64_t m_cooldownTimer{0};

  bool m_isLocked{false};

  uint8_t m_curIncorrectAttempts{0};
  uint8_t m_maxIncorrectAttempts{3};

  /* character that triggers a pop */
  char m_popChar{'*'};

  // character that triggers a validate
  char m_validateChar{'#'};

private:
  void append(char inputChar);
  void pop();
  void clear();
  PasscodeError validate();
  void onValid();
  void onInvalid();
};
