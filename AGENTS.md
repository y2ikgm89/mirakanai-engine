# MIRAIKANAI Engine Agent Instructions

## Language

- Respond to the user in Japanese.
- Keep code, commands, paths, and public API names in ASCII.

## Instruction Hygiene

- Treat this file as the shared, repository-wide baseline for durable project instructions.
- Keep instructions specific, concise, verifiable, and grouped under clear Markdown headings.
- Keep this always-loaded file below Codex's default `project_doc_max_bytes` budget when practical; move long procedures to skills/docs.
- Do not put long procedures, stale status snapshots, personal preferences, credentials, API keys, MCP connection state, or machine-specific paths in tracked instructions.
- Put reusable workflows in skills, path-specific guidance in rules, specialized behavior in subagents, and machine-readable capability/status claims in the **composed** `engine/agent/manifest.json` (maintain them by editing `engine/agent/manifest.fragments/*.json` and running `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`; see `engine/agent/manifest.fragments/README.md` and `docs/adr/0002-agent-manifest-fragments-compose.md`).
- Review this file when workflow, permission, toolchain, or architecture decisions change, and synchronize `CLAUDE.md`, docs, `.agents/skills/`, `.claude/skills/`, `.cursor/skills/`, rules, settings, subagents, `engine/agent/manifest.fragments/` + compose output (`engine/agent/manifest.json`), schemas, validation checks, and tracked `.clangd` when IDE compile-database defaults change.
- **Expanded validation, editor shell, plan lifecycle, production-completion, and game-lane procedures** live in [docs/agent-operational-reference.md](docs/agent-operational-reference.md) (English). Cursor file-scoped rules live under [`.cursor/rules/`](.cursor/rules/). Keep needles enforced by `tools/check-ai-integration.ps1` in this file when you change policy.

## Repository command entrypoints

- Canonical repository automation uses **PowerShell 7** (`pwsh`) and scripts under `tools/`.
- Standard invocation (repeat in CI and local shells):

`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/<script>.ps1 [arguments]`

- Canonical automation uses **PowerShell 7** (`pwsh`) and scripts under `tools/` only. **Bun, Node.js, and `package.json` script aliases are not part of the supported workflow surface.**
- Every tracked `tools/*.ps1` declares `#requires -Version 7.0` followed immediately by `#requires -PSEdition Core`; scripts must be **UTF-8 without BOM**. `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` enforces this plus Codex ↔ Claude `gameengine-*` skill twins and Cursor thin-pointer directory names; register new pairs in `tools/check-agents.ps1` (`claudeToCodexSkillMap`) and add the thin `.cursor/skills/gameengine-<topic>/SKILL.md`.
- For tracked `tools/*.ps1`, use approved verbs and **PSScriptAnalyzer**-friendly patterns: do not bind locals or iterators to automatic variables (`$input`, `$matches`, `$foreach`, `$IsWindows`, `$IsMacOS`, `$IsLinux`), use `$null = ...` for intentional discard, prefer `Write-Information ... -InformationAction Continue` over `Write-Host`, avoid empty `catch { }`, and use `[CmdletBinding(SupportsShouldProcess = $true)]` plus `$PSCmdlet.ShouldProcess` for host-visible mutations.
- Shared automation helpers live in `tools/common.ps1`; `Get-RepoRoot` returns a **string** path, so use `$root` with `Join-Path` or string operations, never `$root.Path`. Keep dot-sourced script pairs synchronized, including `tools/check-json-contracts.ps1` / `tools/check-ai-integration.ps1` with `tools/manifest-command-surface-legacy-guard.ps1`.
- `tools/check-ai-integration.ps1` uses a repository-scoped mutex through `Initialize-RepoExclusiveToolMutex` / `Clear-RepoExclusiveToolMutex`; concurrent `pwsh ... -File tools/check-ai-integration.ps1` launches must queue instead of racing scaffold checks.

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
- Plan work at the smallest useful **behavior/API/validation boundary**. Do not create new dated plans for validation-only follow-up, docs/manifest/static-check synchronization, small mechanical cleanup, or substeps that fit the active plan checklist.
- Tests should lock the smallest externally meaningful guarantee for a behavior change, public API change, or bug fix. Prefer updating existing tests when they already cover the contract; avoid tests that merely mirror implementation details, duplicate an existing guarantee, or over-specify incidental ordering.
- During implementation, use focused build/test/static loops first and batch docs/manifest/skills synchronization after behavior is green unless those files are the behavior under test. Use full `validate.ps1` once at the coherent slice-closing gate unless later edits invalidate that evidence.

