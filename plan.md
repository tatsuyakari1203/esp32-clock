# Xây dựng Đồng hồ NTP Đa năng với Giao diện Đồ họa và Hiệu ứng trên ESP32-2432S028R

## I. Giới thiệu

Báo cáo này trình bày chi tiết quá trình nghiên cứu và phát triển một chương trình mẫu hiển thị giờ Việt Nam trên bo mạch ESP32-2432S028R. Mục tiêu chính là tạo ra một giao diện người dùng (UI) trực quan, thẩm mỹ, tích hợp các hiệu ứng chuyển động (animation) sinh động và sử dụng giao thức NTP (Network Time Protocol) để đồng bộ hóa thời gian một cách chính xác. Dự án này khai thác khả năng của vi điều khiển ESP32, màn hình cảm ứng ILI9341 tích hợp trên bo mạch, thư viện đồ họa LVGL (Light and Versatile Graphics Library) và thư viện trình điều khiển TFT\_eSPI. Báo cáo sẽ đi sâu vào các khía cạnh từ cấu hình phần cứng, thiết lập môi trường phát triển, lập trình giao diện, đồng bộ thời gian, cho đến triển khai các hiệu ứng động, nhằm cung cấp một hướng dẫn toàn diện và chuyên sâu.


## II. Tổng quan về Phần cứng ESP32-2432S028R

Bo mạch ESP32-2432S028R, thường được cộng đồng biết đến với tên gọi "Cheap Yellow Display" (CYD), là một giải pháp phát triển tích hợp mạnh mẽ, kết hợp vi điều khiển ESP32 với màn hình TFT LCD cảm ứng màu và các ngoại vi cần thiết khác.<!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><sup>3</sup><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!---->

Vi điều khiển trung tâm:

Lõi của bo mạch là module ESP32-WROOM-32 (một số tài liệu bán hàng ghi ESP32-WROOM-32 4, trong khi datasheet của ESP32-WROOM-32E 11 cũng có các thông số tương tự và thường được sử dụng). Module này dựa trên chip ESP32-D0WDQ6 (hoặc các biến thể tương đương như ESP32-D0WD-V3 trong ESP32-WROOM-32E 11), tích hợp bộ vi xử lý Xtensa lõi kép 32-bit LX6 với tốc độ lên đến 240 MHz, bộ nhớ SRAM 520 KB, ROM 448 KB và bộ nhớ Flash SPI thường là 4 MB (một số phiên bản có thể có 8MB hoặc 16MB Flash, và có thể kèm PSRAM).3 Module này cung cấp kết nối Wi-Fi (802.11b/g/n) và Bluetooth (V4.2 BR/EDR và BLE), rất phù hợp cho các ứng dụng IoT.2

Màn hình hiển thị:

ESP32-2432S028R được trang bị màn hình TFT LCD màu 2.8 inch với độ phân giải 240x320 pixel.3 Trình điều khiển (driver IC) cho màn hình này là ILI9341.4 ILI9341 hỗ trợ giao tiếp SPI và có khả năng hiển thị màu 16-bit (65K màu) hoặc 18-bit (262K màu).15 Giao tiếp SPI giúp tiết kiệm số lượng chân GPIO cần thiết từ vi điều khiển.

Bộ điều khiển cảm ứng:

Màn hình này là loại cảm ứng điện trở (resistive touchscreen).6 Việc xử lý tín hiệu cảm ứng thường được đảm nhiệm bởi chip XPT2046 hoặc một chip tương thích.3 XPT2046 là một bộ chuyển đổi analog-to-digital (ADC) 12-bit, giao tiếp với ESP32 qua giao thức SPI riêng biệt hoặc chia sẻ bus SPI với màn hình (với chân CS khác nhau).17 Nó có khả năng phát hiện vị trí chạm và cả áp lực chạm.

Các ngoại vi khác:

Ngoài ra, bo mạch còn tích hợp khe cắm thẻ nhớ microSD, đèn LED RGB, mạch điều khiển đèn nền, và các chân GPIO mở rộng.3 Điện áp hoạt động của bo mạch thường là 5V, được cấp qua cổng USB hoặc chân nguồn riêng.3


## III. Thiết lập Môi trường Phát triển với PlatformIO

PlatformIO là một hệ sinh thái phát triển đa nền tảng, mã nguồn mở, hỗ trợ nhiều bo mạch và framework khác nhau, bao gồm cả ESP32 với Arduino framework. Việc sử dụng PlatformIO tích hợp trong Visual Studio Code (VS Code) mang lại nhiều lợi ích như quản lý thư viện thông minh, gỡ lỗi nâng cao và cấu hình dự án linh hoạt.

1\. Cài đặt Visual Studio Code và PlatformIO IDE:

Đầu tiên, cần cài đặt VS Code từ trang chủ. Sau đó, cài đặt tiện ích mở rộng PlatformIO IDE từ Marketplace của VS Code.19

2\. Tạo Dự án PlatformIO Mới:

Trong VS Code, mở PlatformIO Home, chọn "New Project". Đặt tên cho dự án (ví dụ: ESP32\_NTP\_Clock), chọn bo mạch là Espressif ESP32 Dev Module (đây là lựa chọn chung cho hầu hết các bo mạch ESP32, bao gồm ESP32-2432S028R), và chọn Framework là Arduino.19

3\. Cấu hình platformio.ini:

File platformio.ini là trung tâm cấu hình của dự án PlatformIO. Nó quản lý các thiết lập về bo mạch, framework, thư viện phụ thuộc, và các tùy chọn biên dịch.

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

Ini, TOML

<!---->

<!---->

<!---->

<!---->

<!---->

    [env:esp32dev]
    platform = espressif32
    board = esp32dev
    framework = arduino
    monitor_speed = 115200
    lib_deps =
        lvgl/lvgl@^8.3.11 ; Hoặc phiên bản LVGL 9.x nếu muốn, ví dụ lvgl/lvgl@^9.0
        bodmer/TFT_eSPI@^2.5.34
        ; PaulStoffregen/XPT2046_Touchscreen ; Tùy chọn nếu không dùng touch của TFT_eSPI
    build_flags =
        -D LV_CONF_INCLUDE_SIMPLE ; Cho phép LVGL tìm lv_conf.h trong include path
        -I src ; Thêm thư mục src vào include path để tìm lv_conf.h
        -D USER_SETUP_LOADED=1 ; Đảm bảo User_Setup.h của TFT_eSPI được load
        -D LV_LVGL_H_INCLUDE_SIMPLE ; Cho LVGL 8.3.x
        ; -D LV_CONF_PATH="src/lv_conf.h" ; Một cách khác để chỉ định đường dẫn lv_conf.h
    lib_ldf_mode = deep+

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

**Giải thích các thành phần quan trọng trong `platformio.ini`:**

- `platform = espressif32`: Chỉ định nền tảng phát triển là Espressif ESP32.

- `board = esp32dev`: Sử dụng cấu hình chung cho ESP32 Dev Module. ESP32-2432S028R thường tương thích tốt với cấu hình này.

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <sup>

  20

  </sup>

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

- `framework = arduino`: Sử dụng Arduino core cho ESP32.

- `monitor_speed = 115200`: Đặt tốc độ baud cho Serial Monitor.

- `lib_deps`: Liệt kê các thư viện cần thiết cho dự án.

  - `lvgl/lvgl@^8.3.11`: Thư viện LVGL. Phiên bản có thể được điều chỉnh. LVGL 8.3.x là một phiên bản ổn định và được hỗ trợ rộng rãi. Nếu sử dụng LVGL 9.x, một số API và cách cấu hình có thể khác.

  - `bodmer/TFT_eSPI@^2.5.34`: Thư viện trình điều khiển màn hình TFT mạnh mẽ của Bodmer, hỗ trợ nhiều loại chip bao gồm ILI9341 và có khả năng xử lý cảm ứng XPT2046.

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <sup>

    3

    </sup>

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

- `build_flags`: Các cờ truyền cho trình biên dịch.

  - `-D LV_CONF_INCLUDE_SIMPLE` và `-I src`: Giúp LVGL tìm thấy file `lv_conf.h` nếu nó được đặt trong thư mục `src` của dự án. Đây là một cách phổ biến để quản lý cấu hình LVGL.
  - `-D USER_SETUP_LOADED=1`: Cờ này thường được sử dụng cùng với thư viện TFT\_eSPI để báo hiệu rằng một file `User_Setup.h` tùy chỉnh (thường nằm trong thư mục của TFT\_eSPI hoặc được include bởi `User_Setup_Select.h`) sẽ được sử dụng, thay vì các cấu hình mặc định.
  - `-D LV_LVGL_H_INCLUDE_SIMPLE`: Cần thiết cho LVGL phiên bản 8.3.x để đơn giản hóa việc include header chính của LVGL (`#include "lvgl.h"`).

