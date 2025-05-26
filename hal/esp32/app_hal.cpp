#define LGFX_USE_V1 // Sử dụng LovyanGFX V1 API
#include <LovyanGFX.hpp>
#include "app_hal.h" // Header của chính nó
// #include "lvgl.h" // Đã được include trong app_hal.h hoặc main.cpp

// --- Cấu hình LovyanGFX cho ILI9341 và XPT2046 ---
// Dựa trên các define từ User_Setup.h cũ và platformio.ini mới
class LGFX : public lgfx::LGFX_Device {
public:
    // Panel (ILI9341)
    lgfx::Panel_ILI9341 _panel_instance;
    // Bus (SPI)
    lgfx::Bus_SPI _bus_instance;
    // Backlight - Using Light_PWM as a generic solution, can act as GPIO
    lgfx::Light_PWM _light_instance; // Changed from Light_GPIO
    // Touch (XPT2046)
    lgfx::Touch_XPT2046 _touch_instance;

    LGFX(void) {
        // Cấu hình Bus SPI
        auto bcfg = _bus_instance.config();
        bcfg.spi_host = VSPI_HOST; // Hoặc HSPI_HOST, thường là VSPI_HOST (SPI3)
        bcfg.spi_mode = 0;
        bcfg.freq_write = 40000000; // Từ User_Setup.h: SPI_FREQUENCY / platformio.ini
        bcfg.freq_read  = 20000000; // Từ User_Setup.h: SPI_READ_FREQUENCY / platformio.ini
        bcfg.pin_sclk = 14;         // Từ User_Setup.h: TFT_SCLK / platformio.ini
        bcfg.pin_mosi = 13;         // Từ User_Setup.h: TFT_MOSI / platformio.ini
        bcfg.pin_miso = 12;         // Từ User_Setup.h: TFT_MISO / platformio.ini
        bcfg.pin_dc   = 2;          // Từ User_Setup.h: TFT_DC / platformio.ini
        _bus_instance.config(bcfg);
        _panel_instance.setBus(&_bus_instance);

        // Cấu hình Panel (ILI9341)
        auto pcfg = _panel_instance.config();
        pcfg.pin_cs  = 15;          // Từ User_Setup.h: TFT_CS / platformio.ini
        pcfg.pin_rst = -1;          // Từ User_Setup.h: TFT_RST / platformio.ini (nếu nối với EN, LovyanGFX tự xử lý)
        pcfg.pin_busy = -1;         // Không dùng cho ILI9341
        pcfg.panel_width  = SCREEN_WIDTH;  // 240 - từ build_flags
        pcfg.panel_height = SCREEN_HEIGHT; // 320 - từ build_flags
        pcfg.offset_x = 0;
        pcfg.offset_y = 0;
        pcfg.offset_rotation = 0; // Điều chỉnh nếu màn hình xoay vật lý
        pcfg.dummy_read_pixel = 8;
        pcfg.dummy_read_bits = 1;
        pcfg.readable = true;
        pcfg.invert = false;        
        pcfg.rgb_order = false; // Changed to false (RGB order). If true was BGR, this tests RGB.
                               // LovyanGFX ILI9341 default is usually RGB (false).
        pcfg.dlen_16bit = true;
        pcfg.bus_shared = true;     // Nếu touch và display dùng chung bus SPI
        _panel_instance.config(pcfg);

        // Cấu hình Backlight (using Light_PWM, can be used as simple on/off)
        auto lcfg = _light_instance.config();
        lcfg.pin_bl = 21;           // Từ User_Setup.h: TFT_BL / platformio.ini
        lcfg.invert = false;        // false if HIGH turns backlight on       
        // For PWM control, you'd also set lcfg.freq, lcfg.pwm_channel, etc.
        // For simple GPIO-like on/off, these defaults or high brightness is fine.
        _light_instance.config(lcfg);
        _panel_instance.setLight(&_light_instance);

        // Cấu hình Touch (XPT2046)
        auto tcfg = _touch_instance.config();
        tcfg.spi_host = VSPI_HOST;  // Cùng bus với display nếu bus_shared = true
        tcfg.freq = 2500000;        // Từ User_Setup.h: SPI_TOUCH_FREQUENCY / platformio.ini
        tcfg.pin_sclk = 14;         // Chia sẻ chân nếu dùng chung bus
        tcfg.pin_mosi = 13;         // Chia sẻ chân
        tcfg.pin_miso = 12;         // Chia sẻ chân
        tcfg.pin_cs   = 33;         // Từ User_Setup.h: TOUCH_CS / platformio.ini
        tcfg.x_min = 200;  // *** CẦN HIỆU CHỈNH (CALIBRATE) ***
        tcfg.x_max = 3800; // *** CẦN HIỆU CHỈNH (CALIBRATE) ***
        tcfg.y_min = 200;  // *** CẦN HIỆU CHỈNH (CALIBRATE) ***
        tcfg.y_max = 3800; // *** CẦN HIỆU CHỈNH (CALIBRATE) ***
        _touch_instance.config(tcfg);
        _panel_instance.setTouch(&_touch_instance);

        setPanel(&_panel_instance); // Gán panel đã cấu hình cho device
    }
};

