# Asset Identity v2 Reference Cleanup Milestone Implementation Plan (2026-05-12)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Close the `asset-identity-v2` `scene/render/UI/gameplay reference cleanup` blocker by adding testable Asset Identity v2 provenance at runtime-scene boundaries and by requiring generated/game source package references to derive from `AssetKeyV2`.

**Architecture:** `AssetKeyV2` remains the source/authoring identity, while cooked/runtime/render/UI payloads continue to consume validated `AssetId` values. The milestone adds read-only provenance/audit rows where runtime scene references cross from cooked packages into game/render data, then moves game and generated-template constants to key-first construction without changing package formats, renderer/RHI residency, package streaming, or native handles.

**Tech Stack:** C++23, MIRAIKANAI `MK_assets`, `MK_runtime_scene`, sample games, `tools/new-game.ps1`, PowerShell static checks, composed `engine/agent/manifest.json`.

---

**Plan ID:** `asset-identity-v2-reference-cleanup-milestone-v1`

**Status:** Completed.

**Gap:** `asset-identity-v2`

**Context:** `AssetKeyV2`, `asset_id_from_key_v2`, placement planning, command-owned apply-surface evidence, Scene v2 runtime migration placement rows, read-only Content Browser identity rows, source-registry Content Browser population, and visible `MK_editor` source registry loading are already implemented. The remaining blocker is that scene/render/UI/gameplay references still have no broad, enforced Asset Identity v2 boundary: runtime scene refs expose only `AssetId`, sample/generated games create IDs from raw strings, and `tools/new-game.ps1` can still generate those raw calls.

**Constraints:**

- Do not convert cooked `GameEngine.Scene.v1`, runtime package indexes, renderer commands, RHI palettes, or UI payloads to store `AssetKeyV2`.
- Do not claim renderer/RHI residency, package streaming, source registry parsing at runtime, production UI font/image adapters, GPU upload, or broad 2D/3D vertical-slice readiness.
- Keep new provenance APIs dependency-directed: runtime scene may depend on `MK_assets`; renderer/RHI/UI must remain on validated `AssetId` contracts.
- Keep game code on public `mirakana::` headers only.
- Preserve current package keys and smoke behavior.
- Update docs, manifest fragments, composed manifest output, generated-game guidance, and static checks in the same milestone.
- Run focused validation before full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`; commit only coherent, verified milestone files.

**Done When:**

- `MK_runtime_scene` exposes a deterministic audit/report API that maps runtime scene mesh/material/sprite references to `AssetKeyV2`, kind, source path, and diagnostics from an `AssetIdentityDocumentV2`.
- Runtime scene tests cover matched references, missing identity rows, and wrong-kind diagnostics without changing cooked scene serialization.
- No `games/**/*.cpp` file calls `mirakana::AssetId::from_name`.
- `tools/new-game.ps1` no longer generates gameplay/template `AssetId::from_name` calls.
- Static checks reject future game/template raw ID construction and keep cooked/runtime non-goals explicit.
- The `asset-identity-v2` row leaves `unsupportedProductionGaps`; `runtime-resource-v2` becomes the recommended next foundation gap.
- Registry, master plan, roadmap, current capabilities, generated-game guidance, manifest fragments, composed manifest, and JSON/static checks agree.
- Full validation and build gates pass or record an exact blocker.
- A commit records the completed milestone slice.

### Task 1: Runtime Scene Asset Identity Provenance

**Files:**

- Modify: `engine/runtime_scene/include/mirakana/runtime_scene/runtime_scene.hpp`
- Modify: `engine/runtime_scene/src/runtime_scene.cpp`
- Modify: `tests/unit/runtime_scene_tests.cpp`

- [x] **Step 1: Write runtime scene provenance tests**

Add tests covering:

```cpp
const mirakana::AssetIdentityDocumentV2 identities{
    .assets = {
        {.key = {.value = "meshes/player"}, .kind = mirakana::AssetKind::mesh, .source_path = "source/meshes/player.mesh"},
        {.key = {.value = "materials/player"}, .kind = mirakana::AssetKind::material, .source_path = "source/materials/player.material"},
        {.key = {.value = "sprites/nameplate"}, .kind = mirakana::AssetKind::texture, .source_path = "source/textures/nameplate.texture"},
    },
};
```

Expected runtime scene audit rows:

- placement `scene.component.mesh_renderer.mesh`
- placement `scene.component.mesh_renderer.material`
- placement `scene.component.sprite_renderer.sprite`
- placement `scene.component.sprite_renderer.material`

Missing and wrong-kind cases should produce diagnostics instead of throwing.

- [x] **Step 2: Run the focused test and watch it fail**

Run:

```powershell
cmake --build --preset dev --target MK_runtime_scene_tests
ctest --preset dev --output-on-failure -R MK_runtime_scene_tests
```

Expected: compile or test failure because the provenance API does not exist yet.

- [x] **Step 3: Add the provenance API**

Add focused types and a function to `runtime_scene.hpp`:

```cpp
enum class RuntimeSceneAssetIdentityDiagnosticCode {
    missing_identity,
    kind_mismatch,
};