- `lib_ldf_mode = deep+`: Chế độ này hướng dẫn PlatformIO cách tìm kiếm các file header của thư viện. `deep+` thực hiện một quá trình quét sâu hơn, rất hữu ích khi các file cấu hình như `User_Setup.h` (cho TFT\_eSPI) và `lv_conf.h` (cho LVGL) không nằm trực tiếp trong thư mục gốc của thư viện hoặc được include một cách gián tiếp. Điều này giúp tránh các lỗi biên dịch phổ biến liên quan đến việc không tìm thấy các file header cấu hình tùy chỉnh, đảm bảo rằng các thiết lập đặc thù cho phần cứng và dự án được áp dụng chính xác.


## IV. Thiết lập Trình điều khiển Màn hình và Cảm ứng với TFT\_eSPI

Thư viện TFT\_eSPI của Bodmer là một lựa chọn phổ biến và mạnh mẽ để điều khiển các loại màn hình TFT và cảm ứng trên ESP32. Việc cấu hình chính xác thư viện này là yếu tố then chốt để màn hình và cảm ứng hoạt động đúng.

1\. Sơ đồ chân (pinout) của ESP32-2432S028R cho màn hình ILI9341 và cảm ứng XPT2046:

Việc kết nối phần cứng giữa ESP32 và các thành phần hiển thị/cảm ứng phải chính xác. Dưới đây là bảng tổng hợp sơ đồ chân dựa trên các nguồn tham khảo cho ESP32-2432S028R (CYD).3 Cần lưu ý rằng có thể có sự khác biệt nhỏ giữa các phiên bản (revision) của bo mạch.1 Bảng này dựa trên cấu hình phổ biến:

**Table 1: Sơ đồ kết nối chân ESP32-2432S028R cho Màn hình ILI9341 & Cảm ứng XPT2046**

|                            |                                  |                                                                                                                                                                                                                                                                                                                                                                                                                         |
| -------------------------- | -------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Chức năng**              | **Chân GPIO trên ESP32**         | **Ghi chú**                                                                                                                                                                                                                                                                                                                                                                                                             |
| **Màn hình TFT (ILI9341)** |                                  |                                                                                                                                                                                                                                                                                                                                                                                                                         |
| `TFT_MOSI` (SDI)           | `13`                             | Master Out Slave In (Dữ liệu từ ESP32 đến TFT)                                                                                                                                                                                                                                                                                                                                                                          |
| `TFT_MISO` (SDO)           | `12`                             | Master In Slave Out (Dữ liệu từ TFT đến ESP32, không phải lúc nào cũng cần)                                                                                                                                                                                                                                                                                                                                             |
| `TFT_SCLK` (SCK)           | `14`                             | Serial Clock                                                                                                                                                                                                                                                                                                                                                                                                            |
| `TFT_CS`                   | `15`                             | Chip Select cho TFT                                                                                                                                                                                                                                                                                                                                                                                                     |
| `TFT_DC` (RS)              | `2`                              | Data/Command Select                                                                                                                                                                                                                                                                                                                                                                                                     |
| `TFT_RST`                  | `EN` (Reset của ESP32) hoặc `-1` | Reset cho TFT. Nối với chân EN (RST) của ESP32 là phổ biến.<!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><sup>1</sup><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----> Nếu dùng GPIO riêng, cần thay đổi. |
| `TFT_BL`                   | `21`                             | Điều khiển đèn nền (Backlight)                                                                                                                                                                                                                                                                                                                                                                                          |
| **Cảm ứng (XPT2046)**      |                                  |                                                                                                                                                                                                                                                                                                                                                                                                                         |
| `TOUCH_CS` (T\_CS)         | `33`                             | Chip Select cho XPT2046                                                                                                                                                                                                                                                                                                                                                                                                 |
| `TOUCH_IRQ` (T\_IRQ)       | `36`                             | Chân ngắt từ XPT2046 (báo có sự kiện chạm)                                                                                                                                                                                                                                                                                                                                                                              |
| `TOUCH_DIN` (T\_DIN)       | `32`                             | Data In cho XPT2046 (MOSI từ ESP32)                                                                                                                                                                                                                                                                                                                                                                                     |
| `TOUCH_DO` (T\_OUT)        | `39`                             | Data Out từ XPT2046 (MISO về ESP32)                                                                                                                                                                                                                                                                                                                                                                                     |
| `TOUCH_CLK` (T\_CLK)       | `25`                             | Clock cho XPT2046                                                                                                                                                                                                                                                                                                                                                                                                       |

<!---->

<!---->

<!---->

<!---->

<!---->

_Lưu ý về `TFT_RST`:_ Một số hướng dẫn sử dụng GPIO riêng cho `TFT_RST`. Tuy nhiên, trên nhiều bo mạch tích hợp như ESP32-2432S028R, chân reset của màn hình được nối chung với chân `EN` (Enable/Reset) của ESP32. Trong trường hợp này, giá trị `-1` được dùng trong cấu hình TFT\_eSPI. Nếu `TFT_RST` được nối với một GPIO cụ thể, bạn cần chỉ định số GPIO đó.

2\. Cấu hình User\_Setup.h trong thư viện TFT\_eSPI:

File User\_Setup.h là trái tim của việc cấu hình thư viện TFT\_eSPI. Nó cho phép người dùng định nghĩa loại driver màn hình, các chân GPIO được sử dụng, tần số giao tiếp SPI, các font chữ tích hợp sẵn và nhiều tùy chọn khác.3

- **Vị trí file:** Trong môi trường PlatformIO, file `User_Setup.h` thường nằm trong thư mục của thư viện TFT\_eSPI, ví dụ: `.pio/libdeps/esp32dev/TFT_eSPI/User_Setup.h`. Một cách tiếp cận khác là sử dụng `User_Setup_Select.h` để trỏ đến một file cấu hình tùy chỉnh đặt ở nơi khác, giúp quản lý dễ dàng hơn khi cập nhật thư viện.<!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><sup>19</sup><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!---->

- Nội dung User\_Setup.h mẫu cho ESP32-2432S028R:

  Dựa trên sơ đồ chân ở trên và các thông số kỹ thuật của bo mạch 3, dưới đây là một cấu hình mẫu:

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  C++

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

      // Trong file User_Setup.h hoặc file cấu hình tùy chỉnh của bạn
      #define ILI9341_DRIVER

      #define TFT_MOSI 13 // SPI MOSI (Master Out Slave In) - Data to TFT
      #define TFT_MISO 12 // SPI MISO (Master In Slave Out) - Data from TFT (không phải lúc nào cũng dùng)
      #define TFT_SCLK 14 // SPI Clock
      #define TFT_CS   15 // TFT Chip Select
      #define TFT_DC    2 // TFT Data/Command
      #define TFT_RST  -1 // TFT Reset (nối với EN của ESP32) hoặc GPIO nếu dùng chân riêng

      #define TFT_BL   21 // LED Backlight control
      #define TFT_BACKLIGHT_ON HIGH // Mức để bật đèn nền

      #define TOUCH_CS 33 // Touch Chip Select

      // Tần số SPI
      #define SPI_FREQUENCY        40000000     // Cho màn hình ILI9341, có thể lên đến 80MHz nhưng 40MHz ổn định hơn
      #define SPI_READ_FREQUENCY   20000000     // Tần số đọc từ màn hình
      #define SPI_TOUCH_FREQUENCY  2500000      // XPT2046 thường yêu cầu tần số SPI thấp hơn

      // Kích hoạt các font (tùy chọn nếu LVGL quản lý font)
      #define LOAD_GLCD  // Font system 5x7
      #define LOAD_FONT2 // Font cao 16 pixel
      #define LOAD_FONT4 // Font cao 26 pixel
      #define LOAD_FONT6 // Font cao 48 pixel (chỉ số)
      #define LOAD_FONT7 // Font 7 đoạn
      #define LOAD_FONT8 // Font cao 75 pixel (chỉ chữ số)
      #define LOAD_GFXFF // FreeFonts library support

      #define SMOOTH_FONT

      // Sử dụng HSPI cho màn hình
      // #define USE_HSPI_PORT // Nếu dùng VSPI thì comment dòng này

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

**Table 2: Các Định nghĩa Quan trọng trong `User_Setup.h` cho ESP32-2432S028R**

