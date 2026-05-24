# Scripting And Mod Sandbox v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Promote `scripting-and-mod-sandbox-v1` from optional-adapter candidate to a first-party runtime script execution facade with deny-by-default adapter dispatch, host API budgeting, replay evidence, and package-visible counters.

**Plan ID:** `scripting-and-mod-sandbox-v1`

**Status:** Completed.

**Gap:** `scripting-and-mod-sandbox-v1`

**Architecture:** Keep script and mod contracts in `MK_runtime` as first-party value types. `plan_runtime_script_sandbox` remains the policy gate; new execution facade types consume a reviewed plan plus caller-owned adapter callbacks and fail closed before dispatch if filesystem, network, process, native plugin, missing host API, budget, or replay preconditions are not satisfied. No VM type, Lua state, WASM handle, OS handle, package mutation, or middleware object appears in public APIs.

**Tech Stack:** C++23, CMake `MK_runtime`, existing `sample_2d_desktop_runtime_package`, repository PowerShell validation wrappers, composed engine agent manifest fragments.

---

## Classification

Large: this changes public runtime API, sample package counters, backlog status, docs, manifest fragments, and static guards. It uses TDD for runtime behavior, focused build/test loops during implementation, and full `tools/validate.ps1` at slice closeout.

## Official Documentation Review

- Context7 was attempted for Jolt/CMake/vcpkg during optional-adapter planning, but the connector returned an expired OAuth token. Use official documentation directly until Context7 is re-authenticated.
- Microsoft vcpkg manifest mode documents project manifests as the project-owned dependency declaration surface and feature selection as explicit `--x-feature` or CMake `VCPKG_MANIFEST_FEATURES` inputs. This slice adds no third-party scripting dependency, so `vcpkg.json` remains unchanged.
- Microsoft vcpkg `vcpkg.json` reference defines features as optional dependency flags and reminds users to verify exact license requirements. Future Lua/WASM runtime adoption must update `vcpkg.json`, `docs/dependencies.md`, `docs/legal-and-licensing.md`, and `THIRD_PARTY_NOTICES.md`.
- CMake buildsystem docs recommend `PRIVATE` link dependencies for implementation-only dependencies and `PUBLIC` only when headers require them. This slice stays dependency-free in `MK_runtime`; future adapters belong in separate optional targets.
- Wasmtime/WIT is the preferred future mod runtime direction because import lists and WASI preopens map cleanly to reviewed host API allow-lists. It is not added in this slice because the current repo dependency policy requires vcpkg/bootstrap/legal records first and no Wasmtime vcpkg port is available in the current local manifest flow.
- Lua remains a future trusted-gameplay-script option only after sandbox closure for `io`, `os`, `package`, `debug`, native module loading, and dynamic code loading is proven.

## Goal / Context / Constraints / Done When

**Goal:** Provide a reusable public execution facade that lets generated or sample games dispatch reviewed script entrypoints through caller-owned adapters while preserving deterministic budgets and deny-by-default capability rows.

**Context:** `engine-scripting-sandbox-v1` already implemented value-only policy planning and selected package counters. The backlog row `scripting-and-mod-sandbox-v1` remains open because runtime execution adapters, script APIs, replay guarantees, and dependency/legal decisions need a selected plan.

