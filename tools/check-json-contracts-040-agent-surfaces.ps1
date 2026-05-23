#requires -Version 7.0
#requires -PSEdition Core

# Chapter 4 for check-json-contracts.ps1 static contracts.

function Assert-UniqueStringArray {
    param(
        [Parameter(Mandatory = $true)][object[]]$Values,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $strings = @($Values | ForEach-Object { [string]$_ })
    if ($strings.Count -eq 0) {
        Write-Error "$Label must not be empty"
    }

    $duplicates = @($strings | Group-Object | Where-Object { $_.Count -gt 1 } | ForEach-Object { $_.Name })
    if ($duplicates.Count -gt 0) {
        Write-Error "$Label must not contain duplicate entries: $($duplicates -join ', ')"
    }
}

function Assert-StringSetEquals {
    param(
        [Parameter(Mandatory = $true)][object[]]$Actual,
        [Parameter(Mandatory = $true)][object[]]$Expected,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $actualSet = @($Actual | ForEach-Object { [string]$_ } | Sort-Object)
    $expectedSet = @($Expected | ForEach-Object { [string]$_ } | Sort-Object)
    if (($actualSet -join "|") -ne ($expectedSet -join "|")) {
        Write-Error "$Label mismatch. actual=[$($actualSet -join ', ')] expected=[$($expectedSet -join ', ')]"
    }
}

function Assert-StringArrayContains {
    param(
        [Parameter(Mandatory = $true)][object[]]$Values,
        [Parameter(Mandatory = $true)][string]$Expected,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if (@($Values | Where-Object { [string]$_ -eq $Expected }).Count -eq 0) {
        Write-Error "$Label missing required entry: $Expected"
    }
}

function Assert-AgentSurfaceJsonContract {
    $fragmentPath = Join-Path $root "engine/agent/manifest.fragments/011-aiSurfaces.json"
    $fragment = Get-Content -LiteralPath $fragmentPath -Raw | ConvertFrom-Json
    Assert-Properties $fragment @("aiSurfaces") "engine/agent/manifest.fragments/011-aiSurfaces.json"

    $aiSurfaces = $fragment.aiSurfaces
    Assert-Properties $aiSurfaces @("codex", "claudeCode", "cursor", "crossToolAlignment") "aiSurfaces"

    foreach ($surfaceName in @("codex", "claudeCode", "cursor")) {
        $surface = $aiSurfaces.$surfaceName
        Assert-Properties $surface @("instructions", "skills", "agents", "requiredSkills", "requiredAgents", "readOnlyAgents") "aiSurfaces.$surfaceName"
        Assert-UniqueStringArray @($surface.requiredSkills) "aiSurfaces.$surfaceName.requiredSkills"
        Assert-UniqueStringArray @($surface.requiredAgents) "aiSurfaces.$surfaceName.requiredAgents"
        Assert-UniqueStringArray @($surface.readOnlyAgents) "aiSurfaces.$surfaceName.readOnlyAgents"
        Assert-StringSetEquals @($surface.readOnlyAgents) @($aiSurfaces.crossToolAlignment.requiredReadOnlyRoles) "aiSurfaces.$surfaceName.readOnlyAgents"
    }

    Assert-Properties $aiSurfaces.crossToolAlignment @(
        "schemaVersion",
        "capabilityId",
        "officialDocs",
        "toolSurfaces",
        "validationGuards",
        "requiredReadOnlyRoles",
        "forbiddenBroadGrants",
        "unsupportedClaims"
    ) "aiSurfaces.crossToolAlignment"

    Assert-UniqueStringArray @($aiSurfaces.crossToolAlignment.requiredReadOnlyRoles) "aiSurfaces.crossToolAlignment.requiredReadOnlyRoles"
    foreach ($officialDocId in @(
            "openai-codex-agents-md",
            "openai-codex-rules",
            "openai-codex-skills",
            "openai-codex-subagents",
            "openai-developer-docs-mcp",
            "anthropic-claude-code-settings",
            "anthropic-claude-code-subagents",
            "cursor-rules-agents-md",
            "cursor-skills",
            "cursor-subagents",
            "cursor-composer-2-5"
        )) {
        Assert-StringArrayContains @($aiSurfaces.crossToolAlignment.officialDocs | ForEach-Object { $_.id }) $officialDocId "aiSurfaces.crossToolAlignment.officialDocs.id"
    }

    foreach ($guard in @(
            "tools/check-agents.ps1",
            "tools/check-ai-integration.ps1",
            "tools/check-json-contracts.ps1",
            "tools/check-format.ps1",
            "tools/validate.ps1"
        )) {
        Assert-StringArrayContains @($aiSurfaces.crossToolAlignment.validationGuards) $guard "aiSurfaces.crossToolAlignment.validationGuards"
    }

    foreach ($surfaceName in @("codex", "claudeCode", "cursor")) {
        $toolSurface = @($aiSurfaces.crossToolAlignment.toolSurfaces | Where-Object { [string]$_.id -eq $surfaceName })
        if ($toolSurface.Count -ne 1) {
            Write-Error "aiSurfaces.crossToolAlignment.toolSurfaces must contain exactly one '$surfaceName' row"
        }
        Assert-StringSetEquals @($toolSurface[0].validationGuards) @(
            "tools/check-agents.ps1",
            "tools/check-ai-integration.ps1",
            "tools/check-json-contracts.ps1"
        ) "aiSurfaces.crossToolAlignment.toolSurfaces[$surfaceName].validationGuards"
    }
}

Assert-AgentSurfaceJsonContract

foreach ($check in @(
    @{
        Path = "engine/runtime/include/mirakana/runtime/resource_runtime.hpp"
        Needles = @(
            "RuntimeResidentPackageMountSetV2",
            "RuntimeResidentPackageMountIdV2",
            "RuntimeResidentPackageMountStatusV2",
            "RuntimeResidentPackageMountCatalogBuildResultV2",
            "build_runtime_resource_catalog_v2_from_resident_mount_set",
            "RuntimeResidentCatalogCacheV2",
            "RuntimeResidentCatalogCacheStatusV2",
            "RuntimeResidentPackageReplaceCommitStatusV2",
            "RuntimeResidentPackageReplaceCommitResultV2",
            "commit_runtime_resident_package_replace_v2",
            "RuntimeResidentPackageUnmountCommitStatusV2",
            "RuntimeResidentPackageUnmountCommitResultV2",
            "commit_runtime_resident_package_unmount_v2",
            "RuntimeResidentPackageEvictionPlanStatusV2",
            "RuntimeResidentPackageEvictionPlanDescV2",
            "RuntimeResidentPackageEvictionPlanResultV2",
            "plan_runtime_resident_package_evictions_v2",
            "RuntimeResidentPackageReviewedEvictionCommitStatusV2",
            "RuntimeResidentPackageReviewedEvictionCommitDescV2",
            "RuntimeResidentPackageReviewedEvictionCommitResultV2",
            "commit_runtime_resident_package_reviewed_evictions_v2",
            "RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2",
            "RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDescV2",
            "RuntimePackageDiscoveryResidentReplaceReviewedEvictionsResultV2",
            "commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2",
            "RuntimePackageHotReloadCandidateReviewStatusV2",
            "RuntimePackageHotReloadCandidateReviewDescV2",
            "RuntimePackageHotReloadCandidateReviewResultV2",
            "plan_runtime_package_hot_reload_candidate_review_v2",
            "RuntimePackageHotReloadRecookChangeReviewStatusV2",
            "RuntimePackageHotReloadRecookChangeReviewDescV2",
            "RuntimePackageHotReloadRecookChangeReviewResultV2",
            "plan_runtime_package_hot_reload_recook_change_review_v2",
            "RuntimePackageHotReloadReplacementIntentReviewStatusV2",
            "invalid_overlay",
            "RuntimePackageHotReloadReplacementIntentReviewDescV2",
            "RuntimePackageHotReloadReplacementIntentReviewResultV2",
            "plan_runtime_package_hot_reload_replacement_intent_review_v2",
            "RuntimePackageHotReloadRecookReplacementStatusV2",
            "RuntimePackageHotReloadRecookReplacementDescV2",
            "RuntimePackageHotReloadRecookReplacementResultV2",
            "commit_runtime_package_hot_reload_recook_replacement_v2"
        )
    },
    @{
        Path = "engine/runtime/src/resource_runtime.cpp"
        Needles = @(
            "RuntimeResidentPackageReplaceCommitResultV2::succeeded",
            "RuntimeResidentPackageEvictionPlanResultV2::succeeded",
            "RuntimeResidentPackageReviewedEvictionCommitResultV2::succeeded",
            "RuntimePackageDiscoveryResidentReplaceReviewedEvictionsResultV2::succeeded",
            "RuntimePackageHotReloadCandidateReviewResultV2::succeeded",
            "RuntimePackageHotReloadRecookChangeReviewResultV2::succeeded",
            "RuntimePackageHotReloadReplacementIntentReviewResultV2::succeeded",
            "RuntimePackageHotReloadRecookReplacementResultV2::succeeded",
            "plan_runtime_package_hot_reload_candidate_review_v2",
            "plan_runtime_package_hot_reload_recook_change_review_v2",
            "plan_runtime_package_hot_reload_replacement_intent_review_v2",
            "commit_runtime_package_hot_reload_recook_replacement_v2",
            "map_hot_reload_recook_replacement_commit_diagnostic_phase",
            "RuntimePackageHotReloadRecookReplacementDiagnosticPhaseV2::candidate_load",
            "RuntimePackageHotReloadRecookReplacementDiagnosticPhaseV2::resident_budget",
            "replacement-commit-failed",
            "invalid-recook-apply-result-asset",
            "invalid-recook-apply-result-revision",
            "commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2",
            "invalid-selected-candidate",
            "candidate-outside-discovery-root",
            "candidate-content-root-mismatch",
            "invalid-overlay",
            "protected-eviction-candidate-mount-id",
            "map_reviewed_evictions_replace_diagnostic_phase",
            "add_reviewed_evictions_replace_commit_diagnostics",
            "map_reviewed_eviction_commit_plan_status",
            "contains_mount_id",
            "protected-candidate-mount-id",
            "budget_unreachable",
            "projected_mount_set.unmount",
            "RuntimeResidentPackageMountSetReplaceAccessV2",
            "invoked_candidate_catalog_build",
            "RuntimeResidentPackageMountSetV2 projected_mount_set = mount_set",
            "RuntimeResidentCatalogCacheV2 projected_catalog_cache = catalog_cache",
            "mount_set = std::move(projected_mount_set)",
            "catalog_cache = std::move(projected_catalog_cache)"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Resident package mount set",
            "RuntimeResidentPackageMountSetV2",
            "Resident catalog cache",
            "RuntimeResidentCatalogCacheV2",
            "Resident package streaming mount commit",
            "Resident package streaming replace commit",
            "execute_selected_runtime_package_streaming_resident_replace_safe_point",
            "Runtime Package Streaming Resident Unmount v1",
            "execute_selected_runtime_package_streaming_resident_unmount_safe_point",
            "Resident package eviction plan",
            "plan_runtime_resident_package_evictions_v2",
            "Resident package reviewed eviction commit",
            "commit_runtime_resident_package_reviewed_evictions_v2",
            "Runtime Package Hot Reload Reviewed Replacement v1",
            "host-driven reviewed hot-reload replacement safe point",
            "Runtime Package Hot Reload Candidate Review v1",
            "plan_runtime_package_hot_reload_candidate_review_v2",
            "Runtime Package Hot Reload Recook Change Review v1",
            "plan_runtime_package_hot_reload_recook_change_review_v2",
            "Runtime Package Hot Reload Replacement Intent Review v1",
            "plan_runtime_package_hot_reload_replacement_intent_review_v2",
            "RuntimePackageHotReloadReplacementIntentReviewResultV2",
            "Runtime Package Hot Reload Recook Replacement Safe Point v1",
            "commit_runtime_package_hot_reload_recook_replacement_v2",
            "RuntimePackageHotReloadRecookReplacementResultV2",
            "Runtime Hot Reload Recook Package Replacement Execution v1",
            "execute_asset_runtime_package_hot_reload_replacement_safe_point",
            "Runtime Hot Reload Registered Asset Watch Tick v1",
            "execute_asset_runtime_package_hot_reload_registered_asset_watch_tick_safe_point",
            "AssetHotReloadRecookScheduler",
            "ready scheduler rows retryable",
            "selected package matched",
            "invalid overlay modes",
            "Resident package replacement commit",
            "commit_runtime_resident_package_replace_v2",
            "disk/VFS mount discovery"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "Runtime Resource Resident Package Replacement Commit v1 coverage",
            "Runtime Package Streaming Resident Replace v1 coverage",
            "Runtime Package Streaming Resident Unmount v1 coverage",
            "Runtime Resident Package Eviction Plan v1 coverage",
            "Runtime Resident Package Reviewed Eviction Commit v1 coverage",
            "MK_runtime_resource_resident_replace_tests",
            "MK_runtime_package_streaming_resident_mount_tests",
            "commit_runtime_resident_package_replace_v2",
            "execute_selected_runtime_package_streaming_resident_unmount_safe_point",
            "plan_runtime_resident_package_evictions_v2",
            "commit_runtime_resident_package_reviewed_evictions_v2",
            "Runtime Package Hot Reload Reviewed Replacement v1 coverage",
            "commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2",
            "Runtime Package Hot Reload Candidate Review v1 coverage",
            "MK_runtime_package_hot_reload_candidate_review_tests",
            "plan_runtime_package_hot_reload_candidate_review_v2",
            "Runtime Package Hot Reload Recook Change Review v1 coverage",
            "MK_runtime_package_hot_reload_recook_change_review_tests",
            "plan_runtime_package_hot_reload_recook_change_review_v2",
            "Runtime Package Hot Reload Replacement Intent Review v1 coverage",
            "MK_runtime_package_hot_reload_replacement_intent_review_tests",
            "plan_runtime_package_hot_reload_replacement_intent_review_v2",
            "Runtime Package Hot Reload Recook Replacement Safe Point v1 coverage",
            "MK_runtime_package_hot_reload_recook_replacement_tests",
            "commit_runtime_package_hot_reload_recook_replacement_v2",
            "Runtime Hot Reload Recook Package Replacement Execution v1 coverage",
            "MK_tools_runtime_hot_reload_package_tests",
            "execute_asset_runtime_package_hot_reload_replacement_safe_point",
            "Runtime Hot Reload Registered Asset Watch Tick v1 coverage",
            "execute_asset_runtime_package_hot_reload_registered_asset_watch_tick_safe_point",
            "dependency invalidated recook requests",
            "preserves ready scheduler rows for retry",
            "recook failures plus scan exceptions before runtime package reads",
            "scan exceptions before runtime package reads",
            "selected package commit isolation",
            "candidate/discovery root mismatches",
            "invalid overlay modes"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
        Needles = @(
            "Runtime Resource v2 1.0 Scope Closeout v1",
            "registered asset watch-tick orchestration",
            "renderer-rhi-resource-foundation"
        )
    },
    @{
        Path = "tests/unit/runtime_package_hot_reload_replacement_intent_review_tests.cpp"
        Needles = @(
            "runtime package hot reload replacement intent review builds safe point descriptor",
            "runtime package hot reload replacement intent review rejects invalid candidate rows",
            "runtime package hot reload replacement intent review rejects invalid and missing mount ids",
            "runtime package hot reload replacement intent review rejects unsafe discovery descriptors",
            "runtime package hot reload replacement intent review rejects invalid overlay intent",
            "runtime package hot reload replacement intent review validates reviewed eviction candidates"
        )
    },
    @{
        Path = "tests/unit/runtime_package_hot_reload_candidate_review_tests.cpp"
        Needles = @(
            "runtime package hot reload candidate review maps package index paths in stable order",
            "runtime package hot reload candidate review maps changed payload paths under candidate content roots",
            "runtime package hot reload candidate review rejects invalid and reports unmatched changed paths",
            "runtime package hot reload candidate review deduplicates repeated matches for one candidate",
            "runtime package hot reload candidate review ignores invalid candidate rows before review",
            "runtime package hot reload candidate review returns typed no-match statuses without package reads"
        )
    },
    @{
        Path = "tests/unit/runtime_package_hot_reload_recook_change_review_tests.cpp"
        Needles = @(
            "runtime package hot reload recook change review maps staged and applied recook outputs",
            "runtime package hot reload recook change review blocks failed recook rows before candidate review",
            "runtime package hot reload recook change review rejects invalid recook rows before candidate review",
            "static_cast<mirakana::AssetHotReloadApplyResultKind>(255)",
            "runtime package hot reload recook change review reports no recook rows without reading packages",
            "runtime package hot reload recook change review surfaces candidate review failures"
        )
    },
    @{
        Path = "tests/unit/runtime_package_hot_reload_recook_replacement_tests.cpp"
        Needles = @(
            "runtime package hot reload recook replacement commits reviewed selected package at safe point",
            "runtime package hot reload recook replacement blocks failed recook rows before package reads",
            "runtime package hot reload recook replacement requires selected reviewed candidate",
            "runtime package hot reload recook replacement blocks invalid intent before discovery",
            "runtime package hot reload recook replacement preserves state when commit stage fails",
            "runtime package hot reload recook replacement preserves state when reviewed evictions are insufficient"
        )
    },
    @{
        Path = "tests/unit/tools_runtime_hot_reload_package_tests.cpp"
        Needles = @(
            "asset runtime package hot reload replacement commits recook and resident safe point",
            "asset runtime package hot reload replacement commits only the selected package recook assets",
            "asset runtime package hot reload replacement passes external import options into recook",
            "asset runtime package hot reload replacement blocks recook failure before runtime package reads",
            "asset runtime package hot reload replacement reports recook descriptor exceptions",
            "asset runtime package hot reload replacement preserves staged recook when runtime commit fails",
            "asset runtime package registered watch tick primes without recook or native watcher",
            "asset runtime package registered watch tick debounces and commits reviewed runtime package replacement",
            "asset runtime package registered watch tick forwards dependency invalidated recook requests",
            "asset runtime package registered watch tick reports recook failures before runtime package reads",
            "asset runtime package registered watch tick reports scan exceptions before runtime package reads"
        )
    },
    @{
        Path = "tests/unit/runtime_package_discovery_resident_replace_reviewed_evictions_tests.cpp"
        Needles = @(
            "runtime package discovery resident replace with reviewed evictions commits selected candidate after eviction",
            "runtime package discovery resident replace with reviewed evictions rejects descriptors before scans",
            "runtime package discovery resident replace with reviewed evictions reports missing candidate before package ",
            "runtime package discovery resident replace with reviewed evictions preserves state on delegated load failure",
            "runtime package discovery resident replace with reviewed evictions maps reviewed eviction failures",
            "runtime package discovery resident replace with reviewed evictions preserves state when candidates are "
        )
    },
    @{
        Path = "tests/unit/runtime_resource_resident_unmount_tests.cpp"
        Needles = @(
            "runtime resident package eviction plan is no op when current view is within budget",
            "runtime resident package eviction plan returns reviewed candidate order until budget passes",
            "runtime resident package eviction plan rejects protected candidates before partial planning",
            "runtime resident package eviction plan rejects duplicate and missing candidates before partial planning",
            "runtime resident package eviction plan reports unreachable budget without mutating mounts",
            "runtime resident package reviewed eviction commit applies reviewed candidates atomically",
            "runtime resident package reviewed eviction commit succeeds as no op when current view fits",
            "runtime resident package reviewed eviction commit rejects reviewed candidates before mutation",
            "runtime resident package reviewed eviction commit preserves state when candidates are insufficient"
        )
    },
    @{
        Path = "tests/unit/runtime_resource_resident_replace_tests.cpp"
        Needles = @(
            "runtime resident package replacement commit preserves mount slot and refreshes catalog cache",
            "runtime resident package replacement commit rejects invalid and missing ids before mutation",
            "runtime resident package replacement commit rejects duplicate candidate records before mutation",
            "runtime resident package replacement commit preserves state on projected budget failure"
        )
    },
    @{
        Path = "tests/unit/runtime_resource_resident_cache_tests.cpp"
        Needles = @(
            "runtime resident catalog cache reuses catalog for unchanged mount generation and budget",
            "runtime resident catalog cache rebuilds when mount set generation changes",
            "runtime resident catalog cache rejects budget changes without replacing cached catalog"
        )
    },
    @{
        Path = "engine/runtime/include/mirakana/runtime/package_streaming.hpp"
        Needles = @(
            "resident_mount_failed",
            "resident_replace_failed",
            "resident_unmount_failed",
            "resident_catalog_refresh_failed",
            "resident_eviction_plan_failed",
            "RuntimeResidentCatalogCacheV2& catalog_cache",
            "RuntimeResidentPackageMountIdV2 mount_id",
            "RuntimeResidentPackageReplaceCommitResultV2 resident_replace",
            "RuntimeResidentPackageUnmountCommitResultV2 resident_unmount",
            "execute_selected_runtime_package_streaming_candidate_resident_mount_with_reviewed_evictions_safe_point",
            "execute_selected_runtime_package_streaming_resident_replace_safe_point",
            "execute_selected_runtime_package_streaming_resident_unmount_safe_point",
            "RuntimeResidentPackageEvictionPlanResultV2 eviction_plan",
            "resident_catalog_refresh"
        )
    },
    @{
        Path = "engine/runtime/src/package_streaming.cpp"
        Needles = @(
            "project_resident_packages",
            "evaluate_projected_resident_budget",
            "validate_loaded_package_catalog_before_mount",
            "validate_resident_replace_mount_id",
            "validate_projected_resident_catalog_hints",
            "commit_runtime_resident_package_replace_v2",
            "commit_runtime_resident_package_unmount_v2",
            "map_reviewed_evictions_mount_status",
            "map_reviewed_evictions_replace_status",
            "commit_runtime_package_candidate_resident_mount_with_reviewed_evictions_v2",
            "commit_runtime_package_candidate_resident_replace_with_reviewed_evictions_v2",
            "execute_selected_runtime_package_streaming_candidate_resident_replace_with_reviewed_evictions_safe_point",
            "resident_unmount_failed",
            "resident_catalog_refresh_failed",
            "resident_eviction_plan_failed",
            "mount_set.unmount(mount_id)"
        )
    },
    @{
        Path = "tests/unit/runtime_package_streaming_resident_mount_tests.cpp"
        Needles = @(
            "runtime package streaming resident mount commit mounts package and refreshes resident catalog",
            "runtime package streaming resident mount commit rejects duplicate mount id before mutation",
            "runtime package streaming resident mount commit rejects duplicate records before mutation",
            "runtime package streaming resident mount commit preserves catalog on projected budget failure",
            "runtime package streaming resident replace commit replaces mounted package and refreshes resident catalog",
            "runtime package streaming resident replace commit rejects invalid and missing ids before mutation",
            "runtime package streaming resident replace commit preserves state on candidate catalog failure",
            "runtime package streaming resident replace commit preserves state on projected budget failure",
            "runtime package streaming candidate resident mount with reviewed evictions commits after eviction",
            "runtime package streaming candidate resident mount with reviewed evictions rejects reviewed eviction ",
            "runtime package streaming candidate resident replace with reviewed evictions commits after eviction",
            "runtime package streaming candidate resident replace with reviewed evictions rejects reviewed eviction ",
            "runtime package streaming candidate resident replace with reviewed evictions preserves state when candidates ",
            "runtime package streaming resident unmount commit removes mounted package and refreshes resident catalog",
            "runtime package streaming resident unmount commit rejects invalid and missing ids before mutation",
            "runtime package streaming resident unmount commit preserves state on projected residency hint failure"
        )
    }
)) {
    $checkPath = Join-Path $root $check.Path
    if (-not (Test-Path -LiteralPath $checkPath)) {
        Write-Error "Missing runtime resource resident package mount set evidence file: $($check.Path)"
    }
    $fileText = Get-JsonContractSurfaceText $check.Path
    foreach ($needle in $check.Needles) {
        Assert-ContainsText $fileText $needle "$($check.Path) runtime resource resident package mount set evidence"
    }
}
$rendererRhiGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "renderer-rhi-resource-foundation" })
if ($rendererRhiGap.Count -ne 0) {
    Write-Error "engine manifest aiOperableProductionLoop renderer-rhi-resource-foundation gap must leave unsupportedProductionGaps after 1.0 scope closeout"
}
foreach ($check in @(
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "Renderer RHI Resource Foundation 1.0 Scope Closeout",
            "D3D12/Vulkan deferred native teardown",
            "RhiDeviceMemoryDiagnostics",
            "frame-graph-v1",
            "upload-staging-v1",
            "Metal IRhiDevice parity"
        )
    }
)) {
    $fileText = Get-JsonContractSurfaceText $check.Path
    foreach ($needle in $check.Needles) {
        Assert-ContainsText $fileText $needle "$($check.Path) renderer-rhi-resource-foundation closeout evidence"
    }
}
$frameGraphGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "frame-graph-v1" })
if ($frameGraphGap.Count -ne 0) {
    Write-Error "engine manifest aiOperableProductionLoop frame-graph-v1 gap must leave unsupportedProductionGaps after 1.0 scope closeout"
}
foreach ($check in @(
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "Frame Graph v1 1.0 Scope Closeout",
            "upload-staging-v1",
            "FrameGraphRhiMultiQueuePackageEvidence",
            "broad production render graph scheduling",
            "Metal memory alias allocation"
        )
    }
)) {
    $fileText = Get-JsonContractSurfaceText $check.Path
    foreach ($needle in $check.Needles) {
        Assert-ContainsText $fileText $needle "$($check.Path) frame-graph-v1 closeout evidence"
    }
}
$uploadStagingGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "upload-staging-v1" })
if ($uploadStagingGap.Count -ne 0) {
    Write-Error "engine manifest aiOperableProductionLoop upload-staging-v1 gap must leave unsupportedProductionGaps after async-ready resource update closeout"
}
foreach ($check in @(
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "Upload Staging v1 Async-Ready Resource Updates",
            "make_runtime_package_resource_update_readiness",
            "RuntimePackageResourceUpdateReadinessResult",
            "package_upload_staging_resource_updates_ready",
            "broad/background streaming"
        )
    }
)) {
    $fileText = Get-JsonContractSurfaceText $check.Path
    foreach ($needle in $check.Needles) {
        Assert-ContainsText $fileText $needle "$($check.Path) upload-staging-v1 closeout evidence"
    }
}
$playable3dGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "3d-playable-vertical-slice" })
if ($playable3dGap.Count -ne 0) {
    Write-Error "engine manifest aiOperableProductionLoop 3d-playable-vertical-slice gap must leave unsupportedProductionGaps after 1.0 closeout"
}
$recommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "
foreach ($needle in @(
    "3d-playable-vertical-slice",
    "generated desktop 3D package proof",
    "host-gated D3D12/Vulkan package smokes",
    "visible 3D aggregate counters",
    "native UI overlay/atlas package counters"
)) {
    Assert-ContainsText $recommendedText $needle "engine manifest aiOperableProductionLoop recommendedNextPlan 3d closeout"
}
$physicsCollisionGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "physics-1-0-collision-system" })
if ($physicsCollisionGap.Count -ne 0) {
    Write-Error "engine manifest aiOperableProductionLoop physics-1-0-collision-system gap must leave unsupportedProductionGaps after Physics 1.0 closeout"
}
foreach ($needle in @(
    "Frame Graph Transient Texture Alias Planning v1",
    "FrameGraphTransientTextureAliasPlan",
    "plan_frame_graph_transient_texture_aliases",
    "Frame Graph Backend-Neutral Distinct Alias-Group Lease Binding v1",
    "FrameGraphTransientTextureLeaseBindingResult",
    "IRhiDevice::acquire_transient_texture_alias_group",
    "acquire_frame_graph_transient_texture_lease_bindings",
    "Frame Graph Shared Texture State Handoff v1",
    "conflicting initial shared-handle states",
    "Frame Graph RHI Pass Target Access Validation v1",
    "FrameGraphTexturePassTargetAccess",
    "build_frame_graph_texture_pass_target_accesses",
    "RhiPostprocessFrameRenderer scene pass command recording",
    "Frame Graph Shadow Scratch Color Target-State Ownership v1",
    "shadow_color",
    "6 pass callbacks/15 barrier steps",
    "Frame Graph Viewport Surface Color State Executor v1",
    "RhiViewportSurface",
    "viewport_color",
    "Frame Graph Texture Aliasing Barrier Command v1",
    "Frame Graph D3D12 Texture Aliasing Barrier Evidence v1",
    "null_resource_aliasing_barriers",
    "record_frame_graph_texture_aliasing_barriers",
    "Frame Graph Automatic Aliasing Barrier Insertion v1",
    "Package Streaming Frame Graph Texture Binding Handoff v1",
    "make_runtime_package_streaming_frame_graph_texture_bindings",
    "Runtime Package Streaming RHI Upload Binding Transaction v1",
    "upload_runtime_package_streaming_frame_graph_texture_bindings",
    "Package Static Mesh Upload Binding Transaction v1",
    "upload_runtime_package_streaming_mesh_gpu_bindings",
    "Runtime Upload Queue Wait v1",
    "wait_for_runtime_uploads_on_queue",
    "upload_queue_waits_recorded",
    "Frame Graph Render Pass Envelope v1",
    "render_passes_recorded",
    "Frame Graph RHI Queue Dependency Plan v1",
    "plan_frame_graph_rhi_queue_waits",
    "IRhiDevice::wait_for_queue",
    "Frame Graph RHI Multi-Queue Executor v1",
    "execute_frame_graph_rhi_multi_queue_schedule",
    "Frame Graph RHI Multi-Queue Texture Barrier Execution v1",
    "FrameGraphRhiMultiQueueExecutionResult::barriers_recorded",
    "Frame Graph Remaining Render Pass Envelopes v1",
    "Frame Graph v1 1.0 Scope Closeout v1 closes frame-graph-v1",
    "upload-staging-v1",
    "scene-component-prefab-schema-v2",
    "refresh-prefab-instance",
    "2d-playable-vertical-slice",
    "3d-playable-vertical-slice"
)) {
    if (-not $recommendedText.Contains($needle)) {
        Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe frame-graph closeout and upload-staging next gap: $needle"
    }
}
$editorProductizationGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "editor-productization" })
if ($editorProductizationGap.Count -ne 0) {
    Write-Error "engine manifest aiOperableProductionLoop editor-productization gap must leave unsupportedProductionGaps after 1.0 closeout"
}
$editorProductizationCloseoutText = Get-Content -Raw "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
foreach ($needle in @(
    "Editor Productization 1.0 Host-Gated Exclusion Closeout",
    "reviewed editor authoring/playtest/AI command/resource/input/prefab/material-preview evidence",
    "Vulkan/Metal material-preview display parity",
    "explicit 1.0 exclusion",
    "production-ui-importer-platform-adapters",
    "full-repository-quality-gate"
)) {
    if (-not $editorProductizationCloseoutText.Contains($needle)) {
        Write-Error "editor-productization closeout evidence missing: $needle"
    }
}
$productionUiImporterPlatformGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "production-ui-importer-platform-adapters" })
if ($productionUiImporterPlatformGap.Count -ne 0) {
    Write-Error "engine manifest aiOperableProductionLoop production-ui-importer-platform-adapters gap must leave unsupportedProductionGaps after 1.0 closeout"
}
$productionUiCloseoutText = Get-Content -Raw "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
foreach ($needle in @(
    "Production UI Importer Platform Adapters 1.0 Closeout",
    "reviewed adapter-boundary and package evidence",
    "AccessibilityPublishPlan",
    "ImeCompositionPublishPlan",
    "PlatformTextInputSessionPlan",
    "TextShapingRequestPlan",
    "FontRasterizationRequestPlan",
    "ImageDecodeRequestPlan",
    "PngImageDecodingAdapter",
    "author_packed_ui_atlas_from_decoded_images",
    "author_packed_ui_glyph_atlas_from_rasterized_glyphs",
    "selected SDL3 platform bridges",
    "package-visible native UI overlay/atlas smokes",
    "production text shaping implementation",
    "real font loading/rasterization",
    "OS accessibility publication",
    "broad native IME/text services",
    "broader source codecs",
    "SVG/vector parsing",
    "renderer texture-upload APIs",
    "arbitrary importer adapters",
    "UI middleware",
    "full-repository-quality-gate"
)) {
    if (-not $productionUiCloseoutText.Contains($needle)) {
        Write-Error "production-ui closeout evidence missing: $needle"
    }
}
$fullRepoQualityGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "full-repository-quality-gate" })
if ($fullRepoQualityGap.Count -ne 0) {
    Write-Error "engine manifest aiOperableProductionLoop full-repository-quality-gate gap must leave unsupportedProductionGaps after 1.0 closeout"
}
$fullRepoQualityCloseoutText = Get-Content -Raw "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
foreach ($needle in @(
    "Full Repository Quality Gate 1.0 Closeout",
    "local full validate",
    "CI Matrix Contract Check v1",
    "Full Repository Static Analysis CI Contract v1",
    "Linux coverage threshold policy",
    "sanitizer lane documentation",
    "Windows release package artifact evidence",
    "broader analyzer profile expansion",
    "full cross-platform package execution evidence",
    "signing",
    "notarization",
    "release distribution",
    "unsupported_gaps=0"
)) {
    if (-not $fullRepoQualityCloseoutText.Contains($needle)) {
        Write-Error "full repository quality closeout evidence missing: $needle"
    }
}

