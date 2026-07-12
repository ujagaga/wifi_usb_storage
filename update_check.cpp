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
static uint32_t connectedSinceMs = 0;  // 0 = not connected (or not seen yet this connection)
static bool updateAvailable = false;
static String latestVersion = "";
static String latestMd5 = "";
static String lastErr = "";
static String stagedFilename = "";  // name actually used for the last download, so apply reads back the same file

String UPDATE_CHECK_lastError(void){
  return lastErr;
}

// No cert pinning, so a MITM could lie about whether an update exists or
// swap in a different MD5/version - but not silently corrupt a legitimate
// update, since SDSTOR_applyFirmwareUpdate() verifies the flashed content's
// MD5 against latestMd5 before ever activating the new partition.
static void doCheck(void){
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;
  if(https.begin(client, UPDATE_VERSION_URL)){
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

  WiFiClientSecure client2;
  client2.setInsecure();
  HTTPClient https2;
  if(https2.begin(client2, UPDATE_MD5_URL)){
    int code = https2.GET();
    if(code == HTTP_CODE_OK){
      String remote = https2.getString();
      remote.trim();
      remote.toLowerCase();
      if(remote.length() == 32){
        latestMd5 = remote;
      }
    }
    https2.end();
  }
}

void UPDATE_CHECK_init(void){
  lastCheckMs = millis();
}

void UPDATE_CHECK_process(void){
  if(!WIFIC_stationConnected()){
    connectedSinceMs = 0;   // reconnecting later waits the full delay again
    return;
  }
  uint32_t now = millis();
  if(connectedSinceMs == 0){
    connectedSinceMs = now;
  }
  if(!everChecked){
    if((now - connectedSinceMs) < UPDATE_FIRST_CHECK_DELAY_MS){
      return;
    }
    everChecked = true;
    lastCheckMs = now;
    doCheck();
  }else if((now - lastCheckMs) >= UPDATE_CHECK_INTERVAL_MS){
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

void UPDATE_CHECK_forceCheck(void){
  lastCheckMs = millis();
  everChecked = true;
  doCheck();
}

// Streams the new firmware image straight to SD, named
// "firmware_update_<version>.bin" at root, via the same chunked-write path
// uploads use. Purely a file copy - nothing here touches flash, so a
// failed/interrupted download just leaves a partial (or no) file on the
// card, same as any other aborted upload.
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

  // The periodic check (once WiFi connects, then every 24h) may not have
  // run yet this soon after boot - make sure latestVersion/latestMd5 are
  // populated before staging/naming the download, rather than racing it.
  if(latestVersion.length() == 0 || latestMd5.length() == 0){
    doCheck();
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

  stagedFilename = latestVersion.length() > 0
    ? ("firmware_update_" + latestVersion + ".bin")
    : String(UPDATE_FIRMWARE_FILENAME);
  if(!SDSTOR_writeBegin("", stagedFilename)){
    lastErr = "Cannot stage firmware file: " + SDSTOR_lastError();
    https.end();
    return false;
  }

  NetworkClient* stream = https.getStreamPtr();
  uint8_t buf[1024];
  int written = 0;
  bool ok = true;
  // Keep draining as long as the connection is open OR there's still
  // buffered data to read - checking only https.connected() can be false
  // right after the server closes the socket while decrypted data for the
  // tail of the response is still sitting in the TLS receive buffer,
  // silently truncating the download.
  while(total < 0 || written < total){
    size_t avail = stream->available();
    if(avail == 0){
      if(!https.connected()){
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

bool UPDATE_CHECK_applyFromSD(void){
  lastErr = "";
  if(stagedFilename.length() == 0){
    lastErr = "No firmware downloaded yet";
    return false;
  }
  String path = "/" + stagedFilename;
  return SDSTOR_applyFirmwareUpdate(path, latestMd5, lastErr);
}
