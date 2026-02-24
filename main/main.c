#include "esp_log.h"
#include "epaper_driver.h"

#define TEXT_SCALE           3

static const char *TAG = "EPD_TEST";
static epaper_driver_t epaper;

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing e-paper");
    epaper_driver_config_t cfg = epaper_driver_default_config();
    ESP_ERROR_CHECK(epaper_driver_init(&epaper, &cfg));

    epaper_driver_clear(&epaper, true);
    epaper_driver_draw_text_scaled(&epaper, 8, 10, "ESP32-S3", TEXT_SCALE);
    epaper_driver_draw_text_scaled(&epaper, 8, 46, "WeAct 2.13", TEXT_SCALE);
    epaper_driver_draw_text_scaled(&epaper, 8, 82, "Hello world!", TEXT_SCALE);

    ESP_LOGI(TAG, "Updating display");
    ESP_ERROR_CHECK(epaper_driver_update_full(&epaper));
    ESP_LOGI(TAG, "Done");
}
