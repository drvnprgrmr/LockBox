#pragma once

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

static char const *TAG = "wifi";

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1



enum class WifiMode
{
  STA,
  AP,
};

class Wifi
{
public:
  Wifi(WifiMode mode);


private:
  int staRetryMax = 10;
  int staRetryNum = 0;


private:
  static void sEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
  void eventHandler(esp_event_base_t event_base, int32_t event_id, void *event_data);
};
