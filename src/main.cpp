#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
// #include "esp_sntp.h" // REMOVED
#include <NTPClient.h>   // Using NTPClient
#include <WiFiUdp.h>     // Required for NTPClient
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

// NTP configuration
WiFiUDP ntpUDP;
NTPClient* timeClient_ptr = nullptr; // Pointer for dynamic NTPClient instances

std::vector<const char*> ntpServerList = {
    "0.vn.pool.ntp.org", "1.vn.pool.ntp.org", "2.vn.pool.ntp.org",
    "0.asia.pool.ntp.org", "1.asia.pool.ntp.org", "2.asia.pool.ntp.org",
    "time.google.com", "pool.ntp.org",
    "1.ntp.vnix.vn", "2.ntp.vnix.vn"
};
// int currentNtpServerIndex = -1; // No longer needed as we don't display specific server

// Screen dimensions
#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 320

// --- Global Variables for TFT and LVGL ---
TFT_eSPI tft = TFT_eSPI();
static lv_disp_draw_buf_t disp_buf;
static lv_color_t buf_1[SCREEN_WIDTH * 20]; // LVGL buffer (20 lines)

// Pointers to LVGL labels
lv_obj_t *label_hour;
lv_obj_t *label_minute;
lv_obj_t *label_colon_hm; 
lv_obj_t *label_seconds;
lv_obj_t *label_colon_ms;
lv_obj_t *label_date;
lv_obj_t *label_day_of_week;
lv_obj_t *label_ntp_status;
lv_obj_t *label_ip_address;
// lv_obj_t *label_wifi_rssi; // REMOVED WIFI RSSI LABEL
lv_obj_t *label_active_ntp_server;

// Variables to store previous time to detect changes (for animation)
int prev_h = -1, prev_m = -1, prev_s = -1;
char prev_date_str[20] = ""; 
char prev_dow_str[20] = "";  

// --- Global Variables for Time Sync Status & RTOS Task Handles ---
bool initial_sync_done = false; 
TaskHandle_t lvgl_task_handle = NULL;
TaskHandle_t network_task_handle = NULL;
SemaphoreHandle_t lvgl_mutex = NULL; // ADDED LVGL Mutex

// --- Animation Core Code (Section VII) ---
typedef struct {
    lv_obj_t *label_old; 
    lv_obj_t *label_new; 
    lv_coord_t y_start_old; 
    lv_coord_t y_start_new; 
    lv_coord_t y_end_old;   
    lv_coord_t y_end_new;   
    int32_t delta_y;      
} digit_anim_data_t;

static void digit_slide_fade_anim_cb(void *var, int32_t v) {
    digit_anim_data_t *anim_data = (digit_anim_data_t *)var;
    if (anim_data->label_old) {
        lv_obj_set_y(anim_data->label_old, anim_data->y_start_old - v);
        lv_obj_set_style_opa(anim_data->label_old, lv_map(v, 0, anim_data->delta_y, LV_OPA_COVER, LV_OPA_TRANSP), 0);
    }
    if (anim_data->label_new) {
        lv_obj_set_y(anim_data->label_new, anim_data->y_start_new - v);
        lv_obj_set_style_opa(anim_data->label_new, lv_map(v, 0, anim_data->delta_y, LV_OPA_TRANSP, LV_OPA_COVER), 0);
    }
}

static void digit_anim_ready_cb(lv_anim_t *a) {
    digit_anim_data_t *anim_data = (digit_anim_data_t *)a->var;
    if (anim_data->label_old) {
        lv_obj_del(anim_data->label_old); 
    }
    if (anim_data) {
      free(anim_data); 
    }
}

