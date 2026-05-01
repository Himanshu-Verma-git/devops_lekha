idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES button led audio storage wifi mqtt task_manager logger time_sync
                             esp_driver_gpio nvs_flash esp_timer driver esp_system)
