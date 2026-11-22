# Wiring and physical evidence

[Guide home](index.md)

## Documented signal map

| Source | Destination | Configuration | Evidence |
| --- | --- | --- | --- |
| BLE module TX | ESP32 GPIO21 | Receive-only, 9600 baud | Original code and March reports |
| BLE module STATE | ESP32 GPIO22 | Active high, 30 ms debounce | Code and March 26 reports |
| MAX3232 R1OUT | ESP32 GPIO35 | UART1 RX, 19200 baud, 8N1 | Code and March 22 report |
| ESP32 GPIO17 | 330-ohm series resistor, then strip DI | 22 pixels, GRB, 800 kHz | Code and March 23 report |
| External regulated 5 V | Strip +5V | Actual supply model/rating unresolved | March strip report |
| ESP32 ground | Strip and strip-supply ground | Common reference required | Code header and March report |

The BLE TX label is relative to the module. This is the line carrying app intent toward the V3. The scale input to the ESP32 comes from the converter's logic-side R1OUT. RS-232 and the apparently 5 V ST232C paths require electrical-level review before any ESP32 connection.

## Historical connector markings

Reviewed board photos show P6 marked GND/IO16/IO4/IO17, CN1 marked GND/IO22/IO21/3V3, and P3 marked GND/IO35/IO22/IO21. The actual harness connector used in the final assembly remains unresolved.

Earlier March prototypes used GPIO22 for translated command chatter. The v1_13 configuration uses that pin for BLE STATE. Check the sketch version before following an older photograph or report.

## Connections still to establish

- Exact numbered scale-connector contact to MAX3232 R1IN continuity.
- MAX3232 module identity, VCC source and logic-output voltage.
- Actual BLE STATE tap location and readable photographs of its endpoints.
- Complete ESP32/strip power arrangement, supply rating and ground junctions.
- Exact strip part and physical pad order on the strip being used.
- Original scale menu settings, independently corroborated for this assembly.

Reviewed DB9 rear-view photos show blue on molded contact 2, green on 3, black on 5 and red on 9. These establish observed contact numbers and colors only. Electrical functions require continuity or documentary confirmation.

## What the videos establish

March 23 and March 26 videos show the translucent rim operating, with red, yellow and dark leads at its entry. Covered junctions and overlapping wires hide the endpoints. Wire color alone cannot close the open connections above.

Before power-up, attach a proposed wiring diagram and photographs to a wiring-review issue. Every supply, ground and signal endpoint should be identifiable. Resolve the listed electrical uncertainties against that assembly.

Sources are linked in [provenance](provenance.md).