$tidyWrapperContent = Get-Content -LiteralPath (Join-Path $root "tools/check-tidy.ps1") -Raw
if (-not $tidyWrapperContent.Contains('[string[]]$Files')) {
    Write-Error "tools/check-tidy.ps1 must expose a targeted -Files lane for changed-file clang-tidy validation"
}
$testingContent = Get-Content -LiteralPath (Join-Path $root "docs/testing.md") -Raw
if (-not $testingContent.Contains("pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/physics/src/physics3d.cpp")) {
    Write-Error "docs/testing.md must document the targeted tidy -Files lane"
}

$ciMatrixCheckText = Get-Content -LiteralPath (Join-Path $root "tools/check-ci-matrix.ps1") -Raw
$classifierScriptText = Get-Content -LiteralPath (Join-Path $root "tools/classify-pr-validation-tier.ps1") -Raw
$validateWorkflowText = Get-Content -LiteralPath (Join-Path $root ".github/workflows/validate.yml") -Raw
$validateScriptText = Get-Content -LiteralPath (Join-Path $root "tools/validate.ps1") -Raw
$commonScriptText = Get-Content -LiteralPath (Join-Path $root "tools/common.ps1") -Raw
$mobilePackagingScriptText = Get-Content -LiteralPath (Join-Path $root "tools/check-mobile-packaging.ps1") -Raw
$androidReleasePackageScriptText = Get-Content -LiteralPath (Join-Path $root "tools/check-android-release-package.ps1") -Raw
if (-not $validateScriptText.Contains("check-ci-matrix.ps1")) {
    Write-Error "tools/validate.ps1 must run check-ci-matrix.ps1"
}
Assert-ContainsText $commonScriptText "function Join-OptionalPath" "tools/common.ps1"
Assert-ContainsText $commonScriptText "function Get-LocalApplicationDataRoot" "tools/common.ps1"
foreach ($commonHelperNeedle in @(
        "function Read-Json",
        "function ConvertTo-LfText",
        "function ConvertTo-RepoPath",
        "function Get-RelativeRepoPath",
        "function Get-FirstExistingFile",
        "function Get-WindowsSdkDxcCandidates",
        "function Reset-RepoTmpDirectory",
        "function Test-TextStartsWithLine"
    )) {
    Assert-ContainsText $commonScriptText $commonHelperNeedle "tools/common.ps1"
}
foreach ($staticContractLedger in Get-StaticContractLedger) {
    foreach ($ledgerPath in @($staticContractLedger.EntryScript) + @(Get-StaticContractLedgerRepoPaths -Ledger $staticContractLedger)) {
        $fullLedgerPath = Join-Path $root $ledgerPath
        if (-not (Test-Path -LiteralPath $fullLedgerPath -PathType Leaf)) {
            Write-Error "$ledgerPath must exist so static contract ledgers stay chapterized instead of growing monolithic entry scripts"
        }
    }

    $entryText = Get-Content -LiteralPath (Join-Path $root $staticContractLedger.EntryScript) -Raw
    $entryLineCount = (Get-Content -LiteralPath (Join-Path $root $staticContractLedger.EntryScript)).Count
    if ($entryLineCount -gt $staticContractLedger.MaximumEntryLines) {
        Write-Error "$($staticContractLedger.EntryScript) must stay a thin dispatcher; found $entryLineCount lines"
    }
    Assert-ContainsText $entryText 'static-contract-ledger.ps1' $staticContractLedger.EntryScript

    $coreText = Get-Content -LiteralPath (Join-Path $root $staticContractLedger.CoreScript) -Raw
    $coreLineCount = (Get-Content -LiteralPath (Join-Path $root $staticContractLedger.CoreScript)).Count
    if ($coreLineCount -gt $staticContractLedger.MaximumCoreLines) {
        Write-Error "$($staticContractLedger.CoreScript) must keep reusable helpers bounded; found $coreLineCount lines"
    }

    foreach ($sectionFile in $staticContractLedger.SectionFiles) {
        Assert-DoesNotContainText $entryText $sectionFile $staticContractLedger.EntryScript
        Assert-DoesNotContainText $coreText $sectionFile $staticContractLedger.CoreScript

        $sectionPath = "tools/$sectionFile"
        $sectionLineCount = (Get-Content -LiteralPath (Join-Path $root $sectionPath)).Count
        if ($sectionLineCount -gt $staticContractLedger.MaximumSectionLines) {
            Write-Error "$sectionPath must stay below $($staticContractLedger.MaximumSectionLines) lines; found $sectionLineCount lines"
        }
    }
}
$generatedGamesContractText = Get-Content -LiteralPath (Join-Path $root "tools/check-json-contracts-050-generated-games.ps1") -Raw
Assert-ContainsText $generatedGamesContractText '$historicalVerdictArchiveText = Get-JsonContractSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"' "tools/check-json-contracts-050-generated-games.ps1"
Assert-DoesNotContainText $generatedGamesContractText 'Get-Content -LiteralPath (Join-Path $root "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md") -Raw' "tools/check-json-contracts-050-generated-games.ps1"
foreach ($forbiddenNeedle in @(
    'Join-Path $env:LOCALAPPDATA',
    'Join-Path $env:ProgramFiles'
)) {
    Assert-DoesNotContainText $commonScriptText $forbiddenNeedle "tools/common.ps1"
    Assert-DoesNotContainText $mobilePackagingScriptText $forbiddenNeedle "tools/check-mobile-packaging.ps1"
    Assert-DoesNotContainText $androidReleasePackageScriptText $forbiddenNeedle "tools/check-android-release-package.ps1"
}
foreach ($needle in @(
    ".github/workflows/validate.yml",
    ".github/workflows/ios-validate.yml",
    "tools/classify-pr-validation-tier.ps1",
    "Assert-ValidationTierSelection",
    "docs-only PR",
    "static policy PR",
    "runtime PR",
    "workflow PR",
    "non-PR run",
    "windows-packages",
    "linux-coverage",
    "linux-sanitizer-test-logs",
    "static-analysis-tidy-logs",
    "macos-test-logs",
    "ios-simulator-build",
    "contents: read",
    "cancel-in-progress"
)) {
    if (-not $ciMatrixCheckText.Contains($needle)) {
        Write-Error "tools/check-ci-matrix.ps1 missing required CI matrix contract text: $needle"
    }
}
foreach ($needle in @(
    "Test-CiWorkflowPath",
    "Test-RuntimeOrBuildPath",
    "Test-StaticPolicyPath",
    "tools/classify-pr-validation-tier.ps1",
    "tools/check-tidy.ps1",
    "GitHubOutputPath"
)) {
    if (-not $classifierScriptText.Contains($needle)) {
        Write-Error "tools/classify-pr-validation-tier.ps1 missing required PR validation tier classifier text: $needle"
    }
}
foreach ($needle in @(
    "static-analysis:",
    "name: Full Repository Static Analysis",
    "runs-on: ubuntu-latest",
    "Verify preinstalled toolchain and install ccache",
    'command -v clang >/dev/null 2>&1 || { echo "clang is required on ubuntu-latest runner images"; exit 1; }',
    'command -v clang-tidy >/dev/null 2>&1 || { echo "clang-tidy is required on ubuntu-latest runner images"; exit 1; }',
    'command -v ninja >/dev/null 2>&1 || { echo "ninja is required on ubuntu-latest runner images"; exit 1; }',
    "sudo apt-get install -y --no-install-recommends --no-install-suggests ccache",
    "./tools/check-tidy.ps1 -Strict -Preset ci-linux-tidy",
    "-Jobs 0",
    "static-analysis-tidy-logs"
)) {
    if (-not $validateWorkflowText.Contains($needle)) {
        Write-Error ".github/workflows/validate.yml missing required static-analysis lane text: $needle"
    }
}
$fullRepoStaticAnalysisPlanText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md") -Raw
foreach ($needle in @(
    "Full Repository Static Analysis CI Contract v1 Implementation Plan",
    "**Status:** Completed.",
    "static-analysis",
    "tools/check-tidy.ps1 -Strict",
    "static-analysis-tidy-logs",
    "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1",
    "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1"
)) {
    if (-not $fullRepoStaticAnalysisPlanText.Contains($needle)) {
        Write-Error "Full Repository Static Analysis CI Contract v1 plan missing completion evidence: $needle"
    }
}

