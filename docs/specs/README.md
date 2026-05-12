# Specs Registry (single entry point)

This directory holds **design records**, analysis notes, AI handoff/prompt material, and generated-game guidance. It is **not** the implementation queue; execution lives in [`docs/superpowers/plans/README.md`](../superpowers/plans/README.md).

## Truth order when documents disagree

Prefer, in order: latest **completed** implementation plan evidence, [`docs/current-capabilities.md`](../current-capabilities.md), [`docs/roadmap.md`](../roadmap.md), [`engine/agent/manifest.json`](../../engine/agent/manifest.json). Treat dated specs as historical intent unless a current doc explicitly adopts them.

---

## Specs at a glance (read this table first)

| Bucket | Document | One-line summary |
| ------ | -------- | ---------------- |
| **Accepted design** | [`2026-04-25-core-first-mvp-design.md`](2026-04-25-core-first-mvp-design.md) | Original MVP module list and boundaries; **`core-first-mvp` is closed**—use roadmap/manifest for status. |
| **Accepted design** | [`2026-04-26-toolchain-dependency-policy-design.md`](2026-04-26-toolchain-dependency-policy-design.md) | Default build stays third-party-free; `vcpkg.json` baseline; optional `desktop-gui` / manifest features; `check-dependency-policy` expectations. |
| **Accepted design** | [`2026-05-01-ai-operable-game-engine-production-roadmap-v1-design.md`](2026-05-01-ai-operable-game-engine-production-roadmap-v1-design.md) | Target architecture for an **AI-operable** C++23 engine: explicit contracts for scene/asset/package/renderer/validation; strengths, blockers, external anchors. |
| **Accepted design** | [`2026-05-01-ai-operable-game-engine-handoff-prompt.md`](2026-05-01-ai-operable-game-engine-handoff-prompt.md) | **Copy-paste session prompt** listing canonical reads (AGENTS, roadmap, plans, manifest) for continuing that roadmap in a new chat. |
| **Analysis** | [`2026-04-27-engine-essential-gap-analysis.md`](2026-04-27-engine-essential-gap-analysis.md) | “Production-usable” gap list (cooked payloads, GPU binding, editor loop, session services, store-quality gates); P0/P1/P2 style framing. |
| **Analysis** | [`2026-04-27-importer-dependency-audit.md`](2026-04-27-importer-dependency-audit.md) | Optional **`asset-importers`** feature: libspng / fastgltf / miniaudio; **tools-only**, runtime consumes cooked artifacts only. |
| **Analysis** | [`2026-04-27-physics-backend-evaluation.md`](2026-04-27-physics-backend-evaluation.md) | **No third-party physics in current wave**; first-party `mirakana_physics` stays default; Jolt only as a future optional adapter candidate. |
| **Foundation slice** | [`2026-04-27-first-party-byte-payload-cooking.md`](2026-04-27-first-party-byte-payload-cooking.md) | Deterministic **hex byte payloads** on first-party texture/mesh/audio sources preserved through import into cooked packages. |
| **Foundation slice** | [`2026-04-27-navigation-foundation-design.md`](2026-04-27-navigation-foundation-design.md) | **`mirakana_navigation`**: grid A*, deterministic costs/tie-break; no navmesh/crowd claims in the first slice. |
| **Foundation slice** | [`2026-04-27-runtime-mesh-rhi-upload.md`](2026-04-27-runtime-mesh-rhi-upload.md) | Cooked mesh payload bytes → **backend-neutral RHI** vertex/index buffers via `mirakana_runtime_rhi` (no raw gameplay exposure of handles). |
| **Foundation slice** | [`2026-04-27-runtime-session-services-design.md`](2026-04-27-runtime-session-services-design.md) | Per-session **save/settings/localization/input-action** documents over `IFileSystem`; keyboard actions first; no third-party deps. |
| **Foundation slice** | [`2026-04-29-core-observability-foundation-design.md`](2026-04-29-core-observability-foundation-design.md) | **`mirakana_core`** diagnostics recorder (events/counters/profile zones); no GPU/SDL/editor UI in the first slice. |
| **Generated games / agents** | [`game-template.md`](game-template.md) | Blank **game spec** sections (loop, constraints, entities, validation, `game.agent.json` hooks). |
| **Generated games / agents** | [`generated-game-validation-scenarios.md`](generated-game-validation-scenarios.md) | **Validation recipes** and scenario matrix; ties to manifest `productionLoop`, `aiCommandSurfaces`, and editor playtest review rules. |
| **Generated games / agents** | [`game-prompt-pack.md`](game-prompt-pack.md) | Short **prompt templates** for recipe selection, AI command surfaces, Scene v2 migration, and schema contracts—always after `tools/agent-context.ps1`. |
| **Accepted design** | [`2026-05-11-directory-layout-target-v1.md`](2026-05-11-directory-layout-target-v1.md) | Target on-disk split for `MK_tools` sources (`engine/tools/shader|gltf|asset|scene`) with stable public includes; see ADR 0003. |
| **Editor / Play-In-Editor** | [`2026-05-11-editor-game-module-driver-hot-reload-session-state-machine-v1.md`](2026-05-11-editor-game-module-driver-hot-reload-session-state-machine-v1.md) | Phase table for Play-In-Editor activity vs `GameEngine.EditorGameModuleDriver.v1` DLL residency; stops at stopped-state load/reload/unload reviews (no active-session hot reload). |

---

## Suggested reading paths

- **Roadmap / “what is current?”** → [`docs/roadmap.md`](../roadmap.md), [`docs/current-capabilities.md`](../current-capabilities.md), manifest—not the historical MVP spec.
- **Implementing a foundation named in the table** → open that foundation row’s doc for scope, constraints, and “done when”; pair with the matching plan under `docs/superpowers/plans/`.
- **Authoring or validating a game with AI** → [`game-template.md`](game-template.md) + [`generated-game-validation-scenarios.md`](generated-game-validation-scenarios.md) + [`game-prompt-pack.md`](game-prompt-pack.md); engine continuity sessions → [`2026-05-01-ai-operable-game-engine-handoff-prompt.md`](2026-05-01-ai-operable-game-engine-handoff-prompt.md).
- **Dependency / toolchain policy** → [`2026-04-26-toolchain-dependency-policy-design.md`](2026-04-26-toolchain-dependency-policy-design.md) and [`2026-04-27-importer-dependency-audit.md`](2026-04-27-importer-dependency-audit.md).

## Historical or superseded material

Some rows are **snapshots** (for example the MVP design). Do not treat them as active scope unless reconciled with the plan registry and manifest.
