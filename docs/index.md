# PowderBot V3 bridge and lighting guide

The goal is to reproduce the ESP32 bridge and status lights for Leoš and other builders, using a smaller ESP32 without a display while retaining the original V3 dispensing controller.

## How the system fits together

- **Phone app ↔ ESP32 BLE ↔ V3 UART:** the ESP32 takes the place of the stock BLE adapter, forwarding commands and replies.
- **Scale → RS-232 converter → ESP32:** the ESP32 observes scale readings for lighting decisions. The inspected communications configurations leave scale control passive.
- **ESP32 → LED strip:** the lighting code indicates connection, target and result states. The selected revision is `powderbot_merge_v0_6`, with its complete supporting modules.

The standalone bridge advertises as BT05 using service FFE0 and characteristic FFE1. The source sequence and precise file references are in the [recovery checkpoint](evidence/bridge-recovery.md).

## Current status

The complete bridge-plus-lights baseline is recovered and packaged: **powderbot_merge_v0_6**. April 5 reports document its working visual refinements and explicitly defer motor integration. Moving to Bart's board did not itself replace the V3 dispensing controller. See the [baseline and split evidence](evidence/merge-v0_6-recovery.md).

No new compile, upload or compact-board bench test has been performed. The original hardware reports and current source inspection are separate evidence.

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

Both the complete v0.6 baseline and the earlier v1_13 passive-lighting sketch are preserved here. v1_13 listens to the stock BLE module; it cannot replace that module.

[Ask a question or submit a bench report](https://github.com/dankcincy/powderbot-v3-lighting/issues/new/choose). Identify the hardware and firmware revision so evidence from different stages is not mixed.
