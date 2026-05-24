# Native Physics Middleware Adapter v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Promote `native-physics-middleware-adapter-v1` from optional-adapter candidate to an optional, dependency-gated Jolt Physics adapter lane without expanding the default Physics 1.0 ready surface.

**Plan ID:** `native-physics-middleware-adapter-v1`

**Status:** Active.

**Gap:** `native-physics-middleware-adapter-v1`

**Architecture:** Keep `MK_physics` as the first-party public contract. Add `MK_physics_jolt` as an optional adapter target behind `MK_ENABLE_PHYSICS_JOLT` and the vcpkg `physics-jolt` feature. Public headers expose only MIRAIKANAI value types and fail-closed capability negotiation; Jolt headers, `JPH::*` types, native handles, and deterministic-parity claims stay out of gameplay APIs.

**Tech Stack:** C++23, Jolt Physics through vcpkg manifest feature `physics-jolt`, CMake package export with conditional `Mirakanai_HAS_PHYSICS_JOLT`, repository PowerShell validation wrappers, composed engine agent manifest fragments.

---

## Classification

Large: this adds an optional third-party C++ dependency, an installable optional target, public first-party adapter facade, tests, dependency/legal records, docs, manifest fragments, and static guards. It uses TDD for public facade behavior and focused `physics-jolt` build/test loops before full validation.

## Official Documentation Review

- Context7 `/jrouwe/joltphysics` confirmed the official Jolt initialization sequence: `RegisterDefaultAllocator`, allocate `Factory::sInstance`, `RegisterTypes`, then create `PhysicsSystem`, `TempAllocatorImpl`, and `JobSystemThreadPool`; layer/filter interfaces must outlive the `PhysicsSystem`.
- Context7 `/jrouwe/joltphysics` confirmed collision filtering is based on `ObjectLayer`/broadphase filters and that cross-platform determinism is build/config dependent, so the stock vcpkg Jolt adapter does not advertise cross-platform determinism.
- Context7 `/microsoft/vcpkg` and local vcpkg metadata confirm optional dependency selection belongs in manifest features and explicit bootstrap/install lanes, not CMake configure-time package restore.
- Context7 `/kitware/cmake` confirmed `CMakeFindDependencyMacro` / `find_dependency` is the supported package-config path for exported targets with transitive optional package dependencies.

## Goal / Context / Constraints / Done When

**Goal:** Provide a reviewable optional Jolt adapter for authored 3D collision scenes while preserving the default dependency-free `MK_physics` surface and explicit 1.0 exclusion of middleware-backed readiness claims.

**Context:** Physics 1.0 is closed as a first-party deterministic surface. The optional candidate is valuable for native middleware proofing but must remain dependency-gated, fail-closed, and separate from generated-game default recipes.

**Constraints:**
- Clean breaking greenfield implementation; no compatibility shim or legacy alias.
- Public API stays first-party and backend-neutral.
- `MK_physics` must not depend on Jolt or vcpkg.
- `MK_physics_jolt` is optional and built only when `MK_ENABLE_PHYSICS_JOLT=ON` with bootstrapped `physics-jolt`.
- Jolt headers and `JPH::*` symbols must not appear in public headers.
- Stock vcpkg Jolt does not prove cross-platform determinism; capability negotiation must report that honestly.
- `engine/agent/manifest.json` is compose output only; edit fragments and run `tools/compose-agent-manifest.ps1 -Write`.

**Done when:**
- Native facade tests prove missing adapter, native-handle exposure rejection, unavailable determinism rejection, unsupported filters/triggers no-dispatch, and successful first-party scene dispatch.
- Jolt adapter tests prove first-party capability rows, authored scene stepping, unsupported Jolt filter-bit diagnostics, single-backend-body fail-closed behavior, disabled-collision fail-closed behavior, trigger no-dispatch behavior, and dense-scene capacity sizing.
- CMake/vcpkg/install/export surfaces expose `physics-jolt`, `MK_ENABLE_PHYSICS_JOLT`, `MK_physics_jolt`, and `Mirakanai_HAS_PHYSICS_JOLT` without configure-time package install.
- Dependency/legal notices record Jolt Physics and keep default build dependency-free.
- Docs, roadmap, plan registry, manifest fragments, game-code guidance, and static checks separate default/generated-game readiness from the optional gated adapter lane.
- Focused C++ build/test/static checks, `physics-jolt` lane checks, and final `tools/validate.ps1` pass.
- Task-owned branch is committed, pushed, and opened as a PR with validation evidence.

## Task 1: First-Party Native Adapter Facade

