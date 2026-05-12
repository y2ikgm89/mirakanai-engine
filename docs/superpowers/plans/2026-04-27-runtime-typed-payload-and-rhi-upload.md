# Runtime Typed Payload And RHI Upload Implementation Plan (2026-04-27)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add typed runtime access for cooked texture, mesh, audio, material, and scene payloads, then add a renderer/RHI-facing bridge for texture resource upload planning and material descriptor binding.

**Architecture:** Keep `MK_runtime` as the host-independent cooked-content reader that depends only on assets and platform IO. Add a small `MK_runtime_rhi` bridge that depends on `MK_runtime` and `MK_rhi` so RHI resource creation and material descriptor binding do not pull RHI concepts into `engine/runtime`.

**Tech Stack:** C++23, existing first-party cooked text formats, `MK_runtime`, `MK_rhi`, CMake target-based wiring, CTest, PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

---

## Goal

Games should use public typed payload APIs instead of parsing cooked artifact text. The first RHI bridge should convert those typed payloads into backend-neutral `mirakana::rhi` resource descriptors and descriptor writes, using `NullRhiDevice` tests before backend-specific promotion.

## Context

- `MK_runtime` already loads package indexes, validates hashes/dependencies, exposes `RuntimeAssetRecord`, and stages package replacement all-or-nothing.
- `MK_tools` currently cooks deterministic text payloads for texture, mesh, material, and audio. Scene payloads already use `GameEngine.Scene.v1` serialization in `MK_scene`.
- `MK_rhi` already provides `IRhiDevice`, `NullRhiDevice`, texture creation, upload/copy command validation, descriptor set layouts, descriptor sets, and descriptor writes.

## Constraints

- Do not add third-party dependencies.
- Do not make `engine/runtime` depend on renderer, RHI, audio device, editor, or backend modules.
- Keep public APIs free of native OS/GPU handles.
- Keep scene runtime payload access as payload metadata in `MK_runtime`; full `mirakana::Scene` deserialization remains owned by `MK_scene`.
- Existing cooked metadata-only texture payloads remain loadable. RHI upload helpers must report whether a byte upload was recorded instead of pretending unavailable bytes were uploaded.

## Done When

- Typed runtime payload access covers texture, mesh, audio, material, and scene payloads.
- Payload API rejects kind mismatches and malformed cooked payload text with deterministic diagnostics.
- `MK_runtime_rhi` can create an RHI texture resource from a typed texture payload and records no copy when cooked bytes are absent.
- `MK_runtime_rhi` can create material descriptor set layout, descriptor set, uniform buffer, and sampled texture descriptor writes from material binding metadata.
- Docs, roadmap, manifest, CMake install/export, and public API boundary roots are updated.
- Focused tests pass, CTest passes, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` has been run.

---

## File Structure

- Modify `engine/runtime/include/mirakana/runtime/asset_runtime.hpp`
  - Add typed payload structs and payload lookup functions.
- Modify `engine/runtime/src/asset_runtime.cpp`
  - Add strict cooked key/value parsing and typed payload deserialization.
- Create `engine/runtime_rhi/include/mirakana/runtime_rhi/runtime_upload.hpp`
  - Public bridge API from typed runtime payloads/material metadata to backend-neutral RHI resources.
- Create `engine/runtime_rhi/src/runtime_upload.cpp`
  - Implement texture resource creation/upload command planning and material descriptor binding.
- Create `engine/runtime_rhi/CMakeLists.txt`
  - Define `MK_runtime_rhi`, link `MK_runtime` and `MK_rhi`, apply shared target options.
- Modify `tests/unit/runtime_tests.cpp`
  - Add typed runtime payload tests.
- Create `tests/unit/runtime_rhi_tests.cpp`
  - Add RHI upload and material descriptor binding tests against `NullRhiDevice`.
- Modify `CMakeLists.txt`
  - Add `engine/runtime_rhi`, test target, install/export target, and SDK include install.
- Modify `tools/check-public-api-boundaries.ps1`
  - Add `engine/runtime_rhi/include`.
- Modify `docs/architecture.md`, `docs/roadmap.md`, and `engine/agent/manifest.json`
  - Record the new runtime payload and RHI bridge contract.

---

## Task 1: Typed Runtime Payload API

**Files:**
- Modify: `engine/runtime/include/mirakana/runtime/asset_runtime.hpp`
- Modify: `engine/runtime/src/asset_runtime.cpp`
- Modify: `tests/unit/runtime_tests.cpp`

- [x] **Step 1: Write failing tests**

Add tests that create a package with cooked texture, mesh, audio, material, and scene records, then assert:

```cpp
const auto* texture_record = result.package.find(texture);
const auto texture_payload = mirakana::runtime::runtime_texture_payload(*texture_record);
MK_REQUIRE(texture_payload.succeeded());
MK_REQUIRE(texture_payload.payload.width == 4);
MK_REQUIRE(texture_payload.payload.height == 2);
MK_REQUIRE(texture_payload.payload.pixel_format == mirakana::TextureSourcePixelFormat::rgba8_unorm);
MK_REQUIRE(texture_payload.payload.source_bytes == 32);

const auto* mesh_record = result.package.find(mesh);
const auto mesh_payload = mirakana::runtime::runtime_mesh_payload(*mesh_record);
MK_REQUIRE(mesh_payload.succeeded());
MK_REQUIRE(mesh_payload.payload.vertex_count == 3);
MK_REQUIRE(mesh_payload.payload.index_count == 3);

