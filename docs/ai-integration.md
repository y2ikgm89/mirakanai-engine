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

`engine/agent/manifest.json.aiSurfaces.crossToolAlignment` is the machine-readable cross-tool contract for AI Codex/Claude/Cursor Agent Surface v1. It records official docs anchors for OpenAI Codex `AGENTS.md`, Claude Code settings/permissions/subagents, and Cursor rules/AGENTS.md; game-owned write scopes such as `games/<game_name>/`; shared-surface write policy; reviewed command surfaces; validation guards; required read-only roles; forbidden broad grants; and unsupported claims.

Generated-game agents may write game-owned roots by default. Engine, editor, renderer, tools, schemas, docs, rules, skills, subagents, and manifest fragments require a developer-owned capability task, isolated topic branch, reviewed `tools/*.ps1` entrypoints, and PR validation evidence. The reviewed command set includes `tools/create-game-recipe.ps1`, `tools/run-validation-recipe.ps1`, `tools/ready-task-pr.ps1`, `tools/post-merge-task-cleanup.ps1`, `tools/remove-merged-worktree.ps1`, `gh pr create`, `gh pr view`, and `gh pr merge --auto --merge --match-head-commit <headRefOid>`.

The contract intentionally forbids direct default-branch push, force push without explicit branch-owned request, immediate PR merge, raw `gh pr ready`, raw worktree cleanup, arbitrary shell or raw manifest command evaluation, destructive cleanup outside guarded scripts, and credential or secret storage in the repository. It also keeps backend/native handle public game API exposure, renderer/RHI public game API leakage, validation weakening, broad autonomous commercial game creation, and broad production readiness unsupported.

AI Command Surface Foundation v1 makes `aiCommandSurfaces` a descriptor contract. Each row declares `schemaVersion`, `requestModes`, `requestShape`, `resultShape`, `requiredModules`, `capabilityGates`, `hostGates`, `validationRecipes`, `unsupportedGapIds`, and a placeholder `undoToken`. Agents use these rows to choose a supported dry-run/apply/execute surface and to report diagnostics with explicit unsupported gap ids; `unsupportedGapIds` may be empty only when no current `unsupportedProductionGaps` row blocks the narrow reviewed surface. Ready apply surfaces remain narrow package/data mutations such as `register-runtime-package-files`, `register-source-asset`, `cook-registered-source-assets`, `migrate-scene-v2-runtime-package`, `refresh-prefab-instance`, `update-ui-atlas-metadata-package`, `create-material-instance`, and `update-scene-package`, while `run-validation-recipe` is a ready non-mutating validation execution surface for allowlisted validation recipes through `tools/run-validation-recipe.ps1`. `plan_scene_prefab_instance_refresh_v2` is a contract-only `MK_scene` stable-id prefab refresh planner over `PrefabDocumentV2` and `source_node_id` / `source_component_id`, fails closed with `duplicate_prefab_source_identity` diagnostics for duplicate copied source identities, `unsupported_nested_prefab_instance` diagnostics for nested prefab roots, and `unsupported_local_prefab_child` / `unsupported_local_prefab_component` diagnostics for local rows without selected-prefab provenance. `apply_scene_prefab_instance_refresh_v2` returns `ScenePrefabInstanceRefreshResultV2` as a pure value apply over an already reviewed Scene/Prefab v2 document: it can preserve matched ids/state, add deterministic `instance_root/refresh/<source-id>` rows, remove stale sourced rows, and return source-to-result mappings without editor actions, runtime prefab semantics, or nested propagation. The reviewed `refresh-prefab-instance` command surface wraps that value apply through `plan_scene_prefab_authoring` / `apply_scene_prefab_authoring` and writes only the selected authored `.scene` after validation. It does not make editor productization, nested prefab propagation/merge resolution UX, runtime prefab instance semantics, broad package cooking, renderer/RHI residency, package streaming, or native handles ready. `validate-runtime-scene-package` is the ready non-mutating runtime scene package validation surface through `mirakana::plan_runtime_scene_package_validation` and `mirakana::execute_runtime_scene_package_validation`; it validates an explicit `.geindex` package load and `mirakana_runtime_scene` instantiation diagnostics without package cooking, runtime source parsing, renderer/RHI residency, package streaming, editor productization, Metal, public native/RHI handles, arbitrary shell, or free-form edits. The validated authored-to-runtime workflow is `register-source-asset -> cook-registered-source-assets -> migrate-scene-v2-runtime-package -> validate-runtime-scene-package`; broader renderer-facing or desktop package proof remains on separate recipes. The runner does not evaluate arbitrary shell or raw manifest command strings, and free-form validation commands are unsupported.

