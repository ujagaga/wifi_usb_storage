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
# Uses the "Huge APP" partition scheme (3MB app, no OTA): the firmware is large
# and we flash over serial, so OTA/SPIFFS partitions are not needed. In the
# Arduino IDE, set Tools -> Partition Scheme -> "Huge APP (3MB No OTA/1MB SPIFFS)"
# to match. Switching schemes between builds requires a full reflash.
set -euo pipefail

SKETCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$SKETCH_DIR/build"
CACHE_DIR="$SKETCH_DIR/.cache"
FQBN="${FQBN:-esp32:esp32:esp32c6:PartitionScheme=huge_app}"
PORT="${PORT:-/dev/ttyACM0}"

command -v arduino-cli >/dev/null 2>&1 || {
  echo "arduino-cli not found on PATH. Install: https://arduino.github.io/arduino-cli/latest/installation/" >&2
  exit 1
}

echo "Compiling $SKETCH_DIR for $FQBN ..."
arduino-cli compile --fqbn "$FQBN" --output-dir "$BUILD_DIR" --build-path "$CACHE_DIR" "$SKETCH_DIR"

if [ "${1:-}" = "upload" ]; then
  echo "Uploading to $PORT ..."
  arduino-cli upload --fqbn "$FQBN" -p "$PORT" --input-dir "$BUILD_DIR" "$SKETCH_DIR"
fi
