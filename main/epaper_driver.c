#include "epaper_driver.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

// WeAct 2.13" black/white panel controller RAM
#define EPD_RAW_WIDTH        122
#define EPD_RAW_HEIGHT       250
#define EPD_RAW_WIDTH_BYTES  ((EPD_RAW_WIDTH + 7) / 8)
#define EPD_BUF_SIZE         (EPD_RAW_WIDTH_BYTES * EPD_RAW_HEIGHT)

static const char *TAG = "EPD_DRIVER";
static uint8_t epd_fb[EPD_BUF_SIZE];

// Simple 5x7 font for ASCII 0x20..0x7F
static const uint8_t font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x5F,0x00,0x00},{0x00,0x07,0x00,0x07,0x00},{0x14,0x7F,0x14,0x7F,0x14},
    {0x24,0x2A,0x7F,0x2A,0x12},{0x23,0x13,0x08,0x64,0x62},{0x36,0x49,0x56,0x20,0x50},{0x00,0x08,0x07,0x03,0x00},
    {0x00,0x1C,0x22,0x41,0x00},{0x00,0x41,0x22,0x1C,0x00},{0x2A,0x1C,0x7F,0x1C,0x2A},{0x08,0x08,0x3E,0x08,0x08},
    {0x00,0x80,0x70,0x30,0x00},{0x08,0x08,0x08,0x08,0x08},{0x00,0x00,0x60,0x60,0x00},{0x20,0x10,0x08,0x04,0x02},
    {0x3E,0x51,0x49,0x45,0x3E},{0x00,0x42,0x7F,0x40,0x00},{0x72,0x49,0x49,0x49,0x46},{0x21,0x41,0x49,0x4D,0x33},
    {0x18,0x14,0x12,0x7F,0x10},{0x27,0x45,0x45,0x45,0x39},{0x3C,0x4A,0x49,0x49,0x31},{0x41,0x21,0x11,0x09,0x07},
    {0x36,0x49,0x49,0x49,0x36},{0x46,0x49,0x49,0x29,0x1E},{0x00,0x00,0x14,0x00,0x00},{0x00,0x40,0x34,0x00,0x00},
    {0x00,0x08,0x14,0x22,0x41},{0x14,0x14,0x14,0x14,0x14},{0x41,0x22,0x14,0x08,0x00},{0x02,0x01,0x59,0x09,0x06},
    {0x3E,0x41,0x5D,0x59,0x4E},{0x7C,0x12,0x11,0x12,0x7C},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},
    {0x7F,0x41,0x41,0x41,0x3E},{0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x09,0x01},{0x3E,0x41,0x41,0x51,0x73},
    {0x7F,0x08,0x08,0x08,0x7F},{0x00,0x41,0x7F,0x41,0x00},{0x20,0x40,0x41,0x3F,0x01},{0x7F,0x08,0x14,0x22,0x41},
    {0x7F,0x40,0x40,0x40,0x40},{0x7F,0x02,0x1C,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},
    {0x7F,0x09,0x09,0x09,0x06},{0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},{0x26,0x49,0x49,0x49,0x32},
    {0x03,0x01,0x7F,0x01,0x03},{0x3F,0x40,0x40,0x40,0x3F},{0x1F,0x20,0x40,0x20,0x1F},{0x3F,0x40,0x38,0x40,0x3F},
    {0x63,0x14,0x08,0x14,0x63},{0x03,0x04,0x78,0x04,0x03},{0x61,0x59,0x49,0x4D,0x43},{0x00,0x7F,0x41,0x41,0x41},
    {0x02,0x04,0x08,0x10,0x20},{0x00,0x41,0x41,0x41,0x7F},{0x04,0x02,0x01,0x02,0x04},{0x40,0x40,0x40,0x40,0x40},
    {0x00,0x03,0x07,0x08,0x00},{0x20,0x54,0x54,0x78,0x40},{0x7F,0x28,0x44,0x44,0x38},{0x38,0x44,0x44,0x44,0x28},
    {0x38,0x44,0x44,0x28,0x7F},{0x38,0x54,0x54,0x54,0x18},{0x00,0x08,0x7E,0x09,0x02},{0x18,0xA4,0xA4,0x9C,0x78},
    {0x7F,0x08,0x04,0x04,0x78},{0x00,0x44,0x7D,0x40,0x00},{0x20,0x40,0x40,0x3D,0x00},{0x7F,0x10,0x28,0x44,0x00},
    {0x00,0x41,0x7F,0x40,0x00},{0x7C,0x04,0x78,0x04,0x78},{0x7C,0x08,0x04,0x04,0x78},{0x38,0x44,0x44,0x44,0x38},
    {0xFC,0x18,0x24,0x24,0x18},{0x18,0x24,0x24,0x18,0xFC},{0x7C,0x08,0x04,0x04,0x08},{0x48,0x54,0x54,0x54,0x24},
    {0x04,0x04,0x3F,0x44,0x24},{0x3C,0x40,0x40,0x20,0x7C},{0x1C,0x20,0x40,0x20,0x1C},{0x3C,0x40,0x30,0x40,0x3C},
    {0x44,0x28,0x10,0x28,0x44},{0x4C,0x90,0x90,0x90,0x7C},{0x44,0x64,0x54,0x4C,0x44},{0x00,0x08,0x36,0x41,0x00},
    {0x00,0x00,0x77,0x00,0x00},{0x00,0x41,0x36,0x08,0x00},{0x02,0x01,0x02,0x04,0x02},{0x7F,0x7F,0x7F,0x7F,0x7F}
};

