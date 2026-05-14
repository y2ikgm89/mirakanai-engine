---
name: gameengine-cursor-baseline
description: >-
  Runs GameEngine work inside Cursor by prioritizing AGENTS.md, PowerShell validation wrappers
  under tools/, and selective reads of `engine/agent/manifest.fragments/` or the composed `engine/agent/manifest.json`. Use when editing this repository's C++
  engine (engine/), CMake presets, vcpkg policy, games/, public headers, shaders/RHI,
  editor/, manifests/schemas, or when diagnosing validation failures on Windows/Linux/macOS lanes.
paths:
  - "**/*"
disable-model-invocation: false
---

# GameEngine Cursor baseline

## Authority

1. **`AGENTS.md`** — validation recipes, dependency bootstrap rules, language policy, Done Definition.
2. **`CLAUDE.md` / `.claude/` / `.codex/` / `.agents/skills/` / `.cursor/skills/`** — same baseline as Codex/Claude Code; **do not copy large excerpts into Cursor-only prompts**. Resolve repository conflicts by following **`AGENTS.md`** (workspace truth); if Cursor global instructions disagree with this repository's Git Workflow, align the global instruction or add a workspace override before relying on automation.
3. **Thin `.cursor/skills/gameengine-*` topic folders** — name-aligned with `.claude/skills/`; each points at the full Claude skill plus the Codex twin (`.agents/skills/cmake-build-system`, `cpp-engine-debugging`, `editor-change`, `rendering-change`, `license-audit`, or same-name `gameengine-*` where applicable).

## Cursor-first discipline

- **Superpowers / hooks:** Hooks that require a Claude Code **Skill** tool do not apply verbatim in Cursor (no Skill tool). Read the matching `SKILL.md` with the **Read** tool instead; see `docs/ai-integration.md` § **Cursor vs Claude Code hooks (Superpowers)**.
- **Execute** repo wrappers (`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/<script>.ps1`) yourself when diagnosing or claiming readiness; avoid dumping commands without running them when the environment allows. Tracked `tools/*.ps1` declares `#requires -Version 7.0` immediately followed by `#requires -PSEdition Core`—use **PowerShell 7** (`pwsh`), not Windows PowerShell 5.1. `tools/check-agents.ps1` (via `validate.ps1`) enforces UTF-8 **without** BOM and the contiguous `#requires` pair. Shared helpers live in `tools/common.ps1` (`Get-RepoRoot` returns a **string**—use `$root` in joins, never `$root.Path`; `Invoke-CheckedCommand`, and related).
- **GitHub publishing:** Follow official GitHub Flow. Commit and non-forced push task-owned topic-branch changes at coherent, validated checkpoints without asking for per-action confirmation. Follow **`AGENTS.md` → Git Workflow** and **`docs/workflows.md` → Commit, Push, And Pull Request Workflow**: stage only task-owned files, run required validation or record blockers, push topic branches, prefer pull requests, allow task-owned `gh pr create` and `gh pr merge --auto --merge --delete-branch` only after validation checkpoints plus PR preflight (`mergeStateStatus`, `statusCheckRollup`, `reviewDecision`, and `--match-head-commit <headRefOid>` when available), prune stale remote-tracking refs after deleted task branches when useful, and never push directly to the default branch or bypass protected-branch/required-check policy. When `.codex/rules` changes in the same task, wait for policy reload or a new session before relying on newly allowed commands. Prompt-gated PR state changes such as `gh pr edit` or immediate `gh pr merge` still need GitHub Web/Desktop or an approval-capable session if approvals are unavailable. If Git credential helper warnings mention missing helpers such as `credential-manager-core`, inspect host/user `credential.helper` settings and fix them outside the repository.
- Before changing **engine-facing APIs**, **`game.agent.json` contracts**, or **generated-game scaffolding**, skim **`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`** (optional **`-ContextProfile Minimal|Standard|Full`**) or read **targeted** `engine/agent/manifest.fragments/*.json` / composed `engine/agent/manifest.json` slices (avoid loading the whole canonical file unless necessary). When you **author** engine manifest changes, edit fragments and run **`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`**; `tools/check-json-contracts.ps1` enforces parity. 3D desktop package manifests must keep **`prefabScenePackageAuthoringTargets`** and **`registeredSourceAssetCookTargets`** rows aligned on `sourceRegistryPath` / `packageIndexPath`; `schemas/game-agent.schema.json`, `tools/check-json-contracts.ps1`, and `tools/check-ai-integration.ps1` enforce this—do not invent alternate cook inputs outside those descriptors.
- After touching a **single `.cpp`**, you may run **`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files <path>`** (requires prior `cmake --preset dev`) to reproduce IDE `misc-include-cleaner` / clang-tidy feedback narrowly; see `AGENTS.md` Testing section. **`check-tidy.ps1 -Files` accepts only `.cc`, `.cpp`, and `.cxx`**—for a public header change, pass the **primary implementation TU** that includes it or use **clangd** diagnostics on the header.

