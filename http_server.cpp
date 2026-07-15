/*
 *  Author: Rada Berar
 *  email: ujagaga@gmail.com
 *
 *  HTTP server which generates the web browser pages.
 */

#include <WebServer.h>
#include "wifi_connection.h"
#include "config.h"
#include "wifi_usb_storage.h"
#include "sd_storage.h"
#include "http_ui.h"
#include "update_check.h"
#include "theme.h"
#include "storage_mode.h"
#include "lcd_display.h"

// --- Web server object ---
WebServer* webServer = nullptr;

// --- SD-present guard ---
// Without a usable SD card there is nowhere to store files, so every
// SD-dependent endpoint just reports the problem instead of doing its job.
static void showSDMissing(void){
  if(SDSTOR_isFaulty()){
    webServer->send(200, "text/plain", "SD card detected, but its filesystem could not be read. It may be corrupted or unformatted.");
  }else{
    webServer->send(200, "text/plain", "No Micro SD card detected. Please insert!");
  }
}

static bool requireSD(void){
  if(!SDSTOR_isReady()){
    showSDMissing();
    return false;
  }
  return true;
}

// --- Page handlers ---
// HTML_BEGIN plus a ":root" style block exposing the current UI theme colors
// as CSS custom properties, so every page picks up config.txt's overrides.
static String pageStart(void) {
  String s = FPSTR(HTML_BEGIN);
  s += "<style>:root{" + THEME_getCSSVars() + "}</style>";
  return s;
}

void showStartPage() {
  String response = pageStart();
  response += FPSTR(INDEX_HTML_0);
  response += FPSTR(NAV_HTML);
  response += "<p class='ip' id='status'>Station IP: " + WIFIC_getStationIp() + " - v" + VERSION + "</p>";
  if(SDSTOR_isReady()){
    response += FPSTR(INDEX_HTML_1);
  }else if(SDSTOR_isFaulty()){
    response += FPSTR(SD_FAULTY_HTML);
  }else{
    response += FPSTR(SD_MISSING_HTML);
  }
  response += FPSTR(SD_POLL_HTML);
  response += FPSTR(HTML_END);
  webServer->send(200, "text/html", response);
}


static void showApiPage(void){
  String response = pageStart();
  response += FPSTR(API_HTML_0);
  response += FPSTR(NAV_HTML);
  response += FPSTR(API_HTML_1);
  response += FPSTR(HTML_END);
  webServer->send(200, "text/html", response);
}

static void showNotFound(void){
  webServer->send(404, "text/html; charset=iso-8859-1","<html><head> <title>404 Not Found</title></head><body><h1>Not Found</h1></body></html>");
}

static void showStatusPage(bool goToHome = false) {
  String response = pageStart();
  response += "<h1>Connection Status</h1><p>";
  response += MAIN_getStatusMsg() + "</p>";
  if(goToHome){
    response += FPSTR(REDIRECT_HTML);
  }
  response += FPSTR(HTML_END);
  webServer->send(200, "text/html", response);
}

static void selectAP(void) {
  WIFIC_startScan();   // kick off the scan; JS fetches /aplist ~10 s later
  String response = pageStart();
  response += FPSTR(APLIST_HTML_0);
  response += FPSTR(NAV_HTML);
  response += FPSTR(APLIST_HTML_1);
  response += FPSTR(APLIST_HTML_2);
  response += String(WIFIC_getBrightness());
  response += FPSTR(APLIST_HTML_3);
  response += FPSTR(HTML_END);
  webServer->send(200, "text/html", response);
}

static void setBrightness(void){
  int val = webServer->arg("value").toInt();
  if(val < 0){ val = 0; }
  if(val > 100){ val = 100; }
  WIFIC_setBrightness((uint8_t)val);
  webServer->send(200, "text/plain", "OK");
}

static void toggleTheme(void){
  THEME_toggle();
  WIFIC_persistConfig();
  webServer->send(200, "text/plain", "OK");
}

static void rotateScreen180(void){
  LCD_rotate180();
  webServer->send(200, "text/plain", "OK");
}

static void writeMasterStatus(void){
  webServer->send(200, "text/plain", STORAGE_MODE_isWifiWriteMaster() ? "WiFi" : "USB");
}

static void toggleWriteMaster(void){
  STORAGE_MODE_setWifiWriteMaster(!STORAGE_MODE_isWifiWriteMaster());
  WIFIC_persistConfig();
  webServer->send(200, "text/plain", STORAGE_MODE_isWifiWriteMaster() ? "WiFi" : "USB");
}

