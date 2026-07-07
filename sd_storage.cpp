#include <SD.h>
#include <SPI.h>
#include <WebServer.h>
#include "config.h"
#include "lcd_display.h"
#include "sd_storage.h"

static bool cardReady = false;
static uint32_t lastRetryMs = 0;
#define SD_RETRY_INTERVAL_MS 2000

bool SDSTOR_init(void){
  LCD_busRelease();
  cardReady = SD.begin(SD_CS, SPI, SD_SPI_FREQ);
  LCD_busAcquire();
  return cardReady;
}

// If the card wasn't ready at boot (e.g. inserted afterwards), retry
// periodically instead of leaving cardReady stuck false forever.
bool SDSTOR_isReady(void){
  uint32_t now = millis();
  if(!cardReady && (now - lastRetryMs) >= SD_RETRY_INTERVAL_MS){
    lastRetryMs = now;
    SD.end();
    SDSTOR_init();
  }
  return cardReady;
}

// Return root-level file names joined with '|', each as "name:size".
String SDSTOR_list(void){
  String result = "";
  if(!cardReady){
    return result;
  }

  LCD_busRelease();
  File root = SD.open("/");
  if(root){
    for(File entry = root.openNextFile(); entry; entry = root.openNextFile()){
      if(!entry.isDirectory()){
        if(result.length() > 0){
          result += "|";
        }
        result += String(entry.name()) + ":" + String(entry.size());
      }
      entry.close();
    }
    root.close();
  }
  LCD_busAcquire();

  return result;
}

// Stream a file to an HTTP client as application/octet-stream, download-as-attachment.
bool SDSTOR_sendRaw(String name, WebServer* server){
  if(!cardReady){
    return false;
  }

  String path = name.startsWith("/") ? name : ("/" + name);

  LCD_busRelease();
  File f = SD.open(path);
  LCD_busAcquire();
  if(!f){
    return false;
  }

  server->sendHeader("Content-Disposition", "attachment; filename=\"" + name + "\"");
  server->setContentLength(f.size());
  server->send(200, "application/octet-stream", "");

  static uint8_t buf[512];
  while(true){
    LCD_busRelease();
    int got = f.read(buf, sizeof(buf));
    LCD_busAcquire();
    if(got <= 0){
      break;
    }
    server->sendContent((const char*)buf, got);
  }

  f.close();
  return true;
}

// --- Incremental upload to SD ---
static File uploadFile;
static String uploadPath;
static String uploadErr;

String SDSTOR_lastError(void){
  return uploadErr;
}

// Sanitize to a safe root-level path. Returns "" if invalid.
static String sanitizeName(String name){
  int slash = name.lastIndexOf('/');
  if(slash >= 0){
    name = name.substring(slash + 1);
  }
  slash = name.lastIndexOf('\\');
  if(slash >= 0){
    name = name.substring(slash + 1);
  }
  if(name.length() == 0 || name.length() > 64){
    return "";
  }
  for(unsigned i = 0; i < name.length(); i++){
    char c = name[i];
    bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-' || c == ' ';
    if(!ok){
      return "";
    }
  }
  return "/" + name;
}

// The SD bus is held (LCD released) for the whole upload: begin->chunks->end.
// Re-acquiring the LCD bus between chunks re-latches SPI on the C6 and silently
// corrupts the in-progress SD write.
bool SDSTOR_writeBegin(String name){
  uploadErr = "";
  if(!cardReady){
    uploadErr = "SD card not ready";
    return false;
  }
  uploadPath = sanitizeName(name);
  if(uploadPath.length() == 0){
    uploadErr = "Invalid file name: '" + name + "'";
    return false;
  }
  LCD_busRelease();
  if(SD.exists(uploadPath)){
    SD.remove(uploadPath);
  }
  uploadFile = SD.open(uploadPath, FILE_WRITE);
  if(!uploadFile){
    LCD_busAcquire();
    uploadErr = "Cannot open " + uploadPath + " for write";
    return false;
  }
  return true;
}

bool SDSTOR_writeChunk(const uint8_t* buf, size_t len){
  if(!uploadFile){
    return false;
  }
  size_t wrote = uploadFile.write(buf, len);
  if(wrote != len){
    uploadErr = "SD write failed (wrote " + String(wrote) + " of " + String(len) + ")";
    return false;
  }
  return true;
}

bool SDSTOR_writeEnd(void){
  if(!uploadFile){
    return false;
  }
  uploadFile.flush();
  uploadFile.close();
  LCD_busAcquire();
  return true;
}

bool SDSTOR_delete(String name){
  if(!cardReady){
    return false;
  }
  String path = sanitizeName(name);
  if(path.length() == 0){
    return false;
  }
  LCD_busRelease();
  bool ok = SD.exists(path) && SD.remove(path);
  LCD_busAcquire();
  return ok;
}

void SDSTOR_writeAbort(void){
  if(uploadFile){
    uploadFile.close();
    SD.remove(uploadPath);
    LCD_busAcquire();
  }
}
