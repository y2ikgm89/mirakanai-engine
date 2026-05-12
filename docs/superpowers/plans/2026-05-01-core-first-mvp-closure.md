# Core-First MVP Closure Implementation Plan (2026-05-01)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close the current `core-first-mvp` scope as a verified MVP, with the completed baseline and remaining host-gated or next-phase work represented consistently in docs, plan registry, and `engine/agent/manifest.json`.

**Architecture:** Treat `core-first-mvp` as the current verified foundation stage rather than a claim of a complete commercial game engine. Do not add new engine capability unless validation reveals a real blocker. Preserve the existing boundaries: `engine/core` stays standard-library-only, platform and renderer work remain behind first-party contracts, and native OS/GPU/SDL/Dear ImGui handles stay out of public game APIs.

**Tech Stack:** C++23, CMake, CTest, PowerShell 7 scripts under `tools/`, `engine/agent/manifest.json`, docs/roadmap, plan registry, Codex/Claude agent surfaces.

---

## Goal

Make the repository able to say: `core-first-mvp` is 100% complete for its current MVP scope because the source-tree build/test/agent contract is validated, docs and manifest describe the same capability set, and unverified Apple/iOS/Metal or larger production features are explicitly not part of the MVP completion claim.

## Context

- The historical `docs/superpowers/plans/2026-04-25-core-first-mvp.md` plan is already closed as implementation evidence.
- The active strategic ledger is `docs/superpowers/plans/2026-04-26-engine-excellence-roadmap.md`.
- `engine/agent/manifest.json` still uses `engine.stage = "core-first-mvp"` as the machine-readable stage name.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` on this Windows host passes before closure edits, with diagnostic-only host gates for Metal shader tools, Apple packaging/Xcode, Android signing/device smoke, and strict clang-tidy compile database availability.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` generates manifest-derived agent context successfully before closure edits.

## Constraints

- Do not redefine MVP as commercial-engine completeness.
- Do not mark Apple/iOS/Metal complete without macOS/Xcode or CI evidence.
- Do not introduce third-party dependencies for closure documentation.
- Do not move native handles into public game APIs.
- Do not make `engine/core` depend on OS, GPU, asset format, renderer, platform, or editor code.
- Keep sample-game and generated-game claims limited to the currently validated source-tree, desktop-runtime, and package lanes.

## Closure Criteria

`core-first-mvp` is complete for MVP closure when all of these are true:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` exits 0.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` exits 0.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` exit 0 after docs/manifest edits.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` exits 0 if public API or backend interop wording or files changed.
- `engine/agent/manifest.json` honestly reports current capabilities, validation recipes, packaging targets, host-gated Apple/iOS/Metal state, and next runtime targets.
- `docs/roadmap.md` separates completed MVP foundation from next-phase candidates.
- `docs/superpowers/plans/README.md` records this closure plan and marks `core-first-mvp` as closed for MVP scope.
- Host-gated items remain listed with concrete validation blockers instead of being included in the completion claim.
- Sample games, generated-game scaffolds, desktop runtime, and desktop package readiness are described as current validated proofs, not generalized production guarantees.

## Done When

- [x] Create this closure plan with Goal, Context, Constraints, and Done when.
- [x] Update the plan registry so this closure plan is the current focused slice while active.
- [x] Update roadmap language so `core-first-mvp` is 100% complete as MVP scope and next-phase work remains separate.
- [x] Update `engine/agent/manifest.json` so the machine-readable stage has an explicit completion status and closure evidence.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` to refresh sample and generated desktop-runtime source-tree readiness.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1` to refresh default desktop-runtime package readiness on this host.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Update this plan and the plan registry to completed after verification.

## MVP Scope Closed

- Standard-library-first deterministic foundations for core, math, assets, animation, AI, audio, platform contracts, runtime packages/session/input, scene, physics, navigation, first-party runtime UI contracts, renderer/RHI contracts, tools, and editor-core models.
- Optional SDL3 desktop runtime host and presentation lane with validated sample shells, generated desktop runtime package scaffolds, cooked-scene package scaffolds, material/shader package scaffolds, metadata-derived package files, and installed package validation recipes.
- Windows D3D12 and Windows/toolchain-gated Vulkan visible proofs behind private backend ownership and first-party status/report fields.
- Agent-facing docs, manifest, schema checks, Codex/Claude skill and subagent surfaces, and generated-game guidance.

## MVP-External Follow-Up

- Apple-hosted iOS simulator/device validation, device signing, and backend-neutral Metal visible presentation.
- Production material/shader graph authoring, live shader generation, broader authored/cooked packaged-game conventions, and richer editor UX.
- Broader postprocess effects, cascaded/atlased shadows, hardware comparison samplers, production shadow quality, GPU skinning/upload, and Metal shadow presentation.
- Concrete production text/font/IME/accessibility/image decoding/upload adapters for `mirakana_ui`.
- Navmesh/crowd foundations, richer physics controllers/joints/CCD/authored collision assets, streaming codec integration, HRTF/DSP graphs, telemetry/crash reporting, allocator diagnostics, and GPU marker adapters.

## Validation Evidence

- Pre-closure baseline: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` exited 0.
- Pre-closure baseline: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` exited 0 with 22/22 CTest tests passing and diagnostic-only blockers for Metal tools, Apple packaging/Xcode, Android signing/device smoke, and strict clang-tidy compile database availability.
- Closure checks: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` exited 0 and validated AI integration, generated scaffold checks, required skills, agents, and closure manifest fields.
- Closure checks: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` exited 0 and validated JSON contracts, manifest closure fields, runtime package file rules, and game manifests.
- Closure checks: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` exited 0; no public native API boundary regression was detected.
- Desktop runtime: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` initially hit the known sandbox-only vcpkg 7zip extraction blocker, then exited 0 outside the sandbox. It built the SDL3 desktop-runtime preset and passed 12/12 CTest tests, including runtime host, SDL3 platform/audio, presentation public API compile, `sample_desktop_runtime_shell`, `sample_desktop_runtime_game`, generated config package, generated cooked-scene package, and generated material/shader package smokes.
- Desktop runtime package: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1` initially hit the same sandbox-only vcpkg 7zip extraction blocker, then exited 0 outside the sandbox. It passed 7/7 desktop-runtime-release CTest tests, installed and validated `sample_desktop_runtime_shell`, validated the installed SDK consumer, and generated the desktop-runtime ZIP plus SHA256.
- Final default validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` exited 0 with 22/22 CTest tests passing. Diagnostic-only blockers remained explicit for Metal tools, Apple packaging/Xcode, Android release signing/device smoke, and strict clang-tidy compile database availability.
- Final default build: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` exited 0 for the `dev` preset.
