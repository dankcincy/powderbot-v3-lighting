# Lighting behavior

[Guide home](index.md)

The reproduction combines bridge events and observed scale readings with status lighting. The first merge skeleton includes separate engine, presentation and NeoPixel-rendering modules, connected to communications callbacks for BLE connection changes, parsed app commands and scale samples. See [source references](evidence/bridge-recovery.md#inspected-source-sequence).

The exact animation sequence, result thresholds, timing and disconnect behavior of the selected merged build still need to be verified. The standalone BLE bridge does not itself drive the lights.

## Behavior to establish in the merged baseline

- Startup, BLE connection and disconnection indications.
- Target receipt and repeated-target handling.
- Running indication and stable-reading requirements.
- Within-tolerance and outside-tolerance results, including later verdict changes.
- Cup removal, re-arming and stop behavior.
- Stale or missing scale data and reconnect behavior.

The original V3 remains responsible for dispensing. An LED result is an indication calculated by the selected lighting code; its criteria must be checked against that revision.

The earlier v1_13 effects and numerical rules are preserved in the [historical reference](v1_13-reference.md). They are useful comparison evidence, but are not assumed to describe the later merged firmware unchanged.