static void saveWiFi(void){
  String ssid = webServer->arg("s");
  String pass = webServer->arg("p");

  if((ssid.length() > 63) || (pass.length() > 63)){
      MAIN_setStatusMsg("Sorry, this module can only remember SSID and a PASSWORD up to 63 bytes long.");
      showStatusPage(true);
      return;
  }

  String st_ssid = WIFIC_getStSSID();
  String st_pass = WIFIC_getStPass();

  if(st_ssid.equals(ssid) && st_pass.equals(pass)){
      MAIN_setStatusMsg("All parameters are already set as requested.");
      showStatusPage(true);
      return;
  }

  WIFIC_setStSSID(ssid);
  WIFIC_setStPass(pass);

  String http_statusMessage;

  if(ssid.length() > 3){
    http_statusMessage = "Saving settings and connecting to SSID: " + ssid;
  }else{
    http_statusMessage = "No SSID selected...";
  }

  MAIN_setStatusMsg(http_statusMessage);
  showStatusPage();

  WIFIC_stationMode();
}

static void fileList(void){
  if(!requireSD()){
    return;
  }
  webServer->send(200, "text/plain", SDSTOR_list(webServer->arg("dir")));
}

static void downloadFile(void){
  if(!requireSD()){
    return;
  }
  String name = webServer->arg("name");
  if(name.length() == 0 || !SDSTOR_sendRaw(webServer->arg("dir"), name, webServer)){
    webServer->send(404, "text/plain", "File not found");
  }
}

static void previewFile(void){
  if(!requireSD()){
    return;
  }
  String name = webServer->arg("name");
  if(name.length() == 0 || !SDSTOR_sendPreview(webServer->arg("dir"), name, PREVIEW_MAX_BYTES, webServer)){
    webServer->send(404, "text/plain", "File not found");
  }
}

static void deleteFile(void){
  if(!requireSD()){
    return;
  }
  String name = webServer->arg("name");
  if(name.length() == 0 || !SDSTOR_delete(webServer->arg("dir"), name)){
    webServer->send(400, "text/plain", "Delete failed");
    return;
  }
  webServer->send(200, "text/plain", "OK");
}

static void moveFile(void){
  if(!requireSD()){
    return;
  }
  String name = webServer->arg("name");
  if(name.length() == 0 || !SDSTOR_move(webServer->arg("dir"), name, webServer->arg("destDir"))){
    webServer->send(400, "text/plain", "Move failed");
    return;
  }
  webServer->send(200, "text/plain", "OK");
}

static void renameHandler(void){
  if(!requireSD()){
    return;
  }
  String name = webServer->arg("name");
  String newName = webServer->arg("newName");
  if(name.length() == 0 || newName.length() == 0 || !SDSTOR_rename(webServer->arg("dir"), name, newName)){
    webServer->send(400, "text/plain", "Rename failed");
    return;
  }
  webServer->send(200, "text/plain", "OK");
}

static void mkdirHandler(void){
  if(!requireSD()){
    return;
  }
  String name = webServer->arg("name");
  if(name.length() == 0 || !SDSTOR_mkdir(webServer->arg("dir"), name)){
    webServer->send(400, "text/plain", "Create folder failed");
    return;
  }
  webServer->send(200, "text/plain", "OK");
}

static bool uploadOk = false;

// Streams the raw upload body straight to the SD card, chunk by chunk. The
// client sends a non-multipart body (see uploadOne() in http_ui.h) so the
// WebServer library takes its "raw" fast path (plain client.readBytes()
// chunks) instead of its multipart parser, which reads the body one byte at
// a time and is far too slow for large files. Folder/filename travel as
// request headers (collected in HTTP_SERVER_init) since raw-mode requests
// don't get their query string parsed into webServer->arg().
static void uploadFile_handler(void){
  if(!SDSTOR_isReady()){
    uploadOk = false;
    return;
  }
  HTTPRaw& raw = webServer->raw();
  if(raw.status == RAW_START){
    String dir = WebServer::urlDecode(webServer->header("X-Dir"));
    String name = WebServer::urlDecode(webServer->header("X-Name"));
    // If the browser declared the upload's total size, the file is
    // pre-extended to that size immediately (see SDSTOR_writeBegin) so a
    // USB host browsing the card sees the true final size right away
    // instead of only the bytes written so far.
    uint64_t expectedTotalBytes = 0;
    String contentLength = webServer->header("Content-Length");
    if(contentLength.length() > 0){
      expectedTotalBytes = strtoull(contentLength.c_str(), nullptr, 10);
    }
    uploadOk = SDSTOR_writeBegin(dir, name, expectedTotalBytes);
  }else if(raw.status == RAW_WRITE){
    if(uploadOk){
      uploadOk = SDSTOR_writeChunk(raw.buf, raw.currentSize);
    }
    yield();   // the raw read/write loop never blocks on its own; without this,
               // long uploads starve WiFi/watchdog housekeeping on this single-core chip
  }else if(raw.status == RAW_END){
    if(uploadOk){
      uploadOk = SDSTOR_writeEnd();
    }else{
      SDSTOR_writeAbort();   // a chunk failed: close file, reacquire LCD bus
    }
  }else{
    SDSTOR_writeAbort();
    uploadOk = false;
  }
}

