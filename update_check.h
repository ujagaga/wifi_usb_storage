#ifndef UPDATE_CHECK_H
#define UPDATE_CHECK_H

#include <Arduino.h>

extern void UPDATE_CHECK_init(void);
extern void UPDATE_CHECK_process(void);
extern bool UPDATE_CHECK_isAvailable(void);        // true once a differing remote version has been seen
extern String UPDATE_CHECK_getLatestVersion(void); // the version string last fetched, "" if never checked

// Downloads the new firmware image to the SD card (UPDATE_FIRMWARE_FILENAME
// at root), staged for a later flash step. Does not touch flash/OTA at all.
// Returns true on success; check UPDATE_CHECK_lastError() otherwise.
extern bool UPDATE_CHECK_downloadToSD(void);
extern String UPDATE_CHECK_lastError(void);

// Flashes the staged firmware image from SD. Does NOT reboot - caller must
// do that (e.g. after sending an HTTP response) for the new image to run.
extern bool UPDATE_CHECK_applyFromSD(void);

#endif
