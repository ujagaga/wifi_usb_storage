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

#line 15 "/home/rada/Projects/wifi_usb_storage/wifi_usb_storage.ino"
void MAIN_setStatusMsg(String msg);
#line 19 "/home/rada/Projects/wifi_usb_storage/wifi_usb_storage.ino"
String MAIN_getStatusMsg(void);
#line 23 "/home/rada/Projects/wifi_usb_storage/wifi_usb_storage.ino"
static void display_ap_info(void);
#line 44 "/home/rada/Projects/wifi_usb_storage/wifi_usb_storage.ino"
static void display_station_info(String ip);
#line 59 "/home/rada/Projects/wifi_usb_storage/wifi_usb_storage.ino"
void setup(void);
#line 69 "/home/rada/Projects/wifi_usb_storage/wifi_usb_storage.ino"
void loop(void);
#line 15 "/home/rada/Projects/wifi_usb_storage/wifi_usb_storage.ino"
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
  }
}

