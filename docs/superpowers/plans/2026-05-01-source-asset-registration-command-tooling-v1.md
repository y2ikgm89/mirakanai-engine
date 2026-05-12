# Source Asset Registration Command Tooling v1 Implementation Plan (2026-05-01)

> **For agentic workers:** Use this as the next focused C-phase slice after `scene-prefab-authoring-command-tooling-v1`. Do not append this work to completed scene/prefab, scene-package, material, UI, validation-runner, or MVP plans.

**Goal:** Add a reviewed AI-safe dry-run/apply command surface for registering first-party source assets and their deterministic import intent, so agents can connect authored Scene/Prefab v2 data to known texture, mesh, audio, material, and scene sources without broad package cooking, arbitrary filesystem edits, or renderer/RHI residency claims.

**Architecture:** Keep `mirakana_assets` as the owner of asset identity, source-format, import metadata, and dependency contracts. Put command planning/apply helpers in `mirakana_tools` and consume public `mirakana_assets`/`mirakana_platform` APIs only. Do not make `mirakana_scene`, `mirakana_assets`, `mirakana_ui`, or gameplay-facing APIs depend on renderer, RHI, platform backend, SDL3, Dear ImGui, OS, GPU, or native handles.

**Tech Stack:** C++23 `mirakana_assets`/`mirakana_tools`, focused C++ tests, `engine/agent/manifest.json`, schema/static checks, `tools/agent-context.ps1`, docs, and existing validation recipes.

---

## Goal

Make source asset registration AI-operable through reviewed typed operations:

- dry-run a source asset registration request and report planned asset metadata/model/file mutations
- apply only reviewed structured source asset metadata writes after validation
- support initial first-party source asset kinds that existing `mirakana_assets`/`mirakana_tools` contracts can validate deterministically
- reject unsafe paths, duplicate asset keys, unsupported external importer readiness claims, arbitrary shell/free-form edits, package cooking claims, renderer/RHI residency claims, and package streaming claims
- keep actual cooked package row updates on the existing narrow package apply surfaces until a focused package-cooking plan promotes more

## Context

- Scene/Prefab v2 authoring commands are ready for source `.scene` / `.prefab` data.
- `Asset Identity v2` is foundation-only and provides stable asset keys and deterministic identity text IO.
- Existing asset import planning/execution can decode first-party source documents and optional audited import adapters, but the AI command descriptor `register-source-asset` is still descriptor-only.
- The next practical AI authoring gap is allowing agents to register source assets through a structured helper before later package cooking or renderer residency work.

## Constraints

- Do not add third-party dependencies.
- Do not run arbitrary shell, raw importer command strings, or free-form edit commands.
- Do not broaden package cooking, package streaming, renderer/RHI residency, material/shader graph, live shader generation, editor productization, or Metal readiness claims.
- Keep optional PNG/glTF/audio importer adapters behind existing dependency and validation gates; this slice may describe/import-intent them honestly, but must not claim optional adapter readiness unless the existing feature lane validates it.
- Keep cooked package `.geindex` updates on existing reviewed package update surfaces unless this plan explicitly adds focused RED checks for a narrow source registration metadata file.

## Done When

