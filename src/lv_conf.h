#ifndef LV_CONF_H
#define LV_CONF_H

#include <Arduino.h> /* For Serial.printf and other Arduino specifics */

/* Define to 1 to enable the content of this file */
#if 1

/*==================
 * GENERAL SETTINGS
 *==================*/

/* Color depth: 1, 8, 16, 32 */
#define LV_COLOR_DEPTH 16

/* Maximum color depth to be used if LV_COLOR_DEPTH is set to 32*/
#define LV_COLOR_DEPTH_32_BPP 4

/*Use a custom tick source - WE ARE CALLING lv_tick_inc MANUALLY */
#define LV_TICK_CUSTOM 0
#if LV_TICK_CUSTOM
    /*Requires LV_TICK_CUSTOM_SYS_TIME_EXPR to be set define the expression which gives the system time in ms*/
    // #define LV_TICK_CUSTOM_INCLUDE "Arduino.h" /*Header for the system time function*/
    // #define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis()) /*Expression evaluating to current system time in ms*/
#endif   /*LV_TICK_CUSTOM*/

/*Default screen refresh period. LVGL will redraw changed areas with this period time*/
#define LV_DISP_DEF_REFR_PERIOD 30     /*[ms]*/

/*Input device read period in milliseconds*/
#define LV_INDEV_DEF_READ_PERIOD 30    /*[ms]*/

/*Use a custom timer handler for LVGL. Will be called periodically.
 *Can be used to operate LVGL from a real system RTOS*/
#define LV_TMR_CUSTOM 0
#if LV_TMR_CUSTOM
  #define LV_TMR_CUSTOM_INCLUDE "Arduino.h"               /*Header for the system time function*/
  #define LV_TMR_CUSTOM_SYS_TIME_EXPR (millis())          /*Expression evaluating to current system time in ms*/
#endif

/* Enable complex text layout features (e.g. Hindi, Arabic) to support more World languages.
 * As a consequence LVGL will link with [Harfbuzz](https://harfbuzz.github.io/) and
 * [FriBiDi](https://github.com/fribidi/fribidi) libraries.
 * LV_USE_BIDI and LV_USE_ARABIC_PERSIAN_CHARS are reserved for backward compatibility
 * and has no effect. */
#define LV_USE_COMPLEX_TEXT_LAYOUT 0    /*Must be enabled to use the features below*/
#if LV_USE_COMPLEX_TEXT_LAYOUT
/*Set the following options to '1' if the language is used*/
#  define LV_USE_THAI_ARABIC_HEBREW 0   /*Thai, Arabic, Hebrew*/
#  define LV_USE_CJK_LANGUAGES    0   /*Chinese, Japanese, Korean*/
#  define LV_USE_INDIC_LANGUAGES  0   /*Hindi, Bengali, Tamil, etc.*/
#endif

/*System log messages enable/disable*/
#define LV_USE_LOG 1
#if LV_USE_LOG
  /*Howebug log level to use. (See LV_LOG_LEVEL_... options above)*/
  #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

  /*Select function used to print log messages.*/
  #define LV_LOG_PRINTF 1
  #if LV_LOG_PRINTF
    /*Declare your printf like function*/
    void my_lvgl_log_printer(const char * dsc); 
    // #define LV_LOG_PRINTF my_lvgl_log_printer // This is often set in the build flags or if you have a custom printer
  #endif
#endif

/*=========================
 * FONT USAGE
 *=========================*/

/* Montserrat fonts with ASCII range and some symbols */
#define LV_FONT_MONTSERRAT_8  0
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_MONTSERRAT_12 1  // <<< KÍCH HOẠT FONT 12
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 0
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_22 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_26 0
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_30 0
#define LV_FONT_MONTSERRAT_32 0
#define LV_FONT_MONTSERRAT_34 0
#define LV_FONT_MONTSERRAT_36 0
#define LV_FONT_MONTSERRAT_38 1
#define LV_FONT_MONTSERRAT_40 0
#define LV_FONT_MONTSERRAT_42 0
#define LV_FONT_MONTSERRAT_44 0
#define LV_FONT_MONTSERRAT_46 0
#define LV_FONT_MONTSERRAT_48 1

/* Default font */
#define LV_FONT_DEFAULT &lv_font_montserrat_16


/*=========================
 * WIDGETS
 *=========================*/

/*Arc*/
#define LV_USE_ARC        1

/*Bar*/
#define LV_USE_BAR        1

/*Button*/
#define LV_USE_BTN        1

