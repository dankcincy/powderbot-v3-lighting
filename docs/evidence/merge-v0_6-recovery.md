# v0.6 baseline recovery — November 23, 2026

[Guide home](../index.md)

## Decision

Dan selected **powderbot_merge_v0_6, including every supporting file**, as the recovered replication baseline. It is the last recovered complete native-BLE/V3 bridge and refined-lighting build in the inspected archive. This does not assert that no other private or lost revision ever existed.

The original V3 remains the dispensing controller. Bart's-board pin mapping and independent PowderBot motor control were separate transitions. v1_13 remains preserved as historical passive-lighting evidence.

Owner/status: Codex, authorized repository packaging and documentation update. Original local sources were read only. This update packages their bytes; it does not develop new firmware. AI-assisted recovery was reviewed against the original source and dated project reports.

## Primary historical evidence

Both original DOCX reports remain in the maintainer's `Project Reports/` archive. This page publishes the relevant findings without unrelated project material.

| Report | Date inside document | Relevant sections |
| --- | --- | --- |
| `PowderBot_Merge_Session_Report_v1_1.docx` | April 5, 2026 | Sections 1–4: comms baseline, cleanup sequence, visual bench refinements, end-of-session selection |
| `PowderBot_Merge_Cleanup_and_Visual_Refinement_Session_Report_v1_0.docx` | April 5, 2026 | Sections 1, 2, 4 and 6: Bart mapping, GPIO21 LEDs, v0.5 checkpoint, v0.6 visual branch |

The fuller v1.1 report records:

- `powderbot_comms_v1_2` compiled, uploaded and functioned correctly, as reported by the user.
- v0.1 was a functioning rebuild anchor, with LEDs on GPIO21.
- v0.3 serial controls worked after a compile fix; v0.4 was reported good after testing.
- v0.5 was reported okay in bench use and retained as the known-good cleanup checkpoint.
- v0.6 was the end-of-session visual build: center-out result reveal, outside-in reset collapse, and visual verdict filtering tuned to 500 ms.
- The user changed both saved visual-delay defaults to 500 ms.
- Motion integration remained explicitly deferred. The recommended restart was v0.5 for plumbing alone or v0.6 when the refined visuals were required.

These are **historical documentary claims**, not newly witnessed bench results or a recovered upload log uniquely identifying every tested byte.

Report SHA-256:
- v1.1 merge-session report: `103105b97b1da5b020bd04b58f0dbbb3371d2f62b90454fd6c6c280870f9cd38`
- v1.0 cleanup/refinement report: `9f8f00c2ba2f1a43ff196cf64bdd43933febbadb5891a8829512db3658ffc233`

## Source confirmation

Links below point to the packaged unchanged source. Line numbers refer to those original bytes.

| Finding | Source |
| --- | --- |
| BLE app bytes forwarded directly to V3 | [powderbot_comms_v1_2.cpp](../../firmware/powderbot_merge_v0_6/powderbot_comms_v1_2.cpp), lines 466–487 |
| Actual buffered V3 replies notified to BLE | Same CPP, lines 371–377 |
| V3 RX34/TX22, scale RX32/command RX33, passive mode and disabled scale TX | [powderbot_merge_v0_6.ino](../../firmware/powderbot_merge_v0_6/powderbot_merge_v0_6.ino), lines 93–100 |
| UART2 V3, UART1 scale, software serial command tap | Comms CPP, lines 47–49, 403 and 422–426 |
| V3 9600 baud, 8N1 | Comms CPP, lines 15 and 645 |
| LED GPIO21, 22 pixels, brightness 100; USB 115200 | [powderbot_merge_config.h](../../firmware/powderbot_merge_v0_6/powderbot_merge_config.h), lines 7–12 |
| GRB / 800 kHz strip | Main INO, lines 43–46 |
| Default tolerances each 0.02 grains; visual delay 500 ms | Merge config, lines 28–32 |
| Presentation internal default also 500 ms | [powderbot_presentation.h](../../firmware/powderbot_merge_v0_6/powderbot_presentation.h), line 37 |
| Commands/scale observations mapped into lighting engine | [powderbot_merge_adapter.cpp](../../firmware/powderbot_merge_v0_6/powderbot_merge_adapter.cpp), lines 41–83 |

The sketch includes no motor-driver or touchscreen dependency. Its use of a “machine engine” describes observed target/scale/verdict state; it is not evidence of direct motor control.

The original `MERGE_NOTES.txt` retains a v0.3 heading and “first merge skeleton” wording. The INO also calls this not the final machine-control merge. These comments are preserved, not silently corrected. The later April 5 report and executable source establish the relevant distinction: working bridge-plus-lights, with future direct motion still deferred.

## Where the independent machine branch appears

| Archived source | Evidence | Interpretation |
| --- | --- | --- |
| `powderbot_merge_v0_6` | April 5 reports and forwarding source | Complete bridge plus lights; V3 controls dispensing |
| `Command Testing/Bart/bart_bulk_v1` | Saved April 9; header and `forwardToV3()` pass Crow commands to V3 | Still a V3-controlled experiment; LEDs are diagnostics |
| `Command Testing/Married/Bart/pb_link_bart_v1` | Saved April 16; TMCStepper, explicit STEP/DIR lanes and emitted step pulses | Direct independent motor-control branch |
| `pb_link_bart_v1_1` | Saved April 16; direct `ScaleSerial` writes as well as motor control | Direct scale integration added |
| `z_trickler_testing/pb_hotshot_v0_4_auto_bulk` | Saved April 21; auto-bulk, trickler packets and scale feedback | Combined independent dispensing experiment |

Later dates in this table are file timestamps, **not first-use or upload proofs**. Motor experiments also existed in March; this was overlapping development rather than one clean chronological fork. No v0.7 successor or single physical disconnection date was established.

## Packaged identity and verification

Archive source: `Firmware/Arduino/Sketchbook/powderbot_merge_v0_6/powderbot_merge_v0_6.zip`.

Original ZIP SHA-256:
`fd655525f04acaefcf7dc1d90e561e05325adf9398af6d8a1efa09f9b4d3bfa5`

Archive comparison confirms all 15 code files match byte-for-byte. The ZIP names its notes file `MERGE_NOTES.md`; the extracted folder names it `MERGE_NOTES.txt`, with identical bytes. Packaging retains the extracted folder's filename.

All 16 original sketch/support files are included under [firmware/powderbot_merge_v0_6](../../firmware/powderbot_merge_v0_6/). The [SHA-256 manifest](../../firmware/powderbot_merge_v0_6/SHA256SUMS.txt) records their exact bytes. No source cleanup, pin remapping or line-ending conversion is intended. The manifest itself is new packaging metadata.

Recovery checks: source/archive comparison, saved 500 ms defaults, original file fingerprints and guide consistency. No compile, flash, electrical measurement or hardware test was performed during this recovery.

## Exact next step

Identify the actual compact ESP32 and converter pinout; establish dependency/core versions and compile the unchanged package; document physical endpoints/voltages and then test the new assembly using the [verification plan](../verification.md). Original wiring gaps remain in [wiring](../wiring.md). No additional firmware development is implied.