struct RuntimeSceneAssetIdentityReferenceRow {
    std::string placement;
    AssetId asset;
    AssetKeyV2 key;
    AssetKind expected_kind{AssetKind::unknown};
    AssetKind actual_kind{AssetKind::unknown};
    std::string source_path;
};

struct RuntimeSceneAssetIdentityDiagnostic {
    RuntimeSceneAssetIdentityDiagnosticCode code{RuntimeSceneAssetIdentityDiagnosticCode::missing_identity};
    std::string placement;
    AssetId asset;
    AssetKind expected_kind{AssetKind::unknown};
    AssetKind actual_kind{AssetKind::unknown};
};

struct RuntimeSceneAssetIdentityAudit {
    std::vector<RuntimeSceneAssetIdentityReferenceRow> references;
    std::vector<RuntimeSceneAssetIdentityDiagnostic> diagnostics;
};

[[nodiscard]] RuntimeSceneAssetIdentityAudit
audit_runtime_scene_asset_identity(const Scene& scene, const AssetIdentityDocumentV2& identities);
```

- [x] **Step 4: Implement minimal audit logic**

In `runtime_scene.cpp`, build an `AssetId -> AssetIdentityRowV2` lookup from `asset_id_from_key_v2(row.key)`, collect mesh/material/sprite refs from visible scene components, and compare expected kinds:

- mesh refs expect `AssetKind::mesh`
- material refs expect `AssetKind::material`
- sprite refs expect `AssetKind::texture`

- [x] **Step 5: Re-run focused runtime scene tests**

Run:

```powershell
cmake --build --preset dev --target MK_runtime_scene_tests
ctest --preset dev --output-on-failure -R MK_runtime_scene_tests
```

Expected: pass.

### Task 2: Game and Template Key-First References

**Files:**

- Modify: `games/sample_ui_audio_assets/main.cpp`
- Modify: `games/sample_gameplay_foundation/main.cpp`
- Modify: `games/sample_2d_playable_foundation/main.cpp`
- Modify: `games/sample_desktop_runtime_game/main.cpp`
- Modify: `games/sample_2d_desktop_runtime_package/main.cpp`
- Modify: `games/sample_generated_desktop_runtime_cooked_scene_package/main.cpp`
- Modify: `games/sample_generated_desktop_runtime_material_shader_package/main.cpp`
- Modify: `games/sample_generated_desktop_runtime_3d_package/main.cpp`
- Modify: `tools/new-game.ps1`

- [x] **Step 1: Add public identity header and helper**

Add `#include "mirakana/assets/asset_identity.hpp"` to each touched game source. In each unnamed namespace, add:

```cpp
[[nodiscard]] mirakana::AssetId asset_id_from_game_asset_key(std::string_view key) {
    return mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{.value = std::string{key}});
}
```

Keep existing `asset_registry.hpp` includes where `AssetRecord` or `AssetKind` is directly used.