static void uploadDone(void){
  if(!requireSD()){
    return;
  }
  if(uploadOk){
    webServer->send(200, "text/plain", "OK");
  }else{
    String err = SDSTOR_lastError();
    if(err.length() == 0){
      err = "Upload failed";
    }
    webServer->send(400, "text/plain", err);
  }
}

static void apList(void){
  webServer->send(200, "text/plain", WIFIC_getApList());
}

static void sdStatus(void){
  String s = SDSTOR_isReady() ? "ready" : (SDSTOR_isFaulty() ? "faulty" : "missing");
  webServer->send(200, "text/plain", s);
}

static void sdSpace(void){
  webServer->send(200, "text/plain", SDSTOR_getSpaceInfo());
}

static void updateCheckInfo(void){
  webServer->send(200, "text/plain", UPDATE_CHECK_isAvailable() ? UPDATE_CHECK_getLatestVersion() : "");
}

static void checkUpdateNow(void){
  UPDATE_CHECK_forceCheck();
  webServer->send(200, "text/plain", UPDATE_CHECK_isAvailable() ? UPDATE_CHECK_getLatestVersion() : "");
}

static void downloadUpdate(void){
  if(UPDATE_CHECK_downloadToSD()){
    webServer->send(200, "text/plain", "Firmware downloaded to SD card.");
  }else{
    webServer->send(400, "text/plain", UPDATE_CHECK_lastError());
  }
}

static void applyUpdate(void){
  if(!UPDATE_CHECK_applyFromSD()){
    webServer->send(400, "text/plain", UPDATE_CHECK_lastError());
    return;
  }
  webServer->send(200, "text/plain", "Update applied. Rebooting...");
  delay(500);
  ESP.restart();
}

static void wifiRssi(void){
  webServer->send(200, "text/plain", String(WIFIC_getRssi()));
}

static void ejectSD(void){
  if(SDSTOR_eject()){
    webServer->send(200, "text/plain", "SD card safely ejected. You may remove it now.");
  }else{
    webServer->send(200, "text/plain", "No SD card to eject.");
  }
}

static void formatSD(void){
  if(SDSTOR_format()){
    webServer->send(200, "text/plain", "SD card formatted.");
  }else{
    webServer->send(400, "text/plain", "Format failed. Check the card is inserted and not write-protected.");
  }
}

// --- Public functions ---
void HTTP_SERVER_process(void){
  webServer->handleClient();
}

void HTTP_SERVER_init(void){
  if (webServer != nullptr) {
    delete webServer; // Clean up old one
  }
  webServer = new WebServer(80);

  webServer->on("/", HTTP_GET, showStartPage);
  webServer->on("/favicon.ico", HTTP_GET, showNotFound);
  webServer->on("/config", HTTP_GET, selectAP);
  webServer->on("/api", HTTP_GET, showApiPage);
  webServer->on("/wifisave", HTTP_GET, saveWiFi);
  webServer->on("/brightness", HTTP_GET, setBrightness);
  webServer->on("/theme", HTTP_GET, toggleTheme);
  webServer->on("/rotate180", HTTP_GET, rotateScreen180);
  webServer->on("/writemaster", HTTP_GET, toggleWriteMaster);
  webServer->on("/api/writemaster", HTTP_GET, writeMasterStatus);
  webServer->on("/api/filelist", HTTP_GET, fileList);
  webServer->on("/download", HTTP_GET, downloadFile);
  webServer->on("/preview", HTTP_GET, previewFile);
  webServer->on("/upload", HTTP_POST, uploadDone, uploadFile_handler);
  webServer->on("/delete", HTTP_GET, deleteFile);
  webServer->on("/mkdir", HTTP_GET, mkdirHandler);
  webServer->on("/move", HTTP_GET, moveFile);
  webServer->on("/rename", HTTP_GET, renameHandler);
  webServer->on("/aplist", HTTP_GET, apList);
  webServer->on("/api/sdstatus", HTTP_GET, sdStatus);
  webServer->on("/api/sdspace", HTTP_GET, sdSpace);
  webServer->on("/api/updatecheck", HTTP_GET, updateCheckInfo);
  webServer->on("/checkupdatenow", HTTP_GET, checkUpdateNow);
  webServer->on("/downloadupdate", HTTP_GET, downloadUpdate);
  webServer->on("/applyupdate", HTTP_GET, applyUpdate);
  webServer->on("/api/rssi", HTTP_GET, wifiRssi);
  webServer->on("/eject", HTTP_GET, ejectSD);
  webServer->on("/format", HTTP_GET, formatSD);
  webServer->onNotFound(showStartPage);

  static const char* uploadHeaderKeys[] = {"X-Dir", "X-Name", "Content-Length"};
  webServer->collectHeaders(uploadHeaderKeys, 3);

  webServer->begin();
}
