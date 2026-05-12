# AI-Operable Production Loop Foundation v1 Implementation Plan (2026-05-01)

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` or `superpowers:executing-plans` to implement this plan task-by-task. Use project skills for agent integration, game development, rendering, editor, CMake, and license work when their trigger conditions apply. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Give Codex and similar agents a truthful machine-readable production loop for creating 2D and 3D games with GameEngine, before deeper runtime/editor/renderer rewrites begin.

**Architecture:** This slice is contract and guidance work only. It extends the AI-facing manifest, schema/static checks, agent context output, generated-game scenarios, and prompt guidance so agents can select supported recipes and reject unsupported claims. It does not implement Scene v2, Resource v2, Frame Graph v1, 2D sprite/tilemap runtime, 3D production renderer features, or editor productization.

**Tech Stack:** C++23 repository policy, JSON schemas, PowerShell validation scripts, PowerShell 7 `tools/*.ps1` wrappers, Markdown docs, `engine/agent/manifest.json`, `tools/agent-context.ps1`, and existing game manifests.

---

## Goal

Make the next production phase concrete and executable by adding a first machine-readable AI production loop:

- game intent classification
- recipe selection
- supported and unsupported capability checks
- authoring/runtime/package surfaces
- validation recipe mapping
- host-gated backend truthfulness
- next-step diagnostics for AI agents

This plan is the bridge between the completed `core-first-mvp` foundation and later breaking engine work such as Scene/Component/Prefab Schema v2, Asset Identity v2, Runtime Resource v2, Frame Graph v1, and 2D/3D vertical slices.

## Context

- `core-first-mvp` is closed by `docs/superpowers/plans/2026-05-01-core-first-mvp-closure.md` as an MVP scope, not as a complete commercial game engine.
- `docs/specs/2026-05-01-ai-operable-game-engine-production-roadmap-v1-design.md` is the broad design for the next phase.
- The current repo already has strong pieces: `engine/agent/manifest.json`, `tools/agent-context.ps1`, generated game templates, desktop runtime package validation, cooked scene packages, D3D12 package proof, Vulkan gated proof, editor-core authoring models, and many deterministic headless systems.
- The current repo does not yet have an AI-safe game-spec-to-scene-to-package-to-visible-smoke production recipe.
- The user explicitly prefers official best practices, clean architecture, and no backward compatibility burden.

## Constraints

- Do not append this work to historical MVP plans.
- Do not claim production renderer, production editor, production 2D pipeline, production 3D pipeline, production UI adapters, Metal readiness, or unrestricted Vulkan readiness in this slice.
- Do not add third-party dependencies.
- Do not expose SDL3, Win32, D3D12, Vulkan, Metal, Dear ImGui, or RHI backend handles through gameplay-facing APIs or AI game recipes.
- Keep `engine/core` independent from OS, GPU, asset format, renderer, platform, editor, SDL3, Dear ImGui, and UI middleware APIs.
- Keep source asset parsing in tools/editor workflows. Runtime game code consumes cooked packages and public engine APIs.
- Keep generated game recipes conservative: planned, blocked, or host-gated capabilities must be rejected unless the task is explicitly implementing that engine capability.

## Done When

- [x] `docs/superpowers/plans/README.md` marks this plan as the active focused slice while implementation is underway.
- [x] `engine/agent/manifest.json` exposes an AI-operable production loop section with supported recipes, unsupported gaps, command surfaces, authoring surfaces, package surfaces, backend gates, and validation recipes.
- [x] `schemas/engine-agent.schema.json` validates the new manifest contract.
- [x] `tools/check-ai-integration.ps1` statically rejects missing production-loop fields, stale recipe ids, missing validation recipes, or false ready claims.
- [x] `tools/agent-context.ps1` emits the production-loop map so AI agents do not need to infer it from prose.
- [x] `docs/ai-game-development.md`, `docs/specs/generated-game-validation-scenarios.md`, and `docs/specs/game-prompt-pack.md` point to the new production recipes and keep unsupported features explicit.
- [x] `docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md` stays synchronized with the final workflow.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passes and includes the new production-loop fields.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passes.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passes.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes, or a concrete local environment blocker is recorded.
- [x] This plan records validation evidence before it is marked completed.

## Implementation Tasks

### Task 1: Baseline And RED Checks

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` and inspect the current machine-readable context.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` before edits.
- [x] Add schema/check expectations first so the repo fails when the new production-loop contract is absent.
- [x] Confirm the failure is from the intended missing contract, not from unrelated environment state.

### Task 2: Manifest Production Loop Contract

- [x] Add a top-level manifest section such as `aiOperableProductionLoop`.
- [x] Include recipe ids for the first supported or planned lanes:
  - `headless-gameplay`
  - `desktop-runtime-config-package`
  - `desktop-runtime-cooked-scene-package`
  - `desktop-runtime-material-shader-package`
  - `ai-navigation-headless`
  - `runtime-ui-headless`
  - `future-2d-playable-vertical-slice`
  - `future-3d-playable-vertical-slice`
- [x] For each recipe, define:
  - supported status: `ready`, `host-gated`, `planned`, or `blocked`
  - required modules
  - allowed templates
  - allowed packaging targets
  - importer and cooked-runtime assumptions
  - renderer/backend assumptions
  - required validation recipes
  - explicit unsupported claims
  - follow-up engine capability that would make it ready
- [x] Add command-surface rows for planned AI commands without claiming they are implemented:
  - `create-game-recipe`
  - `create-scene`
  - `add-scene-node`
  - `add-or-update-component`
  - `create-prefab`
  - `instantiate-prefab`
  - `create-material-instance`
  - `register-source-asset`
  - `cook-runtime-package`
  - `register-runtime-package-files`
  - `update-game-agent-manifest`
  - `run-validation-recipe`
- [x] Keep every command row honest with `status`, `owner`, `inputContract`, `outputContract`, `dryRun`, `apply`, and `validation` fields.

### Task 3: Schema And Static Integration Checks

- [x] Update `schemas/engine-agent.schema.json` for the new production-loop section.
- [x] Extend `tools/check-json-contracts.ps1` if schema coverage needs stronger structural checks.
- [x] Extend `tools/check-ai-integration.ps1` to require:
  - the production-loop section exists
  - each recipe has a status and validation mapping
  - future 2D/3D recipes are not marked ready
  - D3D12, Vulkan, and Metal readiness matches existing backend truthfulness
  - prompt/scenario docs mention the recipe ids
- [x] Keep checks deterministic and text-based where possible so CI failures are readable.

### Task 4: Agent Context Export

- [x] Extend `tools/agent-context.ps1` to include:
  - production-loop recipes
  - AI command surfaces
  - unsupported production gaps
  - host-gated validation notes
  - first recommended next plan
- [x] Keep output JSON stable and sorted where current script patterns do so.
- [x] Ensure agents can consume this information without scraping long prose documents.

### Task 5: Documentation And Prompt Guidance

- [x] Update `docs/ai-game-development.md` to add the production-loop recipe selection step before game code generation.
- [x] Update `docs/specs/generated-game-validation-scenarios.md` so scenarios map to production-loop recipe ids.
- [x] Update `docs/specs/game-prompt-pack.md` with a constrained production-loop prompt.
- [x] Keep `docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md` synchronized.
- [x] Keep future capabilities explicit:
  - Scene/Component/Prefab Schema v2
  - Asset Identity v2
  - Runtime Resource v2
  - Renderer/RHI Resource Foundation
  - Frame Graph v1
  - Upload/Staging v1
  - 2D Playable Vertical Slice
  - 3D Playable Vertical Slice
  - Editor Productization
  - Production UI/importer/platform adapters

### Task 6: Validation And Closure

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` and inspect that production-loop fields are emitted.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` only if public headers or backend interop surfaces changed.
- [x] Record validation evidence in this plan.
- [x] Update `docs/superpowers/plans/README.md` after completion.

## Proposed Manifest Shape

This shape is illustrative. The implementation should follow the existing manifest style and schema conventions.

```json
{
  "aiOperableProductionLoop": {
    "schemaVersion": 1,
    "design": "docs/specs/2026-05-01-ai-operable-game-engine-production-roadmap-v1-design.md",
    "foundationPlan": "docs/superpowers/plans/2026-05-01-ai-operable-production-loop-foundation-v1.md",
    "currentActivePlan": "docs/superpowers/plans/2026-05-01-scene-component-prefab-schema-v2.md",
    "recipes": [
      {
        "id": "desktop-runtime-material-shader-package",
        "status": "ready-on-supported-windows-hosts",
        "templates": ["DesktopRuntimeMaterialShaderPackage"],
        "requiredModules": [
          "mirakana_runtime",
          "mirakana_scene",
          "mirakana_scene_renderer",
          "mirakana_runtime_host",
          "mirakana_runtime_host_sdl3_presentation"
        ],
        "packagingTargets": ["desktop-game-runtime", "desktop-runtime-release"],
        "validationRecipes": [
          "desktop-game-runtime",
          "selected-desktop-runtime-package"
        ],
        "unsupportedClaims": [
          "production material graph",
          "runtime shader compiler",
          "Metal ready",
          "public native or RHI handle access"
        ]
      }
    ],
    "commands": [
      {
        "id": "create-scene",
        "status": "planned",
        "owner": "editor-core/tools",
        "dryRun": true,
        "apply": false,
        "diagnostics": "structured"
      }
    ],
    "productionGaps": [
      "Scene/Component/Prefab Schema v2",
      "Asset Identity v2",
      "Runtime Resource v2",
      "Renderer/RHI Resource Foundation",
      "Frame Graph v1",
      "Upload/Staging v1",
      "2D Playable Vertical Slice",
      "3D Playable Vertical Slice",
      "Editor Productization"
    ]
  }
}
```

## Validation Evidence

Record command results here while implementing this plan.

- Planning artifact creation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` PASS after adding the roadmap design, focused plan, handoff prompt, roadmap pointer, and prompt-pack pointers.
- Planning artifact creation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` PASS.
- Planning artifact creation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` PASS.
- Planning artifact creation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` PASS with 22/22 CTest tests passing. Diagnostic-only gates remained explicit for missing Metal tools, Apple packaging on non-macOS hosts, Android release signing/device smoke, and strict tidy compile database availability at tidy-check start.
- Baseline before implementation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` PASS.
- Baseline before implementation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` PASS.
- Baseline before implementation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` PASS.
- Baseline before implementation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` PASS with 22/22 CTest tests passing; diagnostic-only gates remained explicit for Metal tools, Apple packaging on non-macOS hosts, Android release signing/device smoke, and tidy compile database availability.
- RED contract check: after adding schema/static expectations first, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed with `engine manifest missing required property: aiOperableProductionLoop`.
- RED integration check: after adding static expectations first, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed with `engine/agent/manifest.json missing required field: aiOperableProductionLoop`.
- Implementation contract check: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` PASS after adding `aiOperableProductionLoop`, schema coverage, and JSON contract checks.
- Implementation integration check: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` PASS after docs/scenario/prompt/handoff synchronization and top-level `agent-context` production-loop output checks.
- Implementation context check: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` PASS and emitted `productionLoop`, `productionRecipes`, `aiCommandSurfaces`, `unsupportedProductionGaps`, `hostGates`, and `recommendedNextPlan`.
- Final contract check: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` PASS after moving the active registry to `docs/superpowers/plans/2026-05-01-scene-component-prefab-schema-v2.md`.
- Final schema check: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` PASS.
- Final integration check: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` PASS.
- Final validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` PASS with 22/22 CTest tests passing. Diagnostic-only gates remained explicit for missing Metal tools, Apple packaging on non-macOS hosts, Android release signing/device smoke, and tidy compile database availability.