- [x] **Step 2: Replace raw game source ID construction**

Replace every `mirakana::AssetId::from_name("...")` in `games/**/*.cpp` with `asset_id_from_game_asset_key("...")`. Remove `noexcept` from local helper functions that now allocate `std::string`.

- [x] **Step 3: Update generated templates**

Update `tools/new-game.ps1` C++ template text so generated desktop/runtime games include `mirakana/assets/asset_identity.hpp`, emit the same key-first helper, and do not generate `AssetId::from_name`.

- [x] **Step 4: Verify source cleanup**

Run:

```powershell
rg -n "AssetId::from_name\(" games tools/new-game.ps1
```

Expected: no production game/template matches. If historical tests or non-game engine internals match outside these paths, leave them alone.

### Task 3: Manifest, Docs, and Static Contract

**Files:**

- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generated: `engine/agent/manifest.json`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/superpowers/plans/2026-05-12-asset-identity-v2-reference-cleanup-milestone-v1.md`

- [x] **Step 1: Add static regression guards**

In `tools/check-ai-integration.ps1`, fail when any `games/**/*.cpp` or `tools/new-game.ps1` contains `AssetId::from_name(`. Require the runtime scene audit API needles in `runtime_scene.hpp`, `runtime_scene.cpp`, and `runtime_scene_tests.cpp`.

- [x] **Step 2: Close the unsupported gap row**

Remove `asset-identity-v2` from `aiOperableProductionLoop.unsupportedProductionGaps`, keep the `asset-identity-v2` authoring surface `ready`, and update notes to include `audit_runtime_scene_asset_identity`, generated/game key-first references, static regression guards, and explicit non-goals.

- [x] **Step 3: Align JSON contract checks**

Update `tools/check-json-contracts.ps1` to expect `asset-identity-v2` absent from unsupported gaps, while `runtime-resource-v2` remains present and next.

- [x] **Step 4: Update human-readable guidance**

Update the registry, master plan, roadmap, current capabilities, AI game-development guide, and generated-game validation scenarios so they say:

- `asset-identity-v2` is closed for 1.0 identity/reference cleanup
- runtime/cooked/render/UI payloads still consume `AssetId`
- renderer/RHI residency and package streaming remain `runtime-resource-v2` / renderer gaps
- generated games and committed sample games derive package references from `AssetKeyV2`

- [x] **Step 5: Compose manifest**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
```

Expected: composed manifest writes successfully.

### Task 4: Validation and Commit

**Files:**

- Modify: all files changed by Tasks 1-3.

- [x] **Step 1: Build focused targets**

Run:

```powershell
cmake --build --preset dev --target MK_runtime_scene_tests sample_ui_audio_assets sample_gameplay_foundation sample_2d_playable_foundation sample_desktop_runtime_game sample_2d_desktop_runtime_package sample_generated_desktop_runtime_cooked_scene_package sample_generated_desktop_runtime_material_shader_package sample_generated_desktop_runtime_3d_package
```

Expected: build succeeds.

- [x] **Step 2: Run focused tests and static checks**

Run:

```powershell
ctest --preset dev --output-on-failure -R "MK_runtime_scene_tests|sample_(ui_audio_assets|gameplay_foundation|2d_playable_foundation)"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
```

Expected: all pass.

- [x] **Step 3: Run touched-file tidy and ScriptAnalyzer**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime_scene/src/runtime_scene.cpp,games/sample_ui_audio_assets/main.cpp,games/sample_gameplay_foundation/main.cpp,games/sample_2d_playable_foundation/main.cpp,games/sample_desktop_runtime_game/main.cpp,games/sample_2d_desktop_runtime_package/main.cpp,games/sample_generated_desktop_runtime_cooked_scene_package/main.cpp,games/sample_generated_desktop_runtime_material_shader_package/main.cpp,games/sample_generated_desktop_runtime_3d_package/main.cpp
```

If `PSScriptAnalyzer` is installed, also run it on `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`, and `tools/new-game.ps1` with error severity.

- [x] **Step 4: Mark this plan completed and run full gates**

Fill the validation evidence table, set the status marker to completed, then run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1
```

Expected: both pass or an exact blocker is recorded before any ready claim.

- [x] **Step 5: Commit only this milestone**

Stage the files touched by this plan only, verify staged diff, and commit:

```powershell
git add engine/runtime_scene/include/mirakana/runtime_scene/runtime_scene.hpp engine/runtime_scene/src/runtime_scene.cpp tests/unit/runtime_scene_tests.cpp games/sample_ui_audio_assets/main.cpp games/sample_gameplay_foundation/main.cpp games/sample_2d_playable_foundation/main.cpp games/sample_desktop_runtime_game/main.cpp games/sample_2d_desktop_runtime_package/main.cpp games/sample_generated_desktop_runtime_cooked_scene_package/main.cpp games/sample_generated_desktop_runtime_material_shader_package/main.cpp games/sample_generated_desktop_runtime_3d_package/main.cpp tools/new-game.ps1 tools/check-ai-integration.ps1 tools/check-json-contracts.ps1 engine/agent/manifest.fragments/010-aiOperableProductionLoop.json engine/agent/manifest.json docs/superpowers/plans/README.md docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md docs/current-capabilities.md docs/roadmap.md docs/ai-game-development.md docs/specs/generated-game-validation-scenarios.md docs/superpowers/plans/2026-05-12-asset-identity-v2-reference-cleanup-milestone-v1.md
git diff --cached --check
git commit -m "Close asset identity reference cleanup gap"
```

Expected: commit succeeds and unrelated dirty worktree files remain unstaged.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `cmake --build --preset dev --target MK_runtime_scene_tests` | Passed | RED failed before the API existed; green passed after adding `audit_runtime_scene_asset_identity` and invalid-document/skinned-mesh coverage. |
| `rg -n "AssetId::from_name\(" games tools/new-game.ps1` | Passed | No committed game or generated-template raw `AssetId::from_name` matches. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Passed | Regenerated `engine/agent/manifest.json` from fragments. |
| `cmake --build --preset dev --target MK_runtime_scene_tests sample_ui_audio_assets sample_gameplay_foundation sample_2d_playable_foundation sample_desktop_runtime_game sample_2d_desktop_runtime_package sample_generated_desktop_runtime_cooked_scene_package sample_generated_desktop_runtime_material_shader_package sample_generated_desktop_runtime_3d_package` | Passed | Focused runtime scene and affected game targets build. |
| `ctest --preset dev --output-on-failure -R "MK_runtime_scene_tests|sample_(ui_audio_assets|gameplay_foundation|2d_playable_foundation)"` | Passed | 4/4 tests passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | Manifest gap row, recommended next plan, and command-surface contracts agree. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | Agent integration needles, generated template dry-run scaffolds, and static raw `AssetId::from_name` guard passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | Public header/API boundary checks passed for the new runtime scene audit API. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Repository format check passed after targeted clang-format on touched C++ files. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime_scene/src/runtime_scene.cpp,games/sample_ui_audio_assets/main.cpp,games/sample_gameplay_foundation/main.cpp,games/sample_2d_playable_foundation/main.cpp,games/sample_desktop_runtime_game/main.cpp,games/sample_2d_desktop_runtime_package/main.cpp,games/sample_generated_desktop_runtime_cooked_scene_package/main.cpp,games/sample_generated_desktop_runtime_material_shader_package/main.cpp,games/sample_generated_desktop_runtime_3d_package/main.cpp` | Passed | `tidy-check: ok (9 files)`; existing warning output did not fail the targeted gate. |
| `Invoke-ScriptAnalyzer -Severity Error` on `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`, `tools/new-game.ps1` | Passed | Error severity returned no diagnostics. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full validation passed; `production-readiness-audit` reports `unsupported_gaps=10`; CTest passed 47/47. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Explicit commit-prep build gate passed after validation. |
