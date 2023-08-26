// based on https://github.com/sukesh-ak/LVGL8-WT32-SC01-IDF
// adapted for MakerFabs parallel TFT dev boards (S2 and S3)

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "esp_log.h"

#define TAG "Demo"

// these do not have to be the same, not sure what is optimal
#define TASK_SLEEP_PERIOD_MS 4
#define LV_TICK_PERIOD_MS 4

#define LGFX_USE_V1

#define LV_DOUBLE_BUFFER
#define LANDSCAPE // if changing this, make sure to uncheck landscape in menuconfig -> components -> lvgl -> demos -> music

#if CONFIG_IDF_TARGET_ESP32S3
  #include "MakerFabs_Parallel_S3.hpp"
  // if you get "will not fit, dram segment overflow" reduce this
  #ifdef LANDSCAPE
    #define LV_BUFFER_SIZE 80 /* if not double buffering, then buf will be 2x this */
  #else
    #define LV_BUFFER_SIZE 120 /* if not double buffering, then buf will be 2x this */
  #endif
#endif

// Uncomment to test benchmark speed without display refresh. You won't see any output on screen, look in the log window to see results
//#define DISABLE_FLUSH_DURING_BENCHMARK

#if defined(DISABLE_FLUSH_DURING_BENCHMARK) && !CONFIG_LV_USE_LOG
#error You'll need to enable LVGL logging (and probably set log to printf) in the menuconfig to get results.
#endif

static LGFX lcd;

#include <lvgl.h>
#include <demos/lv_demos.h>

/*** Setup screen resolution for LVGL ***/
#ifdef LANDSCAPE
static const uint16_t screenWidth = 480;
static const uint16_t screenHeight = 320;
#else
static const uint16_t screenWidth = 320;
static const uint16_t screenHeight = 480;
#endif
static lv_disp_draw_buf_t draw_buf;

#ifdef LV_DOUBLE_BUFFER
static lv_color_t buf[screenWidth * LV_BUFFER_SIZE];
static lv_color_t buf2[screenWidth * LV_BUFFER_SIZE];
#else
static lv_color_t buf[screenWidth * LV_BUFFER_SIZE * 2];
#endif

typedef void (*function_pointer_t)(void);

static bool disable_flush = false;

/*** Function declaration ***/
static void display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
static void touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);
static void lv_tick_task(void *arg);

void init_lvgl_lgfx()
{
    lcd.init();
    lv_init();

#ifdef LANDSCAPE
    // Rotate to landscape
    lcd.setRotation(1);
#endif

    //lcd.setBrightness(10);

#ifdef LV_DOUBLE_BUFFER
    lv_disp_draw_buf_init(&draw_buf, buf, buf2, screenWidth * LV_BUFFER_SIZE);
#else
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * LV_BUFFER_SIZE * 2);
#endif

    /*** LVGL : Setup & Initialize the display device driver ***/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = display_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /*** LVGL : Setup & Initialize the input device driver ***/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lv_indev_drv_register(&indev_drv);

    /* Create and start a periodic timer interrupt to call lv_tick_inc */
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "periodic_gui"};
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));
}

/*** Display callback to flush the buffer to screen ***/
static void display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
#ifdef DISABLE_FLUSH_DURING_BENCHMARK
    if (disable_flush)
    {
        lv_disp_flush_ready(disp);
        return;
    }
#endif
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    lcd.startWrite();
    lcd.setAddrWindow(area->x1, area->y1, w, h);
    lcd.writePixels((uint16_t *)&color_p->full, w * h, true);
    lcd.endWrite();

    lv_disp_flush_ready(disp);
}

/*** Touchpad callback to read the touchpad ***/
static void touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    uint16_t touchX, touchY;
    bool touched = lcd.getTouch(&touchX, &touchY);

    if (!touched)
    {
        data->state = LV_INDEV_STATE_REL;
    }
    else
    {
        data->state = LV_INDEV_STATE_PR;

        /*Set the coordinates*/
        data->point.x = touchX;
        data->point.y = touchY;
    }
}

/* Setting up tick task for lvgl */
static void lv_tick_task(void *arg)
{
    (void)arg;
    lv_tick_inc(LV_TICK_PERIOD_MS);
}



