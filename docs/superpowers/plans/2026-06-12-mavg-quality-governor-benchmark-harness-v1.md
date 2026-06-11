# MAVG Quality Governor Benchmark Harness v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a deterministic, value-only MAVG quality governor and dry-run benchmark harness so future MAVG benchmark evidence can be budgeted without claiming Nanite equivalence, broad optimization, backend parity, or unmerged streaming/deformation/ray-tracing readiness.

**Architecture:** Keep the runtime contract in `MK_renderer` as plain C++23 rows that evaluate caller-supplied counters, host evidence, backend-local evidence, no-hole fallback evidence, RT consistency evidence, and unsupported claim flags. Keep the PowerShell harness dry-run and metadata-only: it describes selected scene/recipe/command fields and does not execute GPU work, profiler captures, package mutation, or native handle access.

**Tech Stack:** C++23, `MK_renderer`, CMake/CTest, PowerShell 7 tools, `tools/check-ai-integration.ps1`, MAVG benchmark methodology and performance foundation specs.

---

**Plan ID:** `mavg-quality-governor-benchmark-harness-v1`

**Date:** 2026-06-12

**Context:** This child plan implements a Phase 9 foundation while PRs #578-#584 are open. It must start from `origin/main` and must not depend on APIs from those draft PRs. It can touch shared docs/manifest at closeout, but the code path should stay in new renderer/tooling files to reduce conflict risk.

**Constraints:**

- Do not claim Nanite compatibility, Nanite equivalence, Nanite superiority, benchmark superiority, broad CPU/GPU/memory optimization, backend parity, Metal readiness, mesh shader execution, deformation readiness, ray-tracing readiness, DirectStorage execution, persistent/autonomous streaming services, or async-overlap/performance proof.
- Do not execute GPU commands, profiler captures, package mutation, validation recipes, filesystem writes for benchmark artifacts, or native handle access from the C++ governor.
- Keep benchmark harness output deterministic and dry-run unless a later plan adds measured host evidence.

## Files

- Create: `engine/renderer/include/mirakana/renderer/mavg_quality_governor.hpp`
- Create: `engine/renderer/src/mavg_quality_governor.cpp`
- Create: `tests/unit/mavg_quality_governor_tests.cpp`
- Create: `tools/benchmark-mavg.ps1`
- Create: `docs/mavg.md`
- Modify: `engine/renderer/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Modify at closeout: `docs/current-capabilities.md`
- Modify at closeout: `docs/roadmap.md`
- Modify at closeout: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify at closeout: `docs/specs/2026-06-05-mavg-benchmark-methodology-v1.md`
- Modify at closeout: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Modify at closeout: `docs/superpowers/plans/README.md`
- Modify at closeout: `engine/agent/manifest.fragments/004-modules.json`
- Modify at closeout: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate at closeout: `engine/agent/manifest.json`
- Create at closeout: `tools/check-ai-integration-121-mavg-quality-governor-benchmark-harness.ps1`

### Task 1: RED Test And Target

- [x] **Step 1: Add a failing test target**

Add `MK_mavg_quality_governor_tests` to the root `CMakeLists.txt` and create `tests/unit/mavg_quality_governor_tests.cpp` that includes:

```cpp
#include "test_framework.hpp"
#include "mirakana/renderer/mavg_quality_governor.hpp"
```

The first test should call `mirakana::evaluate_mavg_quality_governor` with one ready static LOD row and assert `MavgQualityGovernorStatus::ready`.

- [x] **Step 2: Verify RED**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools\cmake.ps1 --build --preset dev --target MK_mavg_quality_governor_tests
```

Expected: fail because `mirakana/renderer/mavg_quality_governor.hpp` does not exist.

### Task 2: Public Governor Contract

- [x] **Step 1: Add the public header**

Define:

- `MavgQualityGovernorStatus`
- `MavgQualityGovernorFeatureKind`
- `MavgQualityGovernorDiagnosticCode`
- `MavgQualityGovernorLimits`
- `MavgQualityGovernorCounterRow`
- `MavgQualityGovernorRequest`
- `MavgQualityGovernorDiagnostic`
- `MavgQualityGovernorResult`
- `evaluate_mavg_quality_governor`
- `has_mavg_quality_governor_diagnostic`

Rows must include scene id, package target id, validation recipe id, benchmark command id, backend, feature kind, host evidence flags, no-hole/fallback/RT evidence flags, CPU/GPU/memory/draw/dispatch/error counters, unsupported-claim flags, and `source_index`.

- [x] **Step 2: Add minimal implementation**

Implement deterministic validation and sorting. Missing required ids, duplicate scene/backend/feature rows, invalid limits, row-budget overflow, missing host/backend/no-hole/fallback/RT evidence, exceeded budgets, native handle access, Nanite claims, backend parity claims, broad optimization claims, and benchmark superiority claims must produce diagnostics. `MavgQualityGovernorResult::invoked_gpu_commands`, `invoked_profiler_capture`, `mutated_packages`, and `accessed_native_handles` must remain false.

- [x] **Step 3: Verify GREEN**

Run the focused build and CTest target.

### Task 3: Budget And Non-Claim Coverage

- [x] **Step 1: Add ready-row tests**

Cover a static LOD row with required ids, D3D12 backend-local evidence, host evidence, no-hole evidence, fallback evidence, bounded CPU/GPU frame p95, screen-error p99, fallback/page-miss/temporal-churn rates, resident/upload bytes, draw count, dispatch count, and zero missing geometry. Assert deterministic replay hash and sorted output rows.

