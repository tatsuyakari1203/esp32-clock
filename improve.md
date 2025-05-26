### Mục tiêu:

- Nâng cấp thư viện LVGL từ phiên bản 8.x lên 9.1.

- Áp dụng cấu trúc thư mục (`hal`, `src`) và cách quản lý cấu hình (qua `platformio.ini` thay vì `lv_conf.h`, `User_Setup.h`) từ repo `lvgl/lv_platformio`.

- Chuyển sang sử dụng thư viện LovyanGFX để quản lý driver màn hình (ILI9341) và cảm ứng (XPT2046), thay thế cho TFT\_eSPI và logic cảm ứng riêng lẻ.


### Điều kiện tiên quyết:

- Đã cài đặt PlatformIO IDE trong Visual Studio Code.

- Có kiến thức cơ bản về PlatformIO, C++ và ESP32.


### Các bước thực hiện chi tiết:

**1. Cấu trúc lại thư mục dự án:**

Hiện tại, `main.cpp`, `lv_conf.h`, và `User_Setup.h` có thể đang nằm cùng cấp hoặc trong `src/`. Để theo cấu trúc mẫu:

- Tạo thư mục `hal` ở thư mục gốc của dự án.

- Bên trong `hal`, tạo thư mục `esp32`.

- Nếu thư mục `src` chưa có, hãy tạo nó ở thư mục gốc.

- Di chuyển file `main.cpp` hiện tại của bạn vào thư mục `src/`.

- Các file `lv_conf.h` và `User_Setup.h` sẽ không còn được sử dụng trực tiếp; cấu hình của chúng sẽ được chuyển vào `platformio.ini` hoặc `app_hal.cpp`.

Sau khi cấu trúc lại, dự án của bạn sẽ có dạng:

    your_project_root/
    ├── platformio.ini
    ├── src/
    │   └── main.cpp
    │   └── (các file .cpp/.h khác của UI nếu có)
    ├── hal/
    │   └── esp32/
    │       ├── app_hal.h
    │       └── app_hal.cpp
    │       └── (có thể thêm file cấu hình display riêng cho LovyanGFX nếu cần)
    ├── lib/      (thư mục do PlatformIO quản lý)
    └── include/  (nếu có header chung)

**2. Cập nhật `platformio.ini`:**