|                       |                         |                                                                                                                                                                                                                                                                                                                                                                                                                          |
| --------------------- | ----------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **Macro định nghĩa**  | **Giá trị khuyến nghị** | **Giải thích**                                                                                                                                                                                                                                                                                                                                                                                                           |
| `ILI9341_DRIVER`      | (chỉ cần `#define`)     | Kích hoạt trình điều khiển cho chip ILI9341.                                                                                                                                                                                                                                                                                                                                                                             |
| `TFT_MOSI`            | `13`                    | Chân GPIO nối với MOSI (SDI) của màn hình.                                                                                                                                                                                                                                                                                                                                                                               |
| `TFT_MISO`            | `12`                    | Chân GPIO nối với MISO (SDO) của màn hình.                                                                                                                                                                                                                                                                                                                                                                               |
| `TFT_SCLK`            | `14`                    | Chân GPIO nối với SCK (Clock) của màn hình.                                                                                                                                                                                                                                                                                                                                                                              |
| `TFT_CS`              | `15`                    | Chân GPIO nối với CS (Chip Select) của màn hình.                                                                                                                                                                                                                                                                                                                                                                         |
| `TFT_DC`              | `2`                     | Chân GPIO nối với DC (Data/Command) của màn hình.                                                                                                                                                                                                                                                                                                                                                                        |
| `TFT_RST`             | `-1`                    | Chân Reset màn hình. `-1` nếu nối với chân EN của ESP32. Nếu dùng GPIO riêng, thay bằng số GPIO đó.                                                                                                                                                                                                                                                                                                                      |
| `TFT_BL`              | `21`                    | Chân GPIO điều khiển đèn nền màn hình.                                                                                                                                                                                                                                                                                                                                                                                   |
| `TFT_BACKLIGHT_ON`    | `HIGH`                  | Mức logic để bật đèn nền (có thể là `LOW` tùy theo thiết kế mạch).                                                                                                                                                                                                                                                                                                                                                       |
| `TOUCH_CS`            | `33`                    | Chân GPIO nối với CS (Chip Select) của chip cảm ứng XPT2046.                                                                                                                                                                                                                                                                                                                                                             |
| `SPI_FREQUENCY`       | `40000000`              | Tần số SPI cho giao tiếp với màn hình (Hz). 40MHz là một lựa chọn tốt cho ILI9341.<!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><sup>3</sup><!----><!----><!----><!----><!----><!----><!----><!----><!----><!---->              |
| `SPI_READ_FREQUENCY`  | `20000000`              | Tần số SPI khi đọc dữ liệu từ màn hình (Hz).                                                                                                                                                                                                                                                                                                                                                                             |
| `SPI_TOUCH_FREQUENCY` | `2500000`               | Tần số SPI cho giao tiếp với chip cảm ứng XPT2046 (Hz). XPT2046 thường yêu cầu tần số thấp hơn.<!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><sup>3</sup><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----> |

<!---->

<!---->

<!---->

<!---->

<!---->

Sự thiếu tương thích của các file `User_Setup.h` chung chung tải từ internet với các ví dụ hoặc phần cứng cụ thể là một vấn đề thường xuyên được ghi nhận.<!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><sup>22</sup><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----> Điều này xảy ra do sự đa dạng trong thiết kế bo mạch ESP32 và cách kết nối màn hình. Một file `User_Setup.h` không chính xác sẽ dẫn đến các lỗi như màn hình không hiển thị, hiển thị sai màu, hoặc cảm ứng không hoạt động. Do đó, việc cung cấp một cấu hình `User_Setup.h` đã được kiểm chứng cho ESP32-2432S028R, hoặc hướng dẫn chi tiết từng bước để người dùng tự tạo ra nó dựa trên sơ đồ chân chính xác, là cực kỳ quan trọng để đảm bảo thành công của dự án.<!----><!----><!----><!----><!---->


## V. Xây dựng Giao diện Người dùng (GUI) với LVGL

LVGL (Light and Versatile Graphics Library) là một thư viện đồ họa mã nguồn mở, được thiết kế đặc biệt cho các hệ thống nhúng với tài nguyên hạn chế. Nó cung cấp một bộ widget phong phú và một hệ thống style mạnh mẽ để tạo ra các giao diện người dùng hấp dẫn và tương tác.<!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><sup>24</sup><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!---->

**1. Khởi tạo LVGL:**

- **Cấu hình `lv_conf.h`:** File `lv_conf.h` là file cấu hình trung tâm của LVGL. Để sử dụng, sao chép `lv_conf_template.h` (thường nằm trong thư mục gốc của thư viện LVGL) thành `lv_conf.h`. Trong môi trường PlatformIO, một cách tiếp cận tốt là đặt `lv_conf.h` vào thư mục `src` của dự án và đảm bảo nó được include (ví dụ, bằng cách sử dụng cờ `-I src` và `-D LV_CONF_INCLUDE_SIMPLE` trong `platformio.ini`). Các thiết lập quan trọng trong `lv_conf.h` cho dự án đồng hồ này bao gồm&#x20;

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <sup>

  21

  </sup>

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  :

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  - `#if 1 /*Set it to "1" to enable the content*/`: Đảm bảo dòng này được đặt thành 1 để kích hoạt các cấu hình bên trong.

  - `#define LV_COLOR_DEPTH 16`: Độ sâu màu 16-bit (RGB565) phù hợp với màn hình ILI9341.

  - `#define LV_TICK_CUSTOM 1`: Cho phép ứng dụng cung cấp "tick" (nhịp thời gian) cho LVGL, quan trọng cho việc tích hợp với hệ thống và timer.

  - Kích hoạt các phông chữ cần thiết: LVGL cung cấp nhiều phông chữ Montserrat tích hợp. Để sử dụng chúng, cần kích hoạt trong `lv_conf.h`. Ví dụ, cho đồng hồ, có thể cần:

    - `#define LV_FONT_MONTSERRAT_16 1`
    - `#define LV_FONT_MONTSERRAT_28 1`
    - `#define LV_FONT_MONTSERRAT_38 1` (hoặc 48 cho giờ lớn hơn)

  - `#define LV_FONT_DEFAULT &lv_font_montserrat_16`: Đặt phông chữ mặc định (tùy chọn).

**Table 3: Cấu hình `lv_conf.h` Cơ bản cho Dự án Đồng hồ**

|                         |                                    |                                                                                                                                  |
| ----------------------- | ---------------------------------- | -------------------------------------------------------------------------------------------------------------------------------- |
| **Macro định nghĩa**    | **Giá trị**                        | **Giải thích**                                                                                                                   |
| `LV_COLOR_DEPTH`        | `16`                               | Độ sâu màu 16-bit (RGB565), tương thích với ILI9341.                                                                             |
| `LV_TICK_CUSTOM`        | `1`                                | Cho phép ứng dụng cung cấp nguồn tick thời gian cho LVGL, cần thiết khi tích hợp với timer của ESP32.                            |
| `LV_FONT_MONTSERRAT_16` | `1`                                | Kích hoạt phông chữ Montserrat kích thước 16px. Dùng cho các text nhỏ như ngày tháng, thứ.                                       |
| `LV_FONT_MONTSERRAT_28` | `1`                                | Kích hoạt phông chữ Montserrat kích thước 28px. Dùng cho giây hoặc các thông tin phụ.                                            |
| `LV_FONT_MONTSERRAT_38` | `1`                                | Kích hoạt phông chữ Montserrat kích thước 38px (hoặc lớn hơn như 48px). Dùng cho hiển thị giờ và phút chính.                     |
| `LV_USE_LOG`            | `1` (khuyến nghị khi debug)        | Kích hoạt tính năng log của LVGL, hữu ích cho việc gỡ lỗi. Có thể tắt (0) trong bản phát hành cuối cùng để tiết kiệm tài nguyên. |
| `LV_LOG_PRINTF`         | `Serial.printf` (nếu dùng Arduino) | Định nghĩa hàm in log.                                                                                                           |

Việc cấu hình `lv_conf.h` đúng đắn là nền tảng để LVGL hoạt động hiệu quả và hiển thị giao diện như mong đợi. Đặc biệt, việc kích hoạt các font chữ cần thiết ngay từ đầu sẽ đảm bảo các thành phần văn bản được render chính xác với kích thước và kiểu dáng mong muốn.

