# Agent Guide: usonic_depth_sensor

## Purpose

Firmware for a custom, ultra-low-power wastewater telemetry node deployed in Biar, Spain. It reads ultrasonic distance data and transmits LoRaWAN uplinks, entering a hardware-timed deep sleep between wake cycles to maximize battery life.

## Target Hardware Architecture

* **Microcontroller:** Wio-E5 mini module featuring the embedded STM32WLE5JC core chip.
* **Telemetry Sensor:** DFRobot ultrasonic distance sensor.
* *Interface:* Connected via `LPUART1` using pins `PC0` (RX) and `PC1` (TX) at 9600 baud.
* *Protocol:* Expects a raw 4-byte serial frame: `[0xFF, Data_H, Data_L, Checksum]`.


* **Power Domain & Regulation:**
* *Main Rail:* Powered by a long-life battery pack stepped down via an **MCP1700-3302E** voltage regulator in a **TO-92** package, chosen for its ultra-low $1.6\text{ }\mu\text{A}$ quiescent current.
* *Current Hardware State:* Node is currently running from a **single-cell LiPo battery** through the MCP1700 regulator, with **1 uF / 50 V input and output capacitors** installed as required for stable operation, plus a **47 uF / 16 V bulk capacitor** on the 3.3 V rail as recommended.
* *Peripheral Switching:* Employs a **TO-92** transistor configuration to act as a load switch on the sensor's power rail, allowing the MCU to completely isolate and cut off the sensor's current draw during sleep.
* *Battery Baseline:* Field baseline is a **2200 mAh, 3.7 V** cell.
* *Runtime Target:* **At least 1 year** per charge is acceptable; multi-year operation is desirable but not mandatory.
* *Voltage Monitor Budget:* A constant battery divider leakage around **39 mAh/year** is currently acceptable for this project profile, so switched-divider hardware is optional unless runtime targets become stricter.
* *Battery Measurement Hardware:* True cell-voltage measurement now assumes a permanent 2:1 divider from battery positive to MCU analog input **PA10 / A0** using two equal-value resistors and a shared ground. Keep the divider output below VDDA at the maximum LiPo charge voltage.


* **Toolchain:** Developed using PlatformIO under the native Arduino framework utilizing the `ststm32` core platform.
* **Debug/Program Probe:** Uses **STLINK-V3MINIE** for both SWD programming and UART debug over a single USB connection. In this setup, UART labeling behavior is non-intuitive: reliable console output was achieved with probe `TX` wired to Wio-E5 `TX` and probe `RX` wired to Wio-E5 `RX` (instead of the usual crossed TX/RX convention). This behavior is specific to the current probe firmware + adapter/cable path and should be treated as the project baseline wiring unless hardware changes.

---

## Current Behavior

* **Wake Cycle:** Operates on a production interval of 24 hours (tested locally using a 15-second cadence).
* **Sensor Distance Processing:** The raw 4-byte serial frame is read, checksum-validated, and converted into meters.
* **Uplink Transmission Criteria:** An uplink is triggered only if one of the following conditions is met:
* **First Run / Volatile Diagnostic:** Triggered immediately upon initial power-on or manual reset.
* **Significant Delta Change:** Triggered if the measured depth shifts by $\ge 0.05\text{ m}$ ($5\text{ cm}$) compared to the last successfully sent reading.
* **Heartbeat Timeout:** Triggered if a fixed 30-day heartbeat interval has elapsed without an update.
* **Low Voltage Alert (One-Shot):** Triggered once when measured battery voltage drops to or below `LOW_VOLTAGE_TRIGGER_MV`. To prevent re-trigger spam due to ADC jitter near the threshold, this alert is latched and only re-armed after voltage recovers above `LOW_VOLTAGE_TRIGGER_MV + LOW_VOLTAGE_RECOVERY_HYSTERESIS_MV`.
* **Battery Voltage Calibration:** `measureBatteryVoltageMv()` applies a configurable `BATTERY_VREF_CALIBRATION_GAIN` to the VREFINT-derived rail reading so the reported voltage can be aligned to the measured 3.3 V/USB baseline, then uses build flags `BATTERY_ADC_PIN=PA10` and `BATTERY_DIVIDER_RATIO=2.0f` for the external divider path.


