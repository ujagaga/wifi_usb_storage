/*
 *  Checks a GitHub-hosted version.txt over HTTPS to see if a newer firmware
 *  build is available, and can stage the new firmware image on the SD card.
 *  Actually flashing the staged image (Update.h) is a later step.
 */

#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "config.h"
#include "wifi_connection.h"
#include "sd_storage.h"
#include "update_check.h"

static bool everChecked = false;
static uint32_t lastCheckMs = 0;
static bool updateAvailable = false;
static String latestVersion = "";
static String lastErr = "";

String UPDATE_CHECK_lastError(void){
  return lastErr;
}

// No cert pinning - this only compares a version string, so a MITM could at
// worst lie about whether an update exists, not push code. Revisit once this
// starts fetching/flashing an actual firmware image.
static void doCheck(void){
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;
  if(!https.begin(client, UPDATE_VERSION_URL)){
    return;
  }
  int code = https.GET();
  if(code == HTTP_CODE_OK){
    String remote = https.getString();
    remote.trim();
    if(remote.length() > 0){
      latestVersion = remote;
      updateAvailable = (remote != VERSION);
    }
  }
  https.end();
}

void UPDATE_CHECK_init(void){
  lastCheckMs = millis();
}

void UPDATE_CHECK_process(void){
  if(!WIFIC_stationConnected()){
    return;
  }
  uint32_t now = millis();
  if(!everChecked || (now - lastCheckMs) >= UPDATE_CHECK_INTERVAL_MS){
    everChecked = true;
    lastCheckMs = now;
    doCheck();
  }
}

bool UPDATE_CHECK_isAvailable(void){
  return updateAvailable;
}

String UPDATE_CHECK_getLatestVersion(void){
  return latestVersion;
}

// Streams the new firmware image straight to SD (UPDATE_FIRMWARE_FILENAME at
// root) via the same chunked-write path uploads use. Purely a file copy -
// nothing here touches flash, so a failed/interrupted download just leaves a
// partial (or no) file on the card, same as any other aborted upload.
bool UPDATE_CHECK_downloadToSD(void){
  lastErr = "";
  if(!WIFIC_stationConnected()){
    lastErr = "Not connected to WiFi";
    return false;
  }
  if(!SDSTOR_isReady()){
    lastErr = "SD card not ready";
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;
  if(!https.begin(client, UPDATE_FIRMWARE_URL)){
    lastErr = "Could not start HTTPS request";
    return false;
  }
  int code = https.GET();
  if(code != HTTP_CODE_OK){
    lastErr = "HTTP error " + String(code);
    https.end();
    return false;
  }
  int total = https.getSize();   // -1 if unknown (chunked transfer)

  if(!SDSTOR_writeBegin("", UPDATE_FIRMWARE_FILENAME)){
    lastErr = "Cannot stage firmware file: " + SDSTOR_lastError();
    https.end();
    return false;
  }

  NetworkClient* stream = https.getStreamPtr();
  uint8_t buf[1024];
  int written = 0;
  bool ok = true;
  while(https.connected() && (total < 0 || written < total)){
    size_t avail = stream->available();
    if(avail == 0){
      if(!stream->connected()){
        break;
      }
      delay(1);
      continue;
    }
    size_t want = (avail > sizeof(buf)) ? sizeof(buf) : avail;
    int got = stream->readBytes(buf, want);
    if(got <= 0){
      break;
    }
    if(!SDSTOR_writeChunk(buf, got)){
      ok = false;
      lastErr = "SD write failed: " + SDSTOR_lastError();
      break;
    }
    written += got;
  }

  if(ok && total >= 0 && written != total){
    ok = false;
    lastErr = "Incomplete download (" + String(written) + " of " + String(total) + " bytes)";
  }

  if(ok){
    SDSTOR_writeEnd();
  }else{
    SDSTOR_writeAbort();
  }
  https.end();
  return ok;
}
