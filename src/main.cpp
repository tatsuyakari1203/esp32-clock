#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TFT_eSPI.h>
#include "lvgl.h"
#include <vector>

// --- LVGL Log Printer Implementation ---
#if LV_USE_LOG && defined(LV_LOG_PRINTF) && LV_LOG_PRINTF == my_lvgl_log_printer
void my_lvgl_log_printer(const char *buf) {
    Serial.print(buf);
}
#endif

// --- Configuration Constants ---
const char* WIFI_SSID = "2.4 KariS";
const char* WIFI_PASSWORD = "12123402";

WiFiUDP ntpUDP;
NTPClient* timeClient_ptr = nullptr;

std::vector<const char*> ntpServerList = {
    "0.vn.pool.ntp.org", "1.vn.pool.ntp.org", "2.vn.pool.ntp.org",
    "0.asia.pool.ntp.org", "1.asia.pool.ntp.org", "2.asia.pool.ntp.org",
    "time.google.com", "pool.ntp.org",
    "1.ntp.vnix.vn", "2.ntp.vnix.vn"
};

#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 320

// --- Global Variables for TFT and LVGL ---
TFT_eSPI tft = TFT_eSPI();
static lv_disp_draw_buf_t disp_buf;
static lv_color_t buf_1[SCREEN_WIDTH * 20];

lv_obj_t *label_hour;
lv_obj_t *label_minute;
lv_obj_t *label_colon_hm;
lv_obj_t *label_colon_ms;
lv_obj_t *label_seconds;
lv_obj_t *calendar_obj; 
lv_obj_t *label_ntp_status;
lv_obj_t *label_ip_address;

int prev_h = -1, prev_m = -1, prev_s = -1;

static int prev_cal_year = -1, prev_cal_month = -1, prev_cal_day = -1;


bool initial_sync_done = false;
TaskHandle_t lvgl_task_handle = NULL;
TaskHandle_t network_task_handle = NULL;
SemaphoreHandle_t lvgl_mutex = NULL;

// --- Styles ---
static lv_style_t style_time_elements;
static lv_style_t style_calendar_main;
static lv_style_t style_calendar_header_text; 
static lv_style_t style_calendar_date_nums; // Style chung cho các items (tên thứ, ngày thường)
static lv_style_t style_calendar_today_date;    // Style cho ngày hôm nay
static lv_style_t style_bottom_info_ntp;
static lv_style_t style_bottom_info_ip;

// Buffer cho việc định dạng ngày trong lịch
static char calendar_day_text_buffer[4]; 

// --- Colon Blinking (using opacity) ---
static bool colon_hm_visible = true;
lv_timer_t *colon_blink_timer = NULL;

static void colon_blink_timer_cb(lv_timer_t *timer) {
    if (label_colon_hm) {
        colon_hm_visible = !colon_hm_visible;
        if (colon_hm_visible) {
            lv_obj_set_style_opa(label_colon_hm, LV_OPA_COVER, 0);
        } else {
            lv_obj_set_style_opa(label_colon_hm, LV_OPA_TRANSP, 0);
        }
    }
}

static void calendar_day_format_event_cb(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e); // Đây là button matrix
    lv_obj_draw_part_dsc_t * dsc = lv_event_get_draw_part_dsc(e);

    if (dsc->part != LV_PART_ITEMS) {
        return;
    }

    if (!dsc->text || dsc->text[0] == '\0') { // Không có text hoặc text rỗng, bỏ qua
        return;
    }

    // Kiểm tra xem text có phải là một số (ngày)
    char* endptr;
    long day_val = strtol(dsc->text, &endptr, 10);

    // Nếu strtol đã xử lý toàn bộ chuỗi và đó là một số ngày hợp lệ
    if (*endptr == '\0' && day_val >= 1 && day_val <= 31) {
        if (day_val < 10) { // Ngày có một chữ số
            snprintf(calendar_day_text_buffer, sizeof(calendar_day_text_buffer), "%02ld", day_val);
            dsc->text = calendar_day_text_buffer; // Trỏ tới buffer tĩnh của chúng ta
        }
    }
}

// --- LVGL Callback Functions ---
void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)color_p, w * h, true);
    tft.endWrite();
    lv_disp_flush_ready(disp_drv);
}

void my_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    uint16_t touchX, touchY;
    bool touched = tft.getTouch(&touchX, &touchY, 600);
    if (!touched) {
        data->state = LV_INDEV_STATE_REL;
    } else {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touchX;
        data->point.y = touchY;
    }
}

