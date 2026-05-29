# Renderer Backend Parity Metal Apple Evidence v1 (2026-05-29)

**Plan ID:** `renderer-backend-parity-metal-apple-evidence-v1`
**Status:** Completed.
**Owner:** `MK_renderer` / agent-surface governance
**Parent:** [Production Completion Master Plan v1](../master-plans/2026-05-03-production-completion-master-plan-v1.md)

## Goal

Select and execute the next post-1.0 renderer backend parity slice for `renderer-backend-parity-v1`: Apple/Metal host evidence. The slice must make Apple/Metal validation explicit, keep Metal readiness host-gated until macOS/Xcode evidence exists, and prevent roadmap/manifest/registry drift from hiding a stale active plan.

## Context

`Physics Navigation Commercial Coverage v1` is completed evidence. The manifest returned to the production-completion selection gate, but `docs/roadmap.md` still claimed that completed slice as active. The next selected production row is `renderer-backend-parity-v1` because the production-completion projections still identify Apple/Metal evidence as host-gated while D3D12 and strict Vulkan evidence must not promote Metal readiness.

This plan is intentionally a narrow evidence and governance candidate. It does not add a Metal backend, does not mark `metal-apple` ready, does not add SDL3, and does not expose D3D12/Vulkan/Metal/native handles through gameplay APIs.

## Official Practice Check

- Apple Metal resource synchronization requires explicit conflict reasoning and synchronization evidence before promotion: <https://developer.apple.com/documentation/metal/resource-synchronization>.
- Apple Metal capability tables are feature-family specific and must be checked per promoted feature: <https://developer.apple.com/metal/capabilities/>.
- Apple command-line validation requires full Xcode command-line tools such as `xcodebuild`, `xcrun`, and Simulator tooling on an Apple host: <https://developer.apple.com/documentation/xcode/xcode-command-line-tool-reference>.
- Context7 `/websites/cmake_cmake_help` confirms preset-driven configure/build/test flows belong in `CMakePresets.json` entries and named build/test presets, matching this repository's `tools/*.ps1` wrapper strategy.
- Android GameActivity remains separate mobile evidence and does not prove Apple readiness: <https://developer.android.com/games/agdk/game-activity/get-started>.

## Constraints

- No SDL3 reintroduction.
- No backward-compatibility shim or duplicate active plan pointer.
- No public native/RHI/Metal handles.
- Windows validation may only record Apple/Metal as diagnostic host-gated evidence.
- `engine/agent/manifest.json` must be composed from fragments, not hand-edited.
- The active plan, `recommendedNextPlan`, plan registry, roadmap, master-plan prose, and static checks must agree.

## Tasks

- [x] Add a fail-closed static guard that catches stale roadmap active-plan prose.
- [x] Switch `currentActivePlan` and `recommendedNextPlan` to this plan through `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, then compose `engine/agent/manifest.json`.
- [x] Update the plan registry, roadmap, production master plan, and projections chapter so they name this active selection and keep `unsupportedProductionGaps = []`.
- [x] Update static contract branches for `renderer-backend-parity-metal-apple-evidence-v1`.
- [x] Run focused docs/agent validation and record evidence.
- [x] Publish as a validated candidate PR and wait for hosted checks.
- [x] Close the active pointer back to the production-completion selection gate after PR #296 merge.

## Done When

- `docs/roadmap.md` no longer claims the completed physics/navigation slice is active.
- During execution, the manifest active path and `recommendedNextPlan.id`/`path` pointed to this plan.
- After PR #296 merge, `currentActivePlan` returns to the production-completion master plan and `recommendedNextPlan.id = next-production-gap-selection`.
- `metal-apple` remains `host-gated` with `shader-toolchain`, `mobile-packaging`, and `ios-simulator-smoke` as validation recipes.
- Static checks reject mismatched active plan, stale roadmap active prose, and missing renderer/Metal evidence-selection needles.
- Local validation evidence includes `compose-agent-manifest`, `check-json-contracts`, `check-ai-integration`, `check-production-readiness-audit`, `check-text-format`, and `git diff --check`.
- Hosted PR evidence includes PR #296 merge with PR Gate, macOS Metal CMake, iOS Simulator smoke, Windows MSVC, Linux CMake, full repository static analysis shards, and CodeQL success.

## Validation Evidence

| Check | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` after static guard only | Failed as expected | RED evidence: `docs/roadmap.md must not claim the completed physics/navigation slice is active`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Passed | Regenerated `engine/agent/manifest.json` from fragments. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | `agent-manifest-compose: ok`; `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | `ai-integration-check: ok`; active plan and roadmap drift guard passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Passed | `unsupported_gaps=0`; `production-readiness-audit-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` | Passed | `text-format-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed | `agent-config-check: ok`. |
| `git diff --check` | Passed | No whitespace errors. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 -StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120` | Passed | `validate: static ok`; Apple/Metal and Apple mobile checks remained diagnostic host gates on this Windows host. |
| PR #296 hosted checks | Passed | PR Gate, macOS Metal CMake, iOS Simulator smoke, Windows MSVC, Linux CMake, full repository static analysis shards, and CodeQL succeeded; merged as `6542c223e349912a42bb490528ded9ef2c7d0a80`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` during closeout | Passed | Regenerated `engine/agent/manifest.json` after returning to `next-production-gap-selection`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` during closeout | Passed | `agent-manifest-compose: ok`; `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` during closeout | Passed | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` during closeout | Passed | `unsupported_gaps=0`; `production-readiness-audit-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` during closeout | Passed | `text-format-check: ok`. |
| `git diff --check` during closeout | Passed | No whitespace errors. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 -StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120` during closeout | Passed | `validate: static ok`; Apple/Metal and Apple mobile checks remained diagnostic host gates on this Windows host. |
