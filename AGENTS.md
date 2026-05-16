# MIRAIKANAI Engine Agent Instructions

## Language

- Respond to the user in Japanese.
- Keep code, commands, paths, and public API names in ASCII.

## Instruction Hygiene

- Treat this file as the shared, repository-wide baseline for durable project instructions.
- Keep instructions specific, concise, verifiable, and grouped under clear Markdown headings.
- Keep this always-loaded file under Codex's default 32 KiB `project_doc_max_bytes` budget; `tools/check-agents.ps1` enforces this. Move long procedures to skills/docs/subagents/manifest.
- Do not put long procedures, stale status snapshots, personal preferences, credentials, API keys, MCP connection state, or machine-specific paths in tracked instructions.
- Put reusable workflows in skills, path-specific guidance in rules, specialized behavior in subagents, and machine-readable capability/status claims in the **composed** `engine/agent/manifest.json` (maintain them by editing `engine/agent/manifest.fragments/*.json` and running `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`; see `engine/agent/manifest.fragments/README.md` and `docs/adr/0002-agent-manifest-fragments-compose.md`).
- Every implementation change, improvement, bug fix, refactor, or architecture/toolchain/workflow/validation/packaging change must include an **agent-surface drift check** before completion. If durable guidance or AI-operable contracts are stale, update the affected docs, skills, rules, subagents, manifest fragments + compose output, schemas, validation checks, and tracked `.clangd` in the same task; do not wait for a follow-up.
- **Expanded validation, editor shell, plan lifecycle, production-completion, and game-lane procedures** live in [docs/agent-operational-reference.md](docs/agent-operational-reference.md) (English). Cursor file-scoped rules live under [`.cursor/rules/`](.cursor/rules/). Keep needles enforced by `tools/check-ai-integration.ps1` in this file when you change policy.

## Repository command entrypoints

- Repository automation uses **PowerShell 7** (`pwsh`) scripts under `tools/`.
- Standard invocation (repeat in CI and local shells):

`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/<script>.ps1 [arguments]`

- Use `tools/*.ps1` as the supported workflow surface; **Bun, Node.js, and `package.json` script aliases are not part of the supported workflow surface**, even when user preferences mention `bun run validate`.
- Every tracked `tools/*.ps1` starts with `#requires -Version 7.0` then `#requires -PSEdition Core`, is **UTF-8 without BOM**, and must parse as PowerShell. `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` also enforces Codex/Claude `gameengine-*` skill twins and Cursor thin pointers; register pairs in `tools/check-agents.ps1` (`claudeToCodexSkillMap`) and add `.cursor/skills/gameengine-<topic>/SKILL.md`.
- For tracked `tools/*.ps1`, keep approved verbs and **PSScriptAnalyzer**-friendly patterns: avoid automatic variable names (`$input`, `$matches`, `$foreach`, `$IsWindows`, `$IsMacOS`, `$IsLinux`), use `$null = ...` for intentional discard, prefer `Write-Information ... -InformationAction Continue`, avoid empty `catch { }`, and use `[CmdletBinding(SupportsShouldProcess = $true)]` plus `$PSCmdlet.ShouldProcess` for host-visible mutations.
- Shared helpers live in `tools/common.ps1`; `Get-RepoRoot` returns a **string**, so use `$root` with `Join-Path` or strings, never `$root.Path`. Keep dot-sourced pairs synchronized, including `tools/check-json-contracts.ps1` / `tools/check-ai-integration.ps1` with `tools/manifest-command-surface-legacy-guard.ps1`. For static contract ledger entrypoints, register chapter files and line budgets in `tools/static-contract-ledger.ps1`, then update `tools/check-*-core.ps1` dispatchers through that ledger.
- `tools/check-ai-integration.ps1` uses a repository-scoped mutex via `Initialize-RepoExclusiveToolMutex` / `Clear-RepoExclusiveToolMutex`; concurrent launches queue.

## Communication Efficiency

- Keep process narration minimal to reduce context and rate usage.
- Do not narrate routine file reads, searches, or obvious command sequencing.
- Provide user-facing updates only for meaningful decisions, planned edits, long-running validation, blockers, and final results.
- Keep Japanese responses concise unless the user asks for detailed reasoning.
- This does not relax requirements for official best practices, clean breaking changes, focused validation, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, or documentation updates.

