#include <Arduino.h>
#include <WiFi.h>
#include "time.h"       // For struct tm, mktime, localtime_r, gmtime_r
#include <NTPClient.h>
#include <WiFiUdp.h>    // For WiFiUDP
#include <TFT_eSPI.h>
#include "lvgl.h"
#include <vector>
#include <stdarg.h>     // For va_list, vsnprintf

#include "DHT.h" // Bao gồm thư viện DHT

// --- Hằng số cấu hình ---
const char* WIFI_SSID = "2.4 KariS";        // SSID WiFi của bạn
const char* WIFI_PASSWORD = "12123402";  // Mật khẩu WiFi của bạn
const std::vector<const char*> NTP_SERVER_LIST = {
    "0.vn.pool.ntp.org", "1.vn.pool.ntp.org", "2.vn.pool.ntp.org",
    "0.asia.pool.ntp.org", "1.asia.pool.ntp.org", "2.asia.pool.ntp.org",
    "time.google.com", "pool.ntp.org",
    "1.ntp.vnix.vn", "2.ntp.vnix.vn"
};
constexpr int SCREEN_WIDTH  = 240;
constexpr int SCREEN_HEIGHT = 320;
constexpr int LVGL_TICK_PERIOD_MS = 5;
constexpr TickType_t UI_UPDATE_MUTEX_TIMEOUT = pdMS_TO_TICKS(200);

// Cấu hình cảm biến DHT
#define DHTPIN 27     // Chân kỹ thuật số kết nối với cảm biến DHT (ESP32 GPIO27)
#define DHTTYPE DHT11   // Loại cảm biến DHT 11
DHT dht(DHTPIN, DHTTYPE);
unsigned long last_dht_read_ms = 0;
const unsigned long DHT_READ_INTERVAL_MS = 5000; // Đọc DHT mỗi 5 giây

// --- Biến toàn cục và cờ ---
SemaphoreHandle_t lvgl_mutex = NULL;
TaskHandle_t lvgl_task_handle = NULL;
TaskHandle_t network_task_handle = NULL;
bool initial_ntp_sync_done = false;
WiFiUDP g_ntpUDP; // Biến UDP toàn cục cho NTPClient

// --- Trình ghi log ứng dụng ---
namespace AppLog {
    enum class Level { INFO, WARN, ERROR };
    static void _log_print(Level level, const char* module, const char* format, va_list args) {
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), format, args);
        const char* levelStr = "INFO";
        if (level == Level::WARN) levelStr = "WARN";
        else if (level == Level::ERROR) levelStr = "ERROR";
        Serial.printf("[%s][%s] %s\n", module, levelStr, buffer);
    }
    void info(const char* module, const char* format, ...) {
        va_list args; va_start(args, format); _log_print(Level::INFO, module, format, args); va_end(args);
    }
    void warn(const char* module, const char* format, ...) {
        va_list args; va_start(args, format); _log_print(Level::WARN, module, format, args); va_end(args);
    }
    void error(const char* module, const char* format, ...) {
        va_list args; va_start(args, format); _log_print(Level::ERROR, module, format, args); va_end(args);
    }
}

// --- Trình in log LVGL ---
#if LV_USE_LOG && defined(LV_LOG_PRINTF) && LV_LOG_PRINTF == my_lvgl_log_printer
void my_lvgl_log_printer(const char *buf) { Serial.print(buf); }
#endif

// Khai báo trước các lớp
class DisplayManager; class UIManager; class NetworkManager; class TimeManager;

// Con trỏ toàn cục tới các thực thể lớp
DisplayManager* g_displayManager = nullptr; UIManager* g_uiManager = nullptr;
NetworkManager* g_networkManager = nullptr; TimeManager* g_timeManager = nullptr;

// -----------------------------------------------------------------------------
// Lớp DisplayManager: Xử lý khởi tạo TFT và driver LVGL
// -----------------------------------------------------------------------------
class DisplayManager {
public:
    TFT_eSPI tft;
    static lv_disp_draw_buf_t disp_buf;
    static lv_color_t buf_1[SCREEN_WIDTH * 20];

    DisplayManager() {}

