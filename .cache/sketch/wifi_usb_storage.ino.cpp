#include <Arduino.h>
#line 1 "/home/rada/Projects/wifi_usb_storage/wifi_usb_storage.ino"
#include "wifi_connection.h"
#include "config.h"
#include "http_server.h"
#include "lcd_display.h"
#include "sd_storage.h"

enum Operation {
  ShowAp,
  ShowStation
};

static Operation state = ShowAp;
static String statusMessage = "";         /* This is set and requested from other modules. */

#define RSSI_REFRESH_MS 60000
static int rssiCursorX = 0, rssiCursorY = 0;
static String lastRssiStr = "";
static uint32_t lastRssiMs = 0;

#line 20 "/home/rada/Projects/wifi_usb_storage/wifi_usb_storage.ino"
void MAIN_setStatusMsg(String msg);
#line 24 "/home/rada/Projects/wifi_usb_storage/wifi_usb_storage.ino"
String MAIN_getStatusMsg(void);
#line 28 "/home/rada/Projects/wifi_usb_storage/wifi_usb_storage.ino"
static void display_ap_info(void);
#line 49 "/home/rada/Projects/wifi_usb_storage/wifi_usb_storage.ino"
static void display_station_info(String ip);
#line 75 "/home/rada/Projects/wifi_usb_storage/wifi_usb_storage.ino"
static void refresh_station_rssi(void);
#line 85 "/home/rada/Projects/wifi_usb_storage/wifi_usb_storage.ino"
void setup(void);
#line 95 "/home/rada/Projects/wifi_usb_storage/wifi_usb_storage.ino"
void loop(void);
#line 20 "/home/rada/Projects/wifi_usb_storage/wifi_usb_storage.ino"
void MAIN_setStatusMsg(String msg){
  statusMessage = msg;
}

String MAIN_getStatusMsg(void){
  return statusMessage;
}

static void display_ap_info(void)
{
  LCD_clear();
  LCD_setFont(Font9pt);
  LCD_color(C_YELLOW);
  LCD_write("\nAP SSID:\n");
  LCD_color(C_WHITE);
  LCD_write(String(WIFIC_getDeviceName()));
  LCD_color(C_YELLOW);
  LCD_write("\n\nPASS: ");
  LCD_color(C_WHITE);
  LCD_setFont(Font12pt);
  LCD_write(AP_PASS);
  LCD_color(C_YELLOW);
  LCD_setFont(Font9pt);
  LCD_write("\n\nIP: ");
  LCD_color(C_WHITE);
  LCD_setFont(Font12pt);
  LCD_write(WIFIC_getApIp());
}

static void display_station_info(String ip)
{
  LCD_clear();
  LCD_setFont(Font9pt);
  LCD_color(C_YELLOW);
  LCD_write("\n\nConnected SSID:\n");
  LCD_color(C_WHITE);
  LCD_write(WIFIC_getStSSID());
  LCD_color(C_YELLOW);
  LCD_write("\n\nIP: ");
  LCD_color(C_WHITE);
  LCD_setFont(Font12pt);
  LCD_write(ip);
  LCD_setFont(Font9pt);
  LCD_color(C_YELLOW);
  LCD_write("\n\nSignal: ");
  LCD_color(C_WHITE);
  rssiCursorX = LCD_getX();
  rssiCursorY = LCD_getY();
  lastRssiStr = String(WIFIC_getRssi()) + " dBm";
  LCD_write(lastRssiStr);
  lastRssiMs = millis();
}

// Overwrites just the signal-strength value in place (no full-screen redraw),
// called periodically while the station screen is showing.
static void refresh_station_rssi(void)
{
  LCD_setCursor(rssiCursorX, rssiCursorY);
  LCD_clearStringArea(lastRssiStr);
  LCD_setCursor(rssiCursorX, rssiCursorY);
  LCD_color(C_WHITE);
  lastRssiStr = String(WIFIC_getRssi()) + " dBm";
  LCD_write(lastRssiStr);
}

void setup(void)
{
  delay(100);
  LCD_init();
  SDSTOR_init();
  WIFIC_init();
  HTTP_SERVER_init();
  display_ap_info();
}

void loop(void){
  HTTP_SERVER_process();
  LCD_process();
  WIFIC_process();

  // The AP screen stays up (SSID/pass/IP) until the station interface actually
  // connects, then it's replaced by the station SSID/IP. Without saved
  // credentials (SD or RAM-only, set via the web UI), station never
  // connects, so the AP screen just stays.
  if(state == ShowAp){
    String stationIp = WIFIC_getStationIp();
    if(stationIp.length() > 1){
      display_station_info(stationIp);
      state = ShowStation;
    }
  }else if(state == ShowStation){
    if((millis() - lastRssiMs) >= RSSI_REFRESH_MS){
      refresh_station_rssi();
      lastRssiMs = millis();
    }
  }
}

