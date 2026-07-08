#line 1 "/home/rada/Projects/wifi_usb_storage/config.h"
#ifndef CONFIG_H
#define CONFIG_H

// Target board: Waveshare ESP32-C6-LCD-1.47 (ST7789, 172x320 IPS).
// Uncomment to use the Adafruit driver instead of the bundled custom one.
// The custom driver (ST7789_Custom.h) handles the 172x320 offsets and the
// remappable ESP32-C6 SPI pins below.
// #define USE_ADAFRUIT_ST7789


#define SCREEN_W  172
#define SCREEN_H  320

#define AP_NAME_PREFIX          "WiFi_Storage"         // Will be appended by device MAC
#define AP_PASS                 "pass1234"

// Once the station is connected, turn the AP off this long after the last AP
// client disconnects (saves power by dropping to station-only mode). The AP is
// brought back automatically if the station connection is later lost.
#define AP_AUTO_OFF_MS          (120000)

// WiFi credentials live in RAM only. If an SD card is present they are also
// mirrored to this file (ssid on line 1, password on line 2) so they survive
// a reboot; without a card (or before one is inserted) they are RAM-only.
#define WIFI_CFG_PATH           "/wifi.cfg"

// ESP32-C6-LCD-1.47 display pins (Waveshare wiki)
#define TFT_MOSI  6
#define TFT_SCLK  7
#define TFT_CS    14
#define TFT_DC    15
#define TFT_RST   21
#define TFT_BL    22

// Backlight PWM. Docs warn against full brightness for long periods, so default
// to ~40% duty (8-bit resolution, 102/255).
#define TFT_BL_FREQ       5000
#define TFT_BL_RES_BITS   8
#define TFT_BL_DUTY       102

// microSD (TF) card. Shares the SPI bus with the LCD (MOSI=6, SCLK=7);
// adds MISO and its own chip select.
#define SD_MISO           5
#define SD_CS             4
#define SD_SPI_FREQ       20000000

#endif