- Tích hợp LVGL với trình điều khiển hiển thị TFT\_eSPI và cảm ứng:

  LVGL là một thư viện GUI độc lập với phần cứng. Do đó, nó cần các hàm "cầu nối" để giao tiếp với trình điều khiển màn hình (TFT\_eSPI) và trình điều khiển cảm ứng (XPT2046) cụ thể. Việc triển khai không chính xác các hàm này là một trong những nguyên nhân phổ biến gây lỗi hiển thị hoặc không nhận cảm ứng.

  - **Bộ đệm hiển thị (Display Buffer):** LVGL yêu cầu một hoặc hai bộ đệm để render đồ họa trước khi gửi đến màn hình.

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    C++

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

        static lv_disp_draw_buf_t disp_buf;
        static lv_color_t buf_1; // Kích thước buffer, ví dụ 10 dòng
        // static lv_color_t buf_2; // Buffer thứ hai (tùy chọn, cho double buffering)

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

  - **Hàm `disp_flush`:** Hàm này được LVGL gọi khi một vùng của màn hình cần được cập nhật. Nó nhận tọa độ vùng và con trỏ đến buffer màu, sau đó sử dụng các hàm của TFT\_eSPI để đẩy dữ liệu pixel ra màn hình.

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    C++

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

        // Khai báo đối tượng TFT_eSPI toàn cục
        TFT_eSPI tft = TFT_eSPI();

        void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
            uint32_t w = (area->x2 - area->x1 + 1);
            uint32_t h = (area->y2 - area->y1 + 1);

            tft.startWrite();
            tft.setAddrWindow(area->x1, area->y1, w, h);
            tft.pushColors((uint16_t *)color_p, w * h, true);
            tft.endWrite();

            lv_disp_flush_ready(disp_drv); // Báo cho LVGL biết việc flush đã hoàn tất
        }

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

  - **Hàm `touchpad_read`:** Hàm này được LVGL gọi để đọc trạng thái và tọa độ của màn hình cảm ứng.

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    C++

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

        void my_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
            uint16_t touchX, touchY;
            bool touched = tft.getTouch(&touchX, &touchY, 600); // 600 là ngưỡng áp lực (tùy chỉnh)

            if (!touched) {
                data->state = LV_INDEV_STATE_REL; // Released
            } else {
                data->state = LV_INDEV_STATE_PR; // Pressed
                data->point.x = touchX;
                data->point.y = touchY;
            }
        }

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

  - **Khởi tạo và Đăng ký Driver:** Trong hàm `setup()` hoặc một hàm khởi tạo LVGL riêng:

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    C++

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

        lv_init(); // Khởi tạo LVGL

        // Khởi tạo buffer hiển thị
        lv_disp_draw_buf_init(&disp_buf, buf_1, NULL, LV_HOR_RES_MAX * 10);

        // Khởi tạo driver hiển thị
        static lv_disp_drv_t disp_drv;
        lv_disp_drv_init(&disp_drv);
        disp_drv.hor_res = 240; // Độ phân giải ngang của màn hình ESP32-2432S028R
        disp_drv.ver_res = 320; // Độ phân giải dọc
        disp_drv.flush_cb = my_disp_flush;
        disp_drv.draw_buf = &disp_buf;
        lv_disp_drv_register(&disp_drv);

        // Khởi tạo driver đầu vào (cảm ứng)
        static lv_indev_drv_t indev_drv;
        lv_indev_drv_init(&indev_drv);
        indev_drv.type = LV_INDEV_TYPE_POINTER; // Cảm ứng là thiết bị trỏ
        indev_drv.read_cb = my_touchpad_read;
        lv_indev_drv_register(&indev_drv);

        // Khởi tạo màn hình TFT_eSPI
        tft.begin();
        tft.setRotation(0); // Hoặc 1, 2, 3 tùy theo hướng màn hình mong muốn

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

  - **Gọi `lv_tick_inc(x)` và `lv_timer_handler()`:** Trong vòng lặp chính `loop()` của Arduino, cần gọi hai hàm này định kỳ:

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    C++

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

        void loop() {
            lv_tick_inc(5); // Thông báo cho LVGL rằng 5ms đã trôi qua
            lv_timer_handler(); // Xử lý các tác vụ của LVGL (timers, animations, etc.)
            delay(5);
            // Các tác vụ khác của bạn
        }

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    `lv_tick_inc(time_ms)` thông báo cho LVGL về thời gian đã trôi qua, cần thiết cho animation và các sự kiện dựa trên thời gian. `lv_timer_handler()` xử lý các timer nội bộ của LVGL, cập nhật UI, và chạy animation.

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <sup>

    26

    </sup>

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

2\. Thiết kế giao diện đồng hồ:

Giao diện đồng hồ sẽ bao gồm các label để hiển thị giờ, phút, giây, và ngày tháng năm.

- **Tạo màn hình và các label:**

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  C++

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

      lv_obj_t *scr = lv_scr_act(); // Lấy màn hình hiện tại
      lv_obj_clean(scr); // Xóa các đối tượng cũ nếu có

      // Label cho Giờ:Phút
      lv_obj_t *label_time = lv_label_create(scr);
      lv_label_set_text(label_time, "00:00"); // Giá trị khởi tạo

      // Label cho Giây
      lv_obj_t *label_seconds = lv_label_create(scr);
      lv_label_set_text(label_seconds, ":00");

      // Label cho Ngày Tháng Năm
      lv_obj_t *label_date = lv_label_create(scr);
      lv_label_set_text(label_date, "DD/MM/YYYY");

      // Label cho Thứ
      lv_obj_t *label_day_of_week = lv_label_create(scr);
      lv_label_set_text(label_day_of_week, "Thứ");

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  Tham khảo <!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><sup>28</sup><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----> cho cấu trúc các label tương tự.<!----><!----><!----><!----><!---->

- Lựa chọn và áp dụng phông chữ:

  Sử dụng các phông chữ Montserrat đã kích hoạt trong lv\_conf.h. Việc chọn kích thước font phù hợp cho từng thành phần (giờ, phút, giây, ngày) là quan trọng để đạt được tính thẩm mỹ và dễ đọc, đáp ứng yêu cầu "giao diện đẹp".29

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  C++

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

      // Tạo style cho giờ:phút (font lớn)
      static lv_style_t style_time;
      lv_style_init(&style_time);
      lv_style_set_text_font(&style_time, &lv_font_montserrat_38); // Hoặc lv_font_montserrat_48
      lv_style_set_text_color(&style_time, lv_color_white());
      lv_obj_add_style(label_time, &style_time, 0);

      // Tạo style cho giây (font nhỏ hơn)
      static lv_style_t style_seconds;
      lv_style_init(&style_seconds);
      lv_style_set_text_font(&style_seconds, &lv_font_montserrat_28);
      lv_style_set_text_color(&style_seconds, lv_palette_main(LV_PALETTE_GREY)); // Màu xám
      lv_obj_add_style(label_seconds, &style_seconds, 0);

      // Tạo style cho ngày tháng và thứ (font nhỏ)
      static lv_style_t style_date;
      lv_style_init(&style_date);
      lv_style_set_text_font(&style_date, &lv_font_montserrat_16);
      lv_style_set_text_color(&style_date, lv_palette_lighten(LV_PALETTE_GREY, 1)); // Xám nhạt hơn
      lv_obj_add_style(label_date, &style_date, 0);
      lv_obj_add_style(label_day_of_week, &style_date, 0);

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  Tham khảo <!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><sup>25</sup><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----> về cách áp dụng font và style.<!----><!----><!----><!----><!---->

- Định vị các label:

  Sử dụng các hàm căn chỉnh của LVGL để sắp xếp các label trên màn hình.

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  C++

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

      // Căn giữa label giờ:phút
      lv_obj_align(label_time, LV_ALIGN_CENTER, 0, -30); // Điều chỉnh offset Y

      // Căn chỉnh label giây bên cạnh label giờ
      lv_obj_align_to(label_seconds, label_time, LV_ALIGN_OUT_RIGHT_MID, 5, 0); // 5px bên phải, cùng giữa Y

      // Căn chỉnh label ngày tháng bên dưới label giờ
      lv_obj_align(label_date, LV_ALIGN_CENTER, 0, 20);

      // Căn chỉnh label thứ bên dưới label ngày tháng
      lv_obj_align(label_day_of_week, LV_ALIGN_CENTER, 0, 45);

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->


## VI. Đồng bộ Thời gian qua NTP cho Múi giờ Việt Nam (GMT+7)

Để đảm bảo đồng hồ hiển thị thời gian chính xác, việc đồng bộ với máy chủ NTP là cần thiết. ESP32 cung cấp các hàm tích hợp để thực hiện việc này.

1\. Kết nối ESP32 vào mạng Wi-Fi:

Sử dụng thư viện WiFi.h để kết nối ESP32 với mạng Wi-Fi cục bộ.

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

C++

<!---->

<!---->

<!---->

<!---->

<!---->

    #include <WiFi.h>

    const char* ssid = "TEN_WIFI_CUA_BAN";
    const char* password = "MAT_KHAU_WIFI_CUA_BAN";

    void init_wifi() {
        Serial.printf("Connecting to %s ", ssid);
        WiFi.begin(ssid, password);
        while (WiFi.status()!= WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }
        Serial.println(" CONNECTED");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    }

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

