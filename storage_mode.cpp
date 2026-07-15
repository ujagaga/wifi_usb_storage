#include "storage_mode.h"
#include "usb_msc.h"

static bool wifiIsWriteMaster = true;  // default: WiFi writes, USB is read-only

bool STORAGE_MODE_isWifiWriteMaster(void){
    return wifiIsWriteMaster;
}

void STORAGE_MODE_setWifiWriteMaster(bool wifiIsMaster){
    wifiIsWriteMaster = wifiIsMaster;
    USB_MSC_setWritable(!wifiIsMaster);
}

void STORAGE_MODE_appendConfigLines(String& content){
    content += "WRITE_MASTER=" + String(wifiIsWriteMaster ? "WIFI" : "USB") + "\n";
}

bool STORAGE_MODE_tryParseConfigLine(const String& key, const String& val){
    if(key != "WRITE_MASTER"){
        return false;
    }
    // Routed through the setter (not just the bool) so the USB write-protect
    // flag stays in sync on every load, including the hot-plug reload path
    // in wifi_connection.cpp - not just the very first boot.
    if(val == "USB"){
        STORAGE_MODE_setWifiWriteMaster(false);
    }else if(val == "WIFI"){
        STORAGE_MODE_setWifiWriteMaster(true);
    }
    return true;
}
