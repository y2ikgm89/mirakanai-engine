# Android Release Device Matrix v1 Implementation Plan (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Record real Android host evidence for the current mobile package lane: Debug APK build, Release APK build with a local non-repository validation key, APK signature verification, upload certificate export, and API 36 emulator install/launch smoke for `sample_headless`.

**Status:** Completed on 2026-05-06

**Architecture:** Use the existing reviewed PowerShell/Gradle Android entrypoints. Do not move dependency bootstrap or signing into CMake. Keep signing keys under user-local storage only, package `games/<game_name>/game.agent.json` as an app asset, and treat emulator/device availability as host evidence rather than a repository runtime dependency.

**Tech Stack:** `tools/check-mobile-packaging.ps1`, `tools/build-mobile-android.ps1`, `tools/check-android-release-package.ps1`, `tools/smoke-android-package.ps1`, Android Gradle Plugin 9.1.0, Gradle 9.3.1, Android SDK/NDK, JDK 17, Android API 36 emulator.

---

## Context

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` reports Android packaging ready on this host, with Android emulator and `Mirakanai_API36` AVD ready.
- Android Release signing environment is not configured, but `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-android-release-package.ps1 -UseLocalValidationKey` can generate a local non-repository PKCS12 upload key for validation.
- Apple/iOS remains blocked on this Windows host by missing macOS/Xcode tools and is a separate host-gated split.

## Constraints

- Do not store Android keystores, passwords, SDKs, Gradle caches, or AVD images in the repository.
- Do not claim Play Store upload, production signing, real-device coverage, ABI breadth beyond the current `arm64-v8a` template, or Apple/iOS readiness.
- Do not edit Android template behavior unless validation exposes a real bug.
- Commands may write Gradle, Android SDK, and emulator state under user-local tool/cache directories.
- End with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Tasks

### Task 1: Android Host Gate Evidence

**Commands:**
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-mobile-packaging.ps1`

- [x] Confirm Android reports ready, emulator reports ready, and `Mirakanai_API36` AVD is visible.
- [x] Record Apple and Android release-signing blockers honestly.

### Task 2: Debug APK Build Evidence

**Commands:**
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-mobile-android.ps1 -Game sample_headless -Configuration Debug`

- [x] Build the Debug APK for `sample_headless`.
- [x] Record the produced APK path or failure blocker.

### Task 3: Release Signing Evidence

**Commands:**
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-android-release-package.ps1 -Game sample_headless -UseLocalValidationKey`

- [x] Generate or use a local non-repository validation upload key.
- [x] Build the Release APK.
- [x] Verify the APK with `apksigner`.
- [x] Export the upload certificate under `out/mobile/android/release`.

### Task 4: Emulator Launch Smoke Evidence

**Commands:**
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/smoke-android-package.ps1 -Game sample_headless -Configuration Release -SkipBuild -StartEmulator -AvdName Mirakanai_API36`

- [x] Install the Release APK on the API 36 emulator.
- [x] Launch `dev.mirakanai.android/.MirakanaiActivity`.
- [x] Verify the app process, then stop the app and emulator.

### Task 5: Docs, Manifest, And Static Checks

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/testing.md`
- Modify: `docs/workflows.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`

- [x] Document the exact validated Android host evidence and remaining boundaries.
- [x] Update the master plan so `android-release-device-matrix-v1` is no longer a remaining split if all required Android evidence passes.
- [x] Keep Apple/iOS and production signing/upload claims host-gated or unsupported.

### Task 6: Final Validation

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record validation evidence here and return `currentActivePlan` to the master plan.

## Done When

- Android host gate evidence is recorded from the current machine.
- `sample_headless` Debug APK builds.
- `sample_headless` Release APK builds with a local non-repository validation key, passes `apksigner verify`, and exports an upload certificate.
- The Release APK installs and launches on the configured API 36 emulator.
- Docs, manifest, static checks, and the plan registry describe the Android evidence without claiming Play upload, production signing, real-device matrix coverage, Apple/iOS readiness, or repository-owned key material.

## Validation Results

- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-mobile-packaging.ps1` reported `android=ready`, `android-emulator=ready`, `android-avd=ready (Mirakanai_API36)`, `android-release-signing=not-configured`, `android-device-smoke=not-connected`, and Apple blockers for missing macOS/Xcode tools.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-mobile-android.ps1 -Game sample_headless -Configuration Debug` initially failed because Android NDK 28 libc++ does not provide floating-point `std::from_chars`; failures were in `engine/assets/src/tilemap_metadata.cpp` and `engine/assets/src/ui_atlas_metadata.cpp`.
- FIXED: Replaced first-party floating-point `std::from_chars` metadata/session parsing with portable `std::istringstream` parsing using `std::locale::classic()` in `tilemap_metadata.cpp`, `ui_atlas_metadata.cpp`, and `session_services.cpp`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-mobile-android.ps1 -Game sample_headless -Configuration Debug` produced the Debug APK.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-android-release-package.ps1 -Game sample_headless -UseLocalValidationKey` generated `C:\Users\y2ikg\AppData\Local\GameEngine\android\keys\gameengine-local-upload-20260506-212447.p12`, built `platform/android/app/build/outputs/apk/release/app-release.apk`, verified v2 APK signing with `apksigner`, and exported `out/mobile/android/release/upload_certificate.pem`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/smoke-android-package.ps1 -Game sample_headless -Configuration Release -SkipBuild -StartEmulator -AvdName Mirakanai_API36` installed and launched the Release APK on `emulator-5584`, reporting `android-smoke: ok`.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` initially failed because the updated static contract required literal `Mirakanai_API36` and `Android Release Device Matrix v1` markers in docs.
- FIXED: Added the missing static markers to the master plan and `docs/current-capabilities.md`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` completed with `validate: ok`; it rebuilt the dev preset, ran `ctest` with 29/29 tests passing, kept shader Metal tooling and Apple packaging as diagnostic-only host blockers, and reported Android packaging ready with `android-avd=ready (Mirakanai_API36)`.
- PASS: `engine/agent/manifest.json` now returns `currentActivePlan` to `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md` and `recommendedNextPlan.id` to `next-production-gap-selection`.
