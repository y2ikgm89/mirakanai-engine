# Android Release Smoke Hardening Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the local Android release validation lane deterministic on Windows by proving Debug/Release APK builds, non-repository upload-key signing, and emulator install/launch smoke for `sample_headless`.

**Architecture:** Keep Android packaging in `platform/android` and workflow scripts, not in public engine APIs. Signing secrets stay outside the repository, emulator state stays under the user Android home, and `tools/common.ps1` normalizes `ANDROID_AVD_HOME` before emulator discovery so AVD lookup is deterministic in sandboxed and normal shells.

**Tech Stack:** PowerShell workflow scripts, Android Gradle Plugin 9.1.0, Gradle 9.3.1, JDK 17, Android SDK Platform 36.1, Android Emulator, `adb`, `apksigner`, existing GameActivity template.

---

## Context

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-mobile-packaging.ps1` reported Android SDK/NDK/JDK/Gradle/emulator readiness on this host.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-mobile-android.ps1` initially failed in the sandbox because Gradle needs write access to the user Gradle native cache under `C:\Users\<user>\.gradle\native`.
- Running the Gradle-backed Android lanes outside the sandbox is required for local package validation.
- The emulator AVD existed under `%USERPROFILE%\.android\avd`, but `emulator -list-avds` returned no entries unless `ANDROID_AVD_HOME` was explicitly set.
- Official Android guidance expects release artifacts to be signed, app signing/upload keys to be kept secure, and emulator/ADB command-line smoke to build, install, launch, and test an APK.

## Official References

- Android app signing: https://developer.android.com/studio/publish/app-signing
- Android Gradle Plugin signing config API: https://developer.android.com/reference/tools/gradle-api/8.13/com/android/build/api/dsl/SigningConfig
- Android Emulator command line and `-list-avds`: https://developer.android.com/studio/run/emulator-commandline
- Android Debug Bridge install/device targeting: https://developer.android.com/studio/command-line/adb

## Constraints

- Do not check in keystores, passwords, AVD images, Gradle caches, or SDK binaries.
- Do not expose Android SDK handles or platform APIs through public game/engine headers.
- Keep the default `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` blocker-aware and diagnostic-only for host/device state.
- Keep release signing verification explicit through `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-android-release-package.ps1`.
- Keep device/emulator launch verification explicit through `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/smoke-android-package.ps1`.

## Done When

- [x] Android Debug APK builds for `sample_headless`.
- [x] Android Release APK builds with a local non-repository validation upload key.
- [x] `apksigner verify --print-certs` verifies the Release APK.
- [x] A local `GameEngine_API36` AVD exists for smoke validation.
- [x] `tools/smoke-android-package.ps1` can discover that AVD without requiring callers to pre-set `ANDROID_AVD_HOME`.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/smoke-android-package.ps1 -Game sample_headless -Configuration Release -SkipBuild -StartEmulator -AvdName GameEngine_API36` installs and launches the Release APK.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-mobile-packaging.ps1` reports Android AVD readiness explicitly.

---

### Task 1: Prove Android Package Build And Signing

**Files:**
- Validate existing: `tools/build-mobile-android.ps1`
- Validate existing: `tools/check-android-release-package.ps1`
- Validate existing: `platform/android/app/build.gradle.kts`

- [x] **Step 1: Run Debug APK build**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-mobile-android.ps1 -Game sample_headless -Configuration Debug
```

Expected: inside the sandbox this fails with Gradle native cache ACL restrictions; outside the sandbox it succeeds and produces `platform/android/app/build/outputs/apk/debug/app-debug.apk`.

- [x] **Step 2: Run local Release signing validation**

Run outside the sandbox:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-android-release-package.ps1 -Game sample_headless -UseLocalValidationKey
```

Expected: creates a PKCS12 upload key under `%LOCALAPPDATA%\GameEngine\android\keys`, builds Release, verifies the APK with `apksigner`, exports `out/mobile/android/release/upload_certificate.pem`, and prints `android-release-package-check: ok`.

### Task 2: Harden AVD Discovery For Smoke

**Files:**
- Modify: `tools/common.ps1`
- Modify: `tools/check-mobile-packaging.ps1`
- Modify: `tools/smoke-android-package.ps1`

- [x] **Step 1: Write failing helper check**