static esp_err_t epd_send(epaper_driver_t *driver, uint8_t dc, const uint8_t *data, size_t len)
{
    gpio_set_level(driver->cfg.pin_dc, dc);
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };
    return spi_device_transmit(driver->spi, &t);
}

static inline esp_err_t epd_cmd(epaper_driver_t *driver, uint8_t cmd)
{
    return epd_send(driver, 0, &cmd, 1);
}

static inline esp_err_t epd_data(epaper_driver_t *driver, const uint8_t *data, size_t len)
{
    return epd_send(driver, 1, data, len);
}

static esp_err_t epd_set_window_raw(epaper_driver_t *driver, uint8_t x_start_byte, uint8_t x_end_byte, uint16_t y_start, uint16_t y_end)
{
    uint8_t x_range[2] = {x_start_byte, x_end_byte};
    uint8_t y_range[4] = {
        y_start & 0xFF,
        (y_start >> 8) & 0xFF,
        y_end & 0xFF,
        (y_end >> 8) & 0xFF
    };

    ESP_ERROR_CHECK(epd_cmd(driver, 0x44));
    ESP_ERROR_CHECK(epd_data(driver, x_range, sizeof(x_range)));
    ESP_ERROR_CHECK(epd_cmd(driver, 0x45));
    ESP_ERROR_CHECK(epd_data(driver, y_range, sizeof(y_range)));
    return ESP_OK;
}

static esp_err_t epd_set_cursor_raw(epaper_driver_t *driver, uint8_t x_byte, uint16_t y)
{
    uint8_t y_bytes[2] = {y & 0xFF, (y >> 8) & 0xFF};
    ESP_ERROR_CHECK(epd_cmd(driver, 0x4E));
    ESP_ERROR_CHECK(epd_data(driver, &x_byte, 1));
    ESP_ERROR_CHECK(epd_cmd(driver, 0x4F));
    ESP_ERROR_CHECK(epd_data(driver, y_bytes, sizeof(y_bytes)));
    return ESP_OK;
}

static void epd_wait_busy(epaper_driver_t *driver)
{
    int timeout_ms = 7000;
    while (gpio_get_level(driver->cfg.pin_busy) == 1 && timeout_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(20));
        timeout_ms -= 20;
    }
    if (timeout_ms <= 0) {
        ESP_LOGW(TAG, "BUSY timeout, continuing anyway");
    }
}

