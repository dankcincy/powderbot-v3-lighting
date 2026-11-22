# Firmware recovery and selection

[Guide home](index.md)

The complete bridge-plus-lights release is still being identified. Downloading this repository currently supplies the earlier v1_13 lighting sketch, not a ready-to-upload BLE replacement.

## Recovered development sequence

| Source revision | Role |
| --- | --- |
| `powderbot_personality_v1_13` | Self-contained passive lighting on JC2432W328C; packaged here unchanged |
| `esp32_bt05_v1_8` | Standalone BT05 BLE replacement; April 1 report identifies it as the working reconnect baseline |
| `esp32_bt05_v1_11_comms_hub` | Bridge plus scale observation and traffic logging, using Max-era pins |
| `powerbot_comms_v1` | Modular communications using the same pins |
| `powderbot_comms_v1_1` | Bart-board GPIO assignments, with two stale scale-pin comments |
| `powderbot_comms_v1_2` | Corrects those comments and changes module names; same executable behavior as v1_1 after name normalization |
| `powderbot_merge_v0_1` | First merge skeleton joining communications to engine, presentation and strip-rendering modules |

The first merge skeleton is not established as the final working build. Later merge revisions must be compared before selecting the reproduction baseline. The [recovery checkpoint](evidence/bridge-recovery.md) gives source locations, line references, fingerprints and confidence limits.

## What remains before a build package

1. Select the complete merged revision using source comparisons and historical test evidence.
2. Recover its board configuration and dependency versions; earlier v1_13 build settings do not establish the bridge environment.
3. Package the required files together, preserving the archived originals.
4. Compile the selected package and record the result.
5. Identify the actual hardware and verify its wiring before an upload or bench test.

No new bridge compile, upload or bench test has been performed for this documentation update. Historical reports are identified separately from current verification.

The earlier sketch's installation instructions, dependencies and build evidence remain in the [v1_13 reference](v1_13-reference.md).