// --- Initialization Functions ---
void init_display() {
    tft.begin();
    tft.setRotation(0);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
}

void init_lvgl_drivers() {
    lv_init();
    lv_disp_draw_buf_init(&disp_buf, buf_1, NULL, SCREEN_WIDTH * 20);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);
}

void init_wifi() {
    Serial.printf("Connecting to %s ", WIFI_SSID);
    if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(200)) == pdTRUE) { 
        if (label_ntp_status) lv_label_set_text(label_ntp_status, "WiFi Connecting...");
        if (label_ip_address) lv_label_set_text(label_ip_address, "IP: ...");
        xSemaphoreGive(lvgl_mutex);
    } else {
        Serial.println("[init_wifi] Failed to get LVGL mutex for status update (timeout 200ms).");
    }

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int connect_timeout = 0;
    while (WiFi.status() != WL_CONNECTED && connect_timeout < 20) { 
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
        connect_timeout++;
    }

    if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(200)) == pdTRUE) { 
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println(" CONNECTED");
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
            if (label_ip_address) {
                lv_label_set_text(label_ip_address, WiFi.localIP().toString().c_str());
            }
        } else {
            Serial.println(" FAILED to connect WiFi");
            if (label_ip_address) lv_label_set_text(label_ip_address, "IP: N/A");
            if (label_ntp_status) lv_label_set_text(label_ntp_status, "WiFi Error");
        }
        xSemaphoreGive(lvgl_mutex);
    } else {
         Serial.println("[init_wifi] Failed to get LVGL mutex for final status update (timeout 200ms).");
    }
}

bool syncTimeWithNTPClientList(bool isInitialAttempt) {
    const int MAX_FORCE_UPDATE_RETRIES = 2;
    const int RETRY_DELAY_MS = 500;

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[NTP Sync] WiFi not connected. Skipping NTP sync.");
        return false;
    }

    Serial.printf("[NTP Sync] Starting %s sync attempt.\n", isInitialAttempt ? "initial" : "periodic");
    if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (label_ntp_status) lv_label_set_text(label_ntp_status, "NTP Syncing... (GMT+7)");
        xSemaphoreGive(lvgl_mutex);
    } else {
        Serial.println("[NTP Sync] Failed to get LVGL mutex for 'Syncing' status.");
    }

    for (const char* currentServer : ntpServerList) {
        Serial.printf("[NTP Sync] Trying server: %s\n", currentServer);
        if (timeClient_ptr != nullptr) {
            delete timeClient_ptr;
            timeClient_ptr = nullptr;
        }
        timeClient_ptr = new NTPClient(ntpUDP, currentServer, 0, 60000);
        timeClient_ptr->begin();

        for (int retry = 0; retry < MAX_FORCE_UPDATE_RETRIES; ++retry) {
            Serial.printf("  Attempting forceUpdate %d/%d for %s...\n", retry + 1, MAX_FORCE_UPDATE_RETRIES, currentServer);
            if (timeClient_ptr->forceUpdate()) {
                unsigned long epochTime = timeClient_ptr->getEpochTime();
                struct tm timeinfo_check;
                gmtime_r((const time_t *)&epochTime, &timeinfo_check);
                if (timeinfo_check.tm_year > (2000 - 1900)) {
                    Serial.printf("[NTP Sync] SUCCESS with %s.\n", currentServer);
                    initial_sync_done = true;
                    if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                        if (label_ntp_status) lv_label_set_text(label_ntp_status, "Synced " LV_SYMBOL_WIFI " (GMT+7)");
                        xSemaphoreGive(lvgl_mutex);
                    } else {
                        Serial.println("[NTP Sync] Failed to get LVGL mutex for 'Synced' status.");
                    }
                    return true;
                } else {
                    Serial.printf("  forceUpdate success but got invalid time (year %d) from %s.\n", timeinfo_check.tm_year + 1900, currentServer);
                }
            } else {
                Serial.printf("  forceUpdate failed for %s (retry %d/%d).\n", currentServer, retry + 1, MAX_FORCE_UPDATE_RETRIES);
            }
            vTaskDelay(pdMS_TO_TICKS(RETRY_DELAY_MS));
        }
    }

    Serial.println("[NTP Sync] All servers failed.");
    initial_sync_done = false;
    if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (label_ntp_status) lv_label_set_text(label_ntp_status, "NTP Fail (GMT+7)");
        xSemaphoreGive(lvgl_mutex);
    } else {
        Serial.println("[NTP Sync] Failed to get LVGL mutex for 'Fail' status.");
    }
    if (timeClient_ptr != nullptr) {
        delete timeClient_ptr;
        timeClient_ptr = nullptr;
    }
    return false;
}

