#ifndef STORAGE_MODE_H
#define STORAGE_MODE_H

#include <Arduino.h>

// Which side may write to the SD card - the other is always read-only (WiFi
// read/list/download, or USB read-only mass storage, stay available either
// way; only mutation is gated). Defaults to WiFi as write master, USB
// read-only, if no setting is found in WIFI_CFG_PATH.
extern bool STORAGE_MODE_isWifiWriteMaster(void);
// Switches the write master. Also flips the USB mass storage write-protect
// flag; caller (see http_server.cpp's toggle handler) is responsible for
// persisting via WIFIC_persistConfig(), same as theme.h's THEME_toggle().
extern void STORAGE_MODE_setWifiWriteMaster(bool wifiIsMaster);

// Persistence hook into wifi_connection.cpp's shared WIFI_CFG_PATH content,
// same pattern as theme.h.
extern void STORAGE_MODE_appendConfigLines(String& content);
extern bool STORAGE_MODE_tryParseConfigLine(const String& key, const String& val);

#endif
