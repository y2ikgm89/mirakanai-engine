#requires -Version 7.0
#requires -PSEdition Core

# Chapter 1 for check-ai-integration.ps1 static contracts.
# Agent baseline, docs, plans, and tooling needles only. Engine/RHI/runtime surface texts load in check-ai-integration-020-engine-manifest.ps1.

$agents = Resolve-RequiredAgentPath "AGENTS.md"
$claude = Resolve-RequiredAgentPath "CLAUDE.md"
$manifestPath = Resolve-RequiredAgentPath "engine/agent/manifest.json"
$manifestRaw = Get-Content -LiteralPath $manifestPath -Raw
$gameAgentSchemaPath = Resolve-RequiredAgentPath "schemas/game-agent.schema.json"
$currentCapabilitiesPath = Resolve-RequiredAgentPath "docs/current-capabilities.md"
$aiGameDevelopmentPath = Resolve-RequiredAgentPath "docs/ai-game-development.md"
$aiIntegrationPath = Resolve-RequiredAgentPath "docs/ai-integration.md"
$architecturePath = Resolve-RequiredAgentPath "docs/architecture.md"
$agentOperationalReferencePath = Resolve-RequiredAgentPath "docs/agent-operational-reference.md"
$cppStylePath = Resolve-RequiredAgentPath "docs/cpp-style.md"
$roadmapPath = Resolve-RequiredAgentPath "docs/roadmap.md"
$gameAgentSchemaText = Get-Content -LiteralPath $gameAgentSchemaPath -Raw
$currentCapabilitiesContent = Get-Content -LiteralPath $currentCapabilitiesPath -Raw
$aiGameDevelopmentContent = Get-Content -LiteralPath $aiGameDevelopmentPath -Raw
$aiIntegrationContent = Get-Content -LiteralPath $aiIntegrationPath -Raw
$architectureContent = Get-Content -LiteralPath $architecturePath -Raw
$agentOperationalReferenceContent = Get-Content -LiteralPath $agentOperationalReferencePath -Raw
$cppStyleContent = Get-Content -LiteralPath $cppStylePath -Raw
$roadmapContent = Get-Content -LiteralPath $roadmapPath -Raw
$workflowsPath = Resolve-RequiredAgentPath "docs/workflows.md"
$testingPath = Resolve-RequiredAgentPath "docs/testing.md"
$buildingPath = Resolve-RequiredAgentPath "docs/building.md"
$releasePath = Resolve-RequiredAgentPath "docs/release.md"
$dependenciesPath = Resolve-RequiredAgentPath "docs/dependencies.md"
$legalPath = Resolve-RequiredAgentPath "docs/legal-and-licensing.md"
$planRegistryPath = Resolve-RequiredAgentPath "docs/superpowers/plans/README.md"
$productionCompletionMasterPlanRelativePath = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
$publicationPreflightToolPath = Resolve-RequiredAgentPath "tools/check-publication-preflight.ps1"
$codexPublicationPreflightSkillPath = Resolve-RequiredAgentPath ".agents/skills/gameengine-git-publication-preflight/SKILL.md"
$claudePublicationPreflightSkillPath = Resolve-RequiredAgentPath ".claude/skills/gameengine-git-publication-preflight/SKILL.md"
$cursorPublicationPreflightSkillPath = Resolve-RequiredAgentPath ".cursor/skills/gameengine-git-publication-preflight/SKILL.md"
$removeMergedWorktreeToolPath = Resolve-RequiredAgentPath "tools/remove-merged-worktree.ps1"
foreach ($textFormatToolPath in @("tools/check-text-format.ps1", "tools/check-text-format-contract.ps1", "tools/format-text.ps1", "tools/text-format-core.ps1")) {
    Resolve-RequiredAgentPath $textFormatToolPath | Out-Null
}

$claudeContent = Get-Content -LiteralPath $claude -Raw
if ($claudeContent -notmatch "@AGENTS\.md") {
    Write-Error "CLAUDE.md must import AGENTS.md"
}
foreach ($importPath in @(".claude/rules/ai-agent-integration.md", ".claude/rules/cpp-engine.md")) {
    if ($claudeContent -notmatch [System.Text.RegularExpressions.Regex]::Escape("@$importPath")) {
        Write-Error "CLAUDE.md must import $importPath"
    }
}

$agentsContent = Get-Content -LiteralPath $agents -Raw
if ($agentsContent -notmatch "Context7") {
    Write-Error "AGENTS.md must document the Context7 documentation lookup policy"
}
foreach ($needle in @("OpenAI developer documentation MCP", "official Anthropic documentation", ".codex/rules", ".claude/settings.json")) {
    Assert-ContainsText $agentsContent $needle "AGENTS.md"
}
foreach ($needle in @(".claude/settings.local.json", ".mcp.json", "AGENTS.override.md")) {
    Assert-ContainsText $agentsContent $needle "AGENTS.md"
}
foreach ($needle in @("Instruction Hygiene", "specific, concise, verifiable", "MCP connection state", "machine-readable capability/status claims")) {
    Assert-ContainsText $agentsContent $needle "AGENTS.md"
}
Assert-ContainsText $agentsContent "agent-surface drift check" "AGENTS.md"
Assert-ContainsText $agentsContent "do not broad-load every agent surface" "AGENTS.md"
Assert-ContainsText $agentsContent 'Active editor/runtime UI uses first-party `mirakana_ui`, `mirakana::ui`, and `MK_editor`' "AGENTS.md first-party UI baseline"
Assert-DoesNotContainText $agentsContent "Treat Dear ImGui as optional developer/editor tooling only" "AGENTS.md stale Dear ImGui guidance"
if ($agentsContent -notmatch "docs/README\.md" -or $agentsContent -notmatch "docs/superpowers/plans/README\.md") {
    Write-Error "AGENTS.md must document the docs entrypoint and implementation plan registry"
}
Assert-ContainsText $agentsContent "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1" "AGENTS.md"
Assert-ContainsText $agentsContent "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1" "AGENTS.md"
Assert-ContainsText $agentsContent "tools/remove-merged-worktree.ps1" "AGENTS.md"
Assert-ContainsText $agentsContent "fast-forwards" "AGENTS.md"
Assert-ContainsText $agentsContent "verifies branch ancestry" "AGENTS.md"
Assert-ContainsText $agentsContent "unlinks worktree-local vcpkg links" "AGENTS.md"
Assert-ContainsText $agentsContent "Windows fallback stays guarded/non-following" "AGENTS.md"
Assert-ContainsText $agentsContent "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1" "AGENTS.md"
Assert-ContainsText $agentsContent "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1 -RequireDirectCMake" "AGENTS.md"
Assert-ContainsText $agentsContent "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev" "AGENTS.md"
Assert-ContainsText $agentsContent "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure" "AGENTS.md"
Assert-ContainsText $agentsContent "normalized-configure-environment" "AGENTS.md"
Assert-ContainsText $agentsContent "normalized-build-environment" "AGENTS.md"
Assert-ContainsText $agentsContent "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1" "AGENTS.md"
Assert-ContainsText $agentsContent "tools/check-text-format.ps1" "AGENTS.md"
Assert-ContainsText $agentsContent "tools/format-text.ps1" "AGENTS.md"
Assert-ContainsText $agentsContent "direct-clang-format-status" "AGENTS.md"
Assert-ContainsText $agentsContent "CMake File API" "AGENTS.md"
Assert-ContainsText $agentsContent "VCPKG_MANIFEST_INSTALL=OFF" "AGENTS.md"
Assert-ContainsText $agentsContent 'VCPKG_INSTALLED_DIR=${sourceDir}/vcpkg_installed' "AGENTS.md"
Assert-ContainsText $agentsContent "CMake configure must not install, restore, or download vcpkg packages" "AGENTS.md"
Assert-ContainsText $agentsContent "phase-gated milestone plan" "AGENTS.md"
Assert-ContainsText $aiIntegrationContent "CMake, vcpkg, Direct3D 12, Vulkan, Metal, and C++ tooling" "docs/ai-integration.md live docs policy"
Assert-DoesNotContainText $aiIntegrationContent "CMake, vcpkg, SDL3, Dear ImGui" "docs/ai-integration.md stale live docs policy"
Assert-ContainsText $architectureContent 'The visible `MK_editor` shell is the active dependency-free `desktop-editor` lane' "docs/architecture.md first-party editor shell"
Assert-ContainsText $architectureContent 'Active editor UI must use first-party retained `MK_editor` / `mirakana::ui` contracts' "docs/architecture.md editor dependency rule"
Assert-DoesNotContainText $architectureContent 'The visible `MK_editor` shell is currently deferred' "docs/architecture.md stale deferred editor shell"
Assert-DoesNotContainText $architectureContent 'may adapt editor-core or future `MK_editor_ui` models to Dear ImGui' "docs/architecture.md stale Dear ImGui editor allowance"
Assert-ContainsText $agentOperationalReferenceContent "the historical Dear ImGui shell implementation files have been deleted" "docs/agent-operational-reference.md first-party editor closeout"
Assert-DoesNotContainText $agentOperationalReferenceContent "remaining historical Dear ImGui shell files are closeout-only deletion targets" "docs/agent-operational-reference.md stale Dear ImGui deletion target"
Assert-ContainsText $cppStyleContent 'The visible `MK_editor` shell is the active first-party `desktop-editor` lane' "docs/cpp-style.md first-party editor shell"
Assert-DoesNotContainText $cppStyleContent 'The visible `MK_editor` shell is deferred after SDL3 removal' "docs/cpp-style.md stale deferred editor shell"
foreach ($planVolumeNeedle in @("live plan stack shallow", "active gap-cluster burn-down or milestone", "capability/gap-cluster/milestone", "not PR/task-count units", "phase behavior/API/validation", "validation-only follow-up", "Git history")) {
    Assert-ContainsText $agentsContent $planVolumeNeedle "AGENTS.md"
}
foreach ($productionPromptNeedle in @("Production Completion Execution", "currentActivePlan", "recommendedNextPlan", "unsupportedProductionGaps", "clean breaking greenfield designs", "completed gap, remaining gaps, next active plan")) {
    Assert-ContainsText $agentsContent $productionPromptNeedle "AGENTS.md"
}
Assert-ContainsText $agentsContent "validated commit checkpoints" "AGENTS.md"
Assert-ContainsText $agentsContent "policy reload" "AGENTS.md"
Assert-ContainsText $agentsContent "GitHub Desktop" "AGENTS.md"
Assert-ContainsText $agentsContent "official GitHub Flow" "AGENTS.md"
Assert-ContainsText $agentsContent 'Before staging/push/PR/merge, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1' "AGENTS.md"
Assert-ContainsText $agentsContent "tools/check-publication-preflight.ps1" "AGENTS.md"
Assert-ContainsText $agentsContent "PR head SHA" "AGENTS.md"

$publicationPreflightToolContent = Get-Content -LiteralPath $publicationPreflightToolPath -Raw
foreach ($needle in @(
        "Test-PublicationPreflight",
        "git update-index -q --refresh",
        "git rev-parse --path-format=absolute --git-path index.lock",
        "Test-NetConnection",
        "gh auth status",
        "gh pr view",
        "publication-preflight: ok",
        "publication-preflight: blocked",
        "PublicationPreflightStatus"
    )) {
    Assert-ContainsText $publicationPreflightToolContent $needle "tools/check-publication-preflight.ps1"
}

$publicationPreflightDetailedSkillSurfaces = @(
    @{ Path = $codexPublicationPreflightSkillPath; Label = ".agents/skills/gameengine-git-publication-preflight/SKILL.md" },
    @{ Path = $claudePublicationPreflightSkillPath; Label = ".claude/skills/gameengine-git-publication-preflight/SKILL.md" },
    @{ Path = $cursorPublicationPreflightSkillPath; Label = ".cursor/skills/gameengine-git-publication-preflight/SKILL.md" }
)
foreach ($publicationPreflightSkillSurface in $publicationPreflightDetailedSkillSurfaces) {
    $publicationPreflightSkillText = Get-Content -LiteralPath $publicationPreflightSkillSurface.Path -Raw
    foreach ($needle in @(
            "tools/check-publication-preflight.ps1",
            "whoami",
            "git update-index -q --refresh",
            "Test-NetConnection github.com -Port 443",
            "gh auth status",
            "gh pr view <pr> --json headRefOid,statusCheckRollup,url",
            "publication-preflight: blocked",
            "publication temp clone"
        )) {
        Assert-ContainsText $publicationPreflightSkillText $needle $publicationPreflightSkillSurface.Label
    }
}

$cursorEditorSkillPath = Resolve-RequiredAgentPath ".cursor/skills/gameengine-editor/SKILL.md"
$cursorEditorSkillText = Get-Content -LiteralPath $cursorEditorSkillPath -Raw
foreach ($needle in @(
        ".claude/skills/gameengine-editor/SKILL.md",
        ".claude/skills/gameengine-editor/references/full-guidance.md",
        "AGENTS.md"
    )) {
    Assert-ContainsText $cursorEditorSkillText $needle ".cursor/skills/gameengine-editor/SKILL.md"
}

$removeMergedWorktreeToolContent = Get-Content -LiteralPath $removeMergedWorktreeToolPath -Raw
Assert-ContainsText $removeMergedWorktreeToolContent "ConvertTo-ExtendedLengthPath" "tools/remove-merged-worktree.ps1"
Assert-ContainsText $removeMergedWorktreeToolContent "long-path-delete-fallback=enabled" "tools/remove-merged-worktree.ps1"
Assert-ContainsText $removeMergedWorktreeToolContent "ReparsePoint" "tools/remove-merged-worktree.ps1"
Assert-ContainsText $removeMergedWorktreeToolContent "Remove-WorktreeLocalReparsePointBeforeGitRemoval" "tools/remove-merged-worktree.ps1"
Assert-ContainsText $removeMergedWorktreeToolContent '$mainWorktreeRecord' "tools/remove-merged-worktree.ps1"
Assert-ContainsText $removeMergedWorktreeToolContent '$standardWorktreeBasePaths' "tools/remove-merged-worktree.ps1"
Assert-ContainsText $removeMergedWorktreeToolContent '"worktree", "list", "--porcelain"' "tools/remove-merged-worktree.ps1"
Assert-ContainsText $removeMergedWorktreeToolContent "external/vcpkg" "tools/remove-merged-worktree.ps1"
Assert-ContainsText $removeMergedWorktreeToolContent "vcpkg_installed" "tools/remove-merged-worktree.ps1"
Assert-ContainsText $removeMergedWorktreeToolContent "[switch]`$DeleteRemoteBranch" "tools/remove-merged-worktree.ps1"
Assert-ContainsText $removeMergedWorktreeToolContent '"push", $Remote, "--delete", $record.Branch' "tools/remove-merged-worktree.ps1"
Assert-ContainsText $agentsContent "Direct default-branch pushes are outside the official GitHub Flow path" "AGENTS.md"
Assert-ContainsText $agentsContent "credential-manager-core" "AGENTS.md"
Assert-ContainsText $agentsContent "gh pr view <pr> --json" "AGENTS.md"
Assert-ContainsText $agentsContent "gh pr create" "AGENTS.md"
Assert-ContainsText $agentsContent "gh pr merge --auto --merge --match-head-commit <headRefOid>" "AGENTS.md"
Assert-ContainsText $agentsContent 'do not pass `--delete-branch`' "AGENTS.md"
Assert-ContainsText $agentsContent "clean PR state" "AGENTS.md"
Assert-ContainsText $agentsContent "selected hosted checks" "AGENTS.md"
Assert-ContainsText $agentsContent "--match-head-commit <headRefOid>" "AGENTS.md"
Assert-ContainsText $agentsContent "clean local worktree" "AGENTS.md"
Assert-ContainsText $agentsContent "commits pushed after merge need a new PR" "AGENTS.md"
Assert-ContainsText $agentsContent "not publication-complete after local validation alone" "AGENTS.md"
Assert-ContainsText $agentsContent "checkpoint-based, not commit-count-based" "AGENTS.md"
foreach ($gitCadenceNeedle in @("purpose/checkpoint-based", "commit validated phases", "push validated checkpoints", "one PR per focused capability/gap-cluster/milestone", "split unrelated work")) {
    Assert-ContainsText $agentsContent $gitCadenceNeedle "AGENTS.md"
}
Assert-ContainsText $agentsContent "network" "AGENTS.md"
Assert-ContainsText $agentsContent "open draft PR" "AGENTS.md"
Assert-ContainsText $agentsContent "before final report" "AGENTS.md"
Assert-ContainsText $agentsContent "codex-<topic>" "AGENTS.md"
Assert-ContainsText $agentsContent "Codex app Worktree/Handoff" "AGENTS.md"
Assert-ContainsText $agentsContent "isolation: worktree" "AGENTS.md"
Assert-ContainsText $agentsContent ".claude/worktrees/" "AGENTS.md"
Assert-ContainsText $agentsContent "For hosted PR/CI failures" "AGENTS.md"
Assert-ContainsText $agentsContent "PR CI selection" "AGENTS.md"
Assert-ContainsText $agentsContent "always-running required gate" "AGENTS.md"
Assert-ContainsText $agentsContent "path-filtered required workflows" "AGENTS.md"
Assert-ContainsText $agentsContent "Docs/agent/rules/subagent-only PRs use agent/static guards" "AGENTS.md"
Assert-ContainsText $agentsContent "HeaderFilterRegex" "AGENTS.md"
Assert-ContainsText $agentsContent "--warnings-as-errors=*" "AGENTS.md"
Assert-ContainsText $agentsContent "NN warnings generated." "AGENTS.md"
Assert-ContainsText $agentsContent "-Jobs 0" "AGENTS.md"
Assert-ContainsText $agentsContent "lcov --ignore-errors unused" "AGENTS.md"
Assert-ContainsText $agentsContent "runtime/.gitattributes" "AGENTS.md"
Assert-ContainsText $agentsContent "tools/new-game-templates.ps1" "AGENTS.md"
Assert-ContainsText $agentsContent "tools/static-contract-common.ps1" "AGENTS.md"
Assert-ContainsText $agentsContent "tools/static-contract-ledger.ps1" "AGENTS.md"
Assert-ContainsText $agentsContent "numeric-prefix discovery" "AGENTS.md"
Assert-ContainsText $agentsContent "Billing/spending-limit failures" "AGENTS.md"
Assert-ContainsText $agentsContent 'Runtime/C++/build/toolchain/public-contract commits need one fresh `tools/validate.ps1`' "AGENTS.md"
Assert-ContainsText $agentsContent 'run `tools/build.ps1` only when standalone build evidence is requested' "AGENTS.md"
Assert-ContainsText $agentsContent "Docs/non-runtime slices may record narrower justified checks" "AGENTS.md"
foreach ($windowsDiagnosticsNeedle in @("Debugging Tools for Windows", "Windows Graphics Tools", "PIX on Windows", "Windows Performance Toolkit")) {
    Assert-ContainsText $agentsContent $windowsDiagnosticsNeedle "AGENTS.md"
}

