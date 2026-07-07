#ifndef SD_STORAGE_H
#define SD_STORAGE_H

#include <Arduino.h>

class WebServer;

// Generic file storage on the microSD card (root directory only). Shares the
// SPI bus with the LCD; see lcd_display.h LCD_busRelease/LCD_busAcquire.

extern bool SDSTOR_init(void);
extern bool SDSTOR_isReady(void);          // true if the SD card was detected
extern String SDSTOR_list(void);           // "name:size|name:size|..."
extern bool SDSTOR_sendRaw(String name, WebServer* server);  // stream file to HTTP client
extern bool SDSTOR_delete(String name);

// Incremental write of an uploaded file to SD (call in START/WRITE/END order).
extern bool SDSTOR_writeBegin(String name);
extern bool SDSTOR_writeChunk(const uint8_t* buf, size_t len);
extern bool SDSTOR_writeEnd(void);
extern void SDSTOR_writeAbort(void);
extern String SDSTOR_lastError(void);      // reason the last upload failed

#endif
