#ifndef CONFIG_H
#define CONFIG_H

#define VERSION "1.3.0"

// Where the device checks for a newer firmware version: a plain text file
// holding just the version string, fetched over HTTPS. First checked
// UPDATE_FIRST_CHECK_DELAY_MS after the station connects (not immediately -
// gives WiFi/DNS time to settle), then every UPDATE_CHECK_INTERVAL_MS
// thereafter. The "Check for Update" menu button bypasses both delays.
#define UPDATE_VERSION_URL      "https://raw.githubusercontent.com/ujagaga/wifi_usb_storage/refs/heads/main/build/version.txt"
#define UPDATE_FIRMWARE_URL     "https://raw.githubusercontent.com/ujagaga/wifi_usb_storage/refs/heads/main/build/wifi_usb_storage.ino.bin"
#define UPDATE_MD5_URL          "https://raw.githubusercontent.com/ujagaga/wifi_usb_storage/refs/heads/main/build/wifi_usb_storage.ino.bin.md5"
#define UPDATE_FIRST_CHECK_DELAY_MS (2UL * 60 * 1000UL)
#define UPDATE_CHECK_INTERVAL_MS (24UL * 60 * 60 * 1000UL)
#define UPDATE_FIRMWARE_FILENAME "firmware_update.bin"  // fallback name if the remote version is unknown; normally "firmware_update_<version>.bin"
// Uncomment to use the Adafruit driver instead of the bundled custom one.
// The custom driver (ST7789_Custom.h) handles the 172x320 offsets and the
// remappable SPI pins below.
// #define USE_ADAFRUIT_ST7789


#define SCREEN_W  172
#define SCREEN_H  320

#define AP_NAME_PREFIX          "WiFi_Storage"         // Will be appended by device MAC
#define AP_PASS                 "pass1234"

// Once the station is connected, turn the AP off this long after the last AP
// client disconnects (saves power by dropping to station-only mode). The AP is
// brought back automatically if the station connection is later lost.
#define AP_AUTO_OFF_MS          (120000)

// WiFi credentials (and screen brightness) live in RAM only. If an SD card is
// present they are also mirrored to this file (ssid on line 1, password on
// line 2, brightness percent on line 3) so they survive a reboot; without a
// card (or before one is inserted) they are RAM-only.
#define WIFI_CFG_PATH           "/config.txt"

// Backlight PWM. Docs warn against full brightness for long periods, so default
// to ~20% duty (8-bit resolution, 50/255).
#define TFT_BL_FREQ       5000
#define TFT_BL_RES_BITS   8
#define TFT_BL_DUTY       50

// Waveshare ESP32-S3-LCD-1.47 display pins (Waveshare wiki). Has native USB,
// so the SD card is exposed read-only over USB (see usb_msc.cpp) at the same
// time it's served over WiFi.
#define TFT_MOSI  45
#define TFT_SCLK  40
#define TFT_CS    42
#define TFT_DC    41
#define TFT_RST   39
#define TFT_BL    48

// microSD (TF) card over SD_MMC (4-bit) - a separate peripheral from the
// LCD's SPI bus, so no bus-sharing is needed between the two.
#define SD_CLK             14
#define SD_CMD             15
#define SD_D0               16
#define SD_D1               18
#define SD_D2               17
#define SD_D3               21
#define SD_MOUNT_POINT      "/sdcard"

// While a file is being uploaded over WiFi, flush it to the card every this
// many bytes so a concurrent USB host reading raw sectors (see usb_msc.cpp)
// sees the growing size promptly instead of only once the upload finishes.
#define USB_MSC_FLUSH_BYTES ((uint32_t)(1024UL * 1024UL))

// How often (in bytes written) the WiFi-upload progress bar is refreshed.
// Bigger than a single chunk on purpose: keeps the LCD bus re-acquire/release
// pair (see SDSTOR_writeChunk in sd_storage.cpp) infrequent.
#define PROGRESS_UPDATE_BYTES ((uint32_t)(64UL * 1024UL))

// USB MSC writes are raw sectors with no notion of a file's total size, so
// the progress bar during a USB copy is a rough activity indicator, not an
// accurate percentage: it fills as if every burst of writes were this many
// bytes, and is considered "done" (bar removed) after this many ms without
// another sector write.
#define MSC_PROGRESS_ASSUMED_BYTES ((uint32_t)(8UL * 1024UL * 1024UL))
#define MSC_PROGRESS_IDLE_MS 400

// FAT32 caps a single file at 4GiB-1. Logical files bigger than this are
// transparently split into hidden numbered part files plus a manifest (see
// sd_storage.cpp); this cap is kept comfortably below the hard limit.
#define SD_PART_MAX_BYTES ((uint64_t)4000000000ULL)

// Text-file preview in the web UI reads at most this many bytes from the
// card, so previewing a large log/text file can't tie up the SD bus or the
// single-threaded web server for long.
#define PREVIEW_MAX_BYTES ((uint32_t)8192)

#endif