$workflowsContent = Get-Content -LiteralPath $workflowsPath -Raw
Assert-ContainsText $workflowsContent "phase-gated milestone plan" "docs/workflows.md"
foreach ($planVolumeNeedle in @("live plan stack shallow", "active gap-cluster burn-down or milestone", "capability/gap-cluster/milestone", "Plan-file width and PR/phase width are different", "phase behavior/API/validation boundary", "validation-only follow-up", "current active plan checklist")) {
    Assert-ContainsText $workflowsContent $planVolumeNeedle "docs/workflows.md"
}
foreach ($productionPromptNeedle in @("Production Completion Prompt", "currentActivePlan", "recommendedNextPlan", "unsupportedProductionGaps", "clean breaking greenfield designs", "completed gap, remaining gaps, next active plan")) {
    Assert-ContainsText $workflowsContent $productionPromptNeedle "docs/workflows.md"
}
Assert-ContainsText $workflowsContent "Agent Surface Governance" "docs/workflows.md"
Assert-ContainsText $workflowsContent "agent-surface drift check" "docs/workflows.md"
Assert-ContainsText $workflowsContent "tools/static-contract-common.ps1" "docs/workflows.md"
Assert-ContainsText $workflowsContent "without hand-listed chapter rosters" "docs/workflows.md"
Assert-ContainsText $workflowsContent "Keep drift checks targeted" "docs/workflows.md"
Assert-ContainsText $workflowsContent "OpenAI developer documentation MCP" "docs/workflows.md"
Assert-ContainsText $workflowsContent ".codex/rules" "docs/workflows.md"
Assert-ContainsText $workflowsContent ".claude/settings.json" "docs/workflows.md"
Assert-ContainsText $workflowsContent ".claude/settings.local.json" "docs/workflows.md"
Assert-ContainsText $workflowsContent ".mcp.json" "docs/workflows.md"
Assert-ContainsText $workflowsContent "AGENTS.override.md" "docs/workflows.md"
Assert-ContainsText $workflowsContent "Commit, Push, And Pull Request Workflow" "docs/workflows.md"
Assert-ContainsText $workflowsContent "Treat publishing as a slice-closing gate" "docs/workflows.md"
Assert-ContainsText $workflowsContent "Publication-capable Codex sessions" "docs/workflows.md"
Assert-ContainsText $workflowsContent "tools/check-publication-preflight.ps1" "docs/workflows.md"
Assert-ContainsText $workflowsContent "git rev-parse --path-format=absolute --git-path index.lock" "docs/workflows.md"
Assert-ContainsText $workflowsContent "git update-index -q --refresh" "docs/workflows.md"
Assert-ContainsText $workflowsContent "publication-preflight: blocked" "docs/workflows.md"
Assert-ContainsText $workflowsContent "publication temp clone" "docs/workflows.md"
Assert-ContainsText $workflowsContent "gh auth status" "docs/workflows.md"
Assert-ContainsText $workflowsContent "do not bypass GitHub Flow by hand-writing GitHub REST/MCP blobs" "docs/workflows.md"
Assert-ContainsText $workflowsContent "do not report a task complete while task-owned changes only exist locally after validation" "docs/workflows.md"
Assert-ContainsText $workflowsContent "branch creation, task-owned staging, commit, non-forced push, and PR creation/update" "docs/workflows.md"
Assert-ContainsText $workflowsContent "codex-<topic>" "docs/workflows.md"
Assert-ContainsText $workflowsContent "gh pr view <pr> --json" "docs/workflows.md"
Assert-ContainsText $workflowsContent "gh pr create" "docs/workflows.md"
Assert-ContainsText $workflowsContent "gh pr merge --auto --merge --match-head-commit <headRefOid>" "docs/workflows.md"
Assert-ContainsText $workflowsContent 'do not pass `--delete-branch` in linked-worktree sessions' "docs/workflows.md"
Assert-ContainsText $workflowsContent "--match-head-commit <headRefOid>" "docs/workflows.md"
Assert-ContainsText $workflowsContent 'stale `headRefOid` invalidates the merge decision' "docs/workflows.md"
Assert-ContainsText $workflowsContent "git log --oneline origin/main..<headRefOid>" "docs/workflows.md"
Assert-ContainsText $workflowsContent "commit was pushed to the same head branch after the PR had already merged" "docs/workflows.md"
Assert-ContainsText $workflowsContent "mergeStateStatus" "docs/workflows.md"
Assert-ContainsText $workflowsContent 'selected hosted check in `IN_PROGRESS`, `QUEUED`, or missing state' "docs/workflows.md"
Assert-ContainsText $workflowsContent '`PR Gate` must report `SUCCESS`' "docs/workflows.md"
Assert-ContainsText $workflowsContent "gh pr merge --merge --delete-branch" "docs/workflows.md"
Assert-ContainsText $workflowsContent "git fetch --prune <remote>" "docs/workflows.md"
Assert-ContainsText $workflowsContent "guarded post-merge branch and worktree cleanup" "docs/workflows.md"
Assert-ContainsText $workflowsContent "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/remove-merged-worktree.ps1" "docs/workflows.md"
Assert-ContainsText $workflowsContent "-DeleteRemoteBranch" "docs/workflows.md"
Assert-ContainsText $workflowsContent "remote branch ancestry" "docs/workflows.md"
Assert-ContainsText $workflowsContent 'Git main worktree record reported by `git worktree list --porcelain`' "docs/workflows.md"
Assert-ContainsText $workflowsContent "standard worktree-root checks from official porcelain records" "docs/workflows.md"
Assert-ContainsText $workflowsContent "fast-forwards the local checkout" "docs/workflows.md"
Assert-ContainsText $workflowsContent "long-path deletion fallback" "docs/workflows.md"
Assert-ContainsText $workflowsContent "without following reparse points" "docs/workflows.md"
Assert-ContainsText $workflowsContent "do not stash, merge into an active feature branch" "docs/workflows.md"
Assert-ContainsText $workflowsContent "Hosted PR Check Failure Triage" "docs/workflows.md"
Assert-ContainsText $workflowsContent "Hosted PR Check Selection" "docs/workflows.md"
Assert-ContainsText $workflowsContent "always-running required gate" "docs/workflows.md"
Assert-ContainsText $workflowsContent "Path-filtered workflows must not be branch-protection-required" "docs/workflows.md"
Assert-ContainsText $workflowsContent "Docs/agent/rules/subagent-only" "docs/workflows.md"
Assert-ContainsText $workflowsContent 'Do not run `Windows MSVC`, `macOS Metal CMake`, or `Full Repository Static Analysis`' "docs/workflows.md"
Assert-ContainsText $workflowsContent "gh pr view <pr> --json headRefOid,statusCheckRollup,url" "docs/workflows.md"
Assert-ContainsText $workflowsContent "GitHub account billing/spending-limit" "docs/workflows.md"
Assert-ContainsText $workflowsContent 'hosted `static-analysis` failures' "docs/workflows.md"
Assert-ContainsText $workflowsContent "NN warnings generated." "docs/workflows.md"
Assert-ContainsText $workflowsContent "Documentation-only or similarly narrow non-runtime slices" "docs/workflows.md"
Assert-ContainsText $workflowsContent "protected branches" "docs/workflows.md"
Assert-ContainsText $workflowsContent "policy reload" "docs/workflows.md"
Assert-ContainsText $workflowsContent "GitHub flow" "docs/workflows.md"
Assert-ContainsText $workflowsContent "official GitHub Flow" "docs/workflows.md"
foreach ($workflowCadenceNeedle in @("purpose/checkpoint-based", "commit complete validated phases", "push validated checkpoints", "one PR per focused capability/gap-cluster/milestone", "Do not open a PR for every commit")) {
    Assert-ContainsText $workflowsContent $workflowCadenceNeedle "docs/workflows.md"
}
Assert-ContainsText $workflowsContent "direct default-branch pushes must stay forbidden" "docs/workflows.md"
Assert-ContainsText $workflowsContent "GITHUB_TOKEN" "docs/workflows.md"
Assert-ContainsText $workflowsContent "credential-manager-core" "docs/workflows.md"
Assert-ContainsText $workflowsContent "git config --show-origin --get-all credential.helper" "docs/workflows.md"
Assert-ContainsText $workflowsContent "approval-capable session" "docs/workflows.md"
Assert-ContainsText $workflowsContent "Worktree And Parallel Agent Workflow" "docs/workflows.md"
Assert-ContainsText $workflowsContent '`planning-auditor`: read-only plan lifecycle audit' "docs/workflows.md"
Assert-ContainsText $workflowsContent '`planning-auditor`, and `rendering-auditor` use `gpt-5.4` with high reasoning' "docs/workflows.md"
Assert-ContainsText $workflowsContent 'Use `planning-auditor` only for bounded read-only plan lifecycle audits' "docs/workflows.md"
Assert-ContainsText $workflowsContent "Codex app Worktree/Handoff" "docs/workflows.md"
Assert-ContainsText $workflowsContent "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1" "docs/workflows.md"
Assert-ContainsText $workflowsContent ".claude/worktrees/" "docs/workflows.md"
Assert-ContainsText $workflowsContent "specific, concise, verifiable" "docs/workflows.md"
Assert-ContainsText $workflowsContent "machine-readable capability/status claims" "docs/workflows.md"
Assert-ContainsText $workflowsContent "direct-clang-format-status" "docs/workflows.md"
Assert-ContainsText $workflowsContent "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1" "docs/workflows.md"
Assert-ContainsText $workflowsContent "tools/check-text-format.ps1" "docs/workflows.md"
Assert-ContainsText $workflowsContent "tools/format-text.ps1" "docs/workflows.md"
Assert-ContainsText $workflowsContent "CMake File API codemodel" "docs/workflows.md"
Assert-ContainsText $workflowsContent "normalized-configure-environment" "docs/workflows.md"
Assert-ContainsText $workflowsContent "normalized-build-environment" "docs/workflows.md"
Assert-ContainsText $workflowsContent 'PATH`/`Path' "docs/workflows.md"
foreach ($windowsDiagnosticsNeedle in @("Debugging Tools for Windows", "Windows Graphics Tools", "PIX on Windows", "Windows Performance Toolkit", "cdb -version", "pixtool --help", "wpr -help", "xperf -help", "_NT_SYMBOL_PATH")) {
    Assert-ContainsText $workflowsContent $windowsDiagnosticsNeedle "docs/workflows.md"
}
$testingContent = Get-Content -LiteralPath $testingPath -Raw
Assert-ContainsText $testingContent "direct-clang-format-status" "docs/testing.md"
Assert-ContainsText $testingContent "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1" "docs/testing.md"
Assert-ContainsText $testingContent "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1" "docs/testing.md"
Assert-ContainsText $testingContent "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format-text.ps1" "docs/testing.md"
Assert-ContainsText $testingContent "tools/check-text-format-contract.ps1" "docs/testing.md"
Assert-ContainsText $testingContent "CMake File API codemodel" "docs/testing.md"
Assert-ContainsText $testingContent "-ReuseExistingFileApiReply" "docs/testing.md"
Assert-ContainsText $testingContent "test.ps1 -SkipBuild" "docs/testing.md"
Assert-ContainsText $testingContent "-ShardCount 4" "docs/testing.md"
Assert-ContainsText $testingContent "-ShardIndex <matrix>" "docs/testing.md"
Assert-ContainsText $testingContent "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1" "docs/testing.md"
Assert-ContainsText $testingContent "normalized-configure-environment" "docs/testing.md"
Assert-ContainsText $testingContent "normalized-build-environment" "docs/testing.md"
Assert-ContainsText $testingContent 'child `Path` on Windows' "docs/testing.md"
Assert-ContainsText $testingContent "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev" "docs/testing.md"
Assert-ContainsText $testingContent "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure" "docs/testing.md"
Assert-ContainsText $testingContent "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/physics/src/physics3d.cpp" "docs/testing.md"
$buildingContent = Get-Content -LiteralPath $buildingPath -Raw
Assert-ContainsText $buildingContent "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1" "docs/building.md"
Assert-ContainsText $buildingContent "normalized-configure-environment" "docs/building.md"
Assert-ContainsText $buildingContent "normalized-build-environment" "docs/building.md"
Assert-ContainsText $buildingContent 'MSBuild a single `Path`' "docs/building.md"
Assert-ContainsText $buildingContent "tools/cmake.ps1 --preset dev" "docs/building.md"
Assert-ContainsText $buildingContent "tools/ctest.ps1 --preset dev --output-on-failure" "docs/building.md"
Assert-ContainsText $agentsContent "/INCREMENTAL:NO" "AGENTS.md"
Assert-ContainsText $agentsContent "COMPILE_PDB_OUTPUT_DIRECTORY" "AGENTS.md"
Assert-ContainsText $agentsContent "compact CMake target names" "AGENTS.md"
Assert-ContainsText $agentsContent "/MP2" "AGENTS.md"
Assert-ContainsText $agentsContent "/Zf" "AGENTS.md"
Assert-ContainsText $agentsContent 'stale MSVC `.tlog` roots' "AGENTS.md"
Assert-ContainsText $buildingContent "/INCREMENTAL:NO" "docs/building.md"
Assert-ContainsText $buildingContent "COMPILE_PDB_OUTPUT_DIRECTORY" "docs/building.md"
Assert-ContainsText $buildingContent "compact" "docs/building.md MSVC object path guidance"
Assert-ContainsText $buildingContent "CTest name" "docs/building.md MSVC object path guidance"
Assert-ContainsText $buildingContent "/MP2" "docs/building.md"
Assert-ContainsText $buildingContent "/Zf" "docs/building.md"
Assert-ContainsText $buildingContent "MSB8028" "docs/building.md"
$cmakeListsContent = Get-AgentSurfaceText "CMakeLists.txt"
Assert-ContainsText $cmakeListsContent 'MK_MSVC_MULTIPROCESSOR_COMPILE_PROCESSES' "CMakeLists.txt"
Assert-ContainsText $cmakeListsContent '/MP${MK_MSVC_MULTIPROCESSOR_COMPILE_PROCESSES}' "CMakeLists.txt"
Assert-ContainsText $cmakeListsContent '/Zf' "CMakeLists.txt"
Assert-ContainsText $cmakeListsContent 'COMPILE_PDB_NAME ${target_name}' "CMakeLists.txt"
Assert-ContainsText $cmakeListsContent 'COMPILE_PDB_OUTPUT_DIRECTORY' "CMakeLists.txt"
Assert-ContainsText $cmakeListsContent 'COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/pdb"' "CMakeLists.txt"
foreach ($compactTestTarget in @(
    "MK_rt_pkg_cand_res_mnt_review_evict_tests",
    "MK_rt_pkg_cand_res_repl_review_evict_tests",
    "MK_rt_pkg_disc_res_mnt_review_evict_tests",
    "MK_rt_pkg_disc_res_repl_review_evict_tests",
    "MK_rt_pkg_hot_reload_repl_intent_review_tests"
)) {
    Assert-ContainsText $cmakeListsContent $compactTestTarget "CMakeLists.txt compact MSVC test target names"
}
Assert-ContainsText $cmakeListsContent 'get_target_property(MK_TARGET_TYPE ${target_name} TYPE)' "CMakeLists.txt"
Assert-ContainsText $cmakeListsContent 'MK_TARGET_TYPE STREQUAL "EXECUTABLE"' "CMakeLists.txt"
Assert-ContainsText $cmakeListsContent 'MK_TARGET_TYPE STREQUAL "SHARED_LIBRARY"' "CMakeLists.txt"
Assert-ContainsText $cmakeListsContent 'MK_TARGET_TYPE STREQUAL "MODULE_LIBRARY"' "CMakeLists.txt"
Assert-ContainsText $cmakeListsContent 'target_link_options(${target_name} PRIVATE /INCREMENTAL:NO)' "CMakeLists.txt"
$packageDesktopRuntimeContent = Get-AgentSurfaceText "tools/package-desktop-runtime.ps1"
Assert-ContainsText $cmakeListsContent "MK_desktop_runtime_package_build" "CMakeLists.txt"
Assert-ContainsText $packageDesktopRuntimeContent "--target MK_desktop_runtime_package_build" "tools/package-desktop-runtime.ps1"
Assert-ContainsText (Get-AgentSurfaceText "docs/workflows.md") "MK_desktop_runtime_package_build" "docs/workflows.md"
foreach ($cmakeSkillPath in @(
    ".agents/skills/cmake-build-system/SKILL.md",
    ".claude/skills/gameengine-cmake-build-system/SKILL.md"
)) {
    Assert-ContainsText (Get-AgentSurfaceText $cmakeSkillPath) "/INCREMENTAL:NO" $cmakeSkillPath
    Assert-ContainsText (Get-AgentSurfaceText $cmakeSkillPath) "COMPILE_PDB_OUTPUT_DIRECTORY" $cmakeSkillPath
    Assert-ContainsText (Get-AgentSurfaceText $cmakeSkillPath) "compact CMake target names" $cmakeSkillPath
    Assert-ContainsText (Get-AgentSurfaceText $cmakeSkillPath) "/MP2" $cmakeSkillPath
    Assert-ContainsText (Get-AgentSurfaceText $cmakeSkillPath) "/Zf" $cmakeSkillPath
    Assert-ContainsText (Get-AgentSurfaceText $cmakeSkillPath) "-ShardCount" $cmakeSkillPath
    Assert-ContainsText (Get-AgentSurfaceText $cmakeSkillPath) "-ShardIndex" $cmakeSkillPath
    Assert-ContainsText (Get-AgentSurfaceText $cmakeSkillPath) "MSB8028" $cmakeSkillPath
    Assert-ContainsText (Get-AgentSurfaceText $cmakeSkillPath) 'Do not repair generated `out/build/<preset>` trees' $cmakeSkillPath
    Assert-ContainsText (Get-AgentSurfaceText $cmakeSkillPath) "CMakeCache.txt" $cmakeSkillPath
}
foreach ($buildFixerPath in @(".codex/agents/build-fixer.toml", ".claude/agents/build-fixer.md")) {
    $buildFixerAgentText = Get-AgentSurfaceText $buildFixerPath
    Assert-ContainsText $buildFixerAgentText "tools/check-toolchain.ps1" $buildFixerPath
    Assert-ContainsText $buildFixerAgentText "tools/cmake.ps1" $buildFixerPath
    Assert-ContainsText $buildFixerAgentText "tools/ctest.ps1" $buildFixerPath
    Assert-ContainsText $buildFixerAgentText "tools/check-format.ps1" $buildFixerPath
    Assert-ContainsText $buildFixerAgentText "tools/check-tidy.ps1" $buildFixerPath
    Assert-ContainsText $buildFixerAgentText 'Do not repair generated `out/build/<preset>` trees' $buildFixerPath
    Assert-ContainsText $buildFixerAgentText "CMakeCache.txt" $buildFixerPath
    Assert-ContainsText $buildFixerAgentText "C1041" $buildFixerPath
    Assert-ContainsText $buildFixerAgentText "COMPILE_PDB_OUTPUT_DIRECTORY" $buildFixerPath
    Assert-ContainsText $buildFixerAgentText 'source-basename `.obj` path' $buildFixerPath
    Assert-ContainsText $buildFixerAgentText "/MP2" $buildFixerPath
    Assert-ContainsText $buildFixerAgentText "/Zf" $buildFixerPath
    Assert-ContainsText $buildFixerAgentText 'global `/FS`' $buildFixerPath
    Assert-ContainsText $buildFixerAgentText "MSB8028" $buildFixerPath
    Assert-ContainsText $buildFixerAgentText ".lastbuildstate" $buildFixerPath
    Assert-ContainsText $buildFixerAgentText "LinkIncremental=false" $buildFixerPath
    Assert-ContainsText $buildFixerAgentText "gh pr view <pr> --json headRefOid,statusCheckRollup,url" $buildFixerPath
    Assert-ContainsText $buildFixerAgentText "billing or account limits" $buildFixerPath
}
$cursorCmakeRuleText = Get-AgentSurfaceText ".cursor/rules/mirakana-cmake-vcpkg.mdc"
Assert-ContainsText $cursorCmakeRuleText "COMPILE_PDB_OUTPUT_DIRECTORY" ".cursor/rules/mirakana-cmake-vcpkg.mdc"
Assert-ContainsText $cursorCmakeRuleText "compact CMake target names" ".cursor/rules/mirakana-cmake-vcpkg.mdc"
Assert-ContainsText $cursorCmakeRuleText "/MP2" ".cursor/rules/mirakana-cmake-vcpkg.mdc"
Assert-ContainsText $cursorCmakeRuleText "/Zf" ".cursor/rules/mirakana-cmake-vcpkg.mdc"
Assert-ContainsText $cursorCmakeRuleText 'global `/FS`' ".cursor/rules/mirakana-cmake-vcpkg.mdc"
Assert-ContainsText $cursorCmakeRuleText ".lastbuildstate" ".cursor/rules/mirakana-cmake-vcpkg.mdc"
Assert-ContainsText $manifestRaw "/INCREMENTAL:NO" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "msvcCompileThroughput" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "/MP2" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "/Zf" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "msvcCompilePdbIsolation" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "msvcObjectPathHygiene" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw ".lastbuildstate" "engine/agent/manifest.json"
Assert-ContainsText (Get-AgentSurfaceText "tools/common.ps1") 'ToolId "cmake-build"' "tools/common.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/common.ps1") 'Test-CMakeBuildCommand' "tools/common.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/common.ps1") 'Clear-StaleMsvcLastBuildState' "tools/common.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/common.ps1") '*.lastbuildstate' "tools/common.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/validate.ps1") "-MaxFiles 1" "tools/validate.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/validate.ps1") '"test.ps1") -SkipBuild' "tools/validate.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/validate.ps1") "-ReuseExistingFileApiReply" "tools/validate.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/validate.ps1") '[int]$StaticJobs = 0' "tools/validate.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/validate.ps1") '[switch]$StaticOnly' "tools/validate.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/validate.ps1") '[switch]$SkipStaticChecks' "tools/validate.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/validate.ps1") '[switch]$SkipTidySmoke' "tools/validate.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/validate.ps1") 'validate: -StaticOnly and -SkipStaticChecks cannot be combined' "tools/validate.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/validate.ps1") 'Invoke-ValidateBackgroundJob' "tools/validate.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/validate.ps1") 'Start-ThreadJob' "tools/validate.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/validate.ps1") 'independent static checks' "tools/validate.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/validate.ps1") 'Get-ValidateOutputCapture' "tools/validate.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/validate.ps1") '[int]$StaticCheckTimeoutSeconds = 1800' "tools/validate.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/validate.ps1") 'validate: timed out after ${TaskTimeoutSeconds}s' "tools/validate.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/validate.ps1") 'OutputLogPath' "tools/validate.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/validate.ps1") 'OmittedOutputLineCount' "tools/validate.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/validate.ps1") 'out" (Join-Path "validation-logs"' "tools/validate.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/validate.ps1") 'validate: parallel static check logs' "tools/validate.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/check-ai-integration-030-runtime-rendering.ps1") '$historicalVerdictArchiveText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"' "tools/check-ai-integration-030-runtime-rendering.ps1"
Assert-DoesNotContainText (Get-AgentSurfaceText "tools/check-ai-integration-030-runtime-rendering.ps1") 'Get-Content -Raw "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"' "tools/check-ai-integration-030-runtime-rendering.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/build.ps1") 'New-CMakeFileApiCodemodelQuery' "tools/build.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/build.ps1") '[int]$Jobs = 0' "tools/build.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/build.ps1") '--parallel' "tools/build.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/test.ps1") '[switch]$SkipBuild' "tools/test.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/test.ps1") '[int]$Jobs = 0' "tools/test.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/test.ps1") '--parallel' "tools/test.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/check-tidy.ps1") 'try existing File API codemodel reply before reconfiguring' "tools/check-tidy.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/check-tidy.ps1") 'Test-ClangTidyIgnoredMsvcCompileOption' "tools/check-tidy.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/check-tidy.ps1") '^/MP(?:\d+)?$' "tools/check-tidy.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/check-tidy.ps1") 'Token -eq "/Zf"' "tools/check-tidy.ps1"
Assert-ContainsText $manifestRaw "test.ps1 -SkipBuild" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "automatic CMake/CTest parallelism" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "bounded parallel jobs" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "-StaticJobs" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "validate.ps1 -StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "validate.ps1 -SkipStaticChecks" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "-SkipTidySmoke" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "actions/cache/restore" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "actions/cache/save" "engine/agent/manifest.json"
Assert-ContainsText (Get-AgentSurfaceText "tools/cmake.ps1") 'Invoke-CheckedCommand $tools.CMake @Arguments' "tools/cmake.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/ctest.ps1") 'Invoke-CheckedCommand $tools.CTest @Arguments' "tools/ctest.ps1"
Assert-ContainsText $agentsContent "automatic CMake/CTest parallelism" "AGENTS.md"
Assert-ContainsText $agentsContent "bounded static jobs" "AGENTS.md"
Assert-ContainsText $agentsContent '`Agent Static Guards` runs `validate.ps1 -StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120`' "AGENTS.md"
Assert-ContainsText $agentsContent "actions/cache/restore" "AGENTS.md"
Assert-ContainsText $agentsContent "actions/cache/save" "AGENTS.md"
Assert-ContainsText $testingContent "automatic CMake/CTest parallelism" "docs/testing.md"
Assert-ContainsText $testingContent "bounded PowerShell background jobs" "docs/testing.md"
Assert-ContainsText $testingContent "tools/validate.ps1 -SkipStaticChecks" "docs/testing.md"
Assert-ContainsText $testingContent "-SkipTidySmoke" "docs/testing.md"
Assert-ContainsText $testingContent "actions/cache/restore" "docs/testing.md"
Assert-ContainsText $testingContent "actions/cache/save" "docs/testing.md"
Assert-ContainsText (Get-AgentSurfaceText "docs/workflows.md") "automatic CMake/CTest parallelism" "docs/workflows.md"
Assert-ContainsText (Get-AgentSurfaceText "docs/workflows.md") "bounded parallel group" "docs/workflows.md"
Assert-ContainsText (Get-AgentSurfaceText "docs/workflows.md") "tools/validate.ps1 -StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120" "docs/workflows.md"
Assert-ContainsText (Get-AgentSurfaceText "docs/workflows.md") "tools/validate.ps1 -SkipStaticChecks" "docs/workflows.md"
Assert-ContainsText (Get-AgentSurfaceText "docs/workflows.md") "-SkipTidySmoke" "docs/workflows.md"
Assert-ContainsText (Get-AgentSurfaceText "docs/workflows.md") "actions/cache/restore" "docs/workflows.md"
Assert-ContainsText (Get-AgentSurfaceText "docs/workflows.md") "actions/cache/save" "docs/workflows.md"
Assert-ContainsText (Get-AgentSurfaceText ".agents/skills/cmake-build-system/SKILL.md") "automatic CMake/CTest parallelism" ".agents/skills/cmake-build-system/SKILL.md"
Assert-ContainsText (Get-AgentSurfaceText ".agents/skills/cmake-build-system/SKILL.md") "bounded parallel jobs" ".agents/skills/cmake-build-system/SKILL.md"
Assert-ContainsText (Get-AgentSurfaceText ".agents/skills/cmake-build-system/SKILL.md") "-SkipStaticChecks" ".agents/skills/cmake-build-system/SKILL.md"
Assert-ContainsText (Get-AgentSurfaceText ".agents/skills/cmake-build-system/SKILL.md") "-SkipTidySmoke" ".agents/skills/cmake-build-system/SKILL.md"
Assert-ContainsText (Get-AgentSurfaceText ".agents/skills/cmake-build-system/SKILL.md") "actions/cache/restore" ".agents/skills/cmake-build-system/SKILL.md"
Assert-ContainsText (Get-AgentSurfaceText ".agents/skills/cmake-build-system/SKILL.md") "actions/cache/save" ".agents/skills/cmake-build-system/SKILL.md"
Assert-ContainsText (Get-AgentSurfaceText ".claude/skills/gameengine-cmake-build-system/SKILL.md") "automatic CMake/CTest parallelism" ".claude/skills/gameengine-cmake-build-system/SKILL.md"
Assert-ContainsText (Get-AgentSurfaceText ".claude/skills/gameengine-cmake-build-system/SKILL.md") "bounded parallel jobs" ".claude/skills/gameengine-cmake-build-system/SKILL.md"
Assert-ContainsText (Get-AgentSurfaceText ".claude/skills/gameengine-cmake-build-system/SKILL.md") "-SkipStaticChecks" ".claude/skills/gameengine-cmake-build-system/SKILL.md"
Assert-ContainsText (Get-AgentSurfaceText ".claude/skills/gameengine-cmake-build-system/SKILL.md") "-SkipTidySmoke" ".claude/skills/gameengine-cmake-build-system/SKILL.md"
Assert-ContainsText (Get-AgentSurfaceText ".claude/skills/gameengine-cmake-build-system/SKILL.md") "actions/cache/restore" ".claude/skills/gameengine-cmake-build-system/SKILL.md"
Assert-ContainsText (Get-AgentSurfaceText ".claude/skills/gameengine-cmake-build-system/SKILL.md") "actions/cache/save" ".claude/skills/gameengine-cmake-build-system/SKILL.md"
Assert-ContainsText (Get-AgentSurfaceText "tools/check-ci-matrix.ps1") "actions/cache/restore@" "tools/check-ci-matrix.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/check-ci-matrix.ps1") "actions/cache/save@" "tools/check-ci-matrix.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/check-ci-matrix.ps1") "github.sha" "tools/check-ci-matrix.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/check-ci-matrix.ps1") "cache-primary-key" "tools/check-ci-matrix.ps1"
$cmakeListsContent = Get-AgentSurfaceText "CMakeLists.txt"
Assert-ContainsText $cmakeListsContent 'set_tests_properties(MK_platform_process_tests PROPERTIES RUN_SERIAL TRUE)' "CMakeLists.txt"
Assert-ContainsText $cmakeListsContent 'set_tests_properties(MK_d3d12_rhi_tests PROPERTIES RUN_SERIAL TRUE)' "CMakeLists.txt"
Assert-ContainsText (Get-AgentSurfaceText "tools/check-agents.ps1") 'pattern\s*=\s*\["gh",\s*"pr",\s*"view"\]' "tools/check-agents.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/check-agents.ps1") 'pattern\s*=\s*\["gh",\s*"pr",\s*"merge",\s*"--auto",\s*"--merge",\s*"--match-head-commit"\]' "tools/check-agents.ps1"
$tidyWrapperContent = Get-AgentSurfaceText "tools/check-tidy.ps1"
Assert-ContainsText $tidyWrapperContent '[string[]]$Files' "tools/check-tidy.ps1"
Assert-ContainsText $tidyWrapperContent '[int]$Jobs' "tools/check-tidy.ps1"
Assert-ContainsText $tidyWrapperContent '[int]$ShardCount' "tools/check-tidy.ps1"
Assert-ContainsText $tidyWrapperContent '[int]$ShardIndex' "tools/check-tidy.ps1"
Assert-ContainsText $tidyWrapperContent 'tidy-check: shard' "tools/check-tidy.ps1"
Assert-ContainsText $tidyWrapperContent "--warnings-as-errors=*" "tools/check-tidy.ps1"
Assert-ContainsText $tidyWrapperContent "ForEach-Object -Parallel" "tools/check-tidy.ps1"
$clangTidyContent = Get-AgentSurfaceText ".clang-tidy"
Assert-ContainsText $clangTidyContent '(^|.*[/\\])(engine|editor|examples|games|tests)[/\\].*' ".clang-tidy"
Assert-DoesNotContainText $clangTidyContent "-performance-enum-size" ".clang-tidy"
Assert-ContainsText $testingContent "--warnings-as-errors=*" "docs/testing.md"
Assert-ContainsText $testingContent "-Jobs" "docs/testing.md"
Assert-ContainsText $testingContent "HeaderFilterRegex" "docs/testing.md"
Assert-ContainsText $testingContent "workflow concurrency" "docs/testing.md"
Assert-ContainsText $testingContent "GITHUB_TOKEN permissions" "docs/testing.md"
Assert-ContainsText $testingContent "PR check selection policy" "docs/testing.md"
Assert-ContainsText $testingContent "always-running aggregate gate" "docs/testing.md"
Assert-ContainsText $testingContent "main push, release, scheduled/nightly, and workflow_dispatch" "docs/testing.md"
Assert-ContainsText $workflowsContent "--warnings-as-errors=*" "docs/workflows.md"
Assert-ContainsText $workflowsContent "-Jobs" "docs/workflows.md"
$agentOperationalReferenceContent = Get-AgentSurfaceText "docs/agent-operational-reference.md"
Assert-ContainsText $agentOperationalReferenceContent "HeaderFilterRegex" "docs/agent-operational-reference.md"
Assert-ContainsText $agentOperationalReferenceContent "NN warnings generated." "docs/agent-operational-reference.md"
Assert-ContainsText $agentOperationalReferenceContent "bounded PowerShell background jobs" "docs/agent-operational-reference.md"
Assert-ContainsText $agentOperationalReferenceContent "tools/new-game-templates.ps1" "docs/agent-operational-reference.md"
foreach ($windowsDiagnosticsNeedle in @("Debugging Tools for Windows", "PIX on Windows", "Windows Performance Toolkit", "Tools.Graphics.DirectX~~~~0.0.1.0", "d3d12SDKLayers.dll", "cdb -version", "pixtool --help", "wpr -help", "xperf -help")) {
    Assert-ContainsText $testingContent $windowsDiagnosticsNeedle "docs/testing.md"
}
$dependenciesContent = Get-Content -LiteralPath $dependenciesPath -Raw
foreach ($windowsDiagnosticsNeedle in @("Windows Host Diagnostics Tooling", "Debugging Tools for Windows", "Windows Graphics Tools", "PIX on Windows", "Windows Performance Toolkit", "OptionId.WindowsDesktopDebuggers", "Tools.Graphics.DirectX~~~~0.0.1.0", "OptionId.WindowsPerformanceToolkit", "ADK servicing patches")) {
    Assert-ContainsText $dependenciesContent $windowsDiagnosticsNeedle "docs/dependencies.md"
}
$legalContent = Get-Content -LiteralPath $legalPath -Raw
foreach ($windowsDiagnosticsNeedle in @("Debugging Tools for Windows", "Windows Graphics Tools", "PIX on Windows", "Windows Performance Toolkit")) {
    Assert-ContainsText $legalContent $windowsDiagnosticsNeedle "docs/legal-and-licensing.md"
}
$planRegistryContent = Get-Content -LiteralPath $planRegistryPath -Raw
Assert-ContainsText $planRegistryContent "Active milestone" "docs/superpowers/plans/README.md"
foreach ($planVolumeNeedle in @("Plan Volume Policy", "live execution stack", "capability/gap-cluster/milestone", "Plan width is wider than PR width", "phase behavior/API/validation boundary", "Distinguish plan files from execution steps", "validation-only follow-up", "Git history", "Historical/static-check retained literals")) {
    Assert-ContainsText $planRegistryContent $planVolumeNeedle "docs/superpowers/plans/README.md"
}
Assert-ProductionCompletionCorpus
Assert-SpecStatusSection

$aiIntegrationContent = Get-AgentSurfaceText "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "Codex rules: https://developers.openai.com/codex/rules" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "git commit" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "gh pr view" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "gh pr" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "gh pr merge --auto --merge --match-head-commit <headRefOid>" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "guarded merged-worktree cleanup" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "official GitHub Flow" "docs/ai-integration.md"
foreach ($aiIntegrationCadenceNeedle in @("purpose/checkpoint-based", "one PR per focused capability/gap-cluster/milestone", "never open PRs per commit or checklist item")) {
    Assert-ContainsText $aiIntegrationContent $aiIntegrationCadenceNeedle "docs/ai-integration.md"
}
Assert-ContainsText $aiIntegrationContent "Direct default-branch pushes are blocked" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "mergeStateStatus" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "selected hosted checks to complete" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent '`PR Gate` to report `SUCCESS`' "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent '--match-head-commit <headRefOid>' "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent 'stale `headRefOid` invalidates the merge decision' "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "commits pushed after a PR merged need a new PR" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent 'latest `headRefOid` and `statusCheckRollup`' "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent '.codex/rules` and `.claude/settings.json` remain command/permission gates' "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "lightweight static validation for docs/agent/rules/subagent-only PRs" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "unrelated Windows/MSVC, macOS, or full repository clang-tidy lanes" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "GitHub account billing/spending-limit" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "policy reload" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "GITHUB_TOKEN" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "credential-manager-core" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "approval-capable session" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "Codex app Worktree/Handoff" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/remove-merged-worktree.ps1" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "local checkout sync plus cleanup" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "Git's main worktree porcelain record" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "Windows long-path deletion fallback inside the guarded script" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "without following reparse points" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent 'worktree.baseRef = "head"' "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "Cursor global instructions" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "OpenAI developer docs MCP" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "Claude Code settings and permissions: https://docs.anthropic.com/en/docs/claude-code/settings" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "windowsDiagnosticsToolchain" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "Debugging Tools for Windows" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "Windows Performance Toolkit" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent ".claude/settings.local.json" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent ".mcp.json" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "AGENTS.override.md" "docs/ai-integration.md"