- [x] **Step 2: Add fail-closed tests**

Cover invalid ids, duplicate rows, missing host evidence, missing backend-local evidence, missing no-hole/fallback evidence, required RT consistency evidence without evidence, CPU/GPU/memory/draw/error budget violations, row-budget overflow, native handle access, Nanite claim, broad optimization claim, backend parity claim, and benchmark superiority claim.

- [x] **Step 3: Verify focused tests**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools\cmake.ps1 --build --preset dev --target MK_mavg_quality_governor_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools\ctest.ps1 --preset dev -R MK_mavg_quality_governor_tests --output-on-failure
```

### Task 4: Benchmark Harness Dry Run

- [x] **Step 1: Add `tools/benchmark-mavg.ps1`**

Create a PowerShell 7 script with `#requires -Version 7.0`, `#requires -PSEdition Core`, dot-source `tools/common.ps1`, validate `-PlanId`, `-SceneId`, `-PackageTarget`, `-ValidationRecipe`, and `-BenchmarkCommand`, and print deterministic `mavg-benchmark-harness:` rows. Default mode is dry-run metadata output only. It must report `executes_benchmark=false`, `writes_artifacts=false`, `invokes_profiler=false`, and `native_handles=false`.

- [x] **Step 2: Add docs**

Create `docs/mavg.md` as the concise operator landing page that points to the master plan, architecture spec, benchmark methodology, quality governor header, and dry-run benchmark harness. Include explicit non-claims.

- [x] **Step 3: Verify tool parse and dry-run output**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools\benchmark-mavg.ps1 -PlanId mavg-quality-governor-benchmark-harness-v1 -SceneId scene-a-static-dense -PackageTarget sample_desktop_runtime_game -ValidationRecipe default -BenchmarkCommand dry-run
```

Expected: deterministic rows with no execution or artifact mutation.

### Task 5: Docs, Manifest, Static Guard

- [x] **Step 1: Update documentation**

Record `mavg-quality-governor-benchmark-harness-v1`, `mavg_quality_governor.hpp`, `MavgQualityGovernorResult`, `evaluate_mavg_quality_governor`, and `tools/benchmark-mavg.ps1` in current capabilities, roadmap, MAVG architecture spec, benchmark methodology spec, master plan, plan registry, and `docs/mavg.md`.

- [x] **Step 2: Update manifest fragments and compose**

Edit `engine/agent/manifest.fragments/004-modules.json` and `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, then run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools\compose-agent-manifest.ps1 -Write
```

- [x] **Step 3: Add static guard**

Create `tools/check-ai-integration-121-mavg-quality-governor-benchmark-harness.ps1` and rely on numeric-prefix discovery from `tools/check-ai-integration.ps1`.

- [x] **Step 4: Validate closeout**

Run focused build/test, harness dry-run, `check-tidy -Files`, `check-ai-integration`, `check-json-contracts`, `check-public-api-boundaries`, `check-format`, `check-agents`, `git diff --check`, full `tools\validate.ps1`, publication preflight, commit, push, and draft PR.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\cmake.ps1 --build --preset dev --target MK_mavg_quality_governor_tests` failed before the header existed with missing include `mirakana/renderer/mavg_quality_governor.hpp`.
- Review regression RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\ctest.ps1 --preset dev -R MK_mavg_quality_governor_tests --output-on-failure` failed before review fixes for Metal ready rows, explicit host-gated ready/status mismatch, native-token separators, fail-closed counters, and replay-hash budget policy.
- Harness injection RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\benchmark-mavg.ps1 ... -SceneId "scene`ninjected=true" ...` previously printed an injected line, then fails after validation with `SceneId must contain only ASCII letters, digits, dot, underscore, or hyphen`.
- Focused dry run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\benchmark-mavg.ps1 -PlanId mavg-quality-governor-benchmark-harness-v1 -SceneId scene-a-static-dense -PackageTarget sample_desktop_runtime_game -ValidationRecipe default -BenchmarkCommand dry-run` prints deterministic `mavg-benchmark-harness:` rows with `executes_benchmark=false`, `writes_artifacts=false`, `invokes_profiler=false`, and `native_handles=false`.
- Focused build: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\cmake.ps1 --build --preset dev --target MK_mavg_quality_governor_tests` passed.
- Focused test: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\ctest.ps1 --preset dev -R MK_mavg_quality_governor_tests --output-on-failure` passed (`100% tests passed, 0 tests failed out of 1`).
- Focused tidy: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-tidy.ps1 -Files engine/renderer/src/mavg_quality_governor.cpp,tests/unit/mavg_quality_governor_tests.cpp` passed (`tidy-check: ok (2 files)`).
- Agent drift/static checks: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-ai-integration.ps1` passed (`ai-integration-check: ok`); `tools\check-json-contracts.ps1` passed; `tools\check-public-api-boundaries.ps1` passed; `tools\check-agents.ps1` passed.
- Format and whitespace: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-format.ps1` passed after `tools\format.ps1`; `git diff --check` passed.
- Full validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\validate.ps1` passed (`validate: ok`, `119/119` tests passed). Expected host-gated diagnostics remain for Apple/Metal tooling on this Windows host; they are diagnostic-only and do not affect this MAVG quality-governor slice.