- [x] Add failing tests for missing adapter, unsupported native handles, unavailable determinism, unsupported filter/trigger capabilities, and successful first-party scene rows.
- [x] Add `mirakana/physics/native_adapter.hpp` and `simulate_native_physics_3d` with fail-closed capability negotiation and request validation.
- [x] Keep Jolt and native handles out of the facade public header.
- [x] Build and run `MK_physics_native_adapter_tests` under the `dev` preset.

## Task 2: Optional Jolt Adapter Lane

- [x] Add `physics-jolt` vcpkg feature, `MK_ENABLE_PHYSICS_JOLT`, `physics-jolt` CMake preset, and optional `MK_physics_jolt` target.
- [x] Add focused Jolt tests for capabilities, authored scene stepping, filter-bit diagnostics, single-backend-body fail-closed behavior, disabled collision fail-closed behavior, trigger no-dispatch behavior, and dense-scene capacity sizing.
- [x] Implement Jolt initialization, official mask-based object-layer filtering, authored scene conversion, Jolt stepping, and first-party snapshot output.
- [x] Build and run `MK_physics_jolt_tests` under the `physics-jolt` preset.

## Task 3: Dependency / Install / Boundary Surface

- [x] Add conditional install/export package config for `Mirakanai_HAS_PHYSICS_JOLT` and `find_dependency(Jolt CONFIG)`.
- [x] Update dependency policy and public API boundary checks for `physics-jolt` and Jolt public-header bans.
- [x] Update `docs/dependencies.md`, `docs/legal-and-licensing.md`, and `THIRD_PARTY_NOTICES.md`.

## Task 4: Agent Surface Drift Sync

- [x] Update current capabilities, roadmap, architecture, AI game-development guidance, plan registry, and manifest fragments.
- [x] Compose `engine/agent/manifest.json`.
- [x] Update static check needles that previously expected Jolt to be future-only.

## Task 5: Validation And PR

- [x] Run focused C++ tests and static checks.
- [x] Run `tools/check-dependency-policy.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-json-contracts.ps1`, and `tools/check-ai-integration.ps1`.
- [x] Run `tools/validate.ps1`.
- [x] Commit, push, and open PR with validation evidence.

## Validation Evidence

- Branch was fast-forwarded onto `origin/main` after PR #208 merged; the `tools/check-json-contracts-010-engine-manifest.ps1` index conflict was resolved by keeping both `removeMergedWorktree [-DeleteRemoteBranch]` and `validatePhysicsJolt`.
- Candidate committed as `d3dc46c7eb5475ac706988486c963bda275e7ad0`, pushed to `codex/native-physics-middleware-adapter-v1`, and opened as PR #209: https://github.com/y2ikgm89/mirakanai-engine/pull/209.
- PR #209 hosted Windows checks initially failed before validation because `actions/cache/restore` restored `external/vcpkg` through `restore-keys`, which leaves `cache-hit` non-`true`; the workflow then cloned into the existing directory. Fixed `.github/workflows/validate.yml` to guard clone by `Test-Path external/vcpkg/.git` and extended `tools/check-ci-matrix.ps1` to lock that contract.
- Read-only agent-surface subagent audit after the main sync reported no findings for manifest fragments, compose output, static checks, dependency docs, and plan registry alignment.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1` - passed; linked worktree, `external/vcpkg`, and `vcpkg_installed` were ready.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` - passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_physics_native_adapter_tests` - passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_physics_native_adapter_tests` - passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset physics-jolt` - passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset physics-jolt --target MK_physics_jolt_tests` - passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset physics-jolt --output-on-failure -R MK_physics_jolt_tests` - passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-physics-jolt.ps1` - passed; configured `physics-jolt`, built the SDK/runtime/Jolt test targets, ran native and Jolt physics tests, installed to `out/install/physics-jolt`, and validated the installed SDK consumer with `Mirakanai_HAS_PHYSICS_JOLT`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/physics/src/native_adapter.cpp,tests/unit/physics_native_adapter_tests.cpp` - passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Preset physics-jolt -Files engine/physics/jolt/src/jolt_physics_adapter.cpp,tests/unit/physics_jolt_tests.cpp` - passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1` - passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` - passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` - passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` - passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` - passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` - passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` - passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-vcpkg-environment.ps1` - passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1` - passed after the hosted Windows vcpkg restore-key clone guard fix.
- `git diff --check` - passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1` - blocked by this Codex session's command policy because approval was required while `AskForApproval=Never`; no configure-time dependency-install workaround was added.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` - passed; static checks, dev build, and 77/77 dev tests passed. Metal/iOS checks remained host-gated or diagnostic-only on this Windows host.
