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
- Static-analysis PR failures are root-cause work, not log cleanup: keep `.clang-tidy` `HeaderFilterRegex` absolute-path and Windows/Linux separator aware; strict hosted lanes must preserve `--warnings-as-errors=*`; investigate actionable diagnostics before suppressing only frontend summaries like `NN warnings generated.`; use `tools/check-tidy.ps1 -Files` for focused TUs and `-Jobs 0` for full CI throttling.
- For hosted PR/CI check failures, inspect the latest PR head SHA first (`gh pr view <pr> --json headRefOid,statusCheckRollup,url`), open the failing job log for that same SHA, reproduce the narrowest local lane, fix the root cause, then add or extend a repository static guard when the failure exposed a drift-prone contract. If all jobs fail before checkout with a GitHub account billing/spending-limit annotation, report it as a hosted account blocker. Do not diagnose against stale runs or solve by loosening branch protection, Codex rules, or Claude permissions.
- For Linux coverage policy changes, remember hosted `lcov` 2.x treats unmatched remove filters as errors. Keep optional `lcovRemovePatterns` guarded with `lcov --ignore-errors unused`, and update `tools/check-coverage-thresholds.ps1` when changing coverage filtering.
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
- Start docs from `docs/README.md`; use `docs/roadmap.md` for status, `docs/superpowers/plans/README.md` before creating/editing/extending plans, and `docs/superpowers/plans/2026-05-11-production-documentation-stack-v1.md` to reconcile manifest pointers, master plan, specs/ADRs, and historical dated plans without bulk-rewriting ledger prose.
- Use `docs/specs/` for feature designs and `docs/superpowers/plans/` for implementation plans. Write new or updated implementation plans in English.
- **Dated plan and spec files:** use `YYYY-MM-DD` in filenames and first headings from the session `Today's date`, explicit operator date, or local `Get-Date -Format yyyy-MM-dd`; keep filename date and heading date identical.
- Keep the live plan stack shallow: one active roadmap, one active gap burn-down or milestone, and at most one active child/phase plan selected by `currentActivePlan`. Create dated focused plans only for distinct production slices with a behavior/API/validation boundary; keep validation-only follow-up, docs/manifest/static-check synchronization, small cleanup, and current-checklist substeps inside the active plan.
- Prefer an active gap burn-down or phase-gated milestone plan over many tiny child plans when work shares one architecture decision and validation surface. Preserve completed plan files as historical implementation evidence; do not append unrelated work to completed plans, broaden ready claims, or weaken host gates.
- Keep active plans concise: put detailed evidence in final validation tables, batch docs/manifest/skills synchronization after behavior is green unless those files are the behavior, and ensure each active phase still has Goal, Context, Constraints, Done When, and validation evidence.
- Before generating game code or changing engine APIs, read `engine/agent/manifest.json`, a targeted `engine/agent/manifest.fragments/` file, or `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`. Prefer `-ContextProfile Minimal` or `-ContextProfile Standard`; use Full only when the decision needs full manifest-shaped output.
- Use `.agents/skills/` when a task matches them, keep overlapping `.claude/skills/` behaviorally equivalent, and keep `.cursor/skills/gameengine-*` folders as thin pointers matching `.claude/skills/` names except the intentional Cursor-only `gameengine-cursor-baseline` and `gameengine-plan-registry`.
- After changing agent surfaces (`tools/*.ps1` dot-source pairs, skill folders, validation scripts, rules, settings, subagents, or manifest fragments), follow **Repository consistency checklist** in `docs/workflows.md` (toolchain -> `check-agents` -> `check-ai-integration` -> public API checks -> `validate.ps1` as appropriate).
- Tracked `.clangd` points at `out/build/dev`; configure `cmake --preset dev` when clangd lacks a database. Use `editor/src/compile_flags.txt` and `editor/include/compile_flags.txt` only as fallback IDE flags. `MK_tools` implementation sources live under `engine/tools/{shader,gltf,asset,scene}/` with public headers in `engine/tools/include/mirakana/tools/`.
- Use project subagents in `.codex/agents/` and `.claude/agents/` only when the user explicitly asks for subagent delegation or parallel agent work; keep reviewer/explorer/architect/auditor roles read-only and write tools limited to builder/fixer roles.
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

