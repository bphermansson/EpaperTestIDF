set(EPAPER_REQS freertos)

# ESP-IDF v5+: split driver components
if(TARGET idf::esp_driver_spi)
    list(APPEND EPAPER_REQS esp_driver_spi)
else()
    # Older ESP-IDF: monolithic driver component
    list(APPEND EPAPER_REQS driver)
endif()

if(TARGET idf::esp_driver_gpio)
    list(APPEND EPAPER_REQS esp_driver_gpio)
endif()

idf_component_register(SRCS "epaper_driver.c"
                    INCLUDE_DIRS "include"
                    REQUIRES ${EPAPER_REQS})
