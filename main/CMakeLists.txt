idf_component_register(SRCS "main.cpp"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES
                        nvs_flash
                        usb
                        esp_ringbuf
                        esp_psram    # Required for CONFIG_SPIRAM
                        esp_timer
                        esp_wifi
                        esp_driver_gpio
                        esp32-camera
                        tinyusb
                        esp_driver_i2c
                        esp_driver_ledc
                        esp_driver_spi
                    )
