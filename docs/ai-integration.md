# AI Agent Integration

## Quick path (machine contract)

1. **Engine manifest SSOT:** edit `engine/agent/manifest.fragments/*.json`, then run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` to refresh `engine/agent/manifest.json`. CI validates with `compose-agent-manifest.ps1 -Verify` (via `tools/check-json-contracts.ps1`).
2. **Agent context size:** `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` defaults to `-ContextProfile Full` (prior behavior). Use `-ContextProfile Standard` to truncate long `modules[].purpose` text, or `-ContextProfile Minimal` for a slim `manifest` object (recipes, command-surface ids, module name/path/status only). Full module headers remain in `publicHeaders` / `moduleOwnership`.
3. **Always-loaded instruction budget:** Codex's documented default `project_doc_max_bytes` for `AGENTS.md` is 32 KiB. Keep root `AGENTS.md` below that cap, keep selected `SKILL.md` files as concise routers, and keep subagents narrowly scoped; `tools/check-agents.ps1` enforces the repository budgets. Move detail to skill-local `references/*.md`, docs, manifest fragments, or this doc.
4. **Schemas:** `schemas/engine-agent.schema.json` (with `schemas/engine-agent/ai-operable-production-loop.schema.json` via `$ref`) and `schemas/game-agent.schema.json` describe manifest shapes; see `tools/check-json-contracts.ps1` for enforced fields.
5. **ADR:** `docs/adr/0002-agent-manifest-fragments-compose.md` records the fragment + compose decision.
6. **Repository layout (`MK_tools`):** `docs/adr/0003-directory-layout-clean-break.md` and `docs/specs/2026-05-11-directory-layout-target-v1.md` define the target tree. `MK_tools` implementation `.cpp` files live under `engine/tools/{shader,gltf,asset,scene}/`; public headers stay `engine/tools/include/mirakana/tools/*.hpp`. If you move tool sources, update path-based Needles in `tools/check-json-contracts.ps1` and `tools/check-ai-integration.ps1` (and any CMake fragment needles such as `engine/tools/gltf/CMakeLists.txt`) in the same task.

GameEngine is designed so Codex, Claude Code, and similar AI coding agents can discover the same project rules, engine API surface, and validation commands.

## Shared Contract

The machine-readable integration contract is the **canonical** file:

```text
engine/agent/manifest.json
```

It is **generated only** from `engine/agent/manifest.fragments/` using `tools/compose-agent-manifest.ps1` (see Quick path above); do not hand-edit it.

Agents should read it directly or run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1 [-ContextProfile Full|Standard|Minimal]
```

The generated context includes the engine manifest, public headers, module ownership, sample game manifests, asset/importer capabilities, platform packaging targets, docs, skills, subagents, and validation commands.
It also exposes top-level `productionLoop`, `productionRecipes`, `aiCommandSurfaces`, `unsupportedProductionGaps`, `hostGates`, and `recommendedNextPlan` fields so agents can choose a supported recipe without scraping prose. The manifest keeps host diagnostics guidance machine-readable through `windowsDiagnosticsToolchain`: Debugging Tools for Windows, Windows Graphics Tools, PIX on Windows, and Windows Performance Toolkit are official host diagnostics, not default build dependencies.

AI Command Surface Foundation v1 makes `aiCommandSurfaces` a descriptor contract. Each row declares `schemaVersion`, `requestModes`, `requestShape`, `resultShape`, `requiredModules`, `capabilityGates`, `hostGates`, `validationRecipes`, `unsupportedGapIds`, and a placeholder `undoToken`. Agents use these rows to choose a supported dry-run/apply/execute surface and to report diagnostics with explicit unsupported gap ids. Ready apply surfaces remain narrow package/data mutations such as `register-runtime-package-files`, `register-source-asset`, `cook-registered-source-assets`, `migrate-scene-v2-runtime-package`, `update-ui-atlas-metadata-package`, `create-material-instance`, and `update-scene-package`, while `run-validation-recipe` is a ready non-mutating validation execution surface for allowlisted validation recipes through `tools/run-validation-recipe.ps1`. `validate-runtime-scene-package` is the ready non-mutating runtime scene package validation surface through `mirakana::plan_runtime_scene_package_validation` and `mirakana::execute_runtime_scene_package_validation`; it validates an explicit `.geindex` package load and `mirakana_runtime_scene` instantiation diagnostics without package cooking, runtime source parsing, renderer/RHI residency, package streaming, editor productization, Metal, public native/RHI handles, arbitrary shell, or free-form edits. The validated authored-to-runtime workflow is `register-source-asset -> cook-registered-source-assets -> migrate-scene-v2-runtime-package -> mirakana::runtime::load_runtime_asset_package -> mirakana::runtime_scene::instantiate_runtime_scene`; broader renderer-facing or desktop package proof remains on separate recipes. The runner does not evaluate arbitrary shell or raw manifest command strings, and free-form validation commands are unsupported.

## AI-driven workflows and Windows GPU diagnostics

- `MK_editor` Resources tooling stays **review + evidence only**: capture **requests** and **execution evidence** rows are not PIX launch, D3D12 debug-layer toggles, ETW capture, or GPU capture execution inside editor core. Agents must not widen ready claims or tell operators that the editor already ran PIX.
- When a task needs D3D12 GPU capture, GPU timings, or counter investigations on Windows, treat **PIX on Windows** as an **operator-installed host diagnostic** aligned with `AGENTS.md` and manifest `windowsDiagnosticsToolchain`. After the operator reviews capture rows in the shell, they may run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/launch-pix-host-helper.ps1` from the repository root (see `docs/superpowers/plans/2026-05-11-editor-resource-capture-pix-launch-helper-v1.md` and `docs/workflows.md` § Windows diagnostics toolchain).
- Installing PIX is a **host action** (for example `winget install --id Microsoft.PIX -e` with local approval for UAC/EULA). Agents should **propose** installation steps and record **host-gated** or **missing-tool** blockers when captures are required but PIX is absent; do not silently assume PIX is installed in CI or on every developer machine.

### Recommended workflow (operator PIX, AI analysis)

This is the **default** collaboration pattern when GPU capture evidence helps: **PIX runs on the operator Windows host; coding agents analyze pasted or attached evidence and engine code—they do not claim unattended GPU capture inside `MK_editor` or CI.**

1. **Preconditions (agent + operator):** Confirm the PIX CLI is on PATH where needed (`pixtool --help`). If PIX is missing, the agent proposes host install steps (for example `winget install --id Microsoft.PIX -e`) and records a **host-gated** or **missing-tool** blocker when capture evidence was required for the task.
2. **Optional path check:** `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/launch-pix-host-helper.ps1 -SkipLaunch` prints the resolved `WinPix.exe` (or legacy `PIX.exe`), `%LocalAppData%` session scratch, and exits without starting the UI. Use `MIRAKANA_PIX_EXE` when installs live outside default locations (see `AGENTS.md` Testing).
3. **Launch PIX UI (operator):** Run the same script **without** `-SkipLaunch` when the operator wants Microsoft PIX opened from the scratch working directory (host-only; not invoked by `MK_editor`).
4. **Capture (operator):** In PIX, attach to the target process, take GPU capture / timing / counters as the task requires, and save or export artifacts locally.
5. **Analysis (agent):** The operator pastes **bounded** summaries, HRESULTs, log lines, or descriptions into the agent session (or attaches exports per host limits). The agent reasons about **code, RHI usage, and reproduction steps** from that evidence; it does not assert that the editor or automation already ran PIX unless the operator supplied that evidence.
6. **Editor handoff:** Keep using Resources panel **capture request** and **execution evidence** rows for structured review only; they remain metadata aligned with `resources.capture_requests` / `resources.capture_execution`, not automated PIX execution.

Coding agents should **bundle** steps 1–3 when suggesting Windows GPU work (CLI check + optional `-SkipLaunch` + operator launch), then wait for step 5 inputs before deep capture-specific conclusions.

Start human-readable documentation from:

```text
docs/README.md
```

Use `docs/current-capabilities.md` for a concise human-readable summary, `docs/roadmap.md` for current status and priorities, and `docs/superpowers/plans/README.md` as the implementation plan registry. Keep the live plan stack shallow: one active roadmap, one active gap burn-down or milestone, and at most one active child/phase plan selected by `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`. New production work should get a new dated focused plan only when it has its own behavior/API/validation boundary; validation-only follow-up, docs/manifest/static-check synchronization, small cleanup, and substeps that fit the current active checklist should stay inside the active gap burn-down or child plan instead of creating more plan files. Completed plans are historical implementation evidence, so preserve them and reduce navigation noise through the registry or evidence indexes rather than deleting traceability.

Agents that change public C++ APIs, source layout, CMake targets, or game scaffolding must also read:

```text
docs/cpp-style.md
```

## Cursor vs Claude Code hooks (Superpowers)

Some operator environments inject Superpowers guidance that references a Claude Code **Skill** tool. **Cursor does not expose that tool.** In Cursor, load a skill by reading its `SKILL.md` with the editor **Read** tool (or open the file) before relying on its workflow—see [`.cursor/skills/gameengine-cursor-baseline/SKILL.md`](../.cursor/skills/gameengine-cursor-baseline/SKILL.md) for the canonical Cursor-first discipline.

## Instruction Hygiene

Keep always-loaded instructions specific, concise, verifiable, and durable. `AGENTS.md` is for repository-wide standards, must stay below the official Codex default 32 KiB project-doc budget, and is checked by `tools/check-agents.ps1`; `CLAUDE.md` imports it for Claude Code parity. Keep `SKILL.md` files as trigger/router guidance and move long procedures to skill-local `references/*.md` or docs (including [`docs/agent-operational-reference.md`](agent-operational-reference.md)); `tools/check-ai-integration.ps1` reads those references when validating shared needles. Path-specific guidance belongs in rules, specialized work belongs in subagents, and capability/status claims belong in `engine/agent/manifest.json`. Personal preferences, credentials, API keys, MCP connection state, and machine-specific paths must stay in user/local configuration rather than tracked instructions.

## Codex

Codex reads:

- `AGENTS.md`
- `.agents/skills/`
- `.codex/agents/`
- `.codex/rules/`

Use Codex project skills for repeatable workflows; keep skill descriptions precise and `SKILL.md` bodies short because selected skills consume context. Put deep workflow detail in skill-local references that agents load only when needed. Use custom agents for focused parallel work only when delegation is explicitly requested, because each subagent adds a separate context and token cost.

Use Codex app Worktree/Handoff for Codex parallel write threads before falling back to manual `git worktree`. Codex worktrees keep Local as the foreground checkout, support background work, and can later be handed off to Local for inspection. Keep `.worktrees/` ignored for manual fallback worktrees; cleanup goes through `git worktree remove` / `git worktree prune`, not ad hoc directory deletion.

Project-local Codex rules are intentionally narrow. They should cover commands that often need local trust, network access, user caches, signing state, destructive review, or repository history rewriting, including Windows PowerShell deletion/network/host-servicing commands, forced Git pushes, direct default-branch pushes, and PR state changes through `gh pr`. Agent publishing follows official GitHub Flow: routine `git commit`, non-forced topic-branch `git push`, task-owned `gh pr create`, and `gh pr merge --auto --merge --delete-branch` are allowed only after validation checkpoints and the PR preflight in `docs/workflows.md` confirms `state`, `isDraft`, `baseRefName`, `headRefOid`, `mergeable`, `mergeStateStatus`, `reviewDecision`, and `statusCheckRollup` values, allowing pending-only `UNSTABLE` / `BLOCKED` states for GitHub auto-merge. Prefer `--match-head-commit <headRefOid>` for auto-merge registration. Every rule should include `match` / `not_match` examples. Do not add broad allow rules for shells, package managers, network tools, destructive commands, force-pushes, or immediate PR merge shortcuts. Direct default-branch pushes are blocked by project policy and should also be blocked by repository branch protection. Treat command policy as session-scoped: `.codex/rules` edits may need policy reload or a new session before newly allowed commands are available. If prompt-gated PR state changes such as `gh pr edit` or immediate `gh pr merge` are blocked, keep the branch pushed and hand off to GitHub Web/Desktop or an approval-capable session.

When a hosted PR check fails, bind triage to the latest `headRefOid` and `statusCheckRollup`, inspect the failing job logs for that head, reproduce the narrowest local lane, and add or extend static checks if the failure exposed a drift-prone repository contract. If all jobs fail before checkout with a GitHub account billing/spending-limit annotation, report it as a hosted account blocker. Keep the diagnostic workflow in `AGENTS.md`, skills, docs, and subagents; `.codex/rules` and `.claude/settings.json` remain command/permission gates.

Git/GitHub authentication remains host-local. Agents rely on the operator's configured Git Credential Manager, GitHub CLI, SSH agent, or browser session and must not add repository requirements for `GITHUB_TOKEN`, personal access tokens, or checked-in credential helper state. If warnings mention missing helpers such as `credential-manager-core`, inspect `git config --show-origin --get-all credential.helper`, prefer current Git for Windows GCM helper `manager`, and fix host/user Git config rather than adding repository overrides.

Read-only review, exploration, architecture, and rendering-audit agents must set `sandbox_mode = "read-only"` so their tool surface matches their purpose. Builder and fixer agents may keep write-capable permissions when their role is expected to edit files. Codex subagents should be spawned only when the user explicitly asks for subagent delegation or parallel agent work, and their instructions should stay short enough to avoid unnecessary delegated-context cost.

## Claude Code

Claude Code reads:

- `CLAUDE.md`
- `.claude/settings.json`
- `.claude/rules/`
- `.claude/skills/`
- `.claude/agents/`

`CLAUDE.md` imports `AGENTS.md` and the project rule files with official memory imports so the baseline instructions stay shared. `.claude/settings.json` keeps shared permissions in the official settings surface with the published JSON schema: secret-bearing files and direct default-branch pushes are denied; destructive, network, dependency-bootstrap, mobile signing/smoke, force-push, and non-auto PR state changes require approval; read-only `gh pr view`, task-owned `gh pr create`, and `gh pr merge --auto --merge --delete-branch` can run after validation checkpoints only when the PR preflight in `docs/workflows.md` satisfies the official GitHub Flow path. Routine commits and non-forced topic-branch pushes rely on `AGENTS.md` / `docs/workflows.md` validation checkpoints instead of per-action prompts. If a gated PR state change needs approval that is unavailable, use GitHub Web/Desktop or an approval-capable session instead of loosening permissions. Keep `.claude/settings.local.json`, `.mcp.json`, and `AGENTS.override.md` out of source control unless project governance explicitly accepts a shared override.

Read-only review, exploration, architecture, and rendering-audit subagents must declare read-only tools in frontmatter. Builder and fixer subagents may keep write-capable tools when their role is expected to edit files. Keep Claude Code skill and subagent bodies aligned with the same context-budget pattern used by Codex: concise `SKILL.md` entrypoints, references for deep procedures, and short role-specific subagent instructions.

Claude Code parallel write sessions should use `--worktree`, and write-capable project subagents should use `isolation: worktree` so edits do not collide with the parent checkout. The shared settings use `worktree.baseRef = "head"` so isolated subagents start from the current branch state; `.claude/worktrees/` is ignored, and `.worktreeinclude` remains operator-local guidance for copied gitignored files.

## Cursor

Cursor loads workspace rules (including imports of `AGENTS.md`) and Cursor Agent Skills under `.cursor/skills/` when enabled.

- **Cursor-only summaries:** `gameengine-cursor-baseline` (PowerShell wrappers, validation shorthand, manifest alignment) and `gameengine-plan-registry` (dated plans under `docs/superpowers/plans/`).
- **Thin pointers:** each `.cursor/skills/gameengine-*` folder (except the two above) must match a `.claude/skills/gameengine-*` name and point readers at the Claude/Codex skill routers and detailed references—`gameengine-agent-integration`, `gameengine-cmake-build-system`, `gameengine-debugging`, `gameengine-editor`, `gameengine-feature`, `gameengine-game-development`, `gameengine-license-audit`, `gameengine-rendering`.

For this repository, `AGENTS.md` Git Workflow is the intended workspace policy. If Cursor global instructions conflict with it, such as a blanket "do not commit until explicitly requested" rule, align the global instruction or add a workspace override before relying on automation.

`tools/check-agents.ps1` validates Cursor skill frontmatter and enforces thin-pointer names against Claude. New shared topics require updating `claudeToCodexSkillMap` in that script; see **Repository consistency checklist** in `docs/workflows.md`.

Tracked `.clangd` sets `CompileFlags.CompilationDatabase` to `out/build/dev`; run `cmake --preset dev` first so clangd and editors resolve `mirakana/...` includes.

## Surface Parity

The required project-level AI surfaces are:

- Codex skills: `cmake-build-system`, `cpp-engine-debugging`, `editor-change`, `gameengine-agent-integration`, `gameengine-feature`, `gameengine-game-development`, `license-audit`, and `rendering-change`.
- Claude Code skills: `gameengine-agent-integration`, `gameengine-cmake-build-system`, `gameengine-debugging`, `gameengine-editor`, `gameengine-feature`, `gameengine-game-development`, `gameengine-license-audit`, and `gameengine-rendering`.
- Cursor Agent Skills (repository-local): Cursor-only `gameengine-cursor-baseline` and `gameengine-plan-registry`, plus thin pointers for every Claude `gameengine-*` topic listed above (same folder names under `.cursor/skills/`).
- Shared subagent roles: `explorer`, `cpp-reviewer`, `engine-architect`, `rendering-auditor`, `gameplay-builder`, and `build-fixer`.

`explorer`, `cpp-reviewer`, `engine-architect`, and `rendering-auditor` are read-only roles. `gameplay-builder` and `build-fixer` may edit only when their delegated task expects production work or a targeted fix.

## Required Checks

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

`agent-check` validates the agent manifest, Claude/Codex instruction surfaces, skills, and custom agents.
When you add retained editor UI ids (`play_in_editor.*`), manifest slices, or editor-shell literals enforced by `tools/check-ai-integration.ps1`, extend the matching **Needles** in that script and keep Codex/Claude editor skills aligned in the same change (see **Repository consistency checklist** in `docs/workflows.md`).
When you edit any `tools/*.ps1` (especially `check-ai-integration.ps1` or `common.ps1`), follow **`AGENTS.md` → Repository command entrypoints** for **PSScriptAnalyzer-friendly** PowerShell patterns and run **`Invoke-ScriptAnalyzer`** on touched files when the module is installed.
`schema-check` validates engine and game agent JSON contracts with repository-local rules.
`toolchain-check` also enforces that checked-in CMake build presets keep inheriting `normalized-build-environment`, and `agent-check` verifies the matching guidance remains present in AGENTS, workflow docs, CMake skills, Claude rules, build-fixer subagents, and the engine manifest so Windows MSBuild `Path`/`PATH` environment regressions are not reintroduced silently.

Engine and game manifests must declare backend readiness, importer requirements, packaging targets, and validation recipes so generated-game agents can avoid planned or dependency-gated capabilities unless they are explicitly implementing them.
The engine manifest also declares `aiOperableProductionLoop`: recipe ids, structured AI command descriptor surfaces, authoring/package surfaces, unsupported production gaps, host gates, and validation recipe mapping. Static checks reject stale claims such as command surfaces using legacy `inputContract`/`outputContract`/`dryRun` fields instead of descriptor fields, future 2D/3D recipes becoming `ready`, Vulkan/Metal host gates being removed, prompt/scenario docs losing the enforced recipe ids, `register-source-asset` losing its reviewed `GameEngine.SourceAssetRegistry.v1` helper contract or claiming importer execution/cooked artifact writes/`.geindex` updates/renderer residency, `cook-registered-source-assets` losing its explicit selected-row helper contract or expanding into unreviewed dependency traversal, renderer residency, streaming, graph, Metal, native-handle, shell, or free-form edit claims, `3d-playable-desktop-package` claiming native GPU UI overlay or production renderer quality without the separate `native-gpu-runtime-ui-overlay` recipe, the colored overlay recipe claiming production text/font/image/atlas/accessibility, the `native-ui-textured-sprite-atlas` recipe claiming source image decoding or production atlas packing, the `native-ui-atlas-package-metadata` recipe claiming runtime source PNG/JPEG image decoding or production atlas packing, Scene/Component/Prefab Schema v2 being described as more than a contract-only `mirakana_scene` surface before editor/runtime/package migrations land, Asset Identity v2 losing its closed identity/reference boundary evidence (`plan_asset_identity_placements_v2`, `audit_runtime_scene_asset_identity`, key-first games/templates) or claiming renderer/RHI residency, package streaming, runtime source-registry parsing, native handles, or 2D/3D vertical slices, Runtime Resource v2 being described as more than a foundation-only contract before renderer/RHI residency and package streaming land, or Frame Graph and Upload/Staging Foundation v1 being described as more than foundation-only `FrameGraphV1Desc` / `RhiUploadStagingPlan` planning before native GPU uploads, production render graph scheduling, allocator/residency budgets, package streaming, and 2D/3D vertical slices land.

## Live Documentation Policy

- Use Context7 MCP for live library, SDK, build-system, and toolchain documentation such as CMake, vcpkg, SDL3, Dear ImGui, Direct3D 12, Vulkan, Metal, and C++ tooling.
- Use the OpenAI developer documentation MCP, or official OpenAI documentation when MCP is unavailable, for OpenAI API, Codex, ChatGPT Apps SDK, OpenAI agent, and OpenAI model questions.
- Use official Anthropic documentation for Claude Code memory, settings, permissions, hooks, skills, and subagent behavior.
- Use official Microsoft documentation for Windows SDK Debugging Tools, Graphics Tools / D3D12 debug layer, PIX on Windows, Windows Performance Toolkit, and ADK servicing guidance. Keep those tools host-local and do not commit installer state, symbol caches, traces, or personal PATH settings.
- Keep MCP API keys, personal connector state, and local tool credentials in user-level configuration only. Do not commit them.
- Keep local override files uncommitted: `.claude/settings.local.json`, `.mcp.json`, and `AGENTS.override.md`.

## Official References

- Codex AGENTS.md: https://developers.openai.com/codex/guides/agents-md
- Codex rules: https://developers.openai.com/codex/rules
- Codex skills: https://developers.openai.com/codex/skills
- Codex subagents: https://developers.openai.com/codex/subagents
- OpenAI developer docs MCP: https://platform.openai.com/docs/docs-mcp
- Claude Code memory: https://docs.anthropic.com/en/docs/claude-code/memory
- Claude Code settings and permissions: https://docs.anthropic.com/en/docs/claude-code/settings
- Claude Code hooks: https://docs.anthropic.com/en/docs/claude-code/hooks
- Claude Code subagents: https://docs.anthropic.com/en/docs/claude-code/sub-agents
- Cursor rules: https://cursor.com/docs/rules
- Cursor Agent Skills: https://cursor.com/docs/skills
- PowerShell approved verbs: https://learn.microsoft.com/en-us/powershell/scripting/developer/cmdlet/approved-verbs-for-windows-powershell-commands
- PSScriptAnalyzer ShouldProcess rule: https://learn.microsoft.com/en-us/powershell/utility-modules/psscriptanalyzer/rules/shouldprocess
- PowerShell ShouldProcess guidance: https://learn.microsoft.com/en-us/powershell/scripting/learn/deep-dives/everything-about-shouldprocess
- PowerShell Write-Information: https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.utility/write-information
