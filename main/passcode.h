#pragma once

/* -------------------------------- INCLUDES -------------------------------- */
#include <freertos/FreeRTOS.h>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_timer.h>
#include <esp_log.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <array>

/* --------------------------------- DEFINES -------------------------------- */
#define PASSCODE_LENGTH 4
#define PASSCODE_SECRET_KEY "secretPasscode"
#define PASSCODE_MAX_INCORRECT_ATTEMPTS 3

#define SPEED_MODE LEDC_HIGH_SPEED_MODE
#define DUTY_RESOLUTION LEDC_TIMER_10_BIT
#define TIMER LEDC_TIMER_0
#define CLK_CFG LEDC_AUTO_CLK
#define INTR_TYPE LEDC_INTR_DISABLE
#define CHANNEL LEDC_CHANNEL_0
#define FREQUENCY 4000 // 4 kHz

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
  Passcode(std::array<gpio_num_t, PASSCODE_LENGTH> inputIndicatorPins, gpio_num_t lockIndicatorPin, gpio_num_t buzzerPin);
  ~Passcode();

  PasscodeError handleInput(char inputChar);

  esp_err_t setSecret(char const *newSecret);

  void print();

private:
  nvs_handle_t m_nvsHandle;

  std::array<gpio_num_t, PASSCODE_LENGTH> m_inputIndicatorPins;
  gpio_num_t m_lockIndicatorPin;
  gpio_num_t m_buzzerPin;
  bool m_pinsEnabled{false};

  char m_input[PASSCODE_LENGTH]{'\0'};
  size_t m_inputPos{0};

  /* cooloff period after max incorrect attempts */
  uint64_t m_cooldown{60 * 1000 * 1000}; // 1 minute
  uint64_t m_cooldownTimer{0};

  // keeps track of the number of incorrect attempts so far
  uint8_t m_incorrectAttempts{0};

  // defines whether the passcode can accept validations
  bool m_isLocked{false};

  /* character that triggers a pop */
  char m_popChar{'*'};

  // character that triggers a validate
  char m_validateChar{'#'};

private:
  /* --------------------------- constructor helpers -------------------------- */
  void initNvs();
  void initPins();

private:
  /* ----------------------------- business logic ----------------------------- */
  void append(char inputChar);
  void pop();
  void clear();
  PasscodeError validate();
  void onValid();
  void onInvalid();
  void inputBeep();
  void validBeep();
  void invalidBeep();
};
