idf_component_register(
    SRCS "makerfabs_esp32_s3_parallel_tft_with_touch_3-5inch.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "priv_include"
    REQUIRES driver
    PRIV_REQUIRES esp_timer esp_lcd esp_lcd_touch
)
