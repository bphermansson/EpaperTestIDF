#ifndef EPAPER_DRIVER_H
#define EPAPER_DRIVER_H

#include <stdbool.h>
#include <stdint.h>
#include "driver/spi_master.h"
#include "esp_err.h"

#define EPAPER_SCREEN_WIDTH   250
#define EPAPER_SCREEN_HEIGHT  122

typedef struct {
    spi_host_device_t spi_host;
    int spi_hz;
    int pin_mosi;
    int pin_clk;
    int pin_cs;
    int pin_dc;
    int pin_rst;
    int pin_busy;
} epaper_driver_config_t;

typedef struct {
    spi_device_handle_t spi;
    epaper_driver_config_t cfg;
} epaper_driver_t;

epaper_driver_config_t epaper_driver_default_config(void);
esp_err_t epaper_driver_init(epaper_driver_t *driver, const epaper_driver_config_t *config);
void epaper_driver_clear(epaper_driver_t *driver, bool white);
void epaper_driver_draw_pixel(epaper_driver_t *driver, int x, int y, bool black);
void epaper_driver_draw_text_scaled(epaper_driver_t *driver, int x, int y, const char *text, int scale);
esp_err_t epaper_driver_update_full(epaper_driver_t *driver);

#endif