## Repository Hygiene

- Keep the source tree clean and intentional.
- Do not leave temporary files, scratch files, dead code, dead files, or unused folders in the repository.
- Do not create unnecessary directories, subdirectories, generated copies, or placeholder files.
- Do not add new top-level folders or nested ownership boundaries unless the existing structure cannot fit the work.
- Use existing build, cache, temp, or ignored output locations for transient validation artifacts.
- Remove task-local scaffolding before reporting completion unless it is intentionally tracked and documented.
- Do not delete files or directories that may contain user work unless the task explicitly requires it and the ownership is clear.
- When cleaning ignored paths, distinguish **build output** from **tool checkouts**. `out/`, Android build/.gradle trees, `*.log`, and `imgui.ini` are disposable; **`external/vcpkg` is a required Microsoft vcpkg clone** referenced by `CMakePresets.json`, so do not delete `external/vcpkg` or the whole `external/` tree as cache. Remove `vcpkg_installed/` only when you intend to rerun `tools/bootstrap-deps.ps1`.

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
- Follow `docs/cpp-style.md` for naming, source layout, public include paths, CMake target naming, and installable package targets.
- When changing **aggregate** types used from many tests (for example `mirakana::editor::ScenePrefabInstanceRefreshPolicy` in `editor/core/include/mirakana/editor/scene_authoring.hpp`), update every **designated** braced initializer in the same task so **all members appear in declaration order**, including empty **`std::function`** members (`{}`); see **`docs/cpp-style.md`** (**Unit tests** → **Aggregate literals**) and clang `-Wmissing-field-initializers` / IDE diagnostics.
- Prefer RAII and value types.
- Use `std::unique_ptr` for ownership. Raw pointers are non-owning and must not be stored ambiguously.
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
- Toolchain preflight: use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1`; use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1 -RequireDirectCMake` only when direct `cmake --preset ...` must be on `PATH`. Checked-in CMake presets must inherit `normalized-build-environment`.
- Formatting and static checks: use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` / `tools/format.ps1`; raw `clang-format --dry-run ...` requires `direct-clang-format-status=ready`. Use `tools/check-public-api-boundaries.ps1`, `tools/check-shader-toolchain.ps1`, `tools/check-tidy.ps1`, `tools/check-coverage.ps1`, and `tools/check-dependency-policy.ps1` only when their lane matches touched files or the slice gate requires them.
- Windows diagnostics use official host tools: Debugging Tools for Windows, Windows Graphics Tools, PIX on Windows, and Windows Performance Toolkit. Treat them as host diagnostics, not repository runtime dependencies.
- If CMake, `clang-format`, or a compiler is missing, report that validation is blocked by missing local tools and include the exact failing command.
- For C++ changes, the intended validation loop is:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1`
  - `cmake --preset dev`
  - `cmake --build --preset dev`
  - `ctest --preset dev --output-on-failure`

## Dependency Bootstrap and vcpkg

