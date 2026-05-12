# Cooked Animation Float Clip v1 (2026-05-05)

**Plan ID:** `cooked-animation-float-clip-v1`  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md) Phase 6 (`root-motion-ik-and-morph-foundation-v1` umbrella)  
**Status:** Completed on 2026-05-05.

## Goal

Introduce a **first-party cooked scalar float animation clip** asset: `GameEngine.AnimationFloatClipSource.v1` source documents, `GameEngine.CookedAnimationFloatClip.v1` cook output, `AssetKind::animation_float_clip`, import/cook/package/runtime payload access (`runtime_animation_float_clip_payload`), and registry/planning/metadata wiring **without** glTF channel import in this slice (first-party text only).

## Context

- Aligns with existing morph/mesh cook patterns (key-value text, hex-encoded little-endian `float32` blobs).
- Runtime stays independent of `mirakana_animation`; gameplay can map tracks to `FloatAnimationTrack` in a later slice.

## Constraints

- **Breaking changes allowed** for new enum values and formats (first introduction).
- **No backward compatibility** with prior releases for this asset kind (none existed).
- Tracks: **non-empty** `target` token (no CR/LF/NUL); times and values **same keyframe count**; all samples **finite**; times **strictly increasing** when keyframe count ≥ 2.

## Done When

- [x] `AnimationFloatClipSourceDocument` + serialize/deserialize/validation + `write_animation_float_clip_document_payload` in `mirakana_assets`.
- [x] `AssetKind::animation_float_clip`, `AssetImportActionKind::animation_float_clip`, metadata registry, import plan, cook tool path, source registry projection, identity/package kind strings.
- [x] `RuntimeAnimationFloatClipPayload` + `runtime_animation_float_clip_payload` + runtime diagnostics branch.
- [x] `mirakana_core_tests` + `mirakana_tools_tests` cover round-trip and cook→package→runtime.
- [x] `check-json-contracts`, `check-ai-integration`, `engine/agent/manifest.json` purposes, plans README sync.

## Validation Evidence

| Command | Result | Evidence |
| --- | --- | --- |
| `cmake --build out/build/dev --parallel 4 --target mirakana_core_tests mirakana_tools_tests` | PASS | MSVC dev preset; targets link after `animation_float_clip` wiring. |
| `ctest -C Debug` (preset `dev`, 29 tests) | PASS | `mirakana_core_tests`, `mirakana_tools_tests`, `mirakana_runtime_tests`, etc.; reported by `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok` after `ai-integration-check`, `json-contract-check`, full CTest sweep. |

## Non-Goals

- glTF translation/rotation/scale channel import into this clip format.
- GPU animation curves, sampler reduction, or `mirakana_animation` runtime coupling in `mirakana_runtime`.
