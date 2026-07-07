/*
 *  Author: Rada Berar
 *  Email: ujagaga@gmail.com
 *
 *  Simplified WiFi connection module for ESP32:
 *  - AP always on
 *  - STA connects if saved
 *  - Automatic reconnection handled by ESP32 core
 *  - SD card stores SSID and password (see WIFI_CFG_PATH)
 */

#include <WiFi.h>
#include <SD.h>
#include "config.h"
#include "lcd_display.h"

// -----------------------------------------------------------------------------
// Local variables
// -----------------------------------------------------------------------------
static char myApName[32] = {0};         // AP name
static String st_ssid = "";             // Saved SSID
static String st_pass = "";             // Saved password
static IPAddress stationIP;
static IPAddress apIP(192, 168, 4, 1);
static bool stationConnectedOnce = false; // mark first successful STA connect
static unsigned long apIdleSince = 0;     // millis the AP last had zero clients
static bool apDisabled = false;           // AP turned off to save power

// -----------------------------------------------------------------------------
// Getters
// -----------------------------------------------------------------------------
char* WIFIC_getDeviceName(void) {
    return myApName;
}

String WIFIC_getApIp(void) {
    return apIP.toString();
}

String WIFIC_getStSSID(void) {
    return String(st_ssid);
}

String WIFIC_getStPass(void) {
    return String(st_pass);
}

// -----------------------------------------------------------------------------
// SD card helpers (WIFI_CFG_PATH: line 1 = ssid, line 2 = password)
// -----------------------------------------------------------------------------
static void saveCredsToSD(void) {
    LCD_busRelease();
    if (SD.exists(WIFI_CFG_PATH)) {
        SD.remove(WIFI_CFG_PATH);
    }
    File f = SD.open(WIFI_CFG_PATH, FILE_WRITE);
    if (f) {
        f.println(st_ssid);
        f.println(st_pass);
        f.close();
    }
    LCD_busAcquire();
}

void WIFIC_setStSSID(String new_ssid) {
    st_ssid = new_ssid;
    saveCredsToSD();
}

void WIFIC_setStPass(String new_pass) {
    st_pass = new_pass;
    saveCredsToSD();
}

// -----------------------------------------------------------------------------
// Wi-Fi AP mode
// -----------------------------------------------------------------------------
static void APMode(void) {
    Serial.println("\nStarting AP");

    WiFi.mode(WIFI_AP_STA);          // Ensure AP+STA mode
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);

    String mac = WiFi.macAddress();
    mac.replace(":", "");
    String apName = String(AP_NAME_PREFIX) + mac;
    apName.toCharArray(myApName, 32);

    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(myApName, AP_PASS);

    Serial.printf("AP active: %s, IP: %s\n", myApName, apIP.toString().c_str());
}

// -----------------------------------------------------------------------------
// Wi-Fi STA mode
// -----------------------------------------------------------------------------
void WIFIC_stationMode(void) {
    if (st_ssid.length() == 0) {
        Serial.println("No saved WiFi credentials.");
        return;
    }

    Serial.printf("Connecting STA [%s]...\n", st_ssid.c_str());
    WiFi.begin(st_ssid.c_str(), st_pass.c_str());
}

// -----------------------------------------------------------------------------
// Wi-Fi event callbacks
// -----------------------------------------------------------------------------
void WIFIC_setupCallbacks(void) {
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        stationIP = WiFi.localIP();
        Serial.printf("\n\nConnected, IP: %s\n", stationIP.toString().c_str());

        stationConnectedOnce = true;
        apIdleSince = millis();          // start the AP power-down countdown

        // If the AP was powered down earlier, the station must have dropped and
        // recovered; AP is re-enabled by the disconnect handler, nothing to do.
    }, ARDUINO_EVENT_WIFI_STA_GOT_IP);

    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        Serial.println("STA disconnected, will auto-reconnect.");
        if (apDisabled) {
            Serial.println("Re-enabling AP (station lost).");
            APMode();                    // bring the AP back so the device stays reachable
            apDisabled = false;
        }
    }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
}

// -----------------------------------------------------------------------------
// Initialize Wi-Fi module
// -----------------------------------------------------------------------------
void WIFIC_init(void) {
    // Load saved credentials from SD (requires SD.begin() already done, e.g.
    // via SDSTOR_init(), and the LCD already initialized for the shared bus).
    LCD_busRelease();
    File f = SD.open(WIFI_CFG_PATH);
    if (f) {
        st_ssid = f.readStringUntil('\n');
        st_ssid.trim();
        st_pass = f.readStringUntil('\n');
        st_pass.trim();
        f.close();
    }
    LCD_busAcquire();

    // Setup AP and STA
    APMode();
    WIFIC_setupCallbacks();
    WIFIC_stationMode();
}

// -----------------------------------------------------------------------------
// Periodic processing: drop the AP once the station is up and no AP clients
// have been connected for AP_AUTO_OFF_MS (saves power in station-only mode).
// -----------------------------------------------------------------------------
void WIFIC_process(void) {
    if (apDisabled || !stationConnectedOnce || WiFi.status() != WL_CONNECTED) {
        return;
    }

    if (WiFi.softAPgetStationNum() > 0) {
        apIdleSince = millis();          // clients present, keep the AP up
        return;
    }

    if ((millis() - apIdleSince) > AP_AUTO_OFF_MS) {
        Serial.println("No AP clients; disabling AP to save power.");
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
        apDisabled = true;
    }
}

// -----------------------------------------------------------------------------
// Return list of scanned APs
// -----------------------------------------------------------------------------
#ifdef USE_WEBSOCKETS
// WebSocket UI: the scan runs inside the WS request handler, so a simple
// blocking scan is fine (the result is sent back in the same call).
String WIFIC_getApList(void) {
    String result = "";
    int n = WiFi.scanNetworks();
    if (n > 0) {
        result = WiFi.SSID(0);
        for (int i = 1; i < n; ++i) {
            result += "|" + WiFi.SSID(i);
        }
    }
    return result;
}
#else
// No-WebSocket UI: kick off a non-blocking scan, read results later so the
// HTTP handler never blocks. The page polls WIFIC_getApList() via /aplist.
void WIFIC_startScan(void) {
    WiFi.scanDelete();
    WiFi.scanNetworks(true);             // async
}

// Return the completed scan as a '|'-joined list, or "" if it is still running.
String WIFIC_getApList(void) {
    int n = WiFi.scanComplete();
    if (n <= 0) {                        // -1 running, -2 failed, 0 none
        return "";
    }
    String result = WiFi.SSID(0);
    for (int i = 1; i < n; ++i) {
        result += "|" + WiFi.SSID(i);
    }
    WiFi.scanDelete();                   // free the result buffer
    return result;
}
#endif

String WIFIC_getStationIp()
{
    if (WiFi.status() == WL_CONNECTED){
        return WiFi.localIP().toString();
    }
    return "";
}

bool WIFIC_stationConnected()
{
    return (WiFi.status() == WL_CONNECTED);
}