## Development Efficiency and Scope Control

- Optimize for small, high-signal context: prefer targeted `rg`/file reads, targeted manifest fragments, and `tools/agent-context.ps1 -ContextProfile Minimal` or `Standard`; load full manifests, historical plans, and broad docs only when the current decision needs them.
- Prefer official best practices, Context7, OpenAI developer docs, Anthropic docs, and project skills for current SDK/toolchain/agent behavior; do not trade correctness, security, host gates, license hygiene, or validation evidence for speed.
- Keep the greenfield clean-break policy useful: avoid backward-compatibility shims, deprecated aliases, duplicate APIs, or migration layers unless an explicit future release policy requires them.
- Treat plan files as capability/gap-cluster/milestone records, not PR/task-count units; keep phase behavior/API/validation gates and small steps in the active plan. Do not create dated plans for validation-only follow-up, docs/manifest/static-check sync, small cleanup, or substeps.
- Tests should lock the smallest externally meaningful guarantee for a behavior change, public API change, or bug fix. Prefer updating existing tests when they already cover the contract; avoid tests that merely mirror implementation details, duplicate an existing guarantee, or over-specify incidental ordering.
- During implementation, use focused build/test/static loops first and batch docs/manifest/skills synchronization after behavior is green unless those files are the behavior under test. Before reporting done, run a targeted agent-surface drift check against the changed behavior/workflow and owning surfaces; do not broad-load every agent surface when no durable guidance changed. Use full `validate.ps1` once at the coherent slice-closing gate unless later edits invalidate that evidence.

## Repository Hygiene

- Keep the source tree clean and intentional.
- Do not leave temporary files, scratch files, dead code, dead files, or unused folders in the repository.
- Do not create unnecessary directories, subdirectories, generated copies, or placeholder files.
- Do not add new top-level folders or nested ownership boundaries unless the existing structure cannot fit the work.
- Use existing build, cache, temp, or ignored output locations for transient validation artifacts.
- Remove task-local scaffolding before reporting completion unless it is intentionally tracked and documented.
- Do not delete files or directories that may contain user work unless the task explicitly requires it and the ownership is clear.
- Root `.gitattributes` is the LF contract (`* text=auto eol=lf`); root `.editorconfig` aligns editors, and `tools/check-agents.ps1` enforces it.
- Cleaning ignored paths: `out/`, Android build/.gradle trees, `*.log`, and `imgui.ini` are disposable; **`external/vcpkg` is a required Microsoft vcpkg clone**, not cache. Remove `vcpkg_installed/` only when rerunning `tools/bootstrap-deps.ps1`.

## Project Goal

- This repository is a clean C++ game engine built for desktop development and future desktop/mobile shipping.
- `core-first-mvp` is closed as an MVP scope by `docs/superpowers/plans/2026-05-01-core-first-mvp-closure.md`; future production work needs new focused plans and must not be appended to the historical MVP plan.
- Prioritize official best practices, explicit architecture boundaries, automated validation, and AI-assisted development.
- This is a greenfield project. Do not preserve backward compatibility unless a future release policy explicitly requires it.

## Architecture Rules

- Keep `engine/core` independent from OS APIs, GPU APIs, asset formats, and editor code.
- Put platform-specific work behind interfaces in `engine/platform`.
- Put graphics API work behind renderer/RHI interfaces in `engine/renderer`.
- Keep native OS, window, GPU, and tool interop handles behind backend/PIMPL or first-party opaque handles unless an explicit interop design is accepted.
- Treat Dear ImGui as the optional developer/editor shell only. Runtime game UI should target first-party `mirakana_ui` contracts and must not depend on editor, SDL3, Dear ImGui, or UI middleware APIs.
- Own the UI contract, not every low-level UI implementation detail: text shaping, font rasterization, IME, accessibility bridges, image decoding, and platform integration belong behind official-SDK or audited-dependency adapters.
- Prefer small modules with clear ownership and dependency direction.
- Do not introduce third-party dependencies without updating `docs/legal-and-licensing.md`, `docs/dependencies.md`, `vcpkg.json`, and `THIRD_PARTY_NOTICES.md`.
- Keep optional C++ dependencies in vcpkg manifest features and pin the official registry with `builtin-baseline` for reproducible builds.
- Do not copy code from blogs, Stack Overflow, books, samples, or GitHub snippets unless the license is explicit and recorded.

