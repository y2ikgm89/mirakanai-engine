# Apple Metal iOS Host Evidence v1 Implementation Plan (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a single repository validation entrypoint that records Apple/iOS/Metal host readiness or explicit host gates without promoting Apple readiness on non-Apple hosts.

**Status:** Completed on 2026-05-06

**Architecture:** Keep Apple validation host-owned and tool-driven. The new PowerShell check verifies repository templates, macOS/full-Xcode requirements, iOS Simulator SDK/runtime availability, Xcode-resolved Metal compiler tools, and CI workflow coverage, but defaults to diagnostic-only host-gated output so Windows `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` remains honest.

**Tech Stack:** PowerShell 7, `tools/common.ps1`, Apple `xcode-select` / `xcodebuild` / `xcrun simctl`, Metal `metal` / `metallib` resolved through `xcrun`, GitHub Actions macOS runners, CMake/Xcode iOS template.

---

## Context

- The master plan allows Apple/iOS/Metal to be either validated on matching hosts/CI or explicitly host-gated.
- This Windows host reports `apple=blocked`, missing `xcodebuild`, missing `xcrun`, missing `metal`, and missing `metallib`.
- Apple documentation says full Xcode is required for `xcodebuild`, `simctl`, and other app-only command-line tools; Command Line Tools alone are not enough for iOS Simulator validation.
- Apple Metal documentation uses command-line Metal compiler tooling through `xcrun` to build Metal libraries.
- GitHub Actions documentation lists versioned macOS runner labels and states `-latest` labels track GitHub's latest stable image, not necessarily the newest OS from Apple.

## Constraints

- Do not claim Apple/iOS/Metal readiness on this Windows host.
- Do not store Apple signing identities, provisioning profiles, certificates, SDKs, simulator runtimes, or Xcode-derived data in the repository.
- Do not require Apple signing for Debug Simulator smoke; report signing separately as configured or not configured.
- Do not expose native Metal handles or add renderer/RHI readiness claims.
- End with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Tasks

### Task 1: Static Contract RED

**Files:**
- Modify: `tools/check-ai-integration.ps1`

- [x] Add assertions that require `tools/check-apple-host-evidence.ps1`, `apple-host-evidence-check`, `Apple Metal iOS Host Evidence v1`, `xcrun`, `metal`, `metallib`, and the new plan registry entry.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Record the expected RED failure for the missing script or docs markers.

### Task 2: Host Evidence Script

**Files:**
- Create: `tools/check-apple-host-evidence.ps1`
- Modify: `package.json`
- Modify: `tools/validate.ps1`

- [x] Implement `tools/check-apple-host-evidence.ps1` with diagnostic-only default output.
- [x] Require repository template/workflow files for `platform/ios`, `tools/build-mobile-apple.ps1`, `tools/smoke-ios-package.ps1`, `.github/workflows/ios-validate.yml`, and `.github/workflows/validate.yml`.
- [x] Report `host`, `xcode`, `ios-simulator`, `metal-library`, `ios-signing`, `workflow-ios`, `workflow-macos-metal`, each blocker, and `apple-host-evidence-check: ready|host-gated`.
- [x] Add `-RequireReady` so Apple-ready hosts can make missing Xcode/iOS/Metal prerequisites fail.
- [x] Add `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-apple-host-evidence.ps1` and include the diagnostic check in `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

### Task 3: Docs, Manifest, And Plan Registry

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/testing.md`
- Modify: `docs/workflows.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`

- [x] Document the new Apple host evidence command and this Windows host's expected host-gated result.
- [x] Update the master plan so `apple-metal-ios-host-evidence-v1` is complete as a diagnostic contract, while Apple/iOS/Metal readiness remains host-gated until macOS/Xcode or CI evidence exists.
- [x] Keep `currentActivePlan` on the master plan and `recommendedNextPlan.id` as `next-production-gap-selection` after this slice completes.

### Task 4: Validation

**Commands:**
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-apple-host-evidence.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`

- [x] Verify `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-apple-host-evidence.ps1` exits 0 with `apple-host-evidence-check: host-gated` on this Windows host.
- [x] Verify `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passes.
- [x] Verify `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes with Apple/Metal blockers diagnostic-only.

## Done When

- Apple/iOS/Metal host evidence has one explicit repository command.
- Windows validation records host gates instead of false Apple readiness.
- macOS/Xcode hosts can opt into `-RequireReady` for hard Apple evidence.
- Docs, manifest, static checks, and the master plan agree on the Apple/iOS/Metal boundary.

## Validation Results

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed because `engine/agent/manifest.json aiOperableProductionLoop.recommendedNextPlan.reason` did not contain `Apple Metal iOS Host Evidence v1`.
- BUG: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-apple-host-evidence.ps1` initially failed on Windows because `$isMacOS` conflicted with PowerShell's read-only `$IsMacOS` automatic variable and because missing `xcrun` was passed to mandatory string parameters before it could become a host gate.
- FIXED: Renamed the host OS variable and made Xcode/`xcrun` probe helpers accept empty values and return `false`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-apple-host-evidence.ps1` reported `host=windows`, `xcode=blocked`, `ios-simulator=blocked`, `metal-library=blocked`, `ios-signing=not-configured`, both Apple workflow markers present, and `apple-host-evidence-check: host-gated`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` completed with `validate: ok`; it ran `apple-host-evidence-check` as part of validation, kept Metal shader tools and Apple packaging diagnostic-only on this Windows host, rebuilt the dev preset, and ran `ctest` with 29/29 tests passing.
- SOURCES: Apple Xcode command-line tool documentation confirms full Xcode is required for tools such as `xcodebuild`/`simctl`; Apple Metal documentation describes command-line `metal`/`metallib` shader library builds through `xcrun`; GitHub Actions documentation is the source for macOS hosted runner labels and hosted-runner behavior.
