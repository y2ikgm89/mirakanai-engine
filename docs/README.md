# MIRAIKANAI Engine / GameEngine Documentation

This directory is the human-readable project knowledge base for MIRAIKANAI Engine (`Mirakanai`). Keep it organized by purpose. Current-truth docs describe what the engine can do now; dated specs and plans are design records and implementation evidence, not live task queues unless the plan registry or `engine/agent/manifest.json` names them as active.

## Naming

- Brand: `MIRAIKANAI` (`MIRAIKANAI Engine`)
- Repository / package identity: `Mirakanai` (`mirakana::` targets, `MirakanaiConfig.cmake`)
- C++ API namespace: `mirakana::`
- Build-time target aliases: `MK_*` (`MK_core`, `MK_rhi`, `MK_editor`, etc.) are local CMake target names; API-level documentation and sample code should reference canonical `mirakana::` namespaces, not aliases.

## Documentation Language Policy

Use English for current-truth documentation, agent-facing guidance, public API explanations, build and validation procedures, and all new documentation by default. Dated specs and implementation plans are historical records; they may remain in their original language unless they are promoted to current truth, actively edited for a new slice, or referenced by an active manifest/registry pointer.

When a historical Japanese note is promoted into a current doc, translate the promoted text instead of linking readers through untranslated operational guidance. This keeps official toolchain terminology, C++ API names, CI validation, and AI-agent context consistent.

## Current Truth

- [Architecture](architecture.md): module boundaries, dependency direction, and runtime/editor separation.
- [Architecture directory verification](architecture-directory-verification.md): dated assessment of repository layout against common C++/CMake modular practice (2026-05-11).
- [Current capabilities](current-capabilities.md): concise human summary of ready, host-gated, planned, and unsupported capabilities.
- [Roadmap](roadmap.md): current implemented foundation, active priorities, and next work.
- [Building](building.md): CMake presets, install layout, `find_package(Mirakanai)`, and module/import-std matrix.
- [Testing](testing.md): validation commands, coverage expectations, and current test coverage.
- [Workflows](workflows.md): task, validation, CI, packaging, and plan lifecycle rules.
- [Codex local environment](codex-local-environment.md): Codex app setup script and project actions for lightweight worktree setup.
- [Agent operational reference](agent-operational-reference.md): extended validation, editor shell, plan lifecycle, production completion, and game lanes (indexed from `AGENTS.md`).
- Optional Windows PIX host helper (operator-run, non-repo scratch dirs): [`tools/launch-pix-host-helper.ps1`](../tools/launch-pix-host-helper.ps1), summarized under [Workflows — Windows diagnostics toolchain](workflows.md#windows-diagnostics-toolchain). Default **operator PIX + coding-agent analysis** pattern: [AI integration](ai-integration.md) subsection **Recommended workflow (operator PIX, AI analysis)**.
- [AI integration](ai-integration.md) and [AI game development](ai-game-development.md): Codex, Claude Code, and Cursor agent-facing usage guidance.

## System Areas

- [RHI and rendering](rhi.md)
- [Editor](editor.md)
- [Runtime UI](ui.md)
- [Release packaging](release.md)
- [C++ standard](cpp-standard.md) and [C++ style](cpp-style.md)
- [Dependencies](dependencies.md), [legal and licensing](legal-and-licensing.md)

## Design and Plan Records

- [docs/specs/](specs/README.md): accepted designs, analysis records, Superpowers-authored design specs, and generated-game guidance.
- [docs/adr/](adr/README.md): concise architecture decision index for accepted durable decisions.
- [docs/superpowers/plans/](superpowers/plans/README.md): active implementation plan registry plus manifest-pinned foundation records. Completed historical execution evidence belongs in `docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md` or Git history.

Start with [the plan registry](superpowers/plans/README.md) before editing an implementation plan. It identifies the active roadmap, active milestone or focused slice, docs/governance cleanup slices, completed historical plans, host-gated work, and the rules for creating new plans.

## Reading Order

1. Start here.
2. Read [current capabilities](current-capabilities.md) for a short status summary.
3. Read [roadmap](roadmap.md) for priorities and next work.
4. Read subsystem docs only for the area you are changing.
5. Read the relevant spec or implementation plan last, after checking whether it is current, historical, or host-gated.