## C++ Rules

- Use C++23 as the required language level.
- For CMake install layout, `find_package(Mirakanai)` after install, and the C++ module / `import std` matrix, read `docs/building.md`.
- Linux line-coverage minimum for CI is enforced via `tools/coverage-thresholds.json` and `tools/check-coverage.ps1 -Strict` (see `docs/testing.md`).
- Prefer C++23-native designs and allow C++23-only language/library features when they simplify ownership, APIs, or compile-time structure.
- Use project C++ modules through CMake `FILE_SET CXX_MODULES`; keep public installed headers available until module export/install support is intentionally designed.
- Use `import std;` only where the active CMake generator/toolchain reports support; keep it gated by the central CMake policy.
- Do not add C++20 compatibility shims or lower the engine standard without a new architecture decision.
- `MK_apply_common_target_options` sets `/INCREMENTAL:NO` on MSVC linkable targets to avoid `.ilk` / Debug relink LNK1104.
- Follow `docs/cpp-style.md` for naming, source layout, public include paths, CMake target naming, and installable package targets.
- When changing **aggregate** types used from many tests (for example `mirakana::editor::ScenePrefabInstanceRefreshPolicy` in `editor/core/include/mirakana/editor/scene_authoring.hpp`), update every **designated** braced initializer in the same task so **all members appear in declaration order**, including empty **`std::function`** members (`{}`); see **`docs/cpp-style.md`** (**Unit tests** → **Aggregate literals**) and clang `-Wmissing-field-initializers` / IDE diagnostics.
- Prefer RAII and value types.
- Use `std::unique_ptr` / `std::make_unique` for ownership; raw pointers are non-owning.
- Avoid global mutable state.
- Keep public headers minimal and stable within a single task, but do not add compatibility shims.
- The project brand is `MIRAIKANAI` (MIRAIKANAI Engine).
- Technical code name is `mirakana`.
- Public C++ API names use `mirakana::` namespace.
- `mirakana::` is the canonical C++ namespace. Avoid adding new public API in compatibility aliases unless an explicit migration plan requires temporary bridges.

## Testing and Validation