LGFX tft; // Tạo một instance của LGFX

// --- Biến LVGL ---
static lv_display_t *lvDisplay = nullptr;
static lv_indev_t *lvInput = nullptr;

#ifndef LV_HOR_RES_MAX
    #define LV_HOR_RES_MAX SCREEN_WIDTH
#endif
#ifndef LV_VER_RES_MAX
    #define LV_VER_RES_MAX SCREEN_HEIGHT
#endif

// Define the height of the draw buffer (e.g., 1/10th of the screen height, or 30 lines)
#define DRAW_BUF_LINES (30)

static lv_draw_buf_t disp_buf;
static lv_color_t buf_1[LV_HOR_RES_MAX * DRAW_BUF_LINES]; 
// static lv_color_t buf_2[LV_HOR_RES_MAX * DRAW_BUF_LINES]; // Optional second buffer

// --- Callbacks cho LVGL ---
static void disp_flush_callback(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.writePixels((lgfx::rgb565_t*)px_map, w * h);
    tft.endWrite();

    lv_display_flush_ready(disp);
}

static void touchpad_read_callback(lv_indev_t *indev, lv_indev_data_t *data) {
    uint16_t touchX, touchY;
    bool touched = tft.getTouch(&touchX, &touchY);

    if (touched) {
        data->point.x = touchX;
        data->point.y = touchY;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

// --- Hàm HAL ---
void hal_setup(void) {
    if (!tft.begin()) {
        Serial.println("LovyanGFX tft.begin() failed!");
        while(1);
    }
    tft.setRotation(0);
    tft.setBrightness(255);

    // lv_init() sẽ được gọi trong main.cpp trước hal_setup()

    lvDisplay = lv_display_create(LV_HOR_RES_MAX, LV_VER_RES_MAX);
    if (lvDisplay == NULL) {
        Serial.println("Failed to create LVGL display!");
        while(1);
    }
    lv_display_set_flush_cb(lvDisplay, disp_flush_callback);
    
    // Initialize the draw buffer for LVGL 9
    // Parameters: draw_buf, width, height (of buffer), color_format, stride, data_pointer, data_size_in_bytes
    uint32_t stride = LV_HOR_RES_MAX * lv_color_format_get_size(LV_COLOR_FORMAT_RGB565); // Calculate stride in bytes
    lv_draw_buf_init(&disp_buf, LV_HOR_RES_MAX, DRAW_BUF_LINES, LV_COLOR_FORMAT_RGB565, stride, buf_1, sizeof(buf_1));
    lv_display_set_draw_buffers(lvDisplay, &disp_buf, NULL); // Set buffer 1, buffer 2 is NULL for single buffering

    lvInput = lv_indev_create();
    if (lvInput == NULL) {
         Serial.println("Failed to create LVGL input device!");
        while(1);
    }
    lv_indev_set_type(lvInput, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(lvInput, touchpad_read_callback);
    lv_indev_set_display(lvInput, lvDisplay);

    #if LV_USE_LOG && LV_LOG_PRINTF
    // extern void my_lvgl_log_printer(lv_log_level_t level, const char *dsc); 
    // lv_log_register_print_cb(my_lvgl_log_printer);
    #endif
    Serial.println("HAL setup completed.");
}

void hal_loop(void) {
    lv_timer_handler();
} 