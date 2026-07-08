#include <SD.h>
#include <SPI.h>
#include <WebServer.h>
#include <sd_diskio.h>   // raw card init (sdcard_init/sdcard_uninit), bypasses the filesystem
#include <diskio.h>      // disk_initialize() - talks SD protocol directly (DSTATUS/STA_NOINIT)
#include <ff.h>          // f_mkfs() - force a fresh FAT filesystem, used by SDSTOR_format()
#include "config.h"
#include "lcd_display.h"
#include "sd_storage.h"

static bool cardReady = false;
static bool cardFaulty = false;  // hardware present, but no readable filesystem
static bool ejected = false;     // true after SDSTOR_eject(), until the card is physically removed
static uint32_t lastRetryMs = 0;
#define SD_RETRY_INTERVAL_MS 2000

// Talks raw SD protocol on a throwaway drive slot (GO_IDLE_STATE etc.) and
// tears it straight back down. This is what tells "no card" apart from "card
// present but its filesystem is unreadable" - SD.begin() alone can't, since
// it fails the same way for both (no card, and no/bad filesystem).
static bool sdCardPresentProbe(void){
  uint8_t pdrv = sdcard_init(SD_CS, &SPI, SD_SPI_FREQ);
  if(pdrv == 0xFF){
    return false;
  }
  DSTATUS status = disk_initialize(pdrv);
  sdcard_uninit(pdrv);
  return (status & STA_NOINIT) == 0;
}

// Probe for hardware presence, then (only if present) try to mount the
// filesystem. Updates cardReady/cardFaulty; leaves `ejected` untouched.
static void attemptMount(void){
  LCD_busRelease();
  SD.end();
  bool present = sdCardPresentProbe();
  bool mounted = present && SD.begin(SD_CS, SPI, SD_SPI_FREQ);
  if(present && !mounted){
    SD.end();
  }
  LCD_busAcquire();
  cardReady = mounted;
  cardFaulty = present && !mounted;
}

bool SDSTOR_init(void){
  ejected = false;
  attemptMount();
  return cardReady;
}

bool SDSTOR_isFaulty(void){
  return cardFaulty;
}

// If the card wasn't ready at boot (e.g. inserted afterwards), retry
// periodically instead of leaving cardReady stuck false forever. Once the
// card has been ejected via SDSTOR_eject(), refuse to remount until the
// hardware probe itself fails, i.e. the card was physically pulled out -
// otherwise an "eject" while the card is still in the slot would be undone
// by the very next retry. While waiting for removal we only probe, we never
// re-mount, so a faulty card doesn't flip back to "ready" on its own either.
bool SDSTOR_isReady(void){
  uint32_t now = millis();
  if(!cardReady && (now - lastRetryMs) >= SD_RETRY_INTERVAL_MS){
    lastRetryMs = now;

    if(ejected){
      LCD_busRelease();
      SD.end();
      bool present = sdCardPresentProbe();
      LCD_busAcquire();
      if(!present){
        ejected = false;   // card actually removed; ready to accept a reinsert
      }
    }else{
      attemptMount();
    }
  }
  return cardReady;
}

// --- Split-file layout for logical files bigger than SD_PART_MAX_BYTES ---
// A logical file "name" that never exceeds the cap is stored exactly as
// before: a single plain file "/name". One that grows past the cap during
// upload is transparently split into hidden part files "/.name.001",
// "/.name.002", ... plus a hidden manifest "/.name.manifest" holding the
// total byte count. Anything starting with '.' is considered internal and
// is skipped by the listing (see SDSTOR_list()).
#define SD_MANIFEST_SUFFIX ".manifest"

static String partPath(const String& baseName, int idx){
  char suffix[8];
  snprintf(suffix, sizeof(suffix), "%03d", idx);
  return "/." + baseName + "." + String(suffix);
}

