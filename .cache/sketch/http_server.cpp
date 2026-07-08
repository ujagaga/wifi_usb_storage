#line 1 "/home/rada/Projects/wifi_usb_storage/http_server.cpp"
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
void showStartPage() {
  String response = FPSTR(HTML_BEGIN);
  response += FPSTR(INDEX_HTML_0);
  response += FPSTR(NAV_HTML);
  response += "<h1>WiFi File Storage</h1>";
  response += "<p class='ip'>Station IP: " + WIFIC_getStationIp() + "</p>";
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
  String response = FPSTR(HTML_BEGIN);
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
  String response = FPSTR(HTML_BEGIN);
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
  String response = FPSTR(HTML_BEGIN);
  response += FPSTR(APLIST_HTML_0);
  response += FPSTR(NAV_HTML);
  response += FPSTR(APLIST_HTML_1);
  response += FPSTR(APLIST_HTML_2);
  response += FPSTR(HTML_END);
  webServer->send(200, "text/html", response);
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
  webServer->send(200, "text/plain", SDSTOR_list());
}

static void downloadFile(void){
  if(!requireSD()){
    return;
  }
  String name = webServer->arg("name");
  if(name.length() == 0 || !SDSTOR_sendRaw(name, webServer)){
    webServer->send(404, "text/plain", "File not found");
  }
}

static void deleteFile(void){
  if(!requireSD()){
    return;
  }
  String name = webServer->arg("name");
  if(name.length() == 0 || !SDSTOR_delete(name)){
    webServer->send(400, "text/plain", "Delete failed");
    return;
  }
  webServer->send(200, "text/plain", "OK");
}

static bool uploadOk = false;

// Streams a multipart file upload straight to the SD card, chunk by chunk.
static void uploadFile_handler(void){
  if(!SDSTOR_isReady()){
    uploadOk = false;
    return;
  }
  HTTPUpload& up = webServer->upload();
  if(up.status == UPLOAD_FILE_START){
    uploadOk = SDSTOR_writeBegin(up.filename);
  }else if(up.status == UPLOAD_FILE_WRITE){
    if(uploadOk){
      uploadOk = SDSTOR_writeChunk(up.buf, up.currentSize);
    }
  }else if(up.status == UPLOAD_FILE_END){
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
  webServer->on("/selectap", HTTP_GET, selectAP);
  webServer->on("/api", HTTP_GET, showApiPage);
  webServer->on("/wifisave", HTTP_GET, saveWiFi);
  webServer->on("/api/filelist", HTTP_GET, fileList);
  webServer->on("/download", HTTP_GET, downloadFile);
  webServer->on("/upload", HTTP_POST, uploadDone, uploadFile_handler);
  webServer->on("/delete", HTTP_GET, deleteFile);
  webServer->on("/aplist", HTTP_GET, apList);
  webServer->on("/api/sdstatus", HTTP_GET, sdStatus);
  webServer->on("/eject", HTTP_GET, ejectSD);
  webServer->on("/format", HTTP_GET, formatSD);
  webServer->onNotFound(showStartPage);

  webServer->begin();
}
