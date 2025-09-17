idf_component_register(
  SRCS "keypad_common.cpp"
  INCLUDE_DIRS "include"
  REQUIRES driver freertos esp_timer
)