void animate_digit_change(lv_obj_t **current_label_obj_ptr, lv_coord_t x_pos, lv_coord_t y_pos, 
                          const char *new_digit_text, const lv_font_t *font, lv_color_t color, 
                          lv_coord_t slide_distance, uint32_t anim_time) {
    lv_obj_t *label_new = lv_label_create(lv_scr_act());
    lv_label_set_text(label_new, new_digit_text);
    lv_obj_set_style_text_font(label_new, font, 0);
    lv_obj_set_style_text_color(label_new, color, 0);
    lv_obj_set_pos(label_new, x_pos, y_pos + slide_distance);
    lv_obj_set_style_opa(label_new, LV_OPA_TRANSP, 0);

    digit_anim_data_t *anim_data = (digit_anim_data_t *)malloc(sizeof(digit_anim_data_t));
    if (!anim_data) return; 

    anim_data->label_old = *current_label_obj_ptr; 
    anim_data->label_new = label_new;
    anim_data->y_start_old = y_pos;                 
    anim_data->y_start_new = y_pos + slide_distance; 
    anim_data->y_end_old = y_pos - slide_distance;   
    anim_data->y_end_new = y_pos;                   
    anim_data->delta_y = slide_distance;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, anim_data); 
    lv_anim_set_exec_cb(&a, digit_slide_fade_anim_cb);
    lv_anim_set_values(&a, 0, slide_distance); 
    lv_anim_set_time(&a, anim_time);     
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out); 
    lv_anim_set_ready_cb(&a, digit_anim_ready_cb);  

    lv_anim_start(&a);
    *current_label_obj_ptr = label_new;
}

// --- Colon Blinking Animation ---
static bool colon_visible = true;
lv_timer_t *colon_blink_timer;

static void colon_blink_timer_cb(lv_timer_t *timer) {
    if (label_colon_hm) { 
        colon_visible = !colon_visible;
        if (colon_visible) {
            lv_obj_clear_flag(label_colon_hm, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(label_colon_hm, LV_OBJ_FLAG_HIDDEN);
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
    if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (label_ntp_status) lv_label_set_text(label_ntp_status, "WiFi Connecting...");
        if (label_ip_address) lv_label_set_text(label_ip_address, "IP: ..."); // Placeholder during connect
        xSemaphoreGive(lvgl_mutex);
    } else {
        Serial.println("[init_wifi] Failed to get LVGL mutex for status update.");
    }

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int connect_timeout = 0;
    while (WiFi.status() != WL_CONNECTED && connect_timeout < 20) { 
        vTaskDelay(pdMS_TO_TICKS(500)); // Use vTaskDelay as this might be called from a task
        Serial.print(".");
        connect_timeout++;
    }

    if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println(" CONNECTED");
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
            if (label_ip_address) { 
                lv_label_set_text(label_ip_address, WiFi.localIP().toString().c_str());
            }
            // NTP status will be updated by network_task_handler based on sync outcome
        } else {
            Serial.println(" FAILED to connect WiFi");
            if (label_ip_address) lv_label_set_text(label_ip_address, "IP: N/A");
            if (label_ntp_status) lv_label_set_text(label_ntp_status, "WiFi Error");
            // label_active_ntp_server was removed
        }
        xSemaphoreGive(lvgl_mutex);
    } else {
         Serial.println("[init_wifi] Failed to get LVGL mutex for final status update.");
    }
}

