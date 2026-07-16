/*
 *  Author: Rada Berar
 *  Email: ujagaga@gmail.com
 *
 *  Simplified WiFi connection module for ESP32:
 *  - AP always on
 *  - STA connects if saved
 *  - Automatic reconnection handled by ESP32 core
 *  - SSID/password live in RAM, mirrored to SD (WIFI_CFG_PATH) if a card is present
 */

#include <WiFi.h>
#include <esp_mac.h>
#include <esp_wifi.h>
#include "config.h"
#include "lcd_display.h"
#include "sd_storage.h"
#include "theme.h"
#include "storage_mode.h"

// -----------------------------------------------------------------------------
// Local variables
// -----------------------------------------------------------------------------
static char myApName[26] = {0};         // AP name: AP_NAME_PREFIX + 12 hex chars + terminator
static String st_ssid = "";             // Saved SSID
static String st_pass = "";             // Saved password
static uint8_t st_brightness = 80;      // Saved screen illumination percent (matches the old fixed boot default)

static IPAddress stationIP;
static IPAddress apIP(192, 168, 4, 1);
static bool stationConnectedOnce = false; // mark first successful STA connect
static unsigned long apIdleSince = 0;     // millis the AP last had zero clients
static bool apDisabled = false;           // AP turned off to save power
static bool sdWasReady = false;           // edge-detects SD card hot-insert

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

uint8_t WIFIC_getBrightness(void) {
    return st_brightness;
}

static void saveCredsToSD(void);   // forward declaration; defined below

void WIFIC_persistConfig(void) {
    saveCredsToSD();
}

// -----------------------------------------------------------------------------
// SD card helpers (WIFI_CFG_PATH: line 1 = ssid, line 2 = password, line 3 =
// brightness percent). Credentials/brightness always live in RAM; the SD file
// is only a mirror that's written/read when a card happens to be present.
// -----------------------------------------------------------------------------
// Extracts the next '\n'-terminated line from content starting at pos,
// advancing pos past it. Once exhausted, returns "" and leaves pos > length.
static String nextLine(const String& content, int& pos){
    if (pos > (int)content.length()) {
        return "";
    }
    int nl = content.indexOf('\n', pos);
    String line;
    if (nl < 0) {
        line = content.substring(pos);
        pos = content.length() + 1;
    } else {
        line = content.substring(pos, nl);
        pos = nl + 1;
    }
    line.replace("\r", "");
    return line;
}

static void saveCredsToSD(void) {
    if (!SDSTOR_isReady()) {
        return;
    }
    String content = st_ssid + "\n" + st_pass + "\n" + String(st_brightness) + "\n";
    THEME_appendConfigLines(content);
    STORAGE_MODE_appendConfigLines(content);
    LCD_appendConfigLines(content);
    SDSTOR_writeTextFile(WIFI_CFG_PATH, content);
}

// Loads RAM creds/brightness from the SD file if one exists; otherwise
// creates the file from whatever's currently in RAM. Returns true if loaded
// from an existing file (caller should then reconnect with the loaded creds).
static bool loadOrCreateCredsFromSD(void) {
    if (!SDSTOR_isReady()) {
        return false;
    }
    String content;
    bool exists = SDSTOR_readTextFile(WIFI_CFG_PATH, content);
    if (exists) {
        int pos = 0;
        st_ssid = nextLine(content, pos);
        st_ssid.trim();
        st_pass = nextLine(content, pos);
        st_pass.trim();
        String br = nextLine(content, pos);
        br.trim();
        if (br.length() > 0) {
            int v = br.toInt();
            if (v >= 0 && v <= 100) {
                st_brightness = (uint8_t)v;
            }
        }
        while (pos <= (int)content.length()) {
            String line = nextLine(content, pos);
            line.trim();
            if (line.length() == 0 || line[0] == '#') {
                continue;
            }
            int eq = line.indexOf('=');
            if (eq <= 0) {
                continue;
            }
            String key = line.substring(0, eq);
            String val = line.substring(eq + 1);
            key.trim();
            val.trim();
            if(!THEME_tryParseConfigLine(key, val)){
                if(!STORAGE_MODE_tryParseConfigLine(key, val)){
                    LCD_tryParseConfigLine(key, val);
                }
            }
        }
    }

    if (!exists) {
        saveCredsToSD();
    }
    return exists;
}

void WIFIC_setStSSID(String new_ssid) {
    st_ssid = new_ssid;
    saveCredsToSD();
}

void WIFIC_setStPass(String new_pass) {
    st_pass = new_pass;
    saveCredsToSD();
}

void WIFIC_setBrightness(uint8_t percent) {
    if (percent > 100) {
        percent = 100;
    }
    st_brightness = percent;
    LCD_setBacklight(st_brightness);
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
    // Modem power-save is on by default and duty-cycles the radio between
    // packets, adding per-round-trip latency that tanks upload throughput.
    WiFi.setSleep(false);

    // WiFi.macAddress() can read back all-zero on the S3 if called this
    // early (before the STA netif is fully up); the factory-programmed base
    // MAC from eFuse is available immediately regardless of WiFi driver
    // state, so read it directly instead.
    uint8_t rawMac[6];
    esp_efuse_mac_get_default(rawMac);
    char macHex[13];
    snprintf(macHex, sizeof(macHex), "%02X%02X%02X%02X%02X%02X",
             rawMac[0], rawMac[1], rawMac[2], rawMac[3], rawMac[4], rawMac[5]);
    String apName = String(AP_NAME_PREFIX) + macHex;
    apName.toCharArray(myApName, sizeof(myApName));

    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(myApName, AP_PASS);
    // 40MHz roughly doubles the theoretical PHY-rate ceiling vs the 20MHz
    // default, independent of the TCP-window/buffer tuning tried elsewhere.
    // Must be called after softAP() brings the AP interface up.
    esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT40);

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
    // Load saved credentials from SD if a card happens to be present already
    // (no-op otherwise - creds simply stay RAM-only until a card shows up).
    loadOrCreateCredsFromSD();
    LCD_setBacklight(st_brightness);
    sdWasReady = SDSTOR_isReady();

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
    // SD card was just inserted: pick up creds saved on it, if any.
    bool sdReady = SDSTOR_isReady();
    if (sdReady && !sdWasReady) {
        if (loadOrCreateCredsFromSD()) {
            WIFIC_stationMode();
        }
        LCD_setBacklight(st_brightness);
    }
    sdWasReady = sdReady;

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

int WIFIC_getRssi(void)
{
    return WiFi.RSSI();
}