Đây là bước quan trọng để quản lý thư viện và cấu hình build. Mở file `platformio.ini` của bạn và thực hiện các thay đổi sau, tham khảo từ `env:esp32_boards` của repo mẫu:

    [platformio]
    default_envs = esp32dev ; Hoặc tên environment bạn muốn

    [env:esp32dev]
    platform = espressif32
    board = esp32dev ; Hoặc board ESP32 cụ thể của bạn
    framework = arduino
    monitor_speed = 115200

    lib_deps =
        ; LVGL phiên bản 9.1
        lvgl/lvgl@^9.1.0
        ; LovyanGFX cho display và touch
        lovyan03/LovyanGFX@^1.1.9 ; Kiểm tra phiên bản mới nhất tương thích
        ; Các thư viện khác của bạn (NTP, DHT)
        arduino-libraries/NTPClient@^3.2.1
        DHT sensor library @ ^1.4.4
        Adafruit Unified Sensor @ ^1.1.14

    build_flags =
        ; --- Cờ chung ---
        -I "${PROJECT_SRC_DIR}" ; Đảm bảo src được include

        ; --- Cờ cấu hình LVGL (thay thế cho lv_conf.h) ---
        -D LV_CONF_SKIP             ; Bỏ qua lv_conf.h mặc định của thư viện LVGL
        -D LV_CONF_INCLUDE_SIMPLE   ; Cho phép include lv_conf_simple.h (nếu cần, LVGL9 có thể không cần)

        ; Cài đặt LVGL cơ bản (chuyển từ lv_conf.h của bạn)
        -D LV_COLOR_DEPTH=16
        ; -D LV_COLOR_DEPTH_32_BPP=4 ; Chỉ cần nếu LV_COLOR_DEPTH=32
        -D LV_TICK_CUSTOM=0         ; Nếu bạn gọi lv_tick_inc() thủ công
        -D LV_DISP_DEF_REFR_PERIOD=30
        -D LV_INDEV_DEF_READ_PERIOD=30
        -D LV_MEM_CUSTOM=0          ; 0 để dùng LV_MEM_SIZE, 1 để dùng custom allocators
        -D LV_MEM_SIZE="(32U * 1024U)" ; Điều chỉnh nếu cần, ví dụ (64U * 1024U) cho PSRAM
        ; -D LV_USE_LOG=1
        ; #if LV_USE_LOG
        ; -D LV_LOG_LEVEL=LV_LOG_LEVEL_WARN ; (LV_LOG_LEVEL_TRACE, INFO, WARN, ERROR, USER, NONE)
        ; -D LV_LOG_PRINTF=1 ; Nếu muốn dùng printf cho log LVGL (cần hàm my_lvgl_log_printer)
        ; #endif

        ; Kích hoạt font (chuyển từ lv_conf.h)
        ; Ví dụ:
        -D LV_FONT_MONTSERRAT_12=1
        -D LV_FONT_MONTSERRAT_14=1
        -D LV_FONT_MONTSERRAT_16=1
        -D LV_FONT_MONTSERRAT_20=1
        -D LV_FONT_MONTSERRAT_22=1
        -D LV_FONT_MONTSERRAT_24=1
        -D LV_FONT_MONTSERRAT_28=1
        -D LV_FONT_MONTSERRAT_38=1
        -D LV_FONT_MONTSERRAT_48=1
        -D LV_FONT_DEFAULT="&lv_font_montserrat_16" ; Đặt font mặc định

        ; Kích hoạt widget (chuyển từ lv_conf.h)
        ; Ví dụ:
        -D LV_USE_ARC=1
        -D LV_USE_BAR=1
        -D LV_USE_BTN=1
        -D LV_USE_LABEL=1
        -D LV_USE_CALENDAR=1
        ; #if LV_USE_CALENDAR
        ; -D LV_CALENDAR_WEEK_STARTS_MONDAY=0
        ; -D LV_USE_CALENDAR_HEADER_ARROW=1
        ; #endif
        ; ... thêm các widget khác bạn dùng ...

        ; --- Cờ cho HAL ---
        ; Thêm đường dẫn include cho thư mục hal/esp32
        !python -c "import os; print(' '.join(['-I {}'.format(i[0].replace('\\\\','/')) for i in os.walk('hal/esp32')]))"
        
        ; --- Cờ màn hình (thay thế User_Setup.h, sẽ dùng trong LovyanGFX config) ---
        ; Các define như TFT_WIDTH, TFT_HEIGHT sẽ được dùng trực tiếp trong app_hal.cpp
        ; Hoặc bạn có thể pass chúng vào đây nếu muốn LGFX config đọc từ define
        -D SCREEN_WIDTH=240
        -D SCREEN_HEIGHT=320

        ; --- Cờ khác cho board ESP32 ---
        ; Ví dụ: nếu có PSRAM
        ; -DBOARD_HAS_PSRAM
        ; -mfix-esp32-psram-cache-issue

    lib_ldf_mode = deep+
    lib_archive = false ; Quan trọng cho LVGL để build đúng các file nguồn

**Lưu ý về `build_flags` cho LVGL:**

- Bạn cần xem lại file `lv_conf.h` cũ của mình và chuyển tất cả các `#define LV_...` quan trọng sang dạng `-D LV_...=value` trong `platformio.ini`.

- Đối với các define có điều kiện (ví dụ `#if LV_USE_CALENDAR ... #endif`), bạn cũng cần chuyển các define bên trong nếu điều kiện chính được bật.

- Các define về font và widget cần được giữ lại để LVGL biên dịch các thành phần đó.

**3. Tạo Hardware Abstraction Layer (HAL) cho ESP32 với LovyanGFX:**

Mục đích của HAL là tách biệt logic phần cứng (điều khiển màn hình, cảm ứng) ra khỏi code ứng dụng chính.

