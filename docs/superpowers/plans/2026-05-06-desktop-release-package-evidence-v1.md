# Desktop Release Package Evidence v1 Implementation Plan (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the default desktop SDK release package lane prove the CPack ZIP artifact and SHA-256 sidecar it creates, instead of only proving the install tree and installed consumer.

**Status:** Completed on 2026-05-06

**Architecture:** Keep artifact validation in PowerShell release tooling after CPack runs. Derive the expected package basename from `CPackConfig.cmake`, validate exactly that ZIP and `.sha256`, inspect the ZIP central directory through .NET archive APIs, and avoid extracting or executing package contents.

**Tech Stack:** PowerShell validation scripts, CPack `release` preset, .NET `System.IO.Compression.ZipArchive`, installed SDK layout, docs/manifest/static AI guidance sync.

---

## Context

- The master plan still lists `desktop-release-package-evidence-v1` as a remaining platform/release split after installed SDK metadata validation.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package.ps1` already configures/builds/tests the release preset, installs to `out/install/release`, validates installed SDK metadata and the clean installed consumer, then runs `cpack --preset release`.
- CMake configures `CPACK_PACKAGE_FILE_NAME`, `CPACK_GENERATOR=ZIP`, and `CPACK_PACKAGE_CHECKSUM=SHA256`; CPack currently emits a ZIP and `.zip.sha256` sidecar under `out/build/release`.
- The package script does not currently prove that the checksum sidecar matches the ZIP or that the archive contains the expected SDK payload entries.

## Constraints

- Do not add signing, notarization, upload, symbol publication, crash telemetry, Android, Apple, or desktop-runtime selected-game readiness claims.
- Do not extract the package into the source tree or execute anything from the package.
- Keep the validation host-feasible on Windows PowerShell with built-in .NET archive/hash APIs.
- Keep stale artifacts from satisfying the check: validate the package basename derived from the current build's `CPackConfig.cmake`.
- End with focused script checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Tasks

### Task 1: RED Artifact Validator Check

**Files:**
- Create: `tools/check-release-package-artifacts.ps1`

- [x] Add a fake package build directory test that writes a minimal `CPackConfig.cmake`, a ZIP, and a `.zip.sha256` sidecar.
- [x] Call a helper named `Assert-ReleasePackageArtifacts` from `tools/release-package-artifacts.ps1`, which does not exist yet.
- [x] Include negative cases for missing checksum sidecar, checksum mismatch, and missing required archive entries.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-release-package-artifacts.ps1` and confirm RED because the helper script or function is missing.

### Task 2: Implement Release Artifact Helper

**Files:**
- Create: `tools/release-package-artifacts.ps1`
- Modify: `tools/package.ps1`

- [x] Add `Assert-ReleasePackageArtifacts -BuildDir <path>` that reads `CPackConfig.cmake` and derives `<CPACK_PACKAGE_FILE_NAME>.zip`.
- [x] Require exactly the expected ZIP and `.zip.sha256` sidecar, reject missing or empty files, and ignore unrelated stale ZIPs.
- [x] Verify the sidecar's SHA-256 hash matches the ZIP bytes.
- [x] Inspect the ZIP central directory and require non-empty package entries for:
  - `lib/cmake/GameEngine/GameEngineConfig.cmake`
  - `lib/cmake/GameEngine/GameEngineConfigVersion.cmake`
  - `share/GameEngine/manifest.json`
  - `share/GameEngine/schemas/engine-agent.schema.json`
  - `share/GameEngine/schemas/game-agent.schema.json`
  - `share/GameEngine/tools/validate.ps1`
  - `share/GameEngine/tools/agent-context.ps1`
  - `share/GameEngine/examples/installed_consumer/CMakeLists.txt`
  - `share/doc/GameEngine/README.md`
  - `share/GameEngine/THIRD_PARTY_NOTICES.md`
  - `share/GameEngine/LICENSES/LicenseRef-Proprietary.txt`
- [x] Run the helper after `cpack --preset release` in `tools/package.ps1`.

### Task 3: Static Validation Wiring

**Files:**
- Modify: `tools/validate.ps1`
- Modify: `package.json`

- [x] Add a `check-release-package-artifacts` npm script.
- [x] Run `tools/check-release-package-artifacts.ps1` from `tools/validate.ps1`.
- [x] Keep this check fake-artifact-only; it must not require CMake, CPack, a compiler, or a real release build.

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

- [x] Document that `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package.ps1` now validates the current CPack ZIP artifact and SHA-256 sidecar after CPack generation.
- [x] Keep ready claims narrow: this is desktop SDK artifact integrity evidence, not signing, notarization, upload, symbol publication, crash telemetry, desktop-runtime package proof, Android matrix, or Apple-host validation.
- [x] Update the master plan, registry, manifest recommended next-plan reason, and static AI integration checks so the release evidence is not dropped.

### Task 5: Validation

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-release-package-artifacts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-release-package-artifacts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record validation evidence here and return `currentActivePlan` to the master plan.

## Done When

- `tools/package.ps1` proves the exact current CPack ZIP and `.zip.sha256` sidecar after package generation.
- The checksum sidecar is parsed and compared with the actual ZIP hash.
- The ZIP archive is inspected without extraction and required SDK metadata, tools, examples, docs, notices, and license entries are present and non-empty.
- A fake-artifact script check proves missing checksum, checksum mismatch, and missing archive entries fail without requiring a full build.
- Docs, manifest, static checks, and the plan registry describe the desktop release package evidence boundary honestly.

## Validation Results

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-release-package-artifacts.ps1` failed before implementation with missing helper evidence: `Missing release package artifact validation helper: G:\workspace\development\GameEngine\tools\release-package-artifacts.ps1`.
- FIXED: The first helper implementation exposed a PowerShell parser error for `$VariableName:` string interpolation; the helper now uses `${VariableName}` for the error text.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-release-package-artifacts.ps1` -> `release-package-artifacts-check: ok`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-release-package-artifacts.ps1` -> `release-package-artifacts-check: ok`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` -> `format-check: ok`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` -> `ai-integration-check: ok`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package.ps1` completed Release configure/build, 29/29 CTest tests, install, installed consumer configure/build/test, `installed-sdk-validation: ok`, CPack ZIP/SHA256 generation, and `release-package-artifacts: ok`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` completed after the plan record was updated.
- `engine/agent/manifest.json` keeps `currentActivePlan` on the master plan and returns `recommendedNextPlan.id` to `next-production-gap-selection`.