    void init() {
        tft.begin(); 
        tft.setRotation(0); // Đặt hướng xoay màn hình của bạn
        #ifdef TFT_BL // TFT_BL được định nghĩa trong cấu hình TFT_eSPI (platformio.ini)
            pinMode(TFT_BL, OUTPUT); 
            digitalWrite(TFT_BL, TFT_BACKLIGHT_ON); // TFT_BACKLIGHT_ON cũng từ cấu hình TFT_eSPI
        #else
            AppLog::warn("Display", "TFT_BL not defined. Backlight control may not work.");
        #endif

        lv_init(); 
        lv_disp_draw_buf_init(&disp_buf, buf_1, NULL, SCREEN_WIDTH * 20);

        static lv_disp_drv_t disp_drv; 
        lv_disp_drv_init(&disp_drv);
        disp_drv.hor_res = SCREEN_WIDTH; 
        disp_drv.ver_res = SCREEN_HEIGHT;
        disp_drv.flush_cb = disp_flush_callback_static; 
        disp_drv.draw_buf = &disp_buf;
        lv_disp_drv_register(&disp_drv);

        static lv_indev_drv_t indev_drv; 
        lv_indev_drv_init(&indev_drv);
        indev_drv.type = LV_INDEV_TYPE_POINTER; 
        indev_drv.read_cb = touchpad_read_callback_static;
        lv_indev_drv_register(&indev_drv);
        AppLog::info("Display", "DisplayManager initialized.");
    }

private:
    static void disp_flush_callback_static(lv_disp_drv_t *d, const lv_area_t *a, lv_color_t *c) { 
        if (g_displayManager) g_displayManager->disp_flush_callback(d,a,c); 
    }
    void disp_flush_callback(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
        uint32_t w = (area->x2 - area->x1 + 1); 
        uint32_t h = (area->y2 - area->y1 + 1);
        tft.startWrite(); 
        tft.setAddrWindow(area->x1, area->y1, w, h);
        tft.pushColors((uint16_t *)color_p, w * h, true); 
        tft.endWrite();
        lv_disp_flush_ready(disp_drv);
    }

    static void touchpad_read_callback_static(lv_indev_drv_t *i, lv_indev_data_t *d) { 
        if (g_displayManager) g_displayManager->touchpad_read_callback(i,d); 
    }
    void touchpad_read_callback(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
        uint16_t tX, tY; 
        bool touched = tft.getTouch(&tX, &tY, 600); // Ngưỡng áp suất 600
        data->state = touched ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
        if (touched) { data->point.x = tX; data->point.y = tY; }
    }
};
lv_disp_draw_buf_t DisplayManager::disp_buf; // Định nghĩa thành viên static
lv_color_t DisplayManager::buf_1[SCREEN_WIDTH * 20]; // Định nghĩa thành viên static

// -----------------------------------------------------------------------------
// Lớp UIManager: Quản lý các yếu tố UI và cập nhật
// -----------------------------------------------------------------------------
class UIManager {
public:
    lv_obj_t *label_hour, *label_minute, *label_colon_hm, *label_colon_ms, *label_seconds;
    lv_obj_t *calendar_obj, *label_ntp_status, *label_ip_address;
    lv_obj_t *label_temperature, *label_humidity; // Label mới cho DHT11

    int prev_h = -1, prev_m = -1, prev_s = -1;
    int prev_cal_year = -1, prev_cal_month = -1, prev_cal_day = -1;
    static char calendar_day_text_buffer[4]; 
    lv_timer_t *colon_blink_timer = nullptr;
    bool colon_visible = true; // Trạng thái hiển thị chung cho cả hai dấu hai chấm

    // Styles - nên là static nếu chúng được chia sẻ và khởi tạo một lần
    static lv_style_t style_time_elements, style_calendar_main, style_calendar_header_text;
    static lv_style_t style_calendar_date_nums, style_calendar_today_date;
    static lv_style_t style_bottom_info_ntp, style_bottom_info_ip, style_sensor_text; 

    UIManager() {}

