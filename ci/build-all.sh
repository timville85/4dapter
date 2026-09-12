#!/usr/bin/env bash
#
# Builds all 6 4dapter firmware variants with arduino-cli and collects the
# resulting .hex files in dist/. Used by both CI (.github/workflows/build-firmware.yml)
# and local development — run this after any firmware change to confirm every
# build target still compiles.
#
# Requires: arduino-cli on PATH (e.g. `brew install arduino-cli`).
#
# IMPORTANT: sketch-level defines (HID_LAYOUT, etc.) must be passed via
# compiler.c.extra_flags/compiler.cpp.extra_flags, NEVER build.extra_flags.
# Both the stock arduino:avr and xinput:avr Leonardo board profiles set
# `leonardo.build.extra_flags={build.usb_flags}` in their own boards.txt,
# which supplies -DUSB_VID/-DUSB_PID/-DUSB_MANUFACTURER/-DUSB_PRODUCT.
# Overriding build.extra_flags here clobbers that entirely, and the board
# fails to compile at all (USB_VID undeclared) — arduino-cli's build cache
# can mask this locally by silently reusing a core object file compiled
# before the override was introduced, so this only surfaces as a hard
# failure on a genuinely clean build (like a fresh CI runner, or --clean).
# --clean is used on every build below specifically so this class of bug
# can never hide behind a stale cache again.

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
  local outfile="$1" defines="$2"
  echo "==> Building $outfile"
  arduino-cli compile \
    --fqbn arduino:avr:leonardo \
    --library ./lib/4dapterCore \
    --library ./lib/4dapterHID \
    --build-property "compiler.c.extra_flags=${defines}" \
    --build-property "compiler.cpp.extra_flags=${defines}" \
    --clean \
    --output-dir "build/${outfile}" \
    4dapter_FW-HID
  cp "build/${outfile}/4dapter_FW-HID.ino.hex" "dist/${outfile}.hex"
}

build_hid "4dapter-hid"        "-DHID_LAYOUT=HID_LAYOUT_TRIPLE"
build_hid "4dapter-hid-alt"    "-DHID_LAYOUT=HID_LAYOUT_TRIPLE_ALT"
build_hid "4dapter-hid-single" "-DHID_LAYOUT=HID_LAYOUT_SINGLE"
build_hid "4dapter-hid-4p"     "-DHID_LAYOUT=HID_LAYOUT_QUAD -DCDC_DISABLED"

echo "==> Building 4dapter-switch"
arduino-cli compile --fqbn Arduino-LUFA:avr:leonardo --library ./lib/4dapterCore \
  --clean \
  --output-dir build/4dapter-switch 4dapter_FW-Switch
cp build/4dapter-switch/4dapter_FW-Switch.ino.hex dist/4dapter-switch.hex

echo "==> Building 4dapter-xinput"
# The xinput:avr core's boards.txt hardcodes build.usb_product="Arduino Leonardo"
# (it's a repurposed stock Leonardo board profile) — override it so the device
# reports as "4dapter" instead of "Arduino Leonardo" while still spoofing an
# Xbox 360 controller's VID/PID (0x045E/0x028E, untouched) for XInput compatibility.
# Unlike HID_LAYOUT above, this is safe to pass via --build-property directly
# since build.usb_product is its own property, not build.extra_flags itself —
# but it still requires --clean, since arduino-cli's cache doesn't reliably
# notice this property changed either.
arduino-cli compile --fqbn xinput:avr:leonardo --library ./lib/4dapterCore \
  --build-property 'build.usb_product="4dapter"' \
  --clean \
  --output-dir build/4dapter-xinput 4dapter_FW-XInput
cp build/4dapter-xinput/4dapter_FW-XInput.ino.hex dist/4dapter-xinput.hex

echo "==> Done. Artifacts in dist/:"
ls -la dist/
