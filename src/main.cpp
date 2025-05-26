#include <Arduino.h>
#include "lvgl.h"       // Luôn include LVGL trước
#include "app_hal.h"    // HAL của bạn

// --- Các include khác của bạn ---
#include <WiFi.h>
#include "time.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "DHT.h"

// --- Hằng số và biến toàn cục của bạn ---
const char* WIFI_SSID = "2.4 KariS"; // Replace with your actual SSID
const char* WIFI_PASSWORD = "12123402"; // Replace with your actual Password

// NTP Client settings
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // Default timezone UTC, update interval 60s

// DHT Sensor settings
#define DHTPIN 4       // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11  // DHT 11
DHT dht(DHTPIN, DHTTYPE);

constexpr int LVGL_TICK_PERIOD_MS = 5;

// --- Các khai báo và con trỏ toàn cục của bạn (LVGL UI elements) ---
lv_obj_t *label_hour, *label_minute, *label_colon_hm, *label_seconds;
lv_obj_t *calendar_obj, *label_ntp_status, *label_ip_address;
lv_obj_t *label_temperature, *label_humidity;

// State variables for UI updates (simplified, your original file had more complex state management)
// static int prev_h = -1, prev_m = -1, prev_s = -1; // Not strictly needed with direct LVGL timer updates
// static float prev_temp = -100.0f, prev_hum = -100.0f; // Not strictly needed
static bool colon_visible = true;
static lv_timer_t *colon_blink_timer = nullptr;
static lv_timer_t *sensor_update_timer = nullptr;
static lv_timer_t *time_update_timer = nullptr;

// --- Trình in log LVGL ---
#if LV_USE_LOG && LV_LOG_PRINTF
// Updated signature for LVGL 9.x
void my_lvgl_log_printer(lv_log_level_t level, const char *buf) {
    LV_UNUSED(level); // Or use it: Serial.printf("LVGL Log (Lvl %d): ", level);
    Serial.print(buf); // In ra Serial Monitor
    // Serial.flush(); // Optional: ensure logs are sent immediately, can slow down performance
}
#endif

// --- Logic UI của bạn (tạo widget, cập nhật) ---

static void update_time_labels(const struct tm &timeinfo) {
    if (label_hour) lv_label_set_text_fmt(label_hour, "%02d", timeinfo.tm_hour);
    if (label_minute) lv_label_set_text_fmt(label_minute, "%02d", timeinfo.tm_min);
    if (label_seconds) lv_label_set_text_fmt(label_seconds, "%02d", timeinfo.tm_sec);
}

static void update_sensor_labels(float temp, float hum) {
    if (label_temperature) lv_label_set_text_fmt(label_temperature, "%.1f C", temp);
    if (label_humidity) lv_label_set_text_fmt(label_humidity, "%.1f %%", hum);
}

static void colon_blink_timer_cb(lv_timer_t *timer) {
    LV_UNUSED(timer);
    colon_visible = !colon_visible;
    if (label_colon_hm) {
        lv_label_set_text(label_colon_hm, colon_visible ? ":" : " ");
    }
}

static void sensor_update_timer_cb(lv_timer_t *timer) {
    LV_UNUSED(timer);
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
        Serial.println("Failed to read from DHT sensor!");
        if (label_temperature) lv_label_set_text(label_temperature, "-- C");
        if (label_humidity) lv_label_set_text(label_humidity, "-- %");
        return;
    }
    update_sensor_labels(t, h);
}

static void time_update_timer_cb(lv_timer_t *timer) {
    LV_UNUSED(timer);
    if (WiFi.status() == WL_CONNECTED) {
        if (timeClient.update()) { // Returns true on successful update
            time_t epochTime = timeClient.getEpochTime();
            struct tm timeinfo;
            // Consider ESP32 timezone settings if not using UTC
            // For UTC:
            // gmtime_r(&epochTime, &timeinfo);
            // For local time (requires ESP32 timezone to be set, e.g., via configTime):
            localtime_r(&epochTime, &timeinfo);
            update_time_labels(timeinfo);
            if (label_ntp_status) lv_label_set_text(label_ntp_status, "NTP: Synced");
        } else {
            if (label_ntp_status) lv_label_set_text(label_ntp_status, "NTP: Update Fail");
        }
    } else {
        if (label_ntp_status) lv_label_set_text(label_ntp_status, "NTP: No WiFi");
        // Consider adding WiFi.begin() here or a separate reconnect logic if WiFi drops
    }
}