**Constraints:**
- Clean breaking greenfield implementation; no compatibility shim or duplicate legacy API.
- Public API stays first-party and backend-neutral.
- Do not add Lua, Wasmtime, WASI, filesystem, network, process, or native plugin dependencies in this slice.
- `MK_runtime` must not depend on editor, SDL3, Dear ImGui, renderer/RHI, or platform native handles.
- `engine/agent/manifest.json` is compose output only; edit fragments and run `tools/compose-agent-manifest.ps1 -Write`.
- Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/*.ps1`; no `bun run`.

**Done when:**
- Runtime tests prove reviewed plan dispatch, deny-by-default no-dispatch, unsupported host API no-dispatch, instruction budget exhaustion, memory budget exhaustion, adapter failure diagnostics, and same-seed deterministic replay signature rows.
- `sample_2d_desktop_runtime_package --require-scripting-sandbox-policy` reports execution facade counters in addition to existing policy counters.
- Docs, backlog, manifest fragments, game-code guidance, and static checks describe the new supported boundary and preserve future VM/dependency gates.
- Focused C++ build/test/static checks and final `tools/validate.ps1` pass.
- Task-owned branch is committed, pushed, and opened as a PR with validation evidence.

## Task 1: Runtime Execution Facade Red Test

**Files:**
- Modify: `tests/unit/runtime_scripting_sandbox_tests.cpp`

- [x] **Step 1: Write failing tests**

Add tests for a new `execute_runtime_script_entrypoint` facade:

```cpp
MK_TEST("runtime scripting sandbox dispatches reviewed entrypoints through host adapter") {
    auto plan = mirakana::runtime::plan_runtime_script_sandbox(make_reviewed_script_policy());
    RecordingScriptAdapter adapter;
    const auto result = mirakana::runtime::execute_runtime_script_entrypoint(
        mirakana::runtime::RuntimeScriptExecutionRequest{
            .plan = &plan,
            .module_id = "gameplay",
            .entrypoint_id = "tick",
            .input_event_id = "frame.0001",
            .instruction_budget = 80U,
            .memory_budget_bytes = 2048U,
            .replay_seed = 7U,
        },
        adapter);

    MK_REQUIRE(result.status == mirakana::runtime::RuntimeScriptExecutionStatus::completed);
    MK_REQUIRE(result.dispatched);
    MK_REQUIRE(result.stats.instructions_consumed == 80U);
    MK_REQUIRE(result.stats.memory_bytes_touched == 2048U);
    MK_REQUIRE(result.replay_signature != 0U);
    MK_REQUIRE(adapter.call_count == 1U);
}
```

Also add tests that invalid plan diagnostics, default-denied permissions, unsupported host APIs, budget exhaustion, and adapter failure all produce `RuntimeScriptExecutionDiagnostic` rows and leave `adapter.call_count == 0U` except the adapter failure case.

- [x] **Step 2: Verify red**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_scripting_sandbox_tests
```

Expected: build fails because `RuntimeScriptExecutionRequest`, `RuntimeScriptExecutionStatus`, and `execute_runtime_script_entrypoint` do not exist.

## Task 2: Runtime Execution Facade Green

**Files:**
- Modify: `engine/runtime/include/mirakana/runtime/scripting_sandbox.hpp`
- Modify: `engine/runtime/src/scripting_sandbox.cpp`

- [x] **Step 1: Add public value types**

Add:
- `RuntimeScriptExecutionStatus`
- `RuntimeScriptExecutionDiagnosticCode`
- `RuntimeScriptExecutionStats`
- `RuntimeScriptHostApiCall`
- `RuntimeScriptAdapterResult`
- `RuntimeScriptExecutionRequest`
- `RuntimeScriptExecutionResult`
- abstract `IRuntimeScriptExecutionAdapter`

Keep the adapter interface first-party:

```cpp
class IRuntimeScriptExecutionAdapter {
  public:
    virtual ~IRuntimeScriptExecutionAdapter() = default;

    [[nodiscard]] virtual RuntimeScriptAdapterResult execute(
        const RuntimeScriptExecutionRequest& request,
        const RuntimeScriptSandboxEntrypointPlanRow& entrypoint,
        std::span<const RuntimeScriptSandboxPermissionPlanRow> permissions) = 0;
};
```

- [x] **Step 2: Implement fail-closed facade**

`execute_runtime_script_entrypoint` should:
- reject null plan, failed policy plan, missing module/entrypoint, replay seed mismatch, zero or over-plan budgets, and entrypoints with denied permissions before adapter dispatch;
- pass only the matching entrypoint row and allowed permission rows to the adapter;
- convert adapter success/failure into first-party result rows;
- compute deterministic replay signatures from module id, entrypoint id, input event id, replay seed, consumed instructions, memory bytes, emitted host calls, and adapter output rows;
- never open files, access network/processes, mutate packages, or call native handles.

- [x] **Step 3: Verify green**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_scripting_sandbox_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_scripting_sandbox_tests
```

Expected: target builds and all scripting sandbox tests pass.

## Task 3: Package Counter Evidence

**Files:**
- Modify: `games/sample_2d_desktop_runtime_package/main.cpp`
- Modify: `games/sample_2d_desktop_runtime_package/README.md`
- Modify if needed: `tools/validate-installed-desktop-runtime.ps1`

- [x] **Step 1: Add package-visible execution counters**

Under the existing `--require-scripting-sandbox-policy` path, dispatch one reviewed entrypoint through a sample-owned deterministic adapter and emit:
- `scripting_sandbox_execution_status=completed`
- `scripting_sandbox_execution_ready=1`
- `scripting_sandbox_execution_dispatches=1`
- `scripting_sandbox_execution_host_api_calls`
- `scripting_sandbox_execution_replay_signature`
- `scripting_sandbox_execution_diagnostics=0`

- [x] **Step 2: Verify focused package smoke**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target sample_2d_desktop_runtime_package
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R sample_2d_desktop_runtime_package
```

Expected: sample tests pass and installed-runtime validation expectations include the new counters.

## Task 4: Docs, Manifest, Static Guards

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `engine/agent/manifest.fragments/014-gameCodeGuidance.json`
- Compose: `engine/agent/manifest.json`
- Modify static guards under `tools/` only where current needles need the new supported surface.

- [x] **Step 1: Add/update static guard needles**

Guard the new public API names, sample counters, backlog status promotion, and manifest guidance. Run the relevant guard before docs are complete if a red static check is useful.

- [x] **Step 2: Update durable surfaces**

Promote `scripting-and-mod-sandbox-v1` to `implemented-1x-foundation` with explicit limits: no bundled VM, no Lua/WASM runtime, no filesystem/network/process/native-plugin capability, no package scripts, no editor command execution, no native handles, and no broad modding readiness.

- [x] **Step 3: Compose and verify**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
```

Expected: static checks pass.

## Task 5: Focused And Full Validation

**Files:**
- All task-owned changes.

- [x] **Step 1: Run focused C++ and text checks**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/scripting_sandbox.cpp,tests/unit/runtime_scripting_sandbox_tests.cpp,games/sample_2d_desktop_runtime_package/main.cpp
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
```

Expected: focused checks pass.

- [x] **Step 2: Run full validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Expected: full validation passes or exact host-gated blockers are recorded.

## Task 6: PR And Next Candidate Handoff

**Files:**
- No new edits unless validation or hosted CI exposes a root-cause failure.

- [ ] **Step 1: Commit and push**

Inspect status, stage only task-owned changes, run `git diff --cached --check`, commit, inspect remote state, and push the topic branch.

- [ ] **Step 2: Create PR**

PR body includes summary, official docs note, clean-breaking boundary, no-new-dependency decision, validation evidence, and next candidate recommendation.

- [ ] **Step 3: Continue after PR evidence**

If the PR is independent, sync `main` after merge and create the next candidate worktree/branch for `native-physics-middleware-adapter-v1`. If hosted checks expose issues, fix only task-owned root causes in this branch.

## Evidence Log

| Phase | Evidence |
| --- | --- |
| Baseline worktree | `tools/prepare-worktree.ps1` passed; `tools/check-toolchain.ps1` passed; `tools/cmake.ps1 --preset dev` passed; `tools/cmake.ps1 --build --preset dev` passed; `tools/ctest.ps1 --preset dev --output-on-failure` passed 76/76 tests. |
| Subagent review | `planning-auditor`, `engine-architect`, and `explorer` completed read-only reviews and were closed after results were consumed. |
| Official docs | Context7 failed with expired OAuth token; official Microsoft vcpkg manifest/reference docs, CMake buildsystem docs, and Jolt docs were reviewed for optional-adapter direction. |
| Red test | `tools/cmake.ps1 --build --preset dev --target MK_runtime_scripting_sandbox_tests` failed before implementation because `IRuntimeScriptExecutionAdapter`, `RuntimeScriptExecutionRequest`, `RuntimeScriptExecutionStatus`, and `execute_runtime_script_entrypoint` did not exist. |
| Focused validation | `tools/cmake.ps1 --build --preset dev --target MK_runtime_scripting_sandbox_tests` passed; `tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_scripting_sandbox_tests` passed 1/1; `tools/cmake.ps1 --build --preset dev --target sample_2d_desktop_runtime_package` passed; source-tree smoke with `--require-scripting-sandbox-policy` passed with `scripting_sandbox_execution_status=completed`, `scripting_sandbox_execution_ready=1`, `scripting_sandbox_execution_dispatches=1`, `scripting_sandbox_execution_host_api_calls=1`, `scripting_sandbox_execution_replay_signature=17484938214348006833`, and `scripting_sandbox_execution_diagnostics=0`; `tools/check-format.ps1` passed after `tools/format.ps1`; `tools/check-public-api-boundaries.ps1` passed; `tools/check-json-contracts.ps1` passed; `tools/check-ai-integration.ps1` passed; `tools/check-agents.ps1` passed; `tools/check-tidy.ps1 -Files engine/runtime/src/scripting_sandbox.cpp,tests/unit/runtime_scripting_sandbox_tests.cpp,games/sample_2d_desktop_runtime_package/main.cpp` passed. |
| Full validation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed; 19 static checks passed, diagnostic-only host gates were reported for Metal/Apple paths on Windows, build passed, tidy smoke passed, and CTest passed 76/76. |
| Hosted checks | Pending. |
