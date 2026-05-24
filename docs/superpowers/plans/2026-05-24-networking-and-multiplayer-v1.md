# Networking And Multiplayer v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Promote `networking-and-multiplayer-v1` from optional-adapter candidate to a narrow first-party runtime network transport facade plus optional ENet loopback adapter proof, without claiming broad multiplayer readiness.

**Plan ID:** `networking-and-multiplayer-v1`

**Status:** Completed.

**Gap:** `networking-and-multiplayer-v1`

**Architecture:** Keep generated-game networking policy value-only by default. Add a first-party `MK_runtime` loopback transport adapter facade and an optional `MK_runtime_network_enet` target behind `MK_ENABLE_NETWORK_ENET` and the vcpkg `network-enet` feature. Public headers expose only MIRAIKANAI value types and adapter factories; ENet headers, ENet native handles, sockets, encryption/authentication, matchmaking, NAT traversal, replication, rollback, and broad multiplayer claims stay out of public gameplay APIs.

**Tech Stack:** C++23, ENet through vcpkg manifest feature `network-enet`, CMake package export with conditional `Mirakanai_HAS_NETWORK_ENET`, repository PowerShell validation wrappers, composed engine agent manifest fragments.

---

## Classification

Large: this adds an optional third-party C++ dependency, install/export wiring, a first-party public runtime adapter facade, focused tests, dependency/legal records, CI validation, docs, manifest fragments, and static guards. It uses TDD for facade validation and delegates optional dependency execution to `tools/validate-network-enet.ps1` plus hosted Windows CI where the dependency bootstrap is available.

## Official Documentation Review

- Context7 `/lsalzman/enet` confirmed the official loopback adapter API shape: initialize/deinitialize ENet, create server/client hosts with `enet_host_create`, connect with `enet_host_connect`, service events through `enet_host_service`, send packets with `enet_packet_create` plus `enet_peer_send`, flush through `enet_host_flush`, use `ENET_PACKET_FLAG_RELIABLE` for reliable ordered packets and no flag for unreliable packets, and destroy received packets after copying their payload.
- Context7 `/microsoft/vcpkg` and Microsoft vcpkg guidance confirm optional dependency selection belongs in manifest features and explicit bootstrap/install lanes. CMake presets consume the already-bootstrapped root `vcpkg_installed` tree with `VCPKG_MANIFEST_INSTALL=OFF` rather than installing packages during configure.
- The upstream ENet repository records MIT license and the vcpkg port `enet` 1.3.18 installs upstream `LICENSE` through `vcpkg_install_copyright`.

## Goal / Context / Constraints / Done When

**Goal:** Provide a reviewable, fail-closed optional network transport adapter proof for loopback exchange tests while preserving the existing value-only generated-game networking foundation and avoiding broad multiplayer claims.

**Context:** `engine-networking-foundation-v1` already provides reviewed multiplayer intent rows but explicitly does not open sockets or run transport middleware. This slice adds a developer-owned adapter lane only for one local loopback proof, useful for host/package validation and future transport work.

**Constraints:**
- Clean breaking greenfield implementation; no compatibility shim or legacy alias.
- `MK_runtime` public API stays first-party and backend-neutral.
- `MK_runtime` must not depend on ENet or vcpkg.
- `MK_runtime_network_enet` is optional and built only when `MK_ENABLE_NETWORK_ENET=ON` with bootstrapped `network-enet`.
- ENet headers, ENet symbols, socket handles, and native transport handles must not appear in public headers.
- v1 loopback supports exactly one client peer; multi-peer requests fail closed until a future multi-peer routing design exists.
- Local dependency bootstrap may be policy-blocked in approval-free sessions; optional ENet execution evidence must then come from hosted Windows CI or an unrestricted local host.
- `engine/agent/manifest.json` is compose output only; edit fragments and run `tools/compose-agent-manifest.ps1 -Write`.

**Done when:**
- Facade tests prove missing adapter, unavailable adapter, native-handle request rejection, loopback/delivery capability rejection, peer/channel/payload/service-budget validation, exception mapping, and successful caller-owned adapter exchange rows.
- Optional ENet tests prove capabilities and reliable/unreliable loopback payload exchange without exposing native handles.
- CMake/vcpkg/install/export surfaces expose `network-enet`, `MK_ENABLE_NETWORK_ENET`, `MK_runtime_network_enet`, and `Mirakanai_HAS_NETWORK_ENET` without configure-time package install.
- Dependency/legal notices record ENet and keep the default build dependency-free.
- Docs, roadmap, plan registry, manifest fragments, game-code guidance, run-validation recipe allowlist, CI, and static checks separate the optional loopback proof from broad generated-game networking readiness.
- Focused C++ build/test/static checks, the optional `network-enet` lane on a dependency-ready host or CI, and final validation evidence are recorded.
- Task-owned branch is committed, pushed, and opened as a PR with validation evidence and any local host blockers.

