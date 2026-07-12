/*
 *  Checks a GitHub-hosted version.txt over HTTPS to see if a newer firmware
 *  build is available. Only compares a version string for now - fetching
 *  and flashing the update itself is a later step.
 */

#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "config.h"
#include "wifi_connection.h"
#include "update_check.h"

static bool everChecked = false;
static uint32_t lastCheckMs = 0;
static bool updateAvailable = false;
static String latestVersion = "";

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