    static void init_styles_static() { // Static để gọi một lần
        lv_style_init(&style_time_elements);
        lv_style_set_text_font(&style_time_elements, &lv_font_montserrat_38);
        lv_style_set_text_color(&style_time_elements, lv_color_white());

        // Style cho văn bản cảm biến
        lv_style_init(&style_sensor_text);
        lv_style_set_text_font(&style_sensor_text, &lv_font_montserrat_16); 
        lv_style_set_text_color(&style_sensor_text, lv_palette_lighten(LV_PALETTE_GREY, 1)); 

        lv_style_init(&style_calendar_main); 
        lv_style_set_bg_opa(&style_calendar_main, LV_OPA_TRANSP);
        lv_style_set_border_width(&style_calendar_main, 0);
        lv_style_set_pad_all(&style_calendar_main, 2);

        lv_style_init(&style_calendar_header_text);
        lv_style_set_text_font(&style_calendar_header_text, &lv_font_montserrat_16);
        lv_style_set_text_color(&style_calendar_header_text, lv_color_white());

        lv_style_init(&style_calendar_date_nums);
        lv_style_set_text_font(&style_calendar_date_nums, &lv_font_montserrat_14);
        lv_style_set_text_color(&style_calendar_date_nums, lv_color_white());
        lv_style_set_bg_opa(&style_calendar_date_nums, LV_OPA_TRANSP);
        lv_style_set_border_width(&style_calendar_date_nums, 0);
        lv_style_set_outline_width(&style_calendar_date_nums, 0);
        lv_style_set_radius(&style_calendar_date_nums, 0);

        lv_style_init(&style_calendar_today_date);
        lv_style_set_text_color(&style_calendar_today_date, lv_palette_main(LV_PALETTE_BLUE));
        lv_style_set_text_font(&style_calendar_today_date, &lv_font_montserrat_14);
        lv_style_set_bg_opa(&style_calendar_today_date, LV_OPA_TRANSP);
        lv_style_set_border_width(&style_calendar_today_date, 0);
        lv_style_set_outline_width(&style_calendar_today_date, 0);
        lv_style_set_radius(&style_calendar_today_date, 0);

        lv_style_init(&style_bottom_info_ntp);
        lv_style_set_text_font(&style_bottom_info_ntp, &lv_font_montserrat_16);
        lv_style_set_text_color(&style_bottom_info_ntp, lv_palette_main(LV_PALETTE_CYAN));

        lv_style_init(&style_bottom_info_ip);
        lv_style_set_text_font(&style_bottom_info_ip, &lv_font_montserrat_14);
        lv_style_set_text_color(&style_bottom_info_ip, lv_palette_darken(LV_PALETTE_GREY, 2));
        AppLog::info("UI", "Styles initialized.");
    }

