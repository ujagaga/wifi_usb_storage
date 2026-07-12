#include <SD.h>
#include <SPI.h>
#include <WebServer.h>
#include <sd_diskio.h>   // raw card init (sdcard_init/sdcard_uninit), bypasses the filesystem
#include <diskio.h>      // disk_initialize() - talks SD protocol directly (DSTATUS/STA_NOINIT)
#include <ff.h>          // f_mkfs() - force a fresh FAT filesystem, used by SDSTOR_format()
#include <Update.h>      // flashes a staged firmware image, used by SDSTOR_applyFirmwareUpdate()
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
// A logical file "name" in folder "dirPrefix" that never exceeds the cap is
// stored exactly as before: a single plain file "<dirPrefix>/name". One that
// grows past the cap during upload is transparently split into hidden part
// files "<dirPrefix>/.name.001", "<dirPrefix>/.name.002", ... plus a hidden
// manifest "<dirPrefix>/.name.manifest" holding the total byte count.
// Anything starting with '.' is considered internal and is skipped by the
// listing (see SDSTOR_list()). "dirPrefix" is "" for root, or "/Sub/Dir"
// otherwise - always produced by sanitizeDir() below, never taken raw.
#define SD_MANIFEST_SUFFIX ".manifest"

static String partPath(const String& dirPrefix, const String& baseName, int idx){
  char suffix[8];
  snprintf(suffix, sizeof(suffix), "%03d", idx);
  return dirPrefix + "/." + baseName + "." + String(suffix);
}

static String manifestPath(const String& dirPrefix, const String& baseName){
  return dirPrefix + "/." + baseName + SD_MANIFEST_SUFFIX;
}

// Cleans one path segment down to a safe set of characters, and rejects "."
// and ".." (which would otherwise pass the character whitelist since '.' is
// allowed) so a directory path can never be built that escapes root. Returns
// "" if the segment is invalid or empty after cleaning.
static String sanitizeSegment(String seg){
  if(seg == "." || seg == ".."){
    return "";
  }
  if(seg.length() > 255){   // FatFs long-filename cap (FF_LFN_BUF in ffconf.h)
    seg = seg.substring(0, 255);
  }
  String cleaned;
  for(unsigned i = 0; i < seg.length(); i++){
    char c = seg[i];
    bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-' || c == ' ';
    if(ok){
      cleaned += c;
    }
  }
  if(cleaned.length() == 0 || cleaned == "." || cleaned == ".."){
    return "";
  }
  return cleaned;
}

// Sanitize a single file/folder leaf name (no path separators). Returns ""
// if invalid.
static String sanitizeName(String name){
  int slash = name.lastIndexOf('/');
  if(slash >= 0){
    name = name.substring(slash + 1);
  }
  slash = name.lastIndexOf('\\');
  if(slash >= 0){
    name = name.substring(slash + 1);
  }
  return sanitizeSegment(name);
}

// Turns a client-supplied relative directory ("" for root, or e.g.
// "Photos/2024") into a safe absolute prefix ("" for root, "/Photos/2024"
// otherwise). Each segment is independently sanitized; stray leading/
// trailing/doubled slashes are simply skipped over. Returns false if any
// non-empty segment is invalid, in which case outPrefix is left as "".
static bool sanitizeDir(const String& dir, String& outPrefix){
  outPrefix = "";
  int start = 0;
  while(start <= (int)dir.length()){
    int slash = dir.indexOf('/', start);
    String seg = (slash < 0) ? dir.substring(start) : dir.substring(start, slash);
    if(seg.length() > 0){
      String cleaned = sanitizeSegment(seg);
      if(cleaned.length() == 0){
        outPrefix = "";
        return false;
      }
      outPrefix += "/" + cleaned;
    }
    if(slash < 0){
      break;
    }
    start = slash + 1;
  }
  return true;
}