- Focused RED -> GREEN evidence is recorded here.
- A reviewed `register-source-asset` dry-run/apply API exists for the initial operation allowlist.
- Static checks require the reviewed source asset registration descriptor and reject broad source import/package/renderer claims.
- Docs tell agents when to use source asset registration, Scene/Prefab v2 authoring commands, and existing cooked package update tooling.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused C++ tests, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`, selected cheap runner execute test, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Implementation Tasks

### Task 1: Inventory And Contract

**Files:**
- Read: `engine/agent/manifest.json`
- Read: `engine/assets/include/mirakana/assets/asset_identity.hpp`
- Read: `engine/assets/include/mirakana/assets/asset_import_metadata.hpp`
- Read: `engine/assets/include/mirakana/assets/asset_source_format.hpp`
- Read: `engine/assets/include/mirakana/assets/asset_registry.hpp`
- Read: `engine/tools/include/mirakana/tools/asset_import_tool.hpp`
- Read: `engine/tools/src/asset_import_tool.cpp`
- Read: `tests/unit/tools_tests.cpp`
- Read: `tools/check-ai-integration.ps1`
- Read: `tools/check-json-contracts.ps1`
- Read: `tools/agent-context.ps1`
- Read: `docs/ai-game-development.md`
- Read: `docs/testing.md`

- [x] Decide the initial operation allowlist and request/result field names.
- [x] Define safe path, asset key, source format, and duplicate registration rules.
- [x] Define dry-run changed-file/model-mutation rows and apply diagnostics.
- [x] Decide whether the first helper writes a source asset registry document, updates an existing first-party asset identity document, or only plans source import metadata for a later apply surface.
- [x] Record explicit non-goals.

**Task 1 decisions, 2026-05-01:**

- Initial operation allowlist: `register_source_asset` only. Free-form edit, update, delete, rename, bulk scan, importer execution, package cooking, manifest patching, renderer/RHI residency, and package streaming are rejected diagnostics.
- Request field names: C++ `SourceAssetRegistrationRequest` with `kind`, `source_registry_path`, optional dry-run `source_registry_content`, `asset_key`, `asset_kind`, `source_path`, `source_format`, `imported_path`, `dependency_rows`, and explicit unsupported claim sentinels. Agent descriptor fields should use `mode`, `operation`, `sourceRegistryPath`, `assetKey`, `assetKind`, `sourcePath`, `sourceFormat`, `importedPath`, `dependencyRows`, and `importSettings`. `cookedOutputHint` is intentionally removed.
- Result field names: C++ `SourceAssetRegistrationResult` with `source_registry_content`, `changed_files`, `model_mutations`, `import_metadata`, `asset_identity_projection`, `diagnostics`, `validation_recipes`, `unsupported_gap_ids`, and placeholder `undo_token`. Agent descriptor dry-run fields are `changedFiles`, `modelMutations`, `importMetadata`, `validationRecipes`, `unsupportedGapIds`, and `undoToken`.
- Safe paths: repository-relative forward-slash paths only; reject empty, absolute, drive colon, backslash, control characters, semicolon, empty segment, `.`, and `..`. Apply performs this validation before filesystem reads.
- Asset keys: no raw `AssetId` input. Keys are lower-case ASCII path-like segments, derived through `asset_id_from_key_v2`, projected through `AssetIdentityDocumentV2`, and rejected on duplicate key/id or invalid canonical projection.
- Source formats: initial ready allowlist is `GameEngine.TextureSource.v1`, `GameEngine.MeshSource.v2`, `GameEngine.AudioSource.v1`, `GameEngine.Material.v1`, and `GameEngine.Scene.v1` mapped to `texture`, `mesh`, `audio`, `material`, and `scene`. `GameEngine.Scene.v2`, `GameEngine.Prefab.v2`, PNG, glTF, common audio, shader, script, and ui_atlas registration remain unsupported in this command.
- Dependencies: `dependency_rows` use source asset keys and the existing dependency kinds `material_texture`, `scene_mesh`, `scene_material`, and `scene_sprite`. Dependency keys must already exist in the same source registry and duplicate dependency rows are rejected.
- Duplicate registration: exact canonical duplicate may be treated as no-op success; conflicting duplicate asset keys, derived ids, source paths, imported paths, or dependency rows are rejected deterministically.
- Mutation target: add a narrow `GameEngine.SourceAssetRegistry.v1` document owned by `mirakana_assets`. `GameEngine.AssetIdentity.v2` remains a projection/export validation surface, not the command-owned write target, because it cannot store source format, imported path, dependency rows, or import settings.
- Dry-run rows: return a changed file only when canonical registry content would change; `document_kind` is `GameEngine.SourceAssetRegistry.v1`. Return one `register_source_asset` model mutation and source import metadata rows, with no filesystem mutation.
- Apply diagnostics: apply reloads current registry content from `IFileSystem` after shape/path validation, reuses the dry-run planner, and writes only the validated `changed_files`. Filesystem read/write failures become deterministic diagnostics.
- Non-goals: importer execution, cooked artifact writes, cooked `.geindex` package updates, runtime source parsing, Scene v2 runtime package migration, material/shader graph, live shader generation, texture compression/mip/LOD/collision generation execution, editor productization, package streaming, renderer/RHI residency, GPU upload, native handles, Metal readiness, and arbitrary shell/free-form edit/eval.

### Task 2: RED Checks And Tests

**Files:**
- Modify: focused `mirakana_tools` / `mirakana_assets` tests
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: this plan

- [x] Add failing focused tests for dry-run source asset registration.
- [x] Add failing focused tests for apply changed files and deterministic serialization.
- [x] Add failing focused tests for duplicate asset keys, unsafe paths, unsupported source formats, unsupported external importer claims, and free-form edits.
- [x] Add failing static checks requiring a reviewed `register-source-asset` descriptor.
- [x] Add failing static checks rejecting package cooking, renderer/RHI residency, package streaming, material/shader graph, live shader generation, and editor productization ready claims.
- [x] Record RED evidence before production implementation.

### Task 3: Production Implementation

**Files:**
- Modify or create: `mirakana_tools` source asset registration helpers
- Modify: focused tests
- Modify: CMake target registration if new test files are added

- [x] Implement typed request/result value APIs with deterministic diagnostics.
- [x] Implement dry-run planning without filesystem mutation.
- [x] Implement apply through validated file writes only.
- [x] Reuse canonical `mirakana_assets` asset identity/source/import metadata validation as the source of truth.
- [x] Keep package cooking and package index updates on existing reviewed package surfaces.

### Task 4: Manifest, Docs, And Agent Context Sync

**Files:**
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `tools/agent-context.ps1` if the top-level output needs a new field
- Modify: `docs/ai-game-development.md`
- Modify: `docs/testing.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/specs/game-prompt-pack.md`
- Modify: `docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`

- [x] Promote only the reviewed source asset registration subset to ready.
- [x] Keep broad source import execution, package cooking, renderer/RHI residency, package streaming, editor productization, material/shader graphs, and live shader generation planned or host-gated.
- [x] Update generated-game guidance so agents register source assets separately from authored Scene/Prefab v2 edits and cooked package row updates.
- [x] Keep `currentActivePlan` and `recommendedNextPlan` synchronized with the plan registry.

### Task 5: Validation

**Files:**
- Modify: this plan's Validation Evidence section
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.json`
- Modify: `docs/roadmap.md`

