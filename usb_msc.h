#ifndef USB_MSC_H
#define USB_MSC_H

// Exposes the SD card as a USB mass storage device. Whether the USB host can
// write is controlled by storage_mode.h (STORAGE_MODE_isWifiWriteMaster()) -
// only one side is ever writable at a time, the other stays read-only. When
// USB is read-only, it's safe to run at the same time as normal WiFi/web
// writes to the same card - see sd_storage.cpp's periodic flush during
// uploads for how a growing file becomes visible to the host promptly. When
// USB is the write master instead, WiFi-side writes are rejected (see
// storage_mode.h's use in sd_storage.cpp) so there's still only one writer.
extern void USB_MSC_init(void);
// Call every loop() iteration: mounts/unmounts the USB-visible media as the
// SD card's own ready state changes.
extern void USB_MSC_process(void);
// Sets the USB-visible media's write-protect flag. Called by storage_mode.cpp
// when the write master is switched; not meant to be called directly.
extern void USB_MSC_setWritable(bool writable);

#endif
