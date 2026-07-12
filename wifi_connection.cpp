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
#include <SD.h>
#include "config.h"
#include "lcd_display.h"
#include "sd_storage.h"

// -----------------------------------------------------------------------------
// Local variables
// -----------------------------------------------------------------------------
static char myApName[24] = {0};         // AP name
static String st_ssid = "";             // Saved SSID
static String st_pass = "";             // Saved password
static uint8_t st_brightness = 80;      // Saved screen illumination percent (matches the old fixed boot default)

// UI theme colors (hex "#rrggbb"), overridable via KEY=VALUE lines in WIFI_CFG_PATH.
static String st_colorBg          = "#11151c";   // page background
static String st_colorCardBg      = "#1b212b";   // card/panel background
static String st_colorBorder      = "#232b37";   // card/table borders
static String st_colorText        = "#e6e9ef";   // main body text
static String st_colorMuted       = "#8b93a3";   // secondary/muted text
static String st_colorAccent      = "#ff5a3c";   // primary button / active nav background
static String st_colorAccentHover = "#ff6f55";   // primary button hover
static String st_colorLink        = "#ff7a5c";   // hyperlink color
static String st_colorBtn         = "#2a3340";   // secondary button background
static String st_colorBtnHover    = "#37424f";   // secondary button hover

struct ColorEntry { const char* key; const char* cssVar; String* value; };
static ColorEntry st_colorEntries[] = {
    { "COLOR_BG",           "bg",           &st_colorBg },
    { "COLOR_CARD_BG",      "card-bg",      &st_colorCardBg },
    { "COLOR_BORDER",       "border",       &st_colorBorder },
    { "COLOR_TEXT",         "text",         &st_colorText },
    { "COLOR_MUTED",        "muted",        &st_colorMuted },
    { "COLOR_ACCENT",       "accent",       &st_colorAccent },
    { "COLOR_ACCENT_HOVER", "accent-hover", &st_colorAccentHover },
    { "COLOR_LINK",         "link",         &st_colorLink },
    { "COLOR_BTN",          "btn",          &st_colorBtn },
    { "COLOR_BTN_HOVER",    "btn-hover",    &st_colorBtnHover },
};
static const int NUM_COLOR_ENTRIES = sizeof(st_colorEntries) / sizeof(st_colorEntries[0]);

// "dark" or "light" - which preset below is currently active (persisted as THEME=).
static String st_theme = "dark";

// Preset color values, in the same order as st_colorEntries above.
static const char* DARK_PRESET[]  = { "#11151c", "#1b212b", "#232b37", "#e6e9ef", "#8b93a3",
                                       "#ff5a3c", "#ff6f55", "#ff7a5c", "#2a3340", "#37424f" };
static const char* LIGHT_PRESET[] = { "#f4f5f7", "#ffffff", "#d8dce2", "#1b212b", "#6b7280",
                                       "#ff5a3c", "#ff7a5c", "#d9480f", "#e9ecef", "#d8dde3" };
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

String WIFIC_getThemeCSSVars(void) {
    String s;
    for (int i = 0; i < NUM_COLOR_ENTRIES; i++) {
        s += String("--") + st_colorEntries[i].cssVar + ":" + *st_colorEntries[i].value + ";";
    }
    return s;
}

static void saveCredsToSD(void);   // forward declaration; defined below

void WIFIC_toggleTheme(void) {
    bool toLight = (st_theme != "light");
    st_theme = toLight ? "light" : "dark";
    const char** preset = toLight ? LIGHT_PRESET : DARK_PRESET;
    for (int i = 0; i < NUM_COLOR_ENTRIES; i++) {
        *st_colorEntries[i].value = preset[i];
    }
    saveCredsToSD();
}

// -----------------------------------------------------------------------------
// SD card helpers (WIFI_CFG_PATH: line 1 = ssid, line 2 = password, line 3 =
// brightness percent). Credentials/brightness always live in RAM; the SD file
// is only a mirror that's written/read when a card happens to be present.
// -----------------------------------------------------------------------------
static void saveCredsToSD(void) {
    if (!SDSTOR_isReady()) {
        return;
    }
    LCD_busRelease();
    if (SD.exists(WIFI_CFG_PATH)) {
        SD.remove(WIFI_CFG_PATH);
    }
    File f = SD.open(WIFI_CFG_PATH, FILE_WRITE);
    if (f) {
        f.println(st_ssid);
        f.println(st_pass);
        f.println(st_brightness);
        f.println("THEME=" + st_theme);
        f.println("# --- UI theme colors (hex #rrggbb) ---");
        for (int i = 0; i < NUM_COLOR_ENTRIES; i++) {
            f.println(String(st_colorEntries[i].key) + "=" + *st_colorEntries[i].value);
        }
        f.close();
    }
    LCD_busAcquire();
}

// Loads RAM creds/brightness from the SD file if one exists; otherwise
// creates the file from whatever's currently in RAM. Returns true if loaded
// from an existing file (caller should then reconnect with the loaded creds).
static bool loadOrCreateCredsFromSD(void) {
    if (!SDSTOR_isReady()) {
        return false;
    }
    LCD_busRelease();
    File f = SD.open(WIFI_CFG_PATH);
    bool exists = (bool)f;
    if (exists) {
        st_ssid = f.readStringUntil('\n');
        st_ssid.trim();
        st_pass = f.readStringUntil('\n');
        st_pass.trim();
        String br = f.readStringUntil('\n');
        br.trim();
        if (br.length() > 0) {
            int v = br.toInt();
            if (v >= 0 && v <= 100) {
                st_brightness = (uint8_t)v;
            }
        }
        while (f.available()) {
            String line = f.readStringUntil('\n');
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
            if (key == "THEME") {
                if (val == "light" || val == "dark") {
                    st_theme = val;
                }
                continue;
            }
            if (val.length() != 7 || val[0] != '#') {
                continue;
            }
            for (int i = 0; i < NUM_COLOR_ENTRIES; i++) {
                if (key == st_colorEntries[i].key) {
                    *st_colorEntries[i].value = val;
                    break;
                }
            }
        }
        f.close();
    }
    LCD_busAcquire();

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

    String mac = WiFi.macAddress();
    mac.replace(":", "");
    String apName = String(AP_NAME_PREFIX) + mac;
    apName.toCharArray(myApName, sizeof(myApName));

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