// --- UI Creation Function (CALENDAR MINIMAL DESIGN) ---
void create_clock_ui() {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr);
    lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);

    // --- Khởi tạo Styles ---
    lv_style_init(&style_time_elements);
    lv_style_set_text_font(&style_time_elements, &lv_font_montserrat_38);
    lv_style_set_text_color(&style_time_elements, lv_color_white());

    // Style cho khối lịch chính
    lv_style_init(&style_calendar_main);
    lv_style_set_bg_opa(&style_calendar_main, LV_OPA_TRANSP); 
    lv_style_set_border_width(&style_calendar_main, 0); 
    lv_style_set_pad_all(&style_calendar_main, 2); // Giảm padding để lịch gọn hơn

    // Style cho text header của lịch (Tháng Năm)
    lv_style_init(&style_calendar_header_text); 
    lv_style_set_text_font(&style_calendar_header_text, &lv_font_montserrat_16);
    lv_style_set_text_color(&style_calendar_header_text, lv_color_white()); // Chữ trắng

    // Style chung cho các items trong lịch (tên thứ, các ngày thường)
    lv_style_init(&style_calendar_date_nums); 
    lv_style_set_text_font(&style_calendar_date_nums, &lv_font_montserrat_14); // Font chung
    lv_style_set_text_color(&style_calendar_date_nums, lv_color_white());      // Chữ màu trắng
    lv_style_set_bg_opa(&style_calendar_date_nums, LV_OPA_TRANSP);             // Nền trong suốt
    lv_style_set_border_width(&style_calendar_date_nums, 0);                   // Không có viền
    lv_style_set_outline_width(&style_calendar_date_nums, 0);                  // Không có viền ngoài
    lv_style_set_radius(&style_calendar_date_nums, 0);                         // Không bo tròn

    // Style cho ngày hôm nay
    lv_style_init(&style_calendar_today_date);
    lv_style_set_text_color(&style_calendar_today_date, lv_palette_main(LV_PALETTE_BLUE)); // Chữ màu xanh
    lv_style_set_text_font(&style_calendar_today_date, &lv_font_montserrat_14);          // Cùng font

    // Nền hoàn toàn trong suốt
    lv_style_set_bg_opa(&style_calendar_today_date, LV_OPA_TRANSP);
    // lv_style_set_bg_color(&style_calendar_today_date, lv_color_black()); // Màu nền không quan trọng khi trong suốt

    // Không có viền
    lv_style_set_border_width(&style_calendar_today_date, 0);
    lv_style_set_border_opa(&style_calendar_today_date, LV_OPA_TRANSP);
    // lv_style_set_border_color(&style_calendar_today_date, lv_color_black()); // Màu viền không quan trọng

    // Không có đường viền ngoài (outline)
    lv_style_set_outline_width(&style_calendar_today_date, 0);
    lv_style_set_outline_opa(&style_calendar_today_date, LV_OPA_TRANSP);
    // lv_style_set_outline_color(&style_calendar_today_date, lv_color_black()); // Màu outline không quan trọng

    // Không có bóng
    lv_style_set_shadow_width(&style_calendar_today_date, 0);
    lv_style_set_shadow_opa(&style_calendar_today_date, LV_OPA_TRANSP);
    // lv_style_set_shadow_color(&style_calendar_today_date, lv_color_black()); // Màu bóng không quan trọng

    lv_style_set_radius(&style_calendar_today_date, 0); // Không bo tròn


    lv_style_init(&style_bottom_info_ntp);
    lv_style_set_text_font(&style_bottom_info_ntp, &lv_font_montserrat_16);
    lv_style_set_text_color(&style_bottom_info_ntp, lv_palette_main(LV_PALETTE_CYAN));

    lv_style_init(&style_bottom_info_ip);
    lv_style_set_text_font(&style_bottom_info_ip, &lv_font_montserrat_14);
    lv_style_set_text_color(&style_bottom_info_ip, lv_palette_darken(LV_PALETTE_GREY, 2));

    // 1. Main Vertical Flex Container
    lv_obj_t *main_vertical_container = lv_obj_create(scr);
    lv_obj_remove_style_all(main_vertical_container);
    lv_obj_set_size(main_vertical_container, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(main_vertical_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(main_vertical_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(main_vertical_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(main_vertical_container, 10, 0); 
    lv_obj_center(main_vertical_container);

    // 2. Time Container (HH:MM:SS)
    lv_obj_t* time_container = lv_obj_create(main_vertical_container);
    lv_obj_remove_style_all(time_container);
    lv_obj_set_size(time_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(time_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(time_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(time_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(time_container, 5, 0);

    label_hour = lv_label_create(time_container);
    lv_obj_add_style(label_hour, &style_time_elements, 0);
    lv_label_set_text(label_hour, "00");

    label_colon_hm = lv_label_create(time_container);
    lv_obj_add_style(label_colon_hm, &style_time_elements, 0);
    lv_label_set_text(label_colon_hm, ":");

    label_minute = lv_label_create(time_container);
    lv_obj_add_style(label_minute, &style_time_elements, 0);
    lv_label_set_text(label_minute, "00");

    label_colon_ms = lv_label_create(time_container);
    lv_obj_add_style(label_colon_ms, &style_time_elements, 0);
    lv_label_set_text(label_colon_ms, ":");
    lv_obj_set_style_opa(label_colon_ms, LV_OPA_COVER, 0);

    label_seconds = lv_label_create(time_container);
    lv_obj_add_style(label_seconds, &style_time_elements, 0);
    lv_label_set_text(label_seconds, "00");

    // 3. Calendar Widget
    calendar_obj = lv_calendar_create(main_vertical_container);
    lv_obj_set_size(calendar_obj, 220, LV_SIZE_CONTENT);
    lv_obj_add_style(calendar_obj, &style_calendar_main, 0);

    // Tạm thời loại bỏ style header text để tránh lỗi biên dịch nếu hàm get không tồn tại
    // lv_obj_t * calendar_header_label = lv_calendar_header_arrow_get_month_label(calendar_obj);
    // if(calendar_header_label) {
    //     lv_obj_add_style(calendar_header_label, &style_calendar_header_text, 0);
    // }

    lv_obj_t *btnm = lv_calendar_get_btnmatrix(calendar_obj);
    if (btnm) {
        // Áp dụng style cho các items (ngày và tên thứ) trên button matrix
        lv_obj_add_style(btnm, &style_calendar_date_nums, LV_PART_ITEMS | LV_STATE_DEFAULT);

        // Áp dụng style cho ngày hôm nay trên button matrix (ghi đè style_calendar_date_nums)
        lv_obj_add_style(btnm, &style_calendar_today_date, LV_PART_ITEMS | LV_STATE_CHECKED);
        lv_obj_add_style(btnm, &style_calendar_today_date, LV_PART_ITEMS | LV_STATE_PRESSED | LV_STATE_CHECKED);
        lv_obj_add_style(btnm, &style_calendar_today_date, LV_PART_ITEMS | LV_STATE_FOCUSED | LV_STATE_CHECKED);

        // Thêm event callback để định dạng số ngày
        lv_obj_add_event_cb(btnm, calendar_day_format_event_cb, LV_EVENT_DRAW_PART_BEGIN, NULL);
    } else {
        // Fallback hoặc ghi log lỗi nếu không lấy được button matrix
        // Nếu điều này xảy ra, bạn có thể thử lại cách cũ hoặc kiểm tra cấu hình LVGL
        Serial.println("Warning: Could not get calendar button matrix. Styling might be incomplete.");
        // Cách cũ (fallback):
        lv_obj_add_style(calendar_obj, &style_calendar_date_nums, LV_PART_ITEMS | LV_STATE_DEFAULT);
        lv_obj_add_style(calendar_obj, &style_calendar_today_date, LV_PART_ITEMS | LV_STATE_CHECKED);
        lv_obj_add_style(calendar_obj, &style_calendar_today_date, LV_PART_ITEMS | LV_STATE_PRESSED | LV_STATE_CHECKED);
        lv_obj_add_style(calendar_obj, &style_calendar_today_date, LV_PART_ITEMS | LV_STATE_FOCUSED | LV_STATE_CHECKED);
    }


    lv_calendar_set_showed_date(calendar_obj, 2024, 1);
    lv_calendar_set_today_date(calendar_obj, 2024, 1, 1);


    // 4. Status Labels
    label_ntp_status = lv_label_create(scr);
    lv_obj_add_style(label_ntp_status, &style_bottom_info_ntp, 0);
    lv_label_set_text(label_ntp_status, "Starting...");
    lv_obj_align(label_ntp_status, LV_ALIGN_BOTTOM_MID, 0, -25);

    label_ip_address = lv_label_create(scr);
    lv_obj_add_style(label_ip_address, &style_bottom_info_ip, 0);
    lv_label_set_text(label_ip_address, "IP: N/A");
    lv_obj_align(label_ip_address, LV_ALIGN_BOTTOM_LEFT, 5, -5);

    if (colon_blink_timer) {
        lv_timer_del(colon_blink_timer);
    }
    colon_blink_timer = lv_timer_create(colon_blink_timer_cb, 500, NULL);
}

// --- Time Display Update Function (WITH CALENDAR UPDATE) ---
void update_time_display(const struct tm &timeinfo_utc) {
    if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(150)) == pdTRUE) { 
        char buf[3];
        struct tm timeinfo_gmt7 = timeinfo_utc;
        time_t raw_time_utc = mktime(&timeinfo_gmt7);
        raw_time_utc += 7 * 3600;
        localtime_r(&raw_time_utc, &timeinfo_gmt7);

        if (label_hour && timeinfo_gmt7.tm_hour != prev_h) {
            snprintf(buf, sizeof(buf), "%02d", timeinfo_gmt7.tm_hour);
            lv_label_set_text(label_hour, buf);
            prev_h = timeinfo_gmt7.tm_hour;
        }
        if (label_minute && timeinfo_gmt7.tm_min != prev_m) {
            snprintf(buf, sizeof(buf), "%02d", timeinfo_gmt7.tm_min);
            lv_label_set_text(label_minute, buf);
            prev_m = timeinfo_gmt7.tm_min;
        }
        if (label_seconds && timeinfo_gmt7.tm_sec != prev_s) {
            snprintf(buf, sizeof(buf), "%02d", timeinfo_gmt7.tm_sec);
            lv_label_set_text(label_seconds, buf);
            prev_s = timeinfo_gmt7.tm_sec;
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
                prev_cal_day = current_day;
                prev_cal_month = current_month;
                prev_cal_year = current_year;
            }
        }
        xSemaphoreGive(lvgl_mutex);
    } else {
        Serial.println("[update_time_display] Failed to get LVGL mutex (timeout 150ms).");
    }
}

// --- RTOS Task Handlers ---
void lvgl_task_handler(void *pvParameters) {
    Serial.println("LVGL task started.");
    for (;;) {
        TickType_t xNextWakeTime = xTaskGetTickCount(); 
        if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            lv_tick_inc(5);
            lv_timer_handler();
            xSemaphoreGive(lvgl_mutex);
        } else {
             Serial.println("[lvgl_task_handler] Failed to get LVGL mutex.");
        }
        vTaskDelayUntil(&xNextWakeTime, pdMS_TO_TICKS(5)); 
    }
}

void network_task_handler(void *pvParameters) {
    Serial.println("Network task started.");
    unsigned long last_periodic_ntp_sync_local = 0;
    const unsigned long NTP_PERIODIC_SYNC_INTERVAL_LOCAL = 1 * 60 * 60 * 1000;
    const unsigned long WIFI_RECONNECT_INTERVAL = 30 * 1000;
    unsigned long last_wifi_check_time = 0;

    init_wifi();

    for (;;) {
        if (WiFi.status() == WL_CONNECTED) {
            if (!initial_sync_done) {
                Serial.println("[NetworkTask] WiFi connected, attempting initial NTP sync.");
                if(syncTimeWithNTPClientList(true)) {
                    if (calendar_obj && timeClient_ptr) {
                         if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(200)) == pdTRUE) { 
                            unsigned long epochTime = timeClient_ptr->getEpochTime();
                            struct tm timeinfo_utc;
                            gmtime_r((const time_t *)&epochTime, &timeinfo_utc);
                            struct tm timeinfo_gmt7 = timeinfo_utc;
                            time_t raw_time_utc_val = mktime(&timeinfo_gmt7);
                            raw_time_utc_val += 7 * 3600;
                            localtime_r(&raw_time_utc_val, &timeinfo_gmt7);

                            prev_cal_year = timeinfo_gmt7.tm_year + 1900;
                            prev_cal_month = timeinfo_gmt7.tm_mon + 1;
                            prev_cal_day = timeinfo_gmt7.tm_mday;
                            
                            lv_calendar_set_today_date(calendar_obj, prev_cal_year, prev_cal_month, prev_cal_day);
                            lv_calendar_set_showed_date(calendar_obj, prev_cal_year, prev_cal_month);
                            Serial.printf("[NetworkTask] Calendar initialized to: %02d/%02d/%d\n", prev_cal_day, prev_cal_month, prev_cal_year);
                            xSemaphoreGive(lvgl_mutex);
                         } else {
                            Serial.println("[NetworkTask] Failed to get LVGL mutex for calendar init.");
                         }
                    }
                }
                last_periodic_ntp_sync_local = millis();
            }
            if (millis() - last_periodic_ntp_sync_local > NTP_PERIODIC_SYNC_INTERVAL_LOCAL) {
                Serial.println("[NetworkTask] Performing periodic NTP re-sync.");
                syncTimeWithNTPClientList(false);
                last_periodic_ntp_sync_local = millis();
            }
            last_wifi_check_time = millis();
            vTaskDelay(pdMS_TO_TICKS(1000));

        } else {
            if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                if (label_ntp_status) lv_label_set_text(label_ntp_status, "WiFi Error");
                if (label_ip_address) lv_label_set_text(label_ip_address, "IP: N/A");
                xSemaphoreGive(lvgl_mutex);
            } else {
                Serial.println("[NetworkTask] Failed to get LVGL mutex for WiFi Error status.");
            }
            if (timeClient_ptr != nullptr) {
                delete timeClient_ptr;
                timeClient_ptr = nullptr;
            }
            initial_sync_done = false;
            if (millis() - last_wifi_check_time > WIFI_RECONNECT_INTERVAL) {
                Serial.println("[NetworkTask] WiFi disconnected. Attempting reconnect...");
                init_wifi();
                last_wifi_check_time = millis();
                if (WiFi.status() == WL_CONNECTED) {
                    Serial.println("[NetworkTask] WiFi reconnected. NTP sync will be attempted.");
                } else {
                    Serial.println("[NetworkTask] WiFi reconnect failed.");
                }
            }
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
}

// --- Setup Function ---
void setup() {
    Serial.begin(115200);
    Serial.println("ESP32 NTP Clock RTOS Version Starting (Calendar Minimal Design)...");

    lvgl_mutex = xSemaphoreCreateMutex();
    if (lvgl_mutex == NULL) {
        Serial.println("Error: Failed to create LVGL mutex!");
        while(1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    init_display();
    init_lvgl_drivers();

    if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY) == pdTRUE) {
        create_clock_ui();
        xSemaphoreGive(lvgl_mutex);
    } else {
        Serial.println("Failed to get LVGL mutex for initial create_clock_ui!");
        while(1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    xTaskCreatePinnedToCore(
        lvgl_task_handler, "LVGL_Task", 8192, NULL, 2, &lvgl_task_handle, 0); 
    Serial.println("LVGL Task created on Core 0.");

    xTaskCreatePinnedToCore(
        network_task_handler, "Network_Task", 8192, NULL, 1, &network_task_handle, 1); 
    Serial.println("Network Task created on Core 1.");
}

// --- Loop Function (Main Task - Arduino default task, Prio 1, usually Core 1) ---
unsigned long last_time_display_update_in_loop = 0;

void loop() {
    if (initial_sync_done && timeClient_ptr != nullptr) {
        if (millis() - last_time_display_update_in_loop >= 1000) {
            last_time_display_update_in_loop = millis();
            unsigned long epochTime = timeClient_ptr->getEpochTime();
            struct tm timeinfo_utc;
            gmtime_r((const time_t *)&epochTime, &timeinfo_utc);
            if (timeinfo_utc.tm_year > (2000 - 1900)) {
                update_time_display(timeinfo_utc);
            } else {
                if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(100)) == pdTRUE) { 
                    if (label_ntp_status) lv_label_set_text(label_ntp_status, "NTP Bad Time (GMT+7)");
                     xSemaphoreGive(lvgl_mutex);
                } else {
                    Serial.println("[loop] Failed to get LVGL mutex for NTP Bad Time status.");
                }
            }
        }
    }
    vTaskDelay(pdMS_TO_TICKS(200)); 
}

