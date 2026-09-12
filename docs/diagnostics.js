/*
 * diagnostics.js — raw device-inspection tools, split out from the main
 * flasher flow so a first-time user isn't confronted with VID/PID/HID report
 * structure just to flash their board. Kept for troubleshooting: this is
 * exactly what answered "why isn't it appearing as XInput" during
 * development, by showing the real reported identity instead of just
 * "nothing detected."
 */

const el = (id) => document.getElementById(id);

function addMonoEntry(panelEl, lines) {
  const entry = document.createElement('div');
  entry.className = 'entry';
  for (const line of lines) {
    const row = document.createElement('div');
    if (Array.isArray(line)) {
      const strong = document.createElement('strong');
      strong.textContent = line[0];
      row.appendChild(strong);
      row.appendChild(document.createTextNode(line[1]));
    } else {
      row.textContent = line;
    }
    entry.appendChild(row);
  }
  panelEl.appendChild(entry);
  return entry;
}

// ---------------------------------------------------------------------------
// Gamepad API: what a game/emulator actually sees. Only recognizes known
// controller descriptors (e.g. the XInput build's Xbox 360 spoof).
// ---------------------------------------------------------------------------

let gamepadWatchRafId = null;

function renderGamepadReadout() {
  const panelEl = el('gamepad-readout');
  panelEl.innerHTML = '';
  const pads = navigator.getGamepads ? navigator.getGamepads() : [];
  const connected = Array.from(pads).filter(Boolean);
  if (connected.length === 0) {
    addMonoEntry(panelEl, ['No gamepad detected yet.']);
    return;
  }
  for (const pad of connected) {
    const buttons = pad.buttons.map((b, i) => `${i}:${b.pressed ? '●' : '○'}`).join(' ');
    const axes = pad.axes.map((a, i) => `${i}:${a.toFixed(2)}`).join(' ');
    addMonoEntry(panelEl, [
      ['id: ', pad.id],
      `index ${pad.index} · mapping: ${pad.mapping || '(none)'}`,
      `Buttons: ${buttons}`,
      `Axes: ${axes}`,
    ]);
  }
}

function toggleGamepadWatch() {
  const button = el('gamepad-watch-button');
  if (gamepadWatchRafId) {
    cancelAnimationFrame(gamepadWatchRafId);
    gamepadWatchRafId = null;
    button.textContent = 'Start watching for a gamepad';
    return;
  }
  button.textContent = 'Stop watching';
  const tick = () => {
    renderGamepadReadout();
    gamepadWatchRafId = requestAnimationFrame(tick);
  };
  tick();
}

// ---------------------------------------------------------------------------
// WebHID: raw USB identity and report structure, regardless of whether
// Chrome's gamepad code recognizes the device as a gamepad at all.
// ---------------------------------------------------------------------------

function checkWebHidSupport() {
  if (!('hid' in navigator)) {
    document.querySelectorAll('button[data-requires-webhid]').forEach((b) => (b.disabled = true));
    return false;
  }
  return true;
}

async function inspectViaWebHid() {
  const panelEl = el('webhid-readout');
  let devices;
  try {
    // Deliberately unfiltered: the whole point is to see whatever the
    // device actually reports, even if it's not the VID/PID we expected.
    devices = await navigator.hid.requestDevice({ filters: [] });
  } catch (e) {
    addMonoEntry(panelEl, [`Error: ${e.message}`]);
    return;
  }
  if (!devices || devices.length === 0) {
    addMonoEntry(panelEl, ['No device selected.']);
    return;
  }

  const device = devices[0];
  try {
    await device.open();
    const lines = [
      ['Vendor ID: ', `0x${device.vendorId.toString(16).padStart(4, '0')}`],
      ['Product ID: ', `0x${device.productId.toString(16).padStart(4, '0')}`],
      ['Product name: ', device.productName || '(none reported)'],
    ];
    for (const collection of device.collections) {
      lines.push(`Collection — usage page 0x${collection.usagePage.toString(16)}, usage 0x${collection.usage.toString(16)}`);
      for (const report of collection.inputReports) {
        lines.push(`  Input report ${report.reportId ?? '(none)'}: ${report.items.length} item(s)`);
      }
      for (const report of collection.outputReports) {
        lines.push(`  Output report ${report.reportId ?? '(none)'}: ${report.items.length} item(s)`);
      }
    }
    addMonoEntry(panelEl, lines);
  } catch (e) {
    addMonoEntry(panelEl, [`Error reading device info: ${e.message}`]);
  } finally {
    try {
      await device.close();
    } catch {
      // Nothing useful to do if closing fails.
    }
  }
}

function checkWebSerialSupport() {
  // This page doesn't use Web Serial itself, but the shared unsupported-banner
  // is worth surfacing if neither API this page relies on is present.
  return 'hid' in navigator || 'getGamepads' in navigator;
}

function init() {
  if (!checkWebSerialSupport()) {
    el('unsupported-banner').hidden = false;
  }
  checkWebHidSupport();

  el('gamepad-watch-button').addEventListener('click', toggleGamepadWatch);
  el('webhid-inspect-button').addEventListener('click', inspectViaWebHid);
}

document.addEventListener('DOMContentLoaded', init);