- [x] Run focused source asset registration tests.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`.
- [x] Run a cheap validation runner execute test, for example `agent-contract`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record evidence, mark this plan complete, and create the next dated focused plan.

## Validation Evidence

Record command results here while implementing this plan.

- RED 2026-05-01:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` failed as expected after adding source asset registration tests, with `tools_tests.cpp(26,10): error C1083: include file cannot be opened: 'mirakana/tools/source_asset_registration_tool.hpp'`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed as expected with `engine manifest aiOperableProductionLoop must expose one ready register-source-asset command surface`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed as expected with `engine/agent/manifest.json aiOperableProductionLoop must expose one ready register-source-asset command surface`.
- GREEN 2026-05-01:
  - `G:\workspace\development\GameEngine\out\build\dev\Debug\mirakana_tools_tests.exe` passed, including source asset registration dry-run/apply/rejection coverage.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed and emitted the manifest context with the active production loop.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed with `json-contract-check: ok`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed with `ai-integration-check: ok`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` passed, 28/28.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed with `public-api-boundary-check: ok`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` completed diagnostic-only: D3D12 DXIL, Vulkan SPIR-V, DXC SPIR-V CodeGen, `dxc`, and `spirv-val` ready; Metal `metal` / `metallib` missing and host-gated.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract` passed with `status: passed` and both `agent-check` and `schema-check` exit code 0.
- Review hardening 2026-05-01:
  - `cpp-reviewer` found dependency target-kind validation, full ASCII control-character path rejection, canonical dry-run dependency ordering, exact duplicate no-op coverage, and plan synchronization gaps.
  - `mirakana_assets` now rejects dependency rows whose dependency kind does not match the referenced source asset kind, including self-dependencies.
  - `mirakana_tools` now rejects all ASCII control characters in command path fields and projects dry-run model/import metadata rows from the canonical registry row rather than raw request dependency order.
  - Focused tests now cover wrong-kind dependencies, self-dependencies, tab/control-character paths, canonical dependency ordering, and exact duplicate no-op results.
- Final GREEN 2026-05-01:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` passed, 28/28, after review hardening.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed with `json-contract-check: ok`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed with `ai-integration-check: ok`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed and now reports `docs/superpowers/plans/2026-05-01-scene-v2-runtime-package-migration-v1.md` as the current active plan.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed with `public-api-boundary-check: ok`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` completed diagnostic-only with the existing Metal `metal` / `metallib` host gate.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract` passed with `status: passed`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `validate: ok`; existing diagnostic-only gates remain Metal/Apple tooling, Android release signing/device smoke configuration, and tidy compile database availability.
