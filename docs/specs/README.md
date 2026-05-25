# Specs Registry (single entry point)

This directory holds **design records**, analysis notes, AI handoff/prompt material, and generated-game guidance. It is **not** the implementation queue; execution lives in [`docs/superpowers/plans/README.md`](../superpowers/plans/README.md).

## Truth order when documents disagree

Prefer, in order: [`engine/agent/manifest.json`](../../engine/agent/manifest.json), [`docs/current-capabilities.md`](../current-capabilities.md), [`docs/roadmap.md`](../roadmap.md), and retained implementation-plan evidence. Treat dated specs as historical intent unless a current doc explicitly adopts them. Deleted historical plan/spec evidence remains available through Git history.

## File Contract

Every non-README Markdown file in this directory must contain a `## Status` section near the top. Use one of these exact status categories so AI readers can classify the file before loading details: Historical design record, Retained policy design record, Retained analysis record, Accepted narrow specification, Current generated-game guide, or Historical handoff prompt.

---

## Specs at a glance (read this table first)

| Bucket | Document | One-line summary |
| ------ | -------- | ---------------- |
| **Historical design record** | [`2026-04-25-core-first-mvp-design.md`](2026-04-25-core-first-mvp-design.md) | Original MVP module list and boundaries; **`core-first-mvp` is closed**—use roadmap/manifest for status. |
| **Retained policy design record** | [`2026-04-26-toolchain-dependency-policy-design.md`](2026-04-26-toolchain-dependency-policy-design.md) | Default build stays third-party-free; `vcpkg.json` baseline; optional `desktop-gui` / manifest features; `check-dependency-policy` expectations. |
| **Historical design record** | [`2026-05-01-ai-operable-game-engine-production-roadmap-v1-design.md`](2026-05-01-ai-operable-game-engine-production-roadmap-v1-design.md) | Target architecture input for an **AI-operable** C++23 engine; use current docs and manifest for status. |
| **Historical handoff prompt** | [`2026-05-01-ai-operable-game-engine-handoff-prompt.md`](2026-05-01-ai-operable-game-engine-handoff-prompt.md) | **Copy-paste session prompt** retained as historical context; use `docs/README.md` and `tools/agent-context.ps1` for current handoffs. |
| **Retained analysis record** | [`2026-04-27-engine-essential-gap-analysis.md`](2026-04-27-engine-essential-gap-analysis.md) | “Production-usable” gap list (cooked payloads, GPU binding, editor loop, session services, store-quality gates); P0/P1/P2 style framing. |
| **Retained analysis record** | [`2026-04-27-importer-dependency-audit.md`](2026-04-27-importer-dependency-audit.md) | Optional **`asset-importers`** feature: libspng / fastgltf / miniaudio; **tools-only**, runtime consumes cooked artifacts only. |
| **Retained analysis record** | [`2026-04-27-physics-backend-evaluation.md`](2026-04-27-physics-backend-evaluation.md) | **No third-party physics in current wave**; first-party `mirakana_physics` stays default; Jolt only as a future optional adapter candidate. |
| **Current generated-game guide** | [`game-template.md`](game-template.md) | Blank **game spec** sections (loop, constraints, entities, validation, `game.agent.json` hooks). |
| **Current generated-game guide** | [`generated-game-validation-scenarios.md`](generated-game-validation-scenarios.md) | **Validation recipes** and scenario matrix; ties to manifest `productionLoop`, `aiCommandSurfaces`, and editor playtest review rules. |
| **Current generated-game guide** | [`game-prompt-pack.md`](game-prompt-pack.md) | Short **prompt templates** for recipe selection, AI command surfaces, Scene v2 migration, and schema contracts—always after `tools/agent-context.ps1`. |
| **Accepted narrow specification** | [`2026-05-11-directory-layout-target-v1.md`](2026-05-11-directory-layout-target-v1.md) | Target on-disk split for `MK_tools` sources (`engine/tools/shader|gltf|asset|scene`) with stable public includes; see ADR 0003. |
| **Accepted narrow specification** | [`2026-05-11-editor-game-module-driver-hot-reload-session-state-machine-v1.md`](2026-05-11-editor-game-module-driver-hot-reload-session-state-machine-v1.md) | Phase table for Play-In-Editor activity vs `GameEngine.EditorGameModuleDriver` DLL residency; stops at stopped-state load/reload/unload reviews (no active-session hot reload). |
| **Accepted narrow specification** | [`2026-05-26-engine-contract-version-suffix-cleanup-design.md`](2026-05-26-engine-contract-version-suffix-cleanup-design.md) | Clean-breaking pre-release contract reset that removes removable current `vN` suffixes from engine-owned saved formats, public C++ APIs, editor ABI names, docs, schemas, manifests, and static checks. |

---

## Suggested reading paths

- **Roadmap / “what is current?”** → [`docs/roadmap.md`](../roadmap.md), [`docs/current-capabilities.md`](../current-capabilities.md), manifest—not the historical MVP spec.
- **Implementing a foundation named in the table** → open that foundation row’s doc for scope, constraints, and “done when”; pair with the matching plan under `docs/superpowers/plans/`.
- **Authoring or validating a game with AI** → [`game-template.md`](game-template.md) + [`generated-game-validation-scenarios.md`](generated-game-validation-scenarios.md) + [`game-prompt-pack.md`](game-prompt-pack.md); engine continuity sessions → [`2026-05-01-ai-operable-game-engine-handoff-prompt.md`](2026-05-01-ai-operable-game-engine-handoff-prompt.md).
- **Dependency / toolchain policy** → [`2026-04-26-toolchain-dependency-policy-design.md`](2026-04-26-toolchain-dependency-policy-design.md) and [`2026-04-27-importer-dependency-audit.md`](2026-04-27-importer-dependency-audit.md).

## Historical or superseded material

Some rows are **snapshots** (for example the MVP design). Do not treat them as active scope unless reconciled with the plan registry and manifest.