const auto* audio_record = result.package.find(audio);
const auto audio_payload = mirakana::runtime::runtime_audio_payload(*audio_record);
MK_REQUIRE(audio_payload.succeeded());
MK_REQUIRE(audio_payload.payload.sample_rate == 48000);
MK_REQUIRE(audio_payload.payload.channel_count == 2);

const auto* material_record = result.package.find(material);
const auto material_payload = mirakana::runtime::runtime_material_payload(*material_record);
MK_REQUIRE(material_payload.succeeded());
MK_REQUIRE(material_payload.payload.material.id == material);
MK_REQUIRE(material_payload.payload.binding_metadata.bindings.size() == 2);

const auto* scene_record = result.package.find(scene);
const auto scene_payload = mirakana::runtime::runtime_scene_payload(*scene_record);
MK_REQUIRE(scene_payload.succeeded());
MK_REQUIRE(scene_payload.payload.name == "Level01");
MK_REQUIRE(scene_payload.payload.node_count == 1);
```

Add separate tests for a texture-kind accessor called on a mesh record and a malformed texture payload.

- [x] **Step 2: Run RED**

Run: `cmake --build --preset dev --target MK_runtime_tests`

Expected: build fails because the typed payload API does not exist. In this environment, the first attempt exposed the same MSBuild sandbox `Path`/`PATH` issue before compilation; the test source still referenced only missing API at that point.

- [x] **Step 3: Implement payload structs and parsers**

Add public structs for `RuntimeTexturePayload`, `RuntimeMeshPayload`, `RuntimeAudioPayload`, `RuntimeMaterialPayload`, `RuntimeScenePayload`, and `RuntimePayloadAccessResult<T>`. Implement strict key/value parsing in `asset_runtime.cpp`, reuse existing asset/material parsers where owned by `MK_assets`, and return deterministic diagnostics instead of throwing from normal accessor failures.

- [x] **Step 4: Run GREEN**

Run: `cmake --build --preset dev --target MK_runtime_tests`

Run: `ctest --test-dir out/build/dev -C Debug -R MK_runtime_tests --output-on-failure`

Expected: runtime tests pass.

## Task 2: Runtime RHI Bridge

**Files:**
- Create: `engine/runtime_rhi/include/mirakana/runtime_rhi/runtime_upload.hpp`
- Create: `engine/runtime_rhi/src/runtime_upload.cpp`
- Create: `engine/runtime_rhi/CMakeLists.txt`
- Create: `tests/unit/runtime_rhi_tests.cpp`
- Modify: `CMakeLists.txt`
- Modify: `tools/check-public-api-boundaries.ps1`

- [x] **Step 1: Write failing tests**

Add tests that assert `upload_runtime_texture` creates a shader-resource/copy-destination texture from `RuntimeTexturePayload`, records no copy for metadata-only payloads, and rejects unsupported texture formats. Add a material binding test that creates a material with a base-color texture binding, uploads a texture, calls `create_runtime_material_gpu_binding`, and verifies `NullRhiDevice` stats for descriptor set layout creation, descriptor allocation, descriptor writes, and uniform buffer creation.

- [x] **Step 2: Run RED**

Run: `cmake --build --preset dev --target MK_runtime_rhi_tests`

Expected: configure/build fails because `MK_runtime_rhi_tests` and the bridge header do not exist.

- [x] **Step 3: Implement bridge target and API**

Add `RuntimeTextureUploadResult`, `upload_runtime_texture`, `RuntimeMaterialTextureResource`, `RuntimeMaterialGpuBinding`, and `create_runtime_material_gpu_binding`. Keep all types backend-neutral and expose only `mirakana::rhi` handles.

- [x] **Step 4: Run GREEN**

Run: `cmake --build --preset dev --target MK_runtime_rhi_tests`

Run: `ctest --test-dir out/build/dev -C Debug -R MK_runtime_rhi_tests --output-on-failure`

Expected: bridge tests pass.

## Task 3: Docs, Manifest, And Full Validation

**Files:**
- Modify: `docs/architecture.md`
- Modify: `docs/roadmap.md`
- Modify: `engine/agent/manifest.json`

- [x] **Step 1: Update docs and manifest**

Record `MK_runtime` typed payload access and `MK_runtime_rhi` as the backend-neutral bridge for RHI texture resources and material descriptor binding. Keep richer cooked pixel/mesh/audio byte payload cooking listed as the next production importer step.

- [x] **Step 2: Run focused checks**

Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`

Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`

Expected: both checks pass.

- [x] **Step 3: Run full validation**

Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`

Expected: validation passes. Existing host/SDK diagnostic blockers for `spirv-val`, SPIR-V-capable DXC, Metal tools, and macOS/Xcode remain diagnostic-only unless this environment has them installed.

## Self-Review

- Spec coverage: The plan covers typed payload access, kind/malformed diagnostics, RHI texture resource upload path, and material descriptor binding.
- Boundary check: `MK_runtime` stays independent from renderer/RHI; `MK_runtime_rhi` owns the bridge.
- Testing: Runtime and runtime-RHI tests are written before implementation and verified through CTest.
- Deferred honestly: Rich binary texture/mesh/audio payload cooking and backend-specific native upload proof are not hidden; this slice creates the public typed API and backend-neutral RHI bridge needed for that follow-up.