Quá trình này bao gồm việc gọi `WiFi.begin()` và chờ cho đến khi `WiFi.status()` trả về `WL_CONNECTED`.<!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><sup>31</sup><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!---->

2\. Sử dụng thư viện NTP Client tích hợp:

ESP32 Arduino core đã tích hợp sẵn các chức năng NTP dựa trên time.h và sntp.h (một phần của lwIP).32 Không cần cài đặt thư viện NTPClient bên ngoài nếu sử dụng phương pháp này.

**3. Cấu hình máy chủ NTP và múi giờ GMT+7 (Indochina Time - ICT):**

- **Máy chủ NTP:** Có thể sử dụng `pool.ntp.org` (máy chủ chung) hoặc `vn.pool.ntp.org` (máy chủ ưu tiên cho khu vực Việt Nam).

- **Cấu hình múi giờ:** Phương pháp được khuyến nghị và linh hoạt nhất là sử dụng `setenv()` và `tzset()` kết hợp với `configTime()`.

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <sup>

  35

  </sup>

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  &#x20;Cách này tuân thủ chuẩn POSIX và xử lý tốt hơn các quy tắc múi giờ phức tạp (mặc dù Việt Nam không có Giờ mùa hè - DST, đây vẫn là cách tiếp cận chuẩn).

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  C++

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

      #include "time.h" // Đã có trong ESP32 core

      const char* ntpServer = "pool.ntp.org";
      // Hoặc const char* ntpServer = "vn.pool.ntp.org";

      // Timezone String cho GMT+7 (Indochina Time)
      // "ICT" là tên múi giờ (Indochina Time).
      // "-7" là offset từ UTC. Dấu âm trong chuỗi POSIX TZ có nghĩa là thời gian địa phương
      // đi trước UTC (ví dụ: UTC+7).
      const char* tzString = "ICT-7";

      void init_ntp() {
          // Khởi tạo thời gian với máy chủ NTP, ban đầu lấy giờ UTC (offset 0, DST 0)
          configTime(0, 0, ntpServer);

          // Đặt biến môi trường TZ (Time Zone)
          setenv("TZ", tzString, 1); // Số 1 nghĩa là ghi đè nếu biến TZ đã tồn tại

          // Cập nhật thông tin múi giờ cho thư viện C
          tzset();

          Serial.println("NTP and Timezone configured for ICT (GMT+7).");
      }

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  Việc sử dụng `configTime(0,0,ntpServer)` để lấy giờ UTC, sau đó dùng `setenv()` và `tzset()` để áp dụng múi giờ địa phương là một quy trình chuẩn. Chuỗi `ICT-7` chỉ định múi giờ Indochina Time với offset là +7 giờ so với UTC.

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

4\. Hàm cập nhật thời gian từ NTP và xử lý trạng thái đồng bộ:

Sau khi cấu hình, thời gian sẽ được tự động đồng bộ. Có thể lấy thời gian cục bộ bằng getLocalTime().

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

C++

<!---->

<!---->

<!---->

<!---->

<!---->

    struct tm timeinfo; // Struct để lưu trữ thông tin thời gian chi tiết

    bool getTime() {
        if (!getLocalTime(&timeinfo)) {
            Serial.println("Failed to obtain time");
            return false;
        }
        // timeinfo bây giờ chứa các thành phần:
        // timeinfo.tm_year + 1900 : Năm
        // timeinfo.tm_mon + 1    : Tháng (0-11, nên +1)
        // timeinfo.tm_mday       : Ngày
        // timeinfo.tm_hour       : Giờ
        // timeinfo.tm_min        : Phút
        // timeinfo.tm_sec        : Giây
        // timeinfo.tm_wday       : Thứ trong tuần (0=Chủ nhật, 1=Thứ Hai,..., 6=Thứ Bảy)
        return true;
    }

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

Struct `tm timeinfo` chứa các thành phần ngày giờ chi tiết.<!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><sup>32</sup><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!---->

- **Đồng bộ định kỳ:** Đồng hồ RTC nội bộ của ESP32 có thể bị trôi. Do đó, nên có cơ chế cập nhật lại thời gian từ NTP định kỳ (ví dụ, mỗi giờ hoặc vài giờ) để duy trì độ chính xác.

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <sup>

  28

  </sup>

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  &#x20;Điều này có thể được thực hiện bằng cách gọi lại `configTime()` hoặc để cơ chế tự động của sntp hoạt động (mặc định là mỗi giờ).

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

- **Kiểm tra trạng thái đồng bộ:** Hàm `sntp_get_sync_status()` có thể được sử dụng để kiểm tra xem thời gian đã được đồng bộ hóa hay chưa. `SNTP_SYNC_STATUS_COMPLETED` cho biết đã đồng bộ thành công.

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <sup>

  35

  </sup>

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  C++

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

      #include "sntp.h" // Cần include để dùng sntp_get_sync_status()

      bool isTimeSynced() {
          return sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED;
      }

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  Có thể hiển thị một chỉ báo trên màn hình (ví dụ: một biểu tượng nhỏ) nếu thời gian chưa được đồng bộ.

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->


## VII. Tạo Hiệu ứng Animation cho Giao diện Đồng hồ

LVGL cung cấp một hệ thống animation linh hoạt, cho phép thay đổi các thuộc tính của đối tượng (widgets) theo thời gian, tạo ra các hiệu ứng chuyển động mượt mà.<!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><sup>38</sup><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!---->

1\. Giới thiệu về cơ chế animation của LVGL:

Một animation trong LVGL được định nghĩa bởi các thông số chính 40:

- `var`: Con trỏ tới biến hoặc đối tượng cần animate.
- `exec_cb`: Hàm callback được gọi ở mỗi bước của animation để cập nhật giá trị của `var`.
- `start_value`, `end_value`: Giá trị bắt đầu và kết thúc của thuộc tính được animate.
- `time`: Tổng thời gian diễn ra animation (ms).
- `path_cb`: Hàm callback định nghĩa "đường cong" của animation (ví dụ: linear, ease-in, ease-out).
- `delay`, `repeat_count`, `playback_time`, v.v.: Các tùy chọn khác.

2\. Kỹ thuật tạo animation cho sự thay đổi của các chữ số:

Khi các chữ số của đồng hồ thay đổi (ví dụ: giây từ '9' sang '0', hoặc phút từ '59' sang '00'), việc thêm hiệu ứng chuyển tiếp sẽ làm cho giao diện trở nên sống động và hấp dẫn hơn.

Phương án đề xuất: Trượt Lên/Xuống (Slide In/Out) kết hợp Fade:

Phương án này cân bằng giữa hiệu quả hình ảnh và độ phức tạp triển khai. Ý tưởng là khi một chữ số thay đổi, chữ số cũ sẽ trượt ra khỏi vị trí và mờ dần, trong khi chữ số mới trượt vào vị trí và hiện rõ dần.

- **Chuẩn bị:**

  - Đối với mỗi vị trí chữ số cần animate (ví dụ: hàng đơn vị của giây, hàng chục của giây, v.v.), chúng ta cần một label chính để hiển thị chữ số hiện tại.
  - Khi một chữ số sắp thay đổi, chúng ta sẽ tạo một label tạm thời cho chữ số mới.

- Hàm thực thi animation (Custom exec\_cb):

  Chúng ta sẽ tạo một hàm exec\_cb tùy chỉnh để đồng thời thay đổi vị trí y và độ mờ opa của label.

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  C++

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

      // Struct để truyền dữ liệu cho animation chữ số
      typedef struct {
          lv_obj_t *label_old; // Label chứa chữ số cũ
          lv_obj_t *label_new; // Label chứa chữ số mới
          lv_coord_t y_start_old; // Vị trí y ban đầu của label cũ
          lv_coord_t y_start_new; // Vị trí y ban đầu của label mới (ẩn)
          lv_coord_t y_end_old;   // Vị trí y kết thúc của label cũ (ẩn)
          lv_coord_t y_end_new;   // Vị trí y kết thúc của label mới (hiển thị)
      } digit_anim_data_t;

      // Hàm callback cho animation trượt và mờ
      static void digit_slide_fade_anim_cb(void *data, int32_t v) {
          digit_anim_data_t *anim_data = (digit_anim_data_t *)data;

          // Animate label cũ: trượt lên và mờ dần
          if (anim_data->label_old) {
              lv_obj_set_y(anim_data->label_old, anim_data->y_start_old - v); // v từ 0 đến delta_y
              lv_obj_set_style_opa(anim_data->label_old, LV_OPA_COVER - (v * LV_OPA_COVER / (anim_data->y_start_old - anim_data->y_end_old)), 0);
          }

          // Animate label mới: trượt lên từ dưới và hiện rõ dần
          if (anim_data->label_new) {
              lv_obj_set_y(anim_data->label_new, anim_data->y_start_new - v);
              lv_obj_set_style_opa(anim_data->label_new, (v * LV_OPA_COVER / (anim_data->y_start_new - anim_data->y_end_new)), 0);
          }
      }

      // Hàm callback khi animation hoàn tất
      static void digit_anim_ready_cb(lv_anim_t *a) {
          digit_anim_data_t *anim_data = (digit_anim_data_t *)a->var;
          if (anim_data->label_old) {
              lv_obj_del(anim_data->label_old); // Xóa label cũ
          }
          // label_new bây giờ trở thành label chính
          // Cập nhật lại con trỏ label chính nếu cần
          free(anim_data); // Giải phóng bộ nhớ cho anim_data
      }

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