// --- NTP Sync Function using NTPClient and Server List (modified to use mutex) ---
bool syncTimeWithNTPClientList(bool isInitialAttempt) {
    const int MAX_FORCE_UPDATE_RETRIES = 3; // Reduced further for faster cycling if a server is bad
    const int RETRY_DELAY_MS = 500; 

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[NTP Sync] WiFi not connected. Skipping NTP sync.");
        return false;
    }

    Serial.printf("[NTP Sync] Starting %s sync attempt.\\n", isInitialAttempt ? "initial" : "periodic");
    if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (label_ntp_status) lv_label_set_text(label_ntp_status, "NTP Syncing... (GMT+7)");
        xSemaphoreGive(lvgl_mutex);
    } else {
        Serial.println("[NTP Sync] Failed to get LVGL mutex for 'Syncing' status.");
    }

    for (int i = 0; i < ntpServerList.size(); ++i) {
        const char* currentServer = ntpServerList[i];
        Serial.printf("[NTP Sync] Trying server: %s\\n", currentServer);

        if (timeClient_ptr != nullptr) {
            delete timeClient_ptr;
            timeClient_ptr = nullptr;
        }
        timeClient_ptr = new NTPClient(ntpUDP, currentServer, 0, 60000); 
        timeClient_ptr->begin();

        for (int retry = 0; retry < MAX_FORCE_UPDATE_RETRIES; ++retry) {
            Serial.printf("  Attempting forceUpdate %d/%d for %s...\\n", retry + 1, MAX_FORCE_UPDATE_RETRIES, currentServer);
            if (timeClient_ptr->forceUpdate()) {
                unsigned long epochTime = timeClient_ptr->getEpochTime();
                struct tm timeinfo_check;
                gmtime_r((const time_t *)&epochTime, &timeinfo_check);

                if (timeinfo_check.tm_year > (2000 - 1900)) { 
                    Serial.printf("[NTP Sync] SUCCESS with %s.\\n", currentServer);
                    initial_sync_done = true;
                    if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                        if (label_ntp_status) lv_label_set_text(label_ntp_status, "Synced " LV_SYMBOL_WIFI " (GMT+7)");
                        xSemaphoreGive(lvgl_mutex);
                    } else {
                        Serial.println("[NTP Sync] Failed to get LVGL mutex for 'Synced' status.");
                    }
                    return true;
                } else {
                    Serial.printf("  forceUpdate success but got invalid time (year %d) from %s.\\n", timeinfo_check.tm_year + 1900, currentServer);
                }
            } else {
                Serial.printf("  forceUpdate failed for %s (retry %d/%d).\\n", currentServer, retry + 1, MAX_FORCE_UPDATE_RETRIES);
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

// --- UI Creation Function ---
void create_clock_ui() {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr);
    lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);

    // --- Styles --- 
    static lv_style_t style_time_elements; 
    lv_style_init(&style_time_elements);
    lv_style_set_text_font(&style_time_elements, &lv_font_montserrat_38);
    lv_style_set_text_color(&style_time_elements, lv_color_white());

    static lv_style_t style_day_of_week; 
    lv_style_init(&style_day_of_week);
    lv_style_set_text_font(&style_day_of_week, &lv_font_montserrat_28);
    lv_style_set_text_color(&style_day_of_week, lv_palette_main(LV_PALETTE_GREY));

    static lv_style_t style_date_format; 
    lv_style_init(&style_date_format);
    lv_style_set_text_font(&style_date_format, &lv_font_montserrat_22);
    lv_style_set_text_color(&style_date_format, lv_palette_lighten(LV_PALETTE_GREY, 1));

    static lv_style_t style_bottom_info; 
    lv_style_init(&style_bottom_info);
    lv_style_set_text_font(&style_bottom_info, &lv_font_montserrat_14);
    lv_style_set_text_color(&style_bottom_info, lv_palette_darken(LV_PALETTE_GREY, 2));

    static lv_style_t style_main_status; 
    lv_style_init(&style_main_status);
    lv_style_set_text_font(&style_main_status, &lv_font_montserrat_16);
    lv_style_set_text_color(&style_main_status, lv_palette_main(LV_PALETTE_CYAN));

    // --- Time Container (HH:MM:SS) using Flexbox ---
    lv_obj_t* time_container = lv_obj_create(scr);
    lv_obj_remove_style_all(time_container); // Make container invisible
    lv_obj_set_size(time_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT); // Size to fit content
    lv_obj_set_layout(time_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(time_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(time_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(time_container, 5, 0); // Horizontal gap between elements in time_container

    // Desired Y position for the center of the time row, relative to screen center
    lv_coord_t time_row_y_center_offset = -55; 
    lv_obj_align(time_container, LV_ALIGN_CENTER, 0, time_row_y_center_offset);

    // Create Time Labels as children of time_container
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

    label_seconds = lv_label_create(time_container); 
    lv_obj_add_style(label_seconds, &style_time_elements, 0); 
    lv_label_set_text(label_seconds, "00"); 

    // --- Date Info Container (Day of Week, DD/MM/YYYY) ---
    lv_obj_t* date_info_container = lv_obj_create(scr);
    lv_obj_remove_style_all(date_info_container); 
    lv_obj_set_size(date_info_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(date_info_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(date_info_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(date_info_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(date_info_container, 5, 0); 
    // Align date_info_container below time_container
    lv_obj_align_to(date_info_container, time_container, LV_ALIGN_OUT_BOTTOM_MID, 0, 15); // 15px below time_container

    label_day_of_week = lv_label_create(date_info_container); 
    lv_obj_add_style(label_day_of_week, &style_day_of_week, 0); 
    lv_label_set_text(label_day_of_week, "Day"); 

    label_date = lv_label_create(date_info_container); 
    lv_obj_add_style(label_date, &style_date_format, 0);
    lv_label_set_text(label_date, "DD/MM/YYYY");
    
    // --- Bottom Info ---
    label_ntp_status = lv_label_create(scr);
    lv_obj_add_style(label_ntp_status, &style_main_status, 0);
    lv_label_set_text(label_ntp_status, "Starting...");
    lv_obj_align(label_ntp_status, LV_ALIGN_BOTTOM_MID, 0, -25); 

    label_ip_address = lv_label_create(scr);
    lv_obj_add_style(label_ip_address, &style_bottom_info, 0); 
    lv_label_set_text(label_ip_address, "IP: N/A");    
    lv_obj_align(label_ip_address, LV_ALIGN_BOTTOM_LEFT, 5, -5); 

    if (colon_blink_timer) { 
        lv_timer_del(colon_blink_timer);
    }
    colon_blink_timer = lv_timer_create(colon_blink_timer_cb, 500, NULL); 
}

// --- Time Display Update Function ---
void update_time_display(const struct tm &timeinfo_utc) {
    if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        char buf[3]; 
        struct tm timeinfo_gmt7 = timeinfo_utc;
        time_t raw_time_utc = mktime(&timeinfo_gmt7); 
        raw_time_utc += 7 * 3600; 
        timeinfo_gmt7 = *localtime(&raw_time_utc); 

        const lv_font_t* time_font_for_anim = &lv_font_montserrat_38; // Ensure this matches create_clock_ui
        lv_color_t white_color = lv_color_white();

        if (timeinfo_gmt7.tm_hour != prev_h) {
            snprintf(buf, sizeof(buf), "%02d", timeinfo_gmt7.tm_hour);
            animate_digit_change(&label_hour, lv_obj_get_x(label_hour), lv_obj_get_y(label_hour), buf, time_font_for_anim, white_color, 20, 300);
            prev_h = timeinfo_gmt7.tm_hour;
        }
        if (timeinfo_gmt7.tm_min != prev_m) {
            snprintf(buf, sizeof(buf), "%02d", timeinfo_gmt7.tm_min);
            animate_digit_change(&label_minute, lv_obj_get_x(label_minute), lv_obj_get_y(label_minute), buf, time_font_for_anim, white_color, 20, 300);
            prev_m = timeinfo_gmt7.tm_min;
        }
        if (timeinfo_gmt7.tm_sec != prev_s) {
            snprintf(buf, sizeof(buf), "%02d", timeinfo_gmt7.tm_sec);
            animate_digit_change(&label_seconds, lv_obj_get_x(label_seconds), lv_obj_get_y(label_seconds), buf, time_font_for_anim, white_color, 20, 300);
            prev_s = timeinfo_gmt7.tm_sec;
        }
        
        char date_str_current[20];
        snprintf(date_str_current, sizeof(date_str_current), "%02d/%02d/%04d", timeinfo_gmt7.tm_mday, timeinfo_gmt7.tm_mon + 1, timeinfo_gmt7.tm_year + 1900);
        if (strcmp(date_str_current, prev_date_str) != 0) {
            lv_label_set_text(label_date, date_str_current);
            strcpy(prev_date_str, date_str_current);
        }

        const char* days_of_week_en[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"}; 
        if (timeinfo_gmt7.tm_wday >= 0 && timeinfo_gmt7.tm_wday < 7) { 
          if (strcmp(days_of_week_en[timeinfo_gmt7.tm_wday], prev_dow_str) != 0) { 
              lv_label_set_text(label_day_of_week, days_of_week_en[timeinfo_gmt7.tm_wday]); 
              strcpy(prev_dow_str, days_of_week_en[timeinfo_gmt7.tm_wday]); 
          }
        }
        xSemaphoreGive(lvgl_mutex);
    } else {
        Serial.println("[update_time_display] Failed to get LVGL mutex.");
    }
}

// --- RTOS Task Handlers (lvgl_task_handler modified to use mutex) ---
void lvgl_task_handler(void *pvParameters) {
    Serial.println("LVGL task started.");
    for (;;) {
        if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(10)) == pdTRUE) { // Try to take mutex
            lv_tick_inc(5);
            lv_timer_handler();
            xSemaphoreGive(lvgl_mutex);
        } else {
            // Serial.println("[LVGL Task] Failed to get mutex, skipping a tick/handler call - this might be an issue if frequent.");
            // If this happens often, it means LVGL updates are being starved by other tasks holding the mutex too long.
            // For now, we just try again next iteration. Critical LVGL timing might be affected if this is frequent.
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// network_task_handler (modified to use mutex for LVGL status updates)
void network_task_handler(void *pvParameters) {
    Serial.println("Network task started.");
    unsigned long last_periodic_ntp_sync_local = 0;
    const unsigned long NTP_PERIODIC_SYNC_INTERVAL_LOCAL = 30 * 60 * 1000; 
    const unsigned long WIFI_RECONNECT_INTERVAL = 60 * 1000; 
    unsigned long last_wifi_check_time = 0;

    init_wifi(); // Initial WiFi connection attempt (init_wifi now uses mutex for its LVGL calls)

    for (;;) {
        if (WiFi.status() == WL_CONNECTED) {
            if (!initial_sync_done) {
                Serial.println("[NetworkTask] WiFi connected, attempting initial NTP sync.");
                syncTimeWithNTPClientList(true); 
                last_periodic_ntp_sync_local = millis(); 
            }

            if (timeClient_ptr != nullptr && initial_sync_done) {
                timeClient_ptr->update(); 
            }

            if (millis() - last_periodic_ntp_sync_local > NTP_PERIODIC_SYNC_INTERVAL_LOCAL) {
                Serial.println("[NetworkTask] Performing periodic NTP re-sync.");
                syncTimeWithNTPClientList(false);
                last_periodic_ntp_sync_local = millis();
            }
            last_wifi_check_time = millis(); 
            vTaskDelay(pdMS_TO_TICKS(1000)); 

        } else { // WiFi not connected
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
                // init_wifi() handles its own LVGL updates with mutex
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

// --- Setup Function (modified to create mutex and increase LVGL task stack) ---
void setup() {
    Serial.begin(115200);
    Serial.println("ESP32 NTP Clock RTOS Version Starting (with LVGL Mutex)...");

    lvgl_mutex = xSemaphoreCreateMutex(); // Create the mutex
    if (lvgl_mutex == NULL) {
        Serial.println("Error: Failed to create LVGL mutex!");
        // Handle error - perhaps by not starting tasks or halting
        return; 
    }

    init_display();
    init_lvgl_drivers();
    // create_clock_ui must be called AFTER mutex is created if it uses the mutex,
    // OR it must be called from a task that takes the mutex.
    // For simplicity now, ensure it doesn't use the mutex itself, or call it after task creation from lvgl_task.
    // However, create_clock_ui only creates objects, doesn't typically clash if done before tasks that then *update* these objects.
    // But to be safe, let's wrap its content if it directly manipulated shared items that tasks would also hit immediately.
    // For now, assuming create_clock_ui is okay before tasks that then *update* these objects.
    if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY) == pdTRUE) { // Take mutex for initial UI creation
        create_clock_ui(); 
        xSemaphoreGive(lvgl_mutex);
    } else {
        Serial.println("Failed to get LVGL mutex for initial create_clock_ui!");
        return;
    }
    
    xTaskCreatePinnedToCore(
        lvgl_task_handler,    
        "LVGL_Task",          
        8192,                 /* Increased Stack size of task */
        NULL,                 
        2,                    
        &lvgl_task_handle,    
        0);                   
    Serial.println("LVGL Task created.");

    xTaskCreatePinnedToCore(
        network_task_handler, 
        "Network_Task",       
        8192,                 
        NULL,                 
        1,                    
        &network_task_handle, 
        1);                   
    Serial.println("Network Task created.");
}

// --- Loop Function (Main Task) ---
// Kept minimal as most logic is in dedicated tasks
unsigned long last_time_display_update = 0;

void loop() {
    // This loop is the original Arduino task (usually priority 1 on core 1)
    // We will use it for updating the time display from the shared time data
    if (initial_sync_done && timeClient_ptr != nullptr) {
        if (millis() - last_time_display_update >= 1000) { 
            last_time_display_update = millis();
            unsigned long epochTime = timeClient_ptr->getEpochTime(); 
            struct tm timeinfo_utc;
            gmtime_r((const time_t *)&epochTime, &timeinfo_utc);

            if (timeinfo_utc.tm_year > (2000 - 1900)) { 
                update_time_display(timeinfo_utc); // This updates LVGL labels
            } else { 
                // This case should ideally be handled by network_task setting initial_sync_done to false
                // if it detects invalid time from a previously good client.
                if (label_ntp_status) lv_label_set_text(label_ntp_status, "NTP Bad Time (GMT+7)");
                // initial_sync_done = false; // Network task should manage this state primarily
            }
        }
    }
    vTaskDelay(pdMS_TO_TICKS(100)); // Yield for other tasks, main loop doesn't need to run too fast
}