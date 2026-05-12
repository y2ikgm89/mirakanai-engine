# Generated 3D Native UI Text Glyph Atlas Package Smoke Implementation Plan (2026-05-09)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a selected generated `DesktopRuntime3DPackage` package smoke that proves a generated 3D game can submit one deterministic text glyph from cooked `GameEngine.UiAtlas.v1` glyph metadata through the host-owned native UI texture overlay path on the D3D12 desktop package lane.

**Architecture:** Build directly on the completed generated 3D native UI overlay and textured image atlas package smokes, but keep this proof as a separate selected flag so image rows and glyph rows can be validated independently. The generated 3D sample and `tools/new-game.ps1 -Template DesktopRuntime3DPackage` will ship a tiny cooked glyph atlas metadata document that references the existing cooked base-color texture page, load it from the runtime package with `mirakana::build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas`, pass `mirakana::UiRendererGlyphAtlasPalette` plus `mirakana::ui::MonospaceTextLayoutPolicy` into `mirakana::submit_ui_renderer_submission`, enable the host-owned native UI texture overlay, emit `hud_text_glyphs`, `text_glyph_sprites_submitted`, `ui_atlas_metadata_glyphs`, and `ui_texture_overlay_*` counters, and fail closed when selected fields are absent or not frame-exact. This is a narrow D3D12-selected generated 3D cooked glyph metadata proof; it must not claim production text shaping, real font loading/rasterization, glyph atlas generation from font files, runtime source image decoding, renderer texture upload as a public gameplay API, public native/RHI handles, Metal readiness, or broad generated 3D production readiness.

**Tech Stack:** C++23, first-party cooked `GameEngine.UiAtlas.v1` metadata, `mirakana_runtime`, `mirakana_ui`, `mirakana_ui_renderer`, `mirakana_runtime_host_sdl3_presentation`, D3D12 DXIL native UI overlay shaders, CMake desktop-runtime target metadata, PowerShell validators/static checks.

---

**Plan ID:** `generated-3d-native-ui-text-glyph-atlas-package-smoke-v1`  
**Status:** Completed.  
**Completed:** 2026-05-09.  
**Master Plan:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Previous Slice:** [2026-05-09-generated-3d-native-ui-textured-sprite-atlas-package-smoke-v1.md](2026-05-09-generated-3d-native-ui-textured-sprite-atlas-package-smoke-v1.md)

## Context

- The completed generated 3D native UI overlay slice proves only a colored `mirakana_ui` HUD box through `--require-native-ui-overlay`.
- The completed generated 3D textured sprite atlas slice proves one cooked image row through `--require-native-ui-textured-sprite-atlas`, `mirakana::UiRendererImagePalette`, `hud_images`, `ui_atlas_metadata_*`, and `ui_texture_overlay_*`.
- `mirakana_ui_renderer` already supports `UiRendererGlyphAtlasPalette`, `build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas`, `UiRenderSubmitDesc::glyph_atlas`, `UiRenderSubmitDesc::text_layout_policy`, and `UiRenderSubmitResult::text_glyph_sprites_submitted`.
- The generated 3D sample already ships `runtime/assets/3d/base_color.texture.geasset`. This slice should reuse that cooked texture as a synthetic glyph atlas page instead of adding real font loading, source glyph rasterization, source image decoding, or atlas packing.
- The master plan still keeps production runtime UI text/image readiness, platform text/IME/accessibility bridges, and broad renderer/backend parity as explicit follow-up work. This child slice narrows only generated 3D package-visible glyph metadata consumption.

## Constraints

- Keep generated gameplay code on public `mirakana::ui`, `mirakana::UiRendererGlyphAtlasPalette`, `mirakana::IRenderer`, and package/runtime APIs only.
- Keep SDL3, D3D12, Vulkan, native window handles, RHI handles, texture upload details, and overlay implementation inside runtime-host/presentation adapters.
- Do not add third-party dependencies, source font loading, source glyph rasterization, source image decoding, or runtime atlas packing in generated runtime gameplay.
- Keep the new flag separate from the completed image atlas proof. `--require-native-ui-text-glyph-atlas` may imply `--require-native-ui-overlay`, scene GPU, postprocess, renderer quality, playable 3D, and D3D12 scene shader requirements, but validators must distinguish text glyph fields from image fields.
- Keep the selected proof D3D12-only unless a future child plan accepts Vulkan or Metal overlay parity. Existing Vulkan metadata may remain unchanged, and no Vulkan/Metal ready claim is allowed in this plan.
- Keep all runtime package file paths lowercase snake_case and keep `.uiatlas` files LF-normalized through `.gitattributes`.

