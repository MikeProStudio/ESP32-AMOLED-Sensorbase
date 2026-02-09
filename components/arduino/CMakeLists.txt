set(arduino_dir "C:/Users/mneum/.platformio/packages/framework-arduinoespressif32")

idf_component_register(SRC_DIRS "${arduino_dir}/cores/esp32"
                      INCLUDE_DIRS "${arduino_dir}/cores/esp32"
                                   "${arduino_dir}/variants/esp32s3" # Oder dein spezifisches Variant
                      REQUIRES esp_adc_cal esp_timer driver)