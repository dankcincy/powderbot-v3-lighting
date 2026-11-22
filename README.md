# PowderBot V3 lighting

A shared workshop guide for reconstructing the March 2026 v1_13 status-lighting setup.

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

The guide describes passive monitoring and LED indication. Firmware changes should be proposed separately with their reason and test results. The original v1_13 hash is recorded in [provenance](docs/provenance.md).

## Working together

Use one issue per question. Include the guide section, board identity, clear photos and observed results. Record resolutions in the guide with the evidence that supports them. See [contributing](CONTRIBUTING.md).
