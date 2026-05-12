# Android Packaging Template

This template is the Wave 9 Android entry path. It uses Android Gradle Plugin 9.1.0, Gradle 9.3.1, JDK 17, Android SDK Platform 36.1, Build Tools 36.0.0, Android SDK CMake 4.1.2, NDK CMake integration, Prefab, AndroidX AppCompat 1.7.1, AndroidX Core 1.18.0, GameActivity, the NDK Vulkan loader, and the NDK AAudio platform library. Native Android SDK, Vulkan, and AAudio handles stay inside this template and future `engine/platform/android` adapter code; game-facing code continues to use first-party `mirakana::` contracts.

Run diagnostics from the repository root:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-mobile-packaging.ps1
```

Build once the Android SDK, NDK, JDK, and Gradle are installed:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-mobile-android.ps1 -Game sample_headless -Configuration Debug
```

Use `-Configuration Release` only with signing environment variables configured:

```powershell
$env:MK_ANDROID_KEYSTORE="C:\path\release.keystore"
$env:MK_ANDROID_KEYSTORE_PASSWORD="<password>"
$env:MK_ANDROID_KEY_ALIAS="<alias>"
$env:MK_ANDROID_KEY_PASSWORD="<password>"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-mobile-android.ps1 -Game sample_headless -Configuration Release
```

For local Release signing validation without storing secrets in the repository, generate a temporary upload key outside the repository, build Release, export the public upload certificate, and verify the APK signature:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-android-release-package.ps1 -UseLocalValidationKey
```

Run emulator/device smoke after a package exists:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/smoke-android-package.ps1 -Configuration Release -SkipBuild -StartEmulator -AvdName Mirakanai_API36
```

The template accepts `-Pmirakanai.game=<games directory name>`, validates that `games/<name>/game.agent.json` exists before attempting a package build, packages that manifest under app assets, and copies `games/<name>/assets` when present. The native bridge exposes the required Android Vulkan surface extension name, creates `VK_KHR_android_surface` surfaces from the private `ANativeWindow`, and starts/stops a private low-latency AAudio float32 output stream on lifecycle events.


