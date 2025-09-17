#pragma once
#include "sdkconfig.h"

#if CONFIG_KEYPAD_ENABLE_LOGGING
#define LOG_LOCAL_LEVEL CONFIG_KEYPAD_LOG_LEVEL
#include "esp_log.h"
#define KEYPAD_TAG "keypad"
#else
#define KEYPAD_TAG "keypad"
#define ESP_LOGE(tag, fmt, ...) (void)0
#define ESP_LOGW(tag, fmt, ...) (void)0
#define ESP_LOGI(tag, fmt, ...) (void)0
#define ESP_LOGD(tag, fmt, ...) (void)0
#define ESP_LOGV(tag, fmt, ...) (void)0
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/** @brief Maximum keys stored in pressed/held buffers */
#define KEYPAD_MAX_KEY_BUFFER_SIZE 10

/** @brief Default debounce time (µs) */
#define KEYPAD_DEFAULT_DEBOUNCE (10 * 1000)

/** @brief Default hold time (µs) */
#define KEYPAD_DEFAULT_HOLD (500 * 1000)

#ifdef __cplusplus
}
#endif
