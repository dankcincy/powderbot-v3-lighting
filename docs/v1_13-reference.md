# Historical v1_13 lighting reference

[Current guide](index.md)

This preserves the earlier passive-lighting configuration on JC2432W328C. The stock BLE module remains installed in this design. These instructions and behavior do not describe the later ESP32 BLE replacement. The original firmware remains unchanged.


## Wiring and physical evidence



### Documented signal map

| Source | Destination | Configuration | Evidence |
| --- | --- | --- | --- |
| BLE module TX | ESP32 GPIO21 | Receive-only, 9600 baud | Original code and March reports |
| BLE module STATE | ESP32 GPIO22 | Active high, 30 ms debounce | Code and March 26 reports |
| MAX3232 R1OUT | ESP32 GPIO35 | UART1 RX, 19200 baud, 8N1 | Code and March 22 report |
| ESP32 GPIO17 | 330-ohm series resistor, then strip DI | 22 pixels, GRB, 800 kHz | Code and March 23 report |
| External regulated 5 V | Strip +5V | Actual supply model/rating unresolved | March strip report |
| ESP32 ground | Strip and strip-supply ground | Common reference required | Code header and March report |

The BLE TX label is relative to the module. This is the line carrying app intent toward the V3. The scale input to the ESP32 comes from the converter's logic-side R1OUT. RS-232 and the apparently 5 V ST232C paths require electrical-level review before any ESP32 connection.

### Historical connector markings

Reviewed board photos show P6 marked GND/IO16/IO4/IO17, CN1 marked GND/IO22/IO21/3V3, and P3 marked GND/IO35/IO22/IO21. The actual harness connector used in the final assembly remains unresolved.

Earlier March prototypes used GPIO22 for translated command chatter. The v1_13 configuration uses that pin for BLE STATE. Check the sketch version before following an older photograph or report.

### Connections still to establish

- Exact numbered scale-connector contact to MAX3232 R1IN continuity.
- MAX3232 module identity, VCC source and logic-output voltage.
- Actual BLE STATE tap location and readable photographs of its endpoints.
- Complete ESP32/strip power arrangement, supply rating and ground junctions.
- Exact strip part and physical pad order on the strip being used.
- Original scale menu settings, independently corroborated for this assembly.

Reviewed DB9 rear-view photos show blue on molded contact 2, green on 3, black on 5 and red on 9. These establish observed contact numbers and colors only. Electrical functions require continuity or documentary confirmation.

### What the videos establish

March 23 and March 26 videos show the translucent rim operating, with red, yellow and dark leads at its entry. Covered junctions and overlapping wires hide the endpoints. Wire color alone cannot close the open connections above.

Before power-up, attach a proposed wiring diagram and photographs to a wiring-review issue. Every supply, ground and signal endpoint should be identifiable. Resolve the listed electrical uncertainties against that assembly.

Sources are linked in [provenance](provenance.md).

## Software preparation



1. Download the repository using GitHub's **Code â†’ Download ZIP**, then extract it.
2. Open `firmware/powderbot_personality_v1_13/powderbot_personality_v1_13.ino` in Arduino IDE. Keep the sketch inside its matching folder.
3. Select **ESP32 Dev Module** and prepare the historical dependencies below.
4. Use **Verify** to compile. Save the complete build output in a bench report. Resolve missing libraries or compilation errors before considering an upload.
5. Select the actual connected board port only after identifying the board and reviewing its wiring. Historical COM6 is a record of Dan's machine.

### Recovered environment

| Item | Recorded version or setting |
| --- | --- |
| Arduino IDE | 2.3.8 |
| Arduino CLI in historical IDE log | 1.4.1 |
| ESP32 board package | 3.3.7 |
| Board | ESP32 Dev Module, `esp32:esp32:esp32` |
| EspSoftwareSerial | 8.1.0, corroborated by local library metadata |
| Adafruit NeoPixel | 1.15.4, corroborated by local library metadata |
| CPU / flash | 240 MHz / 80 MHz, 4 MB |
| Partitions / PSRAM | Default / disabled |
| Upload speed | 921600 |
| Serial Monitor | 115200 baud |

