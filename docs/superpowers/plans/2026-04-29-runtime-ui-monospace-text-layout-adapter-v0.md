# Runtime UI Monospace Text Layout Adapter v0 Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an opt-in dependency-free runtime UI text layout adapter that turns text adapter rows from whole-line placeholders into deterministic monospace line and glyph boxes for tests, generated HUDs, and future renderer/font adapters.

**Architecture:** Keep `mirakana_ui` headless and renderer/RHI-free. Preserve the existing conservative `build_text_adapter_payload(submission)` behavior, and add a second overload that accepts a first-party monospace layout policy for deterministic glyph metrics, simple line wrapping, clipping diagnostics, and explicit unsupported-bidi diagnostics without pretending to be production shaping or font rasterization.

**Tech Stack:** C++23, `mirakana_ui`, `mirakana_ui_renderer`, existing `mirakana_ui_renderer_tests`, no new third-party dependencies.

---

## Constraints

- Do not add fonts, text shaping, image decoding, accessibility, UI middleware, SDL3, Dear ImGui, OS, renderer backend, RHI, or native handle dependencies.
- Do not hand-roll production complex text shaping, bidirectional reordering, IME, font rasterization, glyph atlas generation, or OS accessibility publication.
- Keep the existing placeholder payload path available for callers that do not opt into deterministic monospace metrics.
- Treat the new adapter as a deterministic fallback/test/debug foundation, not a complete production text stack.
- Public header changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.

## Done When

- [x] `mirakana_ui` exposes a `MonospaceTextLayoutPolicy` and a `build_text_adapter_payload(submission, policy)` overload.
- [x] The policy validates finite positive glyph advance, whitespace advance, and line height.
- [x] The opt-in overload lays out glyph placeholder boxes per UTF-8 scalar span in logical order, wraps at ASCII whitespace for `TextWrapMode::wrap`, clips for `clip` / `ellipsis`, and reports deterministic diagnostics for invalid policy, clipped text, and unsupported right-to-left direction.
- [x] Existing text/image/accessibility payload behavior remains source-compatible.
- [x] Focused UI tests, API boundary, schema/agent/format checks, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass with host gates recorded.

---

### Task 1: RED Tests For Monospace Text Layout

**Files:**
- Modify: `tests/unit/ui_renderer_tests.cpp`

- [x] **Step 1: Add a wrapping test.**

Add a test that builds a label with `text.label = "Play Now"`, `TextWrapMode::wrap`, bounds `36x24`, and calls:

```cpp
const auto payload =
    mirakana::ui::build_text_adapter_payload(submission, mirakana::ui::MonospaceTextLayoutPolicy{8.0F, 4.0F, 10.0F});
```

Assert two lines: `"Play"` at offset `0` count `4` with bounds `{0, 0, 32, 10}`, and `"Now"` at offset `5` count `3` with bounds `{0, 10, 24, 10}`. Assert the first line has four glyph boxes at x `0`, `8`, `16`, and `24`.

- [x] **Step 2: Add a clipping/diagnostic test.**

Add a test that lays out `"ABCDE"` into width `24` with `TextWrapMode::clip` and the same policy. Assert one visible line with three glyphs, `code_unit_count == 3`, and one `AdapterPayloadDiagnosticCode::text_layout_clipped` diagnostic.

- [x] **Step 3: Add policy and bidi diagnostics.**

Add a test that passes an invalid policy with `glyph_advance = 0.0F` and asserts `AdapterPayloadDiagnosticCode::invalid_text_layout_policy`. Add a right-to-left text run and assert `AdapterPayloadDiagnosticCode::unsupported_text_direction` is reported while the row still exists.

- [x] **Step 4: Verify RED.**

Run:

```powershell
cmake --build --preset dev --target mirakana_ui_renderer_tests
```

Expected before implementation: build fails because `MonospaceTextLayoutPolicy`, the overload, and new diagnostic codes do not exist.

### Task 2: Add Monospace Text Layout Contracts

**Files:**
- Modify: `engine/ui/include/mirakana/ui/ui.hpp`
- Modify: `engine/ui/src/ui.cpp`

- [x] **Step 1: Add public value types and diagnostic codes.**

Add:

```cpp
struct MonospaceTextLayoutPolicy {
    float glyph_advance{8.0F};
    float whitespace_advance{4.0F};
    float line_height{16.0F};
};

[[nodiscard]] bool is_valid_monospace_text_layout_policy(MonospaceTextLayoutPolicy policy) noexcept;
```

Extend `AdapterPayloadDiagnosticCode` with:

```cpp
invalid_text_layout_policy,
text_layout_clipped,
unsupported_text_direction,
```

Add:

```cpp
[[nodiscard]] TextAdapterPayload build_text_adapter_payload(const RendererSubmission& submission,
                                                            MonospaceTextLayoutPolicy policy);
```

- [x] **Step 2: Implement policy validation.**

Return false unless all three metrics are finite and strictly positive.

- [x] **Step 3: Implement UTF-8 scalar spans.**

Create private helpers that advance over one valid UTF-8 scalar span when possible, falling back to one byte for malformed input. Store glyph offsets/counts in existing code-unit fields; do not claim grapheme clustering or shaping.

- [x] **Step 4: Implement line layout.**

For the opt-in overload, emit glyph boxes with policy metrics inside the run bounds:

- `glyph_advance` for non-ASCII-space spans.
- `whitespace_advance` for ASCII whitespace.
- `line_height` for every line.
- `TextWrapMode::wrap` wraps at ASCII whitespace when the next token would exceed bounds width, or hard-wraps a single oversized token.
- `TextWrapMode::clip` and `ellipsis` stop when width is exhausted and emit `text_layout_clipped`.
- `TextDirection::right_to_left` emits `unsupported_text_direction`; layout remains logical-order fallback.

- [x] **Step 5: Preserve default placeholder behavior.**

Keep `build_text_adapter_payload(submission)` using the existing whole-line placeholder logic so prior callers stay source-compatible.

### Task 3: Renderer Adapter Reporting Proof

**Files:**
- Modify: `tests/unit/ui_renderer_tests.cpp`
- Modify only if needed: `engine/ui_renderer/include/mirakana/ui_renderer/ui_renderer.hpp`
- Modify only if needed: `engine/ui_renderer/src/ui_renderer.cpp`

- [x] **Step 1: Prove renderer adapter compatibility.**

Extend the focused tests only if needed to show the existing `submit_ui_renderer_submission` still reports text adapter row availability and adapter diagnostic counts after the new overload exists. Do not make `mirakana_ui_renderer` render glyphs in this slice.

- [x] **Step 2: Verify GREEN.**

Run:

```powershell
cmake --build --preset dev --target mirakana_ui_renderer_tests
ctest --preset dev --output-on-failure -R "mirakana_ui_renderer_tests"
```

Expected: build and focused CTest pass.

### Task 4: Docs, Manifest, Skills, And Agent Sync

**Files:**
- Modify: `docs/ui.md`
- Modify: `docs/architecture.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `docs/testing.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `.codex/agents/engine-architect.toml`
- Modify: `.claude/agents/engine-architect.md`
- Modify as needed: `.codex/agents/rendering-auditor.toml`
- Modify as needed: `.claude/agents/rendering-auditor.md`

- [x] **Step 1: Document capability honestly.**

Describe the slice as an opt-in deterministic monospace fallback/test layout adapter with glyph placeholder boxes, simple ASCII whitespace wrapping, clipping diagnostics, and unsupported-bidi diagnostics.

- [x] **Step 2: Preserve production caveats.**

State that complex shaping, bidi reordering, font rasterization, glyph atlas generation, IME, OS accessibility, image decoding, renderer texture upload, and middleware remain adapter/dependency-gated follow-up work.

- [x] **Step 3: Keep Codex and Claude guidance equivalent.**

Update both surfaces so generated-game agents can use the monospace policy only for deterministic HUD/menu tests and must not treat it as full production text.

### Task 5: Validation And Review

**Files:**
- Modify: this plan with validation evidence.
- Modify: `docs/superpowers/plans/README.md` after completion.

- [x] **Step 1: Run focused checks.**

```powershell
cmake --build --preset dev --target mirakana_ui_renderer_tests
ctest --preset dev --output-on-failure -R "mirakana_ui_renderer_tests"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

- [x] **Step 2: Run static/docs checks.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
```

- [x] **Step 3: Request review.**

Use `cpp-reviewer` for public C++ contract and implementation review. Use `rendering-auditor` if `mirakana_ui_renderer` behavior changes beyond reporting counts.

- [x] **Step 4: Run final validation.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Record exact PASS/blocker output and known host-gated diagnostics.

## Validation Evidence

- RED build: `cmake --build --preset dev --target mirakana_ui_renderer_tests` failed as expected after adding tests because `mirakana::ui::MonospaceTextLayoutPolicy`, the overload, and the new diagnostic codes were not implemented yet. The direct PowerShell `cmake` lookup was unavailable in this shell, and MSBuild initially hit a host environment `Path` / `PATH` duplicate issue; rerunning through `tools/common.ps1` discovered CMake and a sanitized `cmd` launch reached the intended compile failures.
- Focused UI build: `cmake --build --preset dev --target mirakana_ui_renderer_tests` passed after implementation when launched with a sanitized single `Path` environment.
- Focused UI CTest: `ctest --preset dev --output-on-failure -R "mirakana_ui_renderer_tests"` passed.
- First `cpp-reviewer` pass found malformed UTF-8 fallback, line-feed whitespace handling, and completion-drift issues. Added RED tests for overlong/surrogate malformed byte fallback and LF whitespace, confirmed focused CTest failed, then fixed scalar validation and whitespace classification.
- Focused UI build/CTest passed again after reviewer fixes.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS. Static AI integration coverage now requires `MonospaceTextLayoutPolicy` in manifest/guidance surfaces.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1`: PASS.
- Follow-up `cpp-reviewer`: PASS with no remaining correctness or boundary blockers.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS. Diagnostic-only gates remain: missing Metal `metal` / `metallib`, Apple packaging/iOS/Metal macOS/Xcode requirements (`xcodebuild` / `xcrun` missing), Android release signing not configured, Android device smoke not connected, and strict clang-tidy compile database unavailable for the active Visual Studio generator before configure.