- Kích hoạt animation:

  Khi phát hiện một chữ số cần thay đổi (ví dụ, current\_seconds\_unit khác previous\_seconds\_unit):

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  C++

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

      void animate_digit_change(lv_obj_t **current_label_obj, lv_coord_t x_pos, lv_coord_t y_pos, const char *new_digit_text, const lv_font_t *font, lv_color_t color) {
          // 1. Tạo label mới cho chữ số mới, đặt ở vị trí bắt đầu (ví dụ: bên dưới vị trí hiển thị)
          lv_obj_t *label_new = lv_label_create(lv_scr_act());
          lv_label_set_text(label_new, new_digit_text);
          lv_obj_set_style_text_font(label_new, font, 0);
          lv_obj_set_style_text_color(label_new, color, 0);
          lv_obj_set_pos(label_new, x_pos, y_pos + 20); // Bắt đầu từ dưới, 20 là khoảng cách trượt
          lv_obj_set_style_opa(label_new, LV_OPA_TRANSP, 0); // Ban đầu trong suốt

          // 2. Chuẩn bị dữ liệu cho animation
          digit_anim_data_t *anim_data = (digit_anim_data_t *)malloc(sizeof(digit_anim_data_t));
          anim_data->label_old = *current_label_obj; // Label hiện tại sẽ là label cũ
          anim_data->label_new = label_new;
          anim_data->y_start_old = y_pos;
          anim_data->y_start_new = y_pos + 20; // Vị trí bắt đầu của label mới
          anim_data->y_end_old = y_pos - 20;   // Label cũ trượt lên
          anim_data->y_end_new = y_pos;       // Label mới trượt vào vị trí

          // 3. Khởi tạo animation
          lv_anim_t a;
          lv_anim_init(&a);
          lv_anim_set_var(&a, anim_data); // Truyền con trỏ anim_data
          lv_anim_set_exec_cb(&a, digit_slide_fade_anim_cb);
          lv_anim_set_values(&a, 0, 20); // Animate dựa trên delta_y là 20
          lv_anim_set_time(&a, 300);     // Thời gian animation: 300ms
          lv_anim_set_path_cb(&a, lv_anim_path_ease_out); // Đường cong animation
          lv_anim_set_ready_cb(&a, digit_anim_ready_cb);  // Callback khi hoàn tất
          // lv_anim_set_deleted_cb(&a, deleted_cb); // Nếu cần callback khi anim bị xóa

          lv_anim_start(&a);

          // Cập nhật con trỏ label hiện tại để trỏ đến label mới
          *current_label_obj = label_new;
      }

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  Trong hàm cập nhật hiển thị thời gian, khi một chữ số (ví dụ, đơn vị của giây) thay đổi, bạn sẽ gọi hàm `animate_digit_change` này. Cần có các biến để lưu trữ giá trị trước đó của từng chữ số để phát hiện sự thay đổi.

  Quá trình này đòi hỏi quản lý cẩn thận vòng đời của các đối tượng label và bộ nhớ được cấp phát cho `anim_data`. Label cũ sẽ bị xóa sau khi animation hoàn thành, và label mới sẽ thay thế vị trí của nó.

- Animation cho dấu ":" nhấp nháy:

  Dấu hai chấm giữa giờ và phút, hoặc phút và giây, có thể được làm nhấp nháy bằng cách sử dụng một lv\_timer để thay đổi thuộc tính opa (độ mờ) của label chứa dấu ":" một cách định kỳ.

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  C++

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

      lv_obj_t *label_colon; // Label cho dấu ":"
      static bool colon_visible = true;

      static void colon_blink_timer_cb(lv_timer_t *timer) {
          if (label_colon) {
              colon_visible =!colon_visible;
              lv_obj_set_style_opa(label_colon, colon_visible? LV_OPA_COVER : LV_OPA_TRANSP, 0);
          }
      }

      // Trong phần tạo UI:
      // label_colon = lv_label_create(scr);
      // lv_label_set_text(label_colon, ":");
      //... (set style, position)...
      // lv_timer_create(colon_blink_timer_cb, 500, NULL); // Nhấp nháy mỗi 500ms

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

Việc triển khai animation đòi hỏi sự chú ý đến chi tiết trong việc quản lý đối tượng và tài nguyên, nhưng sẽ mang lại trải nghiệm người dùng phong phú và hiện đại hơn.


## VIII. Chương trình Mẫu Hoàn chỉnh và Giải thích Chi tiết

Phần này sẽ phác thảo cấu trúc của một chương trình mẫu hoàn chỉnh và giải thích các thành phần chính.

**1. Cấu trúc file `main.cpp`:**

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

C++

<!---->

<!---->

<!---->

<!---->

<!---->

    #include <Arduino.h>
    #include <WiFi.h>
    #include "time.h"
    #include "sntp.h"
    #include <TFT_eSPI.h>
    #include "lvgl.h" // Đảm bảo lv_conf.h được include đúng cách

    // --- Khai báo Cấu hình và Hằng số ---
    const char* WIFI_SSID = "TEN_WIFI_CUA_BAN";
    const char* WIFI_PASSWORD = "MAT_KHAU_WIFI_CUA_BAN";
    const char* NTP_SERVER = "vn.pool.ntp.org"; // Hoặc pool.ntp.org
    const char* TZ_STRING = "ICT-7"; // Múi giờ Việt Nam (GMT+7)

    // Kích thước màn hình
    #define SCREEN_WIDTH  240
    #define SCREEN_HEIGHT 320

    // --- Biến toàn cục cho TFT và LVGL ---
    TFT_eSPI tft = TFT_eSPI();
    static lv_disp_draw_buf_t disp_buf;
    static lv_color_t buf_1; // Buffer cho LVGL (20 dòng)

    // Con trỏ tới các label LVGL
    lv_obj_t *label_hour_minute;
    lv_obj_t *label_seconds;
    lv_obj_t *label_date;
    lv_obj_t *label_day_of_week;
    lv_obj_t *label_colon_hm; // Dấu : giữa giờ và phút
    lv_obj_t *label_colon_ms; // Dấu : giữa phút và giây (nếu hiển thị)
    lv_obj_t *label_ntp_status;

    // Biến lưu trữ thời gian trước đó để phát hiện thay đổi (cho animation)
    int prev_h = -1, prev_m = -1, prev_s = -1;
    char prev_date_str[1] = "";
    char prev_dow_str[1] = "";

    // --- Các hàm callback cho LVGL (đã định nghĩa ở Mục V) ---
    void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p);
    void my_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);

    // --- Các hàm khởi tạo ---
    void init_display();
    void init_lvgl_drivers();
    void init_wifi();
    void init_ntp();

    // --- Hàm tạo giao diện ---
    void create_clock_ui();

    // --- Hàm cập nhật hiển thị thời gian ---
    void update_time_display(const struct tm &timeinfo);

    // --- Hàm và biến cho animation (phác thảo, cần hoàn thiện chi tiết) ---
    // (Khai báo các hàm animate_digit_change và các biến liên quan như trong Mục VII)
    // Ví dụ:
    // void animate_text_change(lv_obj_t *label, const char *new_text, lv_coord_t target_y_offset, uint32_t duration);

    // --- Tác vụ FreeRTOS cho đồng bộ NTP (tùy chọn nâng cao) ---
    // void ntp_sync_task(void *parameter);

    // --- Hàm setup ---
    void setup() {
        Serial.begin(115200);
        Serial.println("ESP32 NTP Clock Starting...");

        init_display();
        init_lvgl_drivers();
        create_clock_ui(); // Tạo UI ban đầu với giá trị placeholder

        init_wifi();
        if (WiFi.status() == WL_CONNECTED) {
            init_ntp();
            // xTaskCreate(ntp_sync_task, "NTP_Sync", 4096, NULL, 5, NULL); // Nếu dùng task riêng
        } else {
            lv_label_set_text(label_ntp_status, "WiFi Err");
        }
        // Hiển thị "Syncing..." ban đầu
        if (label_ntp_status) lv_label_set_text(label_ntp_status, "NTP Sync...");
    }

    // --- Hàm loop ---
    unsigned long last_time_update = 0;
    unsigned long last_ntp_check = 0;
    bool initial_sync_done = false;

    void loop() {
        lv_tick_inc(5); // Cung cấp tick cho LVGL
        lv_timer_handler(); // Xử lý các timer của LVGL
        delay(5);

        if (WiFi.status() == WL_CONNECTED) {
            if (millis() - last_time_update > 1000) { // Cập nhật mỗi giây
                last_time_update = millis();
                struct tm timeinfo;
                if (getLocalTime(&timeinfo, 50)) { // Timeout 50ms
                    if (!initial_sync_done && timeinfo.tm_year > (2020 - 1900)) { // Check if time is valid
                         initial_sync_done = true;
                         if (label_ntp_status) lv_label_set_text(label_ntp_status, LV_SYMBOL_WIFI " Synced");
                    }
                    update_time_display(timeinfo);
                } else if (initial_sync_done) {
                    // Thời gian đã từng đồng bộ, nhưng giờ getLocalTime thất bại (hiếm)
                    if (label_ntp_status) lv_label_set_text(label_ntp_status, "Time Err");
                }
            }

            // Kiểm tra trạng thái NTP định kỳ (ví dụ mỗi 30 giây sau khi đã đồng bộ)
            if (initial_sync_done && millis() - last_ntp_check > 30000) {
                last_ntp_check = millis();
                if (sntp_get_sync_status()!= SNTP_SYNC_STATUS_COMPLETED) {
                    if (label_ntp_status) lv_label_set_text(label_ntp_status, "NTP Sync...");
                    // Có thể thử khởi động lại NTP sync nếu cần
                    // sntp_stop(); // Dừng sntp cũ (nếu có)
                    // init_ntp();  // Khởi tạo lại
                } else {
                     if (label_ntp_status) lv_label_set_text(label_ntp_status, LV_SYMBOL_WIFI " Synced");
                }
            }
        } else {
            // Xử lý mất kết nối WiFi
            if (label_ntp_status) lv_label_set_text(label_ntp_status, "WiFi Err");
            // Thử kết nối lại WiFi sau một khoảng thời gian
            // init_wifi(); // Cẩn thận có thể gây blocking
        }
    }

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

