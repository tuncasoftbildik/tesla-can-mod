// TFT_eSPI User Setup for Waveshare LCD boards

#define USER_SETUP_LOADED

#define ST7789_DRIVER

#if defined(BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_2)
// Waveshare ESP32-S3-Touch-LCD-2 (2.0" 240x320)
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

#define TFT_MOSI 38
#define TFT_SCLK 39
#define TFT_MISO 40
#define TFT_DC   42
#define TFT_RST  -1
#define TFT_CS   45
#define TFT_BL   1

#define TFT_RGB_ORDER TFT_BGR
#define TFT_INVERSION_ON

#else
// Waveshare ESP32-C6-LCD-1.47 (1.47" 172x320)
#define TFT_WIDTH  172
#define TFT_HEIGHT 320

#define TFT_CS   14
#define TFT_DC   15
#define TFT_RST  21
#define TFT_BL   22
#define TFT_MOSI 6
#define TFT_SCLK 2

#define TFT_RGB_ORDER TFT_BGR
#endif

#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  20000000
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define SMOOTH_FONT
