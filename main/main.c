#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "epaper_driver.h"

#define TEXT_SCALE           3

static const char *TAG = "EPD_TEST";
static epaper_driver_t epaper;

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing e-paper");
    epaper_driver_config_t cfg = epaper_driver_default_config();
    ESP_ERROR_CHECK(epaper_driver_init(&epaper, &cfg));
    epaper_partial_refresh_t part;
    ESP_ERROR_CHECK(epaper_partial_refresh_init(&part, &epaper, 8, 10, 120, 40));

    epaper_driver_clear(&epaper, true);
    epaper_driver_draw_text_scaled(&epaper, 8, 10, "ESP32-S3", TEXT_SCALE);
    epaper_driver_draw_text_scaled(&epaper, 8, 46, "WeAct 2.13", TEXT_SCALE);
    epaper_driver_draw_text_scaled(&epaper, 8, 82, "Hello world!", TEXT_SCALE);

    ESP_LOGI(TAG, "Updating display");
    ESP_ERROR_CHECK(epaper_driver_update_full(&epaper));

    vTaskDelay(pdMS_TO_TICKS(5 * 1000));
    epaper_driver_draw_text_in_rect(&epaper, part.area.x, part.area.y, part.area.width, part.area.height, "Partial", 2, true);
    ESP_ERROR_CHECK(epaper_partial_refresh_update(&part));
    ESP_LOGI(TAG, "Done");
}
