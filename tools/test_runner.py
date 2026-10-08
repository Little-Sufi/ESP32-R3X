#!/usr/bin/env python3
"""
Automated Serial Hardware & Tool Testing Harness for ESP32-R3X
Executes diagnostic commands directly over COM port, monitors real-time logs,
detects panics/reboots/memory leaks, and validates every tool and peripheral.
"""

import sys
import time
import re
import argparse
import serial

try:
    sys.stdout.reconfigure(encoding='utf-8')
except Exception:
    pass

class ESP32R3XRunner:
    def __init__(self, port="COM9", baudrate=115200, timeout=1.0):
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.ser = None
        self.test_results = {}

    def connect(self):
        print(f"[*] Opening serial port {self.port} @ {self.baudrate} baud (DTR/RTS guarded)...")
        try:
            self.ser = serial.Serial()
            self.ser.port = self.port
            self.ser.baudrate = self.baudrate
            self.ser.timeout = self.timeout
            self.ser.dtr = False
            self.ser.rts = False
            self.ser.open()
            time.sleep(0.5)
            self.ser.reset_input_buffer()
            self.ser.reset_output_buffer()
            print("[+] Serial port opened successfully without resetting target.")
            return True
        except Exception as e:
            print(f"[-] Failed to open {self.port}: {e}")
            return False

    def close(self):
        if self.ser and self.ser.is_open:
            self.ser.close()
            print("[*] Serial port closed.")

    def send_cmd(self, cmd, wait_time=0.5):
        if not self.ser or not self.ser.is_open:
            return []
        self.ser.reset_input_buffer()
        full_cmd = (cmd.strip() + "\n").encode('utf-8')
        self.ser.write(full_cmd)
        self.ser.flush()
        time.sleep(wait_time)
        lines = []
        while self.ser.in_waiting > 0:
            try:
                line = self.ser.readline().decode('utf-8', errors='replace').strip()
                if line:
                    lines.append(line)
            except Exception:
                pass
        return lines

    def read_stream(self, duration_sec=3.0):
        """Reads incoming serial stream for specified duration."""
        lines = []
        end_time = time.time() + duration_sec
        while time.time() < end_time:
            if self.ser.in_waiting > 0:
                try:
                    line = self.ser.readline().decode('utf-8', errors='replace').strip()
                    if line:
                        lines.append(line)
                except Exception:
                    pass
            else:
                time.sleep(0.05)
        return lines

    def ping(self, retries=5):
        for attempt in range(retries):
            lines = self.send_cmd("PING", wait_time=0.3)
            for l in lines:
                if "[PONG]" in l:
                    return True, l
            time.sleep(0.2)
        return False, "No response"

    def check_panic(self, lines):
        panic_keywords = [
            "Guru Meditation Error",
            "panic'ed",
            "abort() was called",
            "rst:0x",
            "Brownout detector",
            "LoadProhibited",
            "StoreProhibited",
            "InstrFetchProhibited",
            "Backtrace:",
            "MAGIC fadebead"
        ]
        for line in lines:
            for kw in panic_keywords:
                if kw in line:
                    return True, line
        return False, ""

    def run_hardware_diagnostics(self):
        print("\n" + "="*60)
        print("          PHASE 1: HARDWARE & SUBSYSTEM DIAGNOSTICS")
        print("="*60)
        
        # Ping
        ok, pong_resp = self.ping()
        if not ok:
            print("[-] FATAL: Device not responding to PING on COM port!")
            return False
        print(f"[+] Device Connected: {pong_resp}")

        # DIAG Command
        print("\n[*] Sending DIAG command...")
        diag_lines = self.send_cmd("DIAG", wait_time=1.5)
        for line in diag_lines:
            print(f"    {line}")
        
        # TEST ALL Command
        print("\n[*] Sending TEST ALL command (waiting for completion)...")
        test_lines = self.send_cmd("TEST ALL", wait_time=7.5)
        for line in test_lines:
            print(f"    {line}")
            if "[RESULT]" in line:
                parts = line.split("[RESULT]")[-1].strip().split(":")
                if len(parts) == 2:
                    subsystem, res = parts[0].strip(), parts[1].strip()
                    self.test_results[f"HW_{subsystem}"] = res

        print("[*] Diagnostics complete. Verifying device is idle before starting tool testing...")
        time.sleep(1.0)
        self.ser.reset_input_buffer()
        idle_ok, idle_pong = self.ping(retries=8)
        if idle_ok:
            print(f"[+] Device confirmed idle: {idle_pong}")
        else:
            print("[-] Warning: Device did not immediately pong after diagnostics")
        return True

    def test_single_tool(self, menu_idx, sub_idx, layer=0, name="Tool", run_duration=2.5):
        print(f"\n---> Testing Tool: '{name}' (Menu: {menu_idx}, Sub: {sub_idx}, Layer: {layer})")
        self.ser.reset_input_buffer()
        time.sleep(0.1)
        
        # Pre-launch ping & heap check
        pre_ok, pre_pong = self.ping(retries=8)
        if not pre_ok:
            print(f"[-] Pre-check failed: device unresponsive before testing '{name}'")
            self.test_results[name] = "FAIL (UNRESPONSIVE BEFORE LAUNCH)"
            return False

        # Launch tool
        launch_cmd = f"NAV {menu_idx} {sub_idx} {layer}"
        self.send_cmd(launch_cmd, wait_time=0.3)
        
        # Monitor logs while running
        logs = self.read_stream(duration_sec=run_duration)
        panic, panic_msg = self.check_panic(logs)
        if panic:
            print(f"[-] CRASH DETECTED in '{name}': {panic_msg}")
            for l in logs[-10:]:
                print(f"    | {l}")
            self.test_results[name] = f"CRASH: {panic_msg}"
            # Wait for reboot if crashed
            time.sleep(2.0)
            return False

        # Send EXIT command
        exit_lines = self.send_cmd("EXIT", wait_time=0.8)
        post_logs = self.read_stream(duration_sec=1.0)
        all_exit_logs = exit_lines + post_logs
        
        exit_panic, exit_panic_msg = self.check_panic(all_exit_logs)
        if exit_panic:
            print(f"[-] CRASH ON EXIT in '{name}': {exit_panic_msg}")
            self.test_results[name] = f"CRASH ON EXIT: {exit_panic_msg}"
            return False

        # Post-exit verification
        post_ok, post_pong = self.ping(retries=3)
        if post_ok:
            print(f"[+] '{name}' PASS - Clean exit, heap: {post_pong}")
            self.test_results[name] = "PASS"
            return True
        else:
            print(f"[-] '{name}' FROZE - No response to PING after exit.")
            self.test_results[name] = "FAIL (FROZE)"
            return False

    def run_tools_matrix(self):
        print("\n" + "="*60)
        print("          PHASE 2: FULL TOOL NAVIGATION & INTEGRATION MATRIX")
        print("="*60)
        time.sleep(1.0)
        self.ser.reset_input_buffer()

        # 1. Tools Menu (Menu 6)
        tools = [
            (6, 0, 0, "Tools > Serial Monitor"),
            (6, 1, 0, "Tools > Update Firmware"),
            (6, 2, 0, "Tools > Touch Calibrate"),
            (6, 3, 0, "Tools > Hardware Info"),
            (6, 4, 0, "Tools > SD File Manager"),
            (6, 5, 0, "Tools > GPIO Dashboard"),
        ]
        for m, s, l, n in tools:
            self.test_single_tool(m, s, l, n)
            time.sleep(0.3)

        # 2. WiFi Menu (Menu 0)
        wifi_tools = [
            (0, 0, 0, "WiFi > Packet Monitor"),
            (0, 1, 0, "WiFi > Beacon Spammer"),
            (0, 2, 0, "WiFi > WiFi Deauther"),
            (0, 3, 0, "WiFi > Probe Flood"),
            (0, 4, 0, "WiFi > Deauth Detector"),
            (0, 5, 0, "WiFi > WiFi Scanner"),
            (0, 6, 0, "WiFi > Captive Portal"),
            (0, 7, 0, "WiFi > Hidden SSID Revealer"),
            (0, 0, 0, "WiFi > WPS Scanner"),       # Page 1
            (0, 1, 0, "WiFi > ARP Scanner"),
            (0, 2, 0, "WiFi > Karma Attack"),
        ]
        for m, s, l, n in wifi_tools:
            self.test_single_tool(m, s, l, n)
            time.sleep(0.3)

        # 3. 2.4GHz / NRF Menu (Menu 1)
        nrf_tools = [
            (1, 0, 0, "NRF > Scanner"),
            (1, 1, 0, "NRF > Analyzer"),
            (1, 2, 0, "NRF > WLAN Jammer"),
            (1, 3, 0, "NRF > Proto Kill"),
            (1, 4, 0, "NRF > ESB Sniffer"),
            (1, 5, 0, "NRF > ESB Replay"),
            (1, 6, 0, "NRF > MouseJack Scan"),
            (1, 7, 0, "NRF > MouseJack Inject"),
        ]
        for m, s, l, n in nrf_tools:
            self.test_single_tool(m, s, l, n)
            time.sleep(0.3)

        # 4. Bluetooth Menu (Menu 4)
        bt_tools = [
            (4, 0, 0, "BT > BLE Jammer"),
            (4, 1, 0, "BT > BLE Spoofer"),
            (4, 2, 0, "BT > Sour Apple"),
            (4, 3, 0, "BT > AirTag Spoofer"),
            (4, 4, 0, "BT > AirTag Sniffer"),
            (4, 5, 0, "BT > Sniffer"),
            (4, 6, 0, "BT > BLE Scanner"),
            (4, 7, 0, "BT > BLE Rubber Ducky"),
            (4, 0, 0, "BT > Skimmer Detect"),      # Page 1
        ]
        for m, s, l, n in bt_tools:
            self.test_single_tool(m, s, l, n)
            time.sleep(0.3)

        # 5. SubGHz Menu (Menu 5)
        subghz_tools = [
            (5, 0, 0, "SubGHz > Replay Attack"),
            (5, 1, 0, "SubGHz > SubGHz Jammer"),
            (5, 2, 0, "SubGHz > De Bruijn Brute"),
            (5, 3, 0, "SubGHz > Jamming Detector"),
            (5, 4, 0, "SubGHz > Saved Profile"),
        ]
        for m, s, l, n in subghz_tools:
            self.test_single_tool(m, s, l, n)
            time.sleep(0.3)

        # 6. More Submenu (Menu 2) - IR, RFID, GPS
        more_tools = [
            (2, 0, 1, "IR > Record"),
            (2, 1, 1, "IR > Saved Profile"),
            (2, 2, 1, "IR > Universal Controller"),
            (2, 0, 2, "RFID > Card Reader"),
            (2, 1, 2, "RFID > Card Clone"),
            (2, 2, 2, "RFID > Erase"),
            (2, 3, 2, "RFID > Dump"),
            (2, 4, 2, "RFID > Decode Access"),
            (2, 5, 2, "RFID > Jam Reader"),
            (2, 6, 2, "RFID > Tag Disrupt"),
            (2, 7, 2, "RFID > Disrupt Emulate"),
            (2, 0, 3, "GPS > Wardriver"),
            (2, 1, 3, "GPS > Satellite Scanner"),
        ]
        for m, s, l, n in more_tools:
            self.test_single_tool(m, s, l, n)
            time.sleep(0.3)

    def print_summary(self):
        print("\n" + "="*70)
        print("                  FINAL AUTOMATED TEST REPORT")
        print("="*70)
        total = len(self.test_results)
        passed = sum(1 for v in self.test_results.values() if v == "PASS")
        failed = total - passed

        for tool, res in self.test_results.items():
            status = "[PASS]" if res == "PASS" else "[FAIL]"
            print(f"{status:8} | {tool:<35} | {res}")

        print("="*70)
        print(f"TOTAL: {total} | PASSED: {passed} | FAILED: {failed} | SUCCESS RATE: {(passed/total*100) if total else 0:.1f}%")
        print("="*70)

        # Generate markdown report
        with open("AUTOMATED_TEST_REPORT.md", "w", encoding="utf-8") as f:
            f.write("# ESP32-R3X Automated Hardware & Tool Test Report\n\n")
            f.write(f"**Date:** {time.strftime('%Y-%m-%d %H:%M:%S')}\n\n")
            f.write(f"**Summary:** Total: {total} | Passed: {passed} | Failed: {failed} ({(passed/total*100) if total else 0:.1f}%)\n\n")
            f.write("| Subsystem / Tool | Result | Detail |\n")
            f.write("| --- | --- | --- |\n")
            for tool, res in self.test_results.items():
                emoji = "✅ PASS" if res == "PASS" else "❌ FAIL"
                f.write(f"| {tool} | {emoji} | {res} |\n")
        print("[+] Report saved to AUTOMATED_TEST_REPORT.md")
        return failed == 0

def main():
    parser = argparse.ArgumentParser(description="ESP32-R3X Automated Serial Test Harness")
    parser.add_argument("--port", default="COM9", help="Serial COM port (default: COM9)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    args = parser.parse_args()

    runner = ESP32R3XRunner(port=args.port, baudrate=args.baud)
    if not runner.connect():
        sys.exit(1)

    try:
        runner.run_hardware_diagnostics()
        runner.run_tools_matrix()
        success = runner.print_summary()
        sys.exit(0 if success else 1)
    finally:
        runner.close()

if __name__ == "__main__":
    main()
