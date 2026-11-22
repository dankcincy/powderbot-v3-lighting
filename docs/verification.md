# Bridge reconstruction verification plan

[Guide home](index.md)

The current work is baseline recovery. No complete bridge-plus-lights package has yet been selected or newly bench-tested.

1. **Identify the baseline.** Trace the bridge and merge revisions, match available upload logs and reports, and record source hashes. Do not infer the sketch from a photo or folder timestamp alone.
2. **Define the assembly.** Identify the ESP32, V3 interface, converter, strip and supplies. Record the selected firmware's GPIO map and every physical endpoint.
3. **Review wiring with power disconnected.** Establish continuity, voltage compatibility and ground references. Resolve the open items in [wiring](wiring.md).
4. **Compile the complete package.** Record dependencies, board settings and full output. Keep original source intact.
5. **Test the bridge.** Record app connection, actual command forwarding, actual V3 replies, disconnect and repeated reconnect behavior under agreed bench conditions.
6. **Test scale observation.** Capture readings and command-side traffic separately. Confirm the selected configuration leaves scale command ownership with the V3.
7. **Test lights against inputs.** Record target, raw readings, timing, verdict changes, cup removal and loss-of-data behavior. Compare with the selected source revision.
8. **Review before porting.** Select a smaller ESP32 only after confirming the required GPIOs, UART resources, BLE support and electrical interfaces; repeat relevant tests on the new assembly.

Use a bench-report issue. State who tested, when, hardware, source revision, environment, connections, observed results and unresolved items. Mark unperformed steps explicitly. Historical reported success does not replace testing a new assembly.

The earlier passive-lighting check sheet remains in the [v1_13 reference](v1_13-reference.md).