**2. Giải thích từng phần code:**

- **Khởi tạo (`init_display`, `init_lvgl_drivers`, `init_wifi`, `init_ntp`):**

  - `init_display()`: Khởi tạo thư viện `TFT_eSPI` (`tft.begin()`, `tft.setRotation()`).
  - `init_lvgl_drivers()`: Khởi tạo LVGL (`lv_init()`), thiết lập display buffer, đăng ký hàm `my_disp_flush` và `my_touchpad_read` như đã mô tả ở Mục V.
  - `init_wifi()`: Kết nối ESP32 vào mạng Wi-Fi đã cấu hình.
  - `init_ntp()`: Cấu hình máy chủ NTP và múi giờ Việt Nam sử dụng `configTime()`, `setenv()`, và `tzset()` như Mục VI.

- **Tạo giao diện (`create_clock_ui`):**

  - Sử dụng các hàm LVGL để tạo các đối tượng label (`lv_label_create`) cho giờ, phút, giây (tùy chọn), ngày, tháng, năm, thứ trong tuần, và dấu hai chấm.

  - Tạo label cho trạng thái NTP (`label_ntp_status`).

  - Áp dụng các style (phông chữ, màu sắc) đã định nghĩa trước cho từng label.

  - Định vị các label trên màn hình sử dụng `lv_obj_align`, `lv_obj_align_to` hoặc layout của LVGL.

  - Ví dụ, tạo label giờ phút:

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    C++

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

        // Trong create_clock_ui()
        label_hour_minute = lv_label_create(lv_scr_act());
        static lv_style_t style_hm; // Khai báo static để style tồn tại
        lv_style_init(&style_hm);
        lv_style_set_text_font(&style_hm, &lv_font_montserrat_38); // Chọn font lớn
        lv_style_set_text_color(&style_hm, lv_color_white());
        lv_obj_add_style(label_hour_minute, &style_hm, 0);
        lv_label_set_text(label_hour_minute, "00:00");
        lv_obj_align(label_hour_minute, LV_ALIGN_CENTER, 0, -40); // Căn giữa, dịch lên trên
        // Tương tự cho các label khác

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

