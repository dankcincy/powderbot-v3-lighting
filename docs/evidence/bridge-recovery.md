# Bridge recovery checkpoint — November 22, 2026

[Guide home](../index.md)

## Decision and scope correction

Dan clarified that the setup to reproduce for Leoš is the later ESP32 replacement for the V3 stock BLE adapter, combined with scale observation and status lights. The original V3 remains the dispensing controller. A smaller ESP32 without a display is the intended destination; a specific replacement board has not been selected.

The packaged `powderbot_personality_v1_13` is an earlier, passive lighting baseline on JC2432W328C. It remains preserved and useful as lighting evidence, but is not the complete target. It was not the last single-file sketch overall: standalone BLE bridge sketches followed. Later modularization is not itself evidence of a hardware change.

## Inspected source sequence

Paths below are relative to the maintainer's archived `Firmware/Arduino/Sketchbook/`. These source files remain outside the published firmware package. Line references describe the files inspected November 22; hashes below identify their bytes.

| Revision | Source finding |
| --- | --- |
| `Ble/esp32_bt05_v1_8/esp32_bt05_v1_8.ino` | Lines 6–17 configure BT05 identity, FFE0/FFE1 and V3 UART RX32/TX33 at 9600 baud. Lines 39–48 forward BLE writes to Serial2; lines 65–78 notify buffered V3 replies. This standalone sketch contains no light renderer. |
| `Ble/esp32_bt05_v1_11_comms_hub/esp32_bt05_v1_11_comms_hub.ino` | Lines 30–32 retain RX32/TX33; lines 62–67 select passive scale monitoring, RX21 and command-monitor RX35. |
| `Ble/powerbot_comms_v1/powerbot_comms_v1/powerbot_comms_v1.cpp` | Lines 13–14 and 28–30 retain V3 RX32/TX33 and scale RX21/command RX35. The spelling `powerbot` is the actual archived name. |
| `Ble/powderbot_comms_v1_1/powderbot_comms_v1_1/powderbot_comms_v1_1.ino` | Lines 23–34 explicitly introduce Bart's-board mapping: V3 RX34/TX22, scale RX32/command RX33. Two comments incorrectly say GPIO36/GPIO35; executable assignments say 32/33. The CPP defaults agree with the executable assignments. |
| `Ble/powderbot_comms_v1_2/powderbot_comms_v1_2/powderbot_comms_v1_2.ino` | Lines 32–34 correct those comments to GPIO32/GPIO33. Compared with v1_1, all five source files are otherwise identical after normalizing the module name. This is not a new executable pin-map transition. |
| `powderbot_merge_v0_1/powderbot_merge_v0_1/` | `MERGE_NOTES.md` calls this the first merge skeleton, not the final merged machine. The INO lines 14–32 include the communications, engine, presentation and NeoPixel renderer; lines 74–80 retain Bart's mapping. This identifies the next branch to investigate, not a selected working release. |

The saved communications pin-map transition is **powerbot_comms_v1 → powderbot_comms_v1_1**. It does not establish the first physical installation or successful upload to Bart's board. Earlier comments saying “Bart-minded” on an ESP32 Max prototype do not establish Bart's actual board was in use.

## Photo anchors supplied by Dan

Times below are Dan's reported phone-gallery local times, not times inferred from filenames.

| Date in 2026 | Evidence and limit |
| --- | --- |
| March 31, 9:42 PM | Initially thought to show stock BLE removal. Dan subsequently noticed it could still have been remotely wired. Not a confirmed electrical-disconnection anchor. |
| March 31, 11:25 PM | `IMG_20260401_002553645_HDR.jpg`: stock blue BLE module visibly loose with disconnected header pins; ESP32 Max remains wired. Filename date/time must not override Dan's gallery timestamp. Does not identify the running firmware or prove successful operation. |
| April 2, 6:08 PM | Dan reports photos show Bart's board being wired in. User-reported hardware anchor; not a verified upload time. |
| April 5, 10:55 AM | `IMG_20260405_115539889_HDR.jpg`: Dan identifies CrowPanel; visible keypad, 70.5 entry and trickler/bulk controls. Shows a running interface, not proof the controls operated hardware or its first use. |

These anchors focus the ESP32 Max bridge investigation on March 31–April 2. They do not prove uninterrupted bridge operation throughout that window. Earlier March 26 display-related build records remain separate evidence; April 5 is not claimed as the first-ever CrowPanel experiment.

Explorer folder modification dates skipped April 2, but the contained `powerbot_comms_v1.ino` has an April 2 modification date. Folder and file timestamps are different evidence, and neither alone establishes upload or bench success. Exact timestamp timezone interpretation has not been reconciled with the IDE logs.

## Documentary evidence versus current verification

The archived April 1 `PowderBot_BLE_Takeover_Bringup_Report_v0_2.docx` identifies `esp32_bt05_v1_8` as the working standalone bridge baseline after reconnect fixes. This is a historical reported result. Source inspection confirms the bridge configuration and forwarding implementation; no new compile, upload, electrical measurement or bench test was performed for this checkpoint.

No firmware bytes were changed. The packaged v1_13 hash remains recorded in [provenance](../provenance.md). The new bridge source has not yet been packaged as a build recommendation.

## Next unresolved work

1. Match March 31–April 4 IDE upload records to the saved revisions and reconcile timestamps with the photo anchors.
2. Trace `powderbot_merge_v0_1` through later merge revisions to identify the complete bridge-plus-lights baseline and its reported tests. Preserve the distinction between a merge skeleton and a working build.
3. Document that baseline's full wiring, dependencies and signal ownership before selecting a smaller ESP32. Do not apply v1_13 GPIO assignments to it.
4. Keep the original V3 in charge of dispensing; distinguish external motor experiments from the bridge reconstruction.
5. Include the upper and lower light-mount models and nozzle when their latest source/export versions are recovered. Model recovery remains parked and tracked in [issue 2](https://github.com/dankcincy/powderbot-v3-lighting/issues/2).

## Source fingerprints

SHA-256 of inspected archived files:

- `Ble/esp32_bt05_v1_8/esp32_bt05_v1_8.ino`: `a8a6718a3fe36cdefe171b5dde316e56781e377a48523831014e76c563a9b663`
- `Ble/powerbot_comms_v1/powerbot_comms_v1/powerbot_comms_v1.cpp`: `dceb537b26dad51b5e2fc2ad2d31f48d6d8041f85d82e9b1b4e139646d845f17`
- `Ble/powderbot_comms_v1_1/powderbot_comms_v1_1/powderbot_comms_v1_1.ino`: `27395d1709bc986334718dd1ee5616b24b6dbd5a4ad3b51c01041077583b12b8`
- `Ble/powderbot_comms_v1_2/powderbot_comms_v1_2/powderbot_comms_v1_2.ino`: `cacdc5daeecf546498b19959fb133938a8dbb3c7cad3de67d0da948737530f6c`
