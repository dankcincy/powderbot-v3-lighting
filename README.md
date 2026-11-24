# PowderBot V3 bridge and lighting

A shared workshop guide for reproducing an ESP32 BLE bridge and status lights for the AutoTrickler V3.

The ESP32 replaces the stock BLE adapter, passes phone-app commands and V3 replies between them, and observes scale readings to drive the lights. The original V3 remains responsible for dispensing. The intended reproduction uses a smaller ESP32 without a display.

**Recovered replication baseline: `powderbot_merge_v0_6`.** The April 5, 2026 session reports document the working native-BLE bridge and refined lighting while the original V3 still controlled dispensing. The complete archived sketch is packaged unchanged. A compact-board reproduction has not yet been compiled or bench-tested; physical wiring and dependency versions remain to be verified.

[Read the guide](https://dankcincy.github.io/powderbot-v3-lighting/) · [Ask a question](https://github.com/dankcincy/powderbot-v3-lighting/issues/new/choose) · [Connect your AI](docs/connect-your-ai.md)

## Start here

1. Read the [system overview](docs/index.md).
2. Review the [signal paths and wiring evidence](docs/wiring.md).
3. Check the [firmware recovery status](docs/software.md) and [verification plan](docs/verification.md).
4. Share version-specific photos and results through an issue.

## Project files

| Location | Contents |
| --- | --- |
| `docs/` | Current understanding, open questions and reproduction plan |
| `docs/evidence/` | Source audits, dated findings and recovery checkpoints |
| `firmware/powderbot_merge_v0_6/` | Complete unchanged replication baseline, including all supporting modules |
| `firmware/powderbot_personality_v1_13/` | Preserved earlier passive-lighting sketch; not the BLE replacement |
| `.github/ISSUE_TEMPLATE/` | Questions and bench reports |

Start with the [v0.6 recovery evidence](docs/evidence/merge-v0_6-recovery.md) and [software instructions](docs/software.md). The [historical v1_13 reference](docs/v1_13-reference.md) preserves the earlier build instructions and behavior. Its source hash is recorded in [provenance](docs/provenance.md).

Printable upper/lower light-mount parts and the nozzle are intended for inclusion; recovery of the latest models is tracked in [issue 2](https://github.com/dankcincy/powderbot-v3-lighting/issues/2).

## Working together

Use one issue per question. Include hardware identity, source revision, photos and observations. Record resolutions in the guide with supporting evidence. See [contributing](CONTRIBUTING.md).
