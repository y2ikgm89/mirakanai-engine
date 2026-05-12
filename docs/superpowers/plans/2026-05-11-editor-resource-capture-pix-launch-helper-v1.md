# Editor Resource Capture PIX Host Helper v1 (2026-05-11)

**Plan ID:** `editor-resource-capture-pix-launch-helper-v1`  
**Gap:** `editor-productization`  
**Parent:** [2026-05-11-editor-productization-resource-management-execution-stream-v1.md](2026-05-11-editor-productization-resource-management-execution-stream-v1.md)  
**Status:** Completed (host-operator helper only; no editor-core mutation)

## Goal

Provide an **optional** repository `tools/` entrypoint that helps Windows operators launch **Microsoft PIX** from a **non-repository scratch directory** under `%LocalAppData%`, matching the resource execution stream item 2 intent without bundling PIX, auto-launching from `MK_editor`, or recording native handles in the engine.

## Context

- [Editor Resource Capture Execution Evidence v1](2026-05-07-editor-resource-capture-execution-evidence-v1.md) keeps PIX launch out of `editor/core`; execution evidence remains caller-supplied.
- [Resource management execution stream](2026-05-11-editor-productization-resource-management-execution-stream-v1.md) lists an optional PIX wrapper as stream item 2.

## Constraints

- PowerShell 7 Core only; follow `tools/*.ps1` `#requires` contract and UTF-8 without BOM (`tools/check-agents.ps1`).
- No writes under the repository root except this script and docs; scratch data lives under `%LocalAppData%\MirakanaiEngine\pix-host-helper\...`.
- Do not claim `requiredBeforeReadyClaim` clearance for resource capture execution; this is operator ergonomics only.

## Done when

- [x] `tools/launch-pix-host-helper.ps1` resolves `PIX.exe` via `MIRAKANA_PIX_EXE`, standard `Program Files` paths, or `PATH`, creates a session scratch directory, writes a short `README.txt` there, and starts PIX with that working directory when found.
- [x] Script fails fast with a clear message when PIX is not installed (exit code non-zero).
- [x] Stream ledger item 2 marked complete with link to this plan.
- [x] Plan registry lists this slice as latest completed evidence where appropriate.
- [x] `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` `editor-productization` notes reference this helper; composed manifest regenerated.

## Validation

| Check | Result |
| --- | --- |
| `tools/check-agents.ps1` (via `validate.ps1`) accepts new `tools/*.ps1` | PASS |
| `tools/launch-pix-host-helper.ps1 -SkipLaunch` on host without PIX | PASS (expected `Write-Error`, exit code 1) |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS |

## Operator usage

```text
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/launch-pix-host-helper.ps1
```

Optional: set `MIRAKANA_PIX_EXE` to the full path of `WinPix.exe` or legacy `PIX.exe` when Microsoft PIX is installed outside default locations.