$cursorBaselineSkillText = Get-AgentSurfaceText ".cursor/skills/gameengine-cursor-baseline/SKILL.md"
Assert-ContainsText $cursorBaselineSkillText "Cursor global instructions" ".cursor/skills/gameengine-cursor-baseline/SKILL.md"
Assert-ContainsText $cursorBaselineSkillText "workspace override" ".cursor/skills/gameengine-cursor-baseline/SKILL.md"
Assert-ContainsText $cursorBaselineSkillText "official GitHub Flow" ".cursor/skills/gameengine-cursor-baseline/SKILL.md"
Assert-ContainsText $cursorBaselineSkillText "tools/check-publication-preflight.ps1" ".cursor/skills/gameengine-cursor-baseline/SKILL.md"
Assert-ContainsText $cursorBaselineSkillText "publication-preflight: blocked" ".cursor/skills/gameengine-cursor-baseline/SKILL.md"
foreach ($cursorBaselineCadenceNeedle in @("purpose/checkpoint-based", "push validated checkpoints", "one PR per focused capability/gap-cluster/milestone", "avoid PRs per commit/checklist item")) {
    Assert-ContainsText $cursorBaselineSkillText $cursorBaselineCadenceNeedle ".cursor/skills/gameengine-cursor-baseline/SKILL.md"
}
Assert-ContainsText $cursorBaselineSkillText "selected hosted checks to complete" ".cursor/skills/gameengine-cursor-baseline/SKILL.md"
Assert-ContainsText $cursorBaselineSkillText '`PR Gate` to report `SUCCESS`' ".cursor/skills/gameengine-cursor-baseline/SKILL.md"
Assert-ContainsText $cursorBaselineSkillText "mergeStateStatus" ".cursor/skills/gameengine-cursor-baseline/SKILL.md"
Assert-ContainsText $cursorBaselineSkillText "--match-head-commit <headRefOid>" ".cursor/skills/gameengine-cursor-baseline/SKILL.md"
Assert-ContainsText $cursorBaselineSkillText 'stale `headRefOid` invalidates the merge decision' ".cursor/skills/gameengine-cursor-baseline/SKILL.md"
Assert-ContainsText $cursorBaselineSkillText "tools/remove-merged-worktree.ps1" ".cursor/skills/gameengine-cursor-baseline/SKILL.md"
Assert-ContainsText $cursorBaselineSkillText "Windows long-path fallback inside the guarded script" ".cursor/skills/gameengine-cursor-baseline/SKILL.md"
Assert-ContainsText $cursorBaselineSkillText "tools/check-text-format.ps1" ".cursor/skills/gameengine-cursor-baseline/SKILL.md"
Assert-ContainsText $cursorBaselineSkillText "docs/agent/rules/subagent-only PRs should use lightweight static validation" ".cursor/skills/gameengine-cursor-baseline/SKILL.md"
Assert-AllCursorThinRouterSkills
$cursorAgentIntegrationSkillText = Get-Content -LiteralPath (Resolve-RequiredAgentPath ".cursor/skills/gameengine-agent-integration/SKILL.md") -Raw

$codexAgentIntegrationSkillText = Get-AgentSurfaceText ".agents/skills/gameengine-agent-integration/SKILL.md"
$claudeAgentIntegrationSkillText = Get-AgentSurfaceText ".claude/skills/gameengine-agent-integration/SKILL.md"
foreach ($agentSkillSurface in @(
        @{ Text = $codexAgentIntegrationSkillText; Label = ".agents/skills/gameengine-agent-integration/SKILL.md" },
        @{ Text = $claudeAgentIntegrationSkillText; Label = ".claude/skills/gameengine-agent-integration/SKILL.md" },
        @{ Text = $cursorAgentIntegrationSkillText; Label = ".cursor/skills/gameengine-agent-integration/SKILL.md" }
    )) {
    Assert-ContainsText $agentSkillSurface.Text "tools/check-publication-preflight.ps1" $agentSkillSurface.Label
    Assert-ContainsText $agentSkillSurface.Text "publication-preflight: blocked" $agentSkillSurface.Label
    Assert-ContainsText $agentSkillSurface.Text "before staging/push/PR/merge" $agentSkillSurface.Label
}

$codexRulesText = Get-AgentSurfaceText ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRulesText '"tools/check-publication-preflight.ps1"' ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRulesText "linked-worktree index.lock ACL" ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRulesText 'pattern = ["git", "clone"]' ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRulesText "Publication temp clones bypass the repository preflight path" ".codex/rules/gameengine.rules"
$claudeRulesText = Get-AgentSurfaceText ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $claudeRulesText "tools/check-publication-preflight.ps1" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $claudeRulesText "publication-preflight: blocked" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $claudeRulesText "Publication temp clones" ".claude/rules/ai-agent-integration.md"
$claudeSettingsText = Get-AgentSurfaceText ".claude/settings.json"
Assert-ContainsText $claudeSettingsText "Bash(pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1:*)" ".claude/settings.json"
Assert-ContainsText $claudeSettingsText "Bash(git clone:*)" ".claude/settings.json"

Assert-ContainsText $aiIntegrationContent "normalized-configure-environment" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "normalized-build-environment" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent 'Path`/`PATH' "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "Instruction Hygiene" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "specific, concise, verifiable" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "Personal preferences, credentials" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "agent-surface drift check" "docs/ai-integration.md"
Assert-ContainsText $aiIntegrationContent "Keep drift checks targeted" "docs/ai-integration.md"
foreach ($planVolumeNeedle in @("live plan stack shallow", "active gap-cluster burn-down or milestone", "capability/gap-cluster/milestone", "Plan-file width can be broader than PR/phase width", "phase behavior/API/validation boundary", "validation-only follow-up", "Completed plans are retained only while referenced")) {
    Assert-ContainsText $aiIntegrationContent $planVolumeNeedle "docs/ai-integration.md"
}
Assert-DoesNotContainText $aiIntegrationContent "code.claude.com/docs" "docs/ai-integration.md"

$gitignoreContent = Get-AgentSurfaceText ".gitignore"
foreach ($ignoredLocalFile in @("AGENTS.override.md", ".claude/settings.local.json", ".mcp.json")) {
    Assert-ContainsText $gitignoreContent $ignoredLocalFile ".gitignore"
}

$gameNamingGuidanceFiles = @(
    "AGENTS.md",
    ".agents/skills/gameengine-agent-integration/SKILL.md",
    ".claude/skills/gameengine-agent-integration/SKILL.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md",
    ".codex/agents/gameplay-builder.toml",
    ".claude/agents/gameplay-builder.md",
    ".claude/rules/ai-agent-integration.md"
)
foreach ($gameNamingGuidanceFile in $gameNamingGuidanceFiles) {
    $gameNamingGuidanceText = Get-AgentSurfaceText $gameNamingGuidanceFile
    Assert-ContainsText $gameNamingGuidanceText '^[a-z][a-z0-9_]*$' $gameNamingGuidanceFile
    Assert-ContainsText $gameNamingGuidanceText "lowercase snake_case" $gameNamingGuidanceFile
    Assert-ContainsText $gameNamingGuidanceText "JSON manifest IDs" $gameNamingGuidanceFile
    Assert-ContainsText $gameNamingGuidanceText "kebab-case" $gameNamingGuidanceFile
}

$manifest = $manifestRaw | ConvertFrom-Json
foreach ($field in @("schemaVersion", "engine", "commands", "windowsDiagnosticsToolchain", "modules", "runtimeBackendReadiness", "importerCapabilities", "packagingTargets", "validationRecipes", "aiOperableProductionLoop", "aiSurfaces")) {
    if (-not $manifest.PSObject.Properties.Name.Contains($field)) {
        Write-Error "engine/agent/manifest.json missing required field: $field"
    }
}
foreach ($field in @("stageStatus", "stageCompletion", "stageClosurePlan", "stageClosureNotes")) {
    if (-not $manifest.engine.PSObject.Properties.Name.Contains($field)) {
        Write-Error "engine/agent/manifest.json engine missing required MVP closure field: $field"
    }
}
if ($manifest.engine.stage -ne "core-first-mvp") {
    Write-Error "engine/agent/manifest.json engine.stage must remain core-first-mvp for the closed MVP foundation"
}
if ($manifest.engine.stageStatus -ne "mvp-closed-not-commercial-complete") {
    Write-Error "engine/agent/manifest.json engine.stageStatus must be mvp-closed-not-commercial-complete"
}
if (-not ([string]$manifest.engine.stageCompletion).Contains("not a commercial-engine completion claim")) {
    Write-Error "engine/agent/manifest.json engine.stageCompletion must distinguish MVP closure from commercial engine completion"
}
Resolve-RequiredAgentPath $manifest.engine.stageClosurePlan | Out-Null
if (-not ((@($manifest.engine.stageClosureNotes) -join " ").Contains("Apple/iOS/Metal"))) {
    Write-Error "engine/agent/manifest.json engine.stageClosureNotes must keep Apple/iOS/Metal host gating explicit"
}
if (-not $manifest.commands.PSObject.Properties.Name.Contains("bootstrapDeps")) {
    Write-Error "engine/agent/manifest.json commands missing required command: bootstrapDeps"
} elseif ($manifest.commands.bootstrapDeps -ne "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1") {
    Write-Error "engine/agent/manifest.json commands.bootstrapDeps must be pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1"
}
if (-not $manifest.commands.PSObject.Properties.Name.Contains("prepareWorktree")) {
    Write-Error "engine/agent/manifest.json commands missing required command: prepareWorktree"
} elseif ($manifest.commands.prepareWorktree -ne "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1") {
    Write-Error "engine/agent/manifest.json commands.prepareWorktree must be pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1"
}
if (-not $manifest.commands.PSObject.Properties.Name.Contains("removeMergedWorktree")) {
    Write-Error "engine/agent/manifest.json commands missing required command: removeMergedWorktree"
} elseif ($manifest.commands.removeMergedWorktree -ne "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/remove-merged-worktree.ps1 -WorktreePath <path> [-BaseRef origin/main] [-BaseBranch main] [-Remote origin] [-LocalCheckoutPath <path>] [-DeleteLocalBranch] [-DeleteRemoteBranch]") {
    Write-Error "engine/agent/manifest.json commands.removeMergedWorktree must expose the guarded post-merge worktree cleanup command"
}
$composeAgentManifestCmd = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 [-Write|-Verify|-SplitFromCanonical]"
if (-not $manifest.commands.PSObject.Properties.Name.Contains("composeAgentManifest")) {
    Write-Error "engine/agent/manifest.json commands missing required command: composeAgentManifest"
} elseif ($manifest.commands.composeAgentManifest -ne $composeAgentManifestCmd) {
    Write-Error "engine/agent/manifest.json commands.composeAgentManifest must be $composeAgentManifestCmd"
}
if (-not $manifest.commands.PSObject.Properties.Name.Contains("textFormat")) {
    Write-Error "engine/agent/manifest.json commands missing required command: textFormat"
} elseif ($manifest.commands.textFormat -ne "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format-text.ps1") {
    Write-Error "engine/agent/manifest.json commands.textFormat must expose tools/format-text.ps1"
}
if (-not $manifest.commands.PSObject.Properties.Name.Contains("textFormatCheck")) {
    Write-Error "engine/agent/manifest.json commands missing required command: textFormatCheck"
} elseif ($manifest.commands.textFormatCheck -ne "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1") {
    Write-Error "engine/agent/manifest.json commands.textFormatCheck must expose tools/check-text-format.ps1"
}
if (-not $manifest.commands.PSObject.Properties.Name.Contains("toolchainCheck")) {
    Write-Error "engine/agent/manifest.json commands missing required command: toolchainCheck"
} elseif ($manifest.commands.toolchainCheck -ne "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1") {
    Write-Error "engine/agent/manifest.json commands.toolchainCheck must be pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1"
}
if (-not $manifest.commands.PSObject.Properties.Name.Contains("directToolchainCheck")) {
    Write-Error "engine/agent/manifest.json commands missing required command: directToolchainCheck"
} elseif ($manifest.commands.directToolchainCheck -ne "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1 -RequireDirectCMake") {
    Write-Error "engine/agent/manifest.json commands.directToolchainCheck must be pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1 -RequireDirectCMake"
}
if (-not $manifest.commands.PSObject.Properties.Name.Contains("validatePhysicsJolt")) {
    Write-Error "engine/agent/manifest.json commands missing required command: validatePhysicsJolt"
} elseif ($manifest.commands.validatePhysicsJolt -ne "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-physics-jolt.ps1") {
    Write-Error "engine/agent/manifest.json commands.validatePhysicsJolt must expose tools/validate-physics-jolt.ps1"
}
if (-not $manifest.commands.PSObject.Properties.Name.Contains("validateNetworkEnet")) {
    Write-Error "engine/agent/manifest.json commands missing required command: validateNetworkEnet"
} elseif ($manifest.commands.validateNetworkEnet -ne "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-network-enet.ps1") {
    Write-Error "engine/agent/manifest.json commands.validateNetworkEnet must expose tools/validate-network-enet.ps1"
}
Assert-ContainsText $manifestRaw "VCPKG_MANIFEST_INSTALL=OFF" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "CMakeCache.txt" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "officialAiToolDocs" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "OpenAI developer documentation MCP" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "direct-clang-format-status" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "CMake File API codemodel" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "tools/check-text-format.ps1" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "tools/format-text.ps1" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "Renderer RHI Resource Foundation 1.0 Scope Closeout v1" "engine/agent/manifest.json"
foreach ($windowsDiagnosticsNeedle in @("windowsDiagnosticsToolchain", "Debugging Tools for Windows", "Windows Graphics Tools", "PIX on Windows", "Windows Performance Toolkit", "Tools.Graphics.DirectX~~~~0.0.1.0", "d3d12SDKLayers.dll", "pixtool --help", "srv*C:\\Symbols*https://msdl.microsoft.com/download/symbols")) {
    Assert-ContainsText $manifestRaw $windowsDiagnosticsNeedle "engine/agent/manifest.json"
}
if ($manifest.windowsDiagnosticsToolchain.graphicsTools.source -ne "Windows optional capability Tools.Graphics.DirectX~~~~0.0.1.0") {
    Write-Error "engine/agent/manifest.json windowsDiagnosticsToolchain.graphicsTools.source must name the Windows Graphics Tools capability"
}
if (@($manifest.windowsDiagnosticsToolchain.debuggingToolsForWindows.verify) -notcontains "cdb -version") {
    Write-Error "engine/agent/manifest.json windowsDiagnosticsToolchain.debuggingToolsForWindows.verify must include cdb -version"
}
if (@($manifest.windowsDiagnosticsToolchain.pixOnWindows.verify) -notcontains "pixtool --help") {
    Write-Error "engine/agent/manifest.json windowsDiagnosticsToolchain.pixOnWindows.verify must include pixtool --help"
}
if ($manifest.aiSurfaces.codex.rules -ne ".codex/rules") {
    Write-Error "engine/agent/manifest.json aiSurfaces.codex.rules must be .codex/rules"
}
if ($manifest.aiSurfaces.claudeCode.settings -ne ".claude/settings.json") {
    Write-Error "engine/agent/manifest.json aiSurfaces.claudeCode.settings must be .claude/settings.json"
}
if ($manifest.aiSurfaces.cursor.rules -ne ".cursor/rules") {
    Write-Error "engine/agent/manifest.json aiSurfaces.cursor.rules must be .cursor/rules"
}
if ($manifest.aiSurfaces.cursor.agents -ne ".cursor/agents") {
    Write-Error "engine/agent/manifest.json aiSurfaces.cursor.agents must be .cursor/agents"
}
if ($manifest.aiSurfaces.cursor.PSObject.Properties.Name.Contains("compatibleAgentRoots")) {
    Write-Error "engine/agent/manifest.json aiSurfaces.cursor must not rely on compatibleAgentRoots; keep Cursor project subagents native under .cursor/agents"
}
foreach ($importPath in @("AGENTS.md", ".claude/rules/ai-agent-integration.md", ".claude/rules/cpp-engine.md")) {
    if (@($manifest.aiSurfaces.claudeCode.memoryImports) -notcontains $importPath) {
        Write-Error "engine/agent/manifest.json aiSurfaces.claudeCode.memoryImports missing $importPath"
    }
}
Assert-ContainsText $gameAgentSchemaText '"runtimeSceneValidationTargets"' "schemas/game-agent.schema.json"
Assert-ContainsText (Get-AgentSurfaceText "schemas/engine-agent.schema.json") '"windowsDiagnosticsToolchain"' "schemas/engine-agent.schema.json"
Assert-ContainsText (Get-AgentSurfaceText "schemas/engine-agent.schema.json") "engine-agent/ai-operable-production-loop.schema.json" "schemas/engine-agent.schema.json"
Resolve-RequiredAgentPath "schemas/engine-agent/ai-operable-production-loop.schema.json" | Out-Null
Assert-ContainsText $gameAgentSchemaText '"materialShaderAuthoringTargets"' "schemas/game-agent.schema.json"
Assert-ContainsText $gameAgentSchemaText '"atlasTilemapAuthoringTargets"' "schemas/game-agent.schema.json"
Assert-ContainsText $gameAgentSchemaText '"packageStreamingResidencyTargets"' "schemas/game-agent.schema.json"
Assert-ContainsText $gameAgentSchemaText '"performanceBudgets"' "schemas/game-agent.schema.json"
Assert-ContainsText $gameAgentSchemaText '"artifactPath"' "schemas/game-agent.schema.json"
Assert-ContainsText $gameAgentSchemaText '"prefabScenePackageAuthoringTargets"' "schemas/game-agent.schema.json"
Assert-ContainsText $gameAgentSchemaText '"registeredSourceAssetCookTargets"' "schemas/game-agent.schema.json"
Assert-ContainsText $manifestRaw "runtimeSceneValidationTargets" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "materialShaderAuthoringLoops" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "atlasTilemapAuthoringLoops" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "packageStreamingResidencyLoops" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "performanceBudgetEvidenceLoops" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "prefabScenePackageAuthoringLoops" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "safePointPackageReplacementLoops" "engine/agent/manifest.json"
Assert-ContainsText $currentCapabilitiesContent "runtimeSceneValidationTargets" "docs/current-capabilities.md"
Assert-ContainsText $currentCapabilitiesContent "Material Shader Authoring Loop v1" "docs/current-capabilities.md"
Assert-ContainsText $currentCapabilitiesContent "create-material-from-graph" "docs/current-capabilities.md"
Assert-ContainsText $currentCapabilitiesContent "packageStreamingResidencyTargets" "docs/current-capabilities.md"
Assert-ContainsText $currentCapabilitiesContent "performanceBudgets" "docs/current-capabilities.md"
Assert-ContainsText $currentCapabilitiesContent "performanceBudgetEvidenceLoops" "docs/current-capabilities.md"
Assert-ContainsText $currentCapabilitiesContent "installed-2d-performance-baseline.trace.json" "docs/current-capabilities.md"
Assert-ContainsText $currentCapabilitiesContent "registeredSourceAssetCookTargets" "docs/current-capabilities.md"
Assert-ContainsText $currentCapabilitiesContent "Safe-Point Package Unload Replacement Execution v1" "docs/current-capabilities.md"
Assert-ContainsText $currentCapabilitiesContent "commit_runtime_package_safe_point_unload" "docs/current-capabilities.md"
Assert-ContainsText $aiGameDevelopmentContent "runtimeSceneValidationTargets" "docs/ai-game-development.md"
Assert-ContainsText $aiGameDevelopmentContent "materialShaderAuthoringTargets" "docs/ai-game-development.md"
Assert-ContainsText $aiGameDevelopmentContent "plan_material_graph_package_update" "docs/ai-game-development.md"
Assert-ContainsText $aiGameDevelopmentContent "atlasTilemapAuthoringTargets" "docs/ai-game-development.md"
Assert-ContainsText $aiGameDevelopmentContent "packageStreamingResidencyTargets" "docs/ai-game-development.md"
Assert-ContainsText $aiGameDevelopmentContent "performanceBudgets" "docs/ai-game-development.md"
Assert-ContainsText $aiGameDevelopmentContent "evidenceRows.artifactPath" "docs/ai-game-development.md"
Assert-ContainsText $aiGameDevelopmentContent "prefabScenePackageAuthoringTargets" "docs/ai-game-development.md"
Assert-ContainsText $aiGameDevelopmentContent "registeredSourceAssetCookTargets" "docs/ai-game-development.md"
Assert-ContainsText $aiGameDevelopmentContent "plan_placeholder_asset_bundle" "docs/ai-game-development.md"
Assert-ContainsText $aiGameDevelopmentContent "plan_sprite_atlas_source_authoring" "docs/ai-game-development.md"
Assert-ContainsText $aiGameDevelopmentContent "SpriteAtlasSourceAuthoringDesc" "docs/ai-game-development.md"
Assert-ContainsText $aiGameDevelopmentContent "PlaceholderAssetBundlePlan" "docs/ai-game-development.md"
Assert-ContainsText $aiGameDevelopmentContent "plan_placeholder_asset_cook_package" "docs/ai-game-development.md"
Assert-ContainsText $aiGameDevelopmentContent "PlaceholderAssetCookPackageRequest" "docs/ai-game-development.md"
Assert-ContainsText $aiGameDevelopmentContent "GameEngine.SourceAssetRegistry.v1" "docs/ai-game-development.md"
Assert-ContainsText $aiGameDevelopmentContent "safe-point package replacement" "docs/ai-game-development.md"
Assert-ContainsText $aiGameDevelopmentContent "commit_runtime_package_safe_point_unload" "docs/ai-game-development.md"
Assert-ContainsText $roadmapContent "runtimeSceneValidationTargets" "docs/roadmap.md"
Assert-ContainsText $roadmapContent "packageStreamingResidencyTargets" "docs/roadmap.md"
Assert-ContainsText $roadmapContent "performanceBudgets" "docs/roadmap.md"
Assert-ContainsText $roadmapContent "registeredSourceAssetCookTargets" "docs/roadmap.md"
Assert-ContainsText $roadmapContent "Safe-Point Package Unload Replacement Execution v1" "docs/roadmap.md"
Assert-ContainsText $roadmapContent "commit_runtime_package_safe_point_unload" "docs/roadmap.md"
Assert-ContainsText $manifestRaw "DesktopRuntime2DPackage" "engine/agent/manifest.json"
Assert-ContainsText $currentCapabilitiesContent "DesktopRuntime2DPackage" "docs/current-capabilities.md"
Assert-ContainsText $aiGameDevelopmentContent "DesktopRuntime2DPackage" "docs/ai-game-development.md"
Assert-ContainsText $roadmapContent "DesktopRuntime2DPackage" "docs/roadmap.md"
Assert-ContainsText $manifestRaw "DesktopRuntime3DPackage" "engine/agent/manifest.json"
Assert-ContainsText $currentCapabilitiesContent "DesktopRuntime3DPackage" "docs/current-capabilities.md"
Assert-ContainsText $aiGameDevelopmentContent "DesktopRuntime3DPackage" "docs/ai-game-development.md"
Assert-ContainsText $roadmapContent "DesktopRuntime3DPackage" "docs/roadmap.md"
$productionCompletionMasterPlanContent = Get-AgentSurfaceText $productionCompletionMasterPlanRelativePath
$historicalPlanEvidenceText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
Assert-DoesNotContainText $productionCompletionMasterPlanContent "3d-camera-controller-and-gameplay-template-v1" $productionCompletionMasterPlanRelativePath
Assert-DoesNotContainText $productionCompletionMasterPlanContent "first generated 3D camera/controller gameplay template" $productionCompletionMasterPlanRelativePath
Assert-ContainsText $currentCapabilitiesContent "glTF Node Transform Animation Import v1" "docs/current-capabilities.md"
Assert-ContainsText $aiGameDevelopmentContent "import_gltf_node_transform_animation_tracks" "docs/ai-game-development.md"
Assert-ContainsText $roadmapContent "import_gltf_node_transform_animation_tracks" "docs/roadmap.md"
Assert-ContainsText $productionCompletionMasterPlanContent "gltf-node-transform-animation-import-v1" $productionCompletionMasterPlanRelativePath
Assert-ContainsText $historicalPlanEvidenceText "2026-05-05-gltf-node-transform-animation-import-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $currentCapabilitiesContent "glTF Node Transform Animation Float Clip Bridge v1" "docs/current-capabilities.md"
Assert-ContainsText $aiGameDevelopmentContent "import_gltf_node_transform_animation_float_clip" "docs/ai-game-development.md"
Assert-ContainsText $roadmapContent "import_gltf_node_transform_animation_float_clip" "docs/roadmap.md"
Assert-ContainsText $productionCompletionMasterPlanContent "gltf-node-transform-animation-float-clip-bridge-v1" $productionCompletionMasterPlanRelativePath
Assert-ContainsText $historicalPlanEvidenceText "2026-05-05-gltf-node-transform-animation-float-clip-bridge-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $currentCapabilitiesContent "Animation Float Transform Application v1" "docs/current-capabilities.md"
Assert-ContainsText $aiGameDevelopmentContent "apply_float_animation_samples_to_transform3d" "docs/ai-game-development.md"
Assert-ContainsText $roadmapContent "apply_float_animation_samples_to_transform3d" "docs/roadmap.md"
Assert-ContainsText $productionCompletionMasterPlanContent "animation-float-transform-application-v1" $productionCompletionMasterPlanRelativePath
Assert-ContainsText $historicalPlanEvidenceText "2026-05-05-animation-float-transform-application-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $currentCapabilitiesContent "Animation Transform Binding Source v1" "docs/current-capabilities.md"
Assert-ContainsText $aiGameDevelopmentContent "AnimationTransformBindingSourceDocument" "docs/ai-game-development.md"
Assert-ContainsText $roadmapContent "serialize_animation_transform_binding_source_document" "docs/roadmap.md"
Assert-ContainsText $productionCompletionMasterPlanContent "animation-transform-binding-source-v1" $productionCompletionMasterPlanRelativePath
Assert-ContainsText $historicalPlanEvidenceText "2026-05-05-animation-transform-binding-source-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $manifestRaw "GameEngine.AnimationTransformBindingSource.v1" "engine/agent/manifest.json"
Assert-ContainsText (Get-AgentSurfaceText "engine/assets/include/mirakana/assets/asset_source_format.hpp") "AnimationTransformBindingSourceDocument" "engine/assets/include/mirakana/assets/asset_source_format.hpp"
Assert-ContainsText $currentCapabilitiesContent "Runtime Scene Animation Transform Binding v1" "docs/current-capabilities.md"
Assert-ContainsText $aiGameDevelopmentContent "resolve_runtime_scene_animation_transform_bindings" "docs/ai-game-development.md"
Assert-ContainsText $roadmapContent "apply_runtime_scene_animation_transform_samples" "docs/roadmap.md"
Assert-ContainsText $productionCompletionMasterPlanContent "runtime-scene-animation-transform-binding-v1" $productionCompletionMasterPlanRelativePath
Assert-ContainsText $historicalPlanEvidenceText "2026-05-05-runtime-scene-animation-transform-binding-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $manifestRaw "resolve_runtime_scene_animation_transform_bindings" "engine/agent/manifest.json"
Assert-ContainsText (Get-AgentSurfaceText "engine/runtime_scene/include/mirakana/runtime_scene/runtime_scene.hpp") "RuntimeSceneAnimationTransformBindingResolution" "engine/runtime_scene/include/mirakana/runtime_scene/runtime_scene.hpp"
Assert-ContainsText $currentCapabilitiesContent "glTF Node Transform Animation Binding Source Bridge v1" "docs/current-capabilities.md"
Assert-ContainsText $aiGameDevelopmentContent "import_gltf_node_transform_animation_binding_source" "docs/ai-game-development.md"
Assert-ContainsText $roadmapContent "import_gltf_node_transform_animation_binding_source" "docs/roadmap.md"
Assert-ContainsText $productionCompletionMasterPlanContent "gltf-node-transform-animation-binding-source-bridge-v1" $productionCompletionMasterPlanRelativePath
Assert-ContainsText $historicalPlanEvidenceText "2026-05-05-gltf-node-transform-animation-binding-source-bridge-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $manifestRaw "import_gltf_node_transform_animation_binding_source" "engine/agent/manifest.json"
Assert-ContainsText $currentCapabilitiesContent "Cooked Animation Quaternion Clip v1" "docs/current-capabilities.md"
Assert-ContainsText $aiGameDevelopmentContent "runtime_animation_quaternion_clip_payload" "docs/ai-game-development.md"
Assert-ContainsText $aiGameDevelopmentContent "make_animation_joint_tracks_3d_from_f32_bytes" "docs/ai-game-development.md"
Assert-ContainsText $aiGameDevelopmentContent "import_gltf_node_transform_animation_quaternion_clip" "docs/ai-game-development.md"
Assert-ContainsText $roadmapContent "GameEngine.CookedAnimationQuaternionClip.v1" "docs/roadmap.md"
Assert-ContainsText $productionCompletionMasterPlanContent "cooked-animation-quaternion-clip-v1" $productionCompletionMasterPlanRelativePath
Assert-ContainsText $historicalPlanEvidenceText "2026-05-05-cooked-animation-quaternion-clip-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $manifestRaw "runtime_animation_quaternion_clip_payload" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "import_gltf_node_transform_animation_quaternion_clip" "engine/agent/manifest.json"
Assert-ContainsText (Get-AgentSurfaceText "engine/assets/include/mirakana/assets/asset_source_format.hpp") "AnimationQuaternionClipSourceDocument" "MK_assets asset source public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/asset_runtime.hpp") "RuntimeAnimationQuaternionClipPayload" "MK_runtime public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/animation/include/mirakana/animation/skeleton.hpp") "AnimationJointTrack3dByteSource" "MK_animation skeleton public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/gltf_node_animation_import.hpp") "GltfNodeTransformAnimationQuaternionClipImportReport" "MK_tools gltf node animation import public header"

