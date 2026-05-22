# Post-1.0 Capability Backlog Consolidation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Consolidate duplicated post-1.0 / 1.x capability candidates into one canonical backlog and make the surrounding production-completion chapters thin, implementation-friendly projections.

**Architecture:** `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md` becomes the only chapter that owns post-1.0 / 1.x capability rows. Chapters `03` and `05` through `09` keep their domain-specific selection rules and coverage views, but they reference capability ids from `04` instead of duplicating backlog tables. No engine ready claim, manifest ready surface, or `unsupportedProductionGaps = []` state changes in this docs/governance slice.

**Tech Stack:** Markdown documentation, existing PowerShell validation entrypoints under `tools/`, and the composed manifest as a read-only source of current ready state.

---

### Task 1: Canonical Backlog Rewrite

**Files:**
- Modify: `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md`

- [x] **Step 1: Replace the historical row list with a canonical 1.x backlog**

  Rewrite the chapter so it contains:

  - One purpose statement saying `04` is the canonical selection ledger.
  - One status vocabulary for candidate, foundation-ready, implemented-1x-foundation, optional-adapter-candidate, host-gated, and excluded rows.
  - The canonical capability rows grouped by `ai-game-creation`, `game-facing-systems`, `content-pipeline`, `renderer-production`, `gameplay-physics-nav-ai`, `scale-persistence`, and `optional-adapters`.
  - A selection protocol that requires a dated plan, official documentation review, focused validation, and manifest promotion only after evidence lands.

- [x] **Step 2: Keep 1.0 status unchanged**

  Confirm the new chapter explicitly says post-1.0 rows are not 1.0 blockers while `engine/agent/manifest.json.aiOperableProductionLoop.unsupportedProductionGaps` remains empty.

### Task 2: Thin Projection Chapters

**Files:**
- Modify: `docs/superpowers/master-plans/production-completion-v1/05-2d-3d-capability-coverage-matrix.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/06-renderer-advanced-production-track.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/07-gameplay-physics-nav-ai-advanced-track.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/08-high-freedom-game-creation-track.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/09-sprite-production-pipeline-track.md`

- [x] **Step 1: Remove duplicate backlog tables**

  Replace each recommended-stream table in `03` and `05` through `09` with a short projection table that names only the canonical capability ids from `04`, the chapter-specific reason to read that projection, and the evidence boundary for that domain.

- [x] **Step 2: Preserve official-practice gates**

  Keep domain rules that matter for implementation: backend-neutral renderer APIs, deterministic simulation, middleware opacity, game-owned mutation boundaries, sprite atlas/batching discipline, and 2D/3D evidence separation.

### Task 3: Index And Registry Sync

**Files:**
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/README.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/10-gameplay-archetype-validation.md`
- Modify: `docs/superpowers/plans/README.md`

- [x] **Step 1: Point indexes at the canonical backlog**

  Update the master plan map and split index so `04` is described as the canonical post-1.0 / 1.x backlog, while `05` through `09` are projections and selection guides.

- [x] **Step 2: Keep archetype guidance thin**

  Update archetype wording so missing reusable capabilities are added to the canonical `04` backlog and not duplicated inside archetype-specific text.

- [x] **Step 3: Record the docs/governance slice**

  Add this plan as a completed docs/governance slice in the plan registry after the edit is validated.

### Task 4: Focused Validation

**Files:**
- Validate docs and agent-facing plan discovery only.

- [x] **Step 1: Run text formatting check**

  Run:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
  ```

  Expected: exits 0.

- [x] **Step 2: Run AI integration check**

  Run:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
  ```

  Expected: exits 0.

- [x] **Step 3: Run repository validation if static checks require it**

  Run:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
  ```

  Expected: exits 0, unless a concrete local tool blocker is reported.