- **Tạo `hal/esp32/app_hal.h`:**

      #ifndef APP_HAL_H
      #define APP_HAL_H

      #include "lvgl.h" // Cần thiết nếu các hàm HAL dùng kiểu dữ liệu của LVGL

      #ifdef __cplusplus
      extern "C" {
      #endif

      /**
       * @brief Khởi tạo phần cứng hiển thị, cảm ứng và các driver LVGL liên quan.
       */
      void hal_setup(void);

      /**
       * @brief Hàm được gọi lặp đi lặp lại trong vòng lặp chính hoặc task RTOS.
       * Chủ yếu để gọi lv_timer_handler().
       */
      void hal_loop(void);

      #ifdef __cplusplus
      }
      #endif

      #endif // APP_HAL_H

- Tạo hal/esp32/app\_hal.cpp:

  Đây là nơi bạn sẽ cấu hình LovyanGFX và các callback cho LVGL.

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
          // Backlight (PWM hoặc GPIO) - ví dụ dùng GPIO
          lgfx::Light_GPIO _light_instance;
          // Touch (XPT2046)
          lgfx::Touch_XPT2046 _touch_instance;

          LGFX(void) {
              // Cấu hình Bus SPI
              auto bcfg = _bus_instance.config();
              bcfg.spi_host = VSPI_HOST; // Hoặc HSPI_HOST, thường là VSPI_HOST (SPI3)
              bcfg.spi_mode = 0;
              bcfg.freq_write = 40000000; // Từ User_Setup.h: SPI_FREQUENCY
              bcfg.freq_read  = 20000000; // Từ User_Setup.h: SPI_READ_FREQUENCY
              bcfg.pin_sclk = 14;         // Từ User_Setup.h: TFT_SCLK
              bcfg.pin_mosi = 13;         // Từ User_Setup.h: TFT_MOSI
              bcfg.pin_miso = 12;         // Từ User_Setup.h: TFT_MISO
              bcfg.pin_dc   = 2;          // Từ User_Setup.h: TFT_DC
              _bus_instance.config(bcfg);
              _panel_instance.setBus(&_bus_instance);

              // Cấu hình Panel (ILI9341)
              auto pcfg = _panel_instance.config();
              pcfg.pin_cs  = 15;          // Từ User_Setup.h: TFT_CS
              pcfg.pin_rst = -1;          // Từ User_Setup.h: TFT_RST (nếu nối với EN, LovyanGFX tự xử lý)
              pcfg.pin_busy = -1;         // Không dùng cho ILI9341
              pcfg.panel_width  = SCREEN_WIDTH;  // 240 - từ build_flags hoặc define trực tiếp
              pcfg.panel_height = SCREEN_HEIGHT; // 320 - từ build_flags hoặc define trực tiếp
              pcfg.offset_x = 0;
              pcfg.offset_y = 0;
              pcfg.offset_rotation = 0; // Điều chỉnh nếu màn hình xoay vật lý
              pcfg.dummy_read_pixel = 8;
              pcfg.dummy_read_bits = 1;
              pcfg.readable = true;
              pcfg.invert = false;        // Thử true/false nếu màu sắc bị ngược
              pcfg.rgb_order = lgfx::rgb_order_bgr; // Thử lgfx::rgb_order_rgb nếu màu R/B bị tráo
              pcfg.dlen_16bit = true;
              pcfg.bus_shared = true;     // Nếu touch và display dùng chung bus SPI
              _panel_instance.config(pcfg);

              // Cấu hình Backlight (ví dụ dùng GPIO)
              auto lcfg = _light_instance.config();
              lcfg.pin_bl = 21;           // Từ User_Setup.h: TFT_BL
              lcfg.invert = false;        // false nếu TFT_BACKLIGHT_ON là HIGH
              _light_instance.config(lcfg);
              _panel_instance.setLight(&_light_instance);

              // Cấu hình Touch (XPT2046)
              auto tcfg = _touch_instance.config();
              tcfg.spi_host = VSPI_HOST;  // Cùng bus với display nếu bus_shared = true
              tcfg.freq = 2500000;        // Từ User_Setup.h: SPI_TOUCH_FREQUENCY
              tcfg.pin_sclk = 14;         // Chia sẻ chân nếu dùng chung bus
              tcfg.pin_mosi = 13;         // Chia sẻ chân
              tcfg.pin_miso = 12;         // Chia sẻ chân
              tcfg.pin_cs   = 33;         // Từ User_Setup.h: TOUCH_CS
              tcfg.x_min = 200;  // *** CẦN HIỆU CHỈNH (CALIBRATE) ***
              tcfg.x_max = 3800; // *** CẦN HIỆU CHỈNH (CALIBRATE) ***
              tcfg.y_min = 200;  // *** CẦN HIỆU CHỈNH (CALIBRATE) ***
              tcfg.y_max = 3800; // *** CẦN HIỆU CHỈNH (CALIBRATE) ***
              // Chú ý: x_min, x_max, y_min, y_max phụ thuộc vào hướng xoay màn hình (rotation)
              // và cách lắp đặt màn cảm ứng. Bạn cần một chương trình hiệu chỉnh riêng hoặc thử nghiệm.
              // tcfg.offset_rotation = 0; // Đặt giống offset_rotation của panel nếu cần
              _touch_instance.config(tcfg);
              _panel_instance.setTouch(&_touch_instance);

              setPanel(&_panel_instance); // Gán panel đã cấu hình cho device
          }
      };

      LGFX tft; // Tạo một instance của LGFX

      // --- Biến LVGL ---
      static lv_display_t *lvDisplay = nullptr;
      static lv_indev_t *lvInput = nullptr;

      // Buffer cho LVGL (kích thước bằng 1/10 màn hình hoặc tùy chỉnh)
      // LVGL 9.1 không yêu cầu buf_2 nếu không dùng LV_DISPLAY_RENDER_MODE_PARTIAL với double buffer
      #define LV_HOR_RES_MAX SCREEN_WIDTH
      #define LV_VER_RES_MAX SCREEN_HEIGHT
      static lv_disp_draw_buf_t disp_buf;
      static lv_color_t buf_1[LV_HOR_RES_MAX * 30]; // 30 dòng buffer

      // --- Callbacks cho LVGL ---
      // Callback để flush buffer ra màn hình
      static void disp_flush_callback(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
          uint32_t w = lv_area_get_width(area);
          uint32_t h = lv_area_get_height(area);

          // px_map là uint8_t* trong LVGL 9.
          // LovyanGFX thường dùng uint16_t* cho màu RGB565.
          // Nếu LV_COLOR_16_SWAP=1 (mặc định thường là 0), LVGL đã swap byte.
          // Nếu LovyanGFX cần swap và LVGL chưa swap, bạn có thể cần:
          // lv_draw_sw_rgb565_swap(px_map, w * h); // Chỉ khi cần thiết

          tft.startWrite();
          tft.setAddrWindow(area->x1, area->y1, w, h);
          // tft.pushImage(area->x1, area->y1, w, h, (lgfx::rgb565_t*)px_map); // Hoặc
          tft.writePixels((lgfx::rgb565_t*)px_map, w * h);
          tft.endWrite();

          lv_display_flush_ready(disp);
      }

      // Callback để đọc trạng thái cảm ứng
      static void touchpad_read_callback(lv_indev_t *indev, lv_indev_data_t *data) {
          uint16_t touchX, touchY;
          bool touched = tft.getTouch(&touchX, &touchY); // Ngưỡng mặc định của LovyanGFX khá tốt

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
          tft.begin();        // Khởi tạo LovyanGFX
          tft.setRotation(0); // Đặt hướng xoay (0, 1, 2, 3)
          tft.setBrightness(255); // Điều khiển độ sáng backlight (0-255)

          // lv_init() sẽ được gọi trong main.cpp trước hal_setup()

          // Khởi tạo display driver cho LVGL 9.1
          lvDisplay = lv_display_create(LV_HOR_RES_MAX, LV_VER_RES_MAX);
          if (lvDisplay == NULL) {
              // Xử lý lỗi nếu không tạo được display
              Serial.println("Failed to create LVGL display!");
              while(1);
          }
          lv_display_set_flush_cb(lvDisplay, disp_flush_callback);
          lv_disp_draw_buf_init(&disp_buf, buf_1, NULL, sizeof(buf_1) / sizeof(lv_color_t));
          lv_display_set_draw_buffers(lvDisplay, &disp_buf);
          // lv_display_set_color_format(lvDisplay, LV_COLOR_FORMAT_RGB565); // Mặc định là RGB565 nếu LV_COLOR_DEPTH=16

          // Khởi tạo input driver (touch) cho LVGL 9.1
          lvInput = lv_indev_create();
          if (lvInput == NULL) {
               Serial.println("Failed to create LVGL input device!");
              while(1);
          }
          lv_indev_set_type(lvInput, LV_INDEV_TYPE_POINTER);
          lv_indev_set_read_cb(lvInput, touchpad_read_callback);
          lv_indev_set_display(lvInput, lvDisplay); // Gán input device cho display

          // Nếu bạn dùng log LVGL và đã define LV_LOG_PRINTF=1
          // #if LV_USE_LOG && LV_LOG_PRINTF
          // extern void my_lvgl_log_printer(const char *dsc); // Khai báo hàm in log
          // lv_log_register_print_cb(my_lvgl_log_printer);
          // #endif
          Serial.println("HAL setup completed.");
      }

      void hal_loop(void) {
          // Được gọi từ loop() trong main.cpp hoặc từ một task RTOS riêng
          lv_timer_handler(); // Xử lý các timer và task của LVGL
      }

  Quan trọng về hiệu chỉnh cảm ứng (Calibration):

  Các giá trị x\_min, x\_max, y\_min, y\_max và cfg.transform\_xy = true/false; cfg.repeat\_xy = ...; trong cấu hình \_touch\_instance của LovyanGFX CỰC KỲ QUAN TRỌNG. Bạn cần chạy một sketch hiệu chỉnh riêng (LovyanGFX có ví dụ, hoặc tìm các sketch "XPT2046 calibration ESP32") để lấy các giá trị raw min/max chính xác cho màn hình của bạn ở hướng xoay mong muốn. Nếu không, cảm ứng sẽ không chính xác.