Assert-ContainsText $currentCapabilitiesContent "sample_and_apply_runtime_scene_render_animation_float_clip" "docs/current-capabilities.md"
Assert-ContainsText $aiGameDevelopmentContent "sample_and_apply_runtime_scene_render_animation_float_clip" "docs/ai-game-development.md"
Assert-ContainsText $roadmapContent "sample_and_apply_runtime_scene_render_animation_float_clip" "docs/roadmap.md"
Assert-ContainsText $productionCompletionMasterPlanContent "generated-3d-transform-animation-scaffold-v1" $productionCompletionMasterPlanRelativePath
Assert-ContainsText $historicalPlanEvidenceText "2026-05-05-generated-3d-transform-animation-scaffold-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $manifestRaw "desktopRuntime3dTransformAnimationScaffold" "engine/agent/manifest.json"
Assert-ContainsText (Get-AgentSurfaceText "engine/scene_renderer/include/mirakana/scene_renderer/scene_renderer.hpp") "sample_and_apply_runtime_scene_render_animation_float_clip" "MK_scene_renderer public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/runtime_scene/include/mirakana/runtime_scene/runtime_scene.hpp") "apply_runtime_scene_animation_pose_3d" "MK_runtime_scene public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/scene_renderer/include/mirakana/scene_renderer/scene_renderer.hpp") "sample_and_apply_runtime_scene_render_animation_pose_3d" "MK_scene_renderer public header"
Assert-ContainsText $currentCapabilitiesContent "Runtime Scene Quaternion Animation Transform Binding v1" "docs/current-capabilities.md"
Assert-ContainsText $aiGameDevelopmentContent "sample_and_apply_runtime_scene_render_animation_pose_3d" "docs/ai-game-development.md"
Assert-ContainsText $roadmapContent "quaternion scene-transform smoke" "docs/roadmap.md"
Assert-ContainsText $manifestRaw "apply_runtime_scene_animation_pose_3d" "engine/agent/manifest.json"
Assert-ContainsText $manifestRaw "quaternion_animation_scene_rotation_z" "engine/agent/manifest.json"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/gltf_node_animation_import.hpp") "GltfNodeTransformAnimationBindingSourceImportReport" "MK_tools gltf node animation import public header"
Assert-ContainsText $currentCapabilitiesContent "sample_runtime_morph_mesh_cpu_animation_float_clip" "docs/current-capabilities.md"
Assert-ContainsText $aiGameDevelopmentContent "sample_runtime_morph_mesh_cpu_animation_float_clip" "docs/ai-game-development.md"
Assert-ContainsText $roadmapContent "sample_runtime_morph_mesh_cpu_animation_float_clip" "docs/roadmap.md"
Assert-ContainsText $productionCompletionMasterPlanContent "generated-3d-morph-package-consumption-v1" $productionCompletionMasterPlanRelativePath
Assert-ContainsText $historicalPlanEvidenceText "2026-05-05-generated-3d-morph-package-consumption-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $manifestRaw "desktopRuntime3dMorphPackageConsumptionScaffold" "engine/agent/manifest.json"
Assert-ContainsText (Get-AgentSurfaceText "engine/scene_renderer/include/mirakana/scene_renderer/scene_renderer.hpp") "sample_runtime_morph_mesh_cpu_animation_float_clip" "MK_scene_renderer public header"
Assert-DoesNotContainText $productionCompletionMasterPlanContent "generated-game morph package consumption/rendering" $productionCompletionMasterPlanRelativePath
Assert-ContainsText $currentCapabilitiesContent "GPU Morph D3D12 Proof v1" "docs/current-capabilities.md"
Assert-ContainsText $aiGameDevelopmentContent "upload_runtime_morph_mesh_cpu" "docs/ai-game-development.md"
Assert-ContainsText $roadmapContent "MeshCommand::gpu_morphing" "docs/roadmap.md"
Assert-ContainsText $productionCompletionMasterPlanContent "gpu-morph-d3d12-proof-v1" $productionCompletionMasterPlanRelativePath
Assert-ContainsText $historicalPlanEvidenceText "2026-05-05-gpu-morph-d3d12-proof-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $manifestRaw "runtimeMorphMeshGpuBinding" "engine/agent/manifest.json"
Assert-ContainsText (Get-AgentSurfaceText "engine/runtime_rhi/include/mirakana/runtime_rhi/runtime_upload.hpp") "upload_runtime_morph_mesh_cpu" "MK_runtime_rhi public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/runtime_rhi/include/mirakana/runtime_rhi/runtime_upload.hpp") "uploaded_normal_delta_bytes" "MK_runtime_rhi public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/runtime_rhi/include/mirakana/runtime_rhi/runtime_upload.hpp") "uploaded_tangent_delta_bytes" "MK_runtime_rhi public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/renderer.hpp") "MorphMeshGpuBinding" "MK_renderer public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/renderer.hpp") "normal_delta_buffer" "MK_renderer public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/renderer.hpp") "tangent_delta_buffer" "MK_renderer public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/rhi_frame_renderer.hpp") "morph_graphics_pipeline" "MK_renderer rhi frame renderer public header"
Assert-ContainsText $currentCapabilitiesContent "GPU Morph NORMAL/TANGENT D3D12 Proof v1" "docs/current-capabilities.md"
Assert-ContainsText $roadmapContent "uploaded_normal_delta_bytes" "docs/roadmap.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-05-gpu-morph-normal-tangent-d3d12-proof-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $currentCapabilitiesContent "runtime scene RHI morph GPU palette bridge" "docs/current-capabilities.md"
Assert-ContainsText $aiGameDevelopmentContent "RuntimeSceneGpuBindingOptions::morph_mesh_assets" "docs/ai-game-development.md"
Assert-ContainsText $roadmapContent "SceneMorphGpuBindingPalette" "docs/roadmap.md"
Assert-ContainsText $productionCompletionMasterPlanContent "runtime-scene-rhi-morph-gpu-palette-v1" $productionCompletionMasterPlanRelativePath
Assert-ContainsText $historicalPlanEvidenceText "2026-05-05-runtime-scene-rhi-morph-gpu-palette-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $manifestRaw "runtimeSceneRhiMorphGpuPalette" "engine/agent/manifest.json"
Assert-ContainsText (Get-AgentSurfaceText "engine/scene_renderer/include/mirakana/scene_renderer/scene_renderer.hpp") "SceneMorphGpuBindingPalette" "MK_scene_renderer public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/runtime_scene_rhi/include/mirakana/runtime_scene_rhi/runtime_scene_rhi.hpp") "morph_mesh_assets" "MK_runtime_scene_rhi public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/runtime_scene_rhi/include/mirakana/runtime_scene_rhi/runtime_scene_rhi.hpp") "uploaded_morph_bytes" "MK_runtime_scene_rhi public header"
Assert-ContainsText $currentCapabilitiesContent "generated 3D morph GPU palette smoke" "docs/current-capabilities.md"
Assert-ContainsText $aiGameDevelopmentContent "scene_gpu_morph_mesh_uploads" "docs/ai-game-development.md"
Assert-ContainsText $roadmapContent "generated 3D morph GPU palette smoke" "docs/roadmap.md"
Assert-ContainsText $productionCompletionMasterPlanContent "generated-3d-morph-gpu-palette-smoke-v1" $productionCompletionMasterPlanRelativePath
Assert-ContainsText $historicalPlanEvidenceText "2026-05-05-generated-3d-morph-gpu-palette-smoke-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $manifestRaw "desktopRuntime3dMorphGpuPaletteSmoke" "engine/agent/manifest.json"
Assert-ContainsText (Get-AgentSurfaceText "engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_desktop_presentation.hpp") "morph_mesh_assets" "MK_runtime_host_win32 public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_desktop_presentation.hpp") "uploaded_morph_bytes" "MK_runtime_host_win32 public header"
Assert-ContainsText $currentCapabilitiesContent "Generated 3D Morph NORMAL/TANGENT Package Smoke v1" "docs/current-capabilities.md"
Assert-ContainsText $aiGameDevelopmentContent "POSITION/NORMAL/TANGENT morph delta buffers" "docs/ai-game-development.md"
Assert-ContainsText $roadmapContent "generated 3D NORMAL/TANGENT morph package smoke" "docs/roadmap.md"
Assert-ContainsText $productionCompletionMasterPlanContent "generated-3d-morph-normal-tangent-package-smoke-v1" $productionCompletionMasterPlanRelativePath
Assert-ContainsText $historicalPlanEvidenceText "2026-05-05-generated-3d-morph-normal-tangent-package-smoke-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $manifestRaw "desktopRuntime3dMorphNormalTangentPackageSmoke" "engine/agent/manifest.json"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-02-2d-packaged-playable-generation-loop-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $manifestRaw "editor-playtest-package-review-loop" "engine/agent/manifest.json"
Assert-ContainsText $currentCapabilitiesContent "Editor Playtest Package Review Loop v1" "docs/current-capabilities.md"
Assert-ContainsText $aiGameDevelopmentContent "Editor Playtest Package Review Loop v1" "docs/ai-game-development.md"
Assert-ContainsText $roadmapContent "Editor Playtest Package Review Loop v1" "docs/roadmap.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-02-editor-playtest-package-review-loop-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $manifestRaw "renderer-resource-residency-upload-execution" "engine/agent/manifest.json"
Assert-ContainsText $currentCapabilitiesContent "Renderer Resource Residency Upload Execution v1" "docs/current-capabilities.md"
Assert-ContainsText $aiGameDevelopmentContent "Renderer Resource Residency Upload Execution v1" "docs/ai-game-development.md"
Assert-ContainsText $roadmapContent "Renderer Resource Residency Upload Execution v1" "docs/roadmap.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-02-renderer-resource-residency-upload-execution-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText (Get-AgentSurfaceText "tools/installed-sdk-validation.ps1") "Assert-InstalledSdkMetadata" "tools/installed-sdk-validation.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/check-installed-sdk-validation.ps1") "installed-sdk-validation-check: ok" "tools/check-installed-sdk-validation.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/validate-installed-sdk.ps1") "Assert-InstalledSdkMetadata" "tools/validate-installed-sdk.ps1"
Assert-ContainsText (Get-Content -LiteralPath $releasePath -Raw) "Assert-InstalledSdkMetadata" "docs/release.md"
Assert-ContainsText (Get-Content -LiteralPath $testingPath -Raw) "validates installed SDK metadata before the installed consumer build" "docs/testing.md"
Assert-ContainsText $currentCapabilitiesContent "Installed SDK release validation now checks installed metadata" "docs/current-capabilities.md"
Assert-ContainsText $roadmapContent "installed SDK payload gate" "docs/roadmap.md"
Assert-ContainsText $productionCompletionMasterPlanContent "installed-sdk-release-metadata-validation-v1" $productionCompletionMasterPlanRelativePath
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-installed-sdk-release-metadata-validation-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $manifestRaw "Validate installed SDK metadata payloads" "engine/agent/manifest.json"
Assert-ContainsText (Get-AgentSurfaceText "tools/release-package-artifacts.ps1") "Assert-ReleasePackageArtifacts" "tools/release-package-artifacts.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/check-release-package-artifacts.ps1") "release-package-artifacts-check: ok" "tools/check-release-package-artifacts.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/package.ps1") "Assert-ReleasePackageArtifacts" "tools/package.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/validate.ps1") "check-release-package-artifacts.ps1" "tools/validate.ps1"
Assert-ContainsText (Get-Content -LiteralPath $releasePath -Raw) "Assert-ReleasePackageArtifacts" "docs/release.md"
Assert-ContainsText (Get-Content -LiteralPath $testingPath -Raw) 'validates the current `CPACK_PACKAGE_FILE_NAME` ZIP' "docs/testing.md"
Assert-ContainsText $currentCapabilitiesContent "Desktop SDK release artifact validation now checks" "docs/current-capabilities.md"
Assert-ContainsText $roadmapContent "desktop SDK release artifact gate" "docs/roadmap.md"
Assert-ContainsText $productionCompletionMasterPlanContent "desktop-release-package-evidence-v1" $productionCompletionMasterPlanRelativePath
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-desktop-release-package-evidence-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $manifestRaw "validate the current ZIP SHA-256 sidecar" "engine/agent/manifest.json"

