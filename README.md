# PowderBot V3 lighting

A shared workshop guide for recovering the PowderBot V3 BLE bridge and status lighting for reproduction on a smaller ESP32.

**Scope correction — November 22, 2026:** the packaged v1_13 sketch is the preserved passive-lighting baseline. It does not replace the V3 Bluetooth module. The intended reconstruction is the later ESP32 BLE replacement plus lights, retaining the original V3 dispensing controller. The complete merged baseline is still being identified. See the [bridge recovery checkpoint](docs/evidence/bridge-recovery.md).

**Current stage — evidence review and assembly planning.** The original source and historical build records are recovered. Several physical connections still require verification before an assembly can be approved for power-up.

[Read the guide](https://dankcincy.github.io/powderbot-v3-lighting/) · [Ask a question](https://github.com/dankcincy/powderbot-v3-lighting/issues/new/choose) · [Connect your AI](docs/connect-your-ai.md)

## Start here

1. Read [the setup overview](docs/index.md).
2. Check [the wiring and open connections](docs/wiring.md).
3. Follow [software preparation](docs/software.md) and the [staged check sheet](docs/verification.md).
4. Post photos and results using the question or bench-report template.

## Project files

| Location | Contents |
| --- | --- |
| `firmware/powderbot_personality_v1_13/` | Byte-preserved original Arduino sketch |
| `docs/` | Illustrated-ready guide, behavior reference and AI connection instructions |
| `docs/evidence/` | Research audits and provenance |
| `.github/ISSUE_TEMPLATE/` | Structured questions and bench reports |

The existing wiring, software, behavior and verification pages describe the historical v1_13 passive-monitoring setup; they are not instructions for the later BLE replacement. Firmware changes should be proposed separately with their reason and test results. The original v1_13 hash is recorded in [provenance](docs/provenance.md).

## Working together

Use one issue per question. Include the guide section, board identity, clear photos and observed results. Record resolutions in the guide with the evidence that supports them. See [contributing](CONTRIBUTING.md).