## Done When

- `games/sample_generated_desktop_runtime_3d_package` accepts `--require-native-ui-text-glyph-atlas`.
- The selected installed D3D12 package smoke reports:
  - `hud_boxes=<expected_frames>`
  - `hud_text_glyphs=<expected_frames>`
  - `text_glyph_sprites_submitted=<expected_frames>`
  - `text_glyphs_available=<expected_frames>`
  - `text_glyphs_resolved=<expected_frames>`
  - `text_glyphs_missing=0`
  - `ui_atlas_metadata_requested=1`
  - `ui_atlas_metadata_status=ready`
  - `ui_atlas_metadata_pages=1`
  - `ui_atlas_metadata_bindings=0`
  - `ui_atlas_metadata_glyphs=1`
  - `ui_texture_overlay_requested=1`
  - `ui_texture_overlay_status=ready`
  - `ui_texture_overlay_atlas_ready=1`
  - `ui_texture_overlay_sprites_submitted=<expected_frames>`
  - `ui_texture_overlay_texture_binds=<expected_frames>`
  - `ui_texture_overlay_draws=<expected_frames>`
- `tools/validate-installed-desktop-runtime.ps1` enforces those fields whenever smoke args include `--require-native-ui-text-glyph-atlas` for `sample_generated_desktop_runtime_3d_package`.
- `tools/new-game.ps1 -Template DesktopRuntime3DPackage` emits the same flag, cooked glyph `.uiatlas` file, package index rows, `runtimePackageFiles`, manifest recipe, README text, and static markers for generated games.
- `engine/agent/manifest.json`, docs, skills, static checks, and the plan registry describe this as a narrow D3D12-selected generated 3D cooked text-glyph atlas smoke.
- Focused sample build, installed package smoke, static checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record concrete host/tool blockers.

## File Structure

- Modify `games/sample_generated_desktop_runtime_3d_package/main.cpp`: option parsing, glyph atlas metadata loading, glyph palette ownership, fixed proof text element, text glyph counters, texture overlay report validation, and fail-closed diagnostics.
- Add `games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/hud_text.uiatlas`: cooked `GameEngine.UiAtlas.v1` glyph metadata referencing `runtime/assets/3d/base_color.texture.geasset`.
- Modify `games/sample_generated_desktop_runtime_3d_package/runtime/sample_generated_desktop_runtime_3d_package.geindex`: add one `ui_atlas` entry and one `ui_atlas_texture` dependency edge.
- Modify `games/sample_generated_desktop_runtime_3d_package/runtime/.gitattributes`: keep `*.uiatlas` line endings deterministic.
- Modify `games/sample_generated_desktop_runtime_3d_package/game.agent.json` and `README.md`: add runtime package file, recipe, command, fields, and unsupported boundaries.
- Modify `tools/new-game.ps1`: propagate cooked glyph UI atlas files, package index rows, generated C++ flag/runtime loading, manifest recipe, README, and CMake registration updates.
- Modify `tools/validate-installed-desktop-runtime.ps1`: parse `--require-native-ui-text-glyph-atlas` and validate package smoke fields.
- Modify `tools/check-ai-integration.ps1` and `tools/check-json-contracts.ps1`: lock scaffold/sample/manifest/docs markers.
- Modify `engine/agent/manifest.json`, `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/specs/generated-game-validation-scenarios.md`, `.agents/skills/gameengine-game-development/SKILL.md`, `.claude/skills/gameengine-game-development/SKILL.md`, and `docs/superpowers/plans/README.md`: synchronize the ready boundary and next pointer.

### Task 1: RED Validation And Static Markers

**Files:**
- Test: `games/sample_generated_desktop_runtime_3d_package/main.cpp`
- Test: `tools/validate-installed-desktop-runtime.ps1`
- Test: `tools/check-json-contracts.ps1`
- Test: `tools/check-ai-integration.ps1`

