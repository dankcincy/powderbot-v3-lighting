# Bridge signal paths and wiring evidence

[Guide home](index.md)

## Selected v0.6 GPIO map

The ESP32 replaces the stock BLE module, observes both directions of the existing V3–scale RS232 link, and drives the lights. The original V3 retains dispensing and scale-command ownership.

| Signal relative to ESP32 | GPIO | Interface |
| --- | --- | --- |
| Receive V3 replies | 34 | Logic-level UART RX |
| Transmit app commands to V3 | 22 | Logic-level UART TX |
| Receive converted scale readings | 32 | MAX3232 receiver 1 logic output |
| Receive converted V3-to-scale commands | 33 | MAX3232 receiver 2 logic output |
| LED strip data | 21 | NeoPixel data output |
| External BLE STATE | None | Native BLE callbacks |
| Scale TX | Disabled (-1) | Passive monitoring only |

V3 UART: **9600 8N1**. Scale observation: **19200 8N1**. USB diagnostics: **115200**. Strip configuration: **22 pixels, GRB, 800 kHz, brightness 100/255**.

Source: `powderbot_merge_v0_6.ino` lines 43–46 and 93–100; `powderbot_merge_config.h` lines 7–12; `powderbot_comms_v1_2.cpp` lines 13–15, 47–49 and 645. See [linked evidence](evidence/merge-v0_6-recovery.md).

These are source-confirmed assignments. April 5 reports additionally document the Bart mapping and LEDs landed on GPIO21. Neither establishes contact-by-contact continuity or measured electrical compatibility for a new assembly.

## Functional wiring

| From | To |
| --- | --- |
| V3 reply path formerly feeding stock BLE module RXD | ESP32 GPIO34 |
| ESP32 GPIO22 | V3 command path formerly driven by stock BLE module TXD |
| Tap on scale-to-V3 RS232 signal | Receiver 1 RS232 input |
| Receiver 1 logic output | GPIO32 |
| Tap on V3-to-scale RS232 signal | Receiver 2 RS232 input |
| Receiver 2 logic output | GPIO33 |
| GPIO21 | Compatible strip DIN interface |
| Converter logic supply | 3.3 V and common ground |
| ESP32, converter, V3 signal reference, strip supply | Established common ground |

Keep the original scale connection intact. Converter transmit outputs are not connected to that bus. The old module TXD/RXD labels describe the module's viewpoint; they are not physical connector numbers.

## Converter and compact-board requirements

The design needs **two independent RS232 receivers and zero RS232 transmitters** for scale observation. The V3 BLE-header path is a separate logic UART, not another RS232 port.

Use a breakout explicitly exposing `R1IN/R1OUT` and `R2IN/R2OUT`, or two modules each exposing one receiver. A transmit channel cannot serve as the second receiver. A MAX3232 chip's two-receiver capability does not prove that a particular miniature board exposes both. The exact HiLetgo Mini pad-to-chip mapping remains unverified; do not select a one-module arrangement solely from “two channels” marketing text.

The ESP-WROOM-32 D1 Mini/MiniKit layout pictured in the proposed Amazon listing exposes GPIO21, 22, 32, 33 and 34. This supports board selection, not a tested reproduction. Check the actual board revision and all electrical interfaces. No Bart motor hardware or CrowPanel is required by the selected sketch.

Minimum functional assembly: original-ESP32 board, two receiver channels, compatible 22-pixel strip, suitable regulated power, and a harness. A 5 V strip may need a suitable 3.3-to-5 V data buffer; establish its requirements and supply capacity before assembly.

## Earlier mappings are historical

| Function | Max communications | Selected v0.6 |
| --- | --- | --- |
| V3 RX / TX | 32 / 33 | 34 / 22 |
| Scale readings / commands | 21 / 35 | 32 / 33 |
| LED | Not present in comms-only builds; passive Max core used 27 | 21 |

Do not mix these tables: GPIO21 is scale RX in the Max communications builds but LED output in v0.6. The earlier [v1_13 reference](v1_13-reference.md) uses another configuration.

## Physical checks still open

- Exact V3 header contacts/orientation, power and UART logic levels.
- Scale connector contacts, converter pad mapping, supply and output voltage.
- Actual harness continuity and ground references.
- Strip identity, supply, data-pad order, level interface and power conditioning.
- Scale settings, including grain output and serial framing.
- Actual compact-board identity and successful compile/upload/bench results.

This is a functional map, not a power-up-ready assembly drawing.