Run before implementation:

```powershell
. .\tools\common.ps1
$old = [Environment]::GetEnvironmentVariable('ANDROID_AVD_HOME','Process')
[Environment]::SetEnvironmentVariable('ANDROID_AVD_HOME',$null,'Process')
Set-AndroidAvdHomeEnvironment
if ([Environment]::GetEnvironmentVariable('ANDROID_AVD_HOME','Process') -ne (Join-Path $env:USERPROFILE '.android\avd')) {
    throw 'ANDROID_AVD_HOME was not set to the default AVD directory'
}
[Environment]::SetEnvironmentVariable('ANDROID_AVD_HOME',$old,'Process')
```

Expected before implementation: fails because `Set-AndroidAvdHomeEnvironment` does not exist.

- [x] **Step 2: Add AVD home normalization**

Add `Set-AndroidAvdHomeEnvironment` to `tools/common.ps1`. It resolves `ANDROID_AVD_HOME` from process/user/machine scope first, then falls back to `%USERPROFILE%\.android\avd` when that directory exists, and writes the resolved path to the process environment.

- [x] **Step 3: Use the helper from Android smoke**

Call `Set-AndroidAvdHomeEnvironment | Out-Null` in `tools/smoke-android-package.ps1` after Android/JDK environment propagation and before `adb`/emulator use.

- [x] **Step 4: Add AVD diagnostics**

Add `Get-AndroidAvdNames` to `tools/common.ps1` and use it in `tools/check-mobile-packaging.ps1` to print `mobile-packaging: android-avd=ready (<names>)` or `mobile-packaging: android-avd=not-configured`.

- [x] **Step 5: Verify helper behavior**

Run:

```powershell
. .\tools\common.ps1
$names = Get-AndroidAvdNames
if (-not (@($names) -contains 'GameEngine_API36')) {
    throw "GameEngine_API36 AVD was not detected. Names: $($names -join ', ')"
}
```

Expected: passes after helper implementation.

### Task 3: Prove Emulator Smoke

**Files:**
- Validate existing: `tools/smoke-android-package.ps1`
- Validate existing output: `platform/android/app/build/outputs/apk/release/app-release.apk`

- [x] **Step 1: Create the local smoke AVD**

Run outside the sandbox when no AVD exists:

```powershell
$avd = Join-Path $env:ANDROID_HOME 'cmdline-tools\latest\bin\avdmanager.bat'
'no' | & $avd create avd --force --name GameEngine_API36 --package 'system-images;android-36;google_apis;x86_64' --device medium_phone
```

Expected: creates `%USERPROFILE%\.android\avd\GameEngine_API36.avd`.

- [x] **Step 2: Run Release APK emulator smoke**

Run outside the sandbox:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/smoke-android-package.ps1 -Game sample_headless -Configuration Release -SkipBuild -StartEmulator -AvdName GameEngine_API36
```

Expected: starts `emulator-5584`, installs the Release APK, launches `dev.mirakanai.android/.MirakanaiActivity`, verifies the process, force-stops the app, kills the emulator, and prints `android-smoke: ok`.

### Task 4: Synchronize Docs And Manifest

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/workflows.md`
- Modify: `docs/testing.md`
- Modify: `docs/dependencies.md`
- Modify: `docs/architecture.md`
- Modify: `docs/roadmap.md`
- Modify: `engine/agent/manifest.json`

- [x] **Step 1: Update human-facing docs**

Document that Android diagnostics include AVD readiness, that Android smoke normalizes `ANDROID_AVD_HOME`, and that Gradle/emulator lanes may need to run outside the sandbox because they write user SDK/cache/emulator state.

- [x] **Step 2: Update AI-facing manifest**

Keep `engine/agent/manifest.json` honest by recording Android AVD readiness diagnostics and deterministic `ANDROID_AVD_HOME` normalization.

- [x] **Step 3: Run validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-mobile-packaging.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-android-release-package.ps1 -Game sample_headless -UseLocalValidationKey
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/smoke-android-package.ps1 -Game sample_headless -Configuration Release -SkipBuild -StartEmulator -AvdName GameEngine_API36
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Expected: Android-specific lanes pass. Default validation may still report host-gated Apple/Metal diagnostics; those are not Android blockers.
