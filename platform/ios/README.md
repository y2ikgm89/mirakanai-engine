# Apple Packaging Template

This template is the Wave 9 Apple entry path. It is configured by `tools/build-mobile-apple.ps1` on macOS with full Xcode selected as the active developer directory. The package script configures this tree with `BUILD_TESTING=OFF` and builds only the `MirakanaiIOS` app target, so Simulator smoke stays focused on the bundle instead of Xcode `ALL_BUILD` test targets. UIKit, Metal, sandbox storage roots, signing, and bundle resources stay inside this packaging template and future Apple platform adapters; game-facing code continues to use first-party `mirakana::` contracts.

Run diagnostics from the repository root:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-mobile-packaging.ps1
```

Build once Xcode tools are available:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-mobile-apple.ps1 -Game sample_headless -Configuration Debug -Platform Simulator
```

Run the Simulator install/launch smoke on a macOS host with an installed iOS Simulator runtime:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/smoke-ios-package.ps1 -Game sample_headless -Configuration Debug
```

Release builds require a configured Apple development team:

```powershell
$env:MK_IOS_BUNDLE_IDENTIFIER="dev.mirakanai.ios"
$env:MK_IOS_DEVELOPMENT_TEAM="<team-id>"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-mobile-apple.ps1 -Game sample_headless -Configuration Release -Platform Device
```

The template packages `games/<name>/game.agent.json` into bundle resources, creates a `CAMetalLayer`-backed UIKit root view, maps Application Support to save data, Caches to cache data, and Documents to shared data through `mirakana::MobileStorageRoots`, and accepts bundle identifier, development team, optional code-sign identity, and simulator/device platform inputs without exposing UIKit, Metal, storage, or signing details to game code. `.github/workflows/ios-validate.yml` mirrors the Simulator smoke path on a hosted macOS runner.