- Keep shared ignores in `.gitignore`, repository-local ignores in `.git/info/exclude`, and user-wide editor/backup ignores in readable user `core.excludesFile`. If sandboxed Git reports unreadable global ignore such as `$HOME/.config/git/ignore`, prefer a repo-local `core.excludesFile` in `.git/config` instead of weakening repo permissions.
- If Git author identity is missing, set `user.name` and `user.email` with `git config --local` unless the user explicitly requests global identity.
- Implementation plans should include validated commit checkpoints. Prefer small commits after coherent, passing slices; never commit known-broken work, scratch output, secrets, credential files, or unrelated user changes. For a slice-closing commit, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` then `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` first, or for documentation-only/non-runtime slices record justified narrower checks in the PR body and completion report.
- Commit and push task-owned changes at coherent, validated checkpoints without per-action confirmation. Before committing, inspect `git status --short --branch`, review staged diff, and stage only task-owned files.
- Treat Codex command policy as session-scoped: after editing `.codex/rules/*.rules`, wait for policy reload or a new session when newly allowed commands still require a prompt and approvals are unavailable; do not retry by weakening rules.
- Prefer topic branches plus GitHub pull requests. Do not push directly to default/protected branches, force-push without explicit branch-owned request, or bypass GitHub branch protection/status checks.
- For GitHub publishing, push the current topic branch, then create/update a PR through GitHub Web, `gh`, or GitHub Desktop with concise title/body and actual validation evidence. In unattended Codex sessions, `gh pr create` may run after validation checkpoints. Before `gh pr merge --auto --merge --delete-branch`, inspect `gh pr view <pr> --json state,isDraft,baseRefName,headRefName,headRefOid,mergeable,mergeStateStatus,reviewDecision,statusCheckRollup,autoMergeRequest,url`; proceed only for an open, non-draft, task-owned PR with fresh validation, expected base, no failing/cancelled/timed-out/action-required checks, no requested changes, and a safe merge state. Prefer `--match-head-commit <headRefOid>`. If status is `UNSTABLE`, `DIRTY`, `UNKNOWN`, auto-merge is unavailable, or the command would merge pending/failing work without satisfied requirements, report the blocker instead of weakening policy or merging unattended.
- PR state changes through `gh pr edit`, immediate `gh pr merge` without `--auto`, `gh pr ready`, `gh pr close`, or `gh pr reopen` remain prompt-gated. If approvals are unavailable, keep the branch pushed and use GitHub Web/Desktop or an approval-capable session.
- If Git prints credential helper warnings such as `credential-manager-core` missing, inspect `git config --show-origin --get-all credential.helper` and fix host/user Git configuration. On Git for Windows, prefer current Git Credential Manager helper `manager`; do not add repository credential helper overrides or tokens.

## AI Tool Integration

- Codex reads `AGENTS.md`, `.agents/skills/`, `.codex/agents/`, and `.codex/rules/`; Claude Code reads `CLAUDE.md`, `.claude/settings.json`, `.claude/skills/`, and `.claude/agents/`; Cursor loads workspace rules and Agent Skills from `.cursor/rules/` and `.cursor/skills/`. Treat `AGENTS.md` as the authoritative baseline and keep Cursor-only skills short pointers.
- Codex project escalation policy lives in `.codex/rules/`. Keep rules narrow, covered by `match` / `not_match` examples, and aligned with **PowerShell 7** (`pwsh`) for `tools/*.ps1`; do not add broad allow rules for shells, package managers, network tools, destructive commands, direct default-branch pushes, force-pushes, or immediate PR merges.
- Claude Code shared permissions live in `.claude/settings.json` with the official JSON schema. Deny secret-bearing files; allow read-only PR preflight, task-owned PR creation, and safe auto-merge registration after validation checkpoints; keep destructive, network, dependency-bootstrap, mobile signing/smoke, force-push, direct default-branch push, immediate PR merge, and other PR state-change commands behind approval. Keep `.claude/settings.local.json`, `.mcp.json`, and `AGENTS.override.md` uncommitted unless governance intentionally changes that.
- Use Context7 MCP for library, SDK, build-system, and toolchain docs such as CMake, vcpkg, SDL3, Dear ImGui, Direct3D 12, Vulkan, Metal, and C++ tooling. Use the OpenAI developer documentation MCP, or official OpenAI documentation when MCP is unavailable, for OpenAI API, Codex, ChatGPT Apps SDK, OpenAI agent, and OpenAI model questions. Use official Anthropic documentation for Claude Code memory, settings, permissions, hooks, skills, and subagent behavior. Keep MCP keys and connector state user-local.
- Keep Codex/Claude/Cursor overlapping instructions behaviorally equivalent. Do not change one Codex/Claude skill pair without updating shared docs or validation checks; update `.cursor/skills/` when the workflow also applies inside Cursor.
- The engine-facing AI integration contract is composed `engine/agent/manifest.json`; **do not hand-edit** it. Edit `engine/agent/manifest.fragments/` and run `tools/compose-agent-manifest.ps1 -Write`. Keep runtime backend readiness, importer capabilities, packaging targets, validation recipes, and JSON Schema entrypoint `schemas/engine-agent.schema.json` honest and machine-readable.
- When adding/renaming retained editor UI row ids such as `play_in_editor.*`, CMake target names, or other AI-contract literals enforced in `tools/check-ai-integration.ps1`, extend the relevant Needles and keep `.agents/skills/editor-change/SKILL.md`, `.claude/skills/gameengine-editor/SKILL.md`, and manifest checks aligned.
- When engine work reveals stale agent guidance, update the relevant `AGENTS.md`, `CLAUDE.md`, docs, Codex/Claude/Cursor skills, rules, settings, subagents, manifests, schemas, validation checks, and tracked `.clangd` when compile-database defaults change in the same task.
- Keep reviewer, explorer, architect, and auditor subagents read-only by default in both `.codex/agents/` and `.claude/agents/`; give write-capable tools only to builder/fixer roles expected to change files. Keep always-loaded instructions concise; move reusable procedures into skills and specialized review/build/debug behavior into subagents.

## AI-Driven Game Development

- Games, manifests, scaffolding, desktop runtime, and Android lanes: see [docs/agent-operational-reference.md](docs/agent-operational-reference.md#ai-driven-game-development-expanded).
- Games live under `games/<game_name>/`; `game_name` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/new-game.ps1 -Name <game_name>` values must match `^[a-z][a-z0-9_]*$`. Keep source-tree game directories and `runtimePackageFiles` path segments lowercase snake_case; JSON manifest IDs, display names, and external package identifiers may use ecosystem formats such as kebab-case.
- Every game must include `game.agent.json` with backend readiness, importer requirements, packaging targets, runtime package files, runtime scene validation targets, and validation recipes.
- Most sample games are headless validation executables. `sample_desktop_runtime_shell` and `sample_desktop_runtime_game` are the optional windowed SDL3 desktop runtime samples; do not promise a visible game window outside the desktop-runtime lane unless a runtime windowed host is implemented. Use `mirakana_editor` for the broader visible desktop/editor smoke path.
- Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/new-game.ps1 -Name <game_name>` to scaffold a C++ game. Shared helpers live in `tools/new-game-helpers.ps1`; update them with `new-game.ps1` when generated formatting or string literals change.
- Register default source-tree games with `MK_add_game` and optional windowed desktop runtime games with `MK_add_desktop_runtime_game` in `games/CMakeLists.txt`.
- For `desktop-game-runtime`, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`; use `tools/package-desktop-runtime.ps1 -GameTarget <target>` for selected registered package targets. Registered desktop runtime game targets must declare source-tree smoke args, package smoke args, shader-artifact requirements, and package files through CMake metadata; keep `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1` as the default sample-shell package lane.
- Runtime package files are hashed byte-for-byte. Any new text cooked/runtime extension included in `runtimePackageFiles` must have matching `runtime/.gitattributes` `text eol=lf` coverage in sample/scaffold output and static checks before package smoke evidence is trusted.
- For `android-gameactivity`, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-mobile-packaging.ps1` first. If Android is ready, validate Debug packages with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-mobile-android.ps1 -Game <game_name> -Configuration Debug`, Release signing with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-android-release-package.ps1 -Game <game_name> -UseLocalValidationKey`, and emulator launch with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/smoke-android-package.ps1 -Game <game_name> -Configuration Release -SkipBuild -StartEmulator -AvdName Mirakanai_API36` when the configured AVD is available. Do not store Android keystores, passwords, SDKs, Gradle caches, or AVD images in the repo.
- Game code should include only public engine headers unless implementing engine internals.

## Done Definition

- Relevant docs/specs/plans are updated.
- Relevant tests are added or updated when behavior/API/regression risk changed, and they cover the smallest durable external guarantee.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` has been run, or a concrete environment blocker is recorded.
- Legal and third-party records are updated for any external material.
