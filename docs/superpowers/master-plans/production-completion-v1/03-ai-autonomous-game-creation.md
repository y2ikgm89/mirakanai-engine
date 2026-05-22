# Production Completion v1 - AI Autonomous Game Creation

Source index: [Production Completion Master Plan v1](../2026-05-03-production-completion-master-plan-v1.md). Load this chapter only when selecting or reviewing Codex / Claude Code / Cursor game-creation workflow work.

## Purpose

This chapter is the AI game-creation projection over the canonical backlog in [04-developer-owned-engine-capability-backlog.md](04-developer-owned-engine-capability-backlog.md). It owns game-creation boundaries, official agent-surface rules, and remediation behavior. It does not own a separate backlog table.

The autonomous game-creation lane is game-surface only by default: agents may generate and update game files, game code, game manifests, source assets, placeholder assets, package registration rows, validation evidence, and repo-owned operation commands. They must not modify engine internals, renderer/RHI/backend code, platform adapters, build-system contracts, toolchain policy, or shared agent surfaces unless the operator starts a separate engine-feature task.

## Official Agent-Surface Anchors

- OpenAI Codex and Codex CLI behavior must be checked against official OpenAI documentation when changing Codex-facing workflows.
- Claude Code memory, subagents, settings, permissions, hooks, and skills must be checked against official Anthropic documentation when changing Claude-facing workflows.
- Cursor-facing behavior must stay aligned with repository `.cursor/` rules, skills, agents, and the shared AGENTS baseline.
- Repository automation must stay on reviewed `tools/*.ps1` entrypoints. Autonomous workflows must not rely on broad shell permissions, untracked user settings, or hidden local state.

## AI Game-Creation Rules

- Start from `engine/agent/manifest.json`, `game.agent.json`, and `tools/agent-context.ps1 -ContextProfile Minimal|Standard`, not broad source-tree exploration.
- Keep game-owned mutation under `games/<game_name>/`, game-local source asset roots, game-local docs/evidence, and reviewed generated/runtime package registration rows.
- Treat `engine/`, `editor/`, shared `tools/`, schemas, skills, rules, subagents, CMake/vcpkg, and validation policy as read-only unless a separate engine-change plan is selected.
- Create or change game content through reviewed dry-run/apply tools, typed command rows, or documented file formats.
- Require a structured game design contract before generation: gameplay family, template, camera, input map, core loop, win/loss/restart, scene list, asset requests, systems, package targets, validation recipes, and quality gates.
- Use first-party or reviewed placeholder assets with provenance/license rows. Do not copy web, store, sample, blog, or repository assets without explicit license evidence.
- Record validation/playtest evidence, classify failures, and repair through remediation recipes. Do not loosen validation or delete evidence to pass.
- If a requested game needs unsupported engine capability, emit a typed developer handoff that references a canonical row in `04`.

## AI Projection

| Workflow concern | Canonical rows | Evidence boundary |
| --- | --- | --- |
| Design before generation | `ai-game-design-spec-v1` | Schema/static checks, examples for 2D/3D templates, and validation that required gameplay/package fields exist. |
| Generation orchestration | `ai-game-generation-orchestrator-v1` | Dry-run/apply rows, deterministic file lists, generated 2D/3D package evidence, and no arbitrary shell. |
| Safe mutation boundaries | `ai-safe-content-mutation-ledger-v1` | `game.agent.json` schema rows, static checks, and failure when AI-owned mutation surfaces drift. |
| Placeholder assets | `ai-placeholder-asset-pipeline-v1`, `engine-asset-placeholder-generation-v1` | Provenance rows, license status, package rows, hash/eol checks, and package-visible placeholder evidence. |
| Playtest and remediation | `ai-generated-game-playtest-loop-v1`, `ai-validation-remediation-recipes-v1` | Recipe output summaries, gameplay counters, package smoke logs, remediation rows, and no validation weakening. |
| Quality gate | `ai-generated-game-quality-rubric-v1` | Objective clarity, controls, feedback, fail/restart, deterministic simulation, package evidence, and performance budget rows. |
| Agent-surface alignment | `ai-codex-claude-agent-surface-v1` | `check-agents`, `check-ai-integration`, official docs review, and no broad permission grants. |
| Developer handoff | `ai-engine-capability-handoff-v1` | Handoff rows include requested capability, current workaround, blocked feature, desired public contract, and required evidence. |

## Missing-Capability Handoff

When a game-creation agent hits a missing reusable engine capability, it must stop at a handoff/remediation row. The row should name the requested game feature, the closest supported workaround, the affected game files, the desired first-party API or data contract, and the evidence needed before future agents can use the capability.

Only a developer-owned engine-feature task may change engine internals and promote the new capability back through the manifest, schema, docs, skills, and validation recipes.
