/**
 * ============================================================================
 * ESP32-R3X MOBILE COMPANION - CLIENT SYNC CONTROLLER v3.0
 * Ultra-Fast Bluetooth, WiFi & Serial Hardware Link with Auto-Reconnect
 * ============================================================================
 */

(function () {
  'use strict';

  // --- Constants & Hardware UUIDs ---
  const BLE_UART_SERVICE_UUID = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
  const BLE_UART_TX_UUID      = '6e400002-b5a3-f393-e0a9-e50e24dcca9e'; // Phone to ESP32
  const BLE_UART_RX_UUID      = '6e400003-b5a3-f393-e0a9-e50e24dcca9e'; // ESP32 to Phone
  
  // Standard BLE Serial Fallback (0xFFE0 / 0xFFE1)
  const BLE_SERIAL_SERVICE_UUID = 0xffe0;
  const BLE_SERIAL_CHAR_UUID    = 0xffe1;

  // --- State Store ---
  let savedProto = localStorage.getItem('r3x_protocol');
  if (savedProto === 'ble' || !savedProto) {
    savedProto = 'bridge';
    localStorage.setItem('r3x_protocol', 'bridge');
  }

  const state = {
    connected: false,
    connecting: false,
    protocol: savedProto,
    signinCode: localStorage.getItem('r3x_signin_code') || 'R3X-8F2A',
    autoReconnect: localStorage.getItem('r3x_auto_reconnect') !== 'false',
    hapticEnabled: localStorage.getItem('r3x_haptic') !== 'false',
    theme: localStorage.getItem('r3x_theme') || 'cyber',
    
    // Live Hardware Telemetry
    telemetry: {
      batteryVoltage: 4.15,
      batteryPercent: 95,
      freeHeap: 133,
      minHeap: 133,
      psramFree: 8192,
      cpuFreq: 240,
      uptime: 0,
      menuIdx: 6,
      menuName: 'Tools',
      subIdx: 0,
      subName: 'Serial Monitor',
      layer: 0,
      featureActive: false
    },

    // Connections
    bridge: {
      pollInterval: null
    },
    ble: {
      device: null,
      server: null,
      rxChar: null,
      txChar: null
    },
    serial: {
      port: null,
      reader: null,
      writer: null
    },
    wifi: {
      baseUrl: 'http://192.168.4.1',
      pollInterval: null
    },
    demo: {
      active: false,
      timer: null
    },

    cmdHistory: [],
    cmdHistoryIdx: -1
  };

  // --- DOM Elements ---
  const el = {
    indicator: document.getElementById('connection-indicator'),
    statusText: document.getElementById('connection-status-text'),
    headerPill: document.getElementById('header-connection-pill'),
    inputCode: document.getElementById('input-signin-code'),
    btnPair: document.getElementById('btn-pair-device'),
    protoChips: document.querySelectorAll('#connection-protocol-selector .chip-btn'),
    
    // Telemetry displays
    valBattery: document.getElementById('val-battery-voltage'),
    barBattery: document.getElementById('bar-battery-fill'),
    valBatterySub: document.getElementById('val-battery-sub'),
    valHeap: document.getElementById('val-heap-free'),
    barHeap: document.getElementById('bar-heap-fill'),
    valHeapSub: document.getElementById('val-heap-sub'),
    valCpu: document.getElementById('val-cpu-freq'),
    valUptime: document.getElementById('val-uptime'),
    valActiveMenu: document.getElementById('val-active-menu'),
    valActiveSub: document.getElementById('val-active-sub'),

    // TFT Screen Mirror
    mirrorHeaderBat: document.getElementById('mirror-header-bat'),
    mirrorMenuTitle: document.getElementById('mirror-menu-title'),
    mirrorSubTitle: document.getElementById('mirror-sub-title'),
    mirrorFooterLayer: document.getElementById('mirror-footer-layer'),
    mirrorFooterSync: document.getElementById('mirror-footer-sync'),

    // Terminal
    termOutput: document.getElementById('term-output'),
    termInput: document.getElementById('term-input'),
    btnSendCli: document.getElementById('btn-send-cli'),
    btnClearTerm: document.getElementById('btn-clear-term'),
    btnExportLog: document.getElementById('btn-export-log'),
    quickCliChips: document.querySelectorAll('#quick-cli-bar .chip-btn'),

    // Nav & Views
    navItems: document.querySelectorAll('.nav-item'),
    views: document.querySelectorAll('.view-section'),

    // Settings
    toggleAutoReconnect: document.getElementById('toggle-auto-reconnect'),
    toggleHaptic: document.getElementById('toggle-haptic'),
    themeBtns: document.querySelectorAll('[data-theme-btn]'),
    btnInstallPwa: document.getElementById('btn-install-pwa'),

    // D-Pad Keys
    btnUp: document.getElementById('btn-key-up'),
    btnDown: document.getElementById('btn-key-down'),
    btnLeft: document.getElementById('btn-key-left'),
    btnRight: document.getElementById('btn-key-right'),
    btnSelect: document.getElementById('btn-key-select'),
    btnExit: document.getElementById('btn-key-exit'),
    btnDiag: document.getElementById('btn-key-quick-diag'),

    // Feature Tiles
    featureTiles: document.querySelectorAll('.feature-tile')
  };

  // --- Haptic Feedback Helper ---
  function triggerHaptic(duration = 20) {
    if (state.hapticEnabled && 'vibrate' in navigator) {
      try { navigator.vibrate(duration); } catch (_) {}
    }
  }

  // --- Terminal Logging ---
  function logTerminal(text, type = 'info') {
    if (!el.termOutput) return;
    const line = document.createElement('div');
    line.className = `term-line ${type}`;
    const time = new Date().toLocaleTimeString();
    line.textContent = `[${time}] ${text}`;
    el.termOutput.appendChild(line);
    el.termOutput.scrollTop = el.termOutput.scrollHeight;
  }

  // --- UI Update Pipeline ---
  function updateTelemetryUI() {
    const t = state.telemetry;
    
    // Battery
    if (el.valBattery) el.valBattery.innerHTML = `${t.batteryVoltage.toFixed(2)}<span>V</span>`;
    if (el.barBattery) {
      el.barBattery.style.width = `${Math.min(100, Math.max(5, t.batteryPercent))}%`;
      el.barBattery.className = t.batteryVoltage < 3.5 ? 'bar-fill warning' : 'bar-fill';
    }
    if (el.valBatterySub) {
      el.valBatterySub.textContent = `${t.batteryPercent}% • ${t.batteryVoltage >= 4.1 ? 'Charged' : 'Normal'} (LiPo)`;
    }

    // Heap
    if (el.valHeap) el.valHeap.innerHTML = `${t.freeHeap}<span>KB</span>`;
    if (el.barHeap) {
      const heapPct = Math.min(100, Math.round((t.freeHeap / 320) * 100));
      el.barHeap.style.width = `${heapPct}%`;
    }
    if (el.valHeapSub) {
      el.valHeapSub.textContent = `Min Free: ${t.minHeap}KB • PSRAM 8MB`;
    }

    // CPU & Uptime
    if (el.valCpu) el.valCpu.innerHTML = `${t.cpuFreq}<span>MHz</span>`;
    if (el.valUptime) {
      const hrs = Math.floor(t.uptime / 3600);
      const mins = Math.floor((t.uptime % 3600) / 60);
      const secs = t.uptime % 60;
      el.valUptime.textContent = `Uptime: ${hrs}h ${mins}m ${secs}s`;
    }

    // Active Module
    if (el.valActiveMenu) el.valActiveMenu.textContent = t.menuName;
    if (el.valActiveSub) el.valActiveSub.textContent = t.featureActive ? 'Running Feature' : t.subName;

    // TFT Screen Mirror
    if (el.mirrorHeaderBat) el.mirrorHeaderBat.textContent = `${t.batteryVoltage.toFixed(2)}V [${'|'.repeat(Math.ceil(t.batteryPercent / 25))}]`;
    if (el.mirrorMenuTitle) el.mirrorMenuTitle.textContent = `${t.menuName.toUpperCase()} SUITE`;
    if (el.mirrorSubTitle) el.mirrorSubTitle.textContent = `> ${t.subName} <`;
    if (el.mirrorFooterLayer) el.mirrorFooterLayer.textContent = `LAYER: ${t.layer}`;
    if (el.mirrorFooterSync) {
      el.mirrorFooterSync.textContent = state.connected 
        ? `LINKED: ${state.signinCode} (${state.protocol.toUpperCase()})` 
        : 'SYNC DISCONNECTED';
    }
  }

  function setConnectionStatus(status, text) {
    if (el.indicator) {
      el.indicator.className = `status-dot ${status}`;
    }
    if (el.statusText) {
      el.statusText.textContent = text;
    }
    state.connected = (status === 'connected');
    state.connecting = (status === 'connecting');

    if (state.connected) {
      logTerminal(`Connected successfully to ESP32-R3X [${state.signinCode}] via ${state.protocol.toUpperCase()}`, 'success');
      if (el.btnPair) el.btnPair.querySelector('span').textContent = 'DISCONNECT';
    } else {
      if (el.btnPair) el.btnPair.querySelector('span').textContent = 'CONNECT';
    }
    updateTelemetryUI();
  }

  // --- Command Dispatcher ---
  function sendCommand(cmdStr) {
    cmdStr = cmdStr.trim();
    if (!cmdStr) return;

    logTerminal(`>>> ${cmdStr}`, 'cmd');

    if (state.protocol === 'bridge') {
      fetch(`/api/send?cmd=${encodeURIComponent(cmdStr)}`, { method: 'GET' })
        .catch(err => logTerminal(`Bridge write error: ${err.message}`, 'error'));
    } else if (state.protocol === 'ble' && state.ble.txChar) {
      const enc = new TextEncoder();
      state.ble.txChar.writeValue(enc.encode(cmdStr + '\n')).catch(err => {
        logTerminal(`BLE write error: ${err.message}`, 'error');
      });
    } else if (state.protocol === 'serial' && state.serial.writer) {
      state.serial.writer.write(cmdStr + '\n').catch(err => {
        logTerminal(`Serial write error: ${err.message}`, 'error');
      });
    } else if (state.protocol === 'wifi' && state.connected) {
      fetch(`${state.wifi.baseUrl}/cmd?c=${encodeURIComponent(cmdStr)}`, { mode: 'no-cors' })
        .catch(err => logTerminal(`WiFi HTTP error: ${err.message}`, 'error'));
    } else if (state.demo.active) {
      handleSimulatedCommand(cmdStr);
    } else {
      logTerminal(`Cannot send "${cmdStr}": Device is offline`, 'warn');
    }
  }

  // --- Response Parser ---
  function handleDeviceResponse(line) {
    line = line.trim();
    if (!line) return;

    logTerminal(line, 'info');

    // Parse [STATUS] menu_idx=6 (Tools), in_sub_menu=0, sub_idx=0, feature_active=0, exit_req=0, vBat=4.15V
    if (line.includes('[STATUS]')) {
      const matchVbat = line.match(/vBat=([\d\.]+)V/);
      if (matchVbat) {
        state.telemetry.batteryVoltage = parseFloat(matchVbat[1]);
        state.telemetry.batteryPercent = Math.round(((state.telemetry.batteryVoltage - 3.2) / (4.2 - 3.2)) * 100);
      }
      const matchMenu = line.match(/menu_idx=(\d+)\s*\(([^)]+)\)/);
      if (matchMenu) {
        state.telemetry.menuIdx = parseInt(matchMenu[1]);
        state.telemetry.menuName = matchMenu[2];
      }
      const matchSub = line.match(/sub_idx=(\d+)/);
      if (matchSub) {
        state.telemetry.subIdx = parseInt(matchSub[1]);
      }
      const matchFeat = line.match(/feature_active=(\d+)/);
      if (matchFeat) {
        state.telemetry.featureActive = (matchFeat[1] === '1');
      }
      updateTelemetryUI();
    }

    // Parse [PONG] uptime=858233 free_heap=133032 min_heap=133032
    if (line.includes('[PONG]')) {
      const matchUp = line.match(/uptime=(\d+)/);
      if (matchUp) state.telemetry.uptime = Math.floor(parseInt(matchUp[1]) / 1000);
      const matchHeap = line.match(/free_heap=(\d+)/);
      if (matchHeap) state.telemetry.freeHeap = Math.round(parseInt(matchHeap[1]) / 1024);
      const matchMin = line.match(/min_heap=(\d+)/);
      if (matchMin) state.telemetry.minHeap = Math.round(parseInt(matchMin[1]) / 1024);
      updateTelemetryUI();
    }

    // Parse [HEAP] Free: 133032 B, Min Free: 133032 B, Max Alloc: 90100 B, PSRAM Free: 0 B
    if (line.includes('[HEAP]')) {
      const matchFree = line.match(/Free:\s*(\d+)\s*B/);
      if (matchFree) state.telemetry.freeHeap = Math.round(parseInt(matchFree[1]) / 1024);
      updateTelemetryUI();
    }
  // --- PC Hardware Serial Bridge Engine (Local Network Link) ---
  let lastBridgeLogCount = 0;
  async function connectBridge() {
    setConnectionStatus('connecting', 'SYNCING PC BRIDGE...');
    logTerminal('Connecting to ESP32 Hardware Bridge on PC...', 'info');

    try {
      const res = await fetch('/api/status');
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      const data = await res.json();

      if (data.ok && data.status) {
        if (data.status.connected) {
          setConnectionStatus('connected', `BRIDGE (${data.status.port || 'USB'})`);
          logTerminal(`Connected to hardware via bridge on ${data.status.port}!`, 'success');
          applyBridgeStatus(data.status);
        } else {
          setConnectionStatus('connecting', 'PROBING USB...');
          logTerminal('PC Bridge active. Waiting for ESP32 on USB port...', 'info');
        }

        if (state.bridge.pollInterval) clearInterval(state.bridge.pollInterval);
        state.bridge.pollInterval = setInterval(async () => {
          if (!state.connected && state.protocol !== 'bridge') return;
          try {
            const pollRes = await fetch('/api/status');
            if (pollRes.ok) {
              const pollData = await pollRes.json();
              if (pollData.ok && pollData.status) {
                if (pollData.status.connected) {
                  if (!state.connected) {
                    setConnectionStatus('connected', `BRIDGE (${pollData.status.port || 'USB'})`);
                  }
                  applyBridgeStatus(pollData.status);
                } else if (state.connected) {
                  setConnectionStatus('connecting', 'PROBING USB...');
                }
              }
            }

            // Poll recent serial bridge logs
            const logRes = await fetch('/api/logs');
            if (logRes.ok) {
              const logData = await logRes.json();
              if (logData.ok && Array.isArray(logData.logs)) {
                if (logData.logs.length > lastBridgeLogCount) {
                  const newLogs = logData.logs.slice(lastBridgeLogCount);
                  newLogs.forEach(l => {
                    if (!l.startsWith('> ')) {
                      logTerminal(l, 'info');
                    }
                  });
                }
                lastBridgeLogCount = logData.logs.length;
              }
            }
          } catch (_) {}
        }, 1200);

        return true;
      }
    } catch (err) {
      setConnectionStatus('disconnected', 'BRIDGE OFFLINE');
      logTerminal(`Bridge unreachable: ${err.message}. If running standalone, try "USB Serial" or "Demo Sim".`, 'warn');
      return false;
    }
  }

  function applyBridgeStatus(st) {
    if (st.vBat !== undefined && st.vBat > 0) {
      state.telemetry.batteryVoltage = st.vBat;
      state.telemetry.batteryPercent = Math.min(100, Math.max(0, Math.round(((st.vBat - 3.2) / (4.2 - 3.2)) * 100)));
    }
    if (st.menu_idx !== undefined) {
      state.telemetry.menuIdx = st.menu_idx;
      state.telemetry.menuName = st.menu_name || state.telemetry.menuName;
    }
    if (st.sub_idx !== undefined) {
      state.telemetry.subIdx = st.sub_idx;
    }
    if (st.free_heap !== undefined) {
      state.telemetry.freeHeap = Math.round(st.free_heap / 1024);
    }
    if (st.uptime !== undefined) {
      state.telemetry.uptime = st.uptime;
    }
    updateTelemetryUI();
  }

  // --- Bluetooth Low Energy (BLE) Engine ---
  async function connectBLE() {
    if (!navigator.bluetooth) {
      logTerminal('Web Bluetooth is not supported in this browser. Use Chrome on Android or Bluefy on iOS.', 'error');
      alert('Web Bluetooth requires Chrome on Android/Desktop or Bluefy browser on iOS. You can also select "WiFi / SoftAP" or "USB Serial"!');
      return false;
    }

    try {
      setConnectionStatus('connecting', 'SCANNING BLE...');
      logTerminal(`Scanning for ESP32-R3X advertising "${state.signinCode}"...`, 'info');

      const device = await navigator.bluetooth.requestDevice({
        filters: [
          { namePrefix: 'ESP32-R3X' },
          { namePrefix: 'R3X' }
        ],
        optionalServices: [BLE_UART_SERVICE_UUID, BLE_SERIAL_SERVICE_UUID, 'battery_service']
      });

      state.ble.device = device;
      device.addEventListener('gattserverdisconnected', onBleDisconnected);

      logTerminal(`Connecting to GATT Server on ${device.name}...`, 'info');
      const server = await device.gatt.connect();
      state.ble.server = server;

      // Try Nordic UART service
      let service;
      try {
        service = await server.getPrimaryService(BLE_UART_SERVICE_UUID);
        state.ble.txChar = await service.getCharacteristic(BLE_UART_TX_UUID);
        state.ble.rxChar = await service.getCharacteristic(BLE_UART_RX_UUID);
      } catch (_) {
        // Fallback to standard serial
        service = await server.getPrimaryService(BLE_SERIAL_SERVICE_UUID);
        state.ble.txChar = await service.getCharacteristic(BLE_SERIAL_CHAR_UUID);
        state.ble.rxChar = state.ble.txChar;
      }

      await state.ble.rxChar.startNotifications();
      state.ble.rxChar.addEventListener('characteristicvaluechanged', (evt) => {
        const val = new TextDecoder().decode(evt.target.value);
        val.split('\n').forEach(line => handleDeviceResponse(line));
      });

      setConnectionStatus('connected', 'BLE SYNCED');
      sendCommand('PING');
      sendCommand('STATUS');
      return true;

    } catch (err) {
      setConnectionStatus('disconnected', 'BLE FAILED');
      logTerminal(`BLE Connection Error: ${err.message}`, 'error');
      return false;
    }
  }

  function onBleDisconnected() {
    logTerminal('ESP32-R3X BLE link disconnected.', 'warn');
    setConnectionStatus('disconnected', 'DISCONNECTED');
    if (state.autoReconnect) {
      logTerminal('Auto-reconnect active: waiting for hardware power-on advertisement...', 'info');
      setTimeout(connectBLE, 3000);
    }
  }

  // --- Web Serial API Engine (USB-OTG) ---
  async function connectSerial() {
    if (!navigator.serial) {
      logTerminal('Web Serial is not supported in this browser.', 'error');
      alert('Web Serial requires Chrome / Edge on Desktop or Android OTG.');
      return false;
    }

    try {
      setConnectionStatus('connecting', 'OPENING SERIAL...');
      const port = await navigator.serial.requestPort();
      await port.open({ baudRate: 115200 });

      state.serial.port = port;
      const textDecoder = new TextDecoderStream();
      port.readable.pipeTo(textDecoder.writable);
      const reader = textDecoder.readable.getReader();
      state.serial.reader = reader;

      const textEncoder = new TextEncoderStream();
      textEncoder.readable.pipeTo(port.writable);
      state.serial.writer = textEncoder.writable.getWriter();

      setConnectionStatus('connected', 'SERIAL SYNCED');

      (async () => {
        let buffer = '';
        while (true) {
          const { value, done } = await reader.read();
          if (done) break;
          buffer += value;
          const lines = buffer.split('\n');
          buffer = lines.pop();
          lines.forEach(line => handleDeviceResponse(line));
        }
      })();

      sendCommand('PING');
      sendCommand('STATUS');
      return true;

    } catch (err) {
      setConnectionStatus('disconnected', 'SERIAL ERR');
      logTerminal(`Serial Error: ${err.message}`, 'error');
      return false;
    }
  }

  // --- WiFi / SoftAP HTTP Engine ---
  function connectWiFi() {
    setConnectionStatus('connecting', 'SYNCING WIFI...');
    logTerminal(`Connecting to ESP32-R3X Web Cyberdeck at ${state.wifi.baseUrl}...`, 'info');

    // Test ping
    fetch(`${state.wifi.baseUrl}/ping`, { mode: 'no-cors' })
      .then(() => {
        setConnectionStatus('connected', 'WIFI SYNCED');
        if (state.wifi.pollInterval) clearInterval(state.wifi.pollInterval);
        state.wifi.pollInterval = setInterval(() => {
          if (state.connected) {
            sendCommand('STATUS');
          }
        }, 1500);
      })
      .catch((err) => {
        setConnectionStatus('disconnected', 'WIFI UNREACHABLE');
        logTerminal(`Cannot reach ${state.wifi.baseUrl}. Make sure your phone is connected to WiFi AP "ESP32-R3X-CYBERDECK".`, 'warn');
      });
  }

  // --- Realistic Demo Simulator Engine ---
  function connectDemo() {
    setConnectionStatus('connecting', 'STARTING SIM...');
    setTimeout(() => {
      state.demo.active = true;
      setConnectionStatus('connected', 'SIM ACTIVE');
      logTerminal('Hardware Simulator started. Full cyberdeck functionality active for testing!', 'success');
      
      // Simulate live battery discharge and status updates
      if (state.demo.timer) clearInterval(state.demo.timer);
      state.demo.timer = setInterval(() => {
        state.telemetry.uptime += 2;
        state.telemetry.batteryVoltage = Math.max(3.3, state.telemetry.batteryVoltage - 0.001);
        state.telemetry.batteryPercent = Math.round(((state.telemetry.batteryVoltage - 3.2) / (4.2 - 3.2)) * 100);
        updateTelemetryUI();
      }, 2000);

      handleDeviceResponse('[STATUS] menu_idx=6 (Tools), in_sub_menu=1, sub_idx=0, feature_active=0, exit_req=0, vBat=4.15V');
      handleDeviceResponse('[PONG] uptime=12840 free_heap=133032 min_heap=132800');
    }, 400);
  }

  function handleSimulatedCommand(cmd) {
    cmd = cmd.toUpperCase().trim();
    if (cmd === 'PING') {
      setTimeout(() => {
        handleDeviceResponse(`[PONG] uptime=${state.telemetry.uptime * 1000} free_heap=${state.telemetry.freeHeap * 1024} min_heap=132800`);
      }, 50);
    } else if (cmd === 'STATUS') {
      setTimeout(() => {
        handleDeviceResponse(`[STATUS] menu_idx=${state.telemetry.menuIdx} (${state.telemetry.menuName}), in_sub_menu=1, sub_idx=${state.telemetry.subIdx}, feature_active=${state.telemetry.featureActive ? 1 : 0}, exit_req=0, vBat=${state.telemetry.batteryVoltage.toFixed(2)}V`);
      }, 50);
    } else if (cmd === 'HEAP') {
      setTimeout(() => {
        handleDeviceResponse(`[HEAP] Free: ${state.telemetry.freeHeap * 1024} B, Min Free: 132800 B, Max Alloc: 90100 B, PSRAM Free: 8388608 B`);
      }, 50);
    } else if (cmd === 'DIAG') {
      setTimeout(() => {
        handleDeviceResponse('================== HARDWARE PROBE & DIAGNOSTICS ==================');
        handleDeviceResponse('[CHIP] ESP32-S3 rev 2, Cores: 2, CPU: 240 MHz');
        handleDeviceResponse('[RESULT] SD:      PASS');
        handleDeviceResponse('[RESULT] NRF24:   PASS');
        handleDeviceResponse('[RESULT] CC1101:  PASS');
        handleDeviceResponse('[RESULT] PN532:   PASS');
        handleDeviceResponse('[RESULT] GPS:     PASS');
        handleDeviceResponse('[RESULT] IR:      PASS');
        handleDeviceResponse('[RESULT] I2C:     PASS');
        handleDeviceResponse('[RESULT] WIFI:    PASS');
        handleDeviceResponse('[RESULT] BLE:     PASS');
        handleDeviceResponse('[RESULT] BATTERY: PASS');
        handleDeviceResponse('==================================================================');
      }, 100);
    } else if (cmd.startsWith('KEY ')) {
      const key = cmd.substring(4);
      logTerminal(`[KEY] Injected ${key} on TFT display`, 'info');
      if (key === 'DOWN') {
        state.telemetry.subIdx = (state.telemetry.subIdx + 1) % 9;
      } else if (key === 'UP') {
        state.telemetry.subIdx = (state.telemetry.subIdx - 1 + 9) % 9;
      } else if (key === 'RIGHT') {
        state.telemetry.menuIdx = (state.telemetry.menuIdx + 1) % 8;
      } else if (key === 'LEFT') {
        state.telemetry.menuIdx = (state.telemetry.menuIdx - 1 + 8) % 8;
      }
      updateTelemetryUI();
    } else if (cmd === 'EXIT') {
      state.telemetry.featureActive = false;
      logTerminal('[EXIT] Feature exited successfully', 'warn');
      updateTelemetryUI();
    } else if (cmd.startsWith('LAUNCH ')) {
      const parts = cmd.split(' ');
      state.telemetry.menuIdx = parseInt(parts[1]) || 0;
      state.telemetry.subIdx = parseInt(parts[2]) || 0;
      state.telemetry.featureActive = true;
      logTerminal(`[LAUNCH] Started Menu ${state.telemetry.menuIdx}, Feature ${state.telemetry.subIdx}`, 'success');
      updateTelemetryUI();
    }
  }

  // --- Connect / Disconnect Toggle ---
  function toggleConnection() {
    triggerHaptic(30);

    if (state.connected) {
      // Disconnect
      if (state.bridge.pollInterval) {
        clearInterval(state.bridge.pollInterval);
        state.bridge.pollInterval = null;
      }
      if (state.ble.server && state.ble.server.connected) {
        state.ble.server.disconnect();
      }
      if (state.serial.port) {
        state.serial.port.close();
      }
      if (state.wifi.pollInterval) {
        clearInterval(state.wifi.pollInterval);
      }
      if (state.demo.timer) {
        clearInterval(state.demo.timer);
      }
      state.demo.active = false;
      setConnectionStatus('disconnected', 'DISCONNECTED');
      return;
    }

    // Save signin code
    const code = (el.inputCode.value || 'R3X-8F2A').trim().toUpperCase();
    state.signinCode = code;
    localStorage.setItem('r3x_signin_code', code);

    if (state.protocol === 'bridge') {
      connectBridge();
    } else if (state.protocol === 'ble') {
      connectBLE();
    } else if (state.protocol === 'serial') {
      connectSerial();
    } else if (state.protocol === 'wifi') {
      connectWiFi();
    } else if (state.protocol === 'demo') {
      connectDemo();
    }
  }

  // --- Event Listeners Setup ---
  function initEvents() {
    // Connect Button
    if (el.btnPair) {
      el.btnPair.addEventListener('click', toggleConnection);
    }

    // Protocol selector chips
    el.protoChips.forEach(chip => {
      chip.addEventListener('click', () => {
        triggerHaptic(15);
        el.protoChips.forEach(c => c.classList.remove('active'));
        chip.classList.add('active');
        state.protocol = chip.dataset.protocol;
        localStorage.setItem('r3x_protocol', state.protocol);
        logTerminal(`Transport protocol switched to: ${state.protocol.toUpperCase()}`);
      });
    });

    // D-Pad Remote Buttons
    if (el.btnUp) el.btnUp.addEventListener('click', () => { triggerHaptic(20); sendCommand('KEY UP'); });
    if (el.btnDown) el.btnDown.addEventListener('click', () => { triggerHaptic(20); sendCommand('KEY DOWN'); });
    if (el.btnLeft) el.btnLeft.addEventListener('click', () => { triggerHaptic(20); sendCommand('KEY LEFT'); });
    if (el.btnRight) el.btnRight.addEventListener('click', () => { triggerHaptic(20); sendCommand('KEY RIGHT'); });
    if (el.btnSelect) el.btnSelect.addEventListener('click', () => { triggerHaptic(30); sendCommand('KEY SELECT'); });
    if (el.btnExit) el.btnExit.addEventListener('click', () => { triggerHaptic(40); sendCommand('EXIT'); });
    if (el.btnDiag) el.btnDiag.addEventListener('click', () => { triggerHaptic(25); sendCommand('DIAG'); });

    // Feature Tiles Launcher
    el.featureTiles.forEach(tile => {
      tile.addEventListener('click', () => {
        triggerHaptic(25);
        const launchArg = tile.dataset.launch;
        if (launchArg) {
          sendCommand(`LAUNCH ${launchArg}`);
        }
      });
    });

    // Terminal Input
    if (el.btnSendCli && el.termInput) {
      const sendInput = () => {
        const val = el.termInput.value.trim();
        if (val) {
          state.cmdHistory.push(val);
          state.cmdHistoryIdx = state.cmdHistory.length;
          sendCommand(val);
          el.termInput.value = '';
        }
      };
      el.btnSendCli.addEventListener('click', sendInput);
      el.termInput.addEventListener('keydown', (e) => {
        if (e.key === 'Enter') {
          sendInput();
        } else if (e.key === 'ArrowUp') {
          if (state.cmdHistoryIdx > 0) {
            state.cmdHistoryIdx--;
            el.termInput.value = state.cmdHistory[state.cmdHistoryIdx] || '';
          }
        } else if (e.key === 'ArrowDown') {
          if (state.cmdHistoryIdx < state.cmdHistory.length - 1) {
            state.cmdHistoryIdx++;
            el.termInput.value = state.cmdHistory[state.cmdHistoryIdx] || '';
          } else {
            state.cmdHistoryIdx = state.cmdHistory.length;
            el.termInput.value = '';
          }
        }
      });
    }

    // Terminal Clear & Export
    if (el.btnClearTerm) {
      el.btnClearTerm.addEventListener('click', () => {
        if (el.termOutput) el.termOutput.innerHTML = '';
      });
    }

    if (el.btnExportLog) {
      el.btnExportLog.addEventListener('click', () => {
        const text = el.termOutput ? el.termOutput.innerText : '';
        const blob = new Blob([text], { type: 'text/plain' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = `r3x-console-log-${Date.now()}.txt`;
        a.click();
        URL.revokeObjectURL(url);
      });
    }

    // Quick Command Bar
    el.quickCliChips.forEach(chip => {
      chip.addEventListener('click', () => {
        triggerHaptic(20);
        sendCommand(chip.dataset.cmd);
      });
    });

    // Navigation Tabs
    el.navItems.forEach(item => {
      item.addEventListener('click', () => {
        triggerHaptic(15);
        el.navItems.forEach(i => i.classList.remove('active'));
        item.classList.add('active');

        const viewId = item.dataset.view;
        el.views.forEach(v => {
          v.classList.toggle('active', v.id === viewId);
        });
      });
    });

    // Settings
    if (el.toggleAutoReconnect) {
      el.toggleAutoReconnect.checked = state.autoReconnect;
      el.toggleAutoReconnect.addEventListener('change', (e) => {
        state.autoReconnect = e.target.checked;
        localStorage.setItem('r3x_auto_reconnect', state.autoReconnect);
      });
    }

    if (el.toggleHaptic) {
      el.toggleHaptic.checked = state.hapticEnabled;
      el.toggleHaptic.addEventListener('change', (e) => {
        state.hapticEnabled = e.target.checked;
        localStorage.setItem('r3x_haptic', state.hapticEnabled);
      });
    }

    // Theme Switcher
    el.themeBtns.forEach(btn => {
      btn.addEventListener('click', () => {
        triggerHaptic(15);
        el.themeBtns.forEach(b => b.classList.remove('active'));
        btn.classList.add('active');
        const theme = btn.dataset.themeBtn;
        state.theme = theme;
        localStorage.setItem('r3x_theme', theme);
        document.body.setAttribute('data-theme', theme);
      });
    });

    // PWA Install Prompt
    let deferredPrompt;
    window.addEventListener('beforeinstallprompt', (e) => {
      e.preventDefault();
      deferredPrompt = e;
      if (el.btnInstallPwa) el.btnInstallPwa.style.display = 'block';
    });

    if (el.btnInstallPwa) {
      el.btnInstallPwa.addEventListener('click', async () => {
        if (deferredPrompt) {
          deferredPrompt.prompt();
          const { outcome } = await deferredPrompt.userChoice;
          logTerminal(`PWA Install outcome: ${outcome}`);
          deferredPrompt = null;
        } else {
          alert('To install on iOS: Tap Share -> "Add to Home Screen".\nOn Android: Tap browser menu -> "Install App".');
        }
      });
    }
  }

  // --- Initializer ---
  function init() {
    // Restore saved settings
    if (el.inputCode) el.inputCode.value = state.signinCode;
    document.body.setAttribute('data-theme', state.theme);

    // Set initial active protocol chip
    el.protoChips.forEach(chip => {
      chip.classList.toggle('active', chip.dataset.protocol === state.protocol);
    });

    // Set initial active theme button
    el.themeBtns.forEach(btn => {
      btn.classList.toggle('active', btn.dataset.themeBtn === state.theme);
    });

    initEvents();
    updateTelemetryUI();

    // Register Service Worker for offline PWA
    if ('serviceWorker' in navigator) {
      navigator.serviceWorker.register('./sw.js').then(() => {
        logTerminal('[PWA] Service Worker registered for offline sync.');
      }).catch(err => {
        console.warn('SW registration failed:', err);
      });
    }

    logTerminal('ESP32-R3X Companion v3.0 ready. Select Connect or Demo Simulator.');

    // Auto-probe PC Bridge or auto-connect
    setTimeout(async () => {
      // Check if PC Bridge API is accessible
      try {
        const testRes = await fetch('/api/status', { method: 'GET' });
        if (testRes.ok) {
          const testData = await testRes.json();
          if (testData && testData.ok) {
            logTerminal(`[PC BRIDGE] Detected active serial bridge (${testData.status.port || 'USB'}). Auto-connecting...`, 'success');
            state.protocol = 'bridge';
            localStorage.setItem('r3x_protocol', 'bridge');
            el.protoChips.forEach(chip => {
              chip.classList.toggle('active', chip.dataset.protocol === 'bridge');
            });
            connectBridge();
            return;
          }
        }
      } catch (_) {
        // Not running via bridge server or bridge offline
      }

      if (state.autoReconnect) {
        logTerminal(`Always-on auto-connect enabled. Probing for hardware [${state.signinCode}]...`);
        if (state.protocol === 'bridge') {
          connectBridge();
        } else if (state.protocol === 'demo') {
          connectDemo();
        } else if (state.protocol === 'wifi') {
          connectWiFi();
        }
      }
    }, 600);
  }

  window.addEventListener('DOMContentLoaded', init);

})();