/*Button matrix*/
#define LV_USE_BTNMATRIX  1

/*Calendar*/
#define LV_USE_CALENDAR   1   // <<< ĐẢM BẢO DÒNG NÀY LÀ 1
#if LV_USE_CALENDAR
#  define LV_CALENDAR_WEEK_STARTS_MONDAY 0
#  if LV_CALENDAR_WEEK_STARTS_MONDAY
#    define LV_CALENDAR_DEFAULT_DAY_NAMES {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"}
#  else
#    define LV_CALENDAR_DEFAULT_DAY_NAMES {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"}
#  endif

#  define LV_CALENDAR_DEFAULT_MONTH_NAMES {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"}
#  define LV_USE_CALENDAR_HEADER_ARROW 1   // <<< ĐẢM BẢO DÒNG NÀY LÀ 1 (cho phép dùng lv_calendar_header_arrow_get_month_label)
#  define LV_USE_CALENDAR_HEADER_DROPDOWN 0 /*Use the drop-down header style*/
#endif

/*Canvas*/
#define LV_USE_CANVAS     1

/*Checkbox*/
#define LV_USE_CHECKBOX   1

/*Chart*/
#define LV_USE_CHART      1

/*Color wheel*/
#define LV_USE_COLORWHEEL  0

/*Dropdown list*/
#define LV_USE_DROPDOWN   1

/*Image*/
#define LV_USE_IMG        1
#if LV_USE_IMG
#  define LV_IMG_CACHE_DEF_SIZE       0
#if LV_COLOR_DEPTH == 16 && LV_COLOR_16_SWAP == 0 && \
    !(LV_COLOR_DEPTH_32_BPP == 4 && LV_COLOR_SCREEN_TRANSP)
# define LV_BIN_DECODER_RAM_LOAD 0
#else
# define LV_BIN_DECODER_RAM_LOAD 0
#endif
#endif

/*Image Button*/
#define LV_USE_IMGBTN     1

/*Keyboard*/
#define LV_USE_KEYBOARD   1

/*Label*/
#define LV_USE_LABEL      1
#if LV_USE_LABEL
#  define LV_LABEL_TEXT_SELECTION 0
#  define LV_LABEL_LONG_TXT_HINT 1
#endif

/*LED*/
#define LV_USE_LED        1

/*Line*/
#define LV_USE_LINE       1

/*List*/
#define LV_USE_LIST       1

/*Menu*/
#define LV_USE_MENU       0

/*Meter*/
#define LV_USE_METER      0

/*MsgBox*/
#define LV_USE_MSGBOX     1

/*Roller*/
#define LV_USE_ROLLER     1
#if LV_USE_ROLLER
#  define LV_ROLLER_INF_PAGES 7
#endif

/*Slider*/
#define LV_USE_SLIDER     1

/*Span*/
#define LV_USE_SPAN       0
#if LV_USE_SPAN
  #define LV_SPAN_SNIPPET_STACK_SIZE 64
#endif

/*Spinbox*/
#define LV_USE_SPINBOX    1

/*Spinner*/
#define LV_USE_SPINNER    1

/*Switch*/
#define LV_USE_SWITCH     1

/*Table*/
#define LV_USE_TABLE      1

/*Tabview*/
#define LV_USE_TABVIEW    1

/*Text area*/
#define LV_USE_TEXTAREA   1
#if LV_USE_TEXTAREA != 0
#  define LV_TEXTAREA_DEF_PWD_SHOW_TIME 1500
#endif

/*TileView*/
#define LV_USE_TILEVIEW   0

/*Window*/
#define LV_USE_WIN        0

/*==================
 * Non-user settings
 *==================*/
#define LV_LOG_LEVEL_TRACE 0
#define LV_LOG_LEVEL_INFO  1
#define LV_LOG_LEVEL_WARN  2
#define LV_LOG_LEVEL_ERROR 3
#define LV_LOG_LEVEL_USER  4
#define LV_LOG_LEVEL_NONE  5

#if LV_USE_LOG && defined(LV_LOG_PRINTF) && LV_LOG_PRINTF == my_lvgl_log_printer
  // void my_lvgl_log_printer(const char *buf); // Đã khai báo nếu cần
#endif

#define LV_HOR_RES_MAX (240)
#define LV_VER_RES_MAX (320)

#define LV_MEM_CUSTOM 0
#if LV_MEM_CUSTOM == 0
  #define LV_MEM_SIZE (32U * 1024U)
#endif

#endif /*End of LV_CONF_H guard*/
#endif /*End of LV_CONF_H*/