Historical build options name QIO, while generated flash arguments and packaged image headers specify DIO. Preserve this distinction when reproducing the environment. See the [cache audit](evidence/build-provenance.md).

The preserved source uses GPIO21 BLE RX, GPIO22 STATE, GPIO35 scale RX and GPIO17 strip output. It defaults to 22 pixels, brightness 100 and full animation mode. Check [behavior](v1_13-reference.md) before changing any constants.

A fresh compile and upload for this repository package have not been performed. Historical build and upload evidence is recorded separately in provenance.

## Understanding the lights



| Condition | Full-mode display |
| --- | --- |
| Startup while disconnected without a target | Brief red startup sequence |
| BLE connects | Blue wake animation |
| Connected, waiting for target | Breathing red center ember |
| New target arrives | Red acknowledgement, then armed heartbeat |
| Two consecutive parsed weights at or above 1.00 gn | Running red scanner |
| Accepted result within tolerance | Green bloom, then green hold |
| Accepted result outside tolerance | Red pulse, then red hold |
| Cup removal after a result | Result-color wipe, then re-arm with the same target |
| Stop | Captured result-color collapse, or red center pinch without a result |
| BLE disconnects | Blue collapse and target/run reset |

### Result calculation

The sketch uses four qualifying stable (`ST`) readings. Their maximum-minus-minimum spread must be at most 0.04 gn. Their average is compared with the target using a tolerance of 0.02 gn. Accepted later windows can change the displayed verdict.

Before the first decision, an ST reading more than 0.08 gn below target clears the window. There is no corresponding upper gate. Unstable (`US`) readings do not clear the window, and samples have no age expiry.

Three consecutive parsed readings below 1.00 gn after a decision trigger cup removal. Both ST and US readings count for run-start and removal detection. The target is retained after removal.

### Details that matter when troubleshooting

- A repeated target within 0.0005 gn preserves the current state.
- Event animations can temporarily cover the base state.
- A new target cancels queued exit effects, while certain effects already running finish first.
- Serial-stale reporting logs a warning without clearing the target or verdict.
- Floating-point comparisons use unrounded values. A printed spread of 0.040 can lie just beyond the internal limit.
- Minimal mode retains verdict-entry animations and cup-removal wipe.

The [source audit](evidence/source-audit.md) documents parsing, priorities, edge cases and revision differences in detail. These are source findings. Bench observations should identify the exact firmware and assembly tested.

## Reconstruction check sheet



Create a **Bench report** issue and record results as you work. The steps below are a review sequence, with power-up dependent on closure of the wiring questions.

1. **Identify the parts.** Photograph board labels, both sides of the converter, strip pads, connector numbers and power-supply label. Record substitutions.
2. **Review wiring with power disconnected.** Map every endpoint against the wiring page. Record continuity results and resolve converter supply, signal levels and ground references with the project maintainer.
3. **Compile the preserved sketch.** Attach build output and dependency versions. Record any difference from the historical environment.
4. **Approve an assembly-specific power-up plan.** Record agreed supply voltage/current capability and measured logic levels. The current guide leaves these physical details open.
5. **Check startup and serial output.** After approved connection and upload, observe Serial Monitor at 115200. Record actual startup text, board port and LED behavior.
6. **Check connection and target indications.** Record BLE connect/disconnect behavior and the received target text. Compare observations with the behavior page.
7. **Record scale input and result behavior.** Capture raw ST/US lines and timestamps alongside the indicated state. Identify which evidence supports each expected transition.
8. **Review discrepancies.** Keep the issue open until the observed behavior and wiring agree with the documented version. Add confirmed corrections to the guide with dated evidence.

A complete report states who tested, date, hardware, source revision, environment, connections, observations and unresolved items. Mark unperformed steps explicitly.
