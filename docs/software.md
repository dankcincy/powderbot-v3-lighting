# Firmware selection and build preparation

[Guide home](index.md)

## Selected replication baseline

Use [firmware/powderbot_merge_v0_6](../firmware/powderbot_merge_v0_6/), opening `powderbot_merge_v0_6.ino` with every supporting file in the same folder. Do not copy the INO alone.

This is the last recovered complete native-BLE/V3 bridge plus refined-lighting build. The April 5 reports identify v0.5 as the known-good cleanup checkpoint and v0.6 as the working visual refinement build. This is historical reported bench success, not a fresh test of the packaged firmware on a new board.

The original V3 still dispenses. The source forwards app bytes to its UART and real V3 replies back to BLE. No touchscreen or external motor-driver library is required by this sketch. See [evidence and source lines](evidence/merge-v0_6-recovery.md).

## Development sequence

| Revision | Role |
| --- | --- |
| v1_13 | Earlier passive lighting with the stock BLE module; retained unchanged |
| core_v2_2_esp32_max_bb_pinmap | Passive Max-era lighting; not native BLE |
| esp32_bt05_v1_8 | Historically working standalone native BLE bridge |
| esp32_bt05_v1_11_comms_hub / powerbot_comms_v1 | Max-era native BLE communications and scale observation, without renderer |
| powderbot_comms_v1_1 / v1_2 | Transition to Bart's GPIO map; v1_2 corrects comments/names |
| powderbot_merge_v0_1–v0_5 | Combined bridge, scale observations and lights; cleanup progression |
| **powderbot_merge_v0_6** | **Selected complete visual build; original V3 remains controller** |
| bart_bulk_v1 | Later Crow-to-V3 command-path experiment, not the complete full-show replacement |
| pb_link_bart / pb_hotshot branches | Independent motor/scale control development, outside this reproduction |

These branches overlap; there is no established single v0.6-to-v0.7 cutover. Earlier motor experiments do not imply the lighting branch stopped using the V3.

## Dependencies and resources

Source includes require:

- Arduino core for the original ESP32 architecture, with `BLEDevice`, `BLEUtils`, `BLEServer`, `BLE2902` and `HardwareSerial`.
- ESP32-compatible `SoftwareSerial.h` (EspSoftwareSerial family).
- `Adafruit_NeoPixel`.

The exact historically compatible core/library versions and board/partition settings are not yet pinned. Do not reuse v1_13 settings as proof of this build's environment. Compilation is **not yet performed in this recovery**.

UART2 serves the V3, UART1 receives scale readings, and software serial receives scale-command observations; USB serial provides diagnostics. Native BLE connection callbacks replace the external BLE STATE input. See [wiring](wiring.md).

## Preparing a reproduction

1. Preserve this complete baseline folder. Make any future changes in a separately named sketch.
2. Confirm the selected original-ESP32 board exposes the five required GPIOs.
3. Establish dependency versions and compile settings; record a reproducible compile result.
4. Verify physical endpoints and voltage compatibility before flashing/testing the assembly.
5. Follow [verification](verification.md), including repeated BLE reconnect and light behavior.

The [SHA-256 manifest](../firmware/powderbot_merge_v0_6/SHA256SUMS.txt) identifies all 16 original files. `MERGE_NOTES.txt` is preserved verbatim, including its stale v0.3 heading and “merge skeleton” wording. The [recovery report](evidence/merge-v0_6-recovery.md) explains those labels.

Historical v1_13 instructions remain in the [reference](v1_13-reference.md).