- Keep optional C++ dependencies in `vcpkg.json` manifest features and keep the official registry pinned with `builtin-baseline`.
- Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1` as the only repository entrypoint that installs or updates optional vcpkg packages. Feature selection belongs there, not in CMake presets.
- `tools/bootstrap-deps.ps1` requires an existing `external/vcpkg` checkout. CMake configure must not install, restore, or download vcpkg packages; vcpkg-enabled presets must set `VCPKG_MANIFEST_INSTALL=OFF` and `VCPKG_INSTALLED_DIR=${sourceDir}/vcpkg_installed`.
- Keep project-wide CMake settings in `CMakePresets.json`, local overrides in ignored `CMakeUserPresets.json`, and `vcpkg_installed/` as the shared source-tree package root unless a new architecture decision accepts per-preset trees.
- Treat `CreateFileW stdin failed with 5` during bootstrap as a host or sandbox dependency-bootstrap blocker; do not work around it by moving package installation back into CMake configure.

## AI Development Workflow

- Plan registry, manifest/agent-context workflow, skills parity, `.clangd` / `compile_flags.txt`, `MK_tools` paths, and subagent delegation: see [docs/agent-operational-reference.md](docs/agent-operational-reference.md#ai-development-workflow-expanded).
- Start documentation navigation from `docs/README.md`.
- Use `docs/roadmap.md` for current engine status and priorities.
- Use `docs/superpowers/plans/README.md` as the implementation plan registry before creating, editing, or extending plans.
- Use `docs/superpowers/plans/2026-05-11-production-documentation-stack-v1.md` to reconcile manifest pointers, the production-completion master plan, specs/ADRs, and historical dated plans without bulk-rewriting ledger prose; foundation layout changes are clean-break and are not a substitute for `unsupportedProductionGaps` burn-down.
- Use `docs/specs/` for feature designs and `docs/superpowers/plans/` for implementation plans.
- Write new and updated implementation plans under `docs/superpowers/plans/` in **English** (headings, prose, tables) so they stay consistent with the production-completion master plan and remain easy to grep and review across tooling; localized explanations may still live in `docs/roadmap.md` or other human-facing docs when needed.
- **Dated plan and spec files:** Use `YYYY-MM-DD` in filenames and in the first heading (for example `# Title v1 (YYYY-MM-DD)`) for the **authoritative calendar date at authoring time**. When working inside Cursor or another agent host, treat the session **`Today's date`** field (or an equivalent user-stated date) as authoritative—do not infer dates from model defaults or UTC-only boundaries. If that field is missing, confirm the date with the operator or run a local `Get-Date -Format yyyy-MM-dd` (or host equivalent) before choosing the prefix; keep filename date and heading date identical.
- Keep the live plan stack shallow: one active roadmap, one active gap burn-down or milestone, and at most one active child/phase plan selected by `currentActivePlan`.
- Create a new dated focused plan only for a distinct production slice with its own behavior/API/validation boundary; do not create new plans for validation-only follow-up, docs synchronization, static-check edits, or substeps that fit the current active plan checklist.
- Prefer extending the active gap burn-down or milestone checklist over creating many tiny child plans. Use a phase-gated milestone plan when tightly related production slices share one end-to-end objective and repeated tiny active-plan hops would hide the decision.
- Do not append unrelated work to completed historical plans. Preserve completed plan files as historical implementation evidence unless an explicit cleanup task proves they are obsolete; reduce noise through the registry and evidence indexes instead of deleting traceability.
- Keep active implementation plans concise enough to execute without excessive context. Put detailed historical evidence in the final validation table, not in repeated progress prose, and batch docs/manifest/static-check synchronization after behavior is green unless those files are the behavior under test.
- Prefer a slightly larger gap-level burn-down or phase plan over many tiny dated plans when the work shares one architecture decision and validation surface; create child plans only when the next phase would be too large to execute, validate, and review safely in one context.
- Each active phase still needs Goal, Context, Constraints, Done When, and validation evidence; do not use gap burn-downs or milestones to broaden ready claims or weaken host gates.
- Read `engine/agent/manifest.json` (or a **targeted fragment** under `engine/agent/manifest.fragments/` when you know the slice) or run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` before generating game code or changing engine APIs. Prefer `tools/agent-context.ps1 -ContextProfile Minimal` or `-ContextProfile Standard` when a smaller JSON summary is enough; default is `-ContextProfile Full` (full manifest-shaped output).
- Use repository skills in `.agents/skills/` when the task matches their description (Codex). Keep overlapping topics in **behavioral parity** with `.claude/skills/` (Claude Code): update both trees in the same task when shared workflows or validation commands change.
- Use Cursor Agent Skills in `.cursor/skills/` when working inside Cursor. Prefer thin `.cursor/skills/gameengine-*` folders that name-match `.claude/skills/` and link readers to the full `.claude/skills/` / `.agents/skills/` bodies; keep IDE-only guidance there and avoid duplicating long prose that already lives in `AGENTS.md` or shared skills. Cursor-only skill folders that intentionally have no Claude twin stay limited to `gameengine-cursor-baseline` and `gameengine-plan-registry` unless `tools/check-agents.ps1` is updated together.
- After changing agent surfaces (`tools/*.ps1` dot-source pairs, skill folders, or validation scripts), follow **Repository consistency checklist** in `docs/workflows.md` (toolchain → `check-agents` → `check-ai-integration` → public API checks → `validate.ps1` as appropriate).
- Tracked `.clangd` points at `out/build/dev`; configure `cmake --preset dev` when clangd lacks a database. Use `editor/src/compile_flags.txt` and `editor/include/compile_flags.txt` only as fallback IDE flags. `MK_tools` implementation sources live under `engine/tools/{shader,gltf,asset,scene}/` with public headers in `engine/tools/include/mirakana/tools/`.
- Use project subagents in `.codex/agents/` and `.claude/agents/` for bounded independent work such as parallel read-only exploration, code review, build failures, gameplay implementation, and rendering-specific audits when the user explicitly asks for subagent delegation or parallel agent work. Give each delegated task clear ownership and keep immediate blocking work local.
- Every task should define: Goal, Context, Constraints, Done when.

## Production Completion Execution

- When executing `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`, use `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`, `recommendedNextPlan`, and `unsupportedProductionGaps` as the current execution index.
- Keep each `unsupportedProductionGaps` row honest, including `oneDotZeroCloseoutTier`, in `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` plus composed output; do not hand-edit `engine/agent/manifest.json`.
- Re-read the master plan, registry, and manifest after user edits or resumes. Finish one selected gap as implemented, host-gated, blocked with evidence, or excluded before switching.
- Prefer official documentation, Context7, project skills, and clean breaking greenfield designs over compatibility shims, broad ready claims, or shortcuts.
- Use focused validation and bounded subagents only when they improve speed or confidence, then run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` at the slice gate.
- Before reporting completion, reconcile code, tests, docs, plans, manifest, static checks, completed gap, remaining gaps, next active plan, and host-gated blockers against evidence.
- Narrative checklist for production completion: [docs/agent-operational-reference.md](docs/agent-operational-reference.md#production-completion-execution-expanded).

## Git Workflow

- Keep shared ignore rules in committed `.gitignore`; keep repository-local, unshared ignore rules in `.git/info/exclude`; keep user-wide editor or backup patterns in `core.excludesFile` only when the user-level path is readable.
- If sandboxed Git reports an unreadable default global ignore such as `$HOME/.config/git/ignore`, do not weaken repository permissions or add host-specific paths to `.gitignore`. Prefer a repository-local `core.excludesFile` pointing at the absolute `.git/info/exclude` path, stored only in `.git/config`.
- If Git author identity is missing, configure `user.name` and `user.email` with `git config --local` for this repository instead of changing global user settings unless the user explicitly asks for a global identity.
- Implementation plans should include validated commit checkpoints. Prefer small commits after coherent, passing slices; do not commit known-broken work, generated scratch output, or unrelated user changes. For a slice-closing commit, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` then `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` first, or for documentation-only/non-runtime slices record the justified narrower checks in the PR body and completion report; otherwise record the exact blocker in the plan and completion report.
- Commit and push task-owned changes at coherent, validated checkpoints without asking for per-action confirmation. Before committing, inspect `git status --short --branch`, review the staged diff, and stage only task-owned files. Never include secrets, credential files, generated scratch output, or unrelated user changes.
- Treat Codex command policy as session-scoped: after editing `.codex/rules/*.rules`, do not assume the active session has reloaded newly allowed commands. If the active policy still requires a prompt and approvals are unavailable (for example `Approval policy: never`), record the blocker and finish in a reloaded or new session instead of retrying or weakening rules.
- Prefer topic branches plus GitHub pull requests for shared or release-facing work. Do not push directly to the default/protected branch, do not force-push unless the user explicitly requests it and branch ownership is clear, and do not bypass GitHub branch protection or required review/status checks.
- For GitHub publishing, push the current topic branch with a reviewed remote/branch target, then create or update a pull request through GitHub Web, `gh`, or GitHub Desktop with a concise title/body and actual validation evidence. In unattended Codex sessions, `gh pr create` and `gh pr merge --auto --merge --delete-branch` may run automatically after task-owned validation checkpoints so GitHub branch protection and required checks control the final `main` merge. If credentials, branch protection, required checks, or auto-merge registration block push/PR work, report the blocker instead of weakening repository policy.
- GitHub PR state changes through `gh pr edit`, immediate `gh pr merge` without `--auto`, `gh pr ready`, `gh pr close`, or `gh pr reopen` remain prompt-gated. If approvals are unavailable in the current session, do not retry those gated commands; keep the branch pushed and use GitHub Web/Desktop or an approval-capable session.
- If Git prints credential helper warnings such as `credential-manager-core` missing, inspect `git config --show-origin --get-all credential.helper` and fix only the host/user Git configuration. On Git for Windows, prefer the current Git Credential Manager helper `manager`; do not add repository credential helper overrides or tokens to mask stale host settings.

## AI Tool Integration

- Codex reads this `AGENTS.md`, `.agents/skills/`, and `.codex/agents/`.
- Cursor loads workspace rules and Agent Skills from `.cursor/rules/` and `.cursor/skills/` when present; treat `AGENTS.md` as the authoritative baseline and keep Cursor-only skills short pointers to it.
- Codex project command escalation policy lives in `.codex/rules/`. Keep rules narrow and covered by `match` / `not_match` examples; do not add broad allow rules for shells, package managers, network tools, destructive commands, direct default-branch pushes, force-pushes, or immediate PR merges. Keep `match` / `not_match` examples aligned with **PowerShell 7** (`pwsh`) for `tools/*.ps1` (see **Repository command entrypoints**); those scripts require PowerShell 7 and are not supported on Windows PowerShell 5.1. Rule edits may require a policy reload or new session before newly allowed commands can run.
- Claude Code reads `CLAUDE.md`, which imports this file and the project rules, plus `.claude/settings.json`, `.claude/skills/`, and `.claude/agents/`.
- Claude Code shared project permissions live in `.claude/settings.json` with the official JSON schema. Keep secret-bearing files denied, allow task-owned PR creation and auto-merge registration after validation checkpoints, and keep destructive, network, dependency-bootstrap, mobile signing/smoke, force-push, direct default-branch push, immediate PR merge, and other PR state-change commands behind approval. Keep `.claude/settings.local.json`, `.mcp.json`, and `AGENTS.override.md` uncommitted unless a future governance decision intentionally changes that.
- Use Context7 MCP as the preferred live documentation lookup for library, SDK, build-system, and toolchain questions before relying on model memory, especially for CMake, vcpkg, SDL3, Dear ImGui, Direct3D 12, Vulkan, Metal, and C++ tooling. Keep Context7 API keys in user-level MCP configuration only; never commit keys or personal MCP settings to this repository.
- Use the OpenAI developer documentation MCP, or official OpenAI documentation when MCP is unavailable, for OpenAI API, Codex, ChatGPT Apps SDK, OpenAI agent, and OpenAI model questions. Use official Anthropic documentation for Claude Code memory, settings, permissions, hooks, skills, and subagent behavior.
- Keep Codex and Claude Code instructions behaviorally equivalent for overlapping topics (`.agents/skills/` versus `.claude/skills/`). When the same workflow applies inside Cursor, update `.cursor/skills/` in the same task instead of letting IDE guidance drift. Do not change one Codex/Claude pair without updating the shared docs or validation checks.
- The engine-facing AI integration contract is `engine/agent/manifest.json` (composed from `engine/agent/manifest.fragments/` via `tools/compose-agent-manifest.ps1`); **do not hand-edit** the canonical file—edit fragments and regenerate. It must keep runtime backend readiness, importer capabilities, packaging targets, and validation recipes honest and machine-readable. JSON Schema entrypoint is `schemas/engine-agent.schema.json` (modular definitions under `schemas/engine-agent/`).
- When adding or renaming **retained editor UI row ids** (for example `play_in_editor.*`), **CMake target names** or other literals enforced for AI contracts in `tools/check-ai-integration.ps1`, extend the relevant **Needles** in that script in the same task and keep `.agents/skills/editor-change/SKILL.md`, `.claude/skills/gameengine-editor/SKILL.md`, and manifest-related checks aligned so validation stays unambiguous.
- When engine work reveals missing or stale agent guidance, proactively update the relevant `AGENTS.md`, `CLAUDE.md`, docs, Codex/Claude/Cursor skills, rules, settings, subagents, manifests, schemas, validation checks, and tracked `.clangd` when compile-database defaults change in the same task.
- Keep reviewer, explorer, architect, and auditor subagents read-only by default in both `.codex/agents/` and `.claude/agents/`; give write-capable tools only to builder/fixer roles that are expected to change files.
- Keep always-loaded instructions concise; move reusable procedures into skills and specialized review/build/debug behavior into subagents.

## AI-Driven Game Development

- Games, manifests, scaffolding, desktop runtime, and Android lanes: see [docs/agent-operational-reference.md](docs/agent-operational-reference.md#ai-driven-game-development-expanded).
- Games live under `games/<game_name>/`.
- `game_name` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/new-game.ps1 -Name <game_name>` values must match `^[a-z][a-z0-9_]*$`; keep source-tree game directories and `runtimePackageFiles` path segments lowercase snake_case.
- JSON manifest IDs, display names, and external package identifiers may use their ecosystem formats, including kebab-case when required; do not convert those identifiers back into source paths.
- Every game must include `game.agent.json` so AI agents can understand the game contract, including backend readiness, importer requirements, packaging targets, runtime package files, runtime scene validation targets, and validation recipes.
- Most current sample games are headless validation executables. `sample_desktop_runtime_shell` and `sample_desktop_runtime_game` are the optional windowed SDL3 desktop runtime samples; do not promise a visible game window outside the desktop-runtime lane unless a runtime windowed host is implemented. Use `mirakana_editor` for the broader visible desktop/editor smoke path.
- Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/new-game.ps1 -Name <game_name>` to scaffold a new C++ game. Shared scaffolding helpers live in `tools/new-game-helpers.ps1` (dot-sourced by `new-game.ps1`); update both when changing formatting or string-literal helpers used by generated sources.
- Register default source-tree games through `games/CMakeLists.txt` with `MK_add_game`. Register optional windowed desktop runtime games with `MK_add_desktop_runtime_game`.
- When a game selects the optional `desktop-game-runtime` lane, validate with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`; use `tools/package-desktop-runtime.ps1 -GameTarget <target>` when validating a selected registered desktop-runtime package target. Registered desktop runtime game targets must declare source-tree smoke args, package smoke args, shader-artifact requirements, and package files through CMake metadata when runtime config/assets must ship beside the executable; keep `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1` as the default sample-shell package lane.
- When a game or engine task selects `android-gameactivity`, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-mobile-packaging.ps1` first. If Android reports ready, validate Debug packages with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-mobile-android.ps1 -Game <game_name> -Configuration Debug`; validate Release signing with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-android-release-package.ps1 -Game <game_name> -UseLocalValidationKey` for local non-repository keys or CI secrets for real upload keys; validate emulator launch with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/smoke-android-package.ps1 -Game <game_name> -Configuration Release -SkipBuild -StartEmulator -AvdName Mirakanai_API36` when the configured AVD is available. Do not store Android keystores, passwords, SDKs, Gradle caches, or AVD images in the repo.
- Game code should include only public engine headers unless implementing engine internals.

## Done Definition

- Relevant docs/specs/plans are updated.
- Relevant tests are added or updated when behavior/API/regression risk changed, and they cover the smallest durable external guarantee.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` has been run, or a concrete environment blocker is recorded.
- Legal and third-party records are updated for any external material.