- [x] **Step 1: Run current generated 3D package smoke with the new flag and record failure**

Run:

```powershell
$smokeArgs = @('--smoke', '--require-config', 'runtime/sample_generated_desktop_runtime_3d_package.config', '--require-scene-package', 'runtime/sample_generated_desktop_runtime_3d_package.geindex', '--require-primary-camera-controller', '--require-transform-animation', '--require-morph-package', '--require-compute-morph', '--require-compute-morph-normal-tangent', '--require-compute-morph-skin', '--require-compute-morph-async-telemetry', '--require-quaternion-animation', '--require-package-streaming-safe-point', '--require-gameplay-systems', '--require-d3d12-scene-shaders', '--video-driver', 'windows', '--require-d3d12-renderer', '--require-scene-gpu-bindings', '--require-postprocess', '--require-postprocess-depth-input', '--require-renderer-quality-gates', '--require-playable-3d-slice', '--require-native-ui-overlay', '--require-native-ui-text-glyph-atlas')
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package -SmokeArgs $smokeArgs
```

Expected now: FAIL because `--require-native-ui-text-glyph-atlas` is unknown or because required `hud_text_glyphs`, `text_glyph_sprites_submitted`, `ui_atlas_metadata_glyphs`, and `ui_texture_overlay_*` fields are absent.

- [x] **Step 2: Add static-check expectations before implementation**

Add checks that fail until the sample and generated scaffold expose the cooked glyph atlas contract:

```powershell
Require-Text -Path 'games/sample_generated_desktop_runtime_3d_package/main.cpp' -Needle '--require-native-ui-text-glyph-atlas'
Require-Text -Path 'games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/hud_text.uiatlas' -Needle 'glyph.0.font_family=engine-default'
Require-Text -Path 'games/sample_generated_desktop_runtime_3d_package/game.agent.json' -Needle 'installed-d3d12-3d-native-ui-text-glyph-atlas-smoke'
Require-Text -Path 'tools/new-game.ps1' -Needle 'runtime/assets/3d/hud_text.uiatlas'
```

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

Expected now: FAIL on missing generated 3D text glyph atlas markers.

### Task 2: Sample Glyph Atlas Loading And HUD Text Submission

**Files:**
- Modify: `games/sample_generated_desktop_runtime_3d_package/main.cpp`
- Add: `games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/hud_text.uiatlas`
- Modify: `games/sample_generated_desktop_runtime_3d_package/runtime/sample_generated_desktop_runtime_3d_package.geindex`
- Modify: `games/sample_generated_desktop_runtime_3d_package/runtime/.gitattributes`

- [x] **Step 1: Add cooked glyph UI atlas metadata to the generated 3D package**

Add `runtime/assets/3d/hud_text.uiatlas` with one glyph row for the fixed proof text `A`. Use the existing cooked texture page:

```text
format=GameEngine.UiAtlas.v1
asset.id=<fnv1a64 sample-generated-desktop-runtime-3d-package/ui-atlas/hud-text>
asset.kind=ui_atlas
source.decoding=unsupported
atlas.packing=unsupported
page.count=1
page.0.asset=9112802916051476641
page.0.asset_uri=runtime/assets/3d/base_color.texture.geasset
image.count=0
glyph.count=1
glyph.0.font_family=engine-default
glyph.0.glyph=65
glyph.0.page=9112802916051476641
glyph.0.u0=0
glyph.0.v0=0
glyph.0.u1=1
glyph.0.v1=1
glyph.0.color=1,1,1,1
```

Compute the numeric `asset.id` with the existing `Get-Fnv1a64Decimal` helper in `tools/new-game.ps1` or an equivalent repository helper. Update the package index to add one `kind=ui_atlas` row pointing at `runtime/assets/3d/hud_text.uiatlas`, and add a `ui_atlas_texture` dependency edge from the glyph UI atlas asset to texture asset `9112802916051476641`. Preserve the existing image atlas row and keep `*.uiatlas text eol=lf` in `runtime/.gitattributes`.

- [x] **Step 2: Load glyph metadata through `mirakana_ui_renderer`**

Add a sibling runtime state to the existing image atlas metadata loader:

