#ifdef __cplusplus
extern "C" {
#endif

#include "esp_log.h"
#include "MakerFabs_Parallel_S3.h"
#include "ui/ui.h"
#include "lvgl.h"

#define TAG "MAKERFABS-EXAMPLE"

void app_main(void)
{
    /* Initialize LovyanGFX */
    init_lvgl_lgfx();

    /* Initialize user interface */
    ui_init();

    ESP_LOGI(TAG, "SquareLine Studio example initialization done.");

    /* UI thread */
    while (true)
    {
        lv_timer_handler(); /* let the GUI do its work */
        vTaskDelay(pdMS_TO_TICKS(TASK_SLEEP_PERIOD_MS));
    }
}

#ifdef __cplusplus
}
#endif