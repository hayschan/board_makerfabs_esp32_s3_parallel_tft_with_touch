/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "esp_timer.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

#include "bsp/makerfabs_esp32_s3_parallel_tft_with_touch_3-5inch.h"
#include "FT6X36.h"
#include "esp_lcd_ili9488.h"
#include "esp_lvgl_port.h"
#include "bsp_err_check.h"

static const char *TAG = "MAKERFABS_3-5INCH";

esp_err_t bsp_i2c_init(void)
{
    const i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BSP_I2C_SDA,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = BSP_I2C_SCL,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = BSP_I2C_CLK_SPEED_HZ
    };
    BSP_ERROR_CHECK_RETURN_ERR(i2c_param_config(BSP_I2C_NUM, &i2c_conf));
    BSP_ERROR_CHECK_RETURN_ERR(i2c_driver_install(BSP_I2C_NUM, i2c_conf.mode, 0, 0, 0));

    return ESP_OK;
}

esp_err_t bsp_i2c_deinit(void)
{
    BSP_ERROR_CHECK_RETURN_ERR(i2c_driver_delete(BSP_I2C_NUM));
    return ESP_OK;
}

// Bit number used to represent command and parameter
#define LCD_CMD_BITS           8
#define LCD_PARAM_BITS         8

static lv_disp_t *bsp_display_lcd_init(void)
{
    ESP_LOGD(TAG, "Initialize Intel 8080 bus");
    /* Init Intel 8080 bus */
    esp_lcd_i80_bus_handle_t i80_bus = NULL;
    esp_lcd_i80_bus_config_t bus_config = {
        .clk_src = LCD_CLK_SRC_PLL160M,
        .dc_gpio_num = BSP_LCD_DC,
        .wr_gpio_num = BSP_LCD_WR,
        .data_gpio_nums = {
            BSP_LCD_DB0,
            BSP_LCD_DB1,
            BSP_LCD_DB2,
            BSP_LCD_DB3,
            BSP_LCD_DB4,
            BSP_LCD_DB5,
            BSP_LCD_DB6,
            BSP_LCD_DB7,
            BSP_LCD_DB8,
            BSP_LCD_DB9,
            BSP_LCD_DB10,
            BSP_LCD_DB11,
            BSP_LCD_DB12,
            BSP_LCD_DB13,
            BSP_LCD_DB14,
            BSP_LCD_DB15,
        },
        .bus_width = BSP_LCD_WIDTH,
        .max_transfer_bytes = (BSP_LCD_H_RES) * 80 * sizeof(uint16_t),
    };
    BSP_ERROR_CHECK_RETURN_NULL(esp_lcd_new_i80_bus(&bus_config, &i80_bus));

    ESP_LOGD(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i80_config_t io_config = {
        .cs_gpio_num = BSP_LCD_CS,
        .pclk_hz = BSP_LCD_PIXEL_CLOCK_HZ,
        .trans_queue_depth = 10,
        .dc_levels = {
            .dc_idle_level = 0,
            .dc_cmd_level = 0,
            .dc_dummy_level = 0,
            .dc_data_level = 1,
        },
        .flags = {
            .swap_color_bytes = 1,
            .pclk_idle_low = 0,
        },
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
    };
    BSP_ERROR_CHECK_RETURN_NULL(esp_lcd_new_panel_io_i80(i80_bus, &io_config, &io_handle));

    ESP_LOGD(TAG, "Install LCD driver of RA8875");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = BSP_LCD_RST,
        .color_space = ESP_LCD_COLOR_SPACE_RGB,
        .bits_per_pixel = 16,
    };
    BSP_ERROR_CHECK_RETURN_NULL(esp_lcd_new_panel_ili9488(io_handle, &panel_config, &panel_handle));

    BSP_ERROR_CHECK_RETURN_NULL(esp_lcd_panel_reset(panel_handle));
    BSP_ERROR_CHECK_RETURN_NULL(esp_lcd_panel_init(panel_handle));
    BSP_ERROR_CHECK_RETURN_NULL(esp_lcd_panel_invert_color(panel_handle, true));
    // the gap is LCD panel specific, even panels with the same driver IC, can have different gap value
    BSP_ERROR_CHECK_RETURN_NULL(esp_lcd_panel_set_gap(panel_handle, 0, 0));

    ESP_LOGI(TAG, "Turn on LCD backlight");
    BSP_ERROR_CHECK_RETURN_NULL(esp_lcd_panel_disp_on_off(panel_handle, true));

    /* Add LCD screen */
    ESP_LOGD(TAG, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = BSP_LCD_H_RES * 20,
        .double_buffer = true,
        .hres = BSP_LCD_H_RES,
        .vres = BSP_LCD_V_RES,
        .monochrome = false,
        /* Rotation values must be same as used in esp_lcd for initial settings of the screen */
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
        }
    };

    return lvgl_port_add_disp(&disp_cfg);
}

static lv_indev_t *bsp_display_indev_init(lv_disp_t *disp)
{
    esp_lcd_touch_handle_t tp;

    /* Initialize touch */
    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = BSP_LCD_H_RES,
        .y_max = BSP_LCD_V_RES,
        .rst_gpio_num = GPIO_NUM_NC, // Shared with LCD reset
        .int_gpio_num = BSP_LCD_TP_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    const esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    BSP_ERROR_CHECK_RETURN_NULL(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)BSP_I2C_NUM, &tp_io_config, &tp_io_handle));
    BSP_ERROR_CHECK_RETURN_NULL(esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &tp));
    assert(tp);

    /* Add touch input (for selected screen) */
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = disp,
        .handle = tp,
    };

    return lvgl_port_add_touch(&touch_cfg);
}

esp_err_t bsp_display_brightness_set(int brightness_percent)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t bsp_display_backlight_off(void)
{
    return bsp_display_brightness_set(0);
}

esp_err_t bsp_display_backlight_on(void)
{
    return bsp_display_brightness_set(100);
}

lv_disp_t *bsp_display_start(void)
{
    lv_disp_t *disp = NULL;
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    BSP_ERROR_CHECK_RETURN_NULL(lvgl_port_init(&lvgl_cfg));
    BSP_NULL_CHECK(disp = bsp_display_lcd_init(), NULL);
    BSP_NULL_CHECK(bsp_display_indev_init(disp), NULL);
    return disp;
}

void bsp_display_rotate(lv_disp_t *disp, lv_disp_rot_t rotation)
{
    lv_disp_set_rotation(disp, rotation);
}

bool bsp_display_lock(uint32_t timeout_ms)
{
    return lvgl_port_lock(timeout_ms);
}

void bsp_display_unlock(void)
{
    lvgl_port_unlock();
}