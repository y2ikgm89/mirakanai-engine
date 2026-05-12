# Windows Official Diagnostics Toolchain Guidance v1 (2026-05-02)

**Status:** Completed docs/governance slice.

## Goal

Record the official Windows diagnostics tools selected in this chat and make the guidance durable across docs, skills, rules, subagents, the engine agent manifest, schemas, and static checks.

## Context

This chat started from a local report that debugger tools were not found on `PATH`. The host had Visual Studio Build Tools, CMake, MSBuild, Android tooling, DXC/SPIR-V tooling, and default validation working, but no native debugger tools such as `cdb`, `windbg`, `gdb`, `lldb`, `vsdbg`, or `devenv`.

The official Microsoft path was selected and installed for Windows diagnostics:

- Debugging Tools for Windows from the Windows SDK, verified with `cdb -version` reporting `10.0.26100.8249`.
- Microsoft public symbol server through `_NT_SYMBOL_PATH=srv*C:\Symbols*https://msdl.microsoft.com/download/symbols`.
- PIX on Windows, verified with `pixtool --help`.
- Windows Performance Toolkit from the Windows ADK, verified with `wpr -help` and `xperf -help`.
- Windows Graphics Tools optional capability `Tools.Graphics.DirectX~~~~0.0.1.0`, verified by installed capability state and `d3d12SDKLayers.dll` in both `System32` and `SysWOW64`.

The ADK servicing review found that the available KB5079391 package contained patches for installed-specific ADK components; no Windows Performance Toolkit patch was present in that package. Future ADK servicing should apply only patches that match installed features.

## Constraints

- Keep these tools classified as host diagnostics, not repository runtime dependencies.
- Do not install diagnostics tools from CMake configure, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, or vcpkg bootstrap.
- Keep Codex and Claude Code behavior synchronized.
- Keep D3D12 debug-layer, PIX, WPT, debugger, symbol, and trace data out of public runtime/game APIs.
- Keep Apple/Metal, Android signing/device smoke, and strict tidy compile database gates separate from Windows diagnostics.

## Done When

- Human docs describe the official Windows diagnostics stack and smoke checks.
- Codex/Claude skills and subagents distinguish host diagnostics from build blockers.
- Rules/settings keep host-modifying Windows diagnostics commands approval-gated.
- `engine/agent/manifest.json` exposes `windowsDiagnosticsToolchain`.
- Static checks fail if the new guidance disappears.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Change Summary

- Added `windowsDiagnosticsToolchain` to the engine agent manifest and context output.
- Added Windows diagnostics guidance to dependency, workflow, testing, AI integration, licensing, and top-level README docs.
- Updated CMake, debugging, rendering, and agent-integration skills for both Codex and Claude Code.
- Updated build-fixer and rendering-auditor subagents with Debugging Tools / Graphics Tools / PIX / WPT guidance.
- Extended Codex rules and Claude settings so `Invoke-WebRequest`, `Add-WindowsCapability`, `dism`, and `msiexec` remain approval-gated.
- Extended `tools/check-ai-integration.ps1` to enforce the guidance across docs, manifest, schema, skills, rules, settings, subagents, and `agent-context`.

## Validation Evidence

- GREEN, 2026-05-02: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` completed and exposed `windowsDiagnosticsToolchain` in the generated JSON context.
- GREEN, 2026-05-02: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed with `ai-integration-check: ok`.
- GREEN, 2026-05-02: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `validate: ok` and CTest `28/28`.
- Diagnostic-only host gates remained unchanged: Metal `metal` / `metallib` missing on Windows, Apple packaging requires macOS/Xcode, Android release signing and Android device smoke are not configured, and strict tidy analysis is gated by compile database availability for the active generator.
