/*
 * flasher.js — UI glue for the 4dapter web flasher.
 *
 * Talks to GitHub Releases to find the firmware .hex files, drives the
 * avr109.js WebSerial client, and renders progress/log into the page. Kept
 * deliberately framework-free (plain DOM) since this is a small, standalone
 * static page.
 */

import { parseIntelHex, touchReset1200, flashFirmware } from './avr109.js';

const GITHUB_REPO = 'timville85/4dapter';

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
  normalPort: null,
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
  el('autoreset-hint').hidden = !state.variant.autoReset;
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

async function connectNormalPort() {
  setStatus('Requesting your 4dapter’s serial port...', 'busy');
  try {
    state.normalPort = await navigator.serial.requestPort();
    logLine('Selected the board’s normal-mode serial port.');
    await touchReset1200(state.normalPort, { log: logLine });
    setStatus('Board should now be in bootloader mode — select it in step 3.', 'ok');
  } catch (e) {
    setStatus(`Couldn't connect: ${e.message}`, 'error');
    logLine(`Error: ${e.message}`);
  }
}

function skipToManualReset() {
  setStatus('Press the reset button on your 4dapter now, then select it in step 3.', '');
}

async function connectBootloaderPort() {
  setStatus('Requesting the bootloader-mode device...', 'busy');
  try {
    state.bootloaderPort = await navigator.serial.requestPort();
  } catch (e) {
    setStatus(`Couldn't connect: ${e.message}`, 'error');
    logLine(`Error: ${e.message}`);
    return;
  }
  // Flash immediately — the bootloader only waits a few seconds for activity
  // before giving up and running the old firmware, so there's no "click
  // Flash when you're ready" step here.
  logLine('Selected a device — flashing now.');
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

function init() {
  renderVariantPicker();
  const supported = checkWebSerialSupport();

  el('connect-normal-button').addEventListener('click', connectNormalPort);
  el('skip-to-manual-button').addEventListener('click', skipToManualReset);
  el('connect-bootloader-button').addEventListener('click', connectBootloaderPort);
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
