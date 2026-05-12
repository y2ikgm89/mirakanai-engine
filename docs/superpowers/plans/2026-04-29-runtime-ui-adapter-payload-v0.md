# Runtime UI Adapter Payload v0 Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the first concrete, dependency-free runtime UI adapter payload models for text, image, and accessibility output so `mirakana_ui` can move beyond box-only renderer proof without adding SDL3, Dear ImGui, OS accessibility APIs, font engines, image decoders, or RHI/native handles to the `mirakana_ui` payload contracts.

**Architecture:** Keep `mirakana_ui` as the public first-party runtime UI contract and add headless adapter payload builders that are deterministic and testable. Keep concrete platform/font/IME/accessibility/image implementations as future adapter work; this slice only defines the data contracts and mock adapters needed for production integration points.

**Tech Stack:** C++23, `mirakana_ui`, `mirakana_ui_renderer`, existing tests, no new third-party dependencies.

---

## Context

- `mirakana_ui` already owns retained documents, layout, renderer boxes/text runs, accessibility node payloads, focus/navigation, transitions, text binding, and adapter boundary interfaces.
- `mirakana_ui_renderer` currently maps boxes to `mirakana_renderer::SpriteCommand` but text and image handling remain payload-only.
- Production UI still needs concrete adapter seams for text measurement, glyph placement policy, image resources, and accessibility publication before a real font/image/platform backend can be added under license review.

## Constraints

- Do not add font, shaping, image, accessibility, or UI middleware dependencies.
- Do not expose SDL3, Dear ImGui, OS handles, renderer backend handles, RHI handles, or platform accessibility APIs through game public APIs.
- Keep bidi/shaping/IME/accessibility implementation as policy/adapter contracts, not hand-rolled production systems.
- Runtime game API remains under `mirakana::` / `mirakana::ui`.
- Public header changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.

## Done When

- [x] `mirakana_ui` exposes dependency-free text adapter payloads for deterministic line boxes/glyph placeholders from existing text runs.
- [x] `mirakana_ui` exposes image adapter payloads that reference first-party asset ids or stable resource ids without decoding image bytes.
- [x] Accessibility payload builders preserve role, label/localization key, bounds, enabled/focusable state, and deterministic tree order.
- [x] `mirakana_ui_renderer` or a focused adapter test proves boxes, text payloads, and image placeholders can be consumed through the existing `mirakana_renderer` adapter without adding SDL3/ImGui/native handles or new RHI exposure.
- [x] Docs, roadmap, gap analysis, manifest, and Codex/Claude guidance describe this as Runtime UI Adapter Payload v0, not a production font/text/image/accessibility implementation.
- [x] Focused UI tests, API boundary, schema/agent/format checks, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact host blockers are recorded.

---

### Task 1: RED UI Adapter Payload Tests

**Files:**
- Modify: `tests/unit/ui_renderer_tests.cpp` or `tests/unit/ui_tests.cpp`
- Inspect: `engine/ui/include/mirakana/ui/ui.hpp`
- Inspect: `engine/ui_renderer/include/mirakana/ui_renderer/ui_renderer.hpp`

- [x] Add tests for deterministic text payload rows generated from a retained `UiDocument` text element.
- [x] Add tests for image placeholder payload rows generated from first-party asset ids or resource ids.
- [x] Add tests for accessibility payload tree order and fields.

### Task 2: UI Text And Image Payload Contracts

**Files:**
- Modify: `engine/ui/include/mirakana/ui/ui.hpp`
- Modify: `engine/ui/src/ui.cpp`
- Modify CMake only if a new translation unit is added.

- [x] Add dependency-free value types for text adapter rows, line/glyph placeholder spans, image adapter rows, and adapter diagnostics.
- [x] Build payloads from existing `RendererSubmission` / `RendererTextRun` / document state without introducing a font or image decoder dependency.
- [x] Keep shaping, bidi, font rasterization, IME, and platform accessibility behind future adapters.

### Task 3: Renderer Adapter Consumption Proof

**Files:**
- Modify: `engine/ui_renderer/include/mirakana/ui_renderer/ui_renderer.hpp`
- Modify: `engine/ui_renderer/src/ui_renderer.cpp`
- Modify: focused tests.

- [x] Preserve existing box-to-sprite behavior.
- [x] Add a deterministic adapter result that reports consumed text rows and image placeholders without trying to render glyphs or decode images.
- [x] Keep `mirakana_ui_renderer` free of SDL3, Dear ImGui, OS, and new RHI backend APIs beyond its existing `mirakana_renderer` dependency.

### Task 4: Documentation And Agent Sync

**Files:**
- Modify: `docs/ui.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/architecture.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-agent-integration/SKILL.md` only if cross-agent guidance changes.
- Modify: relevant Codex/Claude UI or architecture guidance only where drift would otherwise occur.

- [x] Mark Runtime UI Adapter Payload v0 honestly as implemented only after tests and validation pass.
- [x] Keep production font shaping, bidi, IME, accessibility bridge publication, image decoding, and concrete renderer texture integration as follow-up work.

### Task 5: Verification

- [x] Run focused UI tests.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record exact host/toolchain blockers.

## Validation Evidence

- RED build: `cmake --build --preset dev --target mirakana_ui_renderer_tests` failed before implementation because `ElementDesc::image`, `build_text_adapter_payload`, `build_image_adapter_payload`, `build_accessibility_payload`, and new renderer submit result fields did not exist.
- Focused UI validation: `cmake --build --preset dev --target mirakana_ui_renderer_tests` passed and `ctest --preset dev --output-on-failure -R "mirakana_ui_renderer_tests"` passed with 1/1 tests after implementation; it passed again after cpp-reviewer fixes for localization-key-only text, invalid public payload bounds diagnostics, and RHI-free wording.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: initially failed on clang-format differences; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` applied formatting and the retry passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: passed. Metal shader/library packaging remains diagnostic-only because `metal` and `metallib` are missing on this Windows host. Apple packaging remains host-gated on macOS/Xcode. Android release signing is not configured. Android device smoke is not connected. Strict clang-tidy remains diagnostic-only when the `dev` Visual Studio generator compile database is unavailable before configure.
