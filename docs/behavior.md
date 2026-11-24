# Lighting behavior

[Guide home](index.md)

The selected build is **powderbot_merge_v0_6**. Its communications events feed a separate engine, presentation state machine and NeoPixel renderer. The V3 continues dispensing; lighting verdicts do not control its motors.

## Historically reported working refinements

The April 5 merge reports document:

- Center-out result reveal, with color carrying the result.
- Outside-in collapse for cup removal/reset.
- Result animations retained until completion before the hold state takes over.
- A visual-only verdict flip delay tuned to **500 ms**.
- Full-show and boomer modes, with serial controls for show mode, tolerances and visual delay.

The recovered source has 500 ms in both `powderbot_merge_config.h:32` and `powderbot_presentation.h:37`. Default under/over tolerances are each 0.02 grains (`powderbot_merge_config.h:28–29`). These are lighting-engine settings, not V3 dispensing settings. [Evidence and source links](evidence/merge-v0_6-recovery.md).

## Runtime controls

| Commands | Purpose |
| --- | --- |
| `vf` / `full`, `vb` / `boomer`, `vs` / `show` | Set or inspect show mode |
| `vt` / `tol`, `tu <value>`, `to <value>`, `tb <under> <over>` | Inspect or adjust lighting tolerances |
| `vd`, `vd <ms>`, `vv` / `vstate` | Inspect or adjust visual verdict filtering |

The preserved sketch's opening usage notes describe these commands. Do not assume runtime adjustments persist across reset.

## Reproduction checks still required

Test startup, BLE wake/disconnect/reconnect, target receipt, repeated targets, run indication, stable-reading decisions, verdict changes, stop, cup removal and stale/missing scale data on the exact new assembly. Historical reports are not fresh tests of these cases.

The earlier [v1_13 effects](v1_13-reference.md) remain comparison evidence; do not assume every numerical rule carried forward unchanged.