$runtimeUiAccessibilityPlanText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md") -Raw
foreach ($needle in @(
    "Runtime UI Accessibility Publish Plan v1",
    "**Status:** Completed",
    "AccessibilityPublishPlan",
    "AccessibilityPublishResult",
    "plan_accessibility_publish",
    "publish_accessibility_payload",
    "IAccessibilityAdapter",
    "OS accessibility adapter",
    "MK_ui_renderer_tests"
)) {
    if (-not $runtimeUiAccessibilityPlanText.Contains($needle)) {
        Write-Error "Runtime UI accessibility publish plan missing required evidence: $needle"
    }
}
$runtimeUiImePlanText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md") -Raw
foreach ($needle in @(
    "Runtime UI IME Composition Publish Plan v1",
    "**Status:** Completed",
    "ImeCompositionPublishPlan",
    "ImeCompositionPublishResult",
    "plan_ime_composition_update",
    "publish_ime_composition",
    "IImeAdapter",
    "Win32/TSF",
    "MK_ui_renderer_tests"
)) {
    if (-not $runtimeUiImePlanText.Contains($needle)) {
        Write-Error "Runtime UI IME composition publish plan missing required evidence: $needle"
    }
}
$runtimeUiPlatformTextInputSessionPlanText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md") -Raw
foreach ($needle in @(
    "Runtime UI Platform Text Input Session Plan v1",
    "**Status:** Completed",
    "PlatformTextInputSessionPlan",
    "PlatformTextInputSessionResult",
    "PlatformTextInputEndPlan",
    "PlatformTextInputEndResult",
    "begin_platform_text_input",
    "end_platform_text_input",
    "IPlatformIntegrationAdapter",
    "native text-input object/session ownership",
    "MK_ui_renderer_tests"
)) {
    if (-not $runtimeUiPlatformTextInputSessionPlanText.Contains($needle)) {
        Write-Error "Runtime UI platform text input session plan missing required evidence: $needle"
    }
}
$runtimeUiFontRasterizationRequestPlanText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md") -Raw
foreach ($needle in @(
    "Runtime UI Font Rasterization Request Plan v1",
    "**Status:** Completed",
    "FontRasterizationRequestPlan",
    "FontRasterizationResult",
    "plan_font_rasterization_request",
    "rasterize_font_glyph",
    "IFontRasterizerAdapter",
    "invalid_font_allocation",
    "MK_ui_renderer_tests"
)) {
    if (-not $runtimeUiFontRasterizationRequestPlanText.Contains($needle)) {
        Write-Error "Runtime UI font rasterization request plan missing required evidence: $needle"
    }
}
$runtimeUiTextShapingRequestPlanText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md") -Raw
foreach ($needle in @(
    "Runtime UI Text Shaping Request Plan v1",
    "**Status:** Completed",
    "TextShapingRequestPlan",
    "TextShapingResult",
    "plan_text_shaping_request",
    "shape_text_run",
    "ITextShapingAdapter",
    "invalid_text_shaping_result",
    "MK_ui_renderer_tests"
)) {
    if (-not $runtimeUiTextShapingRequestPlanText.Contains($needle)) {
        Write-Error "Runtime UI text shaping request plan missing required evidence: $needle"
    }
}
$runtimeUiImageDecodeRequestPlanText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md") -Raw
foreach ($needle in @(
    "Runtime UI Image Decode Request Plan v1",
    "**Status:** Completed",
    "ImageDecodeRequestPlan",
    "ImageDecodeDispatchResult",
    "ImageDecodePixelFormat",
    "plan_image_decode_request",
    "decode_image_request",
    "IImageDecodingAdapter",
    "invalid_image_decode_result",
    "MK_ui_renderer_tests"
)) {
    if (-not $runtimeUiImageDecodeRequestPlanText.Contains($needle)) {
        Write-Error "Runtime UI image decode request plan missing required evidence: $needle"
    }
}
$runtimeUiPngImageDecodingAdapterPlanText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md") -Raw
foreach ($needle in @(
    "Runtime UI PNG Image Decoding Adapter v1",
    "**Status:** Completed",
    "PngImageDecodingAdapter",
    "IImageDecodingAdapter",
    "decode_audited_png_rgba8",
    "ImageDecodePixelFormat::rgba8_unorm",
    "MK_tools_tests",
    "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1"
)) {
    if (-not $runtimeUiPngImageDecodingAdapterPlanText.Contains($needle)) {
        Write-Error "Runtime UI PNG image decoding adapter plan missing required evidence: $needle"
    }
}
$runtimeUiDecodedImageAtlasPackageBridgePlanText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md") -Raw
foreach ($needle in @(
    "Runtime UI Decoded Image Atlas Package Bridge v1",
    "**Status:** Completed",
    "PackedUiAtlasAuthoringDesc",
    "author_packed_ui_atlas_from_decoded_images",
    "plan_packed_ui_atlas_package_update",
    "apply_packed_ui_atlas_package_update",
    "pack_sprite_atlas_rgba8_max_side",
    "GameEngine.CookedTexture.v1",
    "GameEngine.UiAtlas.v1",
    "MK_tools_tests"
)) {
    if (-not $runtimeUiDecodedImageAtlasPackageBridgePlanText.Contains($needle)) {
        Write-Error "Runtime UI decoded image atlas package bridge plan missing required evidence: $needle"
    }
}
$runtimeUiGlyphAtlasPackageBridgePlanText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md") -Raw
foreach ($needle in @(
    "Runtime UI Glyph Atlas Package Bridge v1",
    "**Status:** Completed",
    "UiAtlasMetadataGlyph",
    "RuntimeUiAtlasGlyph",
    "PackedUiGlyphAtlasAuthoringDesc",
    "author_packed_ui_glyph_atlas_from_rasterized_glyphs",
    "plan_packed_ui_glyph_atlas_package_update",
    "apply_packed_ui_glyph_atlas_package_update",
    "build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas",
    "GameEngine.CookedTexture.v1",
    "GameEngine.UiAtlas.v1",
    "MK_tools_tests"
)) {
    if (-not $runtimeUiGlyphAtlasPackageBridgePlanText.Contains($needle)) {
        Write-Error "Runtime UI glyph atlas package bridge plan missing required evidence: $needle"
    }
}
$runtimeInputRebindingCapturePlanText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md") -Raw
foreach ($needle in @(
    "Runtime Input Rebinding Capture Contract v1",
    "**Status:** Completed",
    "RuntimeInputRebindingCaptureRequest",
    "RuntimeInputRebindingCaptureResult",
    "capture_runtime_input_rebinding_action",
    "MK_runtime_tests"
)) {
    if (-not $runtimeInputRebindingCapturePlanText.Contains($needle)) {
        Write-Error "Runtime input rebinding capture plan missing required evidence: $needle"
    }
}
$runtimeInputRebindingFocusConsumptionPlanText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md") -Raw
foreach ($needle in @(
    "Runtime Input Rebinding Focus Consumption v1",
    "**Status:** Completed",
    "RuntimeInputRebindingFocusCaptureRequest",
    "RuntimeInputRebindingFocusCaptureResult",
    "capture_runtime_input_rebinding_action_with_focus",
    "focus_retained",
    "gameplay_input_consumed",
    "MK_runtime_tests"
)) {
    if (-not $runtimeInputRebindingFocusConsumptionPlanText.Contains($needle)) {
        Write-Error "Runtime input rebinding focus consumption plan missing required evidence: $needle"
    }
}
$runtimeInputRebindingPresentationRowsPlanText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md") -Raw
foreach ($needle in @(
    "Runtime Input Rebinding Presentation Rows v1",
    "**Status:** Completed",
    "RuntimeInputRebindingPresentationToken",
    "RuntimeInputRebindingPresentationRow",
    "RuntimeInputRebindingPresentationModel",
    "present_runtime_input_action_trigger",
    "present_runtime_input_axis_source",
    "make_runtime_input_rebinding_presentation",
    "glyph_lookup_key",
    "model.diagnostics[0].path == row->id",
    "MK_runtime_tests"
)) {
    if (-not $runtimeInputRebindingPresentationRowsPlanText.Contains($needle)) {
        Write-Error "Runtime input rebinding presentation rows plan missing required evidence: $needle"
    }
}
$editorInputRebindingActionCapturePlanText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md") -Raw
foreach ($needle in @(
    "Editor Input Rebinding Action Capture Panel v1",
    "**Status:** Completed",
    "EditorInputRebindingCaptureModel",
    "make_editor_input_rebinding_capture_action_model",
    "make_input_rebinding_capture_action_ui_model",
    "input_rebinding.capture",
    "MK_editor_core_tests",
    "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1"
)) {
    if (-not $editorInputRebindingActionCapturePlanText.Contains($needle)) {
        Write-Error "Editor input rebinding action capture panel plan missing required evidence: $needle"
    }
}
$runtimeUiDocs = @{
    "docs/current-capabilities.md" = @("AccessibilityPublishPlan", "publish_accessibility_payload", "OS accessibility bridge publication", "ImeCompositionPublishPlan", "publish_ime_composition", "native IME/text-input adapter integration", "PlatformTextInputSessionPlan", "begin_platform_text_input", "IPlatformIntegrationAdapter", "virtual keyboard/session ownership", "TextShapingRequestPlan", "shape_text_run", "ITextShapingAdapter", "FontRasterizationRequestPlan", "rasterize_font_glyph", "adapter allocation diagnostics", "ImageDecodeRequestPlan", "decode_image_request", "ImageDecodePixelFormat::rgba8_unorm")
    "docs/ui.md" = @("AccessibilityPublishPlan", "IAccessibilityAdapter", "UI Automation", "NSAccessibility", "ImeCompositionPublishPlan", "IImeAdapter", "Win32/TSF", "PlatformTextInputSessionPlan", "IPlatformIntegrationAdapter", "virtual keyboard behavior", "TextShapingRequestPlan", "ITextShapingAdapter", "TextLayoutRun", "FontRasterizationRequestPlan", "IFontRasterizerAdapter", "GlyphAtlasAllocation", "ImageDecodeRequestPlan", "IImageDecodingAdapter", "ImageDecodePixelFormat::rgba8_unorm")
    "docs/roadmap.md" = @("Runtime UI Accessibility Publish Plan v1", "plan_accessibility_publish", "Runtime UI IME Composition Publish Plan v1", "plan_ime_composition_update", "Runtime UI Platform Text Input Session Plan v1", "begin_platform_text_input", "Runtime UI Text Shaping Request Plan v1", "plan_text_shaping_request", "Runtime UI Font Rasterization Request Plan v1", "plan_font_rasterization_request", "Runtime UI Image Decode Request Plan v1", "plan_image_decode_request", "source codecs", "platform SDK calls")
    "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md" = @("2026-05-08-runtime-ui-text-shaping-request-plan-v1.md", "TextShapingResult", "2026-05-07-runtime-ui-accessibility-publish-plan-v1.md", "AccessibilityPublishResult", "2026-05-07-runtime-ui-ime-composition-publish-plan-v1.md", "ImeCompositionPublishResult", "2026-05-08-runtime-ui-platform-text-input-session-plan-v1.md", "PlatformTextInputSessionResult", "2026-05-07-runtime-ui-font-rasterization-request-plan-v1.md", "FontRasterizationResult", "2026-05-08-runtime-ui-image-decode-request-plan-v1.md", "ImageDecodeDispatchResult", "source codecs", "third-party adapters unsupported")
    "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" = @("Runtime UI Accessibility Publish Plan v1", "IAccessibilityAdapter", "native accessibility objects", "Runtime UI IME Composition Publish Plan v1", "IImeAdapter", "native IME/text-input sessions", "Runtime UI Platform Text Input Session Plan v1", "IPlatformIntegrationAdapter", "virtual keyboard behavior", "Runtime UI Text Shaping Request Plan v1", "ITextShapingAdapter", "Runtime UI Font Rasterization Request Plan v1", "IFontRasterizerAdapter", "Runtime UI Image Decode Request Plan v1", "IImageDecodingAdapter", "source image codecs", "renderer texture upload")
}
foreach ($docPath in $runtimeUiDocs.Keys) {
    $docText = Get-JsonContractSurfaceText $docPath
    foreach ($needle in $runtimeUiDocs[$docPath]) {
        if (-not $docText.Contains($needle)) {
            Write-Error "$docPath missing Runtime UI accessibility publish evidence: $needle"
        }
    }
}
$runtimeUiPngDocs = @{
    "docs/current-capabilities.md" = @("Runtime UI PNG Image Decoding Adapter v1", "PngImageDecodingAdapter", "decode_audited_png_rgba8")
    "docs/dependencies.md" = @("PngImageDecodingAdapter", "decode_audited_png_rgba8", "IImageDecodingAdapter")
    "docs/ui.md" = @("Runtime UI PNG Image Decoding Adapter v1", "PngImageDecodingAdapter", "decode_audited_png_rgba8")
    "docs/architecture.md" = @("PngImageDecodingAdapter", "decode_audited_png_rgba8", "IImageDecodingAdapter")
    "docs/roadmap.md" = @("Runtime UI PNG Image Decoding Adapter v1", "PngImageDecodingAdapter", "decode_audited_png_rgba8")
    "docs/specs/2026-04-27-engine-essential-gap-analysis.md" = @("PngImageDecodingAdapter", "decode_audited_png_rgba8", "asset-importers")
    "docs/ai-game-development.md" = @("PngImageDecodingAdapter", "decode_audited_png_rgba8", "source PNGs")
    "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md" = @("2026-05-08-runtime-ui-png-image-decoding-adapter-v1.md", "PngImageDecodingAdapter", "decode_audited_png_rgba8")
    "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" = @("Runtime UI PNG Image Decoding Adapter v1", "PngImageDecodingAdapter", "decode_audited_png_rgba8")
}
foreach ($docPath in $runtimeUiPngDocs.Keys) {
    $docText = Get-JsonContractSurfaceText $docPath
    foreach ($needle in $runtimeUiPngDocs[$docPath]) {
        if (-not $docText.Contains($needle)) {
            Write-Error "$docPath missing Runtime UI PNG image decoding adapter evidence: $needle"
        }
    }
}
$runtimeUiDecodedAtlasDocs = @{
    "docs/current-capabilities.md" = @("Runtime UI Decoded Image Atlas Package Bridge v1", "author_packed_ui_atlas_from_decoded_images", "GameEngine.CookedTexture.v1")
    "docs/dependencies.md" = @("author_packed_ui_atlas_from_decoded_images", "plan_packed_ui_atlas_package_update", "GameEngine.CookedTexture.v1")
    "docs/ui.md" = @("Runtime UI Decoded Image Atlas Package Bridge v1", "author_packed_ui_atlas_from_decoded_images", "GameEngine.CookedTexture.v1")
    "docs/architecture.md" = @("author_packed_ui_atlas_from_decoded_images", "plan_packed_ui_atlas_package_update", "GameEngine.CookedTexture.v1")
    "docs/roadmap.md" = @("Runtime UI Decoded Image Atlas Package Bridge v1", "author_packed_ui_atlas_from_decoded_images", "GameEngine.CookedTexture.v1")
    "docs/specs/2026-04-27-engine-essential-gap-analysis.md" = @("author_packed_ui_atlas_from_decoded_images", "plan_packed_ui_atlas_package_update", "GameEngine.CookedTexture.v1")
    "docs/ai-game-development.md" = @("author_packed_ui_atlas_from_decoded_images", "plan_packed_ui_atlas_package_update", "GameEngine.CookedTexture.v1")
    "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md" = @("2026-05-08-runtime-ui-decoded-image-atlas-package-bridge-v1.md", "PackedUiAtlasAuthoringDesc", "GameEngine.CookedTexture.v1")
    "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" = @("Runtime UI Decoded Image Atlas Package Bridge v1", "author_packed_ui_atlas_from_decoded_images", "GameEngine.CookedTexture.v1")
}
foreach ($docPath in $runtimeUiDecodedAtlasDocs.Keys) {
    $docText = Get-JsonContractSurfaceText $docPath
    foreach ($needle in $runtimeUiDecodedAtlasDocs[$docPath]) {
        if (-not $docText.Contains($needle)) {
            Write-Error "$docPath missing Runtime UI decoded image atlas package bridge evidence: $needle"
        }
    }
}
$runtimeUiGlyphAtlasDocs = @{
    "docs/current-capabilities.md" = @("Runtime UI Glyph Atlas Package Bridge v1", "UiAtlasMetadataGlyph", "RuntimeUiAtlasGlyph", "author_packed_ui_glyph_atlas_from_rasterized_glyphs", "GameEngine.CookedTexture.v1", "GameEngine.UiAtlas.v1")
    "docs/dependencies.md" = @("author_packed_ui_glyph_atlas_from_rasterized_glyphs", "plan_packed_ui_glyph_atlas_package_update", "GameEngine.CookedTexture.v1", "GameEngine.UiAtlas.v1")
    "docs/testing.md" = @("UiAtlasMetadataGlyph", "RuntimeUiAtlasGlyph", "author_packed_ui_glyph_atlas_from_rasterized_glyphs", "build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas")
    "docs/ui.md" = @("Runtime UI Glyph Atlas Package Bridge v1", "UiAtlasMetadataGlyph", "author_packed_ui_glyph_atlas_from_rasterized_glyphs", "source.decoding=rasterized-glyph-adapter", "atlas.packing=deterministic-glyph-atlas-rgba8-max-side")
    "docs/architecture.md" = @("author_packed_ui_glyph_atlas_from_rasterized_glyphs", "build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas", "GameEngine.CookedTexture.v1")
    "docs/roadmap.md" = @("Runtime UI Glyph Atlas Package Bridge v1", "UiAtlasMetadataGlyph", "RuntimeUiAtlasGlyph", "author_packed_ui_glyph_atlas_from_rasterized_glyphs")
    "docs/specs/2026-04-27-engine-essential-gap-analysis.md" = @("Runtime UI Glyph Atlas Package Bridge v1", "UiAtlasMetadataGlyph", "RuntimeUiAtlasGlyph", "author_packed_ui_glyph_atlas_from_rasterized_glyphs")
    "docs/ai-game-development.md" = @("author_packed_ui_glyph_atlas_from_rasterized_glyphs", "plan_packed_ui_glyph_atlas_package_update", "source.decoding=rasterized-glyph-adapter", "atlas.packing=deterministic-glyph-atlas-rgba8-max-side")
    "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md" = @("2026-05-08-runtime-ui-glyph-atlas-package-bridge-v1.md", "Runtime UI Glyph Atlas Package Bridge v1", "PackedUiGlyphAtlasAuthoringDesc")
    "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" = @("Runtime UI Glyph Atlas Package Bridge v1", "author_packed_ui_glyph_atlas_from_rasterized_glyphs", "build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas")
}
foreach ($docPath in $runtimeUiGlyphAtlasDocs.Keys) {
    $docText = Get-JsonContractSurfaceText $docPath
    foreach ($needle in $runtimeUiGlyphAtlasDocs[$docPath]) {
        if (-not $docText.Contains($needle)) {
            Write-Error "$docPath missing Runtime UI glyph atlas package bridge evidence: $needle"
        }
    }
}
$runtimeInputRebindingCaptureDocs = @{
    "docs/current-capabilities.md" = @("Runtime Input Rebinding Capture Contract v1", "capture_runtime_input_rebinding_action", "RuntimeInputRebindingCaptureRequest")
    "docs/architecture.md" = @("capture_runtime_input_rebinding_action", "RuntimeInputRebindingProfile", "axis capture")
    "docs/roadmap.md" = @("Runtime Input Rebinding Capture Contract v1", "RuntimeInputRebindingCaptureResult", "capture_runtime_input_rebinding_action")
    "docs/ai-game-development.md" = @("RuntimeInputRebindingCaptureRequest", "RuntimeInputRebindingCaptureResult", "capture_runtime_input_rebinding_action")
    "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md" = @("2026-05-08-runtime-input-rebinding-capture-contract-v1.md", "RuntimeInputRebindingCaptureRequest", "capture_runtime_input_rebinding_action")
    "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" = @("Runtime Input Rebinding Capture Contract v1", "RuntimeInputRebindingCaptureRequest", "capture_runtime_input_rebinding_action")
}
foreach ($docPath in $runtimeInputRebindingCaptureDocs.Keys) {
    $docText = Get-JsonContractSurfaceText $docPath
    foreach ($needle in $runtimeInputRebindingCaptureDocs[$docPath]) {
        if (-not $docText.Contains($needle)) {
        Write-Error "$docPath missing Runtime input rebinding capture evidence: $needle"
        }
    }
}
$runtimeInputRebindingFocusConsumptionDocs = @{
    "docs/current-capabilities.md" = @("Runtime Input Rebinding Focus Consumption v1", "RuntimeInputRebindingFocusCaptureRequest", "capture_runtime_input_rebinding_action_with_focus", "gameplay_input_consumed")
    "docs/architecture.md" = @("RuntimeInputRebindingFocusCaptureRequest", "RuntimeInputRebindingFocusCaptureResult", "capture_runtime_input_rebinding_action_with_focus", "pressed-but-rejected candidates")
    "docs/roadmap.md" = @("Runtime Input Rebinding Focus Consumption v1", "RuntimeInputRebindingFocusCaptureResult", "capture_runtime_input_rebinding_action_with_focus", "gameplay_input_consumed")
    "docs/testing.md" = @("RuntimeInputRebindingFocusCaptureRequest", "capture_runtime_input_rebinding_action_with_focus", "pressed rejected-trigger consumption")
    "docs/ai-game-development.md" = @("Runtime Input Rebinding Focus Consumption v1", "RuntimeInputRebindingFocusCaptureRequest", "capture_runtime_input_rebinding_action_with_focus", "gameplay_input_consumed")
    "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md" = @("2026-05-08-runtime-input-rebinding-focus-consumption-v1.md", "RuntimeInputRebindingFocusCaptureRequest", "capture_runtime_input_rebinding_action_with_focus")
    "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" = @("Runtime Input Rebinding Focus Consumption v1", "RuntimeInputRebindingFocusCaptureRequest", "capture_runtime_input_rebinding_action_with_focus", "gameplay_input_consumed")
}
foreach ($docPath in $runtimeInputRebindingFocusConsumptionDocs.Keys) {
    $docText = Get-JsonContractSurfaceText $docPath
    foreach ($needle in $runtimeInputRebindingFocusConsumptionDocs[$docPath]) {
        if (-not $docText.Contains($needle)) {
            Write-Error "$docPath missing Runtime input rebinding focus consumption evidence: $needle"
        }
    }
}
$runtimeInputRebindingPresentationRowsDocs = @{
    "docs/current-capabilities.md" = @("Runtime Input Rebinding Presentation Rows v1", "RuntimeInputRebindingPresentationModel", "make_runtime_input_rebinding_presentation", "symbolic glyph lookup keys", "platform input glyph generation")
    "docs/architecture.md" = @("RuntimeInputRebindingPresentationToken", "make_runtime_input_rebinding_presentation", "symbolic glyph lookup keys", "platform input glyph generation")
    "docs/roadmap.md" = @("Runtime Input Rebinding Presentation Rows v1", "RuntimeInputRebindingPresentationRow", "present_runtime_input_axis_source", "symbolic glyph lookup keys")
    "docs/testing.md" = @("Runtime Input Rebinding Presentation Rows v1", "RuntimeInputRebindingPresentationToken", "glyph_lookup_key", "invalid-profile diagnostics")
    "docs/ai-game-development.md" = @("Runtime Input Rebinding Presentation Rows v1", "RuntimeInputRebindingPresentationModel", "make_runtime_input_rebinding_presentation", "symbolic glyph lookup keys")
    "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md" = @("2026-05-08-runtime-input-rebinding-presentation-rows-v1.md", "Runtime Input Rebinding Presentation Rows v1", "RuntimeInputRebindingPresentationModel")
    "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" = @("Runtime Input Rebinding Presentation Rows v1", "RuntimeInputRebindingPresentationToken", "make_runtime_input_rebinding_presentation", "platform input glyph generation")
}
foreach ($docPath in $runtimeInputRebindingPresentationRowsDocs.Keys) {
    $docText = Get-JsonContractSurfaceText $docPath
    foreach ($needle in $runtimeInputRebindingPresentationRowsDocs[$docPath]) {
        if (-not $docText.Contains($needle)) {
            Write-Error "$docPath missing Runtime input rebinding presentation rows evidence: $needle"
        }
    }
}
$editorInputRebindingActionCaptureDocs = @{
    "docs/current-capabilities.md" = @("Editor Input Rebinding Action Capture Panel v1", "EditorInputRebindingCaptureModel", "in-memory profile")
    "docs/editor.md" = @("Editor Input Rebinding Action Capture Panel v1", "make_editor_input_rebinding_capture_action_model", "input_rebinding.capture")
    "docs/testing.md" = @("Editor Input Rebinding Action Capture Panel v1", "make_input_rebinding_capture_action_ui_model", "pressed-key candidate capture")
    "docs/roadmap.md" = @("EditorInputRebindingCaptureModel", "action-row capture controls", "in-memory profile")
    "docs/architecture.md" = @("reviewed editor action capture rows", "RuntimeInputStateView", "axis capture")
    "docs/ai-game-development.md" = @("EditorInputRebindingCaptureModel", "make_editor_input_rebinding_capture_action_model", "in-memory profile")
    "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md" = @("2026-05-08-editor-input-rebinding-action-capture-panel-v1.md", "Editor Input Rebinding Action Capture Panel v1", "in-memory profile")
    "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md" = @("Editor Input Rebinding Action Capture Panel v1", "EditorInputRebindingCaptureModel", "reviewed editor action capture lane")
}
foreach ($docPath in $editorInputRebindingActionCaptureDocs.Keys) {
    $docText = Get-JsonContractSurfaceText $docPath
    foreach ($needle in $editorInputRebindingActionCaptureDocs[$docPath]) {
        if (-not $docText.Contains($needle)) {
            Write-Error "$docPath missing Editor input rebinding action capture evidence: $needle"
        }
    }
}
$geUiHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/ui/include/mirakana/ui/ui.hpp") -Raw
$geUiSourceText = Get-Content -LiteralPath (Join-Path $root "engine/ui/src/ui.cpp") -Raw
$sourceImageDecodeHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/tools/include/mirakana/tools/source_image_decode.hpp") -Raw
$sourceImageDecodeSourceText = Get-Content -LiteralPath (Join-Path $root "engine/tools/asset/source_image_decode.cpp") -Raw
$uiAtlasToolHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/tools/include/mirakana/tools/ui_atlas_tool.hpp") -Raw
$uiAtlasToolSourceText = Get-Content -LiteralPath (Join-Path $root "engine/tools/asset/ui_atlas_tool.cpp") -Raw
$toolsTestsText = Get-Content -LiteralPath (Join-Path $root "tests/unit/tools_tests.cpp") -Raw
$uiRendererTestsText = Get-Content -LiteralPath (Join-Path $root "tests/unit/ui_renderer_tests.cpp") -Raw
