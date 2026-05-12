# Phase 4–5 Production Closure Milestone v1 (2026-05-09)

**Plan ID:** `phase-4-5-closure-milestone-v1`  
**Status:** Completed (closure evidence: [2026-05-09-phase-4-5-milestone-closure-evidence-index-v1.md](2026-05-09-phase-4-5-milestone-closure-evidence-index-v1.md)). Does **not** replace `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` unless maintainers explicitly pivot the active slice to this milestone.  
**Parent roadmap:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Close **Phase 4** (renderer / material / shader / quality) and **Phase 5** (source import / atlas / cook foundations) from `production-completion-master-plan-v1` through **dated child slices** until this milestone’s **completion definitions** are met, while preserving the master plan’s stance: **no broad “ready” claims** beyond what the *Phase Ledger* and *Remaining To 100% Ledger* allow, and alignment with **official best practices** (module boundaries, validation, manifest honesty).

- **No backward compatibility requirement** (greenfield policy in `AGENTS.md`). Contracts that existed only to satisfy MVP-era constraints may be replaced **breakingly** once a dated child plan plus tests and validation cover the new surface.
- **Do not expand ready claims:** each child slice must not add wording that contradicts **remaining boundaries** in `docs/current-capabilities.md`, `engine/agent/manifest.json`, or this document.

## Milestone closure record

- **Evidence index:** [2026-05-09-phase-4-5-milestone-closure-evidence-index-v1.md](2026-05-09-phase-4-5-milestone-closure-evidence-index-v1.md) maps Phase 4 / Phase 5 completion bullets to plans, tests, and manifests.
- **Editor preview Vulkan/Metal tracking:** [2026-05-09-phase-4-5-milestone-editor-material-preview-vulkan-parity-tracking-v1.md](2026-05-09-phase-4-5-milestone-editor-material-preview-vulkan-parity-tracking-v1.md) (host-gated execution proofs; no automatic CI parity claim).
- **Final validation:** `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` recorded green when this milestone row closed.

## Context

- On the master plan, Phases 4–5 are **partially complete**. Do **not** redo completed slices (for example `material-graph-package-binding-v1`, bounded PBR/shadow/postprocess work, registered-source cook, UI atlas bridges, static generated 3D recipe slices); see the [Completed Implementation Evidence Index](2026-05-03-production-completion-master-plan-v1.md).
- In `engine/agent/manifest.json` `unsupportedProductionGaps`, rows such as **`renderer-rhi-resource-foundation`**, **`frame-graph-v1`**, **`upload-staging-v1`**, **`editor-productization`**, **`production-ui-importer-platform-adapters`**, and **2D/3D vertical-slice** entries can conflict with naïve “Phase 4–5 done” statements. Child slices **narrow** those rows; **bulk ready promotion** is **out of scope** for this milestone.

## Constraints

- Keep `engine/core` free of OS APIs, GPU APIs, asset formats, and editor code; GPU and windowing live under `engine/renderer`, `engine/rhi`, and hosts.
- Third-party additions require updates to `docs/legal-and-licensing.md`, `docs/dependencies.md`, `vcpkg.json`, and `THIRD_PARTY_NOTICES.md`. Do not pull vcpkg from CMake configure; use **`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1`** only.
- Before implementing a child slice, author a **dated focused plan** (Goal / Context / Constraints / Done when / validation table). Skills: `.claude/skills/gameengine-rendering`, `gameengine-feature`, `.agents/skills/rendering-change`, `.cursor/skills/gameengine-cursor-baseline`.
- Verification at slice completion follows **`AGENTS.md` Done Definition** (e.g. scoped `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`).

## Phase 4 — Milestone completion definition (master-plan aligned)

Phase 4 is **complete for this milestone** when all of the following hold (this is **not** “general renderer parity vs Unity/Unreal”).

1. **Quality and diagnostics consistency**  
   - New quality statements stay tied to the existing first-step path (`SdlDesktopPresentationReport`, `IRenderer::stats()`, etc.) consistent with **Renderer Package Quality Gates v1**. General GPU timestamps and public backend-native stats are **another milestone**.

2. **Material / shader cook path**  
   - The explicit **`GameEngine.MaterialGraph.v1` → runtime `GameEngine.Material.v1` package binding** (existing `plan_material_graph_package_update` surface) remains **re-validatable** through **at least one** host-gated desktop package smoke (D3D12 primary lane plus documented Vulkan gates).

