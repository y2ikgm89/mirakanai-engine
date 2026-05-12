# Generated Desktop Runtime Cooked Scene Package Scaffold v0 Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/new-game.ps1` generate a desktop runtime package scaffold that ships a first-party cooked scene package and proves runtime package loading plus renderer-neutral scene submission from day one.

**Architecture:** Keep generation in repository tooling and generated game files. Generated cooked-scene desktop games use `mirakana::SdlDesktopGameHost`, `mirakana::RootedFileSystem`, `mirakana::runtime::load_runtime_asset_package`, `mirakana::instantiate_runtime_scene_render_data`, and `mirakana::submit_scene_render_packet` through public `mirakana::` APIs, with deterministic `NullRenderer` fallback and no shader artifact, native handle, RHI device, editor, SDL3, Dear ImGui, or backend interop exposure to gameplay.

**Tech Stack:** Existing PowerShell generator/check scripts, CMake desktop runtime registration, first-party cooked package text formats, C++23 sample game code, desktop runtime/package validation, no new third-party dependencies.

---

## Context

Generated Desktop Runtime Package Scaffold v0 made `tools/new-game.ps1 -Template DesktopRuntimePackage` create a config-only package-ready SDL3 desktop runtime game. The remaining generated-game production gap is that cooked scene/material package use still requires manual copying from `sample_desktop_runtime_game`. A new scaffold mode should create a small cooked texture/mesh/material/scene package, declare every package file in `game.agent.json.runtimePackageFiles`, and validate `--require-scene-package` without claiming D3D12/Vulkan/Metal scene GPU binding or shader artifacts.

## Constraints

- No new dependencies.
- Do not change public gameplay APIs.
- Do not expose SDL3, OS, native window, RHI, GPU, Dear ImGui, editor, or backend handles to game public APIs.
- Keep default `new-game` behavior headless and keep `DesktopRuntimePackage` as config-only.
- Cooked package files must be game-relative manifest entries consumed through `PACKAGE_FILES_FROM_MANIFEST`.
- v0 does not generate shader artifacts, require D3D12/Vulkan/Metal GPU paths, or claim visible GPU rendering.
- v0 does not invoke source importers or external asset decoders; it emits deterministic first-party cooked text fixtures.
- Public headers are not expected to change; if they do, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.

## Done When

- `tools/new-game.ps1` supports `-Template DesktopRuntimeCookedScenePackage` while preserving `Headless` and `DesktopRuntimePackage`.
- The cooked-scene desktop scaffold generates `main.cpp`, `README.md`, `game.agent.json`, `runtime/<game>.config`, `runtime/<game>.geindex`, and first-party cooked texture/mesh/material/scene payload files.
- Generated manifests declare honest `desktop-game-runtime` and `desktop-runtime-release` recipes, `runtimePackageFiles` for every runtime file, and no shader/GPU readiness claims.
- Generated CMake registration uses `mirakana_add_desktop_runtime_game(... PACKAGE_FILES_FROM_MANIFEST)` plus the required runtime/scene/renderer-neutral libraries.
- Static AI integration checks exercise headless, config-only desktop package, and cooked-scene desktop package scaffold modes in disposable roots.
- A committed generated proof sample validates through `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` and selected package validation.
- Docs, testing docs, plan registry, roadmap/gap docs, manifest, Codex/Claude skills, and relevant subagent guidance are synchronized.
- Focused validation and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` have been run.

## Tasks

### Task 1: Register The Slice

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Create: this plan
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`

- [x] **Step 1: Create this dated focused plan.**
- [x] **Step 2: Move Generated Desktop Runtime Package Scaffold v0 to completed and mark this plan as active.**
- [x] **Step 3: Update gap analysis so generated cooked-scene scaffolding is the active Windows-host-feasible slice.**

### Task 2: Write The Failing Scaffold Contract

**Files:**
- Modify: `tools/check-ai-integration.ps1`
- Exercise: `tools/new-game.ps1`

- [x] **Step 1: Add a disposable scaffold check for `-Template DesktopRuntimeCookedScenePackage`.**

Expected generated files:

```text
games/desktop-cooked-scene-game/main.cpp
games/desktop-cooked-scene-game/README.md
games/desktop-cooked-scene-game/game.agent.json
games/desktop-cooked-scene-game/runtime/desktop-cooked-scene-game.config
games/desktop-cooked-scene-game/runtime/desktop-cooked-scene-game.geindex
games/desktop-cooked-scene-game/runtime/assets/generated/base-color.texture.geasset
games/desktop-cooked-scene-game/runtime/assets/generated/triangle.mesh
games/desktop-cooked-scene-game/runtime/assets/generated/lit.material
games/desktop-cooked-scene-game/runtime/assets/generated/packaged-scene.scene
```

