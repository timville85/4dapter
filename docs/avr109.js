/*
 * avr109.js — minimal AVR109 ("Butterfly"/Caterina bootloader protocol)
 * client over the Web Serial API, plus an Intel HEX parser.
 *
 * Written fresh for the 4dapter web flasher rather than adapting an existing
 * tool, since nothing off-the-shelf speaks this protocol from a browser for
 * an avr109/Caterina-style board (most browser AVR flashers only cover
 * DFU/avr-dfu chips; avr109-over-serial is a different, much simpler, text
 * based protocol).
 *
 * Protocol reference: the AVR109 command set as implemented by Arduino's
 * Caterina bootloader (bootloaders/caterina/Caterina.c in ArduinoCore-avr)
 * and avrdude's avr109.c programmer driver. Every command is a single ASCII
 * byte, optionally followed by argument bytes; the bootloader replies with
 * either a single '\r' (0x0D) acknowledgement or specific data bytes, as
 * documented on each method below. If a command here doesn't behave as
 * documented against real hardware, cross-check against avrdude's avr109.c —
 * that implementation is the de facto spec.
 */

const CR = 0x0d;

export class Avr109Error extends Error {}

export class Avr109Client {
  constructor(port, { log = () => {} } = {}) {
    this.port = port;
    this.log = log;
    this.reader = null;
    this.writer = null;
    this._readBuffer = new Uint8Array(0);
  }

  async open(baudRate = 57600) {
    await this.port.open({ baudRate });
    this.writer = this.port.writable.getWriter();
    this.reader = this.port.readable.getReader();
  }

  async close() {
    try {
      if (this.reader) {
        await this.reader.cancel().catch(() => {});
        this.reader.releaseLock();
      }
    } catch {
      // Best-effort cleanup; nothing useful to do if this fails.
    }
    try {
      if (this.writer) this.writer.releaseLock();
    } catch {
      // Same as above.
    }
    try {
      await this.port.close();
    } catch {
      // Port may already be closed/disconnected (e.g. board reset itself).
    }
  }

  async _writeBytes(bytes) {
    this.log(`-> ${hex(bytes)}`);
    await this.writer.write(new Uint8Array(bytes));
  }

  /**
   * Reads exactly |count| bytes, buffering across multiple underlying reads.
   * Aborts (by cancelling the reader) if no full response arrives within
   * |timeoutMs|. After a timeout this client's reader is no longer usable —
   * callers should treat a timeout as fatal and close() the connection.
   */
  async _readBytes(count, timeoutMs = 3000) {
    while (this._readBuffer.length < count) {
      let timedOut = false;
      const timer = setTimeout(() => {
        timedOut = true;
        this.reader.cancel().catch(() => {});
      }, timeoutMs);

      let result;
      try {
        result = await this.reader.read();
      } finally {
        clearTimeout(timer);
      }

      if (timedOut) {
        throw new Avr109Error(`Timed out waiting for ${count} byte(s) (got ${this._readBuffer.length})`);
      }
      if (result.done) {
        throw new Avr109Error('Serial port closed while waiting for a response');
      }
      if (result.value && result.value.length) {
        const merged = new Uint8Array(this._readBuffer.length + result.value.length);
        merged.set(this._readBuffer, 0);
        merged.set(result.value, this._readBuffer.length);
        this._readBuffer = merged;
      }
    }
    const out = this._readBuffer.slice(0, count);
    this._readBuffer = this._readBuffer.slice(count);
    this.log(`<- ${hex(out)}`);
    return out;
  }

  async _expectCR(context) {
    const resp = await this._readBytes(1);
    if (resp[0] !== CR) {
      throw new Avr109Error(`${context}: expected CR (0x0d) acknowledgement, got 0x${resp[0].toString(16)}`);
    }
  }

  /** 'S' — software identifier: 7 ASCII chars (Caterina replies "CATERIN"). */
  async readSoftwareId() {
    await this._writeBytes([0x53]);
    return new TextDecoder().decode(await this._readBytes(7));
  }

  /** 's' — 3-byte device signature, sent least-significant byte first. */
  async readSignature() {
    await this._writeBytes([0x73]);
    const resp = await this._readBytes(3);
    return [resp[2], resp[1], resp[0]];
  }