- **Cập nhật thời gian (`update_time_display`):**

  - Hàm này được gọi mỗi giây từ `loop()`.

  - Nhận `struct tm timeinfo` làm đầu vào.

  - Định dạng các thành phần thời gian (giờ, phút, giây, ngày, tháng, năm, thứ) thành chuỗi.

  - Sử dụng `lv_label_set_text()` để cập nhật nội dung cho các label tương ứng.

  - Triển khai logic phát hiện sự thay đổi của từng chữ số so với giá trị trước đó (`prev_h`, `prev_m`, `prev_s`, etc.) để kích hoạt animation.

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    C++

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

        void update_time_display(const struct tm &timeinfo) {
            char buf[2];

            // Giờ và Phút
            snprintf(buf, sizeof(buf), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
            // if (timeinfo.tm_hour!= prev_h |

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

\| timeinfo.tm\_min!= prev\_m) {

// // Gọi hàm animation cho label\_hour\_minute nếu có thay đổi

// // animate\_text\_change(label\_hour\_minute, buf,...);

// } else {

lv\_label\_set\_text(label\_hour\_minute, buf);

// }

prev\_h = timeinfo.tm\_hour;

prev\_m = timeinfo.tm\_min;

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

            // Giây
            snprintf(buf, sizeof(buf), ":%02d", timeinfo.tm_sec);
            // Tương tự, kiểm tra và animate cho label_seconds
            lv_label_set_text(label_seconds, buf); // Nối vào sau label giờ phút hoặc label riêng
            prev_s = timeinfo.tm_sec;


            // Ngày tháng năm
            char date_str[1];
            snprintf(date_str, sizeof(date_str), "%02d/%02d/%04d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
            if (strcmp(date_str, prev_date_str)!= 0) {
                 lv_label_set_text(label_date, date_str);
                 strcpy(prev_date_str, date_str);
            }


            // Thứ trong tuần (tiếng Việt)
            const char* days_of_week_vn = {"Chủ Nhật", "Thứ Hai", "Thứ Ba", "Thứ Tư", "Thứ Năm", "Thứ Sáu", "Thứ Bảy"};
            if (strcmp(days_of_week_vn[timeinfo.tm_wday], prev_dow_str)!= 0) {
                lv_label_set_text(label_day_of_week, days_of_week_vn[timeinfo.tm_wday]);
                strcpy(prev_dow_str, days_of_week_vn[timeinfo.tm_wday]);
            }

            // Cập nhật trạng thái nhấp nháy của dấu ":" (nếu có)
        }
        ```

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

<!---->

- **Xử lý animation (`animate_digit_change`):**

  - Triển khai hàm này như đã mô tả chi tiết trong Mục VII, sử dụng `lv_anim_...` để tạo hiệu ứng trượt và mờ khi chữ số thay đổi.
  - Cần quản lý cẩn thận các con trỏ tới label và bộ nhớ động.

- Quản lý tác vụ cho NTP (Nâng cao):

  Để giao diện người dùng (đặc biệt là animation) luôn mượt mà, các tác vụ có khả năng chặn (blocking) như kết nối Wi-Fi hoặc đồng bộ NTP lần đầu nên được xử lý cẩn thận.

  Trong ESP32 Arduino core, FreeRTOS đã có sẵn. Có thể tạo một tác vụ (task) riêng để xử lý việc đồng bộ NTP định kỳ trong nền.

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  C++

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

      // Khai báo task (nếu dùng)
      // TaskHandle_t ntpTaskHandle = NULL;

      // void ntp_sync_task(void *parameter) {
      //     for (;;) {
      //         if (WiFi.status() == WL_CONNECTED) {
      //             // Thực hiện đồng bộ NTP ở đây
      //             // Gọi configTime, setenv, tzset nếu cần đồng bộ lại hoàn toàn
      //             // Hoặc dựa vào cơ chế tự động cập nhật của sntp
      //             Serial.println("NTP Sync Task: Attempting sync...");
      //             // Đợi cho đến khi đồng bộ hoặc timeout
      //             // Cập nhật biến trạng thái toàn cục (có mutex bảo vệ nếu cần)
      //         }
      //         vTaskDelay(pdMS_TO_TICKS(3600000)); // Đồng bộ mỗi giờ
      //     }
      // }

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  Nếu không sử dụng task riêng, cần đảm bảo các lệnh gọi NTP không chặn `loop()` quá lâu, vì `lv_timer_handler()` cần được gọi thường xuyên.<!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><sup>26</sup><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----> `getLocalTime()` với timeout nhỏ là một giải pháp. Việc đồng bộ NTP ban đầu trong `setup()` có thể chấp nhận được vì nó chỉ xảy ra một lần.<!----><!----><!----><!----><!---->


## IX. Biên dịch, Nạp chương trình và Kiểm thử

**1. Biên dịch và Nạp chương trình:**

- Sử dụng PlatformIO:

  - Mở Terminal trong VS Code (PlatformIO CLI).
  - Biên dịch: `pio run`
  - Nạp chương trình: `pio run -t upload`
  - Mở Serial Monitor: `pio device monitor --baud 115200`

- Đảm bảo đã chọn đúng board (`esp32dev`) và cổng COM/ttyUSB.

**2. Kiểm thử:**

- **Serial Monitor:** Theo dõi các thông báo khởi tạo, quá trình kết nối Wi-Fi, trạng thái đồng bộ NTP. Các thông báo lỗi (nếu có) sẽ xuất hiện ở đây.

- **Màn hình hiển thị:**

  - Kiểm tra xem thời gian, ngày tháng có hiển thị đúng định dạng và múi giờ Việt Nam không.
  - Quan sát các hiệu ứng animation khi các chữ số (đặc biệt là giây) thay đổi. Hiệu ứng có mượt mà không?
  - Kiểm tra trạng thái NTP (ví dụ: "Syncing...", "Synced", "WiFi Err").
  - Kiểm tra hoạt động của màn hình cảm ứng (nếu có các yếu tố tương tác được thêm vào).

- **Độ chính xác thời gian:** So sánh thời gian hiển thị trên ESP32 với một nguồn thời gian chuẩn (ví dụ: điện thoại, máy tính đã đồng bộ NTP).

- **Kiểm tra sau khi mất điện:** Khi ESP32 khởi động lại, nó có tự động kết nối Wi-Fi và đồng bộ lại thời gian không?

**3. Gỡ lỗi và Các vấn đề thường gặp:**

- **Màn hình trắng hoặc không hiển thị gì:**

  - Kiểm tra lại kết nối phần cứng (sơ đồ chân).
  - Kiểm tra kỹ file `User_Setup.h` của TFT\_eSPI (các định nghĩa chân `TFT_CS`, `TFT_DC`, `TFT_RST`, `SPI_FREQUENCY`).
  - Đảm bảo driver `ILI9341_DRIVER` được kích hoạt.

- **Màu sắc hiển thị sai:**

  - Kiểm tra định nghĩa `ILI9341_ क्रम_MÀU` (ví dụ `TFT_RGB` hoặc `TFT_BGR`) trong `User_Setup.h` nếu có.
  - Đảm bảo `LV_COLOR_DEPTH` trong `lv_conf.h` là `16`.

- **Cảm ứng không hoạt động hoặc sai tọa độ:**

  - Kiểm tra chân `TOUCH_CS` và các chân SPI cho cảm ứng trong `User_Setup.h`.
  - Kiểm tra `SPI_TOUCH_FREQUENCY`.
  - Có thể cần hiệu chỉnh (calibrate) cảm ứng. TFT\_eSPI có các ví dụ hiệu chỉnh.

- **Không kết nối được Wi-Fi:**

  - Kiểm tra lại SSID và mật khẩu Wi-Fi trong code.
  - Đảm bảo ESP32 nằm trong vùng phủ sóng Wi-Fi.

- **Thời gian không đồng bộ hoặc sai múi giờ:**

  - Kiểm tra kết nối Internet.
  - Kiểm tra cấu hình `NTP_SERVER` và `TZ_STRING`.
  - Theo dõi log trên Serial Monitor để xem quá trình đồng bộ NTP.
  - Đảm bảo `sntp_get_sync_status()` trả về `SNTP_SYNC_STATUS_COMPLETED`.

- **LVGL báo lỗi hoặc giao diện bị treo:**

  - Đảm bảo `lv_tick_inc()` và `lv_timer_handler()` được gọi đều đặn và không bị chặn bởi các tác vụ khác quá lâu.

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <sup>

    26

    </sup>

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

  - Kiểm tra việc cấp phát và giải phóng bộ nhớ cho các đối tượng LVGL, đặc biệt là trong các hàm animation.

  - Kích hoạt log của LVGL (`LV_USE_LOG 1` trong `lv_conf.h`) để xem thông báo lỗi chi tiết.

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <sup>

    41

    </sup>

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->

    <!---->


## X. Hướng phát triển và Tùy chỉnh Thêm

Dự án đồng hồ NTP này là một nền tảng tốt để phát triển thêm nhiều tính năng hấp dẫn khác:

- **Giao diện cài đặt Wi-Fi:** Thay vì hardcode SSID và mật khẩu, có thể triển khai cơ chế cho phép người dùng nhập thông tin Wi-Fi qua giao diện cảm ứng, SmartConfig, hoặc một trang web cấu hình nhỏ do ESP32 host.

- **Hiển thị thông tin thời tiết:** Kết nối với các API thời tiết (ví dụ: OpenWeatherMap) để lấy và hiển thị nhiệt độ, độ ẩm, dự báo thời tiết hiện tại.

- **Chức năng báo thức:** Thêm giao diện để đặt báo thức, lưu trữ thời gian báo thức (có thể vào NVS - Non-Volatile Storage của ESP32), và kích hoạt âm thanh hoặc đèn khi đến giờ.

- **Tùy chỉnh giao diện:** Cho phép người dùng thay đổi màu sắc chủ đạo, kiểu phông chữ, hoặc hình nền của đồng hồ thông qua một menu cài đặt.

- **Tích hợp cảm biến:** Kết nối các cảm biến như DHT11/DHT22 (nhiệt độ, độ ẩm), BME280 (nhiệt độ, độ ẩm, áp suất) và hiển thị dữ liệu lên màn hình cùng với thời gian. Bo mạch ESP32-2432S028R thường có sẵn cổng chờ cho DHT11.

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <sup>

  3

  </sup>

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

- **Sử dụng thẻ SD:** Lưu trữ các file phông chữ lớn, hình ảnh nền tùy chỉnh, file âm thanh cho báo thức, hoặc ghi log hệ thống. Bo mạch này có khe cắm thẻ SD.

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <sup>

  3

  </sup>

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

  <!---->

- **Chế độ hiển thị ban đêm:** Tự động giảm độ sáng màn hình hoặc chuyển sang giao diện tối màu vào ban đêm.

- **Nhiều múi giờ:** Cho phép người dùng chọn và hiển thị thời gian của các múi giờ khác nhau.

Những cải tiến này không chỉ làm tăng tính hữu dụng của thiết bị mà còn giúp người phát triển thực hành sâu hơn với ESP32, LVGL và các công nghệ IoT liên quan.


## XI. Kết luận

Báo cáo này đã trình bày một cách tiếp cận chi tiết để xây dựng một ứng dụng đồng hồ hiển thị thời gian Việt Nam, đồng bộ qua NTP, với giao diện đồ họa đẹp mắt và hiệu ứng chuyển động trên bo mạch ESP32-2432S028R. Từ việc tìm hiểu phần cứng, thiết lập môi trường phát triển PlatformIO, cấu hình các thư viện quan trọng như TFT\_eSPI và LVGL, cho đến việc triển khai logic đồng bộ thời gian và tạo các animation phức tạp, mỗi bước đều được phân tích và hướng dẫn cụ thể.

Kết quả của dự án là một minh chứng cho khả năng mạnh mẽ của vi điều khiển ESP32 kết hợp với thư viện LVGL trong việc tạo ra các ứng dụng nhúng có giao diện người dùng phong phú và tương tác. Bo mạch ESP32-2432S028R, với màn hình cảm ứng tích hợp và giá thành hợp lý, là một lựa chọn tuyệt vời cho các dự án IoT đòi hỏi hiển thị đồ họa.

Thông qua dự án này, người đọc có thể nắm vững các kỹ thuật cần thiết để làm việc với màn hình TFT, cảm ứng, thư viện đồ họa LVGL, và giao thức NTP trên nền tảng ESP32. Đồng thời, các gợi ý về hướng phát triển mở ra nhiều khả năng tùy biến và mở rộng, khuyến khích sự sáng tạo và khám phá không ngừng trong lĩnh vực hệ thống nhúng và IoT.