static void epd_hw_reset(epaper_driver_t *driver)
{
    gpio_set_level(driver->cfg.pin_rst, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(driver->cfg.pin_rst, 1);
    vTaskDelay(pdMS_TO_TICKS(20));
}

static esp_err_t epd_set_window_full(epaper_driver_t *driver)
{
    ESP_ERROR_CHECK(epd_set_window_raw(driver, 0x00, EPD_RAW_WIDTH_BYTES - 1, 0, EPD_RAW_HEIGHT - 1));
    ESP_ERROR_CHECK(epd_set_cursor_raw(driver, 0x00, 0));
    return ESP_OK;
}

static esp_err_t epd_write_window_from_fb(epaper_driver_t *driver, uint8_t ram_cmd, uint8_t x_start_byte, uint8_t x_end_byte, uint16_t y_start, uint16_t y_end)
{
    size_t bytes_per_row = (size_t)(x_end_byte - x_start_byte + 1);

    ESP_ERROR_CHECK(epd_set_window_raw(driver, x_start_byte, x_end_byte, y_start, y_end));
    ESP_ERROR_CHECK(epd_set_cursor_raw(driver, x_start_byte, y_start));
    ESP_ERROR_CHECK(epd_cmd(driver, ram_cmd));

    for (uint16_t y = y_start; y <= y_end; y++) {
        const uint8_t *row = &epd_fb[(y * EPD_RAW_WIDTH_BYTES) + x_start_byte];
        ESP_ERROR_CHECK(epd_data(driver, row, bytes_per_row));
    }
    return ESP_OK;
}

static bool epd_rect_to_raw_window(epaper_rect_t rect, uint8_t *x_start_byte, uint8_t *x_end_byte, uint16_t *y_start, uint16_t *y_end)
{
    int x0 = rect.x;
    int y0 = rect.y;
    int x1 = rect.x + rect.width - 1;
    int y1 = rect.y + rect.height - 1;

    if (x1 < 0 || y1 < 0 || x0 >= EPAPER_SCREEN_WIDTH || y0 >= EPAPER_SCREEN_HEIGHT) {
        return false;
    }

    if (x0 < 0) {
        x0 = 0;
    }
    if (y0 < 0) {
        y0 = 0;
    }
    if (x1 >= EPAPER_SCREEN_WIDTH) {
        x1 = EPAPER_SCREEN_WIDTH - 1;
    }
    if (y1 >= EPAPER_SCREEN_HEIGHT) {
        y1 = EPAPER_SCREEN_HEIGHT - 1;
    }

    int raw_x_min = (EPAPER_SCREEN_HEIGHT - 1) - y1;
    int raw_x_max = (EPAPER_SCREEN_HEIGHT - 1) - y0;
    int raw_y_min = x0;
    int raw_y_max = x1;

    *x_start_byte = (uint8_t)(raw_x_min / 8);
    *x_end_byte = (uint8_t)(raw_x_max / 8);
    *y_start = (uint16_t)raw_y_min;
    *y_end = (uint16_t)raw_y_max;
    return true;
}

static void epd_draw_char_scaled(epaper_driver_t *driver, int x, int y, char c, int scale)
{
    if (c < 32 || c > 127) {
        c = '?';
    }
    const uint8_t *glyph = font5x7[(int)c - 32];

    for (int col = 0; col < 5; col++) {
        uint8_t line = glyph[col];
        for (int row = 0; row < 7; row++) {
            uint8_t px_on = (line >> row) & 0x01;
            if (px_on) {
                for (int dx = 0; dx < scale; dx++) {
                    for (int dy = 0; dy < scale; dy++) {
                        epaper_driver_draw_pixel(driver, x + col * scale + dx, y + row * scale + dy, true);
                    }
                }
            }
        }
    }
}

epaper_driver_config_t epaper_driver_default_config(void)
{
    return (epaper_driver_config_t){
        .spi_host = SPI2_HOST,
        .spi_hz = 4000000,
        .pin_mosi = 6,
        .pin_clk = 4,
        .pin_cs = 5,
        .pin_dc = 1,
        .pin_rst = 2,
        .pin_busy = 3,
    };
}

esp_err_t epaper_driver_init(epaper_driver_t *driver, const epaper_driver_config_t *config)
{
    if (driver == NULL || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    memset(driver, 0, sizeof(*driver));
    driver->cfg = *config;

    spi_bus_config_t buscfg = {
        .mosi_io_num = driver->cfg.pin_mosi,
        .miso_io_num = -1,
        .sclk_io_num = driver->cfg.pin_clk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = EPD_BUF_SIZE + 16,
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = driver->cfg.spi_hz,
        .mode = 0,
        .spics_io_num = driver->cfg.pin_cs,
        .queue_size = 1,
    };

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << driver->cfg.pin_dc) | (1ULL << driver->cfg.pin_rst),
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    gpio_config_t busy_conf = {
        .pin_bit_mask = (1ULL << driver->cfg.pin_busy),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&busy_conf));

    ESP_ERROR_CHECK(spi_bus_initialize(driver->cfg.spi_host, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(driver->cfg.spi_host, &devcfg, &driver->spi));

    epd_hw_reset(driver);
    epd_wait_busy(driver);

    uint8_t tmp[3];

    ESP_ERROR_CHECK(epd_cmd(driver, 0x12));
    epd_wait_busy(driver);

    tmp[0] = (EPD_RAW_HEIGHT - 1) & 0xFF;
    tmp[1] = (EPD_RAW_HEIGHT - 1) >> 8;
    tmp[2] = 0x00;
    ESP_ERROR_CHECK(epd_cmd(driver, 0x01));
    ESP_ERROR_CHECK(epd_data(driver, tmp, 3));

    tmp[0] = 0x03;
    ESP_ERROR_CHECK(epd_cmd(driver, 0x11));
    ESP_ERROR_CHECK(epd_data(driver, tmp, 1));

    ESP_ERROR_CHECK(epd_set_window_full(driver));

    tmp[0] = 0x05;
    ESP_ERROR_CHECK(epd_cmd(driver, 0x3C));
    ESP_ERROR_CHECK(epd_data(driver, tmp, 1));

    tmp[0] = 0x80;
    ESP_ERROR_CHECK(epd_cmd(driver, 0x18));
    ESP_ERROR_CHECK(epd_data(driver, tmp, 1));

    return ESP_OK;
}

void epaper_driver_clear(epaper_driver_t *driver, bool white)
{
    (void)driver;
    memset(epd_fb, white ? 0xFF : 0x00, sizeof(epd_fb));
}

void epaper_driver_draw_pixel(epaper_driver_t *driver, int x, int y, bool black)
{
    (void)driver;
    if (x < 0 || x >= EPAPER_SCREEN_WIDTH || y < 0 || y >= EPAPER_SCREEN_HEIGHT) {
        return;
    }

    // Map to panel RAM and flip so text is upright in landscape.
    int xr = (EPAPER_SCREEN_HEIGHT - 1) - y;
    int yr = x;

    size_t idx = (yr * EPD_RAW_WIDTH_BYTES) + (xr / 8);
    uint8_t bit = 0x80 >> (xr & 7);
    if (black) {
        epd_fb[idx] &= ~bit;
    } else {
        epd_fb[idx] |= bit;
    }
}

void epaper_driver_draw_text_scaled(epaper_driver_t *driver, int x, int y, const char *text, int scale)
{
    int char_w = 6 * scale;
    int line_h = 10 * scale;

    while (*text) {
        epd_draw_char_scaled(driver, x, y, *text++, scale);
        x += char_w;
        if (x > (EPAPER_SCREEN_WIDTH - char_w)) {
            x = 0;
            y += line_h;
        }
        if (y > (EPAPER_SCREEN_HEIGHT - (8 * scale))) {
            break;
        }
    }
}

esp_err_t epaper_driver_update_full(epaper_driver_t *driver)
{
    ESP_ERROR_CHECK(epd_write_window_from_fb(driver, 0x24, 0x00, EPD_RAW_WIDTH_BYTES - 1, 0, EPD_RAW_HEIGHT - 1));
    ESP_ERROR_CHECK(epd_write_window_from_fb(driver, 0x26, 0x00, EPD_RAW_WIDTH_BYTES - 1, 0, EPD_RAW_HEIGHT - 1));

    uint8_t update = 0xF7;
    ESP_ERROR_CHECK(epd_cmd(driver, 0x22));
    ESP_ERROR_CHECK(epd_data(driver, &update, 1));
    ESP_ERROR_CHECK(epd_cmd(driver, 0x20));
    epd_wait_busy(driver);
    return ESP_OK;
}

esp_err_t epaper_partial_refresh_init(epaper_partial_refresh_t *partial, epaper_driver_t *driver, int x, int y, int width, int height)
{
    if (partial == NULL || driver == NULL || width <= 0 || height <= 0) {
        return ESP_ERR_INVALID_ARG;
    }

    partial->driver = driver;
    partial->area.x = x;
    partial->area.y = y;
    partial->area.width = width;
    partial->area.height = height;
    return ESP_OK;
}

esp_err_t epaper_partial_refresh_update(epaper_partial_refresh_t *partial)
{
    if (partial == NULL || partial->driver == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t x_start_byte = 0;
    uint8_t x_end_byte = 0;
    uint16_t y_start = 0;
    uint16_t y_end = 0;

    if (!epd_rect_to_raw_window(partial->area, &x_start_byte, &x_end_byte, &y_start, &y_end)) {
        return ESP_OK;
    }

    ESP_ERROR_CHECK(epd_write_window_from_fb(partial->driver, 0x24, x_start_byte, x_end_byte, y_start, y_end));
    ESP_ERROR_CHECK(epd_write_window_from_fb(partial->driver, 0x26, x_start_byte, x_end_byte, y_start, y_end));

    // 0xFF is commonly used partial update control value for this controller family.
    uint8_t update = 0xFF;
    ESP_ERROR_CHECK(epd_cmd(partial->driver, 0x22));
    ESP_ERROR_CHECK(epd_data(partial->driver, &update, 1));
    ESP_ERROR_CHECK(epd_cmd(partial->driver, 0x20));
    epd_wait_busy(partial->driver);
    return ESP_OK;
}