**4. Cập nhật `src/main.cpp`:**

File `main.cpp` của bạn sẽ được đơn giản hóa, tập trung vào logic ứng dụng và gọi các hàm HAL.

    #include <Arduino.h>
    #include "lvgl.h"       // Luôn include LVGL trước
    #include "app_hal.h"    // HAL của bạn

    // --- Các include khác của bạn ---
    #include <WiFi.h>
    #include "time.h"
    #include <NTPClient.h>
    #include <WiFiUdp.h>
    #include "DHT.h"
    #include <vector>
    #include <stdarg.h>

    // --- Hằng số và biến toàn cục của bạn (giữ nguyên nếu cần) ---
    const char* WIFI_SSID = "2.4 KariS";
    const char* WIFI_PASSWORD = "12123402";
    // ... (NTP_SERVER_LIST, DHTPIN, DHTTYPE, etc.) ...
    constexpr int LVGL_TICK_PERIOD_MS = 5; // Hoặc 10ms

    // --- Các khai báo và con trỏ toàn cục của bạn ---
    // Xóa DisplayManager, UIManager (một phần), NetworkManager, TimeManager nếu logic của chúng
    // đã được tích hợp hoặc bạn muốn tái cấu trúc.
    // Giữ lại các biến UI của LVGL (ví dụ: lv_obj_t *label_hour)
    // hoặc đóng gói chúng vào một struct/class quản lý UI riêng.

    lv_obj_t *label_hour, *label_minute, *label_colon_hm, *label_colon_ms, *label_seconds;
    lv_obj_t *calendar_obj, *label_ntp_status, *label_ip_address;
    lv_obj_t *label_temperature, *label_humidity;
    // ... (các biến trạng thái UI khác: prev_h, colon_blink_timer, etc.)

    // --- Các hàm tiện ích (AppLog, my_lvgl_log_printer) ---
    // Giữ nguyên hoặc điều chỉnh nếu cần.
    // Nếu dùng LV_LOG_PRINTF=1, bạn cần hàm này:
    // void my_lvgl_log_printer(const char *buf) { Serial.print(buf); }

    // --- Logic UI của bạn (tạo widget, cập nhật) ---
    // Ví dụ: void create_clock_ui(lv_obj_t* parent) { ... }
    // void update_time_labels(const struct tm &timeinfo) { ... }
    // Các hàm này sẽ sử dụng API của LVGL 9.1.

    // --- RTOS (nếu bạn dùng) ---
    SemaphoreHandle_t lvgl_mutex = NULL; // Quan trọng nếu gọi API LVGL từ nhiều task
    // TaskHandle_t lvgl_task_handle = NULL; // Không nhất thiết phải là global
    // TaskHandle_t network_task_handle = NULL;

    // Hàm tạo UI (ví dụ)
    void create_main_ui(void) {
        if (lvgl_mutex && xSemaphoreTake(lvgl_mutex, portMAX_DELAY) != pdTRUE) {
            // AppLog::error("UI", "Failed to get mutex for create_ui.");
            return;
        }

        lv_obj_t *scr = lv_scr_act();
        lv_obj_clean(scr); // Xóa các đối tượng cũ nếu có
        lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);

        // Tạo lại các label, calendar, v.v. như trong UIManager cũ của bạn
        // Ví dụ:
        // label_hour = lv_label_create(scr);
        // lv_obj_align(label_hour, LV_ALIGN_TOP_LEFT, 10, 10);
        // lv_label_set_text(label_hour, "--");
        // ...

        // Khởi tạo các style (nếu chưa làm trong UIManager::init_styles_static)
        // UIManager::init_styles_static(); // Gọi một lần

        // Áp dụng style
        // lv_obj_add_style(label_hour, &UIManager::style_time_elements, 0);

        // Tạo các timer LVGL (ví dụ: colon_blink_timer)
        // colon_blink_timer = lv_timer_create(colon_blink_timer_cb_static, 500, this_ptr_if_member_func_or_NULL);

        if (lvgl_mutex) xSemaphoreGive(lvgl_mutex);
        // AppLog::info("UI", "UI elements created.");
    }


    void setup() {
        Serial.begin(115200);
        // AppLog::info("System", "ESP32 NTP Clock with DHT11 Starting (LVGL 9.1)...");

        dht.begin(); // Khởi tạo cảm biến DHT

        // Tạo mutex cho LVGL nếu dùng RTOS và truy cập LVGL từ nhiều task
        lvgl_mutex = xSemaphoreCreateMutex();
        if (lvgl_mutex == NULL) {
            // AppLog::error("System", "Failed to create LVGL mutex! Halting.");
            while(1) vTaskDelay(pdMS_TO_TICKS(1000));
        }

        lv_init();      // Khởi tạo LVGL
        hal_setup();    // Khởi tạo display, input trong HAL

        // Khởi tạo WiFi, NTP, v.v.
        // ... (logic kết nối WiFi, khởi tạo NTPClient của bạn) ...

        // Tạo UI
        create_main_ui(); // Hàm tạo giao diện của bạn
        // Hoặc chạy demo của LVGL 9 để kiểm tra
        // lv_demo_widgets();
        // lv_demo_benchmark();
        // lv_demo_music();

        // Nếu dùng RTOS, tạo các task
        // xTaskCreatePinnedToCore(lvgl_task_handler_rtos, "LVGL_Task", 8192, NULL, 2, &lvgl_task_handle, 0);
        // xTaskCreatePinnedToCore(network_time_task_handler_rtos, "NetTime_Task", 8192, NULL, 1, &network_task_handle, 1);

        // AppLog::info("System", "Setup complete.");
    }

    void loop() {
        // Cập nhật tick cho LVGL
        lv_tick_inc(LVGL_TICK_PERIOD_MS);

        // Gọi HAL loop (chứa lv_timer_handler())
        // Cần bảo vệ nếu hal_loop() hoặc các hàm cập nhật UI gọi API LVGL và có task RTOS khác cũng gọi
        if (lvgl_mutex && xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(10)) == pdTRUE) { // Timeout ngắn
            hal_loop(); // Gọi lv_timer_handler()
            xSemaphoreGive(lvgl_mutex);
        } else if (!lvgl_mutex) { // Nếu không dùng mutex (chạy đơn luồng)
            hal_loop();
        }


        // --- Logic ứng dụng của bạn ---
        // (Cập nhật thời gian, đọc DHT, cập nhật UI)
        // Ví dụ:
        // if (initial_ntp_sync_done && g_timeManager) {
        //     static unsigned long last_display_update_ms = 0;
        //     if (millis() - last_display_update_ms >= 1000) {
        //         last_display_update_ms = millis();
        //         struct tm timeinfo_utc;
        //         if (g_timeManager->get_current_utc_time(&timeinfo_utc)) {
        //             if (lvgl_mutex && xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        //                 // g_uiManager->update_time_display_labels(timeinfo_utc); // Gọi hàm cập nhật UI của bạn
        //                 xSemaphoreGive(lvgl_mutex);
        //             } else if (!lvgl_mutex) {
        //                 // g_uiManager->update_time_display_labels(timeinfo_utc);
        //             }
        //         }
        //     }
        // }
        // ... (logic đọc DHT và cập nhật UI tương tự) ...

        vTaskDelay(pdMS_TO_TICKS(LVGL_TICK_PERIOD_MS)); // Hoặc delay() nếu không dùng RTOS
                                                       // Delay này nên bằng hoặc lớn hơn LVGL_TICK_PERIOD_MS
    }

    // --- Định nghĩa các task RTOS nếu bạn dùng ---
    // void lvgl_task_handler_rtos(void *p) {
    //     AppLog::info("RTOS", "LVGL task started.");
    //     TickType_t xLastWakeTime = xTaskGetTickCount();
    //     const TickType_t xPeriod = pdMS_TO_TICKS(LVGL_TICK_PERIOD_MS);

    //     for (;;) {
    //         if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY) == pdTRUE) {
    //             lv_timer_handler(); // Gọi trực tiếp lv_timer_handler() trong task riêng của nó
    //             xSemaphoreGive(lvgl_mutex);
    //         }
    //         vTaskDelayUntil(&xLastWakeTime, xPeriod); // Đảm bảo chu kỳ đều đặn
    //     }
    // }

    // void network_time_task_handler_rtos(void *p) {
    //     // ... (logic task network/time của bạn) ...
    //     // Khi cập nhật UI từ task này, nhớ dùng mutex:
    //     // if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(UI_UPDATE_MUTEX_TIMEOUT)) == pdTRUE) {
    //     //    lv_label_set_text(label_ntp_status, "Synced");
    //     //    xSemaphoreGive(lvgl_mutex);
    //     // }
    // }