3. **Editor material preview**  
   - Per `unsupportedProductionGaps.editor-productization`, track **Vulkan (and Metal if required) display parity** against the existing **D3D12 host-owned preview** path in a **dated child plan**, with either **host-gated** validation or **macOS/CI evidence**. Do **not** write “ready” on hosts that have not executed the proof.

4. **Frame graph / upload (foundational progress)**  
   - Do **not** require **full production renderer-wide pass migration** for `frame-graph-v1` / `upload-staging-v1`. Instead, complete the **narrow child-slice series** for asynchronous-upload diagnostics, RHI lifetime, and scheduled execution that **matches manifest `notes`**, and keep docs and gap rows **consistent on what “not implemented” means**.

## Phase 5 — Milestone completion definition

Phase 5 is **complete for this milestone** when all of the following hold.

1. **Registered-source cook**  
   - Alignment of `cook-registered-source-assets` with `registeredSourceAssetCookTargets` / `prefabScenePackageAuthoringTargets` remains **schema- and static-check clean**, and both **explicit selection** and **registry-closure** paths are **reproducible** via validation recipes or tests.

2. **Importer / atlas**  
   - Full importer productization is **out of scope**. Instead, fix **reviewed codec paths** (e.g. Content Browser + `ExternalAssetImportAdapters`) so cook / `.geindex` updates remain coherent, backed by **diagnostics models** and **at least one E2E-style test**.

3. **Boundary with the 2D vertical slice**  
   - Production atlas editing workflow and full tilemap canvas work listed under **`2d-playable-vertical-slice`** stay **out of scope**. Phase 5 closure here means **static recipes plus deterministic cook** only.

## Child plan queue (recommended order)

Each row must become its own **`docs/superpowers/plans/2026-__-__-*.md`** focused plan. Order may change with host availability and dependencies.

| Priority | Area | Intent | Primary gap row ids |
| --- | --- | --- | --- |
| P0 | Editor material preview parity | Visible proof on Vulkan (→ Metal) matching D3D12; explicit host gates | `editor-productization` |
| P1 | Frame graph / effect scheduling (narrow) | Additional effect-order binding on existing Frame Graph v1 without renderer-wide migration | `frame-graph-v1` |
| P1 | RHI native teardown / residency next step | Per-backend object-destruction migration slices | `renderer-rhi-resource-foundation` |
| P2 | Upload / streaming boundary | Continue integrating runtime upload evidence and **stale generation** diagnostics (production async is separate) | `upload-staging-v1`, `runtime-resource-v2` |
| P2 | Registered cook + importer | Regression tests and diagnostics lock-in for explicit cook paths | `asset-identity-v2`, `runtime-resource-v2` |
| P3 | UI / importer adapters | Advance **`planned`** state for `production-ui-importer-platform-adapters` one bounded step (deps/legal strict) | `production-ui-importer-platform-adapters` |

### Completed child slices (historical series)