## Validation shorthand (details in AGENTS.md)

| Situation | Start here |
|-----------|------------|
| Any substantive completion claim | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` (or record toolchain blocker) |
| CMake / compiler / PATH issues | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1`; raw cmake lanes → same script with `-RequireDirectCMake` |
| Default dev loop | `cmake --preset dev` → build preset → `ctest --preset dev --output-on-failure` |
| Public headers / backend interop | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` |
| Shader / RHI / viewport toolchain | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` |
| AI + Windows D3D12 capture handoff (editor vs PIX host) | `docs/ai-integration.md` § **AI-driven workflows and Windows GPU diagnostics** and § **Recommended workflow (operator PIX, AI analysis)**; optional `tools/launch-pix-host-helper.ps1` (`WinPix.exe` / legacy `PIX.exe`, `MIRAKANA_PIX_EXE`, `-SkipLaunch`) |
| Agent surfaces / skills / `tools/*.ps1` pairs | `pwsh ... tools/check-agents.ps1`; follow **Repository consistency checklist** in `docs/workflows.md`; after new retained editor ids or `check-ai-integration.ps1` Needles, run `pwsh ... tools/check-ai-integration.ps1` |
| Shared C++ patterns touched | `pwsh ... tools/check-tidy.ps1` or `-Files` for narrow TU runs |
| Large `MK_editor` shell (`editor/src/main.cpp` and focused shell `.cpp` under `editor/src/`) | Same as row above; use `-Files editor/src/main.cpp` or include `editor/src/material_preview_gpu_cache.cpp`, `editor/src/sdl_viewport_texture.cpp` when those TUs change, after `cmake --preset dev`; see **Dear ImGui shell** / **MK_editor Windows and material-preview host cache** in `.claude/skills/gameengine-editor/SKILL.md` / `.agents/skills/editor-change/SKILL.md` and `AGENTS.md` Testing |
| Other `editor/core` translation units | After substantive edits, `check-tidy.ps1 -Files editor/core/src/<file>.cpp` (after `cmake --preset dev`) and `cmake --build --preset dev --target MK_editor_core` when behavior is confined to `editor/core`; see **Editor core C++** in the shared editor skills |
| clangd / IDE includes | Tracked `.clangd` → `CompilationDatabase: out/build/dev` after `cmake --preset dev`; fallback `editor/src/compile_flags.txt` and `editor/include/compile_flags.txt` when TUs or `editor/include/**/*.hpp` are missing from the database—extend them when new include roots are required (`AGENTS.md` AI Development Workflow) |
| Format | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` / `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` |

Optional lanes (`desktop-game-runtime`, `package-desktop-runtime`, Android/iOS) remain **host-gated**; treat missing SDKs as explicit blockers, not silent skips.

## Language

- **User-visible prose**: Japanese when the user writes Japanese (per project agent instructions).
- **Code, CLI, paths, `mirakana::` API names**: ASCII.

## Anti-patterns

- Lowering C++ standard or adding deps without **`docs/legal-and-licensing.md`**, **`docs/dependencies.md`**, **`vcpkg.json`**, **`THIRD_PARTY_NOTICES.md`** (see AGENTS.md).
- Installing vcpkg packages from CMake configure; use **`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1`** only.
- Committing or pushing unrelated changes, pushing directly to protected/default branches, force-pushing without explicit user request, or creating/updating PRs without a reviewed branch/remote target, task-owned staged files, and validation evidence or a recorded blocker.
- Deleting **`external/vcpkg`** (or all of **`external/`**) while cleaning ignored files; presets require that clone for `CMAKE_TOOLCHAIN_FILE`. See **`AGENTS.md`** Repository Hygiene and **`gameengine-cmake-build-system`** skill.
- Editing **`tests/unit/*.cpp`** while leaving outdated substring idioms (`find` + `npos`), incomplete aggregate literals, wrong designated-init field order when structs gain members, split anonymous namespaces that drop internal linkage for helpers, **`main` inside anonymous linkage** in huge TUs, or **public-counter** test doubles where **`class` + private + `const` getters** fit—prefer fixes in **`docs/cpp-style.md`** (**Unit tests**) before broad NOLINT. Editor core coverage often lives in **`tests/unit/editor_core_tests.cpp`** (`MK_editor_core_tests`); use `cmake --build --preset dev --target MK_editor_core_tests` or `ctest --preset dev --output-on-failure -R MK_editor_core_tests` for a narrow loop.