**5. Xóa các file không cần thiết:**

- Xóa `User_Setup.h` từ dự án của bạn.

- Xóa `lv_conf.h` từ dự án của bạn.

**6. Kiểm tra và điều chỉnh API LVGL 8.x sang 9.1:**

LVGL 9.1 có một số thay đổi API so với 8.x. Dưới đây là những điểm chính bạn cần lưu ý khi xem lại code UI của mình:

- **Display Driver:**

  - `lv_disp_drv_init()` và `lv_disp_drv_register()` đã thay đổi.

  - Sử dụng `lv_display_create(hor_res, ver_res)` để tạo display.

  - `lv_display_set_flush_cb(disp, flush_cb)`.

  - `lv_display_set_draw_buffers(disp, draw_buf_dsc)`. (thay cho `disp_drv->draw_buf = &disp_buf;`)

  - `lv_display_set_color_format(disp, LV_COLOR_FORMAT_...)` nếu cần (mặc định dựa trên `LV_COLOR_DEPTH`).

- **Input Driver:**

  - `lv_indev_drv_init()` và `lv_indev_drv_register()` đã thay đổi.

  - Sử dụng `lv_indev_create()` để tạo input device.

  - `lv_indev_set_type(indev, type)`.

  - `lv_indev_set_read_cb(indev, read_cb)`.

  - `lv_indev_set_display(indev, disp)` để gán input device cho một display cụ thể.