// Deletes a logical file regardless of how it's stored: a single plain file,
// or - for anything that outgrew SD_PART_MAX_BYTES during upload - a
// manifest plus a run of hidden numbered part files. Returns true if
// anything existed to delete. Caller holds the SD bus.
static bool removeLogicalFile(const String& dirPrefix, const String& baseName){
  bool existed = false;

  String plain = dirPrefix + "/" + baseName;
  if(SD.exists(plain)){
    SD.remove(plain);
    existed = true;
  }

  String mPath = manifestPath(dirPrefix, baseName);
  if(SD.exists(mPath)){
    SD.remove(mPath);
    existed = true;
    for(int i = 1; ; i++){
      String p = partPath(dirPrefix, baseName, i);
      if(!SD.exists(p)){
        break;
      }
      SD.remove(p);
    }
  }

  return existed;
}

// Recursively removes a folder and everything inside it. Caller holds the SD bus.
static bool removeDirRecursive(const String& path){
  File dir = SD.open(path);
  if(!dir){
    return false;
  }
  if(!dir.isDirectory()){
    dir.close();
    return false;
  }
  for(File entry = dir.openNextFile(); entry; entry = dir.openNextFile()){
    String n = String(entry.name());
    bool isDir = entry.isDirectory();
    entry.close();
    String childPath = path + "/" + n;
    if(isDir){
      removeDirRecursive(childPath);
    }else{
      SD.remove(childPath);
    }
  }
  dir.close();
  return SD.rmdir(path);
}

// Return the names in "dir" joined with '|', each as "name:size" for a file
// or "name/:0" for a subfolder. Split files are reported once, under their
// logical name, with their combined size.
String SDSTOR_list(String dir){
  String result = "";
  if(!cardReady){
    return result;
  }
  String dirPrefix;
  if(!sanitizeDir(dir, dirPrefix)){
    return result;
  }
  String openPath = dirPrefix.length() ? dirPrefix : "/";

  LCD_busRelease();

  // Pass 1: split files, represented by their hidden manifest.
  File root = SD.open(openPath);
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
  root = SD.open(openPath);
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

  // Pass 3: subfolders.
  root = SD.open(openPath);
  if(root){
    for(File entry = root.openNextFile(); entry; entry = root.openNextFile()){
      String n = String(entry.name());
      if(entry.isDirectory() && !(n.length() > 0 && n[0] == '.')){
        if(result.length() > 0){
          result += "|";
        }
        result += n + "/:0";
      }
      entry.close();
    }
    root.close();
  }

  LCD_busAcquire();

  return result;
}

// Creates a subfolder "name" inside "dir".
bool SDSTOR_mkdir(String dir, String name){
  if(!cardReady){
    return false;
  }
  String dirPrefix;
  if(!sanitizeDir(dir, dirPrefix)){
    return false;
  }
  String leaf = sanitizeName(name);
  if(leaf.length() == 0){
    return false;
  }
  LCD_busRelease();
  bool ok = SD.mkdir(dirPrefix + "/" + leaf);
  LCD_busAcquire();
  return ok;
}

