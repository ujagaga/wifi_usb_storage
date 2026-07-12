#!/usr/bin/env bash
#
# Build (and optionally flash) the ESP32-C6 clock firmware.
#
#   tools/build.sh            compile only
#   tools/build.sh upload     compile, then flash
#
# Overrides via env:
#   FQBN=esp32:esp32:esp32c6   target board
#   PORT=/dev/ttyACM0          serial port for upload
#
# Uses the "No FS (2MB APP x2)" partition scheme: two OTA app slots (app0/app1),
# no filesystem partition (storage is the SD card, not SPIFFS/FFat). This is
# what makes firmware updates possible - Update.h flips between app0/app1, so
# a bad/interrupted update still leaves the other slot bootable. In the
# Arduino IDE, set Tools -> Partition Scheme -> "No FS 4MB (2MB APP x2)" to match.
# Switching schemes between builds requires a full reflash.
set -euo pipefail

SKETCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$SKETCH_DIR/build"
CACHE_DIR="$SKETCH_DIR/.cache"
FQBN="${FQBN:-esp32:esp32:esp32c6:PartitionScheme=no_fs}"
PORT="${PORT:-/dev/ttyACM0}"

command -v arduino-cli >/dev/null 2>&1 || {
  echo "arduino-cli not found on PATH. Install: https://arduino.github.io/arduino-cli/latest/installation/" >&2
  exit 1
}

echo "Compiling $SKETCH_DIR for $FQBN ..."
arduino-cli compile --fqbn "$FQBN" --output-dir "$BUILD_DIR" --build-path "$CACHE_DIR" "$SKETCH_DIR"

# Keep build/version.txt and the firmware's .md5 in sync with this build,
# so update_check.cpp's version/MD5 checks always match what's actually
# committed - no manual bookkeeping required.
VERSION=$(grep -oP '(?<=#define VERSION ")[^"]+' "$SKETCH_DIR/config.h")
echo "$VERSION" > "$BUILD_DIR/version.txt"
md5sum "$BUILD_DIR/wifi_usb_storage.ino.bin" | awk '{print $1}' > "$BUILD_DIR/wifi_usb_storage.ino.bin.md5"
echo "Updated build/version.txt ($VERSION) and build/wifi_usb_storage.ino.bin.md5"

if [ "${1:-}" = "upload" ]; then
  echo "Uploading to $PORT ..."
  arduino-cli upload --fqbn "$FQBN" -p "$PORT" --input-dir "$BUILD_DIR" "$SKETCH_DIR"
fi
