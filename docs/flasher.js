/*
 * flasher.js — UI glue for the 4dapter web flasher.
 *
 * Talks to GitHub Releases to find the firmware .hex files, drives the
 * avr109.js WebSerial client, and renders progress/log into the page. Kept
 * deliberately framework-free (plain DOM) since this is a small, standalone
 * static page.
 */

import { parseIntelHex, touchReset1200, flashFirmware, probeBootloader } from './avr109.js';

const GITHUB_REPO = 'timville85/4dapter';

// Narrows the browser's device picker to Arduino-vendor USB devices, so
// unrelated ports (Bluetooth, other USB-serial gadgets, monitor controls,
// etc.) don't clutter the list. Confirmed on real hardware: normal-mode
// firmware enumerates as VID 0x2341 (Arduino) PID 0x8036 ("Arduino
// Leonardo"), and the Caterina bootloader as VID 0x2341 PID 0x0037 ("Arduino
// Micro" — not the Leonardo bootloader ID one might expect). Filtering on
// vendor ID only (no product ID) covers both without hardcoding product IDs
// that might differ on a future hardware revision.
const ARDUINO_VENDOR_FILTER = [{ usbVendorId: 0x2341 }];

// Keep in sync with the filenames ci/build-all.sh produces and
// .github/workflows/build-firmware.yml publishes as release assets.
const VARIANTS = [
  {
    id: '4dapter-hid',
    file: '4dapter-hid.hex',
    label: 'HID — MiSTer / PC / RetroArch',
    detail: '3 gamepads: NES+SNES combined, Genesis, N64. The default build.',
    autoReset: true,
  },
  {
    id: '4dapter-hid-alt',
    file: '4dapter-hid-alt.hex',
    label: 'HID-ALT — MiSTer / PC / RetroArch',
    detail: '3 gamepads: NES, SNES, Genesis+N64 combined.',
    autoReset: true,
  },
  {
    id: '4dapter-hid-single',
    file: '4dapter-hid-single.hex',
    label: 'HID-Single — Batocera',
    detail: '1 gamepad: all four controllers combined onto a single report.',
    autoReset: true,
  },
  {
    id: '4dapter-hid-4p',
    file: '4dapter-hid-4p.hex',
    label: '4-Player HID — MiSTer',
    detail: '4 separate gamepads, one per port.',
    autoReset: false,
    note: 'On Windows, only 3 of the 4 controllers may be recognized — this is a known Windows USB limitation (not something this flasher, or the firmware, can fix).',
  },
  {
    id: '4dapter-switch',
    file: '4dapter-switch.hex',
    label: 'Nintendo Switch',
    detail: 'Single Switch-compatible controller.',
    autoReset: false,
  },
  {
    id: '4dapter-xinput',
    file: '4dapter-xinput.hex',
    label: 'XInput — Analogue Pocket Dock',
    detail: 'Single XInput controller.',
    autoReset: false,
  },
];

const el = (id) => document.getElementById(id);

const state = {
  variant: VARIANTS[0],
  hexBytes: null,
  usingLocalFile: false,
  releaseTag: null,
  bootloaderPort: null,
};

function logLine(text) {
  const logEl = el('log');
  const line = document.createElement('div');
  line.textContent = text;
  logEl.appendChild(line);
  logEl.scrollTop = logEl.scrollHeight;
}

function setStatus(text, kind = '') {
  const statusEl = el('status');
  statusEl.textContent = text;
  statusEl.className = 'status' + (kind ? ' status--' + kind : '');
}

function renderVariantPicker() {
  const select = el('variant-select');
  select.innerHTML = '';
  for (const variant of VARIANTS) {
    const option = document.createElement('option');
    option.value = variant.id;
    option.textContent = variant.label;
    select.appendChild(option);
  }
  select.addEventListener('change', () => {
    state.variant = VARIANTS.find((v) => v.id === select.value);
    renderVariantDetail();
  });
  renderVariantDetail();
}

