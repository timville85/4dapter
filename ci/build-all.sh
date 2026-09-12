#!/usr/bin/env bash
#
# Builds all 6 4dapter firmware variants with arduino-cli and collects the
# resulting .hex files in dist/. Used by both CI (.github/workflows/build-firmware.yml)
# and local development — run this after any firmware change to confirm every
# build target still compiles.
#
# Requires: arduino-cli on PATH (e.g. `brew install arduino-cli`).

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

LUFA_URL="https://github.com/CrazyRedMachine/Arduino-Lufa/raw/master/package_arduino-lufa_index.json"
XINPUT_URL="https://raw.githubusercontent.com/dmadison/ArduinoXInput_Boards/master/package_dmadison_xinput_index.json"

echo "==> Installing board cores and libraries"
arduino-cli core update-index
arduino-cli core install arduino:avr

arduino-cli core update-index --additional-urls "$LUFA_URL"
arduino-cli core install Arduino-LUFA:avr --additional-urls "$LUFA_URL"

arduino-cli core update-index --additional-urls "$XINPUT_URL"
arduino-cli core install xinput:avr --additional-urls "$XINPUT_URL"
arduino-cli lib install XInput

mkdir -p build dist
rm -f dist/*.hex

build_hid() {
  local outfile="$1" extra_flags="$2"
  shift 2
  echo "==> Building $outfile"
  arduino-cli compile \
    --fqbn arduino:avr:leonardo \
    --library ./lib/4dapterCore \
    --library ./lib/4dapterHID \
    --build-property "build.extra_flags=${extra_flags}" \
    --output-dir "build/${outfile}" \
    "$@" \
    4dapter_FW-HID
  cp "build/${outfile}/4dapter_FW-HID.ino.hex" "dist/${outfile}.hex"
}

build_hid "4dapter-hid"        "-DHID_LAYOUT=HID_LAYOUT_TRIPLE"
build_hid "4dapter-hid-alt"    "-DHID_LAYOUT=HID_LAYOUT_TRIPLE_ALT"
build_hid "4dapter-hid-single" "-DHID_LAYOUT=HID_LAYOUT_SINGLE"
build_hid "4dapter-hid-4p"     "-DHID_LAYOUT=HID_LAYOUT_QUAD" \
  --build-property "compiler.c.extra_flags=-DCDC_DISABLED" \
  --build-property "compiler.cpp.extra_flags=-DCDC_DISABLED"

echo "==> Building 4dapter-switch"
arduino-cli compile --fqbn Arduino-LUFA:avr:leonardo --library ./lib/4dapterCore \
  --output-dir build/4dapter-switch 4dapter_FW-Switch
cp build/4dapter-switch/4dapter_FW-Switch.ino.hex dist/4dapter-switch.hex

echo "==> Building 4dapter-xinput"
arduino-cli compile --fqbn xinput:avr:leonardo --library ./lib/4dapterCore \
  --output-dir build/4dapter-xinput 4dapter_FW-XInput
cp build/4dapter-xinput/4dapter_FW-XInput.ino.hex dist/4dapter-xinput.hex

echo "==> Done. Artifacts in dist/:"
ls -la dist/
