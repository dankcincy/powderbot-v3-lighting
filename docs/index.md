# PowderBot V3 bridge and lighting guide

The goal is to reproduce the ESP32 bridge and status lights for Leoš and other builders, using a smaller ESP32 without a display while retaining the original V3 dispensing controller.

## How the system fits together

- **Phone app ↔ ESP32 BLE ↔ V3 UART:** the ESP32 takes the place of the stock BLE adapter, forwarding commands and replies.
- **Scale → RS-232 converter → ESP32:** the ESP32 observes scale readings for lighting decisions. The inspected communications configurations leave scale control passive.
- **ESP32 → LED strip:** the lighting code indicates connection, target and result states. The exact merged revision and its behavior remain under review.

The standalone bridge advertises as BT05 using service FFE0 and characteristic FFE1. The source sequence and precise file references are in the [recovery checkpoint](evidence/bridge-recovery.md).

## Current status

The bridge code and the transition from the ESP32 Max pin mapping to Bart's-board mapping have been recovered. The complete bridge-plus-lights baseline has not yet been selected or newly bench-tested. CrowPanel and broader motor-control experiments are later branches to distinguish from this reproduction.

| Topic | Reference |
| --- | --- |
| Signal paths, separate pin maps and physical gaps | [Wiring](wiring.md) |
| Recovered revisions and firmware selection | [Software](software.md) |
| Lighting behavior and remaining verification | [Lights](behavior.md) |
| Reproduction checks | [Verification](verification.md) |
| Source findings and dated photo anchors | [Recovery checkpoint](evidence/bridge-recovery.md) |
| Earlier passive-lighting build | [Historical v1_13 reference](v1_13-reference.md) |
| Evidence standards and source identity | [Provenance](provenance.md) |
| Working with your own assistant | [Connect your AI](connect-your-ai.md) |

The only firmware currently packaged in this repository is the preserved v1_13 passive-lighting sketch. It listens to the stock BLE module; it cannot replace that module.

[Ask a question or submit a bench report](https://github.com/dankcincy/powderbot-v3-lighting/issues/new/choose). Identify the hardware and firmware revision so evidence from different stages is not mixed.
