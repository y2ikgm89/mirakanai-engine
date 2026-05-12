# sample-generated-desktop-runtime-package

## Goal

Describe the desktop runtime game goal here before expanding gameplay.

## Current Runtime

This game uses the optional desktop runtime package lane:

- `mirakana::GameApp`
- `mirakana::SdlDesktopGameHost`
- `mirakana::IRenderer` with deterministic NullRenderer fallback unless host-owned shader artifacts are added later
- `game.agent.json.runtimePackageFiles` plus `PACKAGE_FILES_FROM_MANIFEST`

## Validate

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_package
```