## AI-driven workflows and Windows GPU diagnostics

- `MK_editor` Resources tooling stays **review + evidence only**: capture **requests** and **execution evidence** rows are not PIX launch, D3D12 debug-layer toggles, ETW capture, or GPU capture execution inside editor core. Agents must not widen ready claims or tell operators that the editor already ran PIX.
- When a task needs D3D12 GPU capture, GPU timings, or counter investigations on Windows, treat **PIX on Windows** as an **operator-installed host diagnostic** aligned with `AGENTS.md` and manifest `windowsDiagnosticsToolchain`. After the operator reviews capture rows in the shell, they may run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/launch-pix-host-helper.ps1` from the repository root; see `docs/workflows.md` § Windows diagnostics toolchain.
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

Use `docs/current-capabilities.md` for a concise human-readable summary, `docs/roadmap.md` for current status and priorities, and `docs/superpowers/plans/README.md` as the implementation plan registry. Keep the live plan stack shallow: one active roadmap, one active gap-cluster burn-down or milestone, and at most one active phase/child plan selected by `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`. New production work should prefer one dated capability/gap-cluster/milestone plan; phase behavior/API/validation boundary decisions, validation-only follow-up, docs/manifest/static-check synchronization, small cleanup, and substeps that fit the current active checklist should stay inside that plan instead of creating more plan files. Plan-file width can be broader than PR/phase width, but each phase and PR should remain one reviewable purpose with focused evidence. Completed plans are retained only while referenced by current docs, manifests, checks, or active decisions; unreferenced historical noise may be removed in explicit cleanup and recovered through Git history.

Agents that change public C++ APIs, source layout, CMake targets, or game scaffolding must also read:

```text
docs/cpp-style.md
```

## Cursor vs Claude Code hooks (Superpowers)

Some operator environments inject Superpowers guidance that references a Claude Code **Skill** tool. **Cursor does not expose that tool.** In Cursor, load a skill by reading its `SKILL.md` with the editor **Read** tool (or open the file) before relying on its workflow—see [`.cursor/skills/gameengine-cursor-baseline/SKILL.md`](../.cursor/skills/gameengine-cursor-baseline/SKILL.md) for the canonical Cursor-first discipline.

## Instruction Hygiene

Keep always-loaded instructions specific, concise, verifiable, and durable. `AGENTS.md` is for repository-wide standards, must stay below the official Codex default 32 KiB project-doc budget, and is checked by `tools/check-agents.ps1`; `CLAUDE.md` imports it for Claude Code parity. Keep `SKILL.md` files as trigger/router guidance and move long procedures to skill-local `references/*.md` or docs (including [`docs/agent-operational-reference.md`](agent-operational-reference.md)); `tools/check-ai-integration.ps1` reads those references when validating shared needles. Path-specific guidance belongs in rules, specialized work belongs in subagents, and capability/status claims belong in `engine/agent/manifest.json`. Personal preferences, credentials, API keys, MCP connection state, and machine-specific paths must stay in user/local configuration rather than tracked instructions.

Every implementation change, improvement, bug fix, refactor, and architecture/toolchain/workflow/validation/packaging change must include an agent-surface drift check before completion. If the change reveals missing or stale durable guidance, update the relevant `AGENTS.md`, `CLAUDE.md`, docs, skills, rules, settings, subagents, manifest fragments plus compose output, schemas, validation checks, and tracked `.clangd` in the same task instead of leaving a follow-up. Keep the result clean-breaking for this greenfield engine unless a future release policy explicitly requires compatibility.

Keep drift checks targeted: compare the changed behavior/API/workflow against the owning docs, skills, rules, subagents, manifest fragments, schemas, and validation guards; do not load every agent surface when the change is local and no durable guidance changed.

## Codex

Codex reads:

- `AGENTS.md`
- `.agents/skills/`
- `.codex/agents/`
- `.codex/rules/`

Use Codex project skills for repeatable workflows; keep skill descriptions precise and `SKILL.md` bodies short because selected skills consume context. Put deep workflow detail in skill-local references that agents load only when needed. Use custom agents for focused parallel work only when delegation is explicitly requested, because each subagent adds a separate context and token cost.

Use Codex app Worktree/Handoff for Codex parallel write threads before falling back to manual `git worktree`. Codex worktrees keep Local as the foreground checkout, support background work, and can later be handed off to Local for inspection. Keep `.worktrees/` ignored for manual fallback worktrees. After entering a manual linked worktree, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1` so ignored worktree roots and the local `external/vcpkg` tool checkout are ready before configure. GitHub `delete_branch_on_merge` is enabled on this repository. After a task-owned PR is merged into `main`, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/post-merge-task-cleanup.ps1 -WorktreePath <path> [-HeadRefOid <headRefOid>]` from Local or another non-target checkout for guarded post-merge local checkout sync plus cleanup (it always passes `-DeleteLocalBranch -DeleteRemoteBranch` to `tools/remove-merged-worktree.ps1`). The script derives allowed standard roots from the script checkout, `-LocalCheckoutPath`, and Git's main worktree porcelain record; it refuses dirty or non-base local checkouts instead of stashing or merging into active feature branches, verifies local and remote branch ancestry before deletion, and keeps any Windows long-path deletion fallback inside the guarded script after merge/clean/root checks without following reparse points. Raw `git worktree remove` / `git worktree prune` stays reviewed; ad hoc directory deletion is not allowed.

Project-local Codex rules are intentionally narrow. They should cover commands that often need local trust, network access, user caches, signing state, destructive review, or repository history rewriting, including Windows PowerShell deletion/network/host-servicing commands, forced Git pushes, direct default-branch pushes, and PR state changes through `gh pr`. Agent publishing follows official GitHub Flow: task-owned validated-phase `git commit`, non-forced topic-branch `git push` after remote-state inspection, one PR per focused capability/gap-cluster/milestone, task-owned `gh pr create` including `--draft`, guarded `tools/ready-task-pr.ps1` draft-to-ready conversion, `gh pr merge --auto --merge --match-head-commit <headRefOid>`, and guarded merged-worktree cleanup are allowed only after validation or merge-evidence checkpoints and the preflights in `docs/workflows.md` confirm publication readiness through `tools/check-publication-preflight.ps1`, plus `state`, `isDraft`, `baseRefName`, `headRefName`, `mergeable`, `mergeStateStatus`, `reviewDecision`, `statusCheckRollup`, `autoMergeRequest`, and `url` values. GitHub repository ruleset `main-pr-gate` protects the default branch with PR-only changes, no bypass actors, latest-main `PR Gate` required status check, branch deletion/non-fast-forward blocks, auto-merge/update-branch enabled, and branch deletion after merge enabled; audit it with `tools/check-github-repository-ruleset.ps1`. Runtime/C++/build/toolchain/public-contract PRs must wait for the latest head's selected hosted checks to complete and for `PR Gate` to report `SUCCESS` before ready conversion or auto-merge registration. Cadence is purpose/checkpoint-based, not commit-count-based: agents push validated checkpoints for remote backup/CI/review/handoff, may push multiple local commits together when they form one reviewable purpose, and inspect or refresh remote state before pushing. Re-run publication and PR preflight after every commit or push because a stale `headRefOid` invalidates the merge decision and any guarded ready decision; after merge, fetch/prune and verify the preflighted head is reachable from `origin/main`; commits pushed after a PR merged need a new PR. Prefer early draft PRs for large or review-sensitive slices, never open PRs per commit or checklist item, and use `tools/post-merge-task-cleanup.ps1` (preferred) or `tools/remove-merged-worktree.ps1` with `-DeleteLocalBranch -DeleteRemoteBranch` for guarded post-merge cleanup after reachability and ancestry checks; Windows long-path fallback stays inside the guarded scripts, not a broad deletion permission. Every rule should include `match` / `not_match` examples. Do not add broad allow rules for shells, package managers, network tools, destructive commands, force-pushes, raw worktree cleanup, raw `gh pr ready`, `gh pr merge --auto --merge --delete-branch`, or immediate PR merge shortcuts. Direct default-branch pushes are blocked by project policy and repository ruleset. Treat command policy as session-scoped: `.codex/rules` edits may need policy reload or a new session before newly allowed commands are available. If guarded ready conversion blocks or prompt-gated PR state changes such as `gh pr edit`, raw `gh pr ready`, or immediate `gh pr merge` are blocked, keep the branch pushed and hand off to GitHub Web/Desktop or an approval-capable session.

When a hosted PR check fails, bind triage to the latest `headRefOid` and `statusCheckRollup`, inspect the failing job logs for that head, reproduce the narrowest local lane, and add or extend static checks if the failure exposed a drift-prone repository contract. If all jobs fail before checkout with a GitHub account billing/spending-limit annotation, report it as a hosted account blocker. Keep the diagnostic workflow in `AGENTS.md`, skills, docs, and subagents; `.codex/rules` and `.claude/settings.json` remain command/permission gates. For PR check selection, keep branch-protection-required checks always-running or aggregate-gated, require `PR Gate` to fail if a selected lane is skipped or missing, and use lightweight static validation for docs/agent/rules/subagent-only PRs instead of unrelated Windows/MSVC, macOS, or full repository clang-tidy lanes. Keep platform host evidence explicit: general runtime/build paths select ordinary build/static lanes, while Linux Vulkan, Metal host evidence, and iOS Metal evidence use their own classifier outputs and guard tests.

Git/GitHub authentication remains host-local. Agents rely on the operator's configured Git Credential Manager, GitHub CLI, SSH agent, or browser session and must not add repository requirements for `GITHUB_TOKEN`, personal access tokens, or checked-in credential helper state. If warnings mention missing helpers such as `credential-manager-core`, inspect `git config --show-origin --get-all credential.helper`, prefer current Git for Windows GCM helper `manager`, and fix host/user Git config rather than adding repository overrides.

Read-only review, exploration, architecture, planning-audit, rendering-audit, and agent-surface-audit agents must set `sandbox_mode = "read-only"` so their tool surface matches their purpose. Builder and fixer agents may keep write-capable permissions when their role is expected to edit files. Codex subagents should be spawned only when the user explicitly asks for subagent delegation or parallel agent work, and their instructions should stay short enough to avoid unnecessary delegated-context cost. Close completed, obsolete, or no-longer-needed Codex subagents promptly after consuming their results; do not wait for the `agents.max_threads` cap to force cleanup, and leave still-useful independent work running. Fixed model defaults are allowed only when the role's cost/quality profile is stable: `explorer` and `agent-surface-auditor` use `gpt-5.4-mini` with medium reasoning, and `cpp-reviewer`, `engine-architect`, `planning-auditor`, and `rendering-auditor` use `gpt-5.5` with high reasoning. Write-capable `build-fixer` and `gameplay-builder` intentionally inherit the parent model unless the parent uses a per-invocation override for the delegated task.

## Claude Code

Claude Code reads:

- `CLAUDE.md`
- `.claude/settings.json`
- `.claude/rules/`
- `.claude/skills/`
- `.claude/agents/`

`CLAUDE.md` imports `AGENTS.md` and the project rule files with official memory imports so the baseline instructions stay shared. `.claude/settings.json` keeps shared permissions in the official settings surface with the published JSON schema: secret-bearing files and direct default-branch pushes are denied; destructive, network, dependency-bootstrap, mobile signing/smoke, force-push, raw worktree cleanup, raw `gh pr ready`, `gh pr merge --auto --merge --delete-branch`, and non-auto PR state changes require approval; read-only `gh pr view`, task-owned `gh pr create` including draft PR creation, guarded `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ready-task-pr.ps1`, `gh pr merge --auto --merge --match-head-commit <headRefOid>`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/post-merge-task-cleanup.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/remove-merged-worktree.ps1` can run after validation/merge evidence only when the preflights in `docs/workflows.md` are satisfied, including clean local checkout fast-forward before cleanup. Routine commits and non-forced topic-branch pushes rely on `AGENTS.md` / `docs/workflows.md` purpose/checkpoint validation instead of per-action prompts: commit complete validated phases, push validated checkpoints for backup/CI/review/handoff, and keep one PR per focused capability/gap-cluster/milestone unless the work is unrelated. Agents inspect or refresh remote state before pushing, use draft PRs for early feedback on large slices, and keep raw ready/merge state changes behind the existing approval gates. If guarded ready conversion blocks or a gated PR state change needs approval that is unavailable, use GitHub Web/Desktop or an approval-capable session instead of loosening permissions. Keep `.claude/settings.local.json`, `.mcp.json`, and `AGENTS.override.md` out of source control unless project governance explicitly accepts a shared override.

Read-only review, exploration, architecture, planning-audit, rendering-audit, and agent-surface-audit subagents must declare read-only tools or `permissionMode: plan` in frontmatter. Builder and fixer subagents may keep write-capable tools when their role is expected to edit files. Keep Claude Code skill and subagent bodies aligned with the same context-budget pattern used by Codex: concise `SKILL.md` entrypoints, references for deep procedures, and short role-specific subagent instructions. Prefer per-invocation `model` overrides for cost control; only put a model in durable frontmatter when the project intentionally accepts that fixed routing.

Claude Code parallel write sessions should use `--worktree`, and write-capable project subagents should use `isolation: worktree` so edits do not collide with the parent checkout. The shared settings use `worktree.baseRef = "head"` so isolated subagents start from the current branch state; `.claude/worktrees/` is ignored, and `.worktreeinclude` remains operator-local guidance for copied gitignored files.

## Cursor

Cursor loads root `AGENTS.md`, workspace rules under `.cursor/rules/`, Agent Skills under `.cursor/skills/`, and project subagents under `.cursor/agents/` when enabled. Cursor rules stay focused, actionable, scoped, and under the official 500-line rule guidance; root `AGENTS.md` remains a plain Markdown global baseline, while `.cursor/rules/*.mdc` owns scoped metadata. This repository treats `.cursor/agents/*.md` as the authoritative Cursor subagent surface; `.claude/agents/` and `.codex/agents/` are sibling surfaces for their own tools, not Cursor fallback roots.

- **Cursor-only summaries:** `gameengine-cursor-baseline` (PowerShell wrappers, validation shorthand, manifest alignment) and `gameengine-plan-registry` (dated plans under `docs/superpowers/plans/`).
- **Thin pointers:** each `.cursor/skills/gameengine-*` folder (except the two above) must match a `.claude/skills/gameengine-*` name and point readers at the Claude/Codex skill routers and detailed references—`gameengine-agent-integration`, `gameengine-cmake-build-system`, `gameengine-debugging`, `gameengine-editor`, `gameengine-feature`, `gameengine-game-development`, `gameengine-license-audit`, `gameengine-performance-optimization`, `gameengine-rendering`.
- **Project subagents:** `.cursor/agents/` mirrors the shared roles `agent-surface-auditor`, `explorer`, `cpp-reviewer`, `engine-architect`, `planning-auditor`, `rendering-auditor`, `gameplay-builder`, and `build-fixer`. Read-only review/audit/exploration roles declare `readonly: true`; all Cursor project subagents declare **`model: composer-2.5-fast`** using Cursor's documented subagent `model` frontmatter field (parent Task delegations use the same default unless the operator overrides), and write-capable builder/fixer roles must stay in the delegated scope. If Cursor rejects the model slug in a future release, verify the replacement in Cursor's model picker or API and update all Cursor agents, docs, and static guards together.

For this repository, `AGENTS.md` Git Workflow is the intended workspace policy. If Cursor global instructions conflict with it, such as a blanket "do not commit until explicitly requested" rule, align the global instruction or add a workspace override before relying on automation.

`tools/check-agents.ps1` validates Cursor skill frontmatter, Cursor project subagent frontmatter, read-only Cursor role markers, and thin-pointer names against Claude. New shared topics require updating `claudeToCodexSkillMap` in that script; see **Repository consistency checklist** in `docs/workflows.md`.

Tracked `.clangd` sets `CompileFlags.CompilationDatabase` to `out/build/dev`; run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` first so clangd and editors resolve `mirakana/...` includes.

## Surface Parity

The required project-level AI surfaces are:

- Codex skills: `cmake-build-system`, `cpp-engine-debugging`, `editor-change`, `gameengine-agent-integration`, `gameengine-feature`, `gameengine-game-development`, `gameengine-git-publication-preflight`, `license-audit`, `performance-optimization-change`, and `rendering-change`.
- Claude Code skills: `gameengine-agent-integration`, `gameengine-cmake-build-system`, `gameengine-debugging`, `gameengine-editor`, `gameengine-feature`, `gameengine-game-development`, `gameengine-git-publication-preflight`, `gameengine-license-audit`, `gameengine-performance-optimization`, and `gameengine-rendering`.
- Cursor Agent Skills (repository-local): Cursor-only `gameengine-cursor-baseline` and `gameengine-plan-registry`, plus thin routers for every Claude `gameengine-*` topic above (same folder names under `.cursor/skills/`, including `gameengine-git-publication-preflight`).
- Shared subagent roles: `agent-surface-auditor`, `explorer`, `cpp-reviewer`, `engine-architect`, `planning-auditor`, `rendering-auditor`, `gameplay-builder`, and `build-fixer` under `.codex/agents/`, `.claude/agents/`, and `.cursor/agents/`.

`agent-surface-auditor`, `explorer`, `cpp-reviewer`, `engine-architect`, `planning-auditor`, and `rendering-auditor` are read-only roles. `gameplay-builder` and `build-fixer` may edit only when their delegated task expects production work or a targeted fix.

## Static contract chapter ownership

`tools/check-ai-integration.ps1` and `tools/check-json-contracts.ps1` dot-source numeric chapter files in one shared scope. Keep reusable surface-text loads in the chapter that owns the assertions:

- **Chapter 1:** `tools/check-ai-integration-010-agent-baseline.ps1` — agent baseline, docs, plans, tooling needles.
- **Chapter 2:** `tools/check-ai-integration-020-engine-manifest.ps1` — `Get-AgentSurfaceText` for engine/RHI/runtime/renderer contracts (later rendering packs reuse through dot-source).
- **Chapter 4:** `tools/check-json-contracts-040-agent-surfaces.ps1` — agent surfaces and cross-check needles only.
- **Chapter 5:** `tools/check-json-contracts-050-generated-games.ps1` — `Get-JsonContractSurfaceText` for runtime UI/tools C++ contracts.

See `.agents/skills/gameengine-agent-integration/references/static-contract-chapters.md` and **Repository consistency checklist** step 3 in `docs/workflows.md`.

## Required Checks

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

`agent-check` validates the agent manifest, Claude/Codex instruction surfaces, skills, and custom agents.
When you add retained editor UI ids (`play_in_editor.*`), manifest slices, or editor-shell literals enforced by `tools/check-ai-integration.ps1`, extend the matching **Needles** in that script and keep Codex/Claude editor skills aligned in the same change; Cursor `.cursor/skills/gameengine-editor/SKILL.md` stays a thin router that points at the Claude skill and `references/full-guidance.md` (see **Repository consistency checklist** in `docs/workflows.md`).
When you edit any `tools/*.ps1` (especially `check-ai-integration.ps1` or `common.ps1`), follow **`AGENTS.md` → Repository command entrypoints** for **PSScriptAnalyzer-friendly** PowerShell patterns and run **`Invoke-ScriptAnalyzer`** on touched files when the module is installed.
`schema-check` validates engine and game agent JSON contracts with repository-local rules.
`toolchain-check` also enforces that checked-in CMake configure/build presets keep inheriting `normalized-configure-environment` / `normalized-build-environment`, reports linked-worktree and `external/vcpkg` readiness, and `agent-check` verifies the matching guidance remains present in AGENTS, workflow docs, CMake skills, Claude rules, build-fixer subagents, and the engine manifest so Windows CMake/MSBuild `Path`/`PATH` environment regressions are not reintroduced silently.

Engine and game manifests must declare backend readiness, importer requirements, packaging targets, and validation recipes so generated-game agents can avoid planned or dependency-gated capabilities unless they are explicitly implementing them.
The engine manifest also declares `aiOperableProductionLoop`: recipe ids, structured AI command descriptor surfaces, authoring/package surfaces, unsupported production gaps, host gates, and validation recipe mapping. `tools/check-production-readiness-audit.ps1` audits both unsupported gap rows and host-gate rows so host-gated evidence remains visible even when `unsupportedProductionGaps` is empty. Static checks reject stale claims such as command surfaces using legacy `inputContract`/`outputContract`/`dryRun` fields instead of descriptor fields, future 2D/3D recipes becoming `ready`, Vulkan/Metal host gates being removed, prompt/scenario docs losing the enforced recipe ids, `register-source-asset` losing its reviewed `GameEngine.SourceAssetRegistry.v1` helper contract or claiming importer execution/cooked artifact writes/`.geindex` updates/renderer residency, `cook-registered-source-assets` losing its explicit selected-row helper contract or expanding into unreviewed dependency traversal, renderer residency, streaming, graph, Metal, native-handle, shell, or free-form edit claims, `3d-playable-desktop-package` claiming native GPU UI overlay or production renderer quality without the separate `native-gpu-runtime-ui-overlay` recipe, `3d-playable-desktop-package` losing the selected `--require-gltf-scene-import-review` package-evidence boundary or claiming broad glTF scene import readiness/runtime source parsing/parser type/native handle/package mutation from that smoke, the colored overlay recipe claiming production text/font/image/atlas/accessibility, the `native-ui-textured-sprite-atlas` recipe claiming source image decoding or production atlas packing, the `native-ui-atlas-package-metadata` recipe claiming runtime source PNG/JPEG image decoding or production atlas packing, `runtime-ui-production-stack-evidence` losing shaping direction/script/language, glyph ids/clusters/advances/offsets, glyph bitmap/metrics/pixel-format, atlas handoff, dependency/host gate, and unsupported-claim evidence before broad text/font readiness claims, Scene/Component/Prefab Schema v2 being described as more than a contract-only `mirakana_scene` surface before editor/runtime/package migrations land, Asset Identity v2 losing its closed identity/reference boundary evidence (`plan_asset_identity_placements_v2`, `audit_runtime_scene_asset_identity`, key-first games/templates) or claiming renderer/RHI residency, package streaming, runtime source-registry parsing, native handles, or 2D/3D vertical slices, Runtime Resource v2 losing its closed reviewed safe-point/controller evidence or expanding into native watcher ownership, background streaming, arbitrary/LRU eviction, renderer/RHI ownership, upload/staging, allocator/GPU enforcement, package scripts, native handles, or broad hot reload productization, or Frame Graph and Upload/Staging Foundation v1 being described as more than foundation-focused `FrameGraphV1Desc` / `RhiUploadStagingPlan` planning, selected RHI upload rings, selected Frame Graph executors, and RHI no-wait upload batch submission before runtime/package native async upload execution queue consumption, production render graph scheduling, allocator/residency budgets, broad package streaming, and 2D/3D vertical slices land.

## Live Documentation Policy

- Use Context7 MCP for live library, SDK, build-system, and toolchain documentation such as CMake, vcpkg, Direct3D 12, Vulkan, Metal, and C++ tooling.
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
- OpenAI developer docs MCP: https://developers.openai.com/learn/docs-mcp
- Claude Code memory: https://docs.anthropic.com/en/docs/claude-code/memory
- Claude Code settings and permissions: https://docs.anthropic.com/en/docs/claude-code/settings
- Claude Code hooks: https://docs.anthropic.com/en/docs/claude-code/hooks
- Claude Code subagents: https://docs.anthropic.com/en/docs/claude-code/sub-agents
- Cursor rules: https://cursor.com/docs/rules
- Cursor Agent Skills: https://cursor.com/docs/skills
- Cursor subagents: https://cursor.com/docs/subagents
- Cursor Composer 2.5: https://cursor.com/changelog/composer-2-5
- PowerShell approved verbs: https://learn.microsoft.com/en-us/powershell/scripting/developer/cmdlet/approved-verbs-for-windows-powershell-commands
- PSScriptAnalyzer ShouldProcess rule: https://learn.microsoft.com/en-us/powershell/utility-modules/psscriptanalyzer/rules/shouldprocess
- PowerShell ShouldProcess guidance: https://learn.microsoft.com/en-us/powershell/scripting/learn/deep-dives/everything-about-shouldprocess
- PowerShell Write-Information: https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.utility/write-information
