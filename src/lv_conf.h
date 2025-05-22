#ifndef LV_CONF_H
#define LV_CONF_H

#include <Arduino.h> /* For Serial.printf and other Arduino specifics */

/* Define to 1 to enable the content of this file */
#if 1

/* Color depth: 1, 8, 16, 32 */
#define LV_COLOR_DEPTH 16

/* Use a custom tick source - WE ARE CALLING lv_tick_inc MANUALLY */
#define LV_TICK_CUSTOM 0

#if LV_TICK_CUSTOM
    /*Requires LV_TICK_CUSTOM_SYS_TIME_EXPR to be set define the expression which gives the system time in ms*/
    // #define LV_TICK_CUSTOM_INCLUDE "Arduino.h" /*Header for the system time function*/
    // #define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis()) /*Expression evaluating to current system time in ms*/
#endif   /*LV_TICK_CUSTOM*/

/* Montserrat fonts */
#define LV_FONT_MONTSERRAT_14 1 // Enabled for bottom info
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_20 1 // Enabled as a general purpose size
#define LV_FONT_MONTSERRAT_22 1 // Enabled for Date
#define LV_FONT_MONTSERRAT_24 1 // Enabled for Seconds
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_38 1
#define LV_FONT_MONTSERRAT_48 1 // Enabled for larger time display (HH:MM)

/* Default font */
#define LV_FONT_DEFAULT &lv_font_montserrat_16

/* Enable LVGL's log system */
#define LV_USE_LOG 1
#if LV_USE_LOG
  /*Declare a custom log print function*/
  void my_lvgl_log_printer(const char *dsc);
  #define LV_LOG_PRINTF my_lvgl_log_printer 
#endif

/* --- Other LVGL settings can be added below as needed --- */
/* For example, enable specific widgets or features */

/* Horizontal and Vertical resolution of the display */
#define LV_HOR_RES_MAX (240)
#define LV_VER_RES_MAX (320)

/* Higher memory usage but better performance */
#define LV_MEM_CUSTOM 0 /*Use LV_MEM_SIZE to non-zero*/
#if LV_MEM_CUSTOM == 0
  #define LV_MEM_SIZE (32U * 1024U) /* Pre-allocate 32KB for LVGL's dynamic memory*/
#endif


#endif /* End of LV_CONF_H guard */
#endif /* End of LV_CONF_H include guard */ 