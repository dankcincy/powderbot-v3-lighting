# PowderBot V3 lighting guide

Recovering the later ESP32 BLE replacement and lighting setup while preserving the earlier v1_13 evidence.

**November 22 scope correction:** v1_13 only listens to the stock BLE module. The target for Leoš is the later bridge that replaces that module, with scale observation and lights, while the V3 retains dispensing control. Read the [bridge recovery checkpoint](evidence/bridge-recovery.md) before choosing hardware or firmware. The pages below still document v1_13; a complete bridge-plus-lights build package has not yet been selected or validated.

**Assembly review in progress.** Read the unresolved items in the wiring guide before connecting power.

## Your route through the project

| Step | What you will learn |
| --- | --- |
| [Wiring](wiring.md) | Signal directions, GPIO assignments and connections awaiting verification |
| [Software](software.md) | Original sketch, historical tool versions and preparation |
| [Light behavior](behavior.md) | What each effect means and how readings become a result |
| [Verification](verification.md) | Checks to record as the reconstruction progresses |
| [Connect your AI](connect-your-ai.md) | Use your own assistant with the project documents |
| [Evidence](provenance.md) | Source history, hashes and confidence limits |

## What the preserved v1_13 setup does

An ESP32 display board listens to the existing V3 BLE traffic, its connection-state signal and translated scale output. A 22-pixel LED strip indicates connection, target receipt, activity and the result calculated by v1_13. The sketch configures both serial taps as receive-only.

The documented board is JC2432W328C, compiled as ESP32 Dev Module. Later CrowPanel and ESP32-S3 work belongs to a separate hardware phase.

## Questions and build records

[Ask a question or submit a bench report](https://github.com/dankcincy/powderbot-v3-lighting/issues/new/choose). You will need a GitHub account to post. Include photos showing both ends of the connection under discussion. Answers and corrections remain attached to the issue for everyone to consult.

This guide is based on recovered source, March reports, build records and media review. A new assembly still needs its own verification record.
