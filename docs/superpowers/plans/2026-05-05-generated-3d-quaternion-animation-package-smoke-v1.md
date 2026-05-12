# Generated 3D Quaternion Animation Package Smoke v1 (2026-05-05)

**Plan ID:** `generated-3d-quaternion-animation-package-smoke-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Prove that generated `DesktopRuntime3DPackage` games can ship and consume a first-party cooked quaternion animation clip
package record through runtime/gameplay code, without runtime glTF parsing or renderer/RHI animation execution.

## Context

- `cooked-animation-quaternion-clip-v1` added `GameEngine.AnimationQuaternionClipSource.v1`,
  `GameEngine.CookedAnimationQuaternionClip.v1`, `AssetKind::animation_quaternion_clip`,
  `mirakana::runtime::runtime_animation_quaternion_clip_payload`, and
  `mirakana::make_animation_joint_tracks_3d_from_f32_bytes`.
- `animation-quaternion-clip-sampling-application-v1` added `AnimationPose3d` sampling for selected 3D joint tracks.
- Before this slice, generated `DesktopRuntime3DPackage` games already proved scalar transform animation and morph package
  consumption through package-visible smoke counters, but not cooked quaternion clip package consumption.

## Constraints

- Keep runtime/gameplay code on first-party cooked packages plus `mirakana_runtime`/`mirakana_animation`; do not parse glTF or source
  animation documents at runtime.
- Keep this slice smoke-level: sampled quaternion pose/counter evidence is enough. Do not add animation graphs,
  retargeting, skeletal renderer deformation, scene-node quaternion playback, blend trees over quaternion tracks, or RHI
  upload/execution.
- Keep package artifacts deterministic text/hex files owned by the generated package scaffold or sample game manifest.
- Do not broaden existing scalar transform animation smoke semantics or reuse `animation_float_clip` for quaternion TRS
  data.
- Add RED tests before production template/sample changes.

## Done When

- A generated `DesktopRuntime3DPackage` path declares and ships a cooked `animation_quaternion_clip` package record.
- Runtime package smoke loads that record with `runtime_animation_quaternion_clip_payload`, converts it to
  `AnimationJointTrack3dDesc` rows, samples an `AnimationPose3d`, and reports deterministic quaternion animation counters.
- Static scaffold tests prove new generated 3D package manifests/package files stay aligned with `runtimePackageFiles`.
- Docs, manifest, registry, and AI guidance describe generated-package quaternion animation consumption as a narrow smoke
  path while keeping graph authoring, retargeting, renderer/RHI execution, 3D local rotation limits/twist controls, and
  broad skeletal animation production readiness as follow-up work.
- Focused tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, JSON contract checks, validate-scoped tidy, and
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or a concrete host/tool blocker is recorded.

## Tasks

- [x] Add RED tests for generated 3D package quaternion animation package files, manifest rows, and smoke counters.
- [x] Add deterministic sample/template package artifacts for a tiny one-joint quaternion clip.
- [x] Add runtime smoke code that loads, decodes, samples, and reports the cooked quaternion clip.
- [x] Update generated 3D scaffold metadata, docs, manifest, skills, and AI integration checks.
- [x] Run focused tests and full validation, then record evidence.

## Validation Evidence

- RED `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed before implementation because
  `games/sample_desktop_runtime_game/game.agent.json` did not list
  `runtime/assets/desktop_runtime/packaged_pose.animation_quaternion_clip`.
- GREEN `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after adding the committed sample package record, generated scaffold package rows,
  smoke arguments, docs, manifest, and static checks.
- GREEN `cmd.exe /d /c 'set Path=%Path%&& set PATH=&& cmake --build --preset desktop-runtime --target sample_desktop_runtime_game'`
  built `out\build\desktop-runtime\games\Debug\sample_desktop_runtime_game\sample_desktop_runtime_game.exe`
  after the first long dependency build attempt timed out and was rerun with a longer timeout.
- GREEN source-tree package smoke:
  `out\build\desktop-runtime\games\Debug\sample_desktop_runtime_game\sample_desktop_runtime_game.exe --smoke --max-frames 2 --require-config runtime/sample_desktop_runtime_game.config --require-scene-package runtime/sample_desktop_runtime_game.geindex --require-quaternion-animation`
  emitted `quaternion_animation=1`, `quaternion_animation_ticks=2`, `quaternion_animation_tracks=2`,
  `quaternion_animation_failures=0`, `quaternion_animation_final_z=1`, and `quaternion_animation_final_w=0`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` initially reported formatting drift in `games/sample_desktop_runtime_game/main.cpp`;
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` fixed it, and the follow-up `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed.
- GREEN `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed.
- GREEN `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed.
- GREEN `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1` passed with the CMake File API
  compile database fallback.
- GREEN `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` passed, including 16/16 CTest desktop-runtime lane tests.
- GREEN `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed, including `ai-integration-check: ok`, `public-api-boundary-check: ok`,
  validate-scoped tidy, build, and 29/29 dev CTest tests.