- [2026-05-09-phase-4-5-milestone-closure-evidence-index-v1.md](2026-05-09-phase-4-5-milestone-closure-evidence-index-v1.md): Maps Phase 4 / Phase 5 milestone **completion definitions** to repository evidence; **does not** bulk-promote unsupported-gap rows.
- [2026-05-09-phase-4-5-milestone-editor-material-preview-vulkan-parity-tracking-v1.md](2026-05-09-phase-4-5-milestone-editor-material-preview-vulkan-parity-tracking-v1.md): Host-gated **tracking checklist** for editor material preview Vulkan/Metal parity vs D3D12; **no** executed parity proof claimed here.
- [2026-05-09-frame-graph-v1-unknown-access-contract-v1.md](2026-05-09-frame-graph-v1-unknown-access-contract-v1.md): `mirakana_renderer_tests` compile-only contract for **`FrameGraphAccess::unknown`** (`invalid_resource`) on Frame Graph v1.
- [2026-05-09-registered-source-cook-registry-path-and-parse-contract-v1.md](2026-05-09-registered-source-cook-registry-path-and-parse-contract-v1.md): `mirakana_tools_tests` contracts for **`unsafe_source_registry_path`** (non-`.geassets` suffix) and **`invalid_source_registry`** (parse failure); narrow registry/path validation only.
- [2026-05-09-frame-graph-v0-duplicate-declaration-contract-v1.md](2026-05-09-frame-graph-v0-duplicate-declaration-contract-v1.md): `mirakana_renderer_tests` compile-only contracts for **duplicate pass names** (`invalid_pass`) and **duplicate resource rows** (`invalid_resource`) on Frame Graph **v0**; legacy API diagnostic lock-in only.
- [2026-05-09-registered-source-cook-invalid-key-and-duplicate-inline-source-contract-v1.md](2026-05-09-registered-source-cook-invalid-key-and-duplicate-inline-source-contract-v1.md): `mirakana_tools_tests` contracts for **`invalid_selected_asset_key`** and **`duplicate_source_file`** on `plan_registered_source_asset_cook_package`; narrow explicit-selection / inline payload validation only.
- [2026-05-09-rhi-resource-lifetime-retired-handle-invalid-contract-v1.md](2026-05-09-rhi-resource-lifetime-retired-handle-invalid-contract-v1.md): `mirakana_rhi_resource_lifetime_tests` contracts for **`invalid_resource`** after `retire_released_resources` removes a deferred row; registry-layer teardown invalidation only, not per-backend native destruction migration.
- [2026-05-09-frame-graph-v1-duplicate-declaration-contract-v1.md](2026-05-09-frame-graph-v1-duplicate-declaration-contract-v1.md): `mirakana_renderer_tests` compile-only contracts for **duplicate pass names** (`invalid_pass`) and **duplicate resource rows** (`invalid_resource`) on Frame Graph v1; does **not** satisfy full P1 effect-order scheduling or `frame-graph-v1` gap closure.
- [2026-05-09-registered-source-cook-zero-source-revision-contract-v1.md](2026-05-09-registered-source-cook-zero-source-revision-contract-v1.md): `mirakana_tools_tests` contract for **`source_revision == 0`** (`invalid_source_revision`); narrow Phase 5 registered-source validation lock-in only.
- [2026-05-09-registered-source-cook-duplicate-selection-contract-v1.md](2026-05-09-registered-source-cook-duplicate-selection-contract-v1.md): `mirakana_tools_tests` contract for **duplicate** entries in `selected_asset_keys` (`duplicate_selected_asset_key`); narrow explicit-selection diagnostics only.
- [2026-05-09-frame-graph-v1-tie-break-and-cycle-contract-v1.md](2026-05-09-frame-graph-v1-tie-break-and-cycle-contract-v1.md): `mirakana_renderer_tests` compile-only proofs for parallel-ready **declaration-order** tie-break and **ping-pong cycle** diagnostics on Frame Graph v1; does **not** satisfy full P1 scheduling migration or `frame-graph-v1` gap closure.
- [2026-05-09-shader-pipeline-cache-empty-requests-and-registered-cook-selection-contract-v1.md](2026-05-09-shader-pipeline-cache-empty-requests-and-registered-cook-selection-contract-v1.md): `mirakana_tools_tests` contracts for empty shader pipeline cache request lists and empty registered cook selection; narrow Phase 4 / Phase 5 tooling regression lock-in only.
- [2026-05-09-frame-graph-minimal-three-pass-stack-proof-v1.md](2026-05-09-frame-graph-minimal-three-pass-stack-proof-v1.md): compile-only `mirakana_renderer_tests` proof for shadow → scene → post process ordering on Frame Graph v1; does **not** satisfy the full P1 row above (no effect-stack wiring into SDL hosts, no `frame-graph-v1` gap closure).

## Validation (milestone-wide)

- After each child slice: for touched targets run `cmake --preset dev` → `cmake --build --preset dev` → `ctest --preset dev`; if public headers or RHI interop changed, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`; if shared renderer patterns changed, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` when available.
- When all milestone sections are closed by child plans, run **`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`** and record evidence in this plan’s validation table.

## Done when

- Every bullet under **Phase 4 / Phase 5 Milestone completion definition** is satisfied by **matching child-plan Done when clauses and validation logs**.
- `docs/superpowers/plans/README.md` registry and `engine/agent/manifest.json` gap rows stay consistent on **notes / status** (reclassify to `implemented-contract-only` etc. when appropriate **without** fake ready claims).
- `docs/current-capabilities.md` does not gain **over-broad ready language**.

## Out of scope (not closed by this milestone)

- Apple / Metal **shipping parity** (host gates remain).
- Production package streaming, arbitrary eviction, enforced GPU allocator budgets.
- **General** live shader generation and **full** automatic graph codegen.
- Unity/UE-class editor UX, dynamic game modules, fully automatic import pipelines.
