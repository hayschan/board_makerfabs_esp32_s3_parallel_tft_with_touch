idf_component_register(
    SRCS "MakerFabs_Parallel_S3.cpp"
    INCLUDE_DIRS "include"
    REQUIRES driver
    PRIV_REQUIRES esp_timer esp_lcd esp_lcd_touch lvgl_indev_port
)
