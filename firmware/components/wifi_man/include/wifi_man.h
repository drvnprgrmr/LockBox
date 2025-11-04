#pragma once

#include <optional>
#include <array>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
//
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_wifi_netif.h"
//
#include "nvs_flash.h"

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

// SAE?
#if CONFIG_WIFI_STA_WPA3_SAE_PWE_HUNT_AND_PECK
#define WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define EXAMPLE_H2E_IDENTIFIER ""
#elif CONFIG_WIFI_STA_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define EXAMPLE_H2E_IDENTIFIER CONFIG_WIFI_STA_WPA3_PASSWORD_ID
#elif CONFIG_WIFI_STA_WPA3_SAE_PWE_BOTH
#define WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER CONFIG_WIFI_STA_WPA3_PASSWORD_ID
#endif

// Configure auth mode threshold
#if CONFIG_WIFI_STA_AUTH_OPEN
#define WIFI_STA_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_WIFI_STA_AUTH_WEP
#define WIFI_STA_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_WIFI_STA_AUTH_WPA_PSK
#define WIFI_STA_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_WIFI_STA_AUTH_WPA2_PSK
#define WIFI_STA_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_WIFI_STA_AUTH_WPA_WPA2_PSK
#define WIFI_STA_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_WIFI_STA_AUTH_WPA3_PSK
#define WIFI_STA_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_WIFI_STA_AUTH_WPA2_WPA3_PSK
#define WIFI_STA_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_WIFI_STA_AUTH_WAPI_PSK
#define WIFI_STA_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

enum class WifiMode
{
  STA,
  AP
};

typedef struct
{
  uint8_t ssid[32] = CONFIG_WIFI_STA_SSID;
  uint8_t password[64] = CONFIG_WIFI_STA_PASSWORD;
} WifiStaConf;

typedef struct
{
  uint8_t ssid[32];
  uint8_t password[64];
} WifiApConf;


typedef struct
{
  WifiMode mode = WifiMode::STA;
  std::optional<char const *> hostname;

  WifiStaConf sta;
  std::optional<WifiApConf> ap;
} WifiConf;

class Wifi
{
public:
  Wifi(WifiConf const &conf);

private:
  int staRetryMax = 10;
  int staRetryNum = 0;

private:
  static void sEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
  void eventHandler(esp_event_base_t event_base, int32_t event_id, void *event_data);
};