  /** 'b' — block-transfer support query: 'Y' + 2-byte big-endian max block size. */
  async queryBlockSupport() {
    await this._writeBytes([0x62]);
    const resp = await this._readBytes(3);
    if (resp[0] !== 0x59 /* 'Y' */) {
      throw new Avr109Error('Bootloader did not report block-transfer support');
    }
    return (resp[1] << 8) | resp[2];
  }

  /** 'P' — enter programming mode. */
  async enterProgramMode() {
    await this._writeBytes([0x50]);
    await this._expectCR('enterProgramMode');
  }

  /** 'L' — leave programming mode. */
  async leaveProgramMode() {
    await this._writeBytes([0x4c]);
    await this._expectCR('leaveProgramMode');
  }

  /** 'A' — set the current memory address, given as a BYTE address (the wire protocol uses 16-bit words). */
  async setAddress(byteAddress) {
    const wordAddress = byteAddress >> 1;
    await this._writeBytes([0x41, (wordAddress >> 8) & 0xff, wordAddress & 0xff]);
    await this._expectCR('setAddress');
  }

  /** 'B' — write |data| to the given memory type (0x46 'F' flash / 0x45 'E' eeprom) at the last-set address. */
  async writeBlock(data, memType = 0x46) {
    const len = data.length;
    await this._writeBytes([0x42, (len >> 8) & 0xff, len & 0xff, memType, ...data]);
    await this._expectCR('writeBlock');
  }

  /** 'g' — read |length| bytes of the given memory type from the last-set address. */
  async readBlock(length, memType = 0x46) {
    await this._writeBytes([0x67, (length >> 8) & 0xff, length & 0xff, memType]);
    return await this._readBytes(length);
  }

  /**
   * 'E' — Caterina-specific extension (not in the base AVR109 spec) to exit
   * the bootloader and jump to the application immediately. If a bootloader
   * doesn't support this, it will fall back to its own inactivity timeout
   * (Caterina's is a few seconds) and the caller should treat a failure here
   * as non-fatal.
   */
  async exitBootloader() {
    await this._writeBytes([0x45]);
    await this._expectCR('exitBootloader');
  }
}

function hex(bytes) {
  return Array.from(bytes, (b) => b.toString(16).padStart(2, '0')).join(' ');
}

/**
 * Parses Intel HEX text (the .hex format avr-gcc/arduino-cli produce) into a
 * flat byte array covering address 0..maxAddress, with unwritten bytes filled
 * as 0xFF (the value flash reads as after an erase — matches what avrdude
 * writes for padding).
 */
export function parseIntelHex(hexText) {
  const lines = hexText.split(/\r?\n/).filter((l) => l.trim().length > 0);
  let extendedAddress = 0;
  let maxAddress = -1;
  const sparse = [];

  for (const line of lines) {
    if (!line.startsWith(':')) continue;

    const byteCount = parseInt(line.substr(1, 2), 16);
    const address = parseInt(line.substr(3, 4), 16);
    const recordType = parseInt(line.substr(7, 2), 16);
    const dataStart = 9;

    let checksum = byteCount + ((address >> 8) & 0xff) + (address & 0xff) + recordType;
    const dataBytes = new Array(byteCount);
    for (let i = 0; i < byteCount; i++) {
      const b = parseInt(line.substr(dataStart + i * 2, 2), 16);
      dataBytes[i] = b;
      checksum += b;
    }
    checksum += parseInt(line.substr(dataStart + byteCount * 2, 2), 16);
    if ((checksum & 0xff) !== 0) {
      throw new Error(`Intel HEX checksum mismatch on line: ${line}`);
    }

    if (recordType === 0x00) {
      // Data record.
      const base = extendedAddress + address;
      for (let i = 0; i < byteCount; i++) {
        const a = base + i;
        sparse[a] = dataBytes[i];
        if (a > maxAddress) maxAddress = a;
      }
    } else if (recordType === 0x01) {
      // End of file.
      break;
    } else if (recordType === 0x02) {
      // Extended segment address.
      extendedAddress = ((dataBytes[0] << 8) | dataBytes[1]) * 16;
    } else if (recordType === 0x04) {
      // Extended linear address.
      extendedAddress = (dataBytes[0] << 24) | (dataBytes[1] << 16) | (dataBytes[2] << 8) | dataBytes[3];
    }
    // Record types 03/05 (start segment/linear address) aren't emitted by
    // avr-gcc output and don't affect data placement; ignored.
  }

  if (maxAddress < 0) {
    throw new Error('No data records found in Intel HEX file');
  }

  const flat = new Uint8Array(maxAddress + 1).fill(0xff);
  for (let i = 0; i <= maxAddress; i++) {
    if (sparse[i] !== undefined) flat[i] = sparse[i];
  }
  return flat;
}

