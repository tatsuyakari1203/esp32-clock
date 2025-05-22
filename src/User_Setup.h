// In file User_Setup.h or your custom configuration file
#define ILI9341_DRIVER

#define TFT_MOSI 13 // SPI MOSI (Master Out Slave In) - Data to TFT
#define TFT_MISO 12 // SPI MISO (Master In Slave Out) - Data from TFT (not always used)
#define TFT_SCLK 14 // SPI Clock
#define TFT_CS   15 // TFT Chip Select
#define TFT_DC    2 // TFT Data/Command
#define TFT_RST  -1 // TFT Reset (connected to EN of ESP32) or GPIO if using a separate pin

#define TFT_BL   21 // LED Backlight control
#define TFT_BACKLIGHT_ON HIGH // Level to turn on the backlight

#define TOUCH_CS 33 // Touch Chip Select

// SPI Frequencies
#define SPI_FREQUENCY        40000000     // For ILI9341 display, can go up to 80MHz but 40MHz is more stable
#define SPI_READ_FREQUENCY   20000000     // Read frequency from the display
#define SPI_TOUCH_FREQUENCY  2500000      // XPT2046 usually requires a lower SPI frequency

// Enable fonts (optional if LVGL manages fonts)
#define LOAD_GLCD  // System 5x7 font
#define LOAD_FONT2 // 16 pixel high font
#define LOAD_FONT4 // 26 pixel high font
#define LOAD_FONT6 // 48 pixel high font (digits only)
#define LOAD_FONT7 // 7 segment font
#define LOAD_FONT8 // 75 pixel high font (digits only)
#define LOAD_GFXFF // FreeFonts library support

#define SMOOTH_FONT

// Use HSPI for the display
// #define USE_HSPI_PORT // If using VSPI, comment this line 