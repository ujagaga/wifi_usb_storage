#ifndef USB_MSC_H
#define USB_MSC_H

// Exposes the SD card as a read-only USB mass storage device. Read-only
// means a USB host (e.g. a TV) can never write to the card, so it's safe to
// run at the same time as normal WiFi/web access to the same card - see
// sd_storage.cpp's periodic flush during uploads for how a growing file
// becomes visible to the host promptly.
extern void USB_MSC_init(void);
// Call every loop() iteration: mounts/unmounts the USB-visible media as the
// SD card's own ready state changes.
extern void USB_MSC_process(void);

#endif
