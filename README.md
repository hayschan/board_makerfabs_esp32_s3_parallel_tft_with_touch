# SquareLine OBP for Makerfabs ESP32-S3 Parallel TFT with Touch 

Hardware involved:

- LCD Driver: ILI9488(16bits parallel line)
- LCD Resolution: 480 x 320
- Touch panel controller: FT6236 Series ICs

## How to use

Download the files from the [latest release](https://github.com/hayschan/board_makerfabs_esp32_s3_parallel_tft_with_touch/releases).

1. Create a `Espressif/makerfabs_esp32_s3_parallel_tft_with_touch` folder in your OBP folder. For detailed intructions, please Refer to SquareLine Studio [OBP documentation](https://docs.squareline.io/docs/obp).
2. Put the three files (`.zip`, `.slb`, `.jpg`) in the latest release inside the folder.
3. In SquareLine Studio's File->Project settings->Board Properties->Board, set Board Group as Espressif, and choose Board as "Makerfabs ESP32-S3 Parallel TFT with Touch".

## Reference

- [LVGL + LGFX for MakerFabs 3.5" Parallel TFT ESP32-S2 and S3](https://github.com/radiosound-com/makerfabs-parallel-tft-lvgl-lgfx)
