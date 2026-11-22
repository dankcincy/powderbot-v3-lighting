# Bridge signal paths and wiring evidence

[Guide home](index.md)

The ESP32 replaces the stock BLE module at the V3's serial interface. Its BLE connection serves the phone app; its wired UART serves the V3. Separate converter outputs let it observe the scale readings and V3-to-scale command traffic.

## Recovered GPIO assignments

These are source configurations, not a completed wiring diagram or proof of physical continuity.

| Signal, relative to ESP32 | ESP32 Max-era communications | Bart-board communications |
| --- | --- | --- |
| Receive V3 replies | GPIO32 | GPIO34 |
| Transmit app commands to V3 | GPIO33 | GPIO22 |
| Observe scale readings | GPIO21 | GPIO32 |
| Observe V3-to-scale commands | GPIO35 | GPIO33 |
| V3 UART | 9600 baud | 9600 baud |
| Scale observation | 19200 baud | 19200 baud |

The Max-era map appears in `esp32_bt05_v1_11_comms_hub` and `powerbot_comms_v1`. Bart's mapping appears in `powderbot_comms_v1_1` and persists in `v1_2` and the first merge skeleton. `v1_1` contains stale scale-pin comments; executable assignments and `v1_2` agree on GPIO32/33. See [exact source references](evidence/bridge-recovery.md#inspected-source-sequence).

The standalone `esp32_bt05_v1_8` only implements the V3 BLE/UART bridge; its source does not provide scale observation or LED output.

## Signal direction at the former BLE module

The stock module's **TXD** path carries commands into the V3. ESP32 **TX** takes over that path. The stock module's **RXD** path carries replies from the V3; ESP32 **RX** receives those replies. Labels are relative to the device, so matching TX labels blindly to RX labels on an unidentified connector can reverse the intended path.

## Connections still to establish for the selected build

- Exact V3 header contacts and logic levels, including power and ground.
- Scale connector contacts, converter identity, converter supply and logic-output voltage.
- Complete signal continuity and shared ground arrangement.
- LED output GPIO, strip supply, pad order and level interface for the selected merged firmware.
- Scale settings and the complete firmware/dependency configuration.
- Pin availability and electrical suitability of the eventual smaller ESP32 board.

Do not reuse the v1_13 wiring table for this bridge. That earlier configuration uses different pins and retains the stock BLE module; it is preserved in the [historical reference](v1_13-reference.md).

Photographs establish visible hardware, not hidden continuity, voltages or running firmware. Review an assembly-specific diagram before power-up and record measurements in a bench report.