function renderVariantDetail() {
  el('variant-detail').textContent = state.variant.detail;
  const noteEl = el('variant-note');
  if (state.variant.note) {
    noteEl.textContent = state.variant.note;
    noteEl.hidden = false;
  } else {
    noteEl.hidden = true;
  }
  el('manualreset-hint').hidden = state.variant.autoReset;
  // A new variant invalidates anything already fetched/flashed so far.
  state.hexBytes = null;
  state.usingLocalFile = false;
  // Fetch it now, in the background, so it's already in memory by the time
  // the bootloader connects — the bootloader only waits a few seconds before
  // giving up and running the old firmware, so there's no time to spend on a
  // network round-trip once flashing is supposed to start.
  prefetchFirmware();
}

function prefetchFirmware() {
  ensureFirmwareFetched().catch((e) => {
    logLine(`Note: couldn't pre-fetch firmware yet (${e.message})`);
  });
}

async function ensureFirmwareFetched() {
  if (state.hexBytes) return;
  // Snapshot which variant this fetch is for, and bail out quietly if the
  // user switches variants or loads a local file while it's in flight,
  // rather than clobbering whatever they picked instead.
  const requestedVariant = state.variant;
  const stillWanted = () => state.variant === requestedVariant && !state.usingLocalFile;

  setStatus('Looking up the latest release...', 'busy');
  const res = await fetch(`https://api.github.com/repos/${GITHUB_REPO}/releases/latest`);
  if (!res.ok) {
    throw new Error(
      `Couldn't find a published release (GitHub API returned ${res.status}). ` +
        `Use "Load a .hex file instead" below if you have one locally.`
    );
  }
  const release = await res.json();
  if (!stillWanted()) return;

  const asset = (release.assets || []).find((a) => a.name === requestedVariant.file);
  if (!asset) {
    throw new Error(
      `Release ${release.tag_name} doesn't include ${requestedVariant.file}. ` +
        `Use "Load a .hex file instead" below if you have one locally.`
    );
  }

  setStatus(`Downloading ${requestedVariant.file} from release ${release.tag_name}...`, 'busy');
  const hexRes = await fetch(asset.browser_download_url);
  if (!hexRes.ok) {
    throw new Error(`Failed to download ${requestedVariant.file} (HTTP ${hexRes.status})`);
  }
  const hexText = await hexRes.text();
  if (!stillWanted()) return;
  state.releaseTag = release.tag_name;
  state.hexBytes = parseIntelHex(hexText);
  setStatus(`Ready: ${requestedVariant.file} from release ${release.tag_name} (${state.hexBytes.length} bytes).`, 'ok');
}

async function loadLocalHexFile(file) {
  const hexText = await file.text();
  state.hexBytes = parseIntelHex(hexText);
  state.usingLocalFile = true;
  state.releaseTag = `local file: ${file.name}`;
  setStatus(`Ready: ${file.name} (${state.hexBytes.length} bytes).`, 'ok');
}

function checkWebSerialSupport() {
  if (!('serial' in navigator)) {
    el('unsupported-banner').hidden = false;
    document.querySelectorAll('button[data-requires-serial]').forEach((b) => (b.disabled = true));
    return false;
  }
  return true;
}

// Holds a cleanup function for a pending navigator.serial 'connect' listener
// set up by listenForAutoReconnect(), if one is currently active.
let autoReconnectCleanup = null;

function stopListeningForAutoReconnect() {
  if (autoReconnectCleanup) {
    autoReconnectCleanup();
    autoReconnectCleanup = null;
  }
}

/**
 * Listens for navigator.serial's 'connect' event, which fires WITHOUT any
 * user gesture when a device the user has granted access to before
 * reconnects — this is the one legitimate way to reconnect a device
 * automatically after it changes USB identity on reset, since Chrome flatly
 * refuses to show a second requestPort() picker from the same click no
 * matter how that's timed (confirmed on real hardware; it's a "one chooser
 * per gesture" rule, not a race against a delay). Only fires at all if this
 * exact bootloader identity was already authorized on an earlier flash in
 * this browser — otherwise it never fires and the manual button is what
 * actually gets used, which is fine since it's shown at the same time.
 */