```cpp
struct UiGlyphAtlasMetadataRuntimeState {
    UiAtlasMetadataStatus status{UiAtlasMetadataStatus::not_requested};
    mirakana::UiRendererGlyphAtlasPalette glyph_atlas;
    mirakana::AssetId atlas_page;
    std::size_t pages{0};
    std::size_t glyphs{0};
    std::vector<std::string> diagnostics;
};

[[nodiscard]] UiGlyphAtlasMetadataRuntimeState
load_required_ui_text_glyph_atlas_metadata(const mirakana::runtime::RuntimeAssetPackage* package) {
    UiGlyphAtlasMetadataRuntimeState state;
    const auto metadata_asset = packaged_ui_text_glyph_atlas_metadata_asset_id();
    if (package == nullptr) {
        state.status = UiAtlasMetadataStatus::missing;
        state.diagnostics.push_back("ui text glyph atlas metadata requires a loaded runtime package");
        return state;
    }

    auto result = mirakana::build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas(*package, metadata_asset);
    if (!result.succeeded()) {
        state.status = UiAtlasMetadataStatus::malformed;
        for (const auto& failure : result.failures) {
            state.diagnostics.push_back(failure.diagnostic);
        }
        return state;
    }

    state.status = UiAtlasMetadataStatus::ready;
    state.glyph_atlas = std::move(result.palette);
    state.pages = result.atlas_page_assets.size();
    state.glyphs = state.glyph_atlas.count();
    if (!result.atlas_page_assets.empty()) {
        state.atlas_page = result.atlas_page_assets.front();
    }
    return state;
}
```

Reuse the existing failure classification helpers where they fit, but keep diagnostics explicitly about the text glyph atlas when the glyph metadata asset is missing or malformed.

- [x] **Step 3: Add the selected flag and host texture overlay setup**

Extend options and parser:

```cpp
bool require_native_ui_text_glyph_atlas{false};

if (arg == "--require-native-ui-text-glyph-atlas") {
    options.require_native_ui_text_glyph_atlas = true;
    options.require_native_ui_overlay = true;
    options.require_scene_gpu_bindings = true;
    options.require_postprocess = true;
    options.require_renderer_quality_gates = true;
    options.require_playable_3d_slice = true;
    options.require_d3d12_scene_shaders = true;
    options.require_d3d12_renderer = true;
    continue;
}
```

Before constructing the game, load glyph metadata when selected and pass its `atlas_page` to `SdlDesktopPresentationD3d12SceneRendererDesc::native_ui_overlay_atlas_asset`. Set `enable_native_ui_overlay_textures` when either `require_native_ui_textured_sprite_atlas` or `require_native_ui_text_glyph_atlas` is selected.

- [x] **Step 4: Submit one deterministic proof glyph per frame**

Extend the game constructor to accept `bool text_glyph_ui_atlas_mode` and `mirakana::UiRendererGlyphAtlasPalette glyph_atlas`. In `build_hud()`, add a fixed one-character label only in text glyph mode:

```cpp
if (text_glyph_ui_atlas_mode_) {
    mirakana::ui::ElementDesc glyph_text;
    glyph_text.id = mirakana::ui::ElementId{"hud.text_glyph_atlas_proof"};
    glyph_text.parent = root.id;
    glyph_text.role = mirakana::ui::SemanticRole::label;
    glyph_text.bounds = mirakana::ui::Rect{0.0F, 0.0F, 16.0F, 16.0F};
    glyph_text.text = mirakana::ui::TextContent{"A", "hud.text_glyph_atlas_proof", "engine-default"};
    glyph_text.accessibility_label = "Text glyph atlas proof";
    if (!hud_.try_add_element(glyph_text)) {
        return false;
    }
}
```

In `on_update()`, set the glyph atlas and deterministic monospace policy:

```cpp
if (text_glyph_ui_atlas_mode_) {
    ui_submit_desc.glyph_atlas = &glyph_atlas_;
    ui_submit_desc.text_layout_policy = mirakana::ui::MonospaceTextLayoutPolicy{8.0F, 4.0F, 10.0F};
}

hud_text_glyphs_available_ += hud_submit.text_glyphs_available;
hud_text_glyphs_resolved_ += hud_submit.text_glyphs_resolved;
hud_text_glyphs_missing_ += hud_submit.text_glyphs_missing;
hud_text_glyph_sprites_submitted_ += hud_submit.text_glyph_sprites_submitted;
```

