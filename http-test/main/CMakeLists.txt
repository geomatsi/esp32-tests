idf_component_register(SRCS "main.c" "http.c" "heartbeat.c"
                    INCLUDE_DIRS ".")

spiffs_create_partition_image(storage ../spiffs_image FLASH_IN_PROJECT)
