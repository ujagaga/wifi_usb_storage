#include "config.h"
#include "usb_msc.h"

#include <USB.h>
#include <USBMSC.h>
#include <SD_MMC.h>
#include <string.h>
#include "sd_storage.h"
#include "storage_mode.h"
#include "lcd_display.h"

static USBMSC msc;
static bool mscMediaPresent = false;

// Best-effort progress bar for USB writes - see MSC_PROGRESS_ASSUMED_BYTES/
// MSC_PROGRESS_IDLE_MS in config.h for why this can only ever be a rough
// activity indicator, not an accurate percentage.
static uint32_t mscBurstBytes = 0;
static uint32_t mscLastWriteMs = 0;
static bool mscProgressActive = false;

// TinyUSB requests are block-size-aligned in practice, but this handles an
// arbitrary offset/length by reading whole sectors and copying the
// requested window out of each - readRAW only ever reads one full sector.
static int32_t mscOnRead(uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize){
  uint8_t sector[512];
  uint32_t done = 0;
  while(done < bufsize){
    uint32_t sectorIdx = lba + (offset + done) / 512;
    uint32_t sectorOff = (offset + done) % 512;
    if(!SD_MMC.readRAW(sector, sectorIdx)){
      break;
    }
    uint32_t take = 512 - sectorOff;
    if(take > bufsize - done){
      take = bufsize - done;
    }
    memcpy((uint8_t*)buffer + done, sector + sectorOff, take);
    done += take;
  }
  return done;
}

// Only reachable when USB is the write master (see storage_mode.h) - TinyUSB
// rejects writes itself otherwise, based on the isWritable() flag below.
// writeRAW only writes a whole 512-byte sector at once, so a partial-sector
// write is a read-modify-write of that one sector.
static int32_t mscOnWrite(uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize){
  uint8_t sector[512];
  uint32_t done = 0;
  while(done < bufsize){
    uint32_t sectorIdx = lba + (offset + done) / 512;
    uint32_t sectorOff = (offset + done) % 512;
    uint32_t take = 512 - sectorOff;
    if(take > bufsize - done){
      take = bufsize - done;
    }
    if(take < 512 && !SD_MMC.readRAW(sector, sectorIdx)){
      break;
    }
    memcpy(sector + sectorOff, buffer + done, take);
    if(!SD_MMC.writeRAW(sector, sectorIdx)){
      break;
    }
    done += take;
  }

  if(done > 0){
    uint32_t now = millis();
    if(!mscProgressActive || (now - mscLastWriteMs) > MSC_PROGRESS_IDLE_MS){
      mscBurstBytes = 0;
      mscProgressActive = true;
    }
    mscBurstBytes += done;
    mscLastWriteMs = now;
    uint64_t percent = ((uint64_t)mscBurstBytes * 100) / MSC_PROGRESS_ASSUMED_BYTES;
    if(percent > 100){
      percent = 100;
    }
    LCD_showProgress((uint8_t)percent);
  }

  return done;
}

static bool mscOnStartStop(uint8_t power_condition, bool start, bool load_eject){
  return true;
}

void USB_MSC_init(void){
  msc.vendorID("WifiUSB");
  msc.productID("SD Card");
  msc.productRevision("1.0");
  msc.isWritable(!STORAGE_MODE_isWifiWriteMaster());
  msc.onRead(mscOnRead);
  msc.onWrite(mscOnWrite);
  msc.onStartStop(mscOnStartStop);
  msc.mediaPresent(false);
  USB.begin();
}

void USB_MSC_setWritable(bool writable){
  msc.isWritable(writable);
}

void USB_MSC_process(void){
  bool ready = SDSTOR_isReady();
  if(ready != mscMediaPresent){
    if(ready){
      msc.begin(SD_MMC.numSectors(), SD_MMC.sectorSize());
    }
    msc.mediaPresent(ready);
    mscMediaPresent = ready;
  }

  // No callback marks "the copy is done" at the block level, so a gap this
  // long since the last sector write is taken as the end of the transfer.
  if(mscProgressActive && (millis() - mscLastWriteMs) > MSC_PROGRESS_IDLE_MS){
    LCD_hideProgress();
    mscProgressActive = false;
  }
}