- [x] **Step 2: Assert the generated manifest and CMake registration stay manifest-driven.**

Expected checks:

```text
runtimePackageFiles contains the config, geindex, texture, mesh, material, and scene files
validationRecipes contains --require-config and --require-scene-package
mirakana_add_desktop_runtime_game uses GAME_MANIFEST and PACKAGE_FILES_FROM_MANIFEST
target_link_libraries includes mirakana_runtime, mirakana_scene, and mirakana_scene_renderer
```

- [x] **Step 3: Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` and confirm RED because the template selector does not exist yet.**

### Task 3: Implement Cooked Scene Scaffold Generation

**Files:**
- Modify: `tools/new-game.ps1`

- [x] **Step 1: Add the `DesktopRuntimeCookedScenePackage` template value.**
- [x] **Step 2: Add deterministic FNV-1a helper functions for generated asset ids and cooked content hashes.**
- [x] **Step 3: Generate cooked texture, mesh, material, scene, and package-index text fixtures with matching ids, dependencies, and content hashes.**
- [x] **Step 4: Generate desktop runtime C++ that loads the package, instantiates render data, submits the scene through `IRenderer`, and validates scene counters in smoke mode.**
- [x] **Step 5: Generate manifest, README, config, and CMake registration with `PACKAGE_FILES_FROM_MANIFEST` and no shader/GPU claims.**

### Task 4: Add A Committed Proof Sample

**Files:**
- Create: `games/sample-generated-desktop-runtime-cooked-scene-package/**`
- Modify: `games/CMakeLists.txt`
- Modify: `tools/validate-desktop-game-runtime.ps1`

- [x] **Step 1: Generate `sample-generated-desktop-runtime-cooked-scene-package` from the new template.**
- [x] **Step 2: Add it to the required desktop runtime validation targets.**
- [x] **Step 3: Keep the sample's package smoke at `--require-config` plus `--require-scene-package` only.**

### Task 5: Sync Docs And AI Guidance

**Files:**
- Modify: `docs/ai-game-development.md`
- Modify: `docs/testing.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/specs/game-prompt-pack.md`
- Modify: `docs/specs/game-template.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `.codex/agents/gameplay-builder.toml`
- Modify: `.claude/agents/gameplay-builder.md`

- [x] **Step 1: Document the new cooked-scene scaffold mode honestly.**
- [x] **Step 2: Fix known drift: testing docs must mention the generated config-only sample, and the Vulkan scene package validation recipe must include `--require-postprocess` where the sample manifest already requires it.**
- [x] **Step 3: Keep non-goals explicit: shader generation, scene GPU binding, visible GPU proof, editor GUI authoring, and mobile lanes remain separate.**

### Task 6: Validate

**Files:**
- Modify: this plan with validation evidence.
- Modify: `docs/superpowers/plans/README.md` after completion.

- [x] **Step 1: Run focused validation.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_cooked_scene_package
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1
```

- [x] **Step 2: Run required final validation.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [x] **Step 3: Run `cpp-reviewer` and fix actionable findings.**

## Validation Evidence

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS. Includes disposable `Headless`, `DesktopRuntimePackage`, and `DesktopRuntimeCookedScenePackage` scaffold checks.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`: PASS. 11/11 tests passed, including `sample_generated_desktop_runtime_cooked_scene_package_smoke`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_cooked_scene_package`: PASS with the known sandbox vcpkg 7zip workaround. Installed smoke reported `scene_meshes=2`, `scene_materials=2`, and `scene_gpu_status=not_requested`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1`: PASS with the known sandbox vcpkg 7zip workaround.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS. Existing diagnostic-only/host-gated results remain: Metal `metal`/`metallib` missing, Apple packaging blocked by missing macOS/Xcode tools, Android release signing not configured, Android device smoke not connected, and strict clang-tidy diagnostic-only because the Visual Studio dev preset does not emit the expected compile database before configure/build completes.
- `cpp-reviewer`: findings fixed. The generator now normalizes hash-sensitive cooked fixture text to LF, emits `runtime/.gitattributes` LF rules, and writes dependency edge kinds/paths that match first-party import/package assembly conventions. `agent-check` now proves those properties for generated cooked-scene scaffolds.
- Final manifest sync: `commands.newGame`, `gameCodeGuidance.desktopRuntimeCookedScenePackageScaffoldCommand`, `desktop-game-runtime` notes, and `nextRuntimeTargets` now mention the cooked-scene scaffold and the selected follow-up `Editor Scene/Prefab GUI Wiring + Package Candidate Workflow v0`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` were rerun after that sync and passed.