function listenForAutoReconnect(onFound) {
  stopListeningForAutoReconnect();
  const handler = (event) => {
    stopListeningForAutoReconnect();
    onFound(event.target);
  };
  navigator.serial.addEventListener('connect', handler);
  autoReconnectCleanup = () => navigator.serial.removeEventListener('connect', handler);
}

/**
 * Single entry point for connecting: works whether the board is currently
 * running normal (CDC-capable) firmware, is already sitting in bootloader
 * mode (e.g. the user pressed reset before clicking), or is running
 * CDC-less firmware and was just manually reset. We never ask the user which
 * case applies — we ask for a device, then probe it with the real protocol
 * to find out, and only fall back to a second manual pick if the browser
 * won't let us silently reconnect after triggering a reset.
 */
async function connectAndFlash() {
  el('manual-bootloader-button').hidden = true;
  stopListeningForAutoReconnect();

  let port;
  setStatus('Requesting your 4dapter...', 'busy');
  try {
    port = await navigator.serial.requestPort({ filters: ARDUINO_VENDOR_FILTER });
  } catch (e) {
    setStatus(`Couldn't connect: ${e.message}`, 'error');
    logLine(`Error: ${e.message}`);
    return;
  }

  setStatus('Checking whether it’s already in bootloader mode...', 'busy');
  const alreadyBootloader = await probeBootloader(port, { log: logLine });

  if (alreadyBootloader) {
    logLine('Already in bootloader mode — flashing now.');
    state.bootloaderPort = port;
    el('flash-button').disabled = false;
    await doFlash();
    return;
  }

  logLine('Not a bootloader yet — this looks like the normal running firmware. Resetting it...');
  setStatus('Resetting your board...', 'busy');
  try {
    await touchReset1200(port, { log: logLine });
  } catch (e) {
    setStatus(`Couldn't reset the board: ${e.message}`, 'error');
    logLine(`Error: ${e.message}`);
    return;
  }

  // Two paths race from here, whichever happens first wins:
  //  1. The board reconnecting fires a 'connect' event with zero clicks
  //     needed, IF this exact bootloader identity was already authorized on
  //     an earlier flash in this browser.
  //  2. The user clicks the fallback button, which Chrome always allows
  //     (it's a fresh click) but always requires when path 1 doesn't apply
  //     (first time ever authorizing this board here) — so it's shown
  //     immediately rather than waiting to see if path 1 pans out.
  setStatus('Board resetting — click below now to select it. (If this board was flashed here before, it may connect automatically instead.)', 'busy');
  logLine('Listening for an automatic reconnect, and showing the manual fallback at the same time...');
  el('manual-bootloader-button').hidden = false;
  el('manual-bootloader-button').focus();

  listenForAutoReconnect(async (autoPort) => {
    el('manual-bootloader-button').hidden = true;
    logLine('Board reconnected automatically — checking it...');
    const ready = await probeBootloader(autoPort, { log: logLine });
    if (!ready) {
      logLine('Not answering yet — click the button below to try again.');
      el('manual-bootloader-button').hidden = false;
      return;
    }
    state.bootloaderPort = autoPort;
    el('flash-button').disabled = false;
    await doFlash();
  });
}