Keep the existing dynamic `hud.status` text unchanged. Its glyphs remain unresolved unless this plan intentionally adds rows for every dynamic character, so `hud.status` must not be counted as part of the selected text glyph proof.

- [x] **Step 5: Fail closed on exact glyph and overlay counters**

Add getters and `hud_passed()` checks so text glyph mode requires exactly one glyph sprite per frame:

```cpp
const bool text_glyph_rows_ok = !text_glyph_ui_atlas_mode_ ||
    (hud_text_glyph_sprites_submitted_ == expected_frames &&
     hud_text_glyphs_available_ == expected_frames &&
     hud_text_glyphs_resolved_ == expected_frames &&
     hud_text_glyphs_missing_ == 0U);
```

When validating the final report, compute textured overlay counts from selected textured rows:

```cpp
const auto expected_textured_ui_sprites =
    static_cast<std::uint64_t>(options.max_frames) *
    static_cast<std::uint64_t>((options.require_native_ui_textured_sprite_atlas ? 1U : 0U) +
                               (options.require_native_ui_text_glyph_atlas ? 1U : 0U));
```

If both image and glyph atlas modes are ever selected together, require the sum. The dedicated glyph smoke should select only the glyph flag so `ui_texture_overlay_sprites_submitted`, `ui_texture_overlay_texture_binds`, and `ui_texture_overlay_draws` equal `max_frames`.

- [x] **Step 6: Run focused build**

Run:

```powershell
cmake --build --preset desktop-runtime --target sample_generated_desktop_runtime_3d_package --config Debug
```

Expected: PASS.

### Task 3: Installed Package Validator And Generated Scaffold

**Files:**
- Modify: `tools/validate-installed-desktop-runtime.ps1`
- Modify: `tools/new-game.ps1`
- Modify: `games/sample_generated_desktop_runtime_3d_package/game.agent.json`
- Modify: `games/sample_generated_desktop_runtime_3d_package/README.md`

- [x] **Step 1: Teach the installed validator the glyph flag**

In `tools/validate-installed-desktop-runtime.ps1`, detect `--require-native-ui-text-glyph-atlas` and require exact fields:

```powershell
$requiresGenerated3dNativeUiTextGlyphAtlas = $SmokeArgs -contains "--require-native-ui-text-glyph-atlas"

if ($TargetName -eq "sample_generated_desktop_runtime_3d_package" -and $requiresGenerated3dNativeUiTextGlyphAtlas) {
    Assert-FieldEquals -Fields $fields -Name "hud_text_glyphs" -Expected $expectedSmokeFrames
    Assert-FieldEquals -Fields $fields -Name "text_glyph_sprites_submitted" -Expected $expectedSmokeFrames
    Assert-FieldEquals -Fields $fields -Name "text_glyphs_available" -Expected $expectedSmokeFrames
    Assert-FieldEquals -Fields $fields -Name "text_glyphs_resolved" -Expected $expectedSmokeFrames
    Assert-FieldEquals -Fields $fields -Name "text_glyphs_missing" -Expected 0
    Assert-FieldEquals -Fields $fields -Name "ui_atlas_metadata_requested" -Expected 1
    Assert-FieldEquals -Fields $fields -Name "ui_atlas_metadata_status" -Expected "ready"
    Assert-FieldEquals -Fields $fields -Name "ui_atlas_metadata_pages" -Expected 1
    Assert-FieldEquals -Fields $fields -Name "ui_atlas_metadata_bindings" -Expected 0
    Assert-FieldEquals -Fields $fields -Name "ui_atlas_metadata_glyphs" -Expected 1
    Assert-FieldEquals -Fields $fields -Name "ui_texture_overlay_requested" -Expected 1
    Assert-FieldEquals -Fields $fields -Name "ui_texture_overlay_status" -Expected "ready"
    Assert-FieldEquals -Fields $fields -Name "ui_texture_overlay_atlas_ready" -Expected 1
    Assert-FieldEquals -Fields $fields -Name "ui_texture_overlay_sprites_submitted" -Expected $expectedSmokeFrames
    Assert-FieldEquals -Fields $fields -Name "ui_texture_overlay_texture_binds" -Expected $expectedSmokeFrames
    Assert-FieldEquals -Fields $fields -Name "ui_texture_overlay_draws" -Expected $expectedSmokeFrames
}
```