- Default loop, clang-tidy, format, shader tools, Windows diagnostics, `CMake File API` synthesis for `dev`, editor `MK_editor` clang-tidy hygiene, coverage, and C++ preset commands: see [docs/agent-operational-reference.md](docs/agent-operational-reference.md#testing-and-validation-expanded).
- Before production C++ behavior, add or update tests first when the environment can run them; target the smallest externally meaningful guarantee for the behavior/API/regression rather than adding tests by habit.
- During implementation, use the smallest relevant focused loop first: targeted CMake build/test commands for the changed target, plus only the static checks that match the files touched. Avoid rerunning full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` after every code or docs edit.
- Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` once near the end of a coherent slice, after code, docs, manifest, and static-check updates are settled. Rerun it only if later edits can affect validated behavior or checked metadata.
- Toolchain preflight: use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1`; use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1 -RequireDirectCMake` only for raw `cmake --preset ...`, and `-RequireVcpkgToolchain` for vcpkg gates. Presets inherit `normalized-configure-environment` / `normalized-build-environment`; local loops use `tools/cmake.ps1` / `tools/ctest.ps1`.
- Formatting: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` / `tools/format.ps1` cover C++ and tracked text; `tools/check-text-format.ps1` / `tools/format-text.ps1` are text-only. Raw `clang-format --dry-run ...` requires `direct-clang-format-status=ready`.
- Static-analysis PR failures are root-cause work, not log cleanup: keep `.clang-tidy` `HeaderFilterRegex` absolute-path and Windows/Linux separator aware; strict hosted lanes must preserve `--warnings-as-errors=*`; investigate actionable diagnostics before suppressing only frontend summaries like `NN warnings generated.`; use `tools/check-tidy.ps1 -Files` for focused TUs and `-Jobs 0` for full CI throttling.
- For hosted PR/CI check failures, inspect the latest PR head SHA first (`gh pr view <pr> --json headRefOid,statusCheckRollup,url`), open the failing job log for that same SHA, reproduce the narrowest local lane, fix the root cause, then add or extend a repository static guard when the failure exposed a drift-prone contract. If all jobs fail before checkout with a GitHub account billing/spending-limit annotation, report it as a hosted account blocker. Do not diagnose against stale runs or solve by loosening branch protection, Codex rules, or Claude permissions.
- PR CI selection uses an always-running required gate plus conditional heavy lanes, not path-filtered required workflows. `tools/classify-pr-validation-tier.ps1` owns PR lane selection; `tools/check-ci-matrix.ps1` guards cases. Full matrix runs for default/release/scheduled/manual. PR docs/agent/rules/subagent-only changes use formatting plus agent/static guards, not Windows/MSVC, macOS, or full repository clang-tidy unless they touch CI, CMake/vcpkg, build/test/package tooling, runtime code/assets, packaging, or public/runtime contracts.
- For Linux coverage policy changes, remember hosted `lcov` 2.x treats unmatched remove filters as errors. Keep optional `lcovRemovePatterns` guarded with `lcov --ignore-errors unused`, and update `tools/check-coverage-thresholds.ps1` when changing coverage filtering.
- Windows diagnostics use official host tools: Debugging Tools for Windows, Windows Graphics Tools, PIX on Windows, and Windows Performance Toolkit. Treat them as host diagnostics, not repository runtime dependencies.
- If CMake, `clang-format`, or a compiler is missing, report that validation is blocked by missing local tools and include the exact failing command.
- For C++ changes, the intended validation loop is:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure`

## Dependency Bootstrap and vcpkg

- Keep optional C++ dependencies in `vcpkg.json` manifest features and keep the official registry pinned with `builtin-baseline`.
- Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1` as the only repository entrypoint that installs or updates optional vcpkg packages. Feature selection belongs there, not in CMake presets.
- `tools/bootstrap-deps.ps1` requires `external/vcpkg`. CMake configure must not install, restore, or download vcpkg packages; vcpkg presets set `VCPKG_MANIFEST_INSTALL=OFF` and `VCPKG_INSTALLED_DIR=${sourceDir}/vcpkg_installed`. In linked worktrees, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1` before configure.
- Keep project-wide CMake settings in `CMakePresets.json`, local overrides in ignored `CMakeUserPresets.json`, and `vcpkg_installed/` as the shared source-tree package root unless a new architecture decision accepts per-preset trees.
- Treat `CreateFileW stdin failed with 5` during bootstrap as a host or sandbox dependency-bootstrap blocker; do not work around it by moving package installation back into CMake configure.

## AI Development Workflow

- Plan registry, manifest/agent-context workflow, skills parity, `.clangd` / `compile_flags.txt`, `MK_tools` paths, and subagent delegation: see [docs/agent-operational-reference.md](docs/agent-operational-reference.md#ai-development-workflow-expanded).
- Start docs from `docs/README.md`; use `docs/roadmap.md` for status, `docs/superpowers/plans/README.md` before creating/editing/extending plans, and `docs/superpowers/plans/2026-05-11-production-documentation-stack-v1.md` to reconcile manifest pointers, master plan, specs/ADRs, and historical dated plans without bulk-rewriting ledger prose.
- Use `docs/specs/` for feature designs and `docs/superpowers/plans/` for implementation plans. Write new or updated implementation plans in English.
- **Dated plan and spec files:** use `YYYY-MM-DD` in filenames and first headings from the session `Today's date`, explicit operator date, or local `Get-Date -Format yyyy-MM-dd`; keep filename date and heading date identical.
- Keep the live plan stack shallow: one roadmap, one active gap-cluster burn-down or milestone, and at most one phase/child plan selected by `currentActivePlan`. Child plans require a distinct architecture/validation/review boundary or a phase too large for one safe context.
- Prefer active capability/gap-cluster/milestone or phase-gated milestone plans over tiny child plans when work shares one architecture decision, public API family, validation surface, and review purpose. Preserve completed plan files as historical implementation evidence; do not append unrelated work, broaden ready claims, or weaken host gates.
- Keep active plans concise: put detailed evidence in final validation tables, batch docs/manifest/skills synchronization after behavior is green unless those files are the behavior, and ensure each active phase still has Goal, Context, Constraints, Done When, and validation evidence.
- Before generating game code or changing engine APIs, read `engine/agent/manifest.json`, a targeted `engine/agent/manifest.fragments/` file, or `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`. Prefer `-ContextProfile Minimal` or `-ContextProfile Standard`; use Full only when the decision needs full manifest-shaped output.
- Use `.agents/skills/` when a task matches them, keep overlapping `.claude/skills/` behaviorally equivalent, and keep `.cursor/skills/gameengine-*` folders as thin pointers matching `.claude/skills/` names except the intentional Cursor-only `gameengine-cursor-baseline` and `gameengine-plan-registry`.
- After changing agent surfaces (`tools/*.ps1` dot-source pairs, skill folders, validation scripts, rules, settings, subagents, or manifest fragments), follow **Repository consistency checklist** in `docs/workflows.md` (toolchain -> `check-agents` -> `check-ai-integration` -> public API checks -> `validate.ps1` as appropriate).
- Tracked `.clangd` points at `out/build/dev`; run `tools/cmake.ps1 --preset dev` when clangd lacks a database. Use `editor/src/compile_flags.txt` and `editor/include/compile_flags.txt` only as fallback IDE flags. `MK_tools` sources live under `engine/tools/{shader,gltf,asset,scene}/`; public headers stay in `engine/tools/include/mirakana/tools/`.
- For parallel write work, prefer Codex app Worktree/Handoff or Claude Code `--worktree` / subagent `isolation: worktree`; keep `.worktrees/` and `.claude/worktrees/` ignored; manual worktrees use setup, merged worktrees use guarded cleanup.
- Use project subagents in `.codex/agents/` and `.claude/agents/` only when the user explicitly asks for subagent delegation or parallel agent work; keep reviewer/explorer/architect/auditor roles read-only and write tools limited to builder/fixer roles.
- Every task should define: Goal, Context, Constraints, Done when.

## Production Completion Execution

- When executing `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`, use `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`, `recommendedNextPlan`, and `unsupportedProductionGaps` as the current execution index.
- Keep each `unsupportedProductionGaps` row honest, including `oneDotZeroCloseoutTier`, in `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` plus composed output; do not hand-edit `engine/agent/manifest.json`.
- Re-read the master plan, registry, and manifest after user edits or resumes. Finish one selected gap as implemented, host-gated, blocked with evidence, or excluded before switching.
- Prefer official documentation, Context7, project skills, and clean breaking greenfield designs over compatibility shims, broad ready claims, or shortcuts.
- Use focused validation and bounded subagents when they improve speed or confidence; reserve `tools/validate.ps1` for slice gates that touch C++/runtime/build/packaging/public contracts.
- Before reporting completion, reconcile code, tests, docs, plans, manifest, static checks, completed gap, remaining gaps, next active plan, and host-gated blockers against evidence.
- Narrative checklist for production completion: [docs/agent-operational-reference.md](docs/agent-operational-reference.md#production-completion-execution-expanded).

## Git Workflow

- Keep shared ignores in `.gitignore`, repository-local ignores in `.git/info/exclude`, and user-wide editor/backup ignores in readable user `core.excludesFile`. If sandboxed Git reports unreadable global ignore such as `$HOME/.config/git/ignore`, prefer a repo-local `core.excludesFile` in `.git/config` instead of weakening repo permissions.
- If Git author identity is missing, set `user.name` and `user.email` with `git config --local` unless the user explicitly requests global identity.
- Use validated commit checkpoints: commit only task-owned, complete, validated phases. Never commit broken work, scratch output, secrets, credentials, or unrelated changes. Runtime/C++/build/toolchain/public-contract commits need one fresh `tools/validate.ps1`; run `tools/build.ps1` only when standalone build evidence is requested. Docs/non-runtime slices may record narrower justified checks.
- Follow official GitHub Flow: topic branch, reviewable PR, review/checks, merge, branch deletion. Cadence is purpose/checkpoint-based, not commit-count-based: commit validated phases, push validated checkpoints for backup/CI/review/handoff, keep one PR per focused capability/gap-cluster/milestone, and split unrelated work. Inspect status, staged diff, `git diff --cached --check`, and remote state before pushing.
- A slice is **not publication-complete after local validation alone**. Unless local-only/no-PR, finish task-owned branch, stage, commit, non-forced push, and `gh pr create`/GitHub Desktop update with validation evidence or exact blocker before final report. For large slices, open draft PR after the first coherent validated push; mark ready only after final validation/preflight. If starting on default/protected branch, create topic branch; if `codex/<topic>` conflicts, use `codex-<topic>`. Leave unrelated changes unstaged.
- Treat Codex command policy as session-scoped: after editing `.codex/rules/*.rules`, wait for policy reload or a new session when newly allowed commands still require a prompt and approvals are unavailable; do not retry by weakening rules.
- Do not push directly to default/protected branches, force-push without explicit branch-owned request, use admin bypasses, or bypass GitHub branch protection/status checks.
- Before `gh pr merge --auto --merge --delete-branch`, inspect `gh pr view <pr> --json state,isDraft,baseRefName,headRefName,headRefOid,mergeable,mergeStateStatus,reviewDecision,statusCheckRollup,autoMergeRequest,url`; proceed only for an open, non-draft, task-owned PR with fresh validation, expected base, no failed checks, no requested changes/conflicts, and required gates. Prefer `--match-head-commit <headRefOid>`. Treat `DIRTY` / `UNKNOWN` as blockers; `UNSTABLE` / `BLOCKED` is acceptable only for pending required checks or reviews.
- After a task-owned PR merges into `main`, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/remove-merged-worktree.ps1 -WorktreePath <path> [-BaseRef origin/main] [-BaseBranch main] [-Remote origin] [-LocalCheckoutPath <path>] [-DeleteLocalBranch]` for linked worktree cleanup; it fetches/prunes, requires a clean base checkout, fast-forwards it, and removes only safe merged worktrees. Windows long-path fallback stays inside this guarded script, never following reparse points; no ad hoc deletion. If no target remains, record no cleanup target.
- PR state changes through `gh pr edit`, immediate `gh pr merge` without `--auto`, `gh pr ready`, `gh pr close`, or `gh pr reopen` remain prompt-gated. If approvals are unavailable, keep the branch pushed and use GitHub Web/Desktop or an approval-capable session.
- If Git prints credential helper warnings such as `credential-manager-core` missing, inspect `git config --show-origin --get-all credential.helper` and fix host/user Git configuration. On Git for Windows, prefer current Git Credential Manager helper `manager`; do not add repository credential helper overrides or tokens.

## AI Tool Integration

- Codex reads `AGENTS.md`, `.agents/skills/`, `.codex/agents/`, and `.codex/rules/`; Claude Code reads `CLAUDE.md`, `.claude/settings.json`, `.claude/skills/`, and `.claude/agents/`; Cursor loads workspace rules and Agent Skills from `.cursor/rules/` and `.cursor/skills/`. Treat `AGENTS.md` as the authoritative baseline and keep Cursor-only skills short pointers.
- Codex project escalation policy lives in `.codex/rules/`. Keep rules narrow, covered by `match` / `not_match` examples, and aligned with **PowerShell 7** (`pwsh`) for `tools/*.ps1`; do not add broad allow rules for shells, package managers, network tools, destructive commands, force-pushes, or immediate PR merges. Direct default-branch pushes are outside the official GitHub Flow path and must be blocked by project policy and repository branch protection.
- Claude Code shared permissions live in `.claude/settings.json` with the official JSON schema. Deny secret-bearing files and direct default-branch pushes; allow read-only PR preflight, task-owned PR creation, and GitHub auto-merge registration after validation checkpoints; keep destructive, network, dependency-bootstrap, mobile signing/smoke, force-push, immediate PR merge, and other PR state-change commands behind approval. Keep `.claude/settings.local.json`, `.mcp.json`, and `AGENTS.override.md` uncommitted unless governance intentionally changes that.
- Use Context7 MCP for library, SDK, build-system, and toolchain docs such as CMake, vcpkg, SDL3, Dear ImGui, Direct3D 12, Vulkan, Metal, and C++ tooling. Use the OpenAI developer documentation MCP, or official OpenAI documentation when MCP is unavailable, for OpenAI API, Codex, ChatGPT Apps SDK, OpenAI agent, and OpenAI model questions. Use official Anthropic documentation for Claude Code memory, settings, permissions, hooks, skills, and subagent behavior. Keep MCP keys and connector state user-local.
- Keep Codex/Claude/Cursor overlapping instructions behaviorally equivalent. Do not change one Codex/Claude skill pair without updating shared docs or validation checks; update `.cursor/skills/` when the workflow also applies inside Cursor.
- The engine-facing AI integration contract is composed `engine/agent/manifest.json`; **do not hand-edit** it. Edit `engine/agent/manifest.fragments/` and run `tools/compose-agent-manifest.ps1 -Write`. Keep runtime backend readiness, importer capabilities, packaging targets, validation recipes, and JSON Schema entrypoint `schemas/engine-agent.schema.json` honest and machine-readable.
- When adding/renaming retained editor UI row ids such as `play_in_editor.*`, CMake target names, or other AI-contract literals enforced in `tools/check-ai-integration.ps1`, extend the relevant Needles and keep `.agents/skills/editor-change/SKILL.md`, `.claude/skills/gameengine-editor/SKILL.md`, and manifest checks aligned.
- Treat agent-surface drift as implementation work. When engine work changes durable behavior, APIs, workflows, validation, packaging, permissions, or tool expectations, proactively update affected instructions, skills, rules, settings, subagents, manifests, schemas, validation checks, and tracked `.clangd` in the same task.
- Keep reviewer, explorer, architect, and auditor subagents read-only by default in both `.codex/agents/` and `.claude/agents/`; give write-capable tools only to builder/fixer roles expected to change files. Keep always-loaded instructions concise; move reusable procedures into skills and specialized review/build/debug behavior into subagents.

## AI-Driven Game Development

- Games, manifests, scaffolding, desktop runtime, and Android lanes live in [docs/agent-operational-reference.md](docs/agent-operational-reference.md#ai-driven-game-development-expanded).
- Games live under `games/<game_name>/`; `game_name` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/new-game.ps1 -Name <game_name>` values match `^[a-z][a-z0-9_]*$`. Source-tree game directories and `runtimePackageFiles` path segments stay lowercase snake_case; JSON manifest IDs, display names, and external package identifiers may use ecosystem formats such as kebab-case.
- Every game has `game.agent.json` with backend readiness, importer requirements, packaging targets, runtime package files, runtime scene validation targets, and validation recipes.
- Most sample games are headless validation executables. Only `sample_desktop_runtime_shell` and `sample_desktop_runtime_game` are optional windowed SDL3 desktop runtime samples; use `mirakana_editor` for broader visible desktop/editor smoke.
- Use `tools/new-game.ps1` plus `tools/new-game-helpers.ps1` and `tools/new-game-templates.ps1` for scaffolding, and register games with `MK_add_game` or `MK_add_desktop_runtime_game` in `games/CMakeLists.txt`.
- For `desktop-game-runtime`, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`; package selected registered targets with `tools/package-desktop-runtime.ps1 -GameTarget <target>` or use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1` for the default sample shell. Runtime package files are hashed byte-for-byte; new text cooked/runtime extensions in `runtimePackageFiles` need matching `runtime/.gitattributes` `text eol=lf` coverage and static checks before package smoke evidence is trusted.
- For `android-gameactivity`, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-mobile-packaging.ps1` first, then Android-ready hosts may run `tools/build-mobile-android.ps1`, `tools/check-android-release-package.ps1`, and `tools/smoke-android-package.ps1`; never store Android keystores, passwords, SDKs, Gradle caches, or AVD images in the repo.
- Game code should include only public engine headers unless implementing engine internals.

## Done Definition

- Relevant docs/specs/plans are updated.
- Agent-surface drift has been checked, and affected guidance/contracts are updated when durable behavior or workflow changed.
- Relevant tests are added or updated when behavior/API/regression risk changed, and they cover the smallest durable external guarantee.
- Validation has run: focused checks for docs/agent-only/non-runtime slices, full `tools/validate.ps1` for C++/runtime/build/packaging/public-contract slices, or a concrete blocker is recorded.
- Legal and third-party records are updated for any external material.
