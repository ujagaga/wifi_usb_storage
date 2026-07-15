#include "config.h"
#include "usb_msc.h"

#include <USB.h>
#include <USBMSC.h>
#include <SD_MMC.h>
#include <string.h>
#include "sd_storage.h"

static USBMSC msc;
static bool mscMediaPresent = false;

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

// Never expected to run - isWritable(false) below makes TinyUSB reject
// writes before this is called - but registered defensively rather than
// left null.
static int32_t mscOnWrite(uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize){
  return bufsize;
}

static bool mscOnStartStop(uint8_t power_condition, bool start, bool load_eject){
  return true;
}

void USB_MSC_init(void){
  msc.vendorID("WifiUSB");
  msc.productID("SD Card");
  msc.productRevision("1.0");
  msc.isWritable(false);
  msc.onRead(mscOnRead);
  msc.onWrite(mscOnWrite);
  msc.onStartStop(mscOnStartStop);
  msc.mediaPresent(false);
  USB.begin();
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
}