/**
 * Opens |port| at 1200 baud and immediately signals a hangup (DTR low), which
 * is the standard trick to make a Caterina-style bootloader reset into
 * programming mode. Only works if the currently-running sketch's USB core
 * still implements CDC serial — firmware built with CDC_DISABLED (the
 * 4-Player HID build) or a non-Arduino-core USB stack (Switch/XInput) won't
 * show up as a serial port at all, so this step doesn't apply to them; the
 * user has to press the board's physical reset button instead.
 */
export async function touchReset1200(port, { log = () => {} } = {}) {
  log('Touching port at 1200 baud to request a bootloader reset...');
  await port.open({ baudRate: 1200 });
  try {
    await port.setSignals({ dataTerminalReady: false });
  } catch (e) {
    log(`(setSignals not supported/failed: ${e.message} — continuing anyway)`);
  }
  await new Promise((r) => setTimeout(r, 250));
  await port.close();
  log('Port closed. Waiting for the board to re-enumerate in bootloader mode...');
  await new Promise((r) => setTimeout(r, 2000));
}

/**
 * Flashes |hexBytes| (from parseIntelHex) to |port|, which must already be
 * the board's bootloader-mode serial port. Writes page-by-page using the
 * bootloader's own reported page size, then reads every page back to verify.
 * Throws Avr109Error (or a plain Error from parseIntelHex) on any failure;
 * the port is always closed before returning, even on failure.
 */
export async function flashFirmware(port, hexBytes, { log = () => {}, onProgress = () => {} } = {}) {
  const client = new Avr109Client(port, { log });
  await client.open();
  try {
    log('Entering programming mode...');
    await client.enterProgramMode();

    const pageSize = await client.queryBlockSupport();
    log(`Bootloader reports a ${pageSize}-byte page size.`);
    if (pageSize <= 0 || pageSize > 1024) {
      throw new Avr109Error(`Unexpected page size (${pageSize}) reported by bootloader`);
    }

    const totalPages = Math.ceil(hexBytes.length / pageSize);

    for (let page = 0; page < totalPages; page++) {
      const start = page * pageSize;
      const padded = new Uint8Array(pageSize).fill(0xff);
      padded.set(hexBytes.subarray(start, start + pageSize), 0);

      await client.setAddress(start);
      await client.writeBlock(padded);
      onProgress({ phase: 'write', page: page + 1, totalPages });
    }

    log('Verifying...');
    for (let page = 0; page < totalPages; page++) {
      const start = page * pageSize;
      const expected = new Uint8Array(pageSize).fill(0xff);
      expected.set(hexBytes.subarray(start, start + pageSize), 0);

      await client.setAddress(start);
      const actual = await client.readBlock(pageSize);
      for (let i = 0; i < pageSize; i++) {
        if (actual[i] !== expected[i]) {
          throw new Avr109Error(
            `Verification failed at byte ${start + i}: wrote 0x${expected[i].toString(16)}, read back 0x${actual[i].toString(16)}`
          );
        }
      }
      onProgress({ phase: 'verify', page: page + 1, totalPages });
    }

    log('Leaving programming mode...');
    await client.leaveProgramMode();
    try {
      await client.exitBootloader();
      log('Told the bootloader to restart the application.');
    } catch (e) {
      log(`Bootloader didn't acknowledge the exit command (${e.message}) — it should still start the new firmware on its own after a few seconds, or you can press the reset button.`);
    }
  } finally {
    await client.close();
  }
}