void create_main_ui(void) {
    lv_obj_t *scr = lv_screen_active();
    lv_obj_clean(scr);
    lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);

    // --- Styles ---
    static lv_style_t style_time_large;
    lv_style_init(&style_time_large);
    lv_style_set_text_font(&style_time_large, LV_FONT_DEFAULT); // Uses font set in platformio.ini (-D LV_FONT_DEFAULT)
    // To use a specific large font for time instead of default:
    // lv_style_set_text_font(&style_time_large, &lv_font_montserrat_48);
    lv_style_set_text_color(&style_time_large, lv_color_white());

    static lv_style_t style_label_small;
    lv_style_init(&style_label_small);
    lv_style_set_text_font(&style_label_small, &lv_font_montserrat_16);
    lv_style_set_text_color(&style_label_small, lv_color_hex(0xCCCCCC)); // Light grey

    static lv_style_t style_sensor_data;
    lv_style_init(&style_sensor_data);
    lv_style_set_text_font(&style_sensor_data, &lv_font_montserrat_22);
    lv_style_set_text_color(&style_sensor_data, lv_color_white());

    // --- Time Display Container (Centered Flexbox) ---
    lv_obj_t* time_container = lv_obj_create(scr);
    lv_obj_remove_style_all(time_container);
    lv_obj_set_width(time_container, LV_PCT(100));
    lv_obj_set_height(time_container, LV_SIZE_CONTENT);
    lv_obj_align(time_container, LV_ALIGN_CENTER, 0, -60); // Position slightly above center
    lv_obj_set_flex_flow(time_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(time_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(time_container, 10, 0);

    label_hour = lv_label_create(time_container);
    lv_obj_add_style(label_hour, &style_time_large, 0);
    lv_label_set_text(label_hour, "--");

    label_colon_hm = lv_label_create(time_container);
    lv_obj_add_style(label_colon_hm, &style_time_large, 0);
    lv_label_set_text(label_colon_hm, ":");

    label_minute = lv_label_create(time_container);
    lv_obj_add_style(label_minute, &style_time_large, 0);
    lv_label_set_text(label_minute, "--");

    label_seconds = lv_label_create(time_container);
    lv_obj_add_style(label_seconds, &style_time_large, 0); // Using same style for seconds
    // For smaller seconds, you could use style_label_small or another custom style:
    // lv_obj_add_style(label_seconds, &style_label_small, 0);
    lv_label_set_text(label_seconds, "--");

    // --- Calendar (Bottom Left) ---
    calendar_obj = lv_calendar_create(scr);
    lv_obj_set_size(calendar_obj, 220, 200);
    lv_obj_align(calendar_obj, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    // Add custom event handlers or styling for the calendar if needed
    // e.g., lv_obj_add_event_cb(calendar_obj, my_calendar_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // --- Status Labels (Top Left) ---
    label_ntp_status = lv_label_create(scr);
    lv_obj_add_style(label_ntp_status, &style_label_small, 0);
    lv_label_set_text(label_ntp_status, "NTP: Init...");
    lv_obj_align(label_ntp_status, LV_ALIGN_TOP_LEFT, 5, 5);

    label_ip_address = lv_label_create(scr);
    lv_obj_add_style(label_ip_address, &style_label_small, 0);
    lv_label_set_text(label_ip_address, "IP: -. -. -. -");
    lv_obj_align_to(label_ip_address, label_ntp_status, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 2);

    // --- Sensor Data Labels (Bottom Right, Column Flexbox) ---
    lv_obj_t* sensor_container = lv_obj_create(scr);
    lv_obj_remove_style_all(sensor_container);
    lv_obj_set_width(sensor_container, LV_SIZE_CONTENT);
    lv_obj_set_height(sensor_container, LV_SIZE_CONTENT);
    lv_obj_align(sensor_container, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_flex_flow(sensor_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(sensor_container, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(sensor_container, 5, 0);

    label_temperature = lv_label_create(sensor_container);
    lv_obj_add_style(label_temperature, &style_sensor_data, 0);
    lv_label_set_text(label_temperature, "-- C");

    label_humidity = lv_label_create(sensor_container);
    lv_obj_add_style(label_humidity, &style_sensor_data, 0);
    lv_label_set_text(label_humidity, "-- %");

    // --- LVGL Timers for UI updates ---
    colon_blink_timer = lv_timer_create(colon_blink_timer_cb, 500, NULL);
    sensor_update_timer = lv_timer_create(sensor_update_timer_cb, 5000, NULL); // Read sensors every 5 seconds
    time_update_timer = lv_timer_create(time_update_timer_cb, 1000, NULL);   // Update time display every 1 second

    Serial.println("UI elements created.");
}

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32 Clock - LVGL 9.1 & LovyanGFX");

    // Register LVGL log printer if enabled
#if LV_USE_LOG && LV_LOG_PRINTF
    lv_log_register_print_cb(my_lvgl_log_printer);
#endif

    dht.begin(); // Initialize DHT sensor

    lv_init();   // Initialize LVGL
    hal_setup(); // Initialize display and input via LovyanGFX and LVGL drivers

    // --- WiFi and NTP Setup ---
    Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int wifi_retries = 0;
    while (WiFi.status() != WL_CONNECTED && wifi_retries < 20) { // Retry for 10 seconds
        delay(500);
        Serial.print(".");
        wifi_retries++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        
        // Configure time: NTP server, timezone (GMT offset), daylight saving offset
        // Example: GMT+0, no DST. Adjust gmtOffset_sec and daylightOffset_sec as needed.
        // For Vietnam (GMT+7): const long gmtOffset_sec = 7 * 3600;
        const long gmtOffset_sec = 0; // UTC
        const int daylightOffset_sec = 0; // No daylight saving
        configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org");

        timeClient.begin(); // Initialize NTPClient (it uses its own server list by default)
        Serial.println("Attempting initial NTP sync...");
        if (timeClient.forceUpdate()) {
            Serial.println("NTP initial sync success.");
        } else {
            Serial.println("NTP initial sync failed. Will retry in background.");
        }
    } else {
        Serial.println("\nWiFi connection FAILED.");
    }

    create_main_ui(); // Create all LVGL UI elements

    // Update UI labels with initial status post-UI creation
    if (label_ip_address) {
        if (WiFi.status() == WL_CONNECTED) {
            lv_label_set_text_fmt(label_ip_address, "IP: %s", WiFi.localIP().toString().c_str());
        } else {
            lv_label_set_text(label_ip_address, "IP: Disconnected");
        }
    }
    if (label_ntp_status) { 
        if (WiFi.status() == WL_CONNECTED) {
             // Check if epoch time is valid (greater than a small threshold like 1000s after epoch)
             if(timeClient.getEpochTime() > 1000) { 
                lv_label_set_text(label_ntp_status, "NTP: Synced");
             } else {
                lv_label_set_text(label_ntp_status, "NTP: Syncing...");
             }
        } else {
            lv_label_set_text(label_ntp_status, "NTP: No WiFi");
        }
    }

    Serial.println("Setup complete.");
}

void loop() {
    lv_tick_inc(LVGL_TICK_PERIOD_MS); // Increment LVGL's internal tick counter
    hal_loop(); // This calls lv_timer_handler()

    // A small delay is good practice. 
    // If using FreeRTOS, vTaskDelay(pdMS_TO_TICKS(LVGL_TICK_PERIOD_MS)) is preferred.
    delay(LVGL_TICK_PERIOD_MS); 
}

// RTOS Notes (from improve.md - for future reference if you add RTOS tasks):
// 1. Create a mutex: SemaphoreHandle_t lvgl_mutex = xSemaphoreCreateMutex(); in setup.
// 2. Protect LVGL calls from different tasks with the mutex.
// 3. lv_timer_handler() (called by hal_loop()) should ideally run in its own task or be protected.
// 4. lv_tick_inc() should be called regularly from a high-priority context. 