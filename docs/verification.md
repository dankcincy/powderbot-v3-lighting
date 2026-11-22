# Reconstruction check sheet

> Scope: this page documents the preserved v1_13 passive-lighting baseline. It does not describe the later stock-BLE replacement. See the [bridge recovery checkpoint](evidence/bridge-recovery.md) for the corrected target and separate pin maps.

[Guide home](index.md)

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
