# 2026-05-20 C++23 Release Evaluation Throughput v1

**Status:** Completed with hosted Release-lane confirmation required on the PR.

**Goal:** Reduce hosted Windows C++23 Release Evaluation wall-clock time while preserving the release artifact quality gate.

**Context:** The latest observed `Validate` run `26164860519` completed the `Windows C++23 Release Evaluation` job in about 19 minutes. Log timing showed `tools/evaluate-cpp23.ps1 -Release` first running the Debug `cpp23-eval` configure/build/CTest path for about 7 minutes, then running the Release/package path for about 10 minutes. The Windows MSVC validation lane already covers Debug Windows build/test confidence separately.

**Official guidance consulted:** Context7 `/kitware/cmake` confirms preset-backed `cmake --build --preset` as the native build abstraction; Context7 `/actions/cache` confirms split restore/save actions, primary-key reuse, restore keys, and explicit cache path selection; Microsoft Learn documents MSBuild `-maxcpucount` parallel project builds and MSVC `/Zf` faster PDB generation with `/MP` or parallel `cl.exe`.

**Constraints:**

- Do not reduce the C++23 Release/package validation surface: keep Release configure, build, CTest, install, installed SDK consumer validation, CPack, package hash/archive validation, and artifact upload.
- Do not add global `/FS`, loosen warnings, disable tests, or weaken PR Gate behavior.
- Keep Debug C++23 eval available locally through an explicit `-Debug` lane and as the no-switch default for existing local muscle memory.
- Keep `unsupportedProductionGaps = []`.

**Implementation:**

- Make `tools/evaluate-cpp23.ps1 -Release` release-only.
- Add explicit `-Debug`; no switches still run Debug C++23 evaluation.
- Keep full local confidence available through `-Debug -Release -Gui`.
- Keep CMake module scanning enabled for engine/library targets, but set `CXX_SCAN_FOR_MODULES OFF` for non-module tests, probes, samples, and games.
- Scope the hosted Windows C++23 build cache and failure logs to Release build/install trees, matching the Release-only job.
- Update CI matrix static guards, docs, skills, and manifest command guidance.

**Done When:**

- `tools/check-ci-matrix.ps1` proves the workflow no longer caches/logs `out/build/cpp23-eval` in the Release job and that `evaluate-cpp23.ps1` exposes the explicit Debug lane.
- Agent-facing CMake guidance and manifest command text describe `[-Debug|-Release|-Gui]`.
- Focused static checks and the coherent build/toolchain validation gate pass, or a concrete host blocker is recorded.

**Validation Evidence:**

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1` | PASS | Static CI contract guard. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-cpp-standard-policy.ps1` | PASS | C++23 policy guard, including non-module executable scan opt-out guard. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Manifest compose parity. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Agent-surface budget and script hygiene. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Agent integration drift guard. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Formatting guard. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/evaluate-cpp23.ps1 -Release` | BLOCKED on this local host | Confirmed release-only execution: Debug `cpp23-eval` no longer ran. Local VS 2022 MSVC `19.44.35222.0` then hit `CL.exe` exit `-1073740791` compiling existing Release C++23 runtime package reviewed-eviction test TUs, reproducible with `--parallel 1` and not fixed by disabling module scanning. Latest hosted `Windows C++23 Release Evaluation` on supported runner had passed before this change; this PR must use the hosted lane for final Release confirmation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Final full local gate passed after desktop-runtime game scan opt-out; `production-readiness-audit: unsupported_gaps=0`. |
