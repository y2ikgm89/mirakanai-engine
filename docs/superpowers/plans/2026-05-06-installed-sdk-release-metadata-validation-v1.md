# Installed SDK Release Metadata Validation v1 Implementation Plan (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the release SDK validator prove the installed AI manifest, JSON schemas, tools, examples, samples, docs, notices, and license payloads that `docs/release.md` already claims are part of the SDK.

**Status:** Completed on 2026-05-06

**Architecture:** Keep the validation in PowerShell release tooling, not in CMake configure or generated package metadata. Add a small testable helper that inspects the installed SDK layout and is reused by `validate-installed-sdk.ps1`; it does not build, install, or package anything by itself.

**Tech Stack:** PowerShell validation scripts, CMake installed SDK layout, `examples/installed_consumer`, `engine/agent/manifest.json`, schemas, docs/manifest/static AI guidance sync.

---

## Context

- The master plan requires honest release and operations recipes, including desktop package release validation.
- `docs/release.md` says `validate-installed-sdk` fails when installed public headers, schemas, and the AI manifest are missing.
- Current `tools/validate-installed-sdk.ps1` verifies the CMake package config and then builds/runs `examples/installed_consumer`, but it does not directly check the installed metadata payloads listed in the release docs.
- CMake already installs `engine/agent/manifest.json`, `schemas/*.json`, tools, examples, samples, docs, licenses, and notices under the release SDK layout.

## Constraints

- Do not add CMake configure-time package restore, new third-party dependencies, or broad release signing/upload claims.
- Do not make Apple, Android, desktop runtime, or editor package readiness broader than existing host gates.
- Keep the helper deterministic and file-system-only so it can be unit-checked without a full SDK build.
- Keep `validate-installed-sdk.ps1` responsible for installed SDK checks; keep `tools/package.ps1` as the release build/install/consumer/package orchestrator.
- End with focused script checks and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Tasks

### Task 1: RED Metadata Validator Check

**Files:**
- Create: `tools/check-installed-sdk-validation.ps1`

- [x] Add a script check that creates a fake install prefix with the required metadata files, then removes `share/GameEngine/manifest.json` and expects the installed SDK metadata assertion to fail.
- [x] The RED check must call a helper named `Assert-InstalledSdkMetadata` from `tools/installed-sdk-validation.ps1`, which does not exist yet.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-installed-sdk-validation.ps1` and confirm RED because the helper script or function is missing.

### Task 2: Implement Installed Metadata Helper

**Files:**
- Create: `tools/installed-sdk-validation.ps1`
- Modify: `tools/validate-installed-sdk.ps1`

- [x] Add `Assert-InstalledSdkMetadata -InstallPrefix <path>` that checks:
  - `lib/cmake/GameEngine/GameEngineConfig.cmake`
  - `lib/cmake/GameEngine/GameEngineConfigVersion.cmake`
  - `share/GameEngine/manifest.json`
  - `share/GameEngine/schemas/engine-agent.schema.json`
  - `share/GameEngine/schemas/game-agent.schema.json`
  - `share/GameEngine/tools/validate.ps1`
  - `share/GameEngine/tools/agent-context.ps1`
  - `share/GameEngine/examples/installed_consumer/CMakeLists.txt`
  - `share/GameEngine/examples/installed_consumer/main.cpp`
  - at least one installed sample `game.agent.json`
  - `share/doc/GameEngine/README.md`
  - `share/GameEngine/THIRD_PARTY_NOTICES.md`
  - `share/GameEngine/LICENSES/LicenseRef-Proprietary.txt`
- [x] Parse the installed manifest and schemas as JSON so malformed payloads fail early.
- [x] Reject missing/empty installed tools, examples, docs, notices, and license files.
- [x] Dot-source the helper from `tools/validate-installed-sdk.ps1` and run it before configuring the installed consumer.

### Task 3: Static Validation Wiring

**Files:**
- Modify: `tools/validate.ps1`
- Modify: `package.json`

- [x] Add a `check-installed-sdk-validation` npm script.
- [x] Run `tools/check-installed-sdk-validation.ps1` from `tools/validate.ps1` before full CMake build/test work.
- [x] Keep this check fake-install-only; it must not require `cmake --install`, CPack, or a compiler.

### Task 4: Docs, Manifest, And Plan Sync

**Files:**
- Modify: `docs/release.md`
- Modify: `docs/testing.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`

- [x] Document that release validation now directly checks installed SDK metadata before the installed consumer build.
- [x] Keep ready claims narrow: this is SDK payload validation, not release signing, upload, desktop runtime target validation, Apple validation, or broad operations telemetry.
- [x] Update the master plan, registry, manifest recommended next-plan reason, and static AI integration checks so the new release evidence is not dropped.

### Task 5: Validation

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-installed-sdk-validation.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record validation evidence here and return `currentActivePlan` to the master plan.

## Done When

- `validate-installed-sdk.ps1` proves the installed release SDK contains parseable AI manifest/schema payloads and non-empty installed tools, examples, samples, docs, notices, and license files before building the installed consumer.
- A fake-install script check proves missing manifest metadata is rejected without requiring a full build.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` includes the new script check.
- Docs, manifest, static checks, and the plan registry describe the release validation boundary honestly.

## Validation Results

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-installed-sdk-validation.ps1` failed before implementation with missing helper evidence: `Missing installed SDK validation helper: G:\workspace\development\GameEngine\tools\installed-sdk-validation.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-installed-sdk-validation.ps1` -> `installed-sdk-validation-check: ok`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-installed-sdk-validation.ps1` -> `installed-sdk-validation-check: ok`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` -> `format-check: ok`.
- PASS: `git diff --check` exited 0; PowerShell reported only LF-to-CRLF whitespace warnings.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` -> `ai-integration-check: ok`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package.ps1` completed Release configure/build, 29/29 CTest tests, install, installed consumer configure/build/test, `installed-sdk-validation: ok`, and CPack ZIP/SHA256 generation.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` completed after the plan record was updated.
- `engine/agent/manifest.json` keeps `currentActivePlan` on the master plan and returns `recommendedNextPlan.id` to `next-production-gap-selection`.
