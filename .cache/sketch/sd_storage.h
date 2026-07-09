#line 1 "/home/rada/Projects/wifi_usb_storage/sd_storage.h"
#ifndef SD_STORAGE_H
#define SD_STORAGE_H

#include <Arduino.h>

class WebServer;

// Generic file storage on the microSD card, organized into folders. Shares
// the SPI bus with the LCD; see lcd_display.h LCD_busRelease/LCD_busAcquire.
//
// Logical files bigger than SD_PART_MAX_BYTES (FAT32's 4GiB-1 per-file cap)
// are transparently split into hidden numbered part files plus a manifest;
// callers never see this - SDSTOR_list/_sendRaw/_delete all operate on the
// single logical name and combined size.
//
// `dir` parameters are a relative path ("" for root, or e.g. "Photos/2024"),
// no leading/trailing slash; every entry point sanitizes it itself.

extern bool SDSTOR_init(void);
extern bool SDSTOR_isReady(void);          // true if the SD card is mounted and usable
extern bool SDSTOR_isFaulty(void);         // true if a card is present but its filesystem can't be read
extern bool SDSTOR_eject(void);            // finish pending writes, unmount for safe removal
extern bool SDSTOR_format(void);           // wipe the card and lay down a fresh FAT filesystem
extern String SDSTOR_list(String dir);     // "name:size|name:size|...", dirs reported as "name/:0"
extern bool SDSTOR_mkdir(String dir, String name);
extern bool SDSTOR_sendRaw(String dir, String name, WebServer* server);  // stream file to HTTP client
extern bool SDSTOR_delete(String dir, String name);  // recursively removes a folder
extern bool SDSTOR_move(String srcDir, String name, String destDir);  // fails if destDir already has this name
extern bool SDSTOR_rename(String dir, String oldName, String newName);  // fails if newName is already taken

// Incremental write of an uploaded file to SD (call in START/WRITE/END order).
extern bool SDSTOR_writeBegin(String dir, String name);
extern bool SDSTOR_writeChunk(const uint8_t* buf, size_t len);
extern bool SDSTOR_writeEnd(void);
extern void SDSTOR_writeAbort(void);
extern String SDSTOR_lastError(void);      // reason the last upload failed

#endif
