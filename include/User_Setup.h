// TFT_eSPI User Setup for Waveshare ESP32-C6-LCD-1.47
// Display: 1.47" ST7789V, 172x320

#define USER_SETUP_LOADED

#define ST7789_DRIVER
#define TFT_WIDTH  172
#define TFT_HEIGHT 320

// ESP32-C6 LCD 1.47 pin mapping
#define TFT_CS   14
#define TFT_DC   15
#define TFT_RST  21
#define TFT_BL   22
#define TFT_MOSI 6
#define TFT_SCLK 2

#define TFT_RGB_ORDER TFT_BGR
#define SPI_FREQUENCY  40000000
#define SPI_READ_FREQUENCY  20000000
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define SMOOTH_FONT