- **Tick và Timer Handler:**

  - `lv_tick_inc(ms)`: Vẫn giữ nguyên.

  - `lv_timer_handler()`: Thay thế cho `lv_task_handler()`.

- **Styles:**

  - Cơ chế style cơ bản vẫn tương tự (`lv_obj_add_style()`). Tuy nhiên, một số thuộc tính style hoặc cách chúng được áp dụng có thể có thay đổi nhỏ. Tham khảo tài liệu LVGL 9.

  - Việc khởi tạo style (`lv_style_init()`, `lv_style_set_...()`) vẫn giữ nguyên.

- **Events:**

  - `lv_obj_add_event_cb(obj, event_cb, LV_EVENT_..., user_data)` vẫn là cách chính.

  - `lv_event_get_draw_part_dsc()` (như bạn đã dùng cho calendar) vẫn hoạt động.

- **Widgets:**

  - Hầu hết API của các widget phổ biến (label, button, calendar) không thay đổi quá nhiều ở mức cơ bản. Tuy nhiên, hãy kiểm tra tài liệu cho từng widget bạn sử dụng.

  - Ví dụ, `lv_calendar_get_btnmatrix()` vẫn tồn tại.

- **Fonts:**

  - Cách khai báo và sử dụng font (`&lv_font_montserrat_14`, `LV_FONT_DEFAULT`) vẫn tương tự.