async function connectBootloaderPortManually() {
  stopListeningForAutoReconnect();
  setStatus('Requesting the bootloader-mode device...', 'busy');
  let port;
  try {
    port = await navigator.serial.requestPort({ filters: ARDUINO_VENDOR_FILTER });
  } catch (e) {
    setStatus(`Couldn't connect: ${e.message}`, 'error');
    logLine(`Error: ${e.message}`);
    return;
  }

  // Guard against picking a device that isn't actually the bootloader yet —
  // e.g. clicking too fast, or accidentally selecting the old normal-mode
  // entry before it disappeared — with a clear message instead of a
  // confusing low-level protocol timeout later.
  const ready = await probeBootloader(port, { log: logLine });
  if (!ready) {
    setStatus("That doesn't look like the bootloader yet — click the button again and pick the newly-appeared device.", 'error');
    return;
  }

  el('manual-bootloader-button').hidden = true;
  logLine('Selected a device — flashing now.');
  state.bootloaderPort = port;
  el('flash-button').disabled = false;
  await doFlash();
}

async function doFlash() {
  el('flash-button').disabled = true;
  const progressEl = el('progress-bar');
  progressEl.hidden = false;

  try {
    // Normally already resolved by the background prefetch kicked off when
    // the variant was chosen; this only actually waits if that hasn't
    // finished yet (e.g. a slow connection) or failed and needs retrying.
    await ensureFirmwareFetched();
  } catch (e) {
    setStatus(e.message, 'error');
    el('flash-button').disabled = false;
    return;
  }

  try {
    setStatus('Flashing...', 'busy');
    await flashFirmware(state.bootloaderPort, state.hexBytes, {
      log: logLine,
      onProgress: ({ phase, page, totalPages }) => {
        const pct = Math.round((page / totalPages) * 100);
        progressEl.value = phase === 'write' ? pct / 2 : 50 + pct / 2;
        setStatus(`${phase === 'write' ? 'Writing' : 'Verifying'}: page ${page}/${totalPages}`, 'busy');
      },
    });
    progressEl.value = 100;
    setStatus('Done! Your 4dapter is now running the new firmware.', 'ok');
  } catch (e) {
    setStatus(`Flashing failed: ${e.message}`, 'error');
    logLine(`Error: ${e.message}`);
    el('flash-button').disabled = false;
  }
}

// ---------------------------------------------------------------------------
// Step 5: a simple, friendly "is it working" check — just watches for any
// button press or stick movement on any connected gamepad via the Gamepad
// API and reports back in plain language. No VID/PID or raw report data;
// that level of detail lives in diagnostics.html for when we need to
// actually troubleshoot something, not in the everyday flashing flow.
// ---------------------------------------------------------------------------

function watchForAnyInput() {
  const statusEl = el('quick-test-status');
  const tick = () => {
    const pads = navigator.getGamepads ? navigator.getGamepads() : [];
    const connected = Array.from(pads).filter(Boolean);
    const anyInput = connected.some(
      (pad) => pad.buttons.some((b) => b.pressed) || pad.axes.some((a) => Math.abs(a) > 0.3)
    );
    if (anyInput) {
      statusEl.textContent = '✅ Got it — your 4dapter is responding!';
      statusEl.className = 'status status--ok';
    } else if (connected.length > 0) {
      statusEl.textContent = 'Device detected — press any button to confirm it responds.';
      statusEl.className = 'status';
    } else {
      statusEl.textContent = 'Plug in your 4dapter and press a button.';
      statusEl.className = 'status';
    }
    requestAnimationFrame(tick);
  };
  tick();
}

function init() {
  renderVariantPicker();
  const supported = checkWebSerialSupport();
  watchForAnyInput();

  el('connect-button').addEventListener('click', connectAndFlash);
  el('manual-bootloader-button').addEventListener('click', connectBootloaderPortManually);
  el('flash-button').addEventListener('click', doFlash);

  el('local-hex-input').addEventListener('change', async (evt) => {
    const file = evt.target.files[0];
    if (!file) return;
    try {
      await loadLocalHexFile(file);
    } catch (e) {
      setStatus(`Couldn't read that file: ${e.message}`, 'error');
    }
  });

  if (supported) {
    setStatus('Pick a firmware variant, then connect your 4dapter.', '');
  }
}

document.addEventListener('DOMContentLoaded', init);