* **LoRaWAN Retry Logic:**
* Max retries: 3
* Retry delay: 2000 ms


* **Device Return State:** The system immediately deactivates peripherals and falls back into true deep sleep after processing.

---

## Payload Contract (Must Stay Stable)

The payload is exactly 7 bytes, packed in big-endian byte order:

1. **Payload Version:** 1 byte (currently hardcoded to `3`).
2. **Range:** 2 bytes (`uint16_t` in millimeters). Meters are converted to mm and rounded to the nearest integer before packing.
3. **Battery Voltage:** 2 bytes (`uint16_t` in millivolts). Derived from the external battery-divider reading on **PA10 / A0**, scaled back to the full cell voltage in firmware.
4. **Boot Count:** 2 bytes (`uint16_t` tracking wake cycles in volatile SRAM). Incremented once per wake cycle; resets on physical reset or power loss.

---

## Power & Sleep Rules

* **Native STOP2 Engine:** Do not use high-level library abstraction layers (such as `STM32LowPower` or `STM32RTC`) as they conflict with the STM32WL architecture. Deep sleep must be managed natively via `HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI)` driven by the internal Low-Speed External (LSE) $32.768\text{ kHz}$ crystal oscillator acting as the RTC wakeup vector.
* **Peripheral Isolation:** Right before entering `STOP2` sleep, code must toggle the designated GPIO pin driving the TO-92 transistor low to entirely cut power to the DFRobot ultrasonic sensor. This pin must be toggled high upon wakeup, allowing a brief stabilization window before querying the LPUART bus.
* **SRAM Preservation:** The `lastSentDistance` variable relies entirely on the volatile nature of SRAM. Because `STOP2` mode natively preserves RAM contents, this baseline automatically survives standard wake cycles.
* **Field Diagnostics Guardrail:** Because the baseline memory is intentionally volatile, pressing the physical **RST button** on the Wio-E5 mini (or swapping the battery) completely wipes the tracking state. This serves as an intentional diagnostic feature: a physical reset forces the node to boot completely fresh, establish an instant sensor baseline, and immediately fire a "fake" test transmission to prove the hardware pipeline is fully intact.
* **Bus Maintenance:** Always call `Serial.flush()` before dropping the core clock into `STOP2` to avoid corrupting active UART debug blocks.

---

## LoRa & Communication Rules

* All communication tasks must route through the internal sub-GHz radio driver of the STM32WLE5JC.
* Distinguish strictly between retryable and fatal radio failures. Treat critical network errors as non-retryable for the current wake window to keep the microcontroller from hanging awake and draining the battery cell.

---

## Logging Rules

* Debugging output outputs over standard UART at `115200` baud, allowing technicians to read diagnostic streams directly on an Android/iOS device using a USB-OTG adapter or low-power Bluetooth pass-through bridge.
* Maintain precise console reporting for:
* Wake cycle execution and core startup.
* Active sensor measurement readings.
* State machine decision logic (e.g., "Delta threshold exceeded" or "Baseline synced").
* Hardware state right before entering `STOP2`.



---

## Change Guardrails

* **Do not modify** payload structures, byte alignment, or version definitions without syncing with the network decoder templates.
* Ensure any modifications to `goToSleep()` re-synchronize core clocks using `SystemClock_Config()` immediately upon wake up, preventing timing drifting on the software serial links.
* Code changes must successfully pass a compilation sweep under the local PlatformIO environment before validation testing.
* Agents must update this `AGENTS.md` whenever important new information becomes available, or when important changes are made to project requirements or hardware.

## Key File

Primary behavior, state parameters, and the low-level `STOP2` control block reside cleanly in `src/main.cpp`.
