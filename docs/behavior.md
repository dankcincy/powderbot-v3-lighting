# Understanding the lights

> Scope: this page documents the preserved v1_13 passive-lighting baseline. It does not describe the later stock-BLE replacement. See the [bridge recovery checkpoint](evidence/bridge-recovery.md) for the corrected target and separate pin maps.

[Guide home](index.md)

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

## Result calculation

The sketch uses four qualifying stable (`ST`) readings. Their maximum-minus-minimum spread must be at most 0.04 gn. Their average is compared with the target using a tolerance of 0.02 gn. Accepted later windows can change the displayed verdict.

Before the first decision, an ST reading more than 0.08 gn below target clears the window. There is no corresponding upper gate. Unstable (`US`) readings do not clear the window, and samples have no age expiry.

Three consecutive parsed readings below 1.00 gn after a decision trigger cup removal. Both ST and US readings count for run-start and removal detection. The target is retained after removal.

## Details that matter when troubleshooting

- A repeated target within 0.0005 gn preserves the current state.
- Event animations can temporarily cover the base state.
- A new target cancels queued exit effects, while certain effects already running finish first.
- Serial-stale reporting logs a warning without clearing the target or verdict.
- Floating-point comparisons use unrounded values. A printed spread of 0.040 can lie just beyond the internal limit.
- Minimal mode retains verdict-entry animations and cup-removal wipe.

The [source audit](evidence/source-audit.md) documents parsing, priorities, edge cases and revision differences in detail. These are source findings. Bench observations should identify the exact firmware and assembly tested.
