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