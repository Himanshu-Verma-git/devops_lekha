idf_component_register(SRCS "sd_card.c"
                        INCLUDE_DIRS "include"
                        REQUIRES fatfs esp_driver_sdmmc esp_driver_sdspi driver) 