Keep the existing image atlas checks unchanged for `--require-native-ui-textured-sprite-atlas`.

- [x] **Step 2: Add a generated manifest recipe**

In `games/sample_generated_desktop_runtime_3d_package/game.agent.json`, add a selected recipe named `installed-d3d12-3d-native-ui-text-glyph-atlas-smoke` that uses the package script with `-SmokeArgs` including `--require-native-ui-text-glyph-atlas` and excluding `--require-native-ui-textured-sprite-atlas`. Include the expected output fields and unsupported boundaries in the recipe description.

Add `runtime/assets/3d/hud_text.uiatlas` to `runtimePackageFiles`.

- [x] **Step 3: Propagate the same scaffold through `tools/new-game.ps1`**

For the `DesktopRuntime3DPackage` template, emit:

```text
runtime/assets/3d/hud_text.uiatlas
--require-native-ui-text-glyph-atlas
installed-d3d12-3d-native-ui-text-glyph-atlas-smoke
hud_text_glyphs
text_glyph_sprites_submitted
ui_atlas_metadata_glyphs
```

The generated `.geindex` must include the glyph `ui_atlas` entry and `ui_atlas_texture` dependency edge. The generated C++ must include the same parser flag, metadata loader, HUD proof label, counters, and fail-closed validation as the committed sample.

- [x] **Step 4: Update README evidence**

Document the selected D3D12 command, exact expected fields, and non-claims:

```text
--require-native-ui-text-glyph-atlas validates one cooked GameEngine.UiAtlas.v1 glyph row through the host-owned native UI texture overlay. It proves hud_text_glyphs=<frames>, text_glyph_sprites_submitted=<frames>, ui_atlas_metadata_glyphs=1, and ui_texture_overlay_* readiness. It does not prove production text shaping, real font loading/rasterization, glyph atlas generation from source fonts, runtime source image decoding, Metal overlay readiness, native handles, or broad runtime UI readiness.
```

### Task 4: Manifest, Docs, Skills, And Registry Closure

**Files:**
- Modify: `engine/agent/manifest.json`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`

- [x] **Step 1: Add machine-readable production recipe evidence**

In `engine/agent/manifest.json`, add a validation recipe and production recipe for `desktopRuntime3dNativeUiTextGlyphAtlasPackageSmoke` with the selected D3D12 package command. Describe the ready surface as cooked glyph metadata consumption through `UiRendererGlyphAtlasPalette`, not production text/font readiness.

- [x] **Step 2: Update human docs and skills**

Update the current capability docs and both game-development skills to state that generated 3D D3D12 package smokes can require `--require-native-ui-text-glyph-atlas` for one fixed `engine-default` glyph from cooked package metadata. Keep the same unsupported boundaries in every location:

```text
not production text shaping, not real font loading/rasterization, not glyph atlas generation from source fonts, not runtime source image decoding, not renderer texture upload as a gameplay API, not Metal overlay readiness, not native handles, not broad runtime UI readiness
```

- [x] **Step 3: Close the active plan pointers**

When the implementation and validation evidence are complete, change this plan to:

```markdown
**Status:** Completed.
**Completed:** 2026-05-09.
```

Then set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` back to the master plan and set `recommendedNextPlan.id` to `next-production-gap-selection`. Update `docs/superpowers/plans/README.md` so this file is the latest completed slice and no child slice is active.

### Task 5: Verification And Commit Checkpoint

**Files:**
- Update: `docs/superpowers/plans/2026-05-09-generated-3d-native-ui-text-glyph-atlas-package-smoke-v1.md`

- [x] **Step 1: Run the selected glyph atlas package smoke**

Run:

```powershell
$smokeArgs = @('--smoke', '--require-config', 'runtime/sample_generated_desktop_runtime_3d_package.config', '--require-scene-package', 'runtime/sample_generated_desktop_runtime_3d_package.geindex', '--require-primary-camera-controller', '--require-transform-animation', '--require-morph-package', '--require-compute-morph', '--require-compute-morph-normal-tangent', '--require-compute-morph-skin', '--require-compute-morph-async-telemetry', '--require-quaternion-animation', '--require-package-streaming-safe-point', '--require-gameplay-systems', '--require-d3d12-scene-shaders', '--video-driver', 'windows', '--require-d3d12-renderer', '--require-scene-gpu-bindings', '--require-postprocess', '--require-postprocess-depth-input', '--require-renderer-quality-gates', '--require-playable-3d-slice', '--require-native-ui-overlay', '--require-native-ui-text-glyph-atlas')
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package -SmokeArgs $smokeArgs
```

Expected: PASS with `hud_text_glyphs=2`, `text_glyph_sprites_submitted=2`, `text_glyphs_available=2`, `text_glyphs_resolved=2`, `text_glyphs_missing=0`, `ui_atlas_metadata_glyphs=1`, and `ui_texture_overlay_draws=2` for the default two-frame smoke.

- [x] **Step 2: Run focused and repository validation**

Run:

```powershell
cmake --build --preset desktop-runtime --target sample_generated_desktop_runtime_3d_package --config Debug
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
git diff --check
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1
```

Expected: PASS, except host-gated diagnostics already allowed by the existing repository wrappers.

- [x] **Step 3: Record validation evidence**

Append or update the table below before closing the plan:

| Command | Result | Evidence |
| --- | --- | --- |
| `cmake --build --preset desktop-runtime --target sample_generated_desktop_runtime_3d_package --config Debug` | PASS | Focused sample build completed with `sample_generated_desktop_runtime_3d_package.exe` rebuilt. |
| `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package -SmokeArgs ... --require-native-ui-text-glyph-atlas` | PASS | Selected installed D3D12 glyph atlas smoke reported `hud_text_glyphs=2`, `text_glyphs_resolved=2`, `text_glyphs_missing=0`, `text_glyph_sprites_submitted=2`, `ui_atlas_metadata_glyphs=1`, and `ui_texture_overlay_draws=2`; package script ended `desktop-runtime-package: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | JSON/schema contract sync passed after closing `currentActivePlan` back to the master plan. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | AI manifest/docs/skills/static-marker sync passed and generated `DesktopRuntime3DPackage` dry-run scaffold included `hud_text.uiatlas` plus `--require-native-ui-text-glyph-atlas`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | Unsupported gap ledger remained honest with 11 tracked gaps and no ready overclaim. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Formatting passed after repository `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` normalized touched C++ files. |
| `git diff --check` | PASS | Whitespace check passed; only line-ending warnings were reported by Git. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Repository validation passed; host-gated Metal/Apple diagnostics remained informational. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Repository Debug build passed through `tools/build.ps1`. |

- [ ] **Step 4: Commit the coherent slice**

Not committed in this checkpoint because the worktree contains multiple pre-existing and parallel slice changes, including files touched by this slice. Staging whole files would risk bundling unrelated work.

After `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 && pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passes, stage only files touched by this plan and commit:

```powershell
git status --short
git add games/sample_generated_desktop_runtime_3d_package/main.cpp games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/hud_text.uiatlas games/sample_generated_desktop_runtime_3d_package/runtime/sample_generated_desktop_runtime_3d_package.geindex games/sample_generated_desktop_runtime_3d_package/runtime/.gitattributes games/sample_generated_desktop_runtime_3d_package/game.agent.json games/sample_generated_desktop_runtime_3d_package/README.md tools/new-game.ps1 tools/validate-installed-desktop-runtime.ps1 tools/check-ai-integration.ps1 tools/check-json-contracts.ps1 engine/agent/manifest.json docs/current-capabilities.md docs/roadmap.md docs/specs/generated-game-validation-scenarios.md .agents/skills/gameengine-game-development/SKILL.md .claude/skills/gameengine-game-development/SKILL.md docs/superpowers/plans/README.md docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md docs/superpowers/plans/2026-05-09-generated-3d-native-ui-text-glyph-atlas-package-smoke-v1.md
git commit -m "feat: prove generated 3d native ui text glyph atlas smoke"
```

If unrelated user changes remain in the worktree, leave them unstaged and record that explicitly in the completion report.