- **Color:**

  - `lv_color_white()`, `lv_color_black()`, `lv_palette_main(LV_PALETTE_...)` vẫn được sử dụng.

**Quan trọng:** Luôn tham khảo **"Migration guide"** từ LVGL 8 lên LVGL 9 trên trang web chính thức của LVGL để biết danh sách đầy đủ các thay đổi.


### Lưu ý quan trọng khi triển khai:

- **Hiệu chỉnh cảm ứng (Calibration):** Đây là bước **CỰC KỲ QUAN TRỌNG** khi dùng LovyanGFX. Các giá trị `x_min, x_max, y_min, y_max` và các thông số transform trong `_touch_instance.config()` phải được xác định chính xác cho màn hình và hướng xoay của bạn. Nếu không, cảm ứng sẽ bị lệch hoặc không hoạt động đúng. Hãy tìm một sketch hiệu chỉnh XPT2046 cho ESP32 với LovyanGFX.

- **RTOS và Mutex:**

  - Code hiện tại của bạn sử dụng FreeRTOS và `lvgl_mutex`. Điều này là tốt.

  - Đảm bảo rằng **MỌI** lệnh gọi API LVGL (tạo widget, thay đổi thuộc tính, v.v.) từ bất kỳ task nào (bao gồm `loop()` nếu nó hoạt động như một task, và các task network/time) đều được bảo vệ bởi `lvgl_mutex`.

  - Hàm `lv_tick_inc()` nên được gọi từ một nơi duy nhất, thường là trong `loop()` hoặc một task có độ ưu tiên cao, đều đặn.

  - Hàm `lv_timer_handler()` (trong `hal_loop()`) cũng nên được gọi trong ngữ cảnh được bảo vệ bởi mutex nếu có nhiều task tương tác với LVGL. Nếu bạn có một task riêng cho LVGL (ví dụ `lvgl_task_handler_rtos`), thì `lv_timer_handler()` nên được gọi bên trong task đó và được bảo vệ bởi mutex. `lv_tick_inc()` có thể được gọi từ `loop()` hoặc một timer ngắt.

