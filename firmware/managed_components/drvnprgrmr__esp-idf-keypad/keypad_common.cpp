#include "keypad_config.h"
#include "esp_log.h"

extern "C" void keypad_log_version(void) {
    ESP_LOGI(KEYPAD_TAG, "Keypad driver v1.0.0");
}
