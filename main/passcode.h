#pragma once

#include <freertos/FreeRTOS.h>
#include <esp_timer.h>
#include <esp_log.h>
#include <esp_random.h>
#include <bootloader_random.h>

#include "mbedtls/sha256.h"
#include "mbedtls/pkcs5.h"

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