- **Thứ tự Include:** Đảm bảo `lvgl.h` được include trước các file HAL hoặc UI của bạn nếu chúng sử dụng các kiểu dữ liệu hoặc macro của LVGL.

- **Kiểm tra Log:** Sử dụng `AppLog` của bạn và bật log của LVGL (`-D LV_USE_LOG=1 -D LV_LOG_LEVEL=LV_LOG_LEVEL_TRACE` trong `platformio.ini`) để gỡ lỗi trong quá trình chuyển đổi.

- **Bắt đầu từ demo:** Sau khi cấu hình HAL và LVGL cơ bản, hãy thử chạy một demo đơn giản của LVGL 9 (ví dụ `lv_demo_widgets()`) để xác nhận màn hình, cảm ứng và LVGL hoạt động đúng trước khi port toàn bộ UI phức tạp của bạn.


### Tổng kết:

Quá trình này đòi hỏi sự cẩn thận trong việc cấu hình `platformio.ini`, viết lại phần HAL cho LovyanGFX, và điều chỉnh các API LVGL. Tuy nhiên, việc áp dụng cấu trúc từ repo mẫu và sử dụng LovyanGFX sẽ giúp dự án của bạn trở nên module hóa hơn, dễ quản lý và tận dụng được các tính năng mạnh mẽ của LVGL 9.1 cũng như LovyanGFX. Chúc bạn thành công!
