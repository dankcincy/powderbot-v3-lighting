# PowderBot V3 lighting guide

Reconstructing the original v1_13 setup, one verified connection at a time.

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

## What the setup does

An ESP32 display board listens to the existing V3 BLE traffic, its connection-state signal and translated scale output. A 22-pixel LED strip indicates connection, target receipt, activity and the result calculated by v1_13. The sketch configures both serial taps as receive-only.

The documented board is JC2432W328C, compiled as ESP32 Dev Module. Later CrowPanel and ESP32-S3 work belongs to a separate hardware phase.

## Questions and build records

[Ask a question or submit a bench report](https://github.com/dankcincy/powderbot-v3-lighting/issues/new/choose). You will need a GitHub account to post. Include photos showing both ends of the connection under discussion. Answers and corrections remain attached to the issue for everyone to consult.

This guide is based on recovered source, March reports, build records and media review. A new assembly still needs its own verification record.
