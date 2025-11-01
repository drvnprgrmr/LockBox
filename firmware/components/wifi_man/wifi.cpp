#include "wifi_man.h"

Wifi::Wifi(WifiMode mode)
{
  /* -------------------------------- Init NVS -------------------------------- */
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

  // Initialize TCP/IP network interface (only call once in application)
  // Must be called prior to initializing the network driver!
  ESP_ERROR_CHECK(esp_netif_init());

  // Create default event loop that runs in the background
  // Must be running prior to initializing the network driver!
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  // configure wifi 
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  // register event handlers
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &sEventHandler, this, nullptr));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &sEventHandler, this, nullptr));

  if (mode == WifiMode::STA)
  {
    ESP_LOGI(TAG, "Starting WiFi in station mode...");

    s_wifi_event_group = xEventGroupCreate();

    esp_netif_create_default_wifi_sta();

    wifi_config_t wifi_config = {
      .sta = {
        .ssid = "q",
      }
    };

  }
  else if (mode == WifiMode::AP)
  {
    ESP_LOGI(TAG, "Starting WiFi in AP mode...");

    esp_netif_create_default_wifi_ap();
  }
}

void Wifi::sEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  // cast the arg back to the wifi instance
  Wifi *wifi = static_cast<Wifi *>(arg);

  // pass control to member function
  wifi->eventHandler(event_base, event_id, event_data);
}

void Wifi::eventHandler(esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  if (event_base == WIFI_EVENT)
  {
    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
      esp_wifi_connect();
      break;

    case WIFI_EVENT_STA_DISCONNECTED:
      if (staRetryNum++ < staRetryMax)
      {
        esp_wifi_connect();
        ESP_LOGI(TAG, "Retrying...");
      }
      else
      {
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
      }

      break;
    case WIFI_EVENT_STA_CONNECTED:
      // reset the number of retries
      staRetryNum = 0;

      xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
      break;

      /* ----------------------------------- AP ----------------------------------- */
    case WIFI_EVENT_AP_STACONNECTED:
      wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;

      // Log info about connected station
      ESP_LOGI(TAG, "Station connected. mac =" MACSTR ", AID = %d", MAC2STR(event->mac), event->aid);
      break;

    case WIFI_EVENT_AP_STADISCONNECTED:
      wifi_event_ap_stadisconnected_t *disconnect_event = (wifi_event_ap_stadisconnected_t *)event_data;

      // Log info about connected station
      ESP_LOGI(TAG, "Station disconnected. mac =" MACSTR ", AID = %d, reason= %d", MAC2STR(disconnect_event->mac), disconnect_event->aid, disconnect_event->reason);

      break;
    default:
      break;
    }
  }
  else if (event_base == IP_EVENT)
  {
    switch (event_id)
    {
    case IP_EVENT_STA_GOT_IP:
      ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
      ESP_LOGI(TAG, "IP Address: " IPSTR, IP2STR(&(event->ip_info.ip)));
      break;

    default:
      break;
    }
  }
}
