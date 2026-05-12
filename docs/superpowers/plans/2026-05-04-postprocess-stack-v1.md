# Postprocess stack v1 (2026-05-04)

## Goal

Promote the swapchain postprocess path from a **single** fullscreen resolve (`scene_color` [+ optional `scene_depth`] → swapchain) to a **small, explicit multi-stage chain** with first-party Frame Graph v1 declarations, transient intermediate color targets, and **no compatibility shims** for the old `RhiPostprocessFrameRendererDesc::postprocess_fragment_shader` scalar field (callers migrate to `postprocess_fragment_stages`).

## Scope (this slice)

- `RhiPostprocessFrameRendererDesc`: replace scalar fragment shader with `std::vector<rhi::ShaderHandle> postprocess_fragment_stages` (`1..2` entries enforced in the renderer constructor).
- `RhiPostprocessFrameRenderer`: when two stages are configured, allocate one `bgra8`-compatible intermediate RT, add a third Frame Graph v1 pass (`scene_color` → `post_work` → `swapchain`), distinct descriptor layouts (`scene+optional depth` vs `chain source only`), and record the extra fullscreen draw before the swapchain composite (native UI overlay remains on the **final** swapchain pass only).
- `RhiDirectionalShadowSmokeFrameRendererDesc`: same vector field with **`size() == 1` enforced** (shadow smoke keeps one resolve; API stays aligned with the postprocess renderer).
- SDL desktop presentation and unit tests migrate initializers; public `SdlDesktopPresentation*SceneRenderer` bytecode structs keep a **single** `postprocess_fragment_shader` field — the host wraps it as a one-element vector at the RHI boundary.

## Non-goals (v1)

- Three or more postprocess stages, HDR formats, explicit tone-map policies, or graph-driven effect lists (extend in a later slice once the two-stage path is stable).
- Changing packaged game shader assets beyond the bridge vector (games still ship one postprocess fragment until a follow-up adds a second cooked stage).

## Verification

| Check | Command / artifact |
| --- | --- |
| Unit tests | `mirakana_renderer_tests` (`renderer_rhi_tests.cpp`) postprocess + shadow smoke cases |
| Repo gates | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` |

## Status

**Completed (2026-05-04).** Phases A–C below are implemented; `RhiPostprocessFrameRendererDesc` uses `postprocess_fragment_stages` only (1–2 entries); two-stage path allocates `post_work`, records `postprocess_chain_0` / `postprocess_chain_1`, and increments `RendererStats::postprocess_passes_executed` by the stage count. `RhiDirectionalShadowSmokeFrameRendererDesc` enforces exactly one fragment stage. SDL desktop presentation wraps the public bytecode field as a one-element vector at the RHI boundary.

- [x] **Phase A (API + 1-stage parity)**: `postprocess_fragment_stages` with exactly one entry preserves prior behavior and Frame Graph pass count (`2` without depth-only changes).
- [x] **Phase B (2-stage chain)**: intermediate RT + third FG pass + second descriptor layout + `postprocess_passes_executed` accounting; null RHI test `rhi postprocess frame renderer two-stage chain uses three frame graph passes and two postprocess draws`.
- [x] **Phase C (docs + manifest)**: this plan, `plans/README.md`, `engine/agent/manifest.json` `recommendedNextPlan` updated after the cascaded shadow slice; **`currentActivePlan` は完了後に次スライス計画**（`2026-05-04-source-image-decode-and-atlas-packing-v1.md`）へ移す（`production-gap-selection-v1` は文書同期専用のため `currentActivePlan` にしない）。

### Validation evidence

| Command | Result | Notes |
| --- | --- | --- |
| `cmake --build --preset dev --target mirakana_renderer_tests` | PASS | MSVC `dev` preset |
| `ctest -C Debug -R mirakana_renderer_tests` | PASS | Includes two-stage postprocess regression |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Run after this slice (host: Windows 2026-05-04) |
