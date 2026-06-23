# Agent Integration Change Matrix

Use this reference only when an agent-surface change touches one of these durable contracts. Keep `SKILL.md` as the concise router and put long procedures here.

## Manifest And Cross-Tool Surfaces

- Documentation starts at `docs/README.md`; plan registry is `docs/superpowers/plans/README.md`. Keep instructions specific, concise, verifiable, and keep machine-readable status claims in the manifest.
- Engine manifest: edit `engine/agent/manifest.fragments/*.json`, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`, and keep `schemas/` plus `tools/check-ai-integration.ps1` needles aligned when literals change. Never hand-edit `engine/agent/manifest.json`.
- Update Codex (`.agents/skills/`), Claude Code (`.claude/skills/`), and Cursor (`.cursor/skills/`, `.cursor/agents/`, `docs/ai-integration.md`) surfaces when cross-tool behavior changes; keep overlapping `.agents` / `.claude` skill bodies equivalent.
- Keep always-loaded files, selected `SKILL.md` bodies, and subagent instructions within `tools/check-agents.ps1` budgets. Put reusable procedures in skills, and move deep procedures to skill-local `references/*.md` when a `SKILL.md` would stop being a concise router.
- Put specialized workers in `.codex/agents` and `.claude/agents`; keep reviewer, explorer, architect, planning, and auditor subagents read-only by default. Write-capable tools belong only on builder/fixer roles expected to change files.
- If implementation work reveals missing or stale agent guidance or AI-operable contract claims, update the relevant instructions, docs, skills, rules, settings, subagents, manifests, schemas, and validation checks in the same task before reporting completion. This is the agent-surface drift check; keep targeted drift checks focused on affected owners.
- Keep engine/game manifest `runtimeBackendReadiness`, `importerCapabilities`, `packagingTargets`, `runtimePackageFiles`, `runtimeSceneValidationTargets`, and `validationRecipes` honest. Do not mark planned or dependency-gated work as implemented.
- When public `mirakana::` diagnostics, counters, profile zones, trace export, or other cross-module capabilities change, update the engine agent manifest, generated-game guidance, and Codex/Claude subagents so agents do not place shared observability in `mirakana_runtime`, renderer, platform, editor, removed SDL3 adapter, or backend modules.
- Local-only ignored surfaces remain uncommitted: `.claude/settings.local.json`, `.mcp.json`, and `AGENTS.override.md`.

## Tooling, Build, And Diagnostics

- Dependency bootstrap changes must keep `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1` as the vcpkg install entrypoint and keep CMake configure lanes on `VCPKG_MANIFEST_INSTALL=OFF`. Update `AGENTS.md`, CMake/license/feature skills, build-fixer subagents, the engine agent manifest, and validation checks together.
- CMake/toolchain preflight or static-analysis changes must update `AGENTS.md`, CMake skills, build-fixer subagents, Claude rules, the
  engine agent manifest, validation checks, `.cursor/skills/`, and `.clangd` when needed. Keep `tools/prepare-worktree.ps1` as
  linked-worktree readiness; `tools/check-toolchain.ps1` and `tools/check-toolchain.ps1 -RequireDirectCMake` scoped; `tools/cmake.ps1` and
  `tools/ctest.ps1` as wrappers. Presets must normalize raw `cmake --preset ...` `PATH` / `Path` for `normalized-configure-environment` and
  `normalized-build-environment`; wrappers also normalize child processes. Keep `direct-clang-format-status`, `tools/check-tidy.ps1`, CMake
  File API synthesis, `.clang-tidy` `HeaderFilterRegex`, hosted `NN warnings generated.` handling, and `tools/check-agents.ps1` hygiene
  aligned.
- Windows host diagnostics guidance must update `docs/dependencies.md`, `docs/workflows.md`, `docs/testing.md`, `AGENTS.md`, Codex/Claude debugging/rendering/CMake skills, build-fixer and rendering-auditor subagents, `.codex/rules`, `.claude/settings.json`, the engine agent manifest, schemas, and validation checks together. Keep Debugging Tools for Windows, Windows Graphics Tools, PIX on Windows, and Windows Performance Toolkit as host-local diagnostics, not default build dependencies.

## Plans, Production Completion, And Game Lanes

- For new production work, update `docs/superpowers/plans/README.md`, keep the live plan stack shallow, and prefer one dated active gap-cluster burn-down or milestone / capability/gap-cluster/milestone plan. A phase-gated milestone plan is allowed for tightly related production slices. Put phase behavior/API/validation boundary decisions, validation-only follow-up, docs/manifest/static-check sync, cleanup, and fine-grained execution steps inside the active plan. Create child plans only for distinct boundaries or unsafe size.
- Plan width may exceed PR width; phases and PRs stay one reviewable purpose and use GitHub Flow checkpoints. Do not split/merge plans for commit/push/PR counts. Avoid copying long completed-slice prose into active sections or `recommendedNextPlan.completedContext`; do not extend historical plans, broaden ready claims, or weaken host gates. Delete unreferenced evidence only after scans and rely on Git history.
- Production-completion work drives from `currentActivePlan`, `recommendedNextPlan`, and `unsupportedProductionGaps`; prefer clean breaking greenfield designs, official documentation, and focused validation over compatibility shims or broad ready claims.
- Mobile packaging changes must keep Android and Apple host gates distinct. Android GameActivity has host-validated Debug/Release/signing/emulator smoke lanes on configured hosts; Apple/iOS remains macOS/Xcode gated. Update `AGENTS.md`, Codex/Claude game-development skills, gameplay/build subagents, `docs/ai-game-development.md`, generated-game validation scenarios, and the engine agent manifest together.
- Reviewed AI-operable content mutation or metadata-repair surface changes must update `docs/ai-game-development.md`, Codex/Claude game-development skills, relevant subagents, the engine agent manifest, schema/static checks, and validation recipes together. Keep arbitrary shell, free-form edit/eval, importer execution, shader compiler execution, cooked package writes, renderer/RHI residency, and package streaming claims rejected unless the reviewed surface implements and validates them.

## Publication And Worktrees

- Git/GitHub publishing workflow changes or PR CI check selection changes must update `AGENTS.md`, `docs/workflows.md`, `docs/testing.md`, Codex/Claude/Cursor skills, command-policy rules/settings only when permissions change, relevant subagents, and `tools/check-ai-integration.ps1` needles together.
- Keep purpose/checkpoint-based cadence: validated commits, checkpoint pushes after remote-state inspection, one PR per focused capability/gap-cluster/milestone, early `--draft` PRs, guarded `tools/ready-task-pr.ps1` conversion, PR create/auto-merge registration / merge registration with `--match-head-commit <headRefOid>`, and guarded merged-worktree cleanup after official GitHub Flow PR preflight.
- A final completion report must not stop after local validation when task-owned changes can be published. Direct default-branch pushes are forbidden. Runtime/build/public-contract PRs wait for selected hosted checks to complete and `PR Gate` to report `SUCCESS`.
- Keep branch-protection-required checks always-running or behind an always-running aggregate gate. Do not make path-filtered required checks:
  GitHub workflow-level path/branch filters can leave required checks pending. Keep heavy validation as job-level conditional lanes selected
  by `tools/classify-pr-validation-tier.ps1` and guarded by `tools/check-ci-matrix.ps1`. Explicit PR outputs include `windows_msvc`,
  `windows_cpu_profiling_host`, `windows_asset_importers`, `windows_desktop_editor`, `windows_network_enet`, `linux_cmake`,
  `linux_vulkan_host`, `macos_metal_cmake`, `metal_host_evidence`, and `ios_metal_evidence`. Classifier diagnostics include
  `selected_lanes` and `classification_reasons`, and `validate.yml` writes them to `GITHUB_STEP_SUMMARY` for latest-head triage.
- Windows CI lanes use feature-scoped bootstrap: `desktop-runtime` for `Windows MSVC` / C++23, `asset-importers` for `windows_asset_importers`, `network-enet` for `windows_network_enet`, and no vcpkg bootstrap for `windows_desktop_editor`. Optional lane failures should be reproduced with their owning wrapper script before broadening `Windows MSVC`.
- CI caches are acceleration only. Use `actions/cache/restore` and `actions/cache/save`, reuse restore `cache-primary-key` when saving, and keep cache restore/save transport failures from becoming validation evidence. Docs/agent/rules/subagent-only changes run formatting plus agent/static guards; docs/agent-only PRs use lightweight static validation.
- Hosted PR failure hardening: Static-analysis drift includes `.clang-tidy` `HeaderFilterRegex`, strict warnings, and `NN warnings generated.` handling. Keep `.codex/rules` and `.claude/settings.json` as command/permission gates, not troubleshooting playbooks.
- Guarded cleanup verifies merged head reaches `origin/main`, accepts Git main worktree porcelain records, unlinks worktree-local vcpkg reparse points, and keeps Windows fallback guarded/non-following.
- Rule changes may need policy reload. Git/GitHub auth stays host-local; do not add repository requirements for `GITHUB_TOKEN`. If `credential-manager-core` warnings appear, fix host/user Git config. Use an approval-capable session when prompt-gated publication actions block.
- Worktree or parallel-agent workflow changes must update `AGENTS.md`, `docs/workflows.md`, `docs/ai-integration.md`, Codex/Claude/Cursor skills, `.cursor/agents/`, `.gitignore`, `.codex/rules`, `.claude/settings.json`, write-capable subagents, and static checks together. Prefer Codex app Worktree/Handoff or Claude Code subagent `isolation: worktree`, keep `worktree.baseRef = "head"` for Claude worktrees, keep raw worktree cleanup reviewed, and expose only guarded post-merge cleanup for automatic removal.

## Validation Needles

- Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` as focused agent-surface loops.
- Run the smallest relevant agent/static check while iterating, usually `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` for manifest or instruction changes, then run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` at the slice-closing gate.
- When extending `tools/check-ai-integration.ps1` needles, edit the scoped target `Path`, keep each needle unambiguous, use single-quoted PowerShell strings for literal needles containing Markdown backticks, and update sibling skill/manifest text.