// Streams the hidden numbered parts of a split logical file back to back.
// The combined size can exceed what WebServer's 32-bit setContentLength()
// can express, so this uses chunked transfer encoding instead of a fixed
// Content-Length.
static bool sendSplitFile(const String& dirPrefix, const String& baseName, const String& displayName, WebServer* server){
  server->sendHeader("Content-Disposition", "attachment; filename=\"" + displayName + "\"");
  server->setContentLength(CONTENT_LENGTH_UNKNOWN);
  server->send(200, "application/octet-stream", "");

  static uint8_t buf[512];
  for(int i = 1; ; i++){
    LCD_busRelease();
    File f = SD.open(partPath(dirPrefix, baseName, i));
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
bool SDSTOR_sendRaw(String dir, String name, WebServer* server){
  if(!cardReady){
    return false;
  }
  String dirPrefix;
  if(!sanitizeDir(dir, dirPrefix)){
    return false;
  }
  String baseName = sanitizeName(name);
  if(baseName.length() == 0){
    return false;
  }
  String path = dirPrefix + "/" + baseName;

  LCD_busRelease();
  bool isSplit = SD.exists(manifestPath(dirPrefix, baseName));
  LCD_busAcquire();

  if(isSplit){
    return sendSplitFile(dirPrefix, baseName, name, server);
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

// Streams up to maxBytes of a file as plain text (no Content-Disposition, so
// it's inline rather than a download) - used for the web UI's text preview.
// If the file was split, only part 1 is read; maxBytes is always far smaller
// than SD_PART_MAX_BYTES, so this never needs to cross into part 2.
bool SDSTOR_sendPreview(String dir, String name, uint32_t maxBytes, WebServer* server){
  if(!cardReady){
    return false;
  }
  String dirPrefix;
  if(!sanitizeDir(dir, dirPrefix)){
    return false;
  }
  String baseName = sanitizeName(name);
  if(baseName.length() == 0){
    return false;
  }

  LCD_busRelease();
  bool isSplit = SD.exists(manifestPath(dirPrefix, baseName));
  String path = isSplit ? partPath(dirPrefix, baseName, 1) : (dirPrefix + "/" + baseName);
  File f = SD.open(path);
  LCD_busAcquire();
  if(!f){
    return false;
  }

  server->setContentLength(CONTENT_LENGTH_UNKNOWN);
  server->send(200, "text/plain", "");

  static uint8_t buf[512];
  uint32_t sent = 0;
  while(sent < maxBytes){
    uint32_t remaining = maxBytes - sent;
    size_t want = sizeof(buf);
    if(remaining < want){
      want = (size_t)remaining;
    }
    LCD_busRelease();
    int got = f.read(buf, want);
    LCD_busAcquire();
    if(got <= 0){
      break;
    }
    server->sendContent((const char*)buf, got);
    sent += got;
  }

  f.close();
  return true;
}

// --- Incremental upload to SD ---
static File uploadFile;
static String uploadDirPrefix;    // absolute folder prefix ("" for root), resolved at writeBegin
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
bool SDSTOR_writeBegin(String dir, String name){
  uploadErr = "";
  if(!cardReady){
    uploadErr = "SD card not ready";
    return false;
  }
  String dirPrefix;
  if(!sanitizeDir(dir, dirPrefix)){
    uploadErr = "Invalid folder: '" + dir + "'";
    return false;
  }
  String baseName = sanitizeName(name);
  if(baseName.length() == 0){
    uploadErr = "Invalid file name: '" + name + "'";
    return false;
  }

  uploadDirPrefix = dirPrefix;
  uploadBaseName = baseName;
  uploadPartBytes = 0;
  uploadTotalBytes = 0;
  uploadPartIndex = 0;

  String path = dirPrefix + "/" + baseName;
  LCD_busRelease();
  removeLogicalFile(uploadDirPrefix, uploadBaseName);   // drop any previous version, split or not
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
    SD.rename(uploadDirPrefix + "/" + uploadBaseName, partPath(uploadDirPrefix, uploadBaseName, 1));
    uploadPartIndex = 1;
  }

  uploadPartIndex++;
  String next = partPath(uploadDirPrefix, uploadBaseName, uploadPartIndex);
  uploadFile = SD.open(next, FILE_WRITE);
  uploadPartBytes = 0;
  if(!uploadFile){
    uploadErr = "Cannot open " + next + " for write";
    return false;
  }
  return true;
}

static String currentUploadPath(void){
  if(uploadPartIndex == 0){
    return uploadDirPrefix + "/" + uploadBaseName;
  }
  return partPath(uploadDirPrefix, uploadBaseName, uploadPartIndex);
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
      if(wrote < want){
        // A brief SD stall (wear-leveling/cache flush) can leave the
        // underlying stdio stream latched in an error state, where every
        // further write() on the SAME handle keeps failing even after the
        // card recovers. Closing and reopening for append clears that
        // latched state and continues from the correct offset.
        int retries = 0;
        while(wrote < want && retries < 5){
          delay(300);
          uploadFile.close();
          uploadFile = SD.open(currentUploadPath(), FILE_APPEND);
          if(!uploadFile){
            retries++;
            continue;
          }
          size_t more = uploadFile.write(buf + offset + wrote, want - wrote);
          wrote += more;
          retries = (more == 0) ? (retries + 1) : 0;
        }
      }
      if(wrote != want){
        uploadErr = "SD write failed (wrote " + String(wrote) + " of " + String(want) +
          ", total=" + String((unsigned long long)uploadTotalBytes) + ")";
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
    File mf = SD.open(manifestPath(uploadDirPrefix, uploadBaseName), FILE_WRITE);
    if(mf){
      mf.println(String((unsigned long long)uploadTotalBytes));
      mf.close();
    }
  }

  LCD_busAcquire();
  return true;
}

// Deletes a file or a folder named "name" inside "dir". A folder is removed
// recursively, along with everything inside it.
bool SDSTOR_delete(String dir, String name){
  if(!cardReady){
    return false;
  }
  String dirPrefix;
  if(!sanitizeDir(dir, dirPrefix)){
    return false;
  }
  String baseName = sanitizeName(name);
  if(baseName.length() == 0){
    return false;
  }

  LCD_busRelease();
  String path = dirPrefix + "/" + baseName;
  File entry = SD.open(path);
  bool isDir = entry && entry.isDirectory();
  if(entry){
    entry.close();
  }
  bool ok = isDir ? removeDirRecursive(path) : removeLogicalFile(dirPrefix, baseName);
  LCD_busAcquire();
  return ok;
}

// Moves a file named "name" from "srcDir" into "destDir". Fails (no changes
// made) if destDir doesn't exist or already has an entry called "name" -
// this never overwrites, unlike upload. SD.rename() is metadata-only on
// FAT, so this doesn't copy file data even for a multi-part split file.
bool SDSTOR_move(String srcDir, String name, String destDir){
  if(!cardReady){
    return false;
  }
  String srcPrefix, destPrefix;
  if(!sanitizeDir(srcDir, srcPrefix) || !sanitizeDir(destDir, destPrefix)){
    return false;
  }
  String baseName = sanitizeName(name);
  if(baseName.length() == 0){
    return false;
  }
  if(srcPrefix == destPrefix){
    return true;   // already there
  }

  LCD_busRelease();

  File destDirFile = SD.open(destPrefix.length() ? destPrefix : "/");
  bool destOk = destDirFile && destDirFile.isDirectory();
  if(destDirFile){
    destDirFile.close();
  }
  if(!destOk){
    LCD_busAcquire();
    return false;
  }

  String destPlain = destPrefix + "/" + baseName;
  if(SD.exists(destPlain) || SD.exists(manifestPath(destPrefix, baseName))){
    LCD_busAcquire();
    return false;   // destination name already taken
  }

  bool ok;
  if(SD.exists(manifestPath(srcPrefix, baseName))){
    ok = SD.rename(manifestPath(srcPrefix, baseName), manifestPath(destPrefix, baseName));
    for(int i = 1; ok; i++){
      String srcPart = partPath(srcPrefix, baseName, i);
      if(!SD.exists(srcPart)){
        break;
      }
      ok = SD.rename(srcPart, partPath(destPrefix, baseName, i));
    }
  }else{
    String srcPlain = srcPrefix + "/" + baseName;
    ok = SD.exists(srcPlain) && SD.rename(srcPlain, destPlain);
  }

  LCD_busAcquire();
  return ok;
}

// Renames a file or folder "oldName" to "newName", both inside "dir". Fails
// (no changes made) if "newName" is already taken. A folder rename is a
// single metadata-only SD.rename() - its contents are addressed relative to
// it, so they're unaffected. A file rename renames the manifest and every
// numbered part too, same as SDSTOR_move().
bool SDSTOR_rename(String dir, String oldName, String newName){
  if(!cardReady){
    return false;
  }
  String dirPrefix;
  if(!sanitizeDir(dir, dirPrefix)){
    return false;
  }
  String oldBase = sanitizeName(oldName);
  String newBase = sanitizeName(newName);
  if(oldBase.length() == 0 || newBase.length() == 0){
    return false;
  }
  if(oldBase == newBase){
    return true;   // no-op
  }

  LCD_busRelease();

  String oldPath = dirPrefix + "/" + oldBase;
  String newPath = dirPrefix + "/" + newBase;

  File entry = SD.open(oldPath);
  bool isDir = entry && entry.isDirectory();
  if(entry){
    entry.close();
  }

  if(SD.exists(newPath) || SD.exists(manifestPath(dirPrefix, newBase))){
    LCD_busAcquire();
    return false;   // destination name already taken
  }

  bool ok;
  if(isDir){
    ok = SD.rename(oldPath, newPath);
  }else if(SD.exists(manifestPath(dirPrefix, oldBase))){
    ok = SD.rename(manifestPath(dirPrefix, oldBase), manifestPath(dirPrefix, newBase));
    for(int i = 1; ok; i++){
      String oldPart = partPath(dirPrefix, oldBase, i);
      if(!SD.exists(oldPart)){
        break;
      }
      ok = SD.rename(oldPart, partPath(dirPrefix, newBase, i));
    }
  }else{
    ok = SD.exists(oldPath) && SD.rename(oldPath, newPath);
  }

  LCD_busAcquire();
  return ok;
}

void SDSTOR_writeAbort(void){
  if(uploadFile){
    uploadFile.close();
    removeLogicalFile(uploadDirPrefix, uploadBaseName);
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

// "total:free" bytes, or "" if no card is mounted.
String SDSTOR_getSpaceInfo(void){
  if(!cardReady){
    return "";
  }
  LCD_busRelease();
  uint64_t total = SD.totalBytes();
  uint64_t used = SD.usedBytes();
  LCD_busAcquire();
  uint64_t free = (total > used) ? (total - used) : 0;
  return String(total) + ":" + String(free);
}

// Whole-file read of an absolute path (e.g. WIFI_CFG_PATH), for small config
// files that don't go through the folder-relative dir/name API above.
bool SDSTOR_readTextFile(const String& path, String& outContent){
  if(!cardReady){
    return false;
  }
  LCD_busRelease();
  File f = SD.open(path);
  bool ok = (bool)f;
  if(ok){
    outContent = "";
    while(f.available()){
      outContent += (char)f.read();
    }
    f.close();
  }
  LCD_busAcquire();
  return ok;
}

bool SDSTOR_writeTextFile(const String& path, const String& content){
  if(!cardReady){
    return false;
  }
  LCD_busRelease();
  if(SD.exists(path)){
    SD.remove(path);
  }
  File f = SD.open(path, FILE_WRITE);
  bool ok = (bool)f;
  if(ok){
    f.write((const uint8_t*)content.c_str(), content.length());
    f.close();
  }
  LCD_busAcquire();
  return ok;
}

// Flashes a firmware image staged on SD into the inactive OTA partition.
// Does not reboot - caller decides when (e.g. after sending an HTTP
// response). CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE is on by default for this
// board, so if the new image never confirms itself valid (see
// esp_ota_mark_app_valid_cancel_rollback() in the .ino), the bootloader
// automatically falls back to the previous partition on the next reset.
bool SDSTOR_applyFirmwareUpdate(const String& path, const String& expectedMd5, String& outError){
  if(!cardReady){
    outError = "SD card not ready";
    return false;
  }
  LCD_busRelease();
  File f = SD.open(path);
  if(!f){
    LCD_busAcquire();
    outError = "Cannot open " + path;
    return false;
  }
  size_t fileSize = f.size();
  if(fileSize == 0){
    f.close();
    LCD_busAcquire();
    outError = "Firmware file is empty";
    return false;
  }
  if(!Update.begin(fileSize, U_FLASH)){
    f.close();
    LCD_busAcquire();
    outError = String("Update.begin failed: ") + Update.errorString();
    return false;
  }
  if(expectedMd5.length() > 0 && !Update.setMD5(expectedMd5.c_str())){
    f.close();
    Update.abort();
    LCD_busAcquire();
    outError = "Invalid expected MD5: " + expectedMd5;
    return false;
  }

  static uint8_t buf[1024];
  size_t written = 0;
  bool ok = true;
  while(written < fileSize){
    int got = f.read(buf, sizeof(buf));
    if(got <= 0){
      ok = false;
      outError = "SD read failed at " + String(written) + " of " + String(fileSize);
      break;
    }
    size_t wrote = Update.write(buf, got);
    if(wrote != (size_t)got){
      ok = false;
      outError = String("Flash write failed: ") + Update.errorString();
      break;
    }
    written += wrote;
  }
  f.close();

  if(ok && !Update.end(true)){
    ok = false;
    outError = String("Update.end failed: ") + Update.errorString();
  }
  if(!ok){
    Update.abort();
  }
  LCD_busAcquire();
  return ok;
}
