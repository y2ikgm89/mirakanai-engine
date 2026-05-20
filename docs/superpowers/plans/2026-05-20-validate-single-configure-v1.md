# Validate Single Configure v1 (2026-05-20)

**Plan ID:** `validate-single-configure-v1`
**Status:** Completed docs/governance slice
**Owner:** Codex
**Completed phase:** Phase 1 - Single configure/build validation path

## Goal

Reduce local `tools/validate.ps1` wall-clock time without reducing validation coverage, diagnostics, or slice-gate expectations.

## Context

- The previous validation path ran CMake configure for `check-tidy.ps1`, then configured/built again through `build.ps1`, then built again before `test.ps1`.
- Context7 `/kitware/cmake` and official CMake File API guidance confirm that clients request codemodel data before generation and read replies after CMake generates the build system.
- CTest can run against an already-built tree, so `validate.ps1` can avoid a second build after `build.ps1` succeeds.

## Constraints

- Keep every existing validation check in `tools/validate.ps1`.
- Keep standalone wrappers conservative: `tools/test.ps1` still builds by default, and `tools/check-tidy.ps1` still configures by default when it needs File API data.
- Do not weaken clang-tidy, format, toolchain, package, public API, mobile, Apple-host evidence, or manifest/static checks.
- Do not hand-edit `engine/agent/manifest.json`; edit fragments and compose.

## Phase 1 - Single Configure/Build Validation Path

### Done When

- `tools/build.ps1` prepares the CMake File API codemodel query before the `dev` configure/build.
- `tools/check-tidy.ps1` can explicitly reuse an existing File API codemodel reply after `build.ps1`, while preserving configure-on-demand as its default behavior.
- `tools/test.ps1 -SkipBuild` runs CTest without rebuilding, while `tools/test.ps1` continues to build by default.
- `tools/validate.ps1` runs `build.ps1` once, reuses the File API reply for the clang-tidy smoke, then runs `test.ps1 -SkipBuild`.
- Docs, manifest fragments, skills, subagents, and static checks describe the optimized path.
- Focused checks pass, followed by `tools/validate.ps1`.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` after RED guard | FAIL | Proved the guard caught missing `test.ps1 -SkipBuild`, build File API query setup, and tidy reply reuse before implementation. |
| Context7 `/kitware/cmake` docs query | PASS | Confirmed File API clients request codemodel data before generation and consume replies after generation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Static guard proves validate/tidy/build/test tool contract and updated docs/manifest needles. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Manifest fragment compose output matches `engine/agent/manifest.json`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Agent surfaces remain in size/parity/script-parse budgets. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | C++ and tracked text formatting valid. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Prepared File API query, configured, and built the `dev` preset. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1 -ReuseExistingFileApiReply` | PASS | Reused existing CMake File API reply for 238 files and analyzed one smoke TU. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1 -SkipBuild` | PASS | Ran CTest directly after the build; 65/65 tests passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full local validation passed; diagnostic-only Metal/Apple host gates unchanged. |