$productionLoop = $manifest.aiOperableProductionLoop
Assert-JsonProperty $productionLoop @("schemaVersion", "design", "foundationPlan", "currentActivePlan", "recommendedNextPlan", "recipeStatusEnum", "recipes", "commandSurfaces", "authoringSurfaces", "packageSurfaces", "physicsBackendAdapterDecisions", "unsupportedProductionGaps", "hostGates", "validationRecipeMap", "performanceBudgetEvidenceLoops") "engine/agent/manifest.json aiOperableProductionLoop"
if ($productionLoop.schemaVersion -ne 1) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop.schemaVersion must be 1"
}
Assert-ActiveProductionPlanDrift $productionLoop
Assert-NoGameSourceRawAssetIdFromName
$physicsBackendAdapterDecision = @($productionLoop.physicsBackendAdapterDecisions | Where-Object { $_.id -eq "physics-1-0-jolt-native-adapter" })
if ($physicsBackendAdapterDecision.Count -ne 1) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must record one physics-1-0-jolt-native-adapter decision"
}
Assert-JsonProperty $physicsBackendAdapterDecision[0] @("id", "status", "decision", "futureGate", "unsupportedClaims") "engine/agent/manifest.json aiOperableProductionLoop physicsBackendAdapterDecisions"
Assert-ContainsText ([string]$physicsBackendAdapterDecision[0].status) "excluded-from-1-0-ready-surface" "engine/agent/manifest.json aiOperableProductionLoop physicsBackendAdapterDecisions"
Assert-ContainsText ([string]$physicsBackendAdapterDecision[0].decision) "first-party MK_physics only" "engine/agent/manifest.json aiOperableProductionLoop physicsBackendAdapterDecisions"
Assert-ContainsText ([string]$physicsBackendAdapterDecision[0].futureGate) "vcpkg manifest feature" "engine/agent/manifest.json aiOperableProductionLoop physicsBackendAdapterDecisions"
Assert-ContainsText ([string]$physicsBackendAdapterDecision[0].futureGate) "fail-closed capability negotiation" "engine/agent/manifest.json aiOperableProductionLoop physicsBackendAdapterDecisions"
foreach ($claim in @("Jolt runtime integration in the default/generated-game 1.0 ready surface", "native physics handles in public gameplay APIs", "middleware type exposure")) {
    if (@($physicsBackendAdapterDecision[0].unsupportedClaims) -notcontains $claim) {
        Write-Error "engine/agent/manifest.json aiOperableProductionLoop physicsBackendAdapterDecisions unsupportedClaims missing: $claim"
    }
}
if (-not $productionLoop.PSObject.Properties.Name.Contains("reviewLoops")) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop missing reviewLoops"
}
$editorPlaytestReviewLoop = @($productionLoop.reviewLoops | Where-Object { $_.id -eq "editor-playtest-package-review-loop" })
if ($editorPlaytestReviewLoop.Count -ne 1) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose one editor-playtest-package-review-loop review loop"
}
if ($editorPlaytestReviewLoop.Count -eq 1) {
    Assert-JsonProperty $editorPlaytestReviewLoop[0] @("id", "status", "owner", "orderedSteps", "requiredManifestFields", "preSmokeGate", "hostGatedSmokeRecipes", "unsupportedClaims") "engine/agent/manifest.json editor-playtest-package-review-loop"
    if ($editorPlaytestReviewLoop[0].status -ne "ready") {
        Write-Error "engine/agent/manifest.json editor-playtest-package-review-loop must be ready inside its reviewed scope"
    }
    $expectedEditorReviewSteps = @(
        "review-editor-package-candidates",
        "apply-reviewed-runtime-package-files",
        "select-runtime-scene-validation-target",
        "validate-runtime-scene-package",
        "run-host-gated-desktop-smoke"
    )
    $actualEditorReviewSteps = @($editorPlaytestReviewLoop[0].orderedSteps | ForEach-Object { $_.id })
    if (($actualEditorReviewSteps -join "|") -ne ($expectedEditorReviewSteps -join "|")) {
        Write-Error "engine/agent/manifest.json editor-playtest-package-review-loop orderedSteps must be: $($expectedEditorReviewSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "validationRecipes")) {
        if (@($editorPlaytestReviewLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json editor-playtest-package-review-loop requiredManifestFields missing: $field"
        }
    }
    if ($editorPlaytestReviewLoop[0].preSmokeGate -ne "validate-runtime-scene-package") {
        Write-Error "engine/agent/manifest.json editor-playtest-package-review-loop preSmokeGate must be validate-runtime-scene-package"
    }
    foreach ($recipe in @("desktop-game-runtime", "desktop-runtime-release-target", "installed-d3d12-3d-package-smoke", "installed-d3d12-3d-directional-shadow-smoke", "installed-d3d12-3d-shadow-morph-composition-smoke", "desktop-runtime-release-target-vulkan-toolchain-gated", "desktop-runtime-release-target-vulkan-directional-shadow-toolchain-gated")) {
        if (@($editorPlaytestReviewLoop[0].hostGatedSmokeRecipes) -notcontains $recipe) {
            Write-Error "engine/agent/manifest.json editor-playtest-package-review-loop hostGatedSmokeRecipes missing: $recipe"
        }
    }
}
$editorAiPackageDiagnosticsLoop = @($productionLoop.reviewLoops | Where-Object { $_.id -eq "editor-ai-package-authoring-diagnostics" })
if ($editorAiPackageDiagnosticsLoop.Count -ne 1) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose one editor-ai-package-authoring-diagnostics review loop"
}
if ($editorAiPackageDiagnosticsLoop.Count -eq 1) {
    Assert-JsonProperty $editorAiPackageDiagnosticsLoop[0] @("id", "status", "owner", "orderedSteps", "requiredManifestFields", "diagnosticInputs", "blockedExecution", "unsupportedClaims") "engine/agent/manifest.json editor-ai-package-authoring-diagnostics"
    if ($editorAiPackageDiagnosticsLoop[0].status -ne "ready") {
        Write-Error "engine/agent/manifest.json editor-ai-package-authoring-diagnostics must be ready as diagnostics-only editor-core model"
    }
    $expectedEditorAiDiagnosticsSteps = @(
        "collect-editor-package-candidate-diagnostics",
        "summarize-manifest-descriptor-rows",
        "inspect-runtime-package-payload-diagnostics",
        "summarize-validation-recipe-status",
        "report-host-gated-desktop-smoke-preflight"
    )
    $actualEditorAiDiagnosticsSteps = @($editorAiPackageDiagnosticsLoop[0].orderedSteps | ForEach-Object {
        Assert-JsonProperty $_ @("id", "surface", "status", "mutates", "executes") "engine/agent/manifest.json editor-ai-package-authoring-diagnostics ordered step"
        if ($_.mutates -ne $false) {
            Write-Error "engine/agent/manifest.json editor-ai-package-authoring-diagnostics step '$($_.id)' must not mutate"
        }
        if ($_.executes -ne $false) {
            Write-Error "engine/agent/manifest.json editor-ai-package-authoring-diagnostics step '$($_.id)' must not execute"
        }
        $_.id
    })
    if (($actualEditorAiDiagnosticsSteps -join "|") -ne ($expectedEditorAiDiagnosticsSteps -join "|")) {
        Write-Error "engine/agent/manifest.json editor-ai-package-authoring-diagnostics orderedSteps must be: $($expectedEditorAiDiagnosticsSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "prefabScenePackageAuthoringTargets", "registeredSourceAssetCookTargets", "validationRecipes")) {
        if (@($editorAiPackageDiagnosticsLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json editor-ai-package-authoring-diagnostics requiredManifestFields missing: $field"
        }
    }
    foreach ($claim in @("arbitrary shell", "free-form manifest edits", "broad package cooking", "runtime source parsing", "play-in-editor productization", "renderer/RHI handle exposure", "Metal readiness", "general renderer quality")) {
        if (-not ((@($editorAiPackageDiagnosticsLoop[0].blockedExecution) -join " ").Contains($claim))) {
            Write-Error "engine/agent/manifest.json editor-ai-package-authoring-diagnostics blockedExecution missing: $claim"
        }
        if (-not ((@($editorAiPackageDiagnosticsLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine/agent/manifest.json editor-ai-package-authoring-diagnostics unsupportedClaims missing: $claim"
        }
    }
}
$editorAiPlaytestOperatorHandoffLoop = @($productionLoop.reviewLoops | Where-Object { $_.id -eq "editor-ai-playtest-operator-handoff" })
if ($editorAiPlaytestOperatorHandoffLoop.Count -ne 1) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose one editor-ai-playtest-operator-handoff review loop"
}
if ($editorAiPlaytestOperatorHandoffLoop.Count -eq 1) {
    Assert-JsonProperty $editorAiPlaytestOperatorHandoffLoop[0] @("id", "status", "owner", "orderedSteps", "requiredManifestFields", "handoffInputs", "commandFields", "blockedExecution", "unsupportedClaims") "engine/agent/manifest.json editor-ai-playtest-operator-handoff"
    if ($editorAiPlaytestOperatorHandoffLoop[0].status -ne "ready") {
        Write-Error "engine/agent/manifest.json editor-ai-playtest-operator-handoff must be ready as read-only editor-core handoff model"
    }
    $expectedEditorAiOperatorHandoffSteps = @(
        "collect-readiness-report",
        "collect-validation-preflight-commands",
        "assemble-reviewed-operator-command-rows",
        "report-host-gates-blockers-and-unsupported-claims"
    )
    $actualEditorAiOperatorHandoffSteps = @($editorAiPlaytestOperatorHandoffLoop[0].orderedSteps | ForEach-Object {
        Assert-JsonProperty $_ @("id", "surface", "status", "mutates", "executes") "engine/agent/manifest.json editor-ai-playtest-operator-handoff ordered step"
        if ($_.mutates -ne $false) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-operator-handoff step '$($_.id)' must not mutate"
        }
        if ($_.executes -ne $false) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-operator-handoff step '$($_.id)' must not execute"
        }
        $_.id
    })
    if (($actualEditorAiOperatorHandoffSteps -join "|") -ne ($expectedEditorAiOperatorHandoffSteps -join "|")) {
        Write-Error "engine/agent/manifest.json editor-ai-playtest-operator-handoff orderedSteps must be: $($expectedEditorAiOperatorHandoffSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "prefabScenePackageAuthoringTargets", "registeredSourceAssetCookTargets", "validationRecipes")) {
        if (@($editorAiPlaytestOperatorHandoffLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-operator-handoff requiredManifestFields missing: $field"
        }
    }
    foreach ($expectedManifestInput in @("EditorAiPlaytestReadinessReportModel", "EditorAiValidationRecipePreflightModel", "run-validation-recipe dry-run command plan data", "host gates", "blocked reasons")) {
        if (-not ((@($editorAiPlaytestOperatorHandoffLoop[0].handoffInputs) -join " ").Contains($expectedManifestInput))) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-operator-handoff handoffInputs missing: $expectedManifestInput"
        }
    }
    foreach ($field in @("recipe id", "reviewed command display", "argv plan data", "host gates", "blocked reasons", "readiness dependency")) {
        if (-not ((@($editorAiPlaytestOperatorHandoffLoop[0].commandFields) -join " ").Contains($field))) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-operator-handoff commandFields missing: $field"
        }
    }
    foreach ($claim in @("arbitrary shell", "raw manifest command strings", "validation execution", "package script execution", "free-form manifest edits", "play-in-editor productization", "renderer/RHI handle exposure", "Metal readiness", "general renderer quality")) {
        if (-not ((@($editorAiPlaytestOperatorHandoffLoop[0].blockedExecution) -join " ").Contains($claim))) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-operator-handoff blockedExecution missing: $claim"
        }
        if (-not ((@($editorAiPlaytestOperatorHandoffLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-operator-handoff unsupportedClaims missing: $claim"
        }
    }
}
$editorAiPlaytestEvidenceSummaryLoop = @($productionLoop.reviewLoops | Where-Object { $_.id -eq "editor-ai-playtest-evidence-summary" })
if ($editorAiPlaytestEvidenceSummaryLoop.Count -ne 1) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose one editor-ai-playtest-evidence-summary review loop"
}
if ($editorAiPlaytestEvidenceSummaryLoop.Count -eq 1) {
    Assert-JsonProperty $editorAiPlaytestEvidenceSummaryLoop[0] @("id", "status", "owner", "orderedSteps", "requiredManifestFields", "evidenceInputs", "evidenceFields", "blockedExecution", "unsupportedClaims") "engine/agent/manifest.json editor-ai-playtest-evidence-summary"
    if ($editorAiPlaytestEvidenceSummaryLoop[0].status -ne "ready") {
        Write-Error "engine/agent/manifest.json editor-ai-playtest-evidence-summary must be ready as read-only editor-core evidence summary model"
    }
    $expectedEditorAiEvidenceSummarySteps = @(
        "collect-operator-handoff",
        "collect-external-validation-evidence",
        "summarize-evidence-status",
        "report-evidence-blockers-and-unsupported-claims"
    )
    $actualEditorAiEvidenceSummarySteps = @($editorAiPlaytestEvidenceSummaryLoop[0].orderedSteps | ForEach-Object {
        Assert-JsonProperty $_ @("id", "surface", "status", "mutates", "executes") "engine/agent/manifest.json editor-ai-playtest-evidence-summary ordered step"
        if ($_.mutates -ne $false) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-evidence-summary step '$($_.id)' must not mutate"
        }
        if ($_.executes -ne $false) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-evidence-summary step '$($_.id)' must not execute"
        }
        $_.id
    })
    if (($actualEditorAiEvidenceSummarySteps -join "|") -ne ($expectedEditorAiEvidenceSummarySteps -join "|")) {
        Write-Error "engine/agent/manifest.json editor-ai-playtest-evidence-summary orderedSteps must be: $($expectedEditorAiEvidenceSummarySteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "prefabScenePackageAuthoringTargets", "registeredSourceAssetCookTargets", "validationRecipes")) {
        if (@($editorAiPlaytestEvidenceSummaryLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-evidence-summary requiredManifestFields missing: $field"
        }
    }
    foreach ($expectedManifestInput in @("EditorAiPlaytestOperatorHandoffModel", "externally supplied run-validation-recipe execute results", "recipe status", "exit code or summary text", "host gates", "blocked reasons", "readiness dependency")) {
        if (-not ((@($editorAiPlaytestEvidenceSummaryLoop[0].evidenceInputs) -join " ").Contains($expectedManifestInput))) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-evidence-summary evidenceInputs missing: $expectedManifestInput"
        }
    }
    foreach ($field in @("recipe id", "handoff row", "status passed failed blocked host-gated missing", "exit code", "summary text", "host gates", "blockers", "readiness dependency")) {
        if (-not ((@($editorAiPlaytestEvidenceSummaryLoop[0].evidenceFields) -join " ").Contains($field))) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-evidence-summary evidenceFields missing: $field"
        }
    }
    foreach ($claim in @("arbitrary shell", "raw manifest command strings", "raw/free-form command evaluation", "validation execution", "package script execution", "free-form manifest edits", "play-in-editor productization", "renderer/RHI handle exposure", "Metal readiness", "general renderer quality")) {
        if (-not ((@($editorAiPlaytestEvidenceSummaryLoop[0].blockedExecution) -join " ").Contains($claim))) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-evidence-summary blockedExecution missing: $claim"
        }
        if (-not ((@($editorAiPlaytestEvidenceSummaryLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-evidence-summary unsupportedClaims missing: $claim"
        }
    }
}
$editorAiPlaytestRemediationQueueLoop = @($productionLoop.reviewLoops | Where-Object { $_.id -eq "editor-ai-playtest-remediation-queue" })
if ($editorAiPlaytestRemediationQueueLoop.Count -ne 1) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose one editor-ai-playtest-remediation-queue review loop"
}
if ($editorAiPlaytestRemediationQueueLoop.Count -eq 1) {
    Assert-JsonProperty $editorAiPlaytestRemediationQueueLoop[0] @("id", "status", "owner", "orderedSteps", "requiredManifestFields", "queueInputs", "queueFields", "blockedExecution", "unsupportedClaims") "engine/agent/manifest.json editor-ai-playtest-remediation-queue"
    if ($editorAiPlaytestRemediationQueueLoop[0].status -ne "ready") {
        Write-Error "engine/agent/manifest.json editor-ai-playtest-remediation-queue must be ready as read-only editor-core remediation queue model"
    }
    $expectedEditorAiRemediationQueueSteps = @(
        "collect-evidence-summary",
        "classify-remediation-rows",
        "prioritize-remediation-categories",
        "report-remediation-blockers-and-unsupported-claims"
    )
    $actualEditorAiRemediationQueueSteps = @($editorAiPlaytestRemediationQueueLoop[0].orderedSteps | ForEach-Object {
        Assert-JsonProperty $_ @("id", "surface", "status", "mutates", "executes") "engine/agent/manifest.json editor-ai-playtest-remediation-queue ordered step"
        if ($_.mutates -ne $false) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-remediation-queue step '$($_.id)' must not mutate"
        }
        if ($_.executes -ne $false) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-remediation-queue step '$($_.id)' must not execute"
        }
        $_.id
    })
    if (($actualEditorAiRemediationQueueSteps -join "|") -ne ($expectedEditorAiRemediationQueueSteps -join "|")) {
        Write-Error "engine/agent/manifest.json editor-ai-playtest-remediation-queue orderedSteps must be: $($expectedEditorAiRemediationQueueSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "prefabScenePackageAuthoringTargets", "registeredSourceAssetCookTargets", "validationRecipes")) {
        if (@($editorAiPlaytestRemediationQueueLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-remediation-queue requiredManifestFields missing: $field"
        }
    }
    foreach ($expectedManifestInput in @("EditorAiPlaytestEvidenceSummaryModel", "failed evidence", "blocked evidence", "missing evidence", "host-gated evidence", "host gates", "blocked reasons", "readiness dependency")) {
        if (-not ((@($editorAiPlaytestRemediationQueueLoop[0].queueInputs) -join " ").Contains($expectedManifestInput))) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-remediation-queue queueInputs missing: $expectedManifestInput"
        }
    }
    foreach ($field in @("recipe id", "evidence status", "remediation category", "next-action text", "host gates", "blockers", "readiness dependency", "unsupported claims")) {
        if (-not ((@($editorAiPlaytestRemediationQueueLoop[0].queueFields) -join " ").Contains($field))) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-remediation-queue queueFields missing: $field"
        }
    }
    foreach ($claim in @("arbitrary shell", "raw manifest command strings", "raw/free-form command evaluation", "validation execution", "package script execution", "free-form manifest edits", "evidence mutation", "fix execution", "play-in-editor productization", "renderer/RHI handle exposure", "Metal readiness", "general renderer quality")) {
        if (-not ((@($editorAiPlaytestRemediationQueueLoop[0].blockedExecution) -join " ").Contains($claim))) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-remediation-queue blockedExecution missing: $claim"
        }
        if (-not ((@($editorAiPlaytestRemediationQueueLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-remediation-queue unsupportedClaims missing: $claim"
        }
    }
}
if (-not $productionLoop.PSObject.Properties.Name.Contains("resourceExecutionLoops")) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop missing resourceExecutionLoops"
}
$rendererResourceExecutionLoop = @($productionLoop.resourceExecutionLoops | Where-Object { $_.id -eq "renderer-resource-residency-upload-execution" })
if ($rendererResourceExecutionLoop.Count -ne 1) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose one renderer-resource-residency-upload-execution loop"
}
if ($rendererResourceExecutionLoop.Count -eq 1) {
    Assert-JsonProperty $rendererResourceExecutionLoop[0] @("id", "status", "owner", "orderedSteps", "requiredManifestFields", "preUploadGate", "hostGatedSmokeRecipes", "reportFields", "unsupportedClaims") "engine/agent/manifest.json renderer-resource-residency-upload-execution"
    if ($rendererResourceExecutionLoop[0].status -ne "host-gated") {
        Write-Error "engine/agent/manifest.json renderer-resource-residency-upload-execution must remain host-gated"
    }
    $expectedResourceExecutionSteps = @(
        "validate-runtime-scene-package",
        "instantiate-runtime-scene",
        "build-scene-render-packet",
        "execute-host-owned-runtime-scene-gpu-upload",
        "report-backend-neutral-upload-residency-counters",
        "run-host-gated-scene-gpu-smoke"
    )
    $actualResourceExecutionSteps = @($rendererResourceExecutionLoop[0].orderedSteps | ForEach-Object { $_.id })
    if (($actualResourceExecutionSteps -join "|") -ne ($expectedResourceExecutionSteps -join "|")) {
        Write-Error "engine/agent/manifest.json renderer-resource-residency-upload-execution orderedSteps must be: $($expectedResourceExecutionSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "validationRecipes")) {
        if (@($rendererResourceExecutionLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json renderer-resource-residency-upload-execution requiredManifestFields missing: $field"
        }
    }
    foreach ($field in @("scene_gpu_status", "scene_gpu_mesh_uploads", "scene_gpu_texture_uploads", "scene_gpu_material_bindings", "scene_gpu_uploaded_texture_bytes", "scene_gpu_uploaded_mesh_bytes", "scene_gpu_uploaded_material_factor_bytes", "scene_gpu_morph_mesh_bindings", "scene_gpu_morph_mesh_uploads", "scene_gpu_uploaded_morph_bytes", "scene_gpu_morph_mesh_resolved", "renderer_gpu_morph_draws", "renderer_morph_descriptor_binds")) {
        if (@($rendererResourceExecutionLoop[0].reportFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json renderer-resource-residency-upload-execution reportFields missing: $field"
        }
    }
}
if (-not $productionLoop.PSObject.Properties.Name.Contains("materialShaderAuthoringLoops")) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop missing materialShaderAuthoringLoops"
}
$materialShaderAuthoringLoop = @($productionLoop.materialShaderAuthoringLoops | Where-Object { $_.id -eq "material-shader-authoring-review-loop" })
if ($materialShaderAuthoringLoop.Count -ne 1) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose one material-shader-authoring-review-loop"
}
if ($materialShaderAuthoringLoop.Count -eq 1) {
    Assert-JsonProperty $materialShaderAuthoringLoop[0] @("id", "status", "owner", "orderedSteps", "requiredManifestFields", "preSmokeGates", "descriptorFields", "hostGatedSmokeRecipes", "unsupportedClaims") "engine/agent/manifest.json material-shader-authoring-review-loop"
    if ($materialShaderAuthoringLoop[0].status -ne "host-gated") {
        Write-Error "engine/agent/manifest.json material-shader-authoring-review-loop must remain host-gated"
    }
    $expectedMaterialShaderSteps = @(
        "review-source-material-authoring-inputs",
        "validate-source-material-and-texture-dependencies",
        "review-material-graph-production-authoring",
        "review-fixed-shader-artifact-requests",
        "validate-shader-artifacts",
        "run-host-gated-material-shader-package-smoke"
    )
    $actualMaterialShaderSteps = @($materialShaderAuthoringLoop[0].orderedSteps | ForEach-Object { $_.id })
    if (($actualMaterialShaderSteps -join "|") -ne ($expectedMaterialShaderSteps -join "|")) {
        Write-Error "engine/agent/manifest.json material-shader-authoring-review-loop orderedSteps must be: $($expectedMaterialShaderSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "materialShaderAuthoringTargets", "validationRecipes")) {
        if (@($materialShaderAuthoringLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json material-shader-authoring-review-loop requiredManifestFields missing: $field"
        }
    }
    foreach ($field in @("id", "sourceMaterialPath", "runtimeMaterialPath", "packageIndexPath", "sourceMaterialGraphPath", "shaderExportPath", "reviewedHlslSourcePath", "compileRequestTargets", "unsupportedBoundaries", "shaderSourcePaths", "d3d12ShaderArtifactPaths", "vulkanShaderArtifactPaths")) {
        if (@($materialShaderAuthoringLoop[0].descriptorFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json material-shader-authoring-review-loop descriptorFields missing: $field"
        }
    }
    foreach ($claim in @("shader graph execution", "live shader generation", "shader compiler execution", "package streaming", "public native/RHI handles", "Metal readiness", "general renderer quality")) {
        if (-not ((@($materialShaderAuthoringLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine/agent/manifest.json material-shader-authoring-review-loop unsupportedClaims missing: $claim"
        }
    }
}
if (-not $productionLoop.PSObject.Properties.Name.Contains("atlasTilemapAuthoringLoops")) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop missing atlasTilemapAuthoringLoops"
}
$atlasTilemapAuthoringLoop = @($productionLoop.atlasTilemapAuthoringLoops | Where-Object { $_.id -eq "2d-atlas-tilemap-package-authoring" })
if ($atlasTilemapAuthoringLoop.Count -ne 1) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose one 2d-atlas-tilemap-package-authoring loop"
}
if ($atlasTilemapAuthoringLoop.Count -eq 1) {
    Assert-JsonProperty $atlasTilemapAuthoringLoop[0] @("id", "status", "owner", "orderedSteps", "requiredManifestFields", "preflightGates", "descriptorFields", "hostGatedSmokeRecipes", "unsupportedClaims") "engine/agent/manifest.json 2d-atlas-tilemap-package-authoring"
    if ($atlasTilemapAuthoringLoop[0].status -ne "host-gated") {
        Write-Error "engine/agent/manifest.json 2d-atlas-tilemap-package-authoring must remain host-gated"
    }
    $expectedAtlasTilemapSteps = @(
        "select-atlas-tilemap-authoring-target",
        "author-deterministic-tilemap-metadata",
        "update-tilemap-package-index",
        "validate-runtime-tilemap-payload",
        "run-host-gated-2d-package-preflight"
    )
    $actualAtlasTilemapSteps = @($atlasTilemapAuthoringLoop[0].orderedSteps | ForEach-Object { $_.id })
    if (($actualAtlasTilemapSteps -join "|") -ne ($expectedAtlasTilemapSteps -join "|")) {
        Write-Error "engine/agent/manifest.json 2d-atlas-tilemap-package-authoring orderedSteps must be: $($expectedAtlasTilemapSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "atlasTilemapAuthoringTargets", "validationRecipes")) {
        if (@($atlasTilemapAuthoringLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json 2d-atlas-tilemap-package-authoring requiredManifestFields missing: $field"
        }
    }
    foreach ($field in @("id", "packageIndexPath", "tilemapPath", "atlasTexturePath", "tilemapAssetKey", "atlasTextureAssetKey", "mode", "sourceDecoding", "atlasPacking", "nativeGpuSpriteBatching", "preflightRecipeIds")) {
        if (@($atlasTilemapAuthoringLoop[0].descriptorFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json 2d-atlas-tilemap-package-authoring descriptorFields missing: $field"
        }
    }
    foreach ($claim in @("source image decoding", "production atlas packing", "tilemap editor UX", "native GPU sprite batching", "package streaming execution", "public native/RHI handles", "Metal readiness", "general renderer quality")) {
        if (-not ((@($atlasTilemapAuthoringLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine/agent/manifest.json 2d-atlas-tilemap-package-authoring unsupportedClaims missing: $claim"
        }
    }
}
if (-not $productionLoop.PSObject.Properties.Name.Contains("prefabScenePackageAuthoringLoops")) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop missing prefabScenePackageAuthoringLoops"
}
$prefabScene3dAuthoringLoop = @($productionLoop.prefabScenePackageAuthoringLoops | Where-Object { $_.id -eq "3d-prefab-scene-package-authoring" })
if ($prefabScene3dAuthoringLoop.Count -ne 1) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose one 3d-prefab-scene-package-authoring loop"
}
if ($prefabScene3dAuthoringLoop.Count -eq 1) {
    Assert-JsonProperty $prefabScene3dAuthoringLoop[0] @("id", "status", "owner", "orderedSteps", "requiredManifestFields", "preflightGates", "descriptorFields", "hostGatedSmokeRecipes", "unsupportedClaims") "engine/agent/manifest.json 3d-prefab-scene-package-authoring"
    if ($prefabScene3dAuthoringLoop[0].status -ne "host-gated") {
        Write-Error "engine/agent/manifest.json 3d-prefab-scene-package-authoring must remain host-gated"
    }
    $expectedPrefabScene3dSteps = @(
        "select-prefab-scene-package-authoring-target",
        "apply-scene-prefab-v2-authoring-commands",
        "cook-selected-source-registry-rows",
        "migrate-scene-v2-runtime-package",
        "validate-runtime-scene-package",
        "run-host-gated-3d-package-smoke"
    )
    $actualPrefabScene3dSteps = @($prefabScene3dAuthoringLoop[0].orderedSteps | ForEach-Object { $_.id })
    if (($actualPrefabScene3dSteps -join "|") -ne ($expectedPrefabScene3dSteps -join "|")) {
        Write-Error "engine/agent/manifest.json 3d-prefab-scene-package-authoring orderedSteps must be: $($expectedPrefabScene3dSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "prefabScenePackageAuthoringTargets", "registeredSourceAssetCookTargets", "validationRecipes")) {
        if (@($prefabScene3dAuthoringLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json 3d-prefab-scene-package-authoring requiredManifestFields missing: $field"
        }
    }
    foreach ($field in @("id", "mode", "sceneAuthoringPath", "prefabAuthoringPath", "sourceRegistryPath", "packageIndexPath", "outputScenePath", "sceneAssetKey", "runtimeSceneValidationTargetId", "authoringCommandRows", "selectedSourceAssetKeys", "sourceCookMode", "sceneMigration", "runtimeSceneValidation", "hostGatedSmokeRecipeIds", "broadImporterExecution", "broadDependencyCooking", "runtimeSourceParsing", "materialGraph", "shaderGraph", "liveShaderGeneration", "skeletalAnimation", "gpuSkinning", "publicNativeRhiHandles", "metalReadiness", "rendererQuality", "cookCommandId", "prefabScenePackageAuthoringTargetId", "selectedAssetKeys", "dependencyExpansion", "dependencyCooking", "externalImporterExecution", "rendererRhiResidency", "packageStreaming", "editorProductization", "generalProductionRendererQuality", "arbitraryShell", "freeFormEdit")) {
        if (@($prefabScene3dAuthoringLoop[0].descriptorFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json 3d-prefab-scene-package-authoring descriptorFields missing: $field"
        }
    }
    foreach ($claim in @("broad importer execution", "broad/dependent package cooking", "runtime source parsing", "material graph", "shader graph", "live shader generation", "skeletal animation", "GPU skinning", "public native/RHI handles", "Metal readiness", "general renderer quality")) {
        if (-not ((@($prefabScene3dAuthoringLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine/agent/manifest.json 3d-prefab-scene-package-authoring unsupportedClaims missing: $claim"
        }
    }
}
if (-not $productionLoop.PSObject.Properties.Name.Contains("performanceBudgetEvidenceLoops")) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop missing performanceBudgetEvidenceLoops"
}
$performanceBudgetEvidenceLoop = @($productionLoop.performanceBudgetEvidenceLoops | Where-Object { $_.id -eq "ai-operable-performance-budget-evidence" })
if ($performanceBudgetEvidenceLoop.Count -ne 1) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose one ai-operable-performance-budget-evidence loop"
}
if ($performanceBudgetEvidenceLoop.Count -eq 1) {
    Assert-JsonProperty $performanceBudgetEvidenceLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "descriptorFields", "budgetCategories", "evidenceKinds", "blockedExecution", "unsupportedClaims") "engine/agent/manifest.json ai-operable-performance-budget-evidence"
    if ($performanceBudgetEvidenceLoop[0].status -ne "ready") {
        Write-Error "engine/agent/manifest.json ai-operable-performance-budget-evidence must be ready"
    }
    $expectedPerformanceBudgetSteps = @(
        "select-performance-budget-set",
        "validate-budget-recipe-links",
        "review-budget-rows",
        "attach-evidence-rows",
        "reject-broad-optimization-claims"
    )
    $actualPerformanceBudgetSteps = @($performanceBudgetEvidenceLoop[0].orderedSteps | ForEach-Object { $_.id })
    if (($actualPerformanceBudgetSteps -join "|") -ne ($expectedPerformanceBudgetSteps -join "|")) {
        Write-Error "engine/agent/manifest.json ai-operable-performance-budget-evidence orderedSteps must be: $($expectedPerformanceBudgetSteps -join ', ')"
    }
    foreach ($field in @("performanceBudgets", "runtimePackageFiles", "runtimeSceneValidationTargets", "packageStreamingResidencyTargets", "validationRecipes")) {
        if (@($performanceBudgetEvidenceLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json ai-operable-performance-budget-evidence requiredManifestFields missing: $field"
        }
    }
    foreach ($field in @("schemaVersion", "capabilityId", "budgetSetId", "selectedRecipeId", "targetBackend", "hostGateId", "validationRecipeIds", "budgetRows", "evidenceRows", "budgetRows.metric", "evidenceRows.source", "evidenceRows.artifactPath", "unsupportedClaims")) {
        if (@($performanceBudgetEvidenceLoop[0].descriptorFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json ai-operable-performance-budget-evidence descriptorFields missing: $field"
        }
    }
    foreach ($claim in @("broad optimized game generation", "cross-vendor performance parity", "cross-backend performance parity", "native handles", "allocator/GPU budget enforcement", "CUDA/HIP runtime path", "GPU-driven rendering readiness")) {
        if (-not ((@($performanceBudgetEvidenceLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine/agent/manifest.json ai-operable-performance-budget-evidence unsupportedClaims missing: $claim"
        }
    }
}
if (-not $productionLoop.PSObject.Properties.Name.Contains("packageStreamingResidencyLoops")) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop missing packageStreamingResidencyLoops"
}
$packageStreamingResidencyLoop = @($productionLoop.packageStreamingResidencyLoops | Where-Object { $_.id -eq "package-streaming-residency-budget-contract" })
if ($packageStreamingResidencyLoop.Count -ne 1) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose one package-streaming-residency-budget-contract loop"
}
if ($packageStreamingResidencyLoop.Count -eq 1) {
    Assert-JsonProperty $packageStreamingResidencyLoop[0] @("id", "status", "owner", "orderedSteps", "requiredManifestFields", "descriptorFields", "preflightGates", "blockedExecution", "unsupportedClaims") "engine/agent/manifest.json package-streaming-residency-budget-contract"
    if ($packageStreamingResidencyLoop[0].status -ne "ready") {
        Write-Error "engine/agent/manifest.json package-streaming-residency-budget-contract must be ready"
    }
    $expectedPackageStreamingSteps = @(
        "select-package-streaming-residency-target",
        "validate-runtime-scene-package",
        "review-residency-budget-intent",
        "confirm-host-owned-resource-upload-gate",
        "select-host-gated-safe-point-execution"
    )
    $actualPackageStreamingSteps = @($packageStreamingResidencyLoop[0].orderedSteps | ForEach-Object { $_.id })
    if (($actualPackageStreamingSteps -join "|") -ne ($expectedPackageStreamingSteps -join "|")) {
        Write-Error "engine/agent/manifest.json package-streaming-residency-budget-contract orderedSteps must be: $($expectedPackageStreamingSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "packageStreamingResidencyTargets", "validationRecipes")) {
        if (@($packageStreamingResidencyLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json package-streaming-residency-budget-contract requiredManifestFields missing: $field"
        }
    }
    foreach ($field in @("id", "packageIndexPath", "runtimeSceneValidationTargetId", "mode", "residentBudgetBytes", "safePointRequired", "preloadAssetKeys", "residentResourceKinds")) {
        if (@($packageStreamingResidencyLoop[0].descriptorFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json package-streaming-residency-budget-contract descriptorFields missing: $field"
        }
    }
    foreach ($claim in @("broad async/background package streaming", "arbitrary eviction", "texture streaming", "public native/RHI handles", "Metal readiness", "general renderer quality")) {
        if (-not ((@($packageStreamingResidencyLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine/agent/manifest.json package-streaming-residency-budget-contract unsupportedClaims missing: $claim"
        }
    }
}
$hostGatedPackageStreamingLoop = @($productionLoop.packageStreamingResidencyLoops | Where-Object { $_.id -eq "host-gated-package-streaming-execution" })
if ($hostGatedPackageStreamingLoop.Count -ne 1) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose one host-gated-package-streaming-execution loop"
}
if ($hostGatedPackageStreamingLoop.Count -eq 1) {
    Assert-JsonProperty $hostGatedPackageStreamingLoop[0] @("id", "status", "owner", "orderedSteps", "requiredManifestFields", "descriptorFields", "preflightGates", "resultFields", "blockedExecution", "unsupportedClaims") "engine/agent/manifest.json host-gated-package-streaming-execution"
    if ($hostGatedPackageStreamingLoop[0].status -ne "host-gated") {
        Write-Error "engine/agent/manifest.json host-gated-package-streaming-execution must be host-gated"
    }
    $expectedHostGatedStreamingSteps = @(
        "select-package-streaming-residency-target",
        "validate-runtime-scene-package",
        "load-selected-runtime-package",
        "commit-safe-point-package-streaming-replacement",
        "commit-resident-package-streaming-mount",
        "commit-resident-package-streaming-replacement",
        "commit-resident-package-streaming-unmount",
        "report-streaming-execution-diagnostics",
        "keep-renderer-rhi-teardown-host-owned"
    )
    $actualHostGatedStreamingSteps = @($hostGatedPackageStreamingLoop[0].orderedSteps | ForEach-Object { $_.id })
    if (($actualHostGatedStreamingSteps -join "|") -ne ($expectedHostGatedStreamingSteps -join "|")) {
        Write-Error "engine/agent/manifest.json host-gated-package-streaming-execution orderedSteps must be: $($expectedHostGatedStreamingSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "packageStreamingResidencyTargets", "validationRecipes")) {
        if (@($hostGatedPackageStreamingLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json host-gated-package-streaming-execution requiredManifestFields missing: $field"
        }
    }
    foreach ($field in @("id", "packageIndexPath", "runtimeSceneValidationTargetId", "mode", "residentBudgetBytes", "safePointRequired", "preloadAssetKeys", "residentResourceKinds", "maxResidentPackages", "preflightRecipeIds")) {
        if (@($hostGatedPackageStreamingLoop[0].descriptorFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json host-gated-package-streaming-execution descriptorFields missing: $field"
        }
    }
    foreach ($gate in @("validate-runtime-scene-package", "safe-point-package-unload-replacement-execution")) {
        if (@($hostGatedPackageStreamingLoop[0].preflightGates) -notcontains $gate) {
            Write-Error "engine/agent/manifest.json host-gated-package-streaming-execution preflightGates missing: $gate"
        }
    }
    foreach ($field in @("status", "target_id", "package_index_path", "runtime_scene_validation_target_id", "estimated_resident_bytes", "resident_budget_bytes", "replacement_status", "resident_mount_status", "resident_replace_status", "resident_unmount_status", "resident_catalog_refresh_status", "resident_package_count", "resident_mount_generation", "stale_handle_count", "diagnostics")) {
        if (@($hostGatedPackageStreamingLoop[0].resultFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json host-gated-package-streaming-execution resultFields missing: $field"
        }
    }
    foreach ($claim in @("broad async/background package streaming", "arbitrary eviction", "texture streaming", "allocator/GPU budget enforcement", "public native/RHI handles", "Metal readiness", "general renderer quality")) {
        if (-not ((@($hostGatedPackageStreamingLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine/agent/manifest.json host-gated-package-streaming-execution unsupportedClaims missing: $claim"
        }
    }
    foreach ($helperPath in @(
        (Join-Path $root "engine/runtime/include/mirakana/runtime/package_streaming.hpp"),
        (Join-Path $root "engine/runtime/src/package_streaming.cpp")
    )) {
        if (Test-Path -LiteralPath $helperPath -PathType Leaf) {
            $helperText = Get-Content -LiteralPath $helperPath -Raw
            foreach ($forbiddenText in @(
                "mirakana/rhi/",
                "mirakana/renderer/",
                "mirakana/runtime_scene_rhi/",
                "IRhiDevice",
                "SceneGpuBindingPalette",
                "MeshGpuBinding",
                "MaterialGpuBinding",
                "BufferHandle",
                "TextureHandle",
                "DescriptorSetHandle",
                "PipelineLayoutHandle",
                "SwapchainFrame",
                "nativeHandle",
                "allocatorHandle"
            )) {
                if ($helperText.Contains($forbiddenText)) {
                    Write-Error "$helperPath must keep package streaming execution free of renderer/RHI/native surfaces: $forbiddenText"
                }
            }
        }
    }
}
if (-not $productionLoop.PSObject.Properties.Name.Contains("safePointPackageReplacementLoops")) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop missing safePointPackageReplacementLoops"
}
$safePointPackageReplacementLoop = @($productionLoop.safePointPackageReplacementLoops | Where-Object { $_.id -eq "safe-point-package-unload-replacement-execution" })
if ($safePointPackageReplacementLoop.Count -ne 1) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose one safe-point-package-unload-replacement-execution loop"
}
if ($safePointPackageReplacementLoop.Count -eq 1) {
    Assert-JsonProperty $safePointPackageReplacementLoop[0] @("id", "status", "owner", "orderedSteps", "requiredModules", "resultFields", "blockedExecution", "unsupportedClaims") "engine/agent/manifest.json safe-point-package-unload-replacement-execution"
    if ($safePointPackageReplacementLoop[0].status -ne "ready") {
        Write-Error "engine/agent/manifest.json safe-point-package-unload-replacement-execution must be ready"
    }
    $expectedSafePointSteps = @(
        "stage-loaded-runtime-package",
        "build-pending-resource-catalog",
        "commit-package-and-resource-catalog-at-safe-point",
        "reject-invalid-package-before-active-swap",
        "commit-unload-and-empty-catalog-at-safe-point",
        "keep-renderer-rhi-teardown-host-owned"
    )
    $actualSafePointSteps = @($safePointPackageReplacementLoop[0].orderedSteps | ForEach-Object { $_.id })
    if (($actualSafePointSteps -join "|") -ne ($expectedSafePointSteps -join "|")) {
        Write-Error "engine/agent/manifest.json safe-point-package-unload-replacement-execution orderedSteps must be: $($expectedSafePointSteps -join ', ')"
    }
    foreach ($module in @("MK_runtime")) {
        if (@($safePointPackageReplacementLoop[0].requiredModules) -notcontains $module) {
            Write-Error "engine/agent/manifest.json safe-point-package-unload-replacement-execution requiredModules missing: $module"
        }
    }
    foreach ($field in @("status", "previous_record_count", "committed_record_count", "previous_generation", "committed_generation", "stale_handle_count", "discarded_pending_package", "diagnostics")) {
        if (@($safePointPackageReplacementLoop[0].resultFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json safe-point-package-unload-replacement-execution resultFields missing: $field"
        }
    }
    Assert-ContainsText (Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/resource_runtime.hpp") "RuntimePackageSafePointUnloadResult" "MK_runtime resource public header"
    Assert-ContainsText (Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/resource_runtime.hpp") "commit_runtime_package_safe_point_unload" "MK_runtime resource public header"
    foreach ($claim in @("broad package streaming", "async eviction", "texture streaming", "allocator/GPU budget enforcement", "public native/RHI handles", "Metal readiness", "general renderer quality")) {
        if (-not ((@($safePointPackageReplacementLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine/agent/manifest.json safe-point-package-unload-replacement-execution unsupportedClaims missing: $claim"
        }
    }
}
Resolve-RequiredAgentPath $productionLoop.design | Out-Null
Resolve-RequiredAgentPath $productionLoop.foundationPlan | Out-Null
Resolve-RequiredAgentPath $productionLoop.currentActivePlan | Out-Null
if ($productionLoop.recommendedNextPlan.PSObject.Properties.Name.Contains("path")) {
    Resolve-RequiredAgentPath $productionLoop.recommendedNextPlan.path | Out-Null
}
$activeChildProductionPlans = @(Get-ActiveChildProductionPlan)
if ($activeChildProductionPlans.Count -eq 0) {
    Assert-ContainsText ([string]$productionLoop.currentActivePlan) "2026-05-03-production-completion-master-plan-v1.md" "engine/agent/manifest.json aiOperableProductionLoop.currentActivePlan"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.id) "next-production-gap-selection" "engine/agent/manifest.json aiOperableProductionLoop.recommendedNextPlan.id"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.path) "2026-05-03-production-completion-master-plan-v1.md" "engine/agent/manifest.json aiOperableProductionLoop.recommendedNextPlan.path"
}
$recommendedNextPlanText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "
if ([string]$productionLoop.recommendedNextPlan.id -eq "general-purpose-game-production-v1") {
    foreach ($needle in @(
        "General Purpose Game Production v1",
        "gameplay-runtime-scheduler-production-v1",
        "world-entity-model-production-v1",
        "addressable-content-streaming-production-v1",
        "production-authoring-workflows-v1",
        "production-runtime-ui-workbench-v1",
        "unsupportedProductionGaps empty"
    )) {
        Assert-ContainsText $recommendedNextPlanText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan production milestone"
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "generated-game-studio-v1") {
    foreach ($needle in @(
        "Generated Game Studio v1",
        "General Purpose Game Production v1",
        "ai-game-design-spec-v1",
        "ai-game-generation-orchestrator-v1",
        "ai-generated-game-playtest-loop-v1",
        "ai-validation-remediation-recipes-v1",
        "ai-generated-game-quality-rubric-v1",
        "EditorAiCommandPanelModel",
        "unsupportedProductionGaps empty"
    )) {
        Assert-ContainsText $recommendedNextPlanText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan generated game studio milestone"
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "engine-1-0-gap-matrix-v1") {
    foreach ($needle in @(
        "Engine 1.0 Gap Matrix v1",
        "Generated Game Studio v1",
        "implemented-1x-foundation",
        "renderer-backend-parity-v1",
        "strict Vulkan evidence",
        "Metal remains Apple-host-gated",
        "unsupportedProductionGaps empty",
        "broad commercial-engine"
    )) {
        Assert-ContainsText $recommendedNextPlanText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan engine gap matrix"
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "next-production-gap-selection") {
    foreach ($needle in @(
        "First-Party Desktop Platform And SDL3 Removal v1",
        "MK_platform_win32",
        "MK_runtime_host_win32",
        "MK_runtime_host_win32_presentation",
        "MK_audio_wasapi",
        "unsupportedProductionGaps = []",
        "selection gate"
    )) {
        Assert-ContainsText $recommendedNextPlanText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan selection gate closeout"
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "first-party-ui-editor-production-stack-v1") {
    foreach ($needle in @(
        "First-Party UI Editor Production Stack v1",
        "MK_editor",
        "MK_editor_core",
        "desktop-editor",
        "mirakana::ui",
        "MK_ui_renderer",
        "dock graph",
        "rich text",
        "DirectWrite",
        "Text Services Framework",
        "UI Automation",
        "D3D12 viewport/material texture display",
        "AI-operable",
        "compatibility shims",
        "unsupportedProductionGaps = []",
        "SDL3",
        "native handles",
        "Dear ImGui"
    )) {
        Assert-ContainsText $recommendedNextPlanText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan first-party UI editor production selection"
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "first-party-editor-shell-v1") {
    foreach ($needle in @(
        "First-Party Editor Shell v1",
        "first-party retained MK_editor shell",
        "desktop-editor",
        "EditorAiOperationSnapshot",
        "EditorAiCommandCatalog",
        "dock graph",
        "rich-text rows",
        "DirectWrite",
        "Text Services Framework",
        "UI Automation",
        "unsupportedProductionGaps = []",
        "SDL3",
        "native handles",
        "Dear ImGui"
    )) {
        Assert-ContainsText $recommendedNextPlanText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan first-party editor shell selection"
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "physics-navigation-commercial-coverage-v1") {
    foreach ($needle in @(
        "Physics Navigation Commercial Coverage v1",
        "Jolt/Recast/Detour-class",
        "adapter_boundary_id",
        "host_validation_recipe_id",
        "adapter_lifecycle_reviewed",
        "unsupportedProductionGaps = []",
        "native handles hidden",
        "broad middleware parity fail-closed"
    )) {
        Assert-ContainsText $recommendedNextPlanText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan physics/navigation selection"
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "renderer-backend-parity-metal-apple-evidence-v1") {
    foreach ($needle in @(
        "Renderer Backend Parity Metal Apple Evidence v1",
        "renderer-backend-parity-v1",
        "metal-apple remains host-gated",
        "shader-toolchain",
        "mobile-packaging",
        "ios-simulator-smoke",
        "Apple/Metal host evidence",
        "Windows/Vulkan proof must not promote Metal readiness",
        "no SDL3",
        "native handles remain hidden",
        "unsupportedProductionGaps = []"
    )) {
        Assert-ContainsText $recommendedNextPlanText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan renderer Metal Apple selection"
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "renderer-postprocess-tone-mapping-evidence-v1") {
    foreach ($needle in @(
        "Renderer Postprocess Tone Mapping Evidence v1",
        "renderer-postprocess-v1",
        "PostprocessToneMappingEvidencePlan",
        "plan_postprocess_tone_mapping_evidence",
        "D3D12/Vulkan",
        "Metal host-gated",
        "no SDL3",
        "native handles hidden",
        "subjective visual quality",
        "unsupportedProductionGaps = []"
    )) {
        Assert-ContainsText $recommendedNextPlanText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan renderer postprocess selection"
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "sandbox-world-network-modding-gate-v1") {
    foreach ($needle in @(
        "Selected focused child plan",
        "sandbox-world-specific mutation replication",
        "reviewed modding policy gates",
        "unsupportedProductionGaps = []",
        "Broad online multiplayer",
        "SDL3",
        "native handle exposure"
    )) {
        Assert-ContainsText $recommendedNextPlanText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan sandbox world network/modding selection"
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "sandbox-world-package-validation-performance-budgets-v1") {
    foreach ($needle in @(
        "Selected focused child plan",
        "sample package smoke flags",
        "installed validation",
        "package-visible counters",
        "--require-sandbox-package-budgets",
        "sandbox_package_budget_*",
        "unsupportedProductionGaps = []",
        "broad renderer quality",
        "package mutation",
        "SDL3",
        "native handle exposure"
    )) {
        Assert-ContainsText $recommendedNextPlanText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan sandbox world package validation and performance budget selection"
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "ai-operable-performance-budget-and-evidence-v1") {
    foreach ($needle in @(
        "performanceBudgets",
        "budgetRows",
        "evidenceRows",
        "validation recipe",
        "unsupported broad optimization claims",
        "CPU/GPU/memory optimization"
    )) {
        Assert-ContainsText $recommendedNextPlanText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan performance budget evidence selection"
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "performance-baseline-v1") {
    foreach ($needle in @(
        "reproducible benchmark scenes/packages",
        "trace export recipes",
        "subsystem counters",
        "p95/p99 frame budget reporting",
        "broad CPU/GPU/memory optimization"
    )) {
        Assert-ContainsText $recommendedNextPlanText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan performance baseline selection"
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "memory-lifetime-taxonomy-v1") {
    foreach ($needle in @(
        "memory lifetime taxonomy",
        "ownership semantics",
        "raw pointers are non-owning",
        "std::unique_ptr",
        "std::span",
        "allocator/job/NUMA/GPU memory",
        "broad CPU/GPU/memory optimization"
    )) {
        Assert-ContainsText $recommendedNextPlanText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan memory lifetime taxonomy selection"
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "memory-diagnostics-v1") {
    foreach ($needle in @(
        "memory diagnostics",
        "memory class counters",
        "high-water marks",
        "budget pressure",
        "stale-generation",
        "use-after-safe-point",
        "allocator replacement",
        "broad CPU/GPU/memory optimization"
    )) {
        Assert-ContainsText $recommendedNextPlanText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan memory diagnostics selection"
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "frame-thread-scratch-v1") {
    foreach ($needle in @(
        "frame temporary",
        "worker scratch",
        "first-party frame arenas",
        "per-worker scratch arenas",
        "explicit ownership APIs",
        "high-water marks",
        "false-sharing diagnostics",
        "allocator replacement",
        "all-core CPU scheduling",
        "broad CPU/GPU/memory optimization"
    )) {
        Assert-ContainsText $recommendedNextPlanText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan frame/thread scratch selection"
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "first-party-ui-editor-production-stack-v1") {
    foreach ($needle in @(
        "First-Party UI Editor Production Stack v1",
        "MK_editor",
        "MK_editor_core",
        "desktop-editor",
        "mirakana::ui",
        "MK_ui_renderer",
        "dock graph",
        "rich text",
        "DirectWrite",
        "Text Services Framework",
        "UI Automation",
        "D3D12 viewport/material texture display",
        "AI-operable",
        "compatibility shims",
        "unsupportedProductionGaps = []",
        "SDL3",
        "native handles",
        "Dear ImGui"
    )) {
        Assert-ContainsText $recommendedNextPlanText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan first-party UI editor production selection"
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "first-party-editor-shell-v1") {
    foreach ($needle in @(
        "First-Party Editor Shell v1",
        "MK_editor",
        "MK_editor_core",
        "first-party retained",
        "desktop-editor",
        "mirakana::ui",
        "MK_ui_renderer",
        "EditorAiOperationSnapshot",
        "EditorAiCommandCatalog",
        "EditorAiCommandRequest",
        "EditorAiCommandDryRunResult",
        "EditorAiCommandApplyResult",
        "dock graph",
        "rich-text",
        "DirectWrite",
        "Text Services Framework",
        "UI Automation",
        "unsupportedProductionGaps = []",
        "SDL3",
        "native handles",
        "Dear ImGui"
    )) {
        Assert-ContainsText $recommendedNextPlanText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan first-party editor shell selection"
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "native-win32-editor-shell-v1") {
    foreach ($needle in @(
        "Native Win32 Editor Shell v1",
        "MK_editor",
        "Dear ImGui",
        "Direct3D 12",
        "desktop-gui",
        "PR #316",
        "unsupportedProductionGaps = []",
        "SDL3",
        "native handles",
        "editor/developer-shell"
    )) {
        Assert-ContainsText $recommendedNextPlanText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan native editor shell selection"
    }
} else {
    foreach ($needle in @(
    "Frame Graph Transient Texture Alias Planning v1",
    "FrameGraphTransientTextureAliasPlan",
    "plan_frame_graph_transient_texture_aliases",
    "Frame Graph Shadow Scratch Color Target-State Ownership v1",
    "shadow_color",
    "6 pass callbacks/15 barrier steps",
    "Frame Graph Viewport Surface Color State Executor v1",
    "RhiViewportSurface",
    "viewport_color",
    "Frame Graph Texture Aliasing Barrier Command v1",
    "record_frame_graph_texture_aliasing_barriers",
    "Frame Graph Automatic Aliasing Barrier Insertion v1",
    "Package Streaming Frame Graph Texture Binding Handoff v1",
    "make_runtime_package_streaming_frame_graph_texture_bindings",
    "Package Static Mesh Upload Binding Transaction v1",
    "upload_runtime_package_streaming_mesh_gpu_bindings",
    "Frame Graph Render Pass Envelope v1",
    "render_passes_recorded",
    "Frame Graph v1 1.0 Scope Closeout v1 closes frame-graph-v1",
    "upload-staging-v1",
    "native async upload execution",
    "package skinned/morph streaming",
    "staging-pool production adoption"
    )) {
        Assert-ContainsText $recommendedNextPlanText $needle "engine/agent/manifest.json aiOperableProductionLoop recommendedNextPlan frame-graph closeout and upload-staging next gap"
    }
}
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$masterPlanText = Get-AgentSurfaceText $productionCompletionMasterPlanRelativePath
$physicsJointsPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$physicsBenchmarkPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$physicsJoltPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$physicsCloseoutPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
Assert-ContainsText $masterPlanText "Gap Burn-down Execution Strategy" "production completion master plan"
Assert-ContainsText $masterPlanText "Execute this master plan by burning down one" "production completion master plan"
foreach ($masterPlanCadenceNeedle in @("dated capability/gap-cluster/milestone plan", "Plan width may be broader than PR/phase width", "one reviewable purpose", 'one fresh `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`', 'tools/build.ps1` only when standalone build/install evidence is requested', "lightweight static checks for docs/agent-only slices")) {
    Assert-ContainsText $masterPlanText $masterPlanCadenceNeedle "production completion master plan"
}
Assert-ContainsText $masterPlanText "physics-1-0-collision-system-closeout-v1" "production completion master plan"
Assert-ContainsText $planRegistryText "Active gap burn-down" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "physics-jolt-adapter-gate-v1" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "physics-1-0-collision-system-closeout-v1" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "physics-benchmark-determinism-gates-v1" "docs/superpowers/plans/README.md"
Assert-ContainsText $physicsJointsPlanText 'Gap:** `physics-1-0-collision-system` Phase P1' "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
Assert-ContainsText $physicsJointsPlanText '**Status:** Completed.' "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
Assert-ContainsText $physicsBenchmarkPlanText 'Plan ID:** `physics-benchmark-determinism-gates-v1`' "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
Assert-ContainsText $physicsBenchmarkPlanText '**Status:** Completed.' "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
Assert-ContainsText $physicsBenchmarkPlanText 'Gap:** `physics-1-0-collision-system` Phase P2' "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
Assert-ContainsText $physicsBenchmarkPlanText "count-based" "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
Assert-ContainsText $physicsBenchmarkPlanText "budget gates" "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
Assert-ContainsText $physicsBenchmarkPlanText "PhysicsReplaySignature3D" "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
Assert-ContainsText $physicsBenchmarkPlanText "evaluate_physics_determinism_gate_3d" "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
Assert-ContainsText $physicsJoltPlanText 'Plan ID:** `physics-jolt-adapter-gate-v1`' "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
Assert-ContainsText $physicsJoltPlanText '**Status:** Completed.' "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
Assert-ContainsText $physicsJoltPlanText 'Gap:** `physics-1-0-collision-system` Phase P3' "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
Assert-ContainsText $physicsJoltPlanText "explicit 1.0 exclusion" "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
Assert-ContainsText $physicsCloseoutPlanText 'Plan ID:** `physics-1-0-collision-system-closeout-v1`' "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
Assert-ContainsText $physicsCloseoutPlanText '**Status:** Completed.' "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
Assert-ContainsText $physicsCloseoutPlanText 'Gap:** `physics-1-0-collision-system` Phase P4' "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$queueSyncPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$queueSyncPackagePlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$computeMorphSkinPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$runtimeSceneComputeMorphSkinPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$generatedComputeMorphSkinPackagePlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$computeMorphAsyncTelemetryPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$generatedComputeMorphAsyncTelemetryPackagePlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$runtimeComputeMorphAsyncOverlapPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$runtimeComputeMorphPipelinedOutputRingPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$runtimeComputeMorphPipelinedSchedulingPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$rhiD3d12PerQueueFencePlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$rhiD3d12QueueTimestampPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$rhiD3d12QueueClockCalibrationPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$rhiD3d12CalibratedQueueTimingPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$runtimeComputeMorphCalibratedOverlapPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$rhiD3d12SubmittedCommandTimingPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$runtimeComputeMorphSubmittedOverlapPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$rhiVulkanComputeDispatchPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$runtimeComputeMorphVulkanProofPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$runtimeComputeMorphRendererConsumptionVulkanPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$runtimeComputeMorphNormalTangentOutputVulkanPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$generatedComputeMorphPackageVulkanPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$generatedComputeMorphSkinPackageVulkanPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$nativeSpriteBatchingExecutionPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$spriteAnimationPackagePlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$tilemapEditorRuntimeUxPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$inputRebindingProfileUxPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$runtimeInputRebindingCapturePlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$runtimeInputRebindingFocusConsumptionPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$runtimeUiFontImageAdapterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$runtimeUiTextShapingRequestPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$runtimeUiFontRasterizationRequestPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$runtimeUiPlatformTextInputSessionPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$runtimeUiImageDecodeRequestPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$runtimeUiPngImageDecodingAdapterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$gameplayDebugOverlayPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$crashTelemetryTraceOpsPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$androidReleaseDeviceMatrixPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$appleMetalIosHostEvidencePlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$mobilePackagingScriptText = Get-AgentSurfaceText "tools/check-mobile-packaging.ps1"
$appleHostEvidenceScriptText = Get-AgentSurfaceText "tools/check-apple-host-evidence.ps1"
$appleHostHelpersText = Get-AgentSurfaceText "tools/apple-host-helpers.ps1"
$productionReadinessAuditPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$productionReadinessAuditScriptText = Get-AgentSurfaceText "tools/check-production-readiness-audit.ps1"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$coreDiagnosticsHeaderText = Get-AgentSurfaceText "engine/core/include/mirakana/core/diagnostics.hpp"
$coreTestsText = Get-AgentSurfaceText "tests/unit/core_tests.cpp"
$tilemapMetadataSourceText = Get-AgentSurfaceText "engine/assets/src/tilemap_metadata.cpp"
$uiAtlasMetadataSourceText = Get-AgentSurfaceText "engine/assets/src/ui_atlas_metadata.cpp"
$sessionServicesSourceText = Get-AgentSurfaceText "engine/runtime/src/session_services.cpp"
$newGameToolText = Get-AgentSurfaceText "tools/new-game.ps1"
$newGameHelpersText = Get-AgentSurfaceText "tools/new-game-helpers.ps1"
$newGameTemplatesText = Get-AgentSurfaceText "tools/new-game-templates.ps1"
Assert-ContainsText $newGameToolText "new-game-helpers.ps1" "tools/new-game.ps1"
Assert-ContainsText $newGameToolText "new-game-templates.ps1" "tools/new-game.ps1"
Assert-ContainsText $newGameHelpersText "function Format-CppSourceText" "tools/new-game-helpers.ps1"
Assert-ContainsText $newGameHelpersText "Get-ClangFormatCommand" "tools/new-game-helpers.ps1"
Assert-ContainsText $newGameTemplatesText "function New-DesktopRuntime3DMainCpp" "tools/new-game-templates.ps1"
Assert-ContainsText $newGameTemplatesText "function New-DesktopRuntime3DPackageFiles" "tools/new-game-templates.ps1"
Assert-ContainsText $newGameToolText '$mainCpp = Format-CppSourceText -Text $mainCpp' "tools/new-game.ps1"
$newGameToolText = "$newGameToolText`n$newGameTemplatesText"
$d3d12RhiTestsText = Get-AgentSurfaceText "tests/unit/d3d12_rhi_tests.cpp"
$testFrameworkText = Get-AgentSurfaceText "tests/test_framework.hpp"
$renderingSkillText = Get-AgentSurfaceText ".agents/skills/rendering-change/SKILL.md"
$claudeRenderingSkillText = Get-AgentSurfaceText ".claude/skills/gameengine-rendering/SKILL.md"
$cursorRenderingSkillText = Get-AgentSurfaceText ".cursor/skills/gameengine-rendering/SKILL.md"
$gameDevelopmentSkillText = Get-AgentSurfaceText ".agents/skills/gameengine-game-development/SKILL.md"
$claudeGameDevelopmentSkillText = Get-AgentSurfaceText ".claude/skills/gameengine-game-development/SKILL.md"
Assert-ContainsText $d3d12RhiTestsText "d3d12_test_device_desc" "tests/unit/d3d12_rhi_tests.cpp"
Assert-ContainsText $d3d12RhiTestsText ".prefer_warp = true" "tests/unit/d3d12_rhi_tests.cpp"
Assert-DoesNotContainText $d3d12RhiTestsText ".prefer_warp = false" "tests/unit/d3d12_rhi_tests.cpp"
Assert-ContainsText $d3d12RhiTestsText "result.used_warp" "tests/unit/d3d12_rhi_tests.cpp"
Assert-ContainsText $d3d12RhiTestsText "context->used_warp()" "tests/unit/d3d12_rhi_tests.cpp"
Assert-ContainsText $testFrameworkText "std::flush" "tests/test_framework.hpp"
Assert-ContainsText $testingContent "Microsoft WARP" "docs/testing.md"
Assert-ContainsText $workflowsContent "D3D12 unit tests use Microsoft WARP" "docs/workflows.md"
Assert-ContainsText $renderingSkillText "Microsoft WARP" ".agents/skills/rendering-change/SKILL.md"
Assert-ContainsText $claudeRenderingSkillText "Microsoft WARP" ".claude/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $cursorRenderingSkillText "Microsoft WARP" ".cursor/skills/gameengine-rendering/SKILL.md"
Assert-ContainsText $gameDevelopmentSkillText "tools/new-game-templates.ps1" ".agents/skills/gameengine-game-development/SKILL.md"
Assert-ContainsText $claudeGameDevelopmentSkillText "tools/new-game-templates.ps1" ".claude/skills/gameengine-game-development/SKILL.md"
Assert-ContainsText $gameDevelopmentSkillText "plan_placeholder_asset_bundle" ".agents/skills/gameengine-game-development/SKILL.md"
Assert-ContainsText $gameDevelopmentSkillText "PlaceholderAssetBundleRequest" ".agents/skills/gameengine-game-development/SKILL.md"
Assert-ContainsText $claudeGameDevelopmentSkillText "plan_placeholder_asset_bundle" ".claude/skills/gameengine-game-development/SKILL.md"
Assert-ContainsText $claudeGameDevelopmentSkillText "PlaceholderAssetBundleRequest" ".claude/skills/gameengine-game-development/SKILL.md"
Assert-ContainsText $gameDevelopmentSkillText "plan_placeholder_asset_cook_package" ".agents/skills/gameengine-game-development/SKILL.md"
Assert-ContainsText $gameDevelopmentSkillText "PlaceholderAssetCookPackageRequest" ".agents/skills/gameengine-game-development/SKILL.md"
Assert-ContainsText $claudeGameDevelopmentSkillText "plan_placeholder_asset_cook_package" ".claude/skills/gameengine-game-development/SKILL.md"
Assert-ContainsText $claudeGameDevelopmentSkillText "PlaceholderAssetCookPackageRequest" ".claude/skills/gameengine-game-development/SKILL.md"
Assert-ContainsText $gameDevelopmentSkillText "plan_sprite_atlas_source_authoring" ".agents/skills/gameengine-game-development/SKILL.md"
Assert-ContainsText $gameDevelopmentSkillText "SpriteAtlasSourceAuthoringDesc" ".agents/skills/gameengine-game-development/SKILL.md"
Assert-ContainsText $claudeGameDevelopmentSkillText "plan_sprite_atlas_source_authoring" ".claude/skills/gameengine-game-development/SKILL.md"
Assert-ContainsText $claudeGameDevelopmentSkillText "SpriteAtlasSourceAuthoringDesc" ".claude/skills/gameengine-game-development/SKILL.md"
Assert-ContainsText $gameDevelopmentSkillText "RuntimeGameplayDebugOverlayRowDesc" ".agents/skills/gameengine-game-development/SKILL.md"
Assert-ContainsText $gameDevelopmentSkillText "plan_runtime_gameplay_debug_overlay" ".agents/skills/gameengine-game-development/SKILL.md"
Assert-ContainsText $claudeGameDevelopmentSkillText "RuntimeGameplayDebugOverlayRowDesc" ".claude/skills/gameengine-game-development/SKILL.md"
Assert-ContainsText $claudeGameDevelopmentSkillText "plan_runtime_gameplay_debug_overlay" ".claude/skills/gameengine-game-development/SKILL.md"
Assert-ContainsText $gameplayDebugOverlayPlanText "Phase 1: Overlay Row Contract" "Engine Gameplay Debug Overlay plan"
Assert-ContainsText $gameplayDebugOverlayPlanText "RED tests describe overlay row planning" "Engine Gameplay Debug Overlay plan"
Assert-ContainsText $gameplayDebugOverlayPlanText "RuntimeGameplayDebugOverlayPlan" "Engine Gameplay Debug Overlay plan"
Assert-ContainsText $gameplayDebugOverlayPlanText "plan_runtime_gameplay_debug_overlay" "Engine Gameplay Debug Overlay plan"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-generated-3d-compute-morph-package-smoke-d3d12-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-runtime-scene-quaternion-animation-transform-binding-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-runtime-rhi-compute-morph-normal-tangent-output-d3d12-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-generated-3d-compute-morph-normal-tangent-package-smoke-d3d12-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-runtime-rhi-compute-morph-queue-synchronization-d3d12-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-generated-3d-compute-morph-queue-sync-package-smoke-d3d12-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-runtime-rhi-compute-morph-skin-composition-d3d12-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-runtime-scene-rhi-compute-morph-skin-palette-d3d12-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-generated-3d-compute-morph-skin-package-smoke-d3d12-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-runtime-rhi-compute-morph-async-telemetry-d3d12-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-generated-3d-compute-morph-async-telemetry-package-smoke-d3d12-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-runtime-rhi-compute-morph-async-overlap-evidence-d3d12-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-runtime-rhi-compute-morph-pipelined-output-ring-d3d12-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-runtime-rhi-compute-morph-pipelined-scheduling-d3d12-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-rhi-d3d12-per-queue-fence-synchronization-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-rhi-d3d12-queue-timestamp-measurement-foundation-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-rhi-d3d12-queue-clock-calibration-foundation-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-rhi-d3d12-calibrated-queue-timing-diagnostics-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-runtime-rhi-compute-morph-calibrated-overlap-diagnostics-d3d12-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-rhi-d3d12-submitted-command-calibrated-timing-scopes-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-runtime-rhi-compute-morph-submitted-overlap-diagnostics-d3d12-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-rhi-vulkan-compute-dispatch-foundation-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-runtime-rhi-compute-morph-vulkan-proof-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-runtime-rhi-compute-morph-renderer-consumption-vulkan-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-runtime-rhi-compute-morph-normal-tangent-output-vulkan-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-generated-3d-compute-morph-package-smoke-vulkan-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-07-generated-3d-compute-morph-skin-package-smoke-vulkan-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-2d-native-sprite-batching-execution-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-2d-sprite-animation-package-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-2d-tilemap-editor-runtime-ux-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-08-runtime-input-rebinding-capture-contract-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-runtime-ui-font-image-adapter-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-07-runtime-ui-font-rasterization-request-plan-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-08-runtime-ui-png-image-decoding-adapter-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-crash-telemetry-trace-ops-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-android-release-device-matrix-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-apple-metal-ios-host-evidence-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-production-1-0-readiness-audit-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-06-runtime-rhi-compute-morph-renderer-consumption-d3d12-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-05-runtime-rhi-compute-morph-d3d12-proof-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $historicalPlanEvidenceText "2026-05-05-rhi-compute-dispatch-foundation-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $masterPlanText "rhi-compute-dispatch-foundation-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "runtime-rhi-compute-morph-d3d12-proof-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "runtime-rhi-compute-morph-renderer-consumption-d3d12-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "generated-3d-compute-morph-package-smoke-d3d12-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "runtime-rhi-compute-morph-normal-tangent-output-d3d12-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "generated-3d-compute-morph-normal-tangent-package-smoke-d3d12-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "runtime-rhi-compute-morph-queue-synchronization-d3d12-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "generated-3d-compute-morph-queue-sync-package-smoke-d3d12-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "runtime-rhi-compute-morph-skin-composition-d3d12-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "runtime-scene-rhi-compute-morph-skin-palette-d3d12-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "generated-3d-compute-morph-skin-package-smoke-d3d12-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "runtime-rhi-compute-morph-async-telemetry-d3d12-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "generated-3d-compute-morph-async-telemetry-package-smoke-d3d12-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "runtime-rhi-compute-morph-async-overlap-evidence-d3d12-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "runtime-rhi-compute-morph-pipelined-output-ring-d3d12-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "runtime-rhi-compute-morph-pipelined-scheduling-d3d12-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "rhi-d3d12-per-queue-fence-synchronization-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "rhi-d3d12-queue-timestamp-measurement-foundation-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "rhi-d3d12-queue-clock-calibration-foundation-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "rhi-d3d12-calibrated-queue-timing-diagnostics-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "runtime-rhi-compute-morph-calibrated-overlap-diagnostics-d3d12-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "rhi-d3d12-submitted-command-calibrated-timing-scopes-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "runtime-rhi-compute-morph-submitted-overlap-diagnostics-d3d12-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "rhi-vulkan-compute-dispatch-foundation-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "runtime-rhi-compute-morph-vulkan-proof-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "runtime-rhi-compute-morph-renderer-consumption-vulkan-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "runtime-rhi-compute-morph-normal-tangent-output-vulkan-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "generated-3d-compute-morph-package-smoke-vulkan-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "runtime-rhi-compute-morph-vulkan-proof-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "runtime-rhi-compute-morph-renderer-consumption-vulkan-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "runtime-rhi-compute-morph-normal-tangent-output-vulkan-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "runtime-ui-font-image-adapter-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "UiRendererGlyphAtlasPalette" "production completion master plan"
Assert-ContainsText $masterPlanText "Runtime UI Font Rasterization Request Plan v1" "production completion master plan"
Assert-ContainsText $masterPlanText "IFontRasterizerAdapter" "production completion master plan"
Assert-ContainsText $masterPlanText "Runtime UI PNG Image Decoding Adapter v1" "production completion master plan"
Assert-ContainsText $masterPlanText "PngImageDecodingAdapter" "production completion master plan"
Assert-ContainsText $masterPlanText "crash-telemetry-trace-ops-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "DiagnosticsOpsPlan" "production completion master plan"
Assert-ContainsText $masterPlanText "android-release-device-matrix-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "Mirakanai_API36" "production completion master plan"
Assert-ContainsText $masterPlanText "apple-metal-ios-host-evidence-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "apple-host-evidence-check" "production completion master plan"
Assert-ContainsText $masterPlanText "production-1-0-readiness-audit-v1" "production completion master plan"
Assert-ContainsText $masterPlanText "production-readiness-audit-check" "production completion master plan"
Assert-ContainsText $crashTelemetryTraceOpsPlanText "Crash Telemetry Trace Ops v1" "Crash Telemetry Trace Ops plan"
Assert-ContainsText $crashTelemetryTraceOpsPlanText "**Status:** Completed" "Crash Telemetry Trace Ops plan"
Assert-ContainsText $crashTelemetryTraceOpsPlanText "build_diagnostics_ops_plan" "Crash Telemetry Trace Ops plan"
Assert-ContainsText $crashTelemetryTraceOpsPlanText "without adding native dump writing" "Crash Telemetry Trace Ops plan"
Assert-ContainsText $coreDiagnosticsHeaderText "DiagnosticsOpsPlan" "engine/core/include/mirakana/core/diagnostics.hpp"
Assert-ContainsText $coreDiagnosticsHeaderText "DiagnosticsOpsArtifactStatus" "engine/core/include/mirakana/core/diagnostics.hpp"
Assert-ContainsText $coreDiagnosticsHeaderText "build_diagnostics_ops_plan" "engine/core/include/mirakana/core/diagnostics.hpp"
Assert-ContainsText $coreTestsText "diagnostics ops plan reports trace summary and unsupported upload boundaries" "tests/unit/core_tests.cpp"
foreach ($memoryDiagnosticsNeedle in @(
        "MemoryLifetimeClass",
        "MemoryCounterRow",
        "MemoryDiagnosticsSummary",
        "MemoryBudgetPressure",
        "MemoryDiagnosticsCode",
        "MemoryDiagnosticsStatus",
        "summarize_memory_diagnostics"
    )) {
    Assert-ContainsText $coreDiagnosticsHeaderText $memoryDiagnosticsNeedle "engine/core/include/mirakana/core/diagnostics.hpp"
}
foreach ($memoryDiagnosticsTestNeedle in @(
        "memory diagnostics summarize classes high water and budget pressure",
        "memory diagnostics fail closed for empty rows and exceeded budgets",
        "memory diagnostics fail closed for invalid stale and safe point rows",
        "memory diagnostics labels are stable"
    )) {
    Assert-ContainsText $coreTestsText $memoryDiagnosticsTestNeedle "tests/unit/core_tests.cpp"
}
foreach ($memoryDiagnosticsDocCheck in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $aiGameDevelopmentContent; Label = "docs/ai-game-development.md" }
    )) {
    foreach ($memoryDiagnosticsDocNeedle in @(
            "Memory Diagnostics v1",
            "MemoryCounterRow",
            "summarize_memory_diagnostics",
            "stale-generation diagnostics",
            "use-after-safe-point diagnostics",
            "sample_desktop_runtime_game --require-memory-diagnostics",
            "memory_diagnostics_status=ready",
            '`memory_diagnostics_*` counters'
        )) {
        Assert-ContainsText $memoryDiagnosticsDocCheck.Text $memoryDiagnosticsDocNeedle $memoryDiagnosticsDocCheck.Label
    }
}
foreach ($diagnosticsOpsDocCheck in @(
    @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
    @{ Text = $roadmapText; Label = "docs/roadmap.md" },
    @{ Text = $masterPlanText; Label = "production completion master plan" },
    @{ Text = $testingContent; Label = "docs/testing.md" },
    @{ Text = $workflowsContent; Label = "docs/workflows.md" }
)) {
    Assert-ContainsText $diagnosticsOpsDocCheck.Text "DiagnosticsOpsPlan" $diagnosticsOpsDocCheck.Label
    Assert-ContainsText $diagnosticsOpsDocCheck.Text "telemetry" $diagnosticsOpsDocCheck.Label
}
Assert-ContainsText $androidReleaseDeviceMatrixPlanText "Android Release Device Matrix v1" "Android Release Device Matrix plan"
Assert-ContainsText $androidReleaseDeviceMatrixPlanText "**Status:** Completed" "Android Release Device Matrix plan"
Assert-ContainsText $androidReleaseDeviceMatrixPlanText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-mobile-android.ps1 -Game sample_headless -Configuration Debug" "Android Release Device Matrix plan"
Assert-ContainsText $androidReleaseDeviceMatrixPlanText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-android-release-package.ps1 -Game sample_headless -UseLocalValidationKey" "Android Release Device Matrix plan"
Assert-ContainsText $androidReleaseDeviceMatrixPlanText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/smoke-android-package.ps1 -Game sample_headless -Configuration Release -SkipBuild -StartEmulator -AvdName Mirakanai_API36" "Android Release Device Matrix plan"
Assert-ContainsText $androidReleaseDeviceMatrixPlanText "without claiming Play upload" "Android Release Device Matrix plan"
foreach ($androidDocCheck in @(
    @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
    @{ Text = $testingContent; Label = "docs/testing.md" },
    @{ Text = $workflowsContent; Label = "docs/workflows.md" },
    @{ Text = $roadmapText; Label = "docs/roadmap.md" },
    @{ Text = $masterPlanText; Label = "production completion master plan" }
)) {
    Assert-ContainsText $androidDocCheck.Text "Android Release Device Matrix v1" $androidDocCheck.Label
    Assert-ContainsText $androidDocCheck.Text "sample_headless" $androidDocCheck.Label
}
foreach ($portableFloatCheck in @(
    @{ Text = $tilemapMetadataSourceText; Label = "engine/assets/src/tilemap_metadata.cpp" },
    @{ Text = $uiAtlasMetadataSourceText; Label = "engine/assets/src/ui_atlas_metadata.cpp" },
    @{ Text = $sessionServicesSourceText; Label = "engine/runtime/src/session_services.cpp" }
)) {
    Assert-ContainsText $portableFloatCheck.Text "std::locale::classic()" $portableFloatCheck.Label
    if ($portableFloatCheck.Text -match "float\s+parsed\s*=\s*0\.0F;\s*const\s+auto\s+\[end,\s*error\]\s*=\s*std::from_chars") {
        Write-Error "$($portableFloatCheck.Label) must not use floating-point std::from_chars; Android NDK libc++ does not provide it."
    }
}
Assert-ContainsText $appleMetalIosHostEvidencePlanText "Apple Metal iOS Host Evidence v1" "Apple Metal iOS Host Evidence plan"
Assert-ContainsText $appleMetalIosHostEvidencePlanText "**Status:** Completed" "Apple Metal iOS Host Evidence plan"
Assert-ContainsText $appleMetalIosHostEvidencePlanText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-apple-host-evidence.ps1" "Apple Metal iOS Host Evidence plan"
Assert-ContainsText $appleMetalIosHostEvidencePlanText "apple-host-evidence-check: host-gated" "Apple Metal iOS Host Evidence plan"
Assert-ContainsText $appleHostEvidenceScriptText "apple-host-evidence-check: host-gated" "tools/check-apple-host-evidence.ps1"
Assert-ContainsText $appleHostEvidenceScriptText "xcodebuild" "tools/check-apple-host-evidence.ps1"
Assert-ContainsText $appleHostEvidenceScriptText "xcrun" "tools/check-apple-host-evidence.ps1"
Assert-ContainsText $appleHostEvidenceScriptText "metal" "tools/check-apple-host-evidence.ps1"
Assert-ContainsText $appleHostEvidenceScriptText "metallib" "tools/check-apple-host-evidence.ps1"
Assert-ContainsText $appleHostEvidenceScriptText "blocker kind=" "tools/check-apple-host-evidence.ps1"
foreach ($appleHelperNeedle in @("Test-IsMacOS", "Get-AppleDeveloperDirectory", "Test-FullXcodeDeveloperDirectory", "Test-IosSimulatorRuntimeAvailable")) {
    Assert-ContainsText $appleHostHelpersText $appleHelperNeedle "tools/apple-host-helpers.ps1"
}
Assert-ContainsText $mobilePackagingScriptText "apple-host-helpers.ps1" "tools/check-mobile-packaging.ps1"
Assert-ContainsText $mobilePackagingScriptText "Invoke-MobilePackagingProbe" "tools/check-mobile-packaging.ps1"
Assert-ContainsText $mobilePackagingScriptText 'Kill($true)' "tools/check-mobile-packaging.ps1"
Assert-ContainsText $mobilePackagingScriptText "OutputWaitMilliseconds" "tools/check-mobile-packaging.ps1"
Assert-ContainsText $mobilePackagingScriptText "Test-MobilePackagingDeviceProbeEnabled" "tools/check-mobile-packaging.ps1"
Assert-ContainsText $mobilePackagingScriptText "GITHUB_ACTIONS" "tools/check-mobile-packaging.ps1"
Assert-ContainsText $mobilePackagingScriptText "probe-timeout" "tools/check-mobile-packaging.ps1"
Assert-ContainsText $mobilePackagingScriptText "probe-skipped-ci" "tools/check-mobile-packaging.ps1"
Assert-ContainsText $mobilePackagingScriptText "blocker kind=" "tools/check-mobile-packaging.ps1"
Assert-DoesNotContainText $mobilePackagingScriptText '$childProcess.WaitForExit()' "tools/check-mobile-packaging.ps1"
Assert-ContainsText $workflowsContent "kind=<slug>" "docs/workflows.md"
Assert-ContainsText $workflowsContent "probe-timeout" "docs/workflows.md"
Assert-ContainsText $workflowsContent "probe-skipped-ci" "docs/workflows.md"
Assert-ContainsText $appleHostEvidenceScriptText "apple-host-helpers.ps1" "tools/check-apple-host-evidence.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/validate.ps1") "check-apple-host-evidence.ps1" "tools/validate.ps1"
foreach ($appleHostEvidenceDocCheck in @(
    @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
    @{ Text = $testingContent; Label = "docs/testing.md" },
    @{ Text = $workflowsContent; Label = "docs/workflows.md" },
    @{ Text = $roadmapText; Label = "docs/roadmap.md" },
    @{ Text = $masterPlanText; Label = "production completion master plan" }
)) {
    Assert-ContainsText $appleHostEvidenceDocCheck.Text "Apple Metal iOS Host Evidence v1" $appleHostEvidenceDocCheck.Label
    Assert-ContainsText $appleHostEvidenceDocCheck.Text "apple-host-evidence-check" $appleHostEvidenceDocCheck.Label
}
Assert-ContainsText $productionReadinessAuditPlanText "Production 1.0 Readiness Audit v1" "Production 1.0 Readiness Audit plan"
Assert-ContainsText $productionReadinessAuditPlanText "**Status:** Completed" "Production 1.0 Readiness Audit plan"
Assert-ContainsText $productionReadinessAuditPlanText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1" "Production 1.0 Readiness Audit plan"
Assert-ContainsText $productionReadinessAuditPlanText "production-readiness-audit-check: ok" "Production 1.0 Readiness Audit plan"
Assert-ContainsText $productionReadinessAuditScriptText "unsupportedProductionGaps" "tools/check-production-readiness-audit.ps1"
Assert-ContainsText $productionReadinessAuditScriptText "production-readiness-audit-check: ok" "tools/check-production-readiness-audit.ps1"
Assert-ContainsText (Get-AgentSurfaceText "tools/validate.ps1") "check-production-readiness-audit.ps1" "tools/validate.ps1"
foreach ($readinessAuditDocCheck in @(
    @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
    @{ Text = $testingContent; Label = "docs/testing.md" },
    @{ Text = $workflowsContent; Label = "docs/workflows.md" },
    @{ Text = $roadmapText; Label = "docs/roadmap.md" },
    @{ Text = $masterPlanText; Label = "production completion master plan" }
)) {
    Assert-ContainsText $readinessAuditDocCheck.Text "Production 1.0 Readiness Audit v1" $readinessAuditDocCheck.Label
    Assert-ContainsText $readinessAuditDocCheck.Text "production-readiness-audit-check" $readinessAuditDocCheck.Label
}
Assert-ContainsText $runtimeUiFontImageAdapterPlanText "Runtime UI Font Image Adapter v1" "Runtime UI font image adapter plan"
Assert-ContainsText $runtimeUiFontImageAdapterPlanText "**Status:** Completed" "Runtime UI font image adapter plan"
Assert-ContainsText $runtimeUiFontImageAdapterPlanText "UiRendererGlyphAtlasPalette" "Runtime UI font image adapter plan"
Assert-ContainsText $runtimeUiFontImageAdapterPlanText "make_ui_text_glyph_sprite_command" "Runtime UI font image adapter plan"
Assert-ContainsText $runtimeUiFontImageAdapterPlanText "text_glyph_sprites_submitted" "Runtime UI font image adapter plan"
Assert-ContainsText $runtimeUiFontImageAdapterPlanText "without claiming font loading/rasterization" "Runtime UI font image adapter plan"
Assert-ContainsText $runtimeUiFontRasterizationRequestPlanText "Runtime UI Font Rasterization Request Plan v1" "Runtime UI font rasterization request plan"
Assert-ContainsText $runtimeUiFontRasterizationRequestPlanText "**Status:** Completed" "Runtime UI font rasterization request plan"
Assert-ContainsText $runtimeUiFontRasterizationRequestPlanText "FontRasterizationRequestPlan" "Runtime UI font rasterization request plan"
Assert-ContainsText $runtimeUiFontRasterizationRequestPlanText "FontRasterizationResult" "Runtime UI font rasterization request plan"
Assert-ContainsText $runtimeUiFontRasterizationRequestPlanText "plan_font_rasterization_request" "Runtime UI font rasterization request plan"
Assert-ContainsText $runtimeUiFontRasterizationRequestPlanText "rasterize_font_glyph" "Runtime UI font rasterization request plan"
Assert-ContainsText $runtimeUiFontRasterizationRequestPlanText "IFontRasterizerAdapter" "Runtime UI font rasterization request plan"
Assert-ContainsText $runtimeUiFontRasterizationRequestPlanText "invalid_font_allocation" "Runtime UI font rasterization request plan"
Assert-ContainsText $runtimeUiFontRasterizationRequestPlanText "without font loading/rasterization implementations" "Runtime UI font rasterization request plan"
Assert-ContainsText $runtimeUiTextShapingRequestPlanText "Runtime UI Text Shaping Request Plan v1" "Runtime UI text shaping request plan"
Assert-ContainsText $runtimeUiTextShapingRequestPlanText "**Status:** Completed" "Runtime UI text shaping request plan"
Assert-ContainsText $runtimeUiTextShapingRequestPlanText "TextShapingRequestPlan" "Runtime UI text shaping request plan"
Assert-ContainsText $runtimeUiTextShapingRequestPlanText "TextShapingResult" "Runtime UI text shaping request plan"
Assert-ContainsText $runtimeUiTextShapingRequestPlanText "plan_text_shaping_request" "Runtime UI text shaping request plan"
Assert-ContainsText $runtimeUiTextShapingRequestPlanText "shape_text_run" "Runtime UI text shaping request plan"
Assert-ContainsText $runtimeUiTextShapingRequestPlanText "ITextShapingAdapter" "Runtime UI text shaping request plan"
Assert-ContainsText $runtimeUiTextShapingRequestPlanText "invalid_text_shaping_result" "Runtime UI text shaping request plan"
Assert-ContainsText $runtimeUiTextShapingRequestPlanText "without shaping implementations" "Runtime UI text shaping request plan"
Assert-ContainsText $runtimeUiImageDecodeRequestPlanText "Runtime UI Image Decode Request Plan v1" "Runtime UI image decode request plan"
Assert-ContainsText $runtimeUiImageDecodeRequestPlanText "**Status:** Completed" "Runtime UI image decode request plan"
Assert-ContainsText $runtimeUiImageDecodeRequestPlanText "ImageDecodeRequestPlan" "Runtime UI image decode request plan"
Assert-ContainsText $runtimeUiImageDecodeRequestPlanText "ImageDecodeDispatchResult" "Runtime UI image decode request plan"
Assert-ContainsText $runtimeUiImageDecodeRequestPlanText "ImageDecodePixelFormat" "Runtime UI image decode request plan"
Assert-ContainsText $runtimeUiImageDecodeRequestPlanText "plan_image_decode_request" "Runtime UI image decode request plan"
Assert-ContainsText $runtimeUiImageDecodeRequestPlanText "decode_image_request" "Runtime UI image decode request plan"
Assert-ContainsText $runtimeUiImageDecodeRequestPlanText "IImageDecodingAdapter" "Runtime UI image decode request plan"
Assert-ContainsText $runtimeUiImageDecodeRequestPlanText "invalid_image_decode_result" "Runtime UI image decode request plan"
Assert-ContainsText $runtimeUiImageDecodeRequestPlanText "MK_ui_renderer_tests" "Runtime UI image decode request plan"
Assert-ContainsText $runtimeUiImageDecodeRequestPlanText "without image decoding implementations" "Runtime UI image decode request plan"
Assert-ContainsText $runtimeUiPlatformTextInputSessionPlanText "Runtime UI Platform Text Input Session Plan v1" "Runtime UI platform text input session plan"
Assert-ContainsText $runtimeUiPlatformTextInputSessionPlanText "**Status:** Completed" "Runtime UI platform text input session plan"
Assert-ContainsText $runtimeUiPlatformTextInputSessionPlanText "PlatformTextInputSessionPlan" "Runtime UI platform text input session plan"
Assert-ContainsText $runtimeUiPlatformTextInputSessionPlanText "PlatformTextInputSessionResult" "Runtime UI platform text input session plan"
Assert-ContainsText $runtimeUiPlatformTextInputSessionPlanText "PlatformTextInputEndPlan" "Runtime UI platform text input session plan"
Assert-ContainsText $runtimeUiPlatformTextInputSessionPlanText "PlatformTextInputEndResult" "Runtime UI platform text input session plan"
Assert-ContainsText $runtimeUiPlatformTextInputSessionPlanText "begin_platform_text_input" "Runtime UI platform text input session plan"
Assert-ContainsText $runtimeUiPlatformTextInputSessionPlanText "end_platform_text_input" "Runtime UI platform text input session plan"
Assert-ContainsText $runtimeUiPlatformTextInputSessionPlanText "IPlatformIntegrationAdapter" "Runtime UI platform text input session plan"
Assert-ContainsText $runtimeUiPlatformTextInputSessionPlanText "MK_ui_renderer_tests" "Runtime UI platform text input session plan"
Assert-ContainsText $runtimeUiPlatformTextInputSessionPlanText "native text-input object/session ownership" "Runtime UI platform text input session plan"
Assert-ContainsText $runtimeUiPngImageDecodingAdapterPlanText "Runtime UI PNG Image Decoding Adapter v1" "Runtime UI PNG image decoding adapter plan"
Assert-ContainsText $runtimeUiPngImageDecodingAdapterPlanText "**Status:** Completed" "Runtime UI PNG image decoding adapter plan"
Assert-ContainsText $runtimeUiPngImageDecodingAdapterPlanText "PngImageDecodingAdapter" "Runtime UI PNG image decoding adapter plan"
Assert-ContainsText $runtimeUiPngImageDecodingAdapterPlanText "IImageDecodingAdapter" "Runtime UI PNG image decoding adapter plan"
Assert-ContainsText $runtimeUiPngImageDecodingAdapterPlanText "decode_audited_png_rgba8" "Runtime UI PNG image decoding adapter plan"
Assert-ContainsText $runtimeUiPngImageDecodingAdapterPlanText "ImageDecodePixelFormat::rgba8_unorm" "Runtime UI PNG image decoding adapter plan"
Assert-ContainsText $runtimeUiPngImageDecodingAdapterPlanText "MK_tools_tests" "Runtime UI PNG image decoding adapter plan"
Assert-ContainsText $runtimeUiPngImageDecodingAdapterPlanText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1" "Runtime UI PNG image decoding adapter plan"
$runtimeUiDecodedImageAtlasPackageBridgePlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
Assert-ContainsText $runtimeUiDecodedImageAtlasPackageBridgePlanText "Runtime UI Decoded Image Atlas Package Bridge v1" "Runtime UI decoded image atlas package bridge plan"
Assert-ContainsText $runtimeUiDecodedImageAtlasPackageBridgePlanText "**Status:** Completed" "Runtime UI decoded image atlas package bridge plan"
Assert-ContainsText $runtimeUiDecodedImageAtlasPackageBridgePlanText "PackedUiAtlasAuthoringDesc" "Runtime UI decoded image atlas package bridge plan"
Assert-ContainsText $runtimeUiDecodedImageAtlasPackageBridgePlanText "author_packed_ui_atlas_from_decoded_images" "Runtime UI decoded image atlas package bridge plan"
Assert-ContainsText $runtimeUiDecodedImageAtlasPackageBridgePlanText "plan_packed_ui_atlas_package_update" "Runtime UI decoded image atlas package bridge plan"
Assert-ContainsText $runtimeUiDecodedImageAtlasPackageBridgePlanText "apply_packed_ui_atlas_package_update" "Runtime UI decoded image atlas package bridge plan"
Assert-ContainsText $runtimeUiDecodedImageAtlasPackageBridgePlanText "GameEngine.CookedTexture.v1" "Runtime UI decoded image atlas package bridge plan"
Assert-ContainsText $runtimeUiDecodedImageAtlasPackageBridgePlanText "MK_tools_tests" "Runtime UI decoded image atlas package bridge plan"
$runtimeUiGlyphAtlasPackageBridgePlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
Assert-ContainsText $runtimeUiGlyphAtlasPackageBridgePlanText "Runtime UI Glyph Atlas Package Bridge v1" "Runtime UI glyph atlas package bridge plan"
Assert-ContainsText $runtimeUiGlyphAtlasPackageBridgePlanText "**Status:** Completed" "Runtime UI glyph atlas package bridge plan"
Assert-ContainsText $runtimeUiGlyphAtlasPackageBridgePlanText "UiAtlasMetadataGlyph" "Runtime UI glyph atlas package bridge plan"
Assert-ContainsText $runtimeUiGlyphAtlasPackageBridgePlanText "RuntimeUiAtlasGlyph" "Runtime UI glyph atlas package bridge plan"
Assert-ContainsText $runtimeUiGlyphAtlasPackageBridgePlanText "PackedUiGlyphAtlasAuthoringDesc" "Runtime UI glyph atlas package bridge plan"
Assert-ContainsText $runtimeUiGlyphAtlasPackageBridgePlanText "author_packed_ui_glyph_atlas_from_rasterized_glyphs" "Runtime UI glyph atlas package bridge plan"
Assert-ContainsText $runtimeUiGlyphAtlasPackageBridgePlanText "plan_packed_ui_glyph_atlas_package_update" "Runtime UI glyph atlas package bridge plan"
Assert-ContainsText $runtimeUiGlyphAtlasPackageBridgePlanText "apply_packed_ui_glyph_atlas_package_update" "Runtime UI glyph atlas package bridge plan"
Assert-ContainsText $runtimeUiGlyphAtlasPackageBridgePlanText "build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas" "Runtime UI glyph atlas package bridge plan"
Assert-ContainsText $runtimeUiGlyphAtlasPackageBridgePlanText "GameEngine.CookedTexture.v1" "Runtime UI glyph atlas package bridge plan"
Assert-ContainsText $runtimeUiGlyphAtlasPackageBridgePlanText "GameEngine.UiAtlas.v1" "Runtime UI glyph atlas package bridge plan"
Assert-ContainsText $runtimeUiGlyphAtlasPackageBridgePlanText "MK_tools_tests" "Runtime UI glyph atlas package bridge plan"
Assert-ContainsText $queueSyncPlanText "Runtime RHI Compute Morph Queue Synchronization D3D12 v1" "Runtime RHI compute morph queue synchronization plan"
Assert-ContainsText $queueSyncPlanText "wait_for_queue(QueueKind queue, FenceValue fence)" "Runtime RHI compute morph queue synchronization plan"
Assert-ContainsText $queueSyncPlanText 'without host-side `IRhiDevice::wait`' "Runtime RHI compute morph queue synchronization plan"
Assert-ContainsText $queueSyncPackagePlanText "Generated 3D Compute Morph Queue Sync Package Smoke D3D12 v1" "Generated 3D compute morph queue sync package smoke plan"
Assert-ContainsText $queueSyncPackagePlanText "**Status:** Completed" "Generated 3D compute morph queue sync package smoke plan"
Assert-ContainsText $queueSyncPackagePlanText "package-visible compute morph queue-wait smoke evidence" "Generated 3D compute morph queue sync package smoke plan"
Assert-ContainsText $queueSyncPackagePlanText "compute_morph_queue_waits" "Generated 3D compute morph queue sync package smoke plan"
Assert-ContainsText $queueSyncPackagePlanText "without exposing RHI or native backend details" "Generated 3D compute morph queue sync package smoke plan"
Assert-ContainsText $computeMorphSkinPlanText "Runtime RHI Compute Morph Skin Composition D3D12 v1" "Runtime RHI compute morph skin composition plan"
Assert-ContainsText $computeMorphSkinPlanText "**Status:** Completed" "Runtime RHI compute morph skin composition plan"
Assert-ContainsText $computeMorphSkinPlanText "distinct joint-palette descriptor" "Runtime RHI compute morph skin composition plan"
Assert-ContainsText $computeMorphSkinPlanText "skin_attribute_vertex_buffer" "Runtime RHI compute morph skin composition plan"
Assert-ContainsText $runtimeSceneComputeMorphSkinPlanText "Runtime Scene RHI Compute Morph Skin Palette D3D12 v1" "Runtime scene RHI compute morph skin palette plan"
Assert-ContainsText $runtimeSceneComputeMorphSkinPlanText "**Status:** Completed" "Runtime scene RHI compute morph skin palette plan"
Assert-ContainsText $runtimeSceneComputeMorphSkinPlanText "SceneSkinnedGpuBindingPalette" "Runtime scene RHI compute morph skin palette plan"
Assert-ContainsText $runtimeSceneComputeMorphSkinPlanText "RuntimeSceneComputeMorphSkinnedMeshBinding" "Runtime scene RHI compute morph skin palette plan"
Assert-ContainsText $runtimeSceneComputeMorphSkinPlanText "compute_morph_skinned_mesh_bindings" "Runtime scene RHI compute morph skin palette plan"
Assert-ContainsText $runtimeSceneComputeMorphSkinPlanText "compute_morph_output_position_bytes" "Runtime scene RHI compute morph skin palette plan"
Assert-ContainsText $generatedComputeMorphSkinPackagePlanText "Generated 3D Compute Morph Skin Package Smoke D3D12 v1" "Generated 3D compute morph skin package smoke plan"
Assert-ContainsText $generatedComputeMorphSkinPackagePlanText "**Status:** Completed" "Generated 3D compute morph skin package smoke plan"
Assert-ContainsText $generatedComputeMorphSkinPackagePlanText "DesktopRuntime3DPackage" "Generated 3D compute morph skin package smoke plan"
Assert-ContainsText $generatedComputeMorphSkinPackagePlanText "without exposing RHI or native handles" "Generated 3D compute morph skin package smoke plan"
Assert-ContainsText $computeMorphAsyncTelemetryPlanText "Runtime RHI Compute Morph Async Telemetry D3D12 v1" "Runtime RHI compute morph async telemetry plan"
Assert-ContainsText $computeMorphAsyncTelemetryPlanText "**Status:** Completed" "Runtime RHI compute morph async telemetry plan"
Assert-ContainsText $computeMorphAsyncTelemetryPlanText "without claiming performance overlap" "Runtime RHI compute morph async telemetry plan"
Assert-ContainsText $computeMorphAsyncTelemetryPlanText "last_graphics_queue_wait_fence_value" "Runtime RHI compute morph async telemetry plan"
Assert-ContainsText $generatedComputeMorphAsyncTelemetryPackagePlanText "Generated 3D Compute Morph Async Telemetry Package Smoke D3D12 v1" "Generated 3D compute morph async telemetry package smoke plan"
Assert-ContainsText $generatedComputeMorphAsyncTelemetryPackagePlanText "**Status:** Completed" "Generated 3D compute morph async telemetry package smoke plan"
Assert-ContainsText $generatedComputeMorphAsyncTelemetryPackagePlanText "without exposing queue/fence handles" "Generated 3D compute morph async telemetry package smoke plan"
Assert-ContainsText $generatedComputeMorphAsyncTelemetryPackagePlanText "--require-compute-morph-async-telemetry" "Generated 3D compute morph async telemetry package smoke plan"
Assert-ContainsText $runtimeComputeMorphAsyncOverlapPlanText "Runtime RHI Compute Morph Async Overlap Evidence D3D12 v1" "Runtime RHI compute morph async overlap evidence plan"
Assert-ContainsText $runtimeComputeMorphAsyncOverlapPlanText "**Status:** Completed" "Runtime RHI compute morph async overlap evidence plan"
Assert-ContainsText $runtimeComputeMorphAsyncOverlapPlanText "runtime/RHI-only D3D12 evidence" "Runtime RHI compute morph async overlap evidence plan"
Assert-ContainsText $runtimeComputeMorphAsyncOverlapPlanText "official Microsoft D3D12" "Runtime RHI compute morph async overlap evidence plan"
Assert-ContainsText $runtimeComputeMorphAsyncOverlapPlanText "RhiAsyncOverlapReadinessDiagnostics" "Runtime RHI compute morph async overlap evidence plan"
Assert-ContainsText $runtimeComputeMorphAsyncOverlapPlanText "not_proven_serial_dependency" "Runtime RHI compute morph async overlap evidence plan"
Assert-ContainsText $runtimeComputeMorphAsyncOverlapPlanText "Do not update generated package validation to require overlap" "Runtime RHI compute morph async overlap evidence plan"
Assert-ContainsText $runtimeComputeMorphPipelinedOutputRingPlanText "Runtime RHI Compute Morph Pipelined Output Ring D3D12 v1" "Runtime RHI compute morph pipelined output ring plan"
Assert-ContainsText $runtimeComputeMorphPipelinedOutputRingPlanText "**Status:** Completed" "Runtime RHI compute morph pipelined output ring plan"
Assert-ContainsText $runtimeComputeMorphPipelinedOutputRingPlanText "multi-slot compute morph output-ring" "Runtime RHI compute morph pipelined output ring plan"
Assert-ContainsText $runtimeComputeMorphPipelinedOutputRingPlanText "not_proven_serial_dependency" "Runtime RHI compute morph pipelined output ring plan"
Assert-ContainsText $runtimeComputeMorphPipelinedOutputRingPlanText "Do not update generated" "Runtime RHI compute morph pipelined output ring plan"
Assert-ContainsText $runtimeComputeMorphPipelinedOutputRingPlanText "DesktopRuntime3DPackage" "Runtime RHI compute morph pipelined output ring plan"
Assert-ContainsText $runtimeComputeMorphPipelinedOutputRingPlanText "RuntimeMorphMeshComputeOutputSlot" "Runtime RHI compute morph pipelined output ring plan"
Assert-ContainsText $runtimeComputeMorphPipelinedOutputRingPlanText "output_slot_count" "Runtime RHI compute morph pipelined output ring plan"
Assert-ContainsText $runtimeComputeMorphPipelinedSchedulingPlanText "Runtime RHI Compute Morph Pipelined Scheduling D3D12 v1" "Runtime RHI compute morph pipelined scheduling plan"
Assert-ContainsText $runtimeComputeMorphPipelinedSchedulingPlanText "**Status:** Completed" "Runtime RHI compute morph pipelined scheduling plan"
Assert-ContainsText $runtimeComputeMorphPipelinedSchedulingPlanText "previously completed output slot" "Runtime RHI compute morph pipelined scheduling plan"
Assert-ContainsText $runtimeComputeMorphPipelinedSchedulingPlanText "different current slot" "Runtime RHI compute morph pipelined scheduling plan"
Assert-ContainsText $runtimeComputeMorphPipelinedSchedulingPlanText "Do not update generated" "Runtime RHI compute morph pipelined scheduling plan"
Assert-ContainsText $runtimeComputeMorphPipelinedSchedulingPlanText "RhiPipelinedComputeGraphicsScheduleEvidence" "Runtime RHI compute morph pipelined scheduling plan"
Assert-ContainsText $rhiD3d12PerQueueFencePlanText "RHI D3D12 Per-Queue Fence Synchronization v1" "RHI D3D12 per-queue fence synchronization plan"
Assert-ContainsText $rhiD3d12PerQueueFencePlanText "**Status:** Completed" "RHI D3D12 per-queue fence synchronization plan"
Assert-ContainsText $rhiD3d12PerQueueFencePlanText "per-queue fences" "RHI D3D12 per-queue fence synchronization plan"
Assert-ContainsText $rhiD3d12PerQueueFencePlanText "FenceValue" "RHI D3D12 per-queue fence synchronization plan"
Assert-ContainsText $rhiD3d12PerQueueFencePlanText "Do not expose native fences" "RHI D3D12 per-queue fence synchronization plan"
Assert-ContainsText $rhiD3d12QueueTimestampPlanText "RHI D3D12 Queue Timestamp Measurement Foundation v1" "RHI D3D12 queue timestamp measurement foundation plan"
Assert-ContainsText $rhiD3d12QueueTimestampPlanText "**Status:** Completed" "RHI D3D12 queue timestamp measurement foundation plan"
Assert-ContainsText $rhiD3d12QueueTimestampPlanText "Microsoft Learn: Timing" "RHI D3D12 queue timestamp measurement foundation plan"
Assert-ContainsText $rhiD3d12QueueTimestampPlanText 'Do not expose `ID3D12QueryHeap`' "RHI D3D12 queue timestamp measurement foundation plan"
Assert-ContainsText $rhiD3d12QueueTimestampPlanText "timestamp query heap recording" "RHI D3D12 queue timestamp measurement foundation plan"
Assert-ContainsText $rhiD3d12QueueTimestampPlanText "ResolveQueryData" "RHI D3D12 queue timestamp measurement foundation plan"
Assert-ContainsText $rhiD3d12QueueClockCalibrationPlanText "RHI D3D12 Queue Clock Calibration Foundation v1" "RHI D3D12 queue clock calibration foundation plan"
Assert-ContainsText $rhiD3d12QueueClockCalibrationPlanText "**Status:** Completed" "RHI D3D12 queue clock calibration foundation plan"
Assert-ContainsText $rhiD3d12QueueClockCalibrationPlanText "GetClockCalibration" "RHI D3D12 queue clock calibration foundation plan"
Assert-ContainsText $rhiD3d12QueueClockCalibrationPlanText "CPU QPC sample" "RHI D3D12 queue clock calibration foundation plan"
Assert-ContainsText $rhiD3d12QueueClockCalibrationPlanText "QueueClockCalibration" "RHI D3D12 queue clock calibration foundation plan"
Assert-ContainsText $rhiD3d12CalibratedQueueTimingPlanText "RHI D3D12 Calibrated Queue Timing Diagnostics v1" "RHI D3D12 calibrated queue timing diagnostics plan"
Assert-ContainsText $rhiD3d12CalibratedQueueTimingPlanText "**Status:** Completed" "RHI D3D12 calibrated queue timing diagnostics plan"
Assert-ContainsText $rhiD3d12CalibratedQueueTimingPlanText "calibrated interval rows" "RHI D3D12 calibrated queue timing diagnostics plan"
Assert-ContainsText $rhiD3d12CalibratedQueueTimingPlanText "QueueCalibratedTiming" "RHI D3D12 calibrated queue timing diagnostics plan"
Assert-ContainsText $runtimeComputeMorphCalibratedOverlapPlanText "Runtime RHI Compute Morph Calibrated Overlap Diagnostics D3D12 v1" "Runtime RHI compute morph calibrated overlap diagnostics plan"
Assert-ContainsText $runtimeComputeMorphCalibratedOverlapPlanText "**Status:** Completed" "Runtime RHI compute morph calibrated overlap diagnostics plan"
Assert-ContainsText $runtimeComputeMorphCalibratedOverlapPlanText "calibrated overlap diagnostic" "Runtime RHI compute morph calibrated overlap diagnostics plan"
Assert-ContainsText $runtimeComputeMorphCalibratedOverlapPlanText "RhiPipelinedComputeGraphicsScheduleEvidence" "Runtime RHI compute morph calibrated overlap diagnostics plan"
Assert-ContainsText $runtimeComputeMorphCalibratedOverlapPlanText "QueryPerformanceCounter" "Runtime RHI compute morph calibrated overlap diagnostics plan"
Assert-ContainsText $rhiD3d12SubmittedCommandTimingPlanText "RHI D3D12 Submitted Command Calibrated Timing Scopes v1" "RHI D3D12 submitted command calibrated timing scopes plan"
Assert-ContainsText $rhiD3d12SubmittedCommandTimingPlanText "**Status:** Completed" "RHI D3D12 submitted command calibrated timing scopes plan"
Assert-ContainsText $rhiD3d12SubmittedCommandTimingPlanText "actual submitted D3D12 graphics/compute command lists" "RHI D3D12 submitted command calibrated timing scopes plan"
Assert-ContainsText $rhiD3d12SubmittedCommandTimingPlanText "SubmittedCommandCalibratedTiming" "RHI D3D12 submitted command calibrated timing scopes plan"
Assert-ContainsText $rhiD3d12SubmittedCommandTimingPlanText "ResolveQueryData" "RHI D3D12 submitted command calibrated timing scopes plan"
Assert-ContainsText $runtimeComputeMorphSubmittedOverlapPlanText "Runtime RHI Compute Morph Submitted Overlap Diagnostics D3D12 v1" "Runtime RHI compute morph submitted overlap diagnostics plan"
Assert-ContainsText $runtimeComputeMorphSubmittedOverlapPlanText "**Status:** Completed" "Runtime RHI compute morph submitted overlap diagnostics plan"
Assert-ContainsText $runtimeComputeMorphSubmittedOverlapPlanText "D3D12-only helper" "Runtime RHI compute morph submitted overlap diagnostics plan"
Assert-ContainsText $runtimeComputeMorphSubmittedOverlapPlanText "diagnose_rhi_device_submitted_command_compute_graphics_overlap" "Runtime RHI compute morph submitted overlap diagnostics plan"
Assert-ContainsText $runtimeComputeMorphSubmittedOverlapPlanText "actual submitted compute/graphics fences" "Runtime RHI compute morph submitted overlap diagnostics plan"
Assert-ContainsText $rhiVulkanComputeDispatchPlanText "RHI Vulkan Compute Dispatch Foundation v1" "RHI Vulkan compute dispatch foundation plan"
Assert-ContainsText $rhiVulkanComputeDispatchPlanText "**Status:** Completed" "RHI Vulkan compute dispatch foundation plan"
Assert-ContainsText $rhiVulkanComputeDispatchPlanText "vkCreateComputePipelines" "RHI Vulkan compute dispatch foundation plan"
Assert-ContainsText $rhiVulkanComputeDispatchPlanText "vkCmdDispatch" "RHI Vulkan compute dispatch foundation plan"
Assert-ContainsText $rhiVulkanComputeDispatchPlanText "MK_VULKAN_TEST_COMPUTE_SPV" "RHI Vulkan compute dispatch foundation plan"
Assert-ContainsText $rhiVulkanComputeDispatchPlanText "Do not claim Vulkan compute morph parity" "RHI Vulkan compute dispatch foundation plan"
Assert-ContainsText $runtimeComputeMorphVulkanProofPlanText "Runtime RHI Compute Morph Vulkan Proof v1" "Runtime RHI compute morph Vulkan proof plan"
Assert-ContainsText $runtimeComputeMorphVulkanProofPlanText "**Status:** Completed" "Runtime RHI compute morph Vulkan proof plan"
Assert-ContainsText $runtimeComputeMorphVulkanProofPlanText "RuntimeMorphMeshComputeBinding" "Runtime RHI compute morph Vulkan proof plan"
Assert-ContainsText $runtimeComputeMorphVulkanProofPlanText "create_runtime_morph_mesh_compute_binding" "Runtime RHI compute morph Vulkan proof plan"
Assert-ContainsText $runtimeComputeMorphVulkanProofPlanText "MK_VULKAN_TEST_COMPUTE_MORPH_SPV" "Runtime RHI compute morph Vulkan proof plan"
Assert-ContainsText $runtimeComputeMorphVulkanProofPlanText "Do not claim generated-package Vulkan compute morph readiness" "Runtime RHI compute morph Vulkan proof plan"
Assert-ContainsText $runtimeComputeMorphRendererConsumptionVulkanPlanText "Runtime RHI Compute Morph Renderer Consumption Vulkan v1" "Runtime RHI compute morph renderer consumption Vulkan plan"
Assert-ContainsText $runtimeComputeMorphRendererConsumptionVulkanPlanText "**Status:** Completed" "Runtime RHI compute morph renderer consumption Vulkan plan"
Assert-ContainsText $runtimeComputeMorphRendererConsumptionVulkanPlanText "make_runtime_compute_morph_output_mesh_gpu_binding" "Runtime RHI compute morph renderer consumption Vulkan plan"
Assert-ContainsText $runtimeComputeMorphRendererConsumptionVulkanPlanText "RhiFrameRenderer" "Runtime RHI compute morph renderer consumption Vulkan plan"
Assert-ContainsText $runtimeComputeMorphRendererConsumptionVulkanPlanText "MK_VULKAN_TEST_COMPUTE_MORPH_RENDER_VERTEX_SPV" "Runtime RHI compute morph renderer consumption Vulkan plan"
Assert-ContainsText $runtimeComputeMorphRendererConsumptionVulkanPlanText "MK_VULKAN_TEST_COMPUTE_MORPH_RENDER_FRAGMENT_SPV" "Runtime RHI compute morph renderer consumption Vulkan plan"
Assert-ContainsText $runtimeComputeMorphRendererConsumptionVulkanPlanText "Do not claim generated-package Vulkan compute morph readiness" "Runtime RHI compute morph renderer consumption Vulkan plan"
Assert-ContainsText $runtimeComputeMorphNormalTangentOutputVulkanPlanText "Runtime RHI Compute Morph NORMAL/TANGENT Output Vulkan v1" "Runtime RHI compute morph normal/tangent output Vulkan plan"
Assert-ContainsText $runtimeComputeMorphNormalTangentOutputVulkanPlanText "**Status:** Completed" "Runtime RHI compute morph normal/tangent output Vulkan plan"
Assert-ContainsText $runtimeComputeMorphNormalTangentOutputVulkanPlanText "RuntimeMorphMeshComputeBindingOptions::output_normal_usage" "Runtime RHI compute morph normal/tangent output Vulkan plan"
Assert-ContainsText $runtimeComputeMorphNormalTangentOutputVulkanPlanText "output_tangent_usage" "Runtime RHI compute morph normal/tangent output Vulkan plan"
Assert-ContainsText $runtimeComputeMorphNormalTangentOutputVulkanPlanText 'descriptor bindings `4..7`' "Runtime RHI compute morph normal/tangent output Vulkan plan"
Assert-ContainsText $runtimeComputeMorphNormalTangentOutputVulkanPlanText "MK_VULKAN_TEST_COMPUTE_MORPH_TANGENT_FRAME_SPV" "Runtime RHI compute morph normal/tangent output Vulkan plan"
Assert-ContainsText $runtimeComputeMorphNormalTangentOutputVulkanPlanText "vulkan_compute_morph_tangent_frame.cs.spv" "Runtime RHI compute morph normal/tangent output Vulkan plan"
Assert-ContainsText $runtimeComputeMorphNormalTangentOutputVulkanPlanText "Do not claim generated-package Vulkan compute morph readiness" "Runtime RHI compute morph normal/tangent output Vulkan plan"
Assert-ContainsText $generatedComputeMorphPackageVulkanPlanText "Generated 3D Compute Morph Package Smoke Vulkan v1" "Generated 3D compute morph package smoke Vulkan plan"
Assert-ContainsText $generatedComputeMorphPackageVulkanPlanText "**Status:** Completed" "Generated 3D compute morph package smoke Vulkan plan"
Assert-ContainsText $generatedComputeMorphPackageVulkanPlanText "DesktopRuntime3DPackage" "Generated 3D compute morph package smoke Vulkan plan"
Assert-ContainsText $generatedComputeMorphPackageVulkanPlanText "Win32DesktopPresentationVulkanSceneRendererDesc" "Generated 3D compute morph package smoke Vulkan plan"
Assert-ContainsText $generatedComputeMorphPackageVulkanPlanText "--require-compute-morph" "Generated 3D compute morph package smoke Vulkan plan"
Assert-ContainsText $generatedComputeMorphPackageVulkanPlanText "Do not claim Vulkan NORMAL/TANGENT package smoke" "Generated 3D compute morph package smoke Vulkan plan"
Assert-ContainsText $generatedComputeMorphSkinPackageVulkanPlanText "Generated 3D Compute Morph Skin Package Smoke Vulkan v1" "Generated 3D compute morph skin package smoke Vulkan plan"
Assert-ContainsText $generatedComputeMorphSkinPackageVulkanPlanText "**Status:** Completed" "Generated 3D compute morph skin package smoke Vulkan plan"
Assert-ContainsText $generatedComputeMorphSkinPackageVulkanPlanText "DesktopRuntime3DPackage" "Generated 3D compute morph skin package smoke Vulkan plan"
Assert-ContainsText $generatedComputeMorphSkinPackageVulkanPlanText "Win32DesktopPresentationVulkanSceneRendererDesc::compute_morph_skinned_shader" "Generated 3D compute morph skin package smoke Vulkan plan"
Assert-ContainsText $generatedComputeMorphSkinPackageVulkanPlanText "compute_morph_skinned_mesh_bindings" "Generated 3D compute morph skin package smoke Vulkan plan"
Assert-ContainsText $generatedComputeMorphSkinPackageVulkanPlanText "without exposing Vulkan/native handles" "Generated 3D compute morph skin package smoke Vulkan plan"
Assert-ContainsText $nativeSpriteBatchingExecutionPlanText "2D Native Sprite Batching Execution v1" "2D native sprite batching execution plan"
Assert-ContainsText $nativeSpriteBatchingExecutionPlanText "**Status:** Completed" "2D native sprite batching execution plan"
Assert-ContainsText $nativeSpriteBatchingExecutionPlanText "plan_sprite_batches" "2D native sprite batching execution plan"
Assert-ContainsText $nativeSpriteBatchingExecutionPlanText "sample_2d_desktop_runtime_package" "2D native sprite batching execution plan"
Assert-ContainsText $nativeSpriteBatchingExecutionPlanText "without changing sprite order" "2D native sprite batching execution plan"
Assert-ContainsText $spriteAnimationPackagePlanText "2D Sprite Animation Package v1" "2D sprite animation package plan"
Assert-ContainsText $spriteAnimationPackagePlanText "**Status:** Completed" "2D sprite animation package plan"
Assert-ContainsText $spriteAnimationPackagePlanText "DesktopRuntime2DPackage" "2D sprite animation package plan"
Assert-ContainsText $spriteAnimationPackagePlanText "deterministic frame sampling" "2D sprite animation package plan"
Assert-ContainsText $spriteAnimationPackagePlanText "sprite_animation_frames_sampled=3" "2D sprite animation package plan"
Assert-ContainsText $tilemapEditorRuntimeUxPlanText "2D Tilemap Editor Runtime UX v1" "2D tilemap editor runtime UX plan"
Assert-ContainsText $tilemapEditorRuntimeUxPlanText "**Status:** Completed" "2D tilemap editor runtime UX plan"
Assert-ContainsText $tilemapEditorRuntimeUxPlanText "GameEngine.Tilemap.v1" "2D tilemap editor runtime UX plan"
Assert-ContainsText $tilemapEditorRuntimeUxPlanText "package-visible tilemap runtime/editor counters" "2D tilemap editor runtime UX plan"
Assert-ContainsText $tilemapEditorRuntimeUxPlanText "tilemap_cells_sampled=3" "2D tilemap editor runtime UX plan"
Assert-ContainsText $inputRebindingProfileUxPlanText "Input Rebinding Profile UX v1" "input rebinding profile UX plan"
Assert-ContainsText $inputRebindingProfileUxPlanText "**Status:** Completed" "input rebinding profile UX plan"
Assert-ContainsText $inputRebindingProfileUxPlanText "GameEngine.RuntimeInputRebindingProfile.v1" "input rebinding profile UX plan"
Assert-ContainsText $inputRebindingProfileUxPlanText "apply_runtime_input_rebinding_profile" "input rebinding profile UX plan"
Assert-ContainsText $inputRebindingProfileUxPlanText "EditorInputRebindingProfileReviewModel" "input rebinding profile UX plan"
Assert-ContainsText $runtimeInputRebindingCapturePlanText "Runtime Input Rebinding Capture Contract v1" "runtime input rebinding capture plan"
Assert-ContainsText $runtimeInputRebindingCapturePlanText "**Status:** Completed" "runtime input rebinding capture plan"
Assert-ContainsText $runtimeInputRebindingCapturePlanText "RuntimeInputRebindingCaptureRequest" "runtime input rebinding capture plan"
Assert-ContainsText $runtimeInputRebindingCapturePlanText "RuntimeInputRebindingCaptureResult" "runtime input rebinding capture plan"
Assert-ContainsText $runtimeInputRebindingCapturePlanText "capture_runtime_input_rebinding_action" "runtime input rebinding capture plan"
Assert-ContainsText $runtimeInputRebindingCapturePlanText "MK_runtime_tests" "runtime input rebinding capture plan"
Assert-ContainsText $runtimeInputRebindingFocusConsumptionPlanText "Runtime Input Rebinding Focus Consumption v1" "runtime input rebinding focus consumption plan"
Assert-ContainsText $runtimeInputRebindingFocusConsumptionPlanText "**Status:** Completed" "runtime input rebinding focus consumption plan"
Assert-ContainsText $runtimeInputRebindingFocusConsumptionPlanText "RuntimeInputRebindingFocusCaptureRequest" "runtime input rebinding focus consumption plan"
