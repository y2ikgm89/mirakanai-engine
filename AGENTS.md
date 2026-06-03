# MIRAIKANAI Engine Agent Instructions

## Language

- Respond in Japanese.
- Keep code, commands, paths, and public API names ASCII.

## Instruction Hygiene

- Treat this file as the shared durable project baseline.
- Keep instructions specific, concise, verifiable, and grouped.
- Follow the AGENTS.md open format: standard Markdown, no required metadata, focused on setup, validation, style, architecture, and security. Put scoped or long guidance elsewhere.
- Keep this always-loaded file under Codex's default 32 KiB `project_doc_max_bytes` budget; `tools/check-agents.ps1` enforces this. Move long procedures to skills/docs/subagents/manifest.
- Do not put long procedures, stale status snapshots, personal preferences, credentials, API keys, MCP connection state, or machine-specific paths in tracked instructions.
- Put reusable workflows in skills, path-specific guidance in rules, specialized behavior in subagents, and machine-readable capability/status claims in the **composed** `engine/agent/manifest.json` (edit `engine/agent/manifest.fragments/*.json` and run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`).
- Every implementation, refactor, architecture/toolchain/workflow/validation/packaging change must include an **agent-surface drift check** before completion. If durable guidance or AI-operable contracts are stale, update owning docs, skills, rules, subagents, manifest fragments + compose output, schemas, validation checks, and tracked `.clangd` in the same task; do not broad-load every agent surface when nothing durable changed.
- **Expanded validation, editor shell, plan lifecycle, production-completion, and game-lane procedures** live in [docs/agent-operational-reference.md](docs/agent-operational-reference.md) (English). Cursor file-scoped rules live under [`.cursor/rules/`](.cursor/rules/). Keep needles enforced by `tools/check-ai-integration.ps1` in this file when you change policy.

## Repository command entrypoints

- Repository automation uses **PowerShell 7** (`pwsh`) scripts under `tools/`.
- Standard invocation (repeat in CI and local shells):

`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/<script>.ps1 [arguments]`

- Use `tools/*.ps1` as the supported workflow surface; **Bun, Node.js, and `package.json` script aliases are not part of the supported workflow surface**, even when user preferences mention `bun run validate`.
- Every tracked `tools/*.ps1` starts with `#requires -Version 7.0` then `#requires -PSEdition Core`, is **UTF-8 without BOM**, and must parse as PowerShell. `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` also enforces Codex/Claude `gameengine-*` skill twins and Cursor thin pointers; register pairs in `tools/check-agents.ps1` (`claudeToCodexSkillMap`) and add `.cursor/skills/gameengine-<topic>/SKILL.md`.
- For tracked `tools/*.ps1`, keep approved verbs and **PSScriptAnalyzer**-friendly patterns: avoid automatic variables (`$input`, `$matches`, `$foreach`, `$IsWindows`, `$IsMacOS`, `$IsLinux`), use `$null = ...`, prefer `Write-Information ... -InformationAction Continue`, avoid empty `catch { }`, and use `ShouldProcess` for host-visible mutations.
- Shared helpers live in `tools/common.ps1`; static-contract-only assertions live in `tools/static-contract-common.ps1`. `Get-RepoRoot` returns a **string**; use `$root`, never `$root.Path`. Keep dot-sourced pairs synchronized. Static contract ledger entrypoints use numeric-prefix discovery from `tools/static-contract-ledger.ps1`; update `tools/check-*-core.ps1` through that ledger, not hand-listed chapter files.
- `tools/check-ai-integration.ps1` uses a repository-scoped mutex via `Initialize-RepoExclusiveToolMutex` / `Clear-RepoExclusiveToolMutex`; concurrent launches queue.

## Communication Efficiency

- Keep process narration minimal to reduce context and rate usage.
- Do not narrate routine file reads, searches, or obvious command sequencing.
- Provide user-facing updates only for meaningful decisions, planned edits, long-running validation, blockers, and final results.
- Keep Japanese responses concise unless the user asks for detailed reasoning.
- This does not relax validation, best-practice, or documentation requirements.

## Development Efficiency and Scope Control

- Optimize for small, high-signal context: prefer targeted `rg`/file reads, targeted manifest fragments, and `tools/agent-context.ps1 -ContextProfile Minimal` or `Standard`; load full manifests, historical plans, and broad docs only when the current decision needs them.
- Prefer official best practices, Context7, OpenAI developer docs, Anthropic docs, and project skills for current SDK/toolchain/agent behavior; do not trade correctness, security, host gates, license hygiene, or validation evidence for speed.
- Keep the greenfield clean-break policy useful: avoid backward-compatibility shims, deprecated aliases, duplicate APIs, or migration layers unless an explicit future release policy requires them.
- Treat plan files as capability/gap-cluster/milestone records, not PR/task-count units; keep phase behavior/API/validation gates and small steps in the active plan. Do not create dated plans for validation-only follow-up, docs/manifest/static-check sync, small cleanup, or substeps.
- Tests should lock the smallest externally meaningful guarantee for a behavior change, public API change, or bug fix. Prefer updating existing tests when they already cover the contract; avoid tests that merely mirror implementation details, duplicate an existing guarantee, or over-specify incidental ordering.
- During implementation, use focused build/test/static loops first and batch docs/manifest/skills sync after behavior is green unless those files are under test. Before reporting done, run a targeted agent-surface drift check; use full `validate.ps1` once at slice close unless later edits invalidate it.

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
- Active editor/runtime UI uses first-party `mirakana_ui`, `mirakana::ui`, and `MK_editor`; do not add Dear ImGui/Qt/Slint/RmlUi/SDL3/UI-middleware deps without a new architecture decision plus dependency/license updates.
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
- MSVC: `MK_apply_common_target_options` owns `/MP2`+`/Zf`, short target-named `COMPILE_PDB_OUTPUT_DIRECTORY`, `/INCREMENTAL:NO`, and compact CMake target names for long one-source tests; wrappers serialize builds and clear stale MSVC `.tlog` roots.
- Follow `docs/cpp-style.md` for naming, source layout, public include paths, CMake target naming, and installable package targets.
- For test aggregates, keep designated initializers in declaration order and include empty `std::function` members (`{}`); see `docs/cpp-style.md`.
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
- Static-analysis PR failures need root fixes: keep `HeaderFilterRegex` path aware and hosted `--warnings-as-errors=*`; suppress only `NN warnings generated.`; use `tools/check-tidy.ps1 -Files` locally and sharded `-Jobs 0` in CI. `validate.ps1` uses bounded static jobs, CMake File API reuse, `test.ps1 -SkipBuild`, CI-only static flags, and automatic CMake/CTest parallelism.
- For hosted PR/CI failures, inspect latest PR head SHA, open the failing job log for that SHA, reproduce the narrowest local lane, fix root cause, then extend static guards when the failure exposed drift-prone contracts. Billing/spending-limit failures before checkout are hosted account blockers. Do not diagnose stale runs or loosen branch protection, Codex rules, or Claude permissions.
- PR CI selection uses an always-running required gate plus conditional lanes, not path-filtered required workflows. `tools/classify-pr-validation-tier.ps1` selects; `tools/check-ci-matrix.ps1` guards. `Agent Static Guards` runs `validate.ps1 -StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120`; PR `Windows MSVC` runs `validate.ps1 -SkipStaticChecks -SkipTidySmoke`; checkout is pinned/read-only; caches use `actions/cache/restore` / `actions/cache/save`. Docs/agent/rules/subagent-only PRs use agent/static guards unless they touch CI/build/runtime/packaging/public contracts.
- Hosted `MK_d3d12_rhi_tests` uses Microsoft WARP for CI; hardware proof needs host diagnostics/package smoke.
- For Linux coverage policy changes, remember hosted `lcov` 2.x treats unmatched remove filters as errors. Keep optional `lcovRemovePatterns` guarded with `lcov --ignore-errors unused`, and update `tools/check-coverage-thresholds.ps1` when changing coverage filtering.
- Windows diagnostics use official host tools: Debugging Tools for Windows, Windows Graphics Tools, PIX on Windows, and Windows Performance Toolkit. Treat them as host diagnostics, not repository runtime dependencies.
- If CMake, `clang-format`, or a compiler is missing, report that validation is blocked by missing local tools and include the exact failing command.
- For C++ changes, the intended loop is `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev`, then `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure`.

## Dependency Bootstrap and vcpkg

- Keep optional C++ dependencies in `vcpkg.json` manifest features and keep the official registry pinned with `builtin-baseline`.
- Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1` as the only repository entrypoint that installs or updates optional vcpkg packages. Feature selection belongs there, not in CMake presets.
- `tools/bootstrap-deps.ps1` requires `external/vcpkg`. CMake configure must not install, restore, or download vcpkg packages; vcpkg presets set `VCPKG_MANIFEST_INSTALL=OFF` and `VCPKG_INSTALLED_DIR=${sourceDir}/vcpkg_installed`. In linked worktrees, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1` before configure.
- Keep project-wide CMake settings in `CMakePresets.json`, local overrides in ignored `CMakeUserPresets.json`, and `vcpkg_installed/` as the shared source-tree package root unless a new architecture decision accepts per-preset trees.
- Treat `CreateFileW stdin failed with 5` during bootstrap as a host or sandbox dependency-bootstrap blocker; do not work around it by moving package installation back into CMake configure.

## AI Development Workflow

- Plan registry, manifest/agent-context workflow, skills parity, `.clangd` / `compile_flags.txt`, `MK_tools` paths, and subagent delegation: see [docs/agent-operational-reference.md](docs/agent-operational-reference.md#ai-development-workflow-expanded).
- Start docs from `docs/README.md`; use `docs/roadmap.md` for status, `docs/superpowers/plans/README.md` before creating/editing/extending plans, and reconcile manifest pointers, the master plan, specs/ADRs, and archived historical plan evidence without bulk-rewriting ledger prose.
- Use `docs/specs/` for feature designs and `docs/superpowers/plans/` for implementation plans. Write new or updated implementation plans in English.
- **Dated plan and spec files:** use `YYYY-MM-DD` in filenames and first headings from the session `Today's date`, explicit operator date, or local `Get-Date -Format yyyy-MM-dd`; keep filename date and heading date identical.
- Keep the live plan stack shallow: one roadmap, one active gap-cluster burn-down or milestone, and at most one phase/child plan selected by `currentActivePlan`. Child plans require a distinct architecture/validation/review boundary or a phase too large for one safe context.
- Prefer active gap/milestone or phase-gated milestone plan over tiny child plans for one architecture/API/validation/review purpose. Preserve completed plans while referenced by current docs, manifests, checks, or decisions; delete unreferenced noise in cleanup and rely on Git history. Do not append unrelated work, broaden ready claims, or weaken host gates
- Keep active plans concise: put detailed evidence in final validation tables, batch docs/manifest/skills synchronization after behavior is green unless those files are the behavior, and ensure each active phase still has Goal, Context, Constraints, Done When, and validation evidence.
- Before generating game code or changing engine APIs, read `engine/agent/manifest.json`, a targeted `engine/agent/manifest.fragments/` file, or `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`. Prefer `-ContextProfile Minimal` or `-ContextProfile Standard`; use Full only when the decision needs full manifest-shaped output.
- Use `.agents/skills/` when a task matches them, keep overlapping `.claude/skills/` behaviorally equivalent, and keep `.cursor/skills/gameengine-*` folders as thin pointers matching `.claude/skills/` names except the intentional Cursor-only `gameengine-cursor-baseline` and `gameengine-plan-registry`.
- After changing agent surfaces (`tools/*.ps1` dot-source pairs, skill folders, validation scripts, rules, settings, subagents, or manifest fragments), follow **Repository consistency checklist** in `docs/workflows.md` (toolchain -> `check-agents` -> `check-ai-integration` -> `check-json-contracts` -> public API checks -> `validate.ps1`).
- Tracked `.clangd` points at `out/build/dev`; run `tools/cmake.ps1 --preset dev` when clangd lacks a database. Use `editor/src/compile_flags.txt` and `editor/include/compile_flags.txt` only as fallback IDE flags. `MK_tools` sources live under `engine/tools/{shader,gltf,asset,scene}/`; public headers stay in `engine/tools/include/mirakana/tools/`.
- For parallel write work, prefer Codex app Worktree/Handoff or Claude Code `--worktree` / subagent `isolation: worktree`; keep `.worktrees/` and `.claude/worktrees/` ignored; manual worktrees use setup, merged worktrees use guarded cleanup.
- Use subagents in `.codex/agents/` and `.claude/agents/` only when explicitly asked; keep roles scoped, close completed/obsolete/no-longer-needed agents promptly after their result is consumed and before spawning replacements, and leave useful work running.
- Every task should define: Goal, Context, Constraints, Done when.

## Production Completion Execution

- When executing the lightweight production-completion master-plan index, use `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`, `recommendedNextPlan`, and `unsupportedProductionGaps`; load only the needed production-completion corpus chapter.
- Keep each `unsupportedProductionGaps` row honest, including `oneDotZeroCloseoutTier`, in `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` plus composed output; do not hand-edit `engine/agent/manifest.json`.
- Re-read the master plan, registry, and manifest after user edits or resumes. Finish one selected gap as implemented, host-gated, blocked with evidence, or excluded before switching.
- Prefer official documentation, Context7, project skills, and clean breaking greenfield designs over compatibility shims, broad ready claims, or shortcuts.
- Use focused validation and bounded subagents when they improve speed or confidence; reserve `tools/validate.ps1` for slice gates that touch C++/runtime/build/packaging/public contracts.
- Before reporting completion, reconcile code, tests, docs, plans, manifest, static checks, completed gap, remaining gaps, next active plan, and host-gated blockers against evidence.
- Narrative checklist for production completion: [docs/agent-operational-reference.md](docs/agent-operational-reference.md#production-completion-execution-expanded).

## Git Workflow

- Keep shared ignores in `.gitignore`, repo-local ignores in `.git/info/exclude`, and user editor/backup ignores in readable user `core.excludesFile`; if unreadable, prefer a repo-local `core.excludesFile` in `.git/config` instead of weakening permissions.
- If Git author identity is missing, set `user.name` and `user.email` with `git config --local` unless global identity is explicitly requested.
- Use validated commit checkpoints: commit only task-owned, complete, validated phases; never commit broken work, scratch output, secrets, credentials, or unrelated changes. Runtime/C++/build/toolchain/public-contract commits need one fresh `tools/validate.ps1`; run `tools/build.ps1` only when standalone build evidence is requested. Docs/non-runtime slices may record narrower justified checks.
- Follow official GitHub Flow. Cadence is purpose/checkpoint-based, not commit-count-based: commit validated phases, push validated checkpoints, keep one PR per focused capability/gap-cluster/milestone, and split unrelated work. Before staging/push/PR/merge, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1`; it checks Git write, network, `origin`, `gh auth`, PR head SHA, and `index.lock`. If blocked, switch context.
- A slice is **not publication-complete after local validation alone**. Unless local-only/no-PR, publish validated changes through `gh pr create` with evidence/blocker before final report; open draft PRs for large slices; use `codex-<topic>` on ref conflict; use `tools/ready-task-pr.ps1` for ready after preflight. Leave unrelated changes unstaged.
- Treat Codex command policy as session-scoped: after editing `.codex/rules/*.rules`, wait for policy reload or a new session when newly allowed commands still require a prompt and approvals are unavailable; do not retry by weakening rules.
- Do not push directly to default/protected branches, force-push without explicit branch-owned request, use admin bypasses, or bypass GitHub branch protection/status checks.
- Auto-merge registration must use `gh pr merge --auto --merge --match-head-commit <headRefOid>` after `gh pr view <pr> --json ...`; do not pass `--delete-branch` in linked-worktree sessions. Require clean local worktree, task-owned PR, fresh validation, current `headRefOid`, clean PR state, selected hosted checks, and `PR Gate` `SUCCESS`. After merge, verify the merged head reaches `origin/main`; commits pushed after merge need a new PR.
- Draft-to-ready automation must use `tools/ready-task-pr.ps1`, not raw `gh pr ready`.
- After merge to `main`, run `tools/remove-merged-worktree.ps1` from a clean base checkout; it fetches/prunes, fast-forwards, verifies branch ancestry, unlinks worktree-local vcpkg links; Windows fallback stays guarded/non-following.
- Raw PR state changes through `gh pr edit`, immediate `gh pr merge` without `--auto`, raw `gh pr ready`, `gh pr close`, or `gh pr reopen` remain prompt-gated. If the wrapper blocks, use GitHub Desktop/Web or an approval-capable session.
- Git warnings are local maintenance. Missing `credential-manager-core`: inspect `git config --show-origin --get-all credential.helper`, prefer `manager`, and avoid repo overrides. Auto-GC loose objects: clean `git status --short --branch`, inspect `git count-objects -vH`, then `git maintenance run --task=gc`; avoid `git prune` / `git gc --prune=now` unless requested.

## AI Tool Integration

- Codex reads `AGENTS.md`, `.agents/skills/`, `.codex/agents/`, and `.codex/rules/`; Claude Code reads `CLAUDE.md`, `.claude/settings.json`, `.claude/skills/`, and `.claude/agents/`; Cursor reads root `AGENTS.md`, `.cursor/rules/`, `.cursor/skills/`, and `.cursor/agents/`. Treat `AGENTS.md` as the authoritative baseline.
- Codex project escalation policy lives in `.codex/rules/`. Keep rules narrow, covered by `match` / `not_match`, and aligned with **PowerShell 7** (`pwsh`) for `tools/*.ps1`; do not add broad allow rules. Direct default-branch pushes are outside the official GitHub Flow path and must be blocked by project policy and branch protection.
- Claude Code shared permissions live in `.claude/settings.json`. Deny secrets and default-branch pushes; allow guarded publication preflight, task-owned PR creation, guarded `tools/ready-task-pr.ps1`, and auto-merge registration after validation; keep destructive, network, dependency-bootstrap, mobile signing/smoke, force-push, immediate PR merge, raw `gh pr ready`, and other PR state changes behind approval. Keep `.claude/settings.local.json`, `.mcp.json`, and `AGENTS.override.md` uncommitted.
- Cursor project rules belong in focused `.cursor/rules/*.mdc` files with metadata, concrete examples, scoped content, and Cursor's 500-line rule budget. Cursor treats root `AGENTS.md` as global guidance; use `.cursor/rules/`, `.cursor/skills/*/SKILL.md`, and `.cursor/agents/*.md`. Cursor project subagents pin `model: composer-2.5-fast` unless the operator names another supported Cursor model.
- Use Context7 MCP for library/SDK/build/toolchain docs. Use OpenAI developer documentation MCP for OpenAI API, ChatGPT Apps SDK, Codex, OpenAI agent, and model questions; otherwise use official OpenAI docs. Use official Anthropic documentation for Claude Code memory, settings, permissions, hooks, skills, and subagents. Keep MCP keys and connector state user-local.
- Keep Codex/Claude/Cursor overlapping instructions behaviorally equivalent. Do not change one Codex/Claude skill pair without updating shared docs or validation checks; update `.cursor/skills/` and `.cursor/agents/` when the workflow also applies inside Cursor.
- The engine-facing AI integration contract is composed `engine/agent/manifest.json`; **do not hand-edit** it. Edit `engine/agent/manifest.fragments/` and run `tools/compose-agent-manifest.ps1 -Write`. Keep runtime backend readiness, importer capabilities, packaging targets, validation recipes, and JSON Schema entrypoint `schemas/engine-agent.schema.json` honest and machine-readable.
- When adding/renaming retained editor UI row ids such as `play_in_editor.*`, CMake target names, or other AI-contract literals enforced in `tools/check-ai-integration.ps1`, extend the relevant Needles and keep `.agents/skills/editor-change/SKILL.md`, `.claude/skills/gameengine-editor/SKILL.md`, and manifest checks aligned.
- Treat agent-surface drift as implementation work. When engine work changes durable behavior, APIs, workflows, validation, packaging, permissions, or tool expectations, proactively update affected instructions, skills, rules, settings, subagents, manifests, schemas, validation checks, and tracked `.clangd` in the same task.
- Keep reviewer, explorer, architect, planning, and auditor subagents read-only by default in `.codex/agents/`, `.claude/agents/`, and `.cursor/agents/`; give write-capable tools only to builder/fixer roles expected to change files. Move reusable procedures into skills and specialized review/build/debug behavior into subagents.

## AI-Driven Game Development

- Games, manifests, scaffolding, desktop runtime, Android lanes, and procedures live in [docs/agent-operational-reference.md](docs/agent-operational-reference.md#ai-driven-game-development-expanded).
- Games live under `games/<game_name>/`; game names match `^[a-z][a-z0-9_]*$`; source/runtime package path segments stay lowercase snake_case; JSON manifest IDs may use kebab-case.
- Every game has `game.agent.json` with backend readiness, importer requirements, packaging targets, runtime package files, runtime scene validation targets, and validation recipes. Scaffold with `tools/new-game.ps1`, `tools/new-game-helpers.ps1`, and `tools/new-game-templates.ps1`, then register with `MK_add_game` or `MK_add_desktop_runtime_game`.
- Generated Game Studio v1 is the reviewed read-only 2D/3D agent loop. Use `EditorAiGeneratedGameStudioV1Model` / `generated_game_studio` rows as evidence, route missing engine capability through `ai-engine-capability-handoff-v1`, and do not execute recipes, mutate manifests, edit internals, expose native handles, or claim renderer/RHI/Metal/broad editor readiness.
- Sample games are usually headless; only `sample_desktop_runtime_shell` and `sample_desktop_runtime_game` are optional windowed Windows-native runtime samples. The visible editor shell is deferred; use `MK_editor_core` tests for editor logic until a first-party shell is added.
- For `desktop-game-runtime`, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`; package with `tools/package-desktop-runtime.ps1 [-GameTarget <target>]`. New text cooked/runtime extensions need matching `runtime/.gitattributes` `text eol=lf` coverage and static checks.
- For `android-gameactivity`, run `tools/check-mobile-packaging.ps1` first; Android-ready hosts may run build/release/smoke scripts. Never store Android keystores, passwords, SDKs, Gradle caches, or AVD images in the repo.
- Game code includes only public engine headers unless implementing internals.

## Done Definition

- Relevant docs/specs/plans are updated.
- Agent-surface drift has been checked, and affected guidance/contracts are updated when durable behavior or workflow changed.
- Relevant tests are added or updated when behavior/API/regression risk changed, and they cover the smallest durable external guarantee.
- Validation has run: focused checks for docs/agent-only/non-runtime slices, full `tools/validate.ps1` for C++/runtime/build/packaging/public-contract slices, or a concrete blocker is recorded.
- Legal and third-party records are updated for any external material.
