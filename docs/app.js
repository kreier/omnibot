(() => {
  const themeToggleBtn = document.getElementById("themeToggle");
  const macInput = document.getElementById("macInput");
  const cliCmdEl = document.getElementById("cliCommand");
  const logEl = document.getElementById("ds3Log");
  const serialBanner = document.getElementById("serialSupportBanner");
  const binaryCheckEl = document.getElementById("binaryCheck");

  const btnWebUsbRead = document.getElementById("btnWebUsbRead");
  const btnWebUsbWrite = document.getElementById("btnWebUsbWrite");
  const btnWebHidRead = document.getElementById("btnWebHidRead");
  const btnWebHidWrite = document.getElementById("btnWebHidWrite");

  function applyTheme(theme) {
    document.documentElement.setAttribute("data-theme", theme);
    if (themeToggleBtn) {
      const isLight = theme === "light";
      themeToggleBtn.textContent = isLight ? "Dark mode" : "Light mode";
      themeToggleBtn.setAttribute("aria-pressed", String(isLight));
    }
    try {
      localStorage.setItem("omnibot-theme", theme);
    } catch (_) {}
  }

  const initialTheme =
    document.documentElement.getAttribute("data-theme") ||
    (window.matchMedia && window.matchMedia("(prefers-color-scheme: light)").matches ? "light" : "dark");
  applyTheme(initialTheme);

  if (themeToggleBtn) {
    themeToggleBtn.addEventListener("click", () => {
      const current = document.documentElement.getAttribute("data-theme") === "light" ? "light" : "dark";
      applyTheme(current === "light" ? "dark" : "light");
    });
  }

  function log(msg) {
    const ts = new Date().toLocaleTimeString();
    logEl.textContent += `[${ts}] ${msg}\n`;
    logEl.scrollTop = logEl.scrollHeight;
  }

  function parseMac(raw) {
    const cleaned = raw.trim().replace(/[:-]/g, "");
    if (!/^[0-9a-fA-F]{12}$/.test(cleaned)) {
      return null;
    }
    const bytes = new Uint8Array(6);
    for (let i = 0; i < 6; i++) {
      bytes[i] = parseInt(cleaned.slice(i * 2, i * 2 + 2), 16);
    }
    return bytes;
  }

  function formatMac(bytes) {
    return Array.from(bytes)
      .map((b) => b.toString(16).padStart(2, "0").toUpperCase())
      .join(":");
  }

  function formatHex(bytes) {
    return Array.from(bytes)
      .map((b) => b.toString(16).padStart(2, "0").toUpperCase())
      .join(":");
  }

  function updateCliCommand() {
    const parsed = parseMac(macInput.value);
    const mac = parsed ? formatMac(parsed) : "XX:XX:XX:XX:XX:XX";
    cliCmdEl.textContent = `sudo python3 ds3write.py ${mac}`;
  }

  macInput.addEventListener("input", updateCliCommand);
  updateCliCommand();

  // Check Web Serial availability for ESP Web Tools
  if (!("serial" in navigator)) {
    serialBanner.hidden = false;
  }

  // Check if compiled firmware artifacts are hosted alongside this page
  fetch("firmware/firmware.bin", { method: "HEAD" })
    .then((res) => {
      if (res.ok) {
        binaryCheckEl.textContent = "Compiled v0.1.0 firmware binaries detected and ready to flash.";
        binaryCheckEl.className = "notice info";
      } else {
        binaryCheckEl.textContent =
          "Firmware binaries are not in this local folder yet — they are compiled and attached automatically during the GitHub Actions Pages deploy.";
        binaryCheckEl.className = "notice";
      }
    })
    .catch(() => {
      binaryCheckEl.textContent =
        "Open this page over HTTPS (GitHub Pages) or a local HTTP server to enable Web Serial flashing.";
      binaryCheckEl.className = "notice";
    });

  // WebUSB path for Xbox-mode DualShock 3 clones (045e:028e)
  async function openCloneWebUsb() {
    if (!("usb" in navigator)) {
      throw new Error("WebUSB is not supported in this browser (use Chrome, Edge, or Opera).");
    }
    const device = await navigator.usb.requestDevice({
      filters: [{ vendorId: 0x045e, productId: 0x028e }]
    });
    await device.open();
    if (device.configuration === null) {
      await device.selectConfiguration(1);
    }
    try {
      await device.claimInterface(0);
    } catch (err) {
      throw new Error(
        `claimInterface(0) failed (${err.message}). ` +
          "The OS kernel driver (Linux 'xpad' or Windows 'xusb22.sys') is holding the device. " +
          "On Linux run 'sudo modprobe -r xpad' (or use 'ds3write.py'); on Windows use Zadig (WinUSB) or SixAxisPairTool."
      );
    }
    return device;
  }

  async function webUsbGetReport(device, reportId) {
    const res = await device.controlTransferIn(
      {
        requestType: "class",
        recipient: "interface",
        request: 0x01, // HID GET_REPORT
        value: 0x0300 | reportId,
        index: 0
      },
      64
    );
    if (res.status !== "ok") {
      throw new Error(`GET_REPORT 0x${reportId.toString(16)} failed: ${res.status}`);
    }
    return new Uint8Array(res.data.buffer);
  }

  btnWebUsbRead.addEventListener("click", async () => {
    let device = null;
    try {
      device = await openCloneWebUsb();
      const f2 = await webUsbGetReport(device, 0xf2);
      const f5 = await webUsbGetReport(device, 0xf5);
      log(`WebUSB [0xF2] raw: ${formatHex(f2)}`);
      log(`WebUSB [0xF2] Controller MAC: ${formatMac(f2.slice(4, 10))}`);
      log(`WebUSB [0xF5] raw: ${formatHex(f5)}`);
      log(`WebUSB [0xF5] Stored Host MAC: ${formatMac(f5.slice(2, 8))}`);
    } catch (err) {
      log(`WebUSB Read Error: ${err.message}`);
    } finally {
      if (device && device.opened) {
        await device.close().catch(() => {});
      }
    }
  });

  btnWebUsbWrite.addEventListener("click", async () => {
    const mac = parseMac(macInput.value);
    if (!mac) {
      log("Invalid MAC address. Enter 6 hex bytes like 10:52:1C:66:50:7E.");
      return;
    }
    let device = null;
    try {
      device = await openCloneWebUsb();
      const before = await webUsbGetReport(device, 0xf5);
      log(`WebUSB Host MAC before: ${formatMac(before.slice(2, 8))}`);

      const payload = new Uint8Array([0x01, 0x00, ...mac]);
      const out = await device.controlTransferOut(
        {
          requestType: "class",
          recipient: "interface",
          request: 0x09, // HID SET_REPORT
          value: 0x03f5,
          index: 0
        },
        payload
      );
      if (out.status !== "ok") {
        throw new Error(`SET_REPORT 0xF5 failed: ${out.status}`);
      }
      const after = await webUsbGetReport(device, 0xf5);
      log(`WebUSB Host MAC after:  ${formatMac(after.slice(2, 8))}`);
    } catch (err) {
      log(`WebUSB Write Error: ${err.message}`);
    } finally {
      if (device && device.opened) {
        await device.close().catch(() => {});
      }
    }
  });

  // WebHID path for original Sony DualShock 3 (054c:0268)
  async function openOriginalWebHid() {
    if (!("hid" in navigator)) {
      throw new Error("WebHID is not supported in this browser (use Chrome, Edge, or Opera).");
    }
    const devices = await navigator.hid.requestDevice({
      filters: [{ vendorId: 0x054c, productId: 0x0268 }]
    });
    if (!devices.length) {
      throw new Error("No Sony DualShock 3 device selected.");
    }
    const device = devices[0];
    if (!device.opened) {
      await device.open();
    }
    return device;
  }

  btnWebHidRead.addEventListener("click", async () => {
    let device = null;
    try {
      device = await openOriginalWebHid();
      const f2View = await device.receiveFeatureReport(0xf2);
      const f2 = new Uint8Array(f2View.buffer, f2View.byteOffset, f2View.byteLength);
      const f5View = await device.receiveFeatureReport(0xf5);
      const f5 = new Uint8Array(f5View.buffer, f5View.byteOffset, f5View.byteLength);

      log(`WebHID [0xF2] raw: ${formatHex(f2)}`);
      if (f2.length >= 10) {
        log(`WebHID [0xF2] Controller MAC: ${formatMac(f2.slice(4, 10))}`);
      }
      log(`WebHID [0xF5] raw: ${formatHex(f5)}`);
      if (f5.length >= 8) {
        log(`WebHID [0xF5] Stored Host MAC: ${formatMac(f5.slice(2, 8))}`);
      }
    } catch (err) {
      log(
        `WebHID Read Error: ${err.message}. Note: Some OS HID drivers block feature reports 0xF2/0xF5 if not declared in the HID descriptor.`
      );
    } finally {
      if (device && device.opened) {
        await device.close().catch(() => {});
      }
    }
  });

  btnWebHidWrite.addEventListener("click", async () => {
    const mac = parseMac(macInput.value);
    if (!mac) {
      log("Invalid MAC address. Enter 6 hex bytes like 10:52:1C:66:50:7E.");
      return;
    }
    let device = null;
    try {
      device = await openOriginalWebHid();
      const payload = new Uint8Array([0x01, 0x00, ...mac]);
      await device.sendFeatureReport(0xf5, payload);
      const f5View = await device.receiveFeatureReport(0xf5);
      const f5 = new Uint8Array(f5View.buffer, f5View.byteOffset, f5View.byteLength);
      log(`WebHID [0xF5] after write raw: ${formatHex(f5)}`);
      if (f5.length >= 8) {
        log(`WebHID [0xF5] Stored Host MAC: ${formatMac(f5.slice(2, 8))}`);
      }
    } catch (err) {
      log(
        `WebHID Write Error: ${err.message}. Use SixAxisPairTool/DsHidMini (Windows) or ds3write.py (Linux) if blocked by OS HID driver.`
      );
    } finally {
      if (device && device.opened) {
        await device.close().catch(() => {});
      }
    }
  });
})();