## Task 1: First-Party Runtime Transport Facade

- [x] Add failing tests for missing adapter, unavailable adapter, native-handle requests, unsupported loopback/delivery capabilities, invalid peer/channel/payload/service budgets, adapter exceptions, and successful first-party exchange rows.
- [x] Add `mirakana/runtime/network_transport.hpp` and `execute_runtime_network_loopback_exchange` with fail-closed pre-dispatch validation.
- [x] Clamp payload and service limits to the stricter of facade constants and adapter capabilities.
- [x] Fail closed for `peer_count != 1` until multi-peer routing is designed end-to-end.
- [x] Build and run `MK_runtime_network_transport_adapter_tests` under the `dev` preset.

## Task 2: Optional ENet Adapter Lane

- [x] Add `network-enet` vcpkg feature, `MK_ENABLE_NETWORK_ENET`, `network-enet` CMake preset, and optional `MK_runtime_network_enet` target.
- [x] Add focused ENet tests for capabilities and reliable/unreliable loopback exchange.
- [x] Implement ENet library lifetime, server/client host creation, loopback connection, packet send/receive, flush, disconnect cleanup, and packet destruction through private ENet implementation code.
- [x] Run `tools/validate-network-enet.ps1` on a dependency-ready host or hosted Windows CI.

## Task 3: Dependency / Install / Boundary Surface

- [x] Add conditional install/export package config for `Mirakanai_HAS_NETWORK_ENET` and `find_dependency(unofficial-enet CONFIG)`.
- [x] Update dependency policy, vcpkg environment checks, and public API boundary checks for `network-enet` and ENet public-header bans.
- [x] Update `docs/dependencies.md`, `docs/legal-and-licensing.md`, and `THIRD_PARTY_NOTICES.md`.

## Task 4: Agent Surface Drift Sync

- [x] Update current capabilities, roadmap, architecture, AI game-development guidance, plan registry, and production backlog.
- [x] Update manifest fragments for command, module, runtime readiness, validation recipe, active plan, and game-code guidance.
- [x] Compose `engine/agent/manifest.json`.
- [x] Update static check needles and run-validation recipe allowlist for `network-enet`.

## Task 5: Validation And PR

- [x] RED evidence: new facade tests failed before validation fixes for unsupported multi-peer requests and facade payload ceiling enforcement.
- [x] GREEN evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_network_transport_adapter_tests`.
- [x] GREEN evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_network_transport_adapter_tests`.
- [x] Focused static checks listed in this plan pass.
- [x] `tools/validate-network-enet.ps1` either passes on a dependency-ready host/CI or records the exact local dependency-bootstrap blocker.
- [x] `tools/validate.ps1` passes for the coherent runtime/build/public-contract slice or records a concrete blocker.
- [x] Candidate branch is committed, pushed, and opened as a PR with validation evidence.

## Validation Evidence

- Context7 ENet and vcpkg documentation review completed before adapter implementation.
- Local optional dependency state before bootstrap: `vcpkg_installed/x64-windows/include/enet/enet.h` and `share/unofficial-enet/unofficial-enet-config.cmake` were absent.
- Local `tools/bootstrap-deps.ps1` was blocked in the approval-free Codex command-policy session because vcpkg bootstrap/install intentionally launches network/dependency commands that require approval in this repository policy.
- Local `tools/validate-network-enet.ps1` therefore failed at CMake configure with missing `unofficial-enet`; hosted Windows CI now runs the wrapper after `tools/bootstrap-deps.ps1`.
- `cpp-reviewer` found two facade issues: multi-peer public request shape was not fail-closed, and payload validation used only adapter capability. Both were fixed with regression tests.
- `agent-surface-auditor` found missing dependency/legal/plan/manifest/static sync; those surfaces are updated in this slice.
- Focused static checks passed before PR publication: `tools/check-dependency-policy.ps1`, `tools/check-validation-recipe-runner.ps1`, `tools/check-format.ps1`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-public-api-boundaries.ps1`, and `git diff --check`.
- Full local validation passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, log `out/validation-logs/validate-20260524-141930-36696`, with 78/78 CTest tests passed.
- Hosted PR #210 evidence passed at head `0dc4d2a84857f2698d29b0b3ed21133d04c6688d`: PR Gate, Windows MSVC, Windows C++23 Release Evaluation, Agent Static Guards, Linux CMake, Linux Clang ASan/UBSan, Linux Coverage, Full Repository Static Analysis shards, macOS Metal CMake, iOS Simulator smoke, and CodeQL. The Windows MSVC lane ran `tools/validate-network-enet.ps1` after dependency bootstrap and validated the optional ENet adapter.
- PR #210 merged into `main` as merge commit `e63799b64716ad1cccb6841850edd7c5aac9d8ef`; `git log origin/main..0dc4d2a84857f2698d29b0b3ed21133d04c6688d` was empty after fetch, proving the PR head is contained in `origin/main`.