static String manifestPath(const String& baseName){
  return "/." + baseName + SD_MANIFEST_SUFFIX;
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

// Deletes a logical file regardless of how it's stored: a single plain file,
// or - for anything that outgrew SD_PART_MAX_BYTES during upload - a
// manifest plus a run of hidden numbered part files. Returns true if
// anything existed to delete. Caller holds the SD bus.
static bool removeLogicalFile(const String& baseName){
  bool existed = false;

  String plain = "/" + baseName;
  if(SD.exists(plain)){
    SD.remove(plain);
    existed = true;
  }

  String mPath = manifestPath(baseName);
  if(SD.exists(mPath)){
    SD.remove(mPath);
    existed = true;
    for(int i = 1; ; i++){
      String p = partPath(baseName, i);
      if(!SD.exists(p)){
        break;
      }
      SD.remove(p);
    }
  }

  return existed;
}

// Return root-level file names joined with '|', each as "name:size". Split
// files are reported once, under their logical name, with their combined size.
String SDSTOR_list(void){
  String result = "";
  if(!cardReady){
    return result;
  }

  LCD_busRelease();

  // Pass 1: split files, represented by their hidden manifest.
  File root = SD.open("/");
  if(root){
    for(File entry = root.openNextFile(); entry; entry = root.openNextFile()){
      String n = String(entry.name());
      if(!entry.isDirectory() && n.length() > 0 && n[0] == '.' && n.endsWith(SD_MANIFEST_SUFFIX)){
        String total = "";
        while(entry.available()){
          char c = (char)entry.read();
          if(c == '\n' || c == '\r'){
            break;
          }
          total += c;
        }
        String logicalName = n.substring(1, n.length() - strlen(SD_MANIFEST_SUFFIX));
        if(result.length() > 0){
          result += "|";
        }
        result += logicalName + ":" + total;
      }
      entry.close();
    }
    root.close();
  }

  // Pass 2: plain files (anything starting with '.' is an internal part/manifest).
  root = SD.open("/");
  if(root){
    for(File entry = root.openNextFile(); entry; entry = root.openNextFile()){
      String n = String(entry.name());
      if(!entry.isDirectory() && !(n.length() > 0 && n[0] == '.')){
        if(result.length() > 0){
          result += "|";
        }
        result += n + ":" + String(entry.size());
      }
      entry.close();
    }
    root.close();
  }

  LCD_busAcquire();

  return result;
}

// Streams the hidden numbered parts of a split logical file back to back.
// The combined size can exceed what WebServer's 32-bit setContentLength()
// can express, so this uses chunked transfer encoding instead of a fixed
// Content-Length.
static bool sendSplitFile(const String& baseName, const String& displayName, WebServer* server){
  server->sendHeader("Content-Disposition", "attachment; filename=\"" + displayName + "\"");
  server->setContentLength(CONTENT_LENGTH_UNKNOWN);
  server->send(200, "application/octet-stream", "");

  static uint8_t buf[512];
  for(int i = 1; ; i++){
    LCD_busRelease();
    File f = SD.open(partPath(baseName, i));
    LCD_busAcquire();
    if(!f){
      break;   // no more parts
    }
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
  }
  return true;
}

// Stream a file to an HTTP client as application/octet-stream, download-as-attachment.
bool SDSTOR_sendRaw(String name, WebServer* server){
  if(!cardReady){
    return false;
  }

  String path = name.startsWith("/") ? name : ("/" + name);
  String baseName = path.substring(1);

  LCD_busRelease();
  bool isSplit = SD.exists(manifestPath(baseName));
  LCD_busAcquire();

  if(isSplit){
    return sendSplitFile(baseName, name, server);
  }

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
static String uploadBaseName;     // logical name, no leading slash (e.g. "foo.bin")
static uint64_t uploadPartBytes;  // bytes written to the currently open plain/part file
static uint64_t uploadTotalBytes; // bytes written overall, across all parts
static int uploadPartIndex;       // 0 = still writing the plain (unsplit) file; >=1 once split
static String uploadErr;

String SDSTOR_lastError(void){
  return uploadErr;
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
  String path = sanitizeName(name);
  if(path.length() == 0){
    uploadErr = "Invalid file name: '" + name + "'";
    return false;
  }

  uploadBaseName = path.substring(1);
  uploadPartBytes = 0;
  uploadTotalBytes = 0;
  uploadPartIndex = 0;

  LCD_busRelease();
  removeLogicalFile(uploadBaseName);   // drop any previous version, split or not
  uploadFile = SD.open(path, FILE_WRITE);
  if(!uploadFile){
    LCD_busAcquire();
    uploadErr = "Cannot open " + path + " for write";
    return false;
  }
  return true;
}

// Closes the current plain/part file and opens the next one, promoting the
// plain file to hidden part 1 the first time a file outgrows the cap.
static bool rollToNextPart(void){
  uploadFile.flush();
  uploadFile.close();

  if(uploadPartIndex == 0){
    SD.rename("/" + uploadBaseName, partPath(uploadBaseName, 1));
    uploadPartIndex = 1;
  }

  uploadPartIndex++;
  String next = partPath(uploadBaseName, uploadPartIndex);
  uploadFile = SD.open(next, FILE_WRITE);
  uploadPartBytes = 0;
  if(!uploadFile){
    uploadErr = "Cannot open " + next + " for write";
    return false;
  }
  return true;
}

bool SDSTOR_writeChunk(const uint8_t* buf, size_t len){
  if(!uploadFile){
    return false;
  }

  size_t offset = 0;
  while(offset < len){
    uint64_t remaining = SD_PART_MAX_BYTES - uploadPartBytes;
    size_t want = len - offset;
    if((uint64_t)want > remaining){
      want = (size_t)remaining;
    }
    if(want > 0){
      size_t wrote = uploadFile.write(buf + offset, want);
      if(wrote != want){
        uploadErr = "SD write failed (wrote " + String(wrote) + " of " + String(want) + ")";
        return false;
      }
      uploadPartBytes += wrote;
      uploadTotalBytes += wrote;
      offset += wrote;
    }
    if(uploadPartBytes >= SD_PART_MAX_BYTES && offset < len){
      if(!rollToNextPart()){
        return false;
      }
    }
  }
  return true;
}

bool SDSTOR_writeEnd(void){
  if(!uploadFile){
    return false;
  }
  uploadFile.flush();
  uploadFile.close();

  if(uploadPartIndex > 0){
    // File was split: the manifest is only written now that the final total
    // is known, so an interrupted upload never leaves a manifest behind
    // pointing at an incomplete part run.
    File mf = SD.open(manifestPath(uploadBaseName), FILE_WRITE);
    if(mf){
      mf.println(String((unsigned long long)uploadTotalBytes));
      mf.close();
    }
  }

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
  bool ok = removeLogicalFile(path.substring(1));
  LCD_busAcquire();
  return ok;
}

void SDSTOR_writeAbort(void){
  if(uploadFile){
    uploadFile.close();
    removeLogicalFile(uploadBaseName);
    LCD_busAcquire();
  }
}

// Finish any in-flight write, unmount, and refuse to remount until the card
// is physically pulled out (see SDSTOR_isReady()). Returns false if there
// was no mounted card to eject.
bool SDSTOR_eject(void){
  if(uploadFile){
    uploadFile.flush();
    uploadFile.close();
  }
  if(!cardReady){
    return false;
  }
  LCD_busRelease();
  SD.end();
  LCD_busAcquire();
  cardReady = false;
  ejected = true;
  return true;
}

// Force a fresh FAT filesystem onto the card, wiping all data - this is what
// actually fixes a "faulty" (unreadable filesystem) card. sdcard_mount()'s
// own format_if_empty only kicks in when there's no filesystem at all, so a
// direct f_mkfs() is used here to reformat regardless of current contents.
bool SDSTOR_format(void){
  if(uploadFile){
    uploadFile.close();
  }
  LCD_busRelease();
  SD.end();

  bool ok = false;
  if(sdCardPresentProbe()){
    uint8_t pdrv = sdcard_init(SD_CS, &SPI, SD_SPI_FREQ);
    if(pdrv != 0xFF){
      char drv[3] = {(char)('0' + pdrv), ':', 0};
      BYTE *work = (BYTE*)malloc(FF_MAX_SS);
      if(work){
        MKFS_PARM opt = {(BYTE)FM_ANY, 0, 0, 0, 0};
        ok = (f_mkfs(drv, &opt, work, FF_MAX_SS) == FR_OK);
        free(work);
      }
      sdcard_uninit(pdrv);
    }
  }

  bool mounted = ok && SD.begin(SD_CS, SPI, SD_SPI_FREQ);
  LCD_busAcquire();

  cardReady = mounted;
  cardFaulty = !mounted;
  ejected = false;
  return mounted;
}
