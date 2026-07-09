#line 1 "/home/rada/Projects/wifi_usb_storage/README.md"
# WiFi USB Storage

Firmware for a Waveshare **ESP32-C6-LCD-1.47** (ST7789, 172x320 IPS) that turns a
microSD card into WiFi-accessible file storage, with hot-plug SD card support
and a small on-device status display.

## Hardware

- Board: ESP32-C6-LCD-1.47 (172x320 ST7789 LCD)
- Storage: microSD (TF) card over SPI, sharing the bus with the LCD (separate
  chip-selects; see `config.h` for pin assignments)
- Build: Arduino core via `arduino-cli` (`tools/build.sh` / `tools/build.sh upload`),
  "Huge APP" partition scheme

## Features

### WiFi
- **Access Point always available.** SSID is `WiFi_Storage<MAC>` (see
  `AP_NAME_PREFIX`/`AP_PASS` in `config.h`), IP `192.168.4.1`. This is how you
  first reach the device's web UI.
- **Station mode** to join your own WiFi: pick a network from a scan and enter
  its password on the `/config` page. Credentials (and screen brightness) live
  in RAM; they're mirrored to a file on the SD card (if one is present) so
  they survive a reboot. Without a card, they're RAM-only and are lost on
  power-cycle.
- **Automatic AP power-down**: once station mode is connected and no AP
  clients have been seen for a while (`AP_AUTO_OFF_MS`), the AP is switched
  off to save power. It comes back automatically if the station connection
  drops.
- The on-device LCD shows the AP's SSID/password/IP at boot, then switches to
  the station SSID/IP once connected.

### SD card storage
- Files live in the SD card's root directory (no subfolders). Upload,
  download, delete, and list them from the web UI or the HTTP API.
- **Hot-plug.** The card doesn't need to be present at boot, and can be
  inserted or removed while the device is running - the web UI updates
  itself automatically (polls status every few seconds and reloads).
- **Fault detection.** The device distinguishes "no card inserted" from "a
  card is inserted but its filesystem can't be read" (corrupted/unformatted),
  using a raw SD hardware probe that's independent of the filesystem layer.
  The web UI shows which one it is.
- **Eject button.** Finishes any in-flight write and unmounts the card so it
  can be safely pulled out. The device won't try to remount it until it
  detects the card was actually removed, so clicking Eject while the card is
  still seated won't get silently undone.
- **Format button.** Wipes the card and lays down a fresh FAT filesystem -
  the fix for a "faulty" card. A confirmation dialog guards against
  accidental data loss.
- **Files larger than 4GiB.** FAT32 caps a single file at 4GiB-1, so any
  logical file that grows past `SD_PART_MAX_BYTES` (`config.h`, ~3.7GiB) is
  transparently split into hidden numbered part files plus a small manifest.
  This is invisible from the UI/API - uploads, downloads, listing, and
  deletion all work on the one logical file name and its true combined size.

### Web UI
- `/` - main page: station IP, file list with download/delete, upload
  (multi-file), Eject, and Format buttons. When no SD card is present, or
  it's faulty, this area is replaced with the relevant message instead.
- `/config` - WiFi network picker (scans, lets you pick an SSID and enter a
  password) and a screen brightness slider.
- `/api` - HTTP API reference page (see below).

### HTTP API
| Method | Endpoint | Description |
|---|---|---|
| GET | `/api/filelist` | List files as `name:size` (`\|`-separated) |
| GET | `/download?name=NAME` | Download a file |
| POST | `/upload` | Upload a file (multipart field `f`) |
| GET | `/delete?name=NAME` | Delete a file |
| GET | `/eject` | Finish pending writes, unmount the SD card |
| GET | `/format` | Erase the SD card and create a fresh filesystem |
| GET | `/api/sdstatus` | `ready`, `faulty`, or `missing` |
| GET | `/aplist` | Last WiFi scan result |
| GET | `/wifisave?s=SSID&p=PASS` | Save WiFi credentials, switch to station mode |

## Known limitations
- No subfolders - all files live flat in the SD card's root.
- Filenames: up to 64 characters, restricted to letters, digits, `. _ -` and
  space.
- No exFAT support (disabled in the Arduino core's bundled FatFs), so the
  card is always formatted FAT16/FAT32 - the >4GiB split-file scheme above is
  what makes large files possible despite that.
- Filenames starting with `.` are reserved for internal use (split-file parts
  and manifests) and won't appear in the file list if uploaded directly.