    void create_ui() {
        if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY) != pdTRUE) {
            AppLog::error("UI", "Failed to get mutex for create_ui."); return;
        }
        lv_obj_t *scr = lv_scr_act(); lv_obj_clean(scr);
        lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);

        lv_obj_t *main_flex_container = lv_obj_create(scr);
        lv_obj_remove_style_all(main_flex_container);
        lv_obj_set_size(main_flex_container, lv_pct(100), LV_SIZE_CONTENT);
        lv_obj_set_layout(main_flex_container, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(main_flex_container, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(main_flex_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(main_flex_container, 8, 0); 
        lv_obj_center(main_flex_container);

        // Time Container
        lv_obj_t* time_container = lv_obj_create(main_flex_container);
        lv_obj_remove_style_all(time_container);
        lv_obj_set_size(time_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_layout(time_container, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(time_container, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(time_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(time_container, 5, 0);

        label_hour = lv_label_create(time_container);
        lv_obj_add_style(label_hour, &style_time_elements, 0); lv_label_set_text(label_hour, "--");
        label_colon_hm = lv_label_create(time_container);
        lv_obj_add_style(label_colon_hm, &style_time_elements, 0); lv_label_set_text(label_colon_hm, ":");
        label_minute = lv_label_create(time_container);
        lv_obj_add_style(label_minute, &style_time_elements, 0); lv_label_set_text(label_minute, "--");
        label_colon_ms = lv_label_create(time_container);
        lv_obj_add_style(label_colon_ms, &style_time_elements, 0); lv_label_set_text(label_colon_ms, ":");
        label_seconds = lv_label_create(time_container);
        lv_obj_add_style(label_seconds, &style_time_elements, 0); lv_label_set_text(label_seconds, "--");

        // Sensor Data Container
        lv_obj_t* sensor_container = lv_obj_create(main_flex_container);
        lv_obj_remove_style_all(sensor_container);
        lv_obj_set_width(sensor_container, lv_pct(90)); 
        lv_obj_set_height(sensor_container, LV_SIZE_CONTENT);
        lv_obj_set_layout(sensor_container, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(sensor_container, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(sensor_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER); 
        lv_obj_set_style_pad_hor(sensor_container, 10, 0); 

        label_temperature = lv_label_create(sensor_container);
        lv_obj_add_style(label_temperature, &style_sensor_text, 0);
        lv_label_set_text(label_temperature, "T: --.- C");

        label_humidity = lv_label_create(sensor_container);
        lv_obj_add_style(label_humidity, &style_sensor_text, 0);
        lv_label_set_text(label_humidity, "H: --.- %");

        // Calendar
        calendar_obj = lv_calendar_create(main_flex_container);
        lv_obj_set_size(calendar_obj, 220, LV_SIZE_CONTENT);
        lv_obj_add_style(calendar_obj, &style_calendar_main, 0);
        lv_obj_t *btnm = lv_calendar_get_btnmatrix(calendar_obj);
        if (btnm) {
            lv_obj_add_style(btnm, &style_calendar_date_nums, LV_PART_ITEMS | LV_STATE_DEFAULT);
            lv_obj_add_style(btnm, &style_calendar_today_date, LV_PART_ITEMS | LV_STATE_CHECKED);
            lv_obj_add_style(btnm, &style_calendar_today_date, LV_PART_ITEMS | LV_STATE_PRESSED | LV_STATE_CHECKED);
            lv_obj_add_style(btnm, &style_calendar_today_date, LV_PART_ITEMS | LV_STATE_FOCUSED | LV_STATE_CHECKED);
            lv_obj_add_event_cb(btnm, calendar_day_format_event_cb_static, LV_EVENT_DRAW_PART_BEGIN, this);
        }
        lv_calendar_set_showed_date(calendar_obj, 2024, 1);
        lv_calendar_set_today_date(calendar_obj, 2024, 1, 1);

        // Status Labels
        label_ntp_status = lv_label_create(scr); 
        lv_obj_add_style(label_ntp_status, &style_bottom_info_ntp, 0);
        lv_label_set_text(label_ntp_status, "Initializing...");
        lv_obj_align(label_ntp_status, LV_ALIGN_BOTTOM_MID, 0, -25);

        label_ip_address = lv_label_create(scr);
        lv_obj_add_style(label_ip_address, &style_bottom_info_ip, 0);
        lv_label_set_text(label_ip_address, "IP: N/A");
        lv_obj_align(label_ip_address, LV_ALIGN_BOTTOM_LEFT, 5, -5);

        if (colon_blink_timer) lv_timer_del(colon_blink_timer);
        colon_blink_timer = lv_timer_create(colon_blink_timer_cb_static, 500, this);
        
        xSemaphoreGive(lvgl_mutex);
        AppLog::info("UI", "UI elements created.");
    }

    void update_time_display_labels(const struct tm &timeinfo_utc) {
        if (xSemaphoreTake(lvgl_mutex, UI_UPDATE_MUTEX_TIMEOUT) != pdTRUE) {
            AppLog::warn("UI", "Failed to get mutex for time display update."); return;
        }
        char buf[3]; struct tm timeinfo_gmt7 = timeinfo_utc;
        time_t raw_time_utc_val = mktime(&timeinfo_gmt7);
        raw_time_utc_val += 7 * 3600; localtime_r(&raw_time_utc_val, &timeinfo_gmt7);
        if (label_hour && timeinfo_gmt7.tm_hour != prev_h) {
            snprintf(buf, sizeof(buf), "%02d", timeinfo_gmt7.tm_hour);
            lv_label_set_text(label_hour, buf); prev_h = timeinfo_gmt7.tm_hour;
        }
        if (label_minute && timeinfo_gmt7.tm_min != prev_m) {
            snprintf(buf, sizeof(buf), "%02d", timeinfo_gmt7.tm_min);
            lv_label_set_text(label_minute, buf); prev_m = timeinfo_gmt7.tm_min;
        }
        if (label_seconds && timeinfo_gmt7.tm_sec != prev_s) {
            snprintf(buf, sizeof(buf), "%02d", timeinfo_gmt7.tm_sec);
            lv_label_set_text(label_seconds, buf); prev_s = timeinfo_gmt7.tm_sec;
        }
        if (calendar_obj) { 
            int current_year = timeinfo_gmt7.tm_year + 1900;
            int current_month = timeinfo_gmt7.tm_mon + 1;
            int current_day = timeinfo_gmt7.tm_mday;
            if (current_day != prev_cal_day || current_month != prev_cal_month || current_year != prev_cal_year) {
                lv_calendar_set_today_date(calendar_obj, current_year, current_month, current_day);
                const lv_calendar_date_t *shown_date = lv_calendar_get_showed_date(calendar_obj);
                if (shown_date && (shown_date->year != current_year || shown_date->month != current_month)) {
                    lv_calendar_set_showed_date(calendar_obj, current_year, current_month);
                }
                prev_cal_day = current_day; prev_cal_month = current_month; prev_cal_year = current_year;
            }
        }
        xSemaphoreGive(lvgl_mutex);
    }
    
    void update_sensor_display(float temp, float humidity_val) { // Renamed humidity to humidity_val to avoid conflict
        if (xSemaphoreTake(lvgl_mutex, UI_UPDATE_MUTEX_TIMEOUT) != pdTRUE) {
            AppLog::warn("UI_Sensor", "Failed to get mutex for sensor display."); return;
        }
        char temp_buf[16]; char hum_buf[16];
        if (label_temperature) {
            if (isnan(temp)) lv_label_set_text(label_temperature, "T: N/A C");
            else { snprintf(temp_buf, sizeof(temp_buf), "T: %.1f C", temp); lv_label_set_text(label_temperature, temp_buf); }
        }
        if (label_humidity) {
             if (isnan(humidity_val)) lv_label_set_text(label_humidity, "H: N/A %%");
            else { snprintf(hum_buf, sizeof(hum_buf), "H: %.1f %%", humidity_val); lv_label_set_text(label_humidity, hum_buf); }
        }
        xSemaphoreGive(lvgl_mutex);
    }

    void update_status_text(lv_obj_t* lbl, const char* txt, const char* mod) { 
        if (xSemaphoreTake(lvgl_mutex, UI_UPDATE_MUTEX_TIMEOUT) == pdTRUE) {
            if (lbl) lv_label_set_text(lbl, txt); xSemaphoreGive(lvgl_mutex);
        } else { AppLog::warn("UI", "Failed to get mutex for %s status label", mod); }
    }
    void update_ntp_status(const char* t) { update_status_text(label_ntp_status, t, "NTP"); }
    void update_ip_address(const char* t) { update_status_text(label_ip_address, t, "IP"); }
    void set_calendar_after_sync(const struct tm& t) { 
         if (xSemaphoreTake(lvgl_mutex, UI_UPDATE_MUTEX_TIMEOUT) != pdTRUE) {
            AppLog::warn("UI", "Failed to get mutex for calendar sync init."); return;
        }
        if (calendar_obj) {
            prev_cal_year = t.tm_year + 1900; prev_cal_month = t.tm_mon + 1; prev_cal_day = t.tm_mday;
            lv_calendar_set_today_date(calendar_obj, prev_cal_year, prev_cal_month, prev_cal_day);
            lv_calendar_set_showed_date(calendar_obj, prev_cal_year, prev_cal_month);
            AppLog::info("UI", "Calendar initialized post-NTP: %02d/%02d/%d", prev_cal_day, prev_cal_month, prev_cal_year);
        } xSemaphoreGive(lvgl_mutex);
    }
private:
    static void colon_blink_timer_cb_static(lv_timer_t *timer) {
        UIManager* self = (UIManager*)timer->user_data; 
        if (self) self->colon_blink_timer_cb_member(timer);
    }
    void colon_blink_timer_cb_member(lv_timer_t *timer) {
        if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(20)) == pdTRUE) { // Tăng nhẹ timeout
            colon_visible = !colon_visible; 
            if (label_colon_hm) {
                lv_obj_set_style_opa(label_colon_hm, colon_visible ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
            }
            if (label_colon_ms) { // Điều khiển cả dấu hai chấm MM:SS
                lv_obj_set_style_opa(label_colon_ms, colon_visible ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
            }
            xSemaphoreGive(lvgl_mutex);
        }
    }
    static void calendar_day_format_event_cb_static(lv_event_t * e) { 
        UIManager* self = (UIManager*)lv_event_get_user_data(e);
        lv_obj_draw_part_dsc_t * dsc = lv_event_get_draw_part_dsc(e);
        if (dsc->part != LV_PART_ITEMS || !dsc->text || dsc->text[0] == '\0') return;
        char* endptr; long day_val = strtol(dsc->text, &endptr, 10);
        if (*endptr == '\0' && day_val >= 1 && day_val <= 31 && day_val < 10) {
            char* target_buffer = (self) ? self->calendar_day_text_buffer : UIManager::calendar_day_text_buffer;
            snprintf(target_buffer, 4, "%02ld", day_val);
            dsc->text = target_buffer;
        }
    }
};
char UIManager::calendar_day_text_buffer[4];
lv_style_t UIManager::style_time_elements; lv_style_t UIManager::style_calendar_main;
lv_style_t UIManager::style_calendar_header_text; lv_style_t UIManager::style_calendar_date_nums;
lv_style_t UIManager::style_calendar_today_date; lv_style_t UIManager::style_bottom_info_ntp;
lv_style_t UIManager::style_bottom_info_ip; lv_style_t UIManager::style_sensor_text;

class NetworkManager {
public:
    NetworkManager() {} // UDP instance is global g_ntpUDP
    bool connect_wifi() { 
        AppLog::info("WiFi", "Connecting to %s...", WIFI_SSID);
        if(g_uiManager) {
            g_uiManager->update_ntp_status("WiFi Connecting...");
            g_uiManager->update_ip_address("IP: ...");
        }
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD); int retries = 0; const int MAX_RETRIES = 30; 
        while (WiFi.status() != WL_CONNECTED && retries < MAX_RETRIES) {
            vTaskDelay(pdMS_TO_TICKS(500)); Serial.print("."); retries++;
        } Serial.println(); 
        if (WiFi.status() == WL_CONNECTED) {
            AppLog::info("WiFi", "Connected. IP: %s", WiFi.localIP().toString().c_str());
            if(g_uiManager) g_uiManager->update_ip_address(WiFi.localIP().toString().c_str());
            return true;
        } else {
            AppLog::error("WiFi", "Connection failed.");
            if(g_uiManager) {
                g_uiManager->update_ip_address("IP: N/A");
                g_uiManager->update_ntp_status("WiFi Error");
            } return false;
        }
    }
    bool is_connected() { return WiFi.status() == WL_CONNECTED; }
};

class TimeManager {
public:
    NTPClient* ntp_client = nullptr;
    TimeManager() {}
    bool init_ntp_client(const char* server) {
        if (ntp_client != nullptr) { delete ntp_client; ntp_client = nullptr; }
        ntp_client = new NTPClient(g_ntpUDP, server, 0, 60000); 
        ntp_client->begin(); return ntp_client != nullptr;
    }
    bool sync_time(bool isInitialAttempt) {
        if (!g_networkManager || !g_networkManager->is_connected()) {
            AppLog::warn("NTP", "WiFi not connected. Skipping NTP sync.");
            if(g_uiManager) g_uiManager->update_ntp_status("NTP: No WiFi"); return false;
        }
        AppLog::info("NTP", "Starting %s sync attempt.", isInitialAttempt ? "initial" : "periodic");
        if(g_uiManager) g_uiManager->update_ntp_status("NTP Syncing... (GMT+7)");
        const int MAX_RETRIES_PER_SERVER = 2; const int RETRY_DELAY_MS = 500;
        for (const char* server : NTP_SERVER_LIST) {
            AppLog::info("NTP", "Trying server: %s", server);
            if (!init_ntp_client(server)) {
                AppLog::error("NTP", "Failed to initialize NTPClient for server %s", server); continue; 
            }
            for (int retry = 0; retry < MAX_RETRIES_PER_SERVER; ++retry) {
                AppLog::info("NTP", "Attempting forceUpdate %d/%d for %s...", retry + 1, MAX_RETRIES_PER_SERVER, server);
                if (ntp_client->forceUpdate()) {
                    unsigned long epochTime = ntp_client->getEpochTime(); struct tm timeinfo_check;
                    gmtime_r((const time_t *)&epochTime, &timeinfo_check);
                    if (timeinfo_check.tm_year > (2000 - 1900)) {
                        AppLog::info("NTP", "SUCCESS with %s. Epoch: %lu", server, epochTime);
                        initial_ntp_sync_done = true;
                        if(g_uiManager) g_uiManager->update_ntp_status("Synced (GMT+7)");
                        return true;
                    } else { AppLog::warn("NTP", "forceUpdate success but got invalid time (year %d) from %s.", timeinfo_check.tm_year + 1900, server); }
                } else { AppLog::warn("NTP", "forceUpdate failed for %s (retry %d/%d).", server, retry + 1, MAX_RETRIES_PER_SERVER); }
                vTaskDelay(pdMS_TO_TICKS(RETRY_DELAY_MS));
            }
        }
        AppLog::error("NTP", "All servers failed."); initial_ntp_sync_done = false;
        if(g_uiManager) g_uiManager->update_ntp_status("NTP Fail (GMT+7)");
        if (ntp_client != nullptr) { delete ntp_client; ntp_client = nullptr; } return false;
    }
    bool get_current_utc_time(struct tm* t) {
        if (!initial_ntp_sync_done || !ntp_client) return false;
        unsigned long epochTime = ntp_client->getEpochTime();
        gmtime_r((const time_t *)&epochTime, t);
        return t->tm_year > (2000 - 1900); 
    }
};

void lvgl_task_handler_rtos(void *p) {
    AppLog::info("RTOS", "LVGL task started."); TickType_t xNWakeTime;
    const TickType_t xP = pdMS_TO_TICKS(LVGL_TICK_PERIOD_MS);
    for (;;) { xNWakeTime = xTaskGetTickCount();
        if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY) == pdTRUE) {
            lv_tick_inc(LVGL_TICK_PERIOD_MS); lv_timer_handler(); xSemaphoreGive(lvgl_mutex);
        } else { AppLog::error("RTOS", "LVGL_Task failed to get mutex (critical error).");}
        vTaskDelayUntil(&xNWakeTime, xP);
    }
}
void network_time_task_handler_rtos(void *p) {
    AppLog::info("RTOS", "Network/Time task started."); vTaskDelay(pdMS_TO_TICKS(500)); 
    g_networkManager = new NetworkManager(); g_timeManager = new TimeManager();       
    unsigned long last_periodic_ntp_sync_ms = 0;
    const unsigned long NTP_PERIODIC_SYNC_INTERVAL_MS = 1 * 60 * 60 * 1000; 
    const unsigned long WIFI_RECONNECT_INTERVAL_MS = 30 * 1000;
    unsigned long last_wifi_check_time_ms = 0;
    if (g_networkManager->connect_wifi()) { g_timeManager->sync_time(true); }
    for (;;) {
        if (g_networkManager->is_connected()) {
            if (!initial_ntp_sync_done) { AppLog::info("RTOS_NetTime", "WiFi connected, ensuring NTP sync.");
                 if (g_timeManager->sync_time(true)) { struct tm timeinfo_utc;
                    if (g_timeManager->get_current_utc_time(&timeinfo_utc)) {
                        struct tm timeinfo_gmt7 = timeinfo_utc; time_t raw_time_utc_val = mktime(&timeinfo_gmt7);
                        raw_time_utc_val += 7 * 3600; localtime_r(&raw_time_utc_val, &timeinfo_gmt7);
                        if(g_uiManager) g_uiManager->set_calendar_after_sync(timeinfo_gmt7);
                    }
                 } last_periodic_ntp_sync_ms = millis(); 
            }
            if (millis() - last_periodic_ntp_sync_ms > NTP_PERIODIC_SYNC_INTERVAL_MS) {
                AppLog::info("RTOS_NetTime", "Performing periodic NTP re-sync.");
                g_timeManager->sync_time(false); last_periodic_ntp_sync_ms = millis();
            } last_wifi_check_time_ms = millis(); vTaskDelay(pdMS_TO_TICKS(1000));
        } else { 
            if(g_uiManager) { g_uiManager->update_ntp_status("WiFi Error"); g_uiManager->update_ip_address("IP: N/A");}
            initial_ntp_sync_done = false; 
            if (g_timeManager && g_timeManager->ntp_client) { delete g_timeManager->ntp_client; g_timeManager->ntp_client = nullptr;}
            if (millis() - last_wifi_check_time_ms > WIFI_RECONNECT_INTERVAL_MS) {
                AppLog::info("RTOS_NetTime", "WiFi disconnected. Attempting reconnect...");
                if(g_networkManager) g_networkManager->connect_wifi(); 
                last_wifi_check_time_ms = millis();
            } vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
}

void setup() {
    Serial.begin(115200);
    AppLog::info("System", "ESP32 NTP Clock with DHT11 Starting...");
    dht.begin(); // Khởi tạo cảm biến DHT
    lvgl_mutex = xSemaphoreCreateMutex();
    if (lvgl_mutex == NULL) {
        AppLog::error("System", "Failed to create LVGL mutex! Halting.");
        while(1) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    g_displayManager = new DisplayManager(); g_displayManager->init();
    UIManager::init_styles_static(); 
    g_uiManager = new UIManager(); g_uiManager->create_ui();     
    xTaskCreatePinnedToCore(lvgl_task_handler_rtos, "LVGL_Task", 8192, NULL, 2, &lvgl_task_handle, 0);
    AppLog::info("System", "LVGL Task created on Core 0.");
    xTaskCreatePinnedToCore(network_time_task_handler_rtos, "NetTime_Task", 8192, NULL, 1, &network_task_handle, 1);
    AppLog::info("System", "Network/Time Task created on Core 1.");
    AppLog::info("System", "Setup complete.");
}

void loop() {
    // Cập nhật hiển thị thời gian
    if (initial_ntp_sync_done && g_timeManager && g_uiManager) {
        static unsigned long last_display_update_ms = 0;
        if (millis() - last_display_update_ms >= 1000) { // Cập nhật mỗi giây
            last_display_update_ms = millis(); struct tm timeinfo_utc;
            if (g_timeManager->get_current_utc_time(&timeinfo_utc)) {
                g_uiManager->update_time_display_labels(timeinfo_utc);
            } else { // Nếu không lấy được thời gian từ NTP (ví dụ: client bị lỗi sau khi đã đồng bộ)
                 if (g_networkManager && g_networkManager->is_connected()) { // Chỉ hiển thị lỗi nếu WiFi vẫn kết nối
                     if(g_uiManager) g_uiManager->update_ntp_status("NTP Bad Time (GMT+7)");
                }
                // initial_ntp_sync_done = false; // Có thể đặt lại để task network thử đồng bộ lại
            }
        }
    }

    // Đọc và cập nhật dữ liệu cảm biến DHT11
    if (g_uiManager && (millis() - last_dht_read_ms >= DHT_READ_INTERVAL_MS)) {
        last_dht_read_ms = millis();
        float humidity = dht.readHumidity();
        float temperature = dht.readTemperature(); // Mặc định là Celsius

        if (isnan(humidity) || isnan(temperature)) {
            AppLog::warn("DHT11", "Failed to read from DHT sensor!");
            g_uiManager->update_sensor_display(NAN, NAN); // Truyền NAN để hiển thị lỗi
        } else {
            // AppLog::info("DHT11", "Temp: %.1f C, Hum: %.1f %%", temperature, humidity); // Ghi log nếu cần debug
            g_uiManager->update_sensor_display(temperature, humidity);
        }
    }

    vTaskDelay(pdMS_TO_TICKS(200)); // Delay vòng lặp chính, nhường cho các task khác
}

