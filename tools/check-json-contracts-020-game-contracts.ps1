#requires -Version 7.0
#requires -PSEdition Core

# Chapter 2 for check-json-contracts.ps1 static contracts.

if ($editorAiPlaytestOperatorWorkflowLoop.Count -eq 1) {
    Assert-Properties $editorAiPlaytestOperatorWorkflowLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "workflowInputs", "workflowFields", "structuredReportSurface", "closeoutPolicy", "blockedExecution", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop editor-ai-playtest-operator-workflow"
    if ($editorAiPlaytestOperatorWorkflowLoop[0].status -ne "ready") {
        Write-Error "engine manifest editor-ai-playtest-operator-workflow must be ready as a read-only consolidated operator workflow"
    }
    $expectedEditorAiOperatorWorkflowSteps = @(
        "inspect-package-authoring-diagnostics",
        "preflight-validation-recipes",
        "report-playtest-readiness",
        "handoff-reviewed-operator-commands",
        "summarize-external-validation-evidence",
        "queue-nonpassing-remediation",
        "handoff-remediation-actions",
        "closeout-by-rerunning-evidence-summary"
    )
    $actualEditorAiOperatorWorkflowSteps = @($editorAiPlaytestOperatorWorkflowLoop[0].orderedSteps | ForEach-Object {
        Assert-Properties $_ @("id", "surface", "status", "mutates", "executes", "notes") "engine manifest editor-ai-playtest-operator-workflow ordered step"
        if ($_.mutates -ne $false) {
            Write-Error "engine manifest editor-ai-playtest-operator-workflow step '$($_.id)' must not mutate"
        }
        if ($_.executes -ne $false) {
            Write-Error "engine manifest editor-ai-playtest-operator-workflow step '$($_.id)' must not execute"
        }
        $_.id
    })
    if (($actualEditorAiOperatorWorkflowSteps -join "|") -ne ($expectedEditorAiOperatorWorkflowSteps -join "|")) {
        Write-Error "engine manifest editor-ai-playtest-operator-workflow orderedSteps must be exactly: $($expectedEditorAiOperatorWorkflowSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "prefabScenePackageAuthoringTargets", "registeredSourceAssetCookTargets", "validationRecipes")) {
        if (@($editorAiPlaytestOperatorWorkflowLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine manifest editor-ai-playtest-operator-workflow requiredManifestFields missing: $field"
        }
    }
    foreach ($requiredInput in @("EditorAiPackageAuthoringDiagnosticsModel", "EditorAiValidationRecipePreflightModel", "EditorAiPlaytestReadinessReportModel", "EditorAiPlaytestOperatorHandoffModel", "externally supplied validation evidence", "EditorAiPlaytestEvidenceSummaryModel", "EditorAiPlaytestRemediationQueueModel", "EditorAiPlaytestRemediationHandoffModel")) {
        if (-not ((@($editorAiPlaytestOperatorWorkflowLoop[0].workflowInputs) -join " ").Contains($requiredInput))) {
            Write-Error "engine manifest editor-ai-playtest-operator-workflow workflowInputs missing: $requiredInput"
        }
    }
    foreach ($field in @("package diagnostics", "validation preflight", "readiness dependency", "operator command rows", "evidence status", "remediation category", "external-owner", "closeout through existing evidence summary")) {
        if (-not ((@($editorAiPlaytestOperatorWorkflowLoop[0].workflowFields) -join " ").Contains($field))) {
            Write-Error "engine manifest editor-ai-playtest-operator-workflow workflowFields missing: $field"
        }
    }
    Assert-Properties $editorAiPlaytestOperatorWorkflowLoop[0].structuredReportSurface @("model", "stageFields", "closeoutFields", "blockedExecution", "unsupportedClaims", "notes") "engine manifest editor-ai-playtest-operator-workflow structuredReportSurface"
    if ($editorAiPlaytestOperatorWorkflowLoop[0].structuredReportSurface.model -ne "EditorAiPlaytestOperatorWorkflowReportModel") {
        Write-Error "engine manifest editor-ai-playtest-operator-workflow structuredReportSurface.model must be EditorAiPlaytestOperatorWorkflowReportModel"
    }
    foreach ($field in @("operator workflow stage", "source model", "source row count", "source row ids", "status", "host gates", "blockers", "diagnostic")) {
        if (-not ((@($editorAiPlaytestOperatorWorkflowLoop[0].structuredReportSurface.stageFields) -join " ").Contains($field))) {
            Write-Error "engine manifest editor-ai-playtest-operator-workflow structuredReportSurface.stageFields missing: $field"
        }
    }
    foreach ($field in @("all_required_evidence_passed", "remediation_required=false", "handoff_required=false", "closeout_complete", "external_action_required")) {
        if (-not ((@($editorAiPlaytestOperatorWorkflowLoop[0].structuredReportSurface.closeoutFields) -join " ").Contains($field))) {
            Write-Error "engine manifest editor-ai-playtest-operator-workflow structuredReportSurface.closeoutFields missing: $field"
        }
    }
    foreach ($claim in @("arbitrary shell", "raw manifest command strings", "raw/free-form command evaluation", "validation execution", "package script execution", "free-form manifest edits", "manifest mutation", "evidence mutation", "remediation mutation", "fix execution", "play-in-editor productization", "renderer/RHI handle exposure", "Metal readiness", "general renderer quality")) {
        if (-not ((@($editorAiPlaytestOperatorWorkflowLoop[0].structuredReportSurface.blockedExecution) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-playtest-operator-workflow structuredReportSurface.blockedExecution missing: $claim"
        }
        if (-not ((@($editorAiPlaytestOperatorWorkflowLoop[0].structuredReportSurface.unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-playtest-operator-workflow structuredReportSurface.unsupportedClaims missing: $claim"
        }
    }
    foreach ($policy in @("no separate remediation evidence-review row model", "no separate remediation closeout-report row model", "all_required_evidence_passed", "remediation_required=false", "handoff_required=false")) {
        if (-not ((@($editorAiPlaytestOperatorWorkflowLoop[0].closeoutPolicy) -join " ").Contains($policy))) {
            Write-Error "engine manifest editor-ai-playtest-operator-workflow closeoutPolicy missing: $policy"
        }
    }
    foreach ($claim in @("arbitrary shell", "raw manifest command strings", "raw/free-form command evaluation", "validation execution", "package script execution", "free-form manifest edits", "evidence mutation", "remediation mutation", "fix execution", "play-in-editor productization", "renderer/RHI handle exposure", "Metal readiness", "general renderer quality")) {
        if (-not ((@($editorAiPlaytestOperatorWorkflowLoop[0].blockedExecution) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-playtest-operator-workflow blockedExecution missing: $claim"
        }
        if (-not ((@($editorAiPlaytestOperatorWorkflowLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-playtest-operator-workflow unsupportedClaims missing: $claim"
        }
    }
}
$rendererResourceExecutionLoop = @($productionLoop.resourceExecutionLoops | Where-Object { $_.id -eq "renderer-resource-residency-upload-execution" })
if ($rendererResourceExecutionLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one renderer-resource-residency-upload-execution loop"
}

$materialShaderAuthoringLoop = @($productionLoop.materialShaderAuthoringLoops | Where-Object { $_.id -eq "material-shader-authoring-review-loop" })
if ($materialShaderAuthoringLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one material-shader-authoring-review-loop"
} else {
    Assert-Properties $materialShaderAuthoringLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "preSmokeGates", "descriptorFields", "hostGatedSmokeRecipes", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop material-shader-authoring-review-loop"
    if ($materialShaderAuthoringLoop[0].status -ne "host-gated") {
        Write-Error "engine manifest material-shader-authoring-review-loop must remain host-gated"
    }
    $expectedMaterialShaderSteps = @(
        "review-source-material-authoring-inputs",
        "validate-source-material-and-texture-dependencies",
        "review-fixed-shader-artifact-requests",
        "validate-shader-artifacts",
        "run-host-gated-material-shader-package-smoke"
    )
    $actualMaterialShaderSteps = @($materialShaderAuthoringLoop[0].orderedSteps | ForEach-Object {
            Assert-Properties $_ @("id", "surface", "status", "mutates", "notes") "engine manifest material-shader-authoring-review-loop ordered step"
            $_.id
        })
    if (($actualMaterialShaderSteps -join "|") -ne ($expectedMaterialShaderSteps -join "|")) {
        Write-Error "engine manifest material-shader-authoring-review-loop orderedSteps must be exactly: $($expectedMaterialShaderSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "materialShaderAuthoringTargets", "validationRecipes")) {
        if (@($materialShaderAuthoringLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine manifest material-shader-authoring-review-loop requiredManifestFields missing: $field"
        }
    }
    foreach ($field in @("id", "sourceMaterialPath", "runtimeMaterialPath", "packageIndexPath", "shaderSourcePaths", "d3d12ShaderArtifactPaths", "vulkanShaderArtifactPaths")) {
        if (@($materialShaderAuthoringLoop[0].descriptorFields) -notcontains $field) {
            Write-Error "engine manifest material-shader-authoring-review-loop descriptorFields missing: $field"
        }
    }
    foreach ($claim in @("shader graph", "material graph", "live shader generation", "package streaming", "public native/RHI handles", "Metal readiness", "general renderer quality")) {
        if (-not ((@($materialShaderAuthoringLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest material-shader-authoring-review-loop unsupportedClaims missing: $claim"
        }
    }
}
$atlasTilemapAuthoringLoop = @($productionLoop.atlasTilemapAuthoringLoops | Where-Object { $_.id -eq "2d-atlas-tilemap-package-authoring" })
if ($atlasTilemapAuthoringLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one 2d-atlas-tilemap-package-authoring loop"
} else {
    Assert-Properties $atlasTilemapAuthoringLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "descriptorFields", "preflightGates", "hostGatedSmokeRecipes", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop 2d-atlas-tilemap-package-authoring"
    if ($atlasTilemapAuthoringLoop[0].status -ne "host-gated") {
        Write-Error "engine manifest 2d-atlas-tilemap-package-authoring must remain host-gated because desktop package recipes are optional host lanes"
    }
    $expectedAtlasTilemapSteps = @(
        "select-atlas-tilemap-authoring-target",
        "author-deterministic-tilemap-metadata",
        "update-tilemap-package-index",
        "validate-runtime-tilemap-payload",
        "run-host-gated-2d-package-preflight"
    )
    $actualAtlasTilemapSteps = @($atlasTilemapAuthoringLoop[0].orderedSteps | ForEach-Object {
            Assert-Properties $_ @("id", "surface", "status", "mutates", "notes") "engine manifest 2d-atlas-tilemap-package-authoring ordered step"
            $_.id
        })
    if (($actualAtlasTilemapSteps -join "|") -ne ($expectedAtlasTilemapSteps -join "|")) {
        Write-Error "engine manifest 2d-atlas-tilemap-package-authoring orderedSteps must be exactly: $($expectedAtlasTilemapSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "atlasTilemapAuthoringTargets", "validationRecipes")) {
        if (@($atlasTilemapAuthoringLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine manifest 2d-atlas-tilemap-package-authoring requiredManifestFields missing: $field"
        }
    }
    foreach ($field in @("id", "packageIndexPath", "tilemapPath", "atlasTexturePath", "tilemapAssetKey", "atlasTextureAssetKey", "mode", "sourceDecoding", "atlasPacking", "nativeGpuSpriteBatching", "preflightRecipeIds")) {
        if (@($atlasTilemapAuthoringLoop[0].descriptorFields) -notcontains $field) {
            Write-Error "engine manifest 2d-atlas-tilemap-package-authoring descriptorFields missing: $field"
        }
    }
    foreach ($gate in @("validate-runtime-scene-package", "desktop-game-runtime", "desktop-runtime-release-target")) {
        if (@($atlasTilemapAuthoringLoop[0].preflightGates) -notcontains $gate) {
            Write-Error "engine manifest 2d-atlas-tilemap-package-authoring preflightGates missing: $gate"
        }
    }
    foreach ($claim in @("source image decoding", "production atlas packing", "tilemap editor UX", "native GPU sprite batching", "package streaming execution", "public native/RHI handles", "Metal readiness", "general renderer quality")) {
        if (-not ((@($atlasTilemapAuthoringLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest 2d-atlas-tilemap-package-authoring unsupportedClaims missing: $claim"
        }
    }
}
if (-not $productionLoop.PSObject.Properties.Name.Contains("prefabScenePackageAuthoringLoops")) {
    Write-Error "engine manifest aiOperableProductionLoop missing prefabScenePackageAuthoringLoops"
}
$prefabScene3dAuthoringLoop = @($productionLoop.prefabScenePackageAuthoringLoops | Where-Object { $_.id -eq "3d-prefab-scene-package-authoring" })
if ($prefabScene3dAuthoringLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one 3d-prefab-scene-package-authoring loop"
} else {
    Assert-Properties $prefabScene3dAuthoringLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "descriptorFields", "preflightGates", "hostGatedSmokeRecipes", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop 3d-prefab-scene-package-authoring"
    if ($prefabScene3dAuthoringLoop[0].status -ne "host-gated") {
        Write-Error "engine manifest 3d-prefab-scene-package-authoring must remain host-gated because desktop package recipes are optional host lanes"
    }
    $expectedPrefabScene3dSteps = @(
        "select-prefab-scene-package-authoring-target",
        "apply-scene-prefab-v2-authoring-commands",
        "cook-selected-source-registry-rows",
        "migrate-scene-v2-runtime-package",
        "validate-runtime-scene-package",
        "run-host-gated-3d-package-smoke"
    )
    $actualPrefabScene3dSteps = @($prefabScene3dAuthoringLoop[0].orderedSteps | ForEach-Object {
            Assert-Properties $_ @("id", "surface", "status", "mutates", "notes") "engine manifest 3d-prefab-scene-package-authoring ordered step"
            $_.id
        })
    if (($actualPrefabScene3dSteps -join "|") -ne ($expectedPrefabScene3dSteps -join "|")) {
        Write-Error "engine manifest 3d-prefab-scene-package-authoring orderedSteps must be exactly: $($expectedPrefabScene3dSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "prefabScenePackageAuthoringTargets", "registeredSourceAssetCookTargets", "validationRecipes")) {
        if (@($prefabScene3dAuthoringLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine manifest 3d-prefab-scene-package-authoring requiredManifestFields missing: $field"
        }
    }
    foreach ($field in @("id", "mode", "sceneAuthoringPath", "prefabAuthoringPath", "sourceRegistryPath", "packageIndexPath", "outputScenePath", "sceneAssetKey", "runtimeSceneValidationTargetId", "authoringCommandRows", "selectedSourceAssetKeys", "sourceCookMode", "sceneMigration", "runtimeSceneValidation", "hostGatedSmokeRecipeIds", "broadImporterExecution", "broadDependencyCooking", "runtimeSourceParsing", "materialGraph", "shaderGraph", "liveShaderGeneration", "skeletalAnimation", "gpuSkinning", "publicNativeRhiHandles", "metalReadiness", "rendererQuality", "cookCommandId", "prefabScenePackageAuthoringTargetId", "selectedAssetKeys", "dependencyExpansion", "dependencyCooking", "externalImporterExecution", "rendererRhiResidency", "packageStreaming", "editorProductization", "generalProductionRendererQuality", "arbitraryShell", "freeFormEdit")) {
        if (@($prefabScene3dAuthoringLoop[0].descriptorFields) -notcontains $field) {
            Write-Error "engine manifest 3d-prefab-scene-package-authoring descriptorFields missing: $field"
        }
    }
    foreach ($gate in @("scene-prefab-authoring", "cook-registered-source-assets", "migrate-scene-v2-runtime-package", "validate-runtime-scene-package", "desktop-game-runtime", "desktop-runtime-release-target")) {
        if (@($prefabScene3dAuthoringLoop[0].preflightGates) -notcontains $gate) {
            Write-Error "engine manifest 3d-prefab-scene-package-authoring preflightGates missing: $gate"
        }
    }
    foreach ($claim in @("broad importer execution", "broad/dependent package cooking", "runtime source parsing", "material graph", "shader graph", "live shader generation", "skeletal animation", "GPU skinning", "public native/RHI handles", "Metal readiness", "general renderer quality")) {
        if (-not ((@($prefabScene3dAuthoringLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest 3d-prefab-scene-package-authoring unsupportedClaims missing: $claim"
        }
    }
}
$packageStreamingResidencyLoop = @($productionLoop.packageStreamingResidencyLoops | Where-Object { $_.id -eq "package-streaming-residency-budget-contract" })
if ($packageStreamingResidencyLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one package-streaming-residency-budget-contract loop"
} else {
    Assert-Properties $packageStreamingResidencyLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "descriptorFields", "preflightGates", "blockedExecution", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop package-streaming-residency-budget-contract"
    if ($packageStreamingResidencyLoop[0].status -ne "ready") {
        Write-Error "engine manifest package-streaming-residency-budget-contract must be ready after the descriptor contract is implemented"
    }
    $expectedPackageStreamingSteps = @(
        "select-package-streaming-residency-target",
        "validate-runtime-scene-package",
        "review-residency-budget-intent",
        "confirm-host-owned-resource-upload-gate",
        "select-host-gated-safe-point-execution"
    )
    $actualPackageStreamingSteps = @($packageStreamingResidencyLoop[0].orderedSteps | ForEach-Object {
            Assert-Properties $_ @("id", "surface", "status", "mutates", "notes") "engine manifest package-streaming-residency-budget-contract ordered step"
            $_.id
        })
    if (($actualPackageStreamingSteps -join "|") -ne ($expectedPackageStreamingSteps -join "|")) {
        Write-Error "engine manifest package-streaming-residency-budget-contract orderedSteps must be exactly: $($expectedPackageStreamingSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "packageStreamingResidencyTargets", "validationRecipes")) {
        if (@($packageStreamingResidencyLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine manifest package-streaming-residency-budget-contract requiredManifestFields missing: $field"
        }
    }
    foreach ($field in @("id", "packageIndexPath", "runtimeSceneValidationTargetId", "mode", "residentBudgetBytes", "safePointRequired", "preloadAssetKeys", "residentResourceKinds")) {
        if (@($packageStreamingResidencyLoop[0].descriptorFields) -notcontains $field) {
            Write-Error "engine manifest package-streaming-residency-budget-contract descriptorFields missing: $field"
        }
    }
    foreach ($gate in @("validate-runtime-scene-package", "renderer-resource-residency-upload-execution")) {
        if (@($packageStreamingResidencyLoop[0].preflightGates) -notcontains $gate) {
            Write-Error "engine manifest package-streaming-residency-budget-contract preflightGates missing: $gate"
        }
    }
    foreach ($blocked in @("background package streaming", "arbitrary eviction", "public native/RHI handles", "Metal readiness", "production renderer quality")) {
        if (-not ((@($packageStreamingResidencyLoop[0].blockedExecution) -join " ").Contains($blocked))) {
            Write-Error "engine manifest package-streaming-residency-budget-contract blockedExecution missing: $blocked"
        }
    }
    foreach ($claim in @("broad async/background package streaming", "arbitrary eviction", "texture streaming", "public native/RHI handles", "Metal readiness", "general renderer quality")) {
        if (-not ((@($packageStreamingResidencyLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest package-streaming-residency-budget-contract unsupportedClaims missing: $claim"
        }
    }
}
$hostGatedPackageStreamingLoop = @($productionLoop.packageStreamingResidencyLoops | Where-Object { $_.id -eq "host-gated-package-streaming-execution" })
if ($hostGatedPackageStreamingLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one host-gated-package-streaming-execution loop"
} else {
    Assert-Properties $hostGatedPackageStreamingLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "descriptorFields", "preflightGates", "resultFields", "blockedExecution", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop host-gated-package-streaming-execution"
    if ($hostGatedPackageStreamingLoop[0].status -ne "host-gated") {
        Write-Error "engine manifest host-gated-package-streaming-execution must be host-gated"
    }
    $expectedHostGatedStreamingSteps = @(
        "select-package-streaming-residency-target",
        "validate-runtime-scene-package",
        "load-selected-runtime-package",
        "commit-safe-point-package-streaming-replacement",
        "report-streaming-execution-diagnostics",
        "keep-renderer-rhi-teardown-host-owned"
    )
    $actualHostGatedStreamingSteps = @($hostGatedPackageStreamingLoop[0].orderedSteps | ForEach-Object {
            Assert-Properties $_ @("id", "surface", "status", "mutates", "notes") "engine manifest host-gated-package-streaming-execution ordered step"
            $_.id
        })
    if (($actualHostGatedStreamingSteps -join "|") -ne ($expectedHostGatedStreamingSteps -join "|")) {
        Write-Error "engine manifest host-gated-package-streaming-execution orderedSteps must be exactly: $($expectedHostGatedStreamingSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "packageStreamingResidencyTargets", "validationRecipes")) {
        if (@($hostGatedPackageStreamingLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine manifest host-gated-package-streaming-execution requiredManifestFields missing: $field"
        }
    }
    foreach ($field in @("id", "packageIndexPath", "runtimeSceneValidationTargetId", "mode", "residentBudgetBytes", "safePointRequired", "preloadAssetKeys", "residentResourceKinds", "maxResidentPackages", "preflightRecipeIds")) {
        if (@($hostGatedPackageStreamingLoop[0].descriptorFields) -notcontains $field) {
            Write-Error "engine manifest host-gated-package-streaming-execution descriptorFields missing: $field"
        }
    }
    foreach ($gate in @("validate-runtime-scene-package", "safe-point-package-unload-replacement-execution")) {
        if (@($hostGatedPackageStreamingLoop[0].preflightGates) -notcontains $gate) {
            Write-Error "engine manifest host-gated-package-streaming-execution preflightGates missing: $gate"
        }
    }
    foreach ($field in @("status", "target_id", "package_index_path", "runtime_scene_validation_target_id", "estimated_resident_bytes", "resident_budget_bytes", "replacement_status", "stale_handle_count", "diagnostics")) {
        if (@($hostGatedPackageStreamingLoop[0].resultFields) -notcontains $field) {
            Write-Error "engine manifest host-gated-package-streaming-execution resultFields missing: $field"
        }
    }
    foreach ($blocked in @("background package streaming", "arbitrary eviction", "dependency-driven streaming middleware", "renderer/RHI teardown execution", "public native/RHI handles", "allocator/GPU budget enforcement", "Metal readiness", "production renderer quality")) {
        if (-not ((@($hostGatedPackageStreamingLoop[0].blockedExecution) -join " ").Contains($blocked))) {
            Write-Error "engine manifest host-gated-package-streaming-execution blockedExecution missing: $blocked"
        }
    }
    foreach ($claim in @("broad async/background package streaming", "arbitrary eviction", "texture streaming", "allocator/GPU budget enforcement", "public native/RHI handles", "Metal readiness", "general renderer quality")) {
        if (-not ((@($hostGatedPackageStreamingLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest host-gated-package-streaming-execution unsupportedClaims missing: $claim"
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
$safePointPackageReplacementLoop = @($productionLoop.safePointPackageReplacementLoops | Where-Object { $_.id -eq "safe-point-package-unload-replacement-execution" })
if ($safePointPackageReplacementLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one safe-point-package-unload-replacement-execution loop"
} else {
    Assert-Properties $safePointPackageReplacementLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredModules", "resultFields", "blockedExecution", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop safe-point-package-unload-replacement-execution"
    if ($safePointPackageReplacementLoop[0].status -ne "ready") {
        Write-Error "engine manifest safe-point-package-unload-replacement-execution must be ready after runtime package/catalog safe-point replacement tests pass"
    }
    $expectedSafePointSteps = @(
        "stage-loaded-runtime-package",
        "build-pending-resource-catalog",
        "commit-package-and-resource-catalog-at-safe-point",
        "reject-invalid-package-before-active-swap",
        "commit-unload-and-empty-catalog-at-safe-point",
        "keep-renderer-rhi-teardown-host-owned"
    )
    $actualSafePointSteps = @($safePointPackageReplacementLoop[0].orderedSteps | ForEach-Object {
            Assert-Properties $_ @("id", "surface", "status", "mutates", "notes") "engine manifest safe-point-package-unload-replacement-execution ordered step"
            $_.id
        })
    if (($actualSafePointSteps -join "|") -ne ($expectedSafePointSteps -join "|")) {
        Write-Error "engine manifest safe-point-package-unload-replacement-execution orderedSteps must be exactly: $($expectedSafePointSteps -join ', ')"
    }
    foreach ($module in @("MK_runtime")) {
        if (@($safePointPackageReplacementLoop[0].requiredModules) -notcontains $module) {
            Write-Error "engine manifest safe-point-package-unload-replacement-execution requiredModules missing: $module"
        }
    }
    foreach ($field in @("status", "previous_record_count", "committed_record_count", "previous_generation", "committed_generation", "stale_handle_count", "discarded_pending_package", "diagnostics")) {
        if (@($safePointPackageReplacementLoop[0].resultFields) -notcontains $field) {
            Write-Error "engine manifest safe-point-package-unload-replacement-execution resultFields missing: $field"
        }
    }
    foreach ($blocked in @("async package streaming", "background eviction", "renderer/RHI teardown execution", "public native/RHI handles", "Metal readiness", "production renderer quality")) {
        if (-not ((@($safePointPackageReplacementLoop[0].blockedExecution) -join " ").Contains($blocked))) {
            Write-Error "engine manifest safe-point-package-unload-replacement-execution blockedExecution missing: $blocked"
        }
    }
    foreach ($claim in @("broad package streaming", "async eviction", "texture streaming", "allocator/GPU budget enforcement", "public native/RHI handles", "Metal readiness", "general renderer quality")) {
        if (-not ((@($safePointPackageReplacementLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest safe-point-package-unload-replacement-execution unsupportedClaims missing: $claim"
        }
    }
}
if ($rendererResourceExecutionLoop.Count -eq 1) {
    Assert-Properties $rendererResourceExecutionLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "preUploadGate", "hostGatedSmokeRecipes", "reportFields", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop renderer-resource-residency-upload-execution"
    if ($rendererResourceExecutionLoop[0].status -ne "host-gated") {
        Write-Error "engine manifest renderer-resource-residency-upload-execution must remain host-gated"
    }
    $expectedResourceExecutionSteps = @(
        "validate-runtime-scene-package",
        "instantiate-runtime-scene",
        "build-scene-render-packet",
        "execute-host-owned-runtime-scene-gpu-upload",
        "report-backend-neutral-upload-residency-counters",
        "run-host-gated-scene-gpu-smoke"
    )
    $actualResourceExecutionSteps = @($rendererResourceExecutionLoop[0].orderedSteps | ForEach-Object {
        Assert-Properties $_ @("id", "surface", "status", "mutates", "notes") "engine manifest renderer-resource-residency-upload-execution ordered step"
        $_.id
    })
    if (($actualResourceExecutionSteps -join "|") -ne ($expectedResourceExecutionSteps -join "|")) {
        Write-Error "engine manifest renderer-resource-residency-upload-execution orderedSteps must be exactly: $($expectedResourceExecutionSteps -join ', ')"
    }
    if ($rendererResourceExecutionLoop[0].preUploadGate -ne "validate-runtime-scene-package") {
        Write-Error "engine manifest renderer-resource-residency-upload-execution preUploadGate must be validate-runtime-scene-package"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "validationRecipes")) {
        if (@($rendererResourceExecutionLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine manifest renderer-resource-residency-upload-execution requiredManifestFields missing: $field"
        }
    }
    foreach ($field in @("scene_gpu_status", "scene_gpu_mesh_uploads", "scene_gpu_texture_uploads", "scene_gpu_material_bindings", "scene_gpu_uploaded_texture_bytes", "scene_gpu_uploaded_mesh_bytes", "scene_gpu_uploaded_material_factor_bytes", "scene_gpu_morph_mesh_bindings", "scene_gpu_morph_mesh_uploads", "scene_gpu_uploaded_morph_bytes", "scene_gpu_morph_mesh_resolved", "renderer_gpu_morph_draws", "renderer_morph_descriptor_binds")) {
        if (@($rendererResourceExecutionLoop[0].reportFields) -notcontains $field) {
            Write-Error "engine manifest renderer-resource-residency-upload-execution reportFields missing: $field"
        }
    }
    foreach ($claim in @("public native handles", "IRhiDevice exposure to gameplay", "package streaming", "broad residency budgets", "material/shader graph", "live shader generation", "Metal readiness", "general renderer quality")) {
        if (-not ((@($rendererResourceExecutionLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest renderer-resource-residency-upload-execution unsupportedClaims missing: $claim"
        }
    }
}
$expectedProductionRecipeIds = @(
    "headless-gameplay",
    "desktop-runtime-config-package",
    "desktop-runtime-cooked-scene-package",
    "desktop-runtime-material-shader-package",
    "ai-navigation-headless",
    "runtime-ui-headless",
    "2d-playable-source-tree",
    "2d-desktop-runtime-package",
    "3d-playable-desktop-package",
    "native-gpu-runtime-ui-overlay",
    "native-ui-textured-sprite-atlas",
    "native-ui-atlas-package-metadata",
    "future-3d-playable-vertical-slice"
)
$productionRecipeIds = @{}
foreach ($recipe in $productionLoop.recipes) {
    Assert-Properties $recipe @("id", "status", "summary", "requiredModules", "allowedTemplates", "allowedPackagingTargets", "importerAssumptions", "cookedRuntimeAssumptions", "rendererBackendAssumptions", "validationRecipes", "unsupportedClaims", "followUpCapability") "engine manifest aiOperableProductionLoop recipe"
    if ($productionRecipeIds.ContainsKey($recipe.id)) {
        Write-Error "engine manifest aiOperableProductionLoop recipe id is duplicated: $($recipe.id)"
    }
    $productionRecipeIds[$recipe.id] = $true
    if ($allowedProductionStatuses -notcontains $recipe.status) {
        Write-Error "engine manifest aiOperableProductionLoop recipe '$($recipe.id)' has invalid status: $($recipe.status)"
    }
    foreach ($module in @($recipe.requiredModules)) {
        if (-not $moduleNames.ContainsKey($module)) {
            Write-Error "engine manifest aiOperableProductionLoop recipe '$($recipe.id)' references unknown module: $module"
        }
    }
    foreach ($target in @($recipe.allowedPackagingTargets)) {
        if (-not $packagingTargetNames.ContainsKey($target)) {
            Write-Error "engine manifest aiOperableProductionLoop recipe '$($recipe.id)' references unknown packaging target: $target"
        }
    }
    foreach ($validationRecipe in @($recipe.validationRecipes)) {
        if (-not $validationRecipeNames.ContainsKey($validationRecipe)) {
            Write-Error "engine manifest aiOperableProductionLoop recipe '$($recipe.id)' references unknown validation recipe: $validationRecipe"
        }
    }
    if (@("ready", "host-gated") -contains $recipe.status -and @($recipe.validationRecipes).Count -lt 1) {
        Write-Error "engine manifest aiOperableProductionLoop recipe '$($recipe.id)' must declare validation recipes when status is $($recipe.status)"
    }
    if (@($recipe.unsupportedClaims).Count -lt 1) {
        Write-Error "engine manifest aiOperableProductionLoop recipe '$($recipe.id)' must list unsupportedClaims"
    }
}
foreach ($recipeId in $expectedProductionRecipeIds) {
    if (-not $productionRecipeIds.ContainsKey($recipeId)) {
        Write-Error "engine manifest aiOperableProductionLoop missing recipe id: $recipeId"
    }
}
foreach ($recipeId in @("future-3d-playable-vertical-slice")) {
    $futureRecipe = @($productionLoop.recipes | Where-Object { $_.id -eq $recipeId })
    if ($futureRecipe.Count -ne 1 -or $futureRecipe[0].status -ne "planned") {
        Write-Error "engine manifest aiOperableProductionLoop recipe '$recipeId' must remain planned"
    }
}
foreach ($recipeId in @("desktop-runtime-config-package", "desktop-runtime-cooked-scene-package", "desktop-runtime-material-shader-package", "2d-desktop-runtime-package", "3d-playable-desktop-package", "native-gpu-runtime-ui-overlay", "native-ui-textured-sprite-atlas", "native-ui-atlas-package-metadata")) {
    $desktopRecipe = @($productionLoop.recipes | Where-Object { $_.id -eq $recipeId })
    if ($desktopRecipe.Count -ne 1 -or $desktopRecipe[0].status -ne "host-gated") {
        Write-Error "engine manifest aiOperableProductionLoop recipe '$recipeId' must remain host-gated"
    }
}
foreach ($recipeId in @("headless-gameplay", "ai-navigation-headless", "runtime-ui-headless", "2d-playable-source-tree")) {
    $readyRecipe = @($productionLoop.recipes | Where-Object { $_.id -eq $recipeId })
    if ($readyRecipe.Count -ne 1 -or $readyRecipe[0].status -ne "ready") {
        Write-Error "engine manifest aiOperableProductionLoop recipe '$recipeId' must remain ready"
    }
}
$sourceTree2dRecipe = @($productionLoop.recipes | Where-Object { $_.id -eq "2d-playable-source-tree" })
if ($sourceTree2dRecipe.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose exactly one 2d-playable-source-tree recipe"
} else {
    foreach ($module in @("MK_core", "MK_runtime", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_audio", "MK_renderer")) {
        if (@($sourceTree2dRecipe[0].requiredModules) -notcontains $module) {
            Write-Error "engine manifest 2d-playable-source-tree recipe missing required module: $module"
        }
    }
    if (@($sourceTree2dRecipe[0].allowedTemplates) -notcontains "Headless") {
        Write-Error "engine manifest 2d-playable-source-tree recipe must allow the Headless template for source-tree proof"
    }
    if (@($sourceTree2dRecipe[0].allowedPackagingTargets) -notcontains "source-tree-default") {
        Write-Error "engine manifest 2d-playable-source-tree recipe must be source-tree-default validated"
    }
    if (@($sourceTree2dRecipe[0].validationRecipes) -notcontains "default") {
        Write-Error "engine manifest 2d-playable-source-tree recipe must include the default validation recipe"
    }
    foreach ($claim in @("visible desktop package proof", "texture atlas cook", "tilemap editor UX", "runtime image decoding", "production sprite batching", "native GPU output", "public native or RHI handle access")) {
        if (-not ((@($sourceTree2dRecipe[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest 2d-playable-source-tree recipe must keep unsupported claim explicit: $claim"
        }
    }
}
if (@($productionLoop.recipes | Where-Object { $_.id -eq "future-2d-playable-vertical-slice" }).Count -ne 0) {
    Write-Error "engine manifest aiOperableProductionLoop must not keep stale future-2d-playable-vertical-slice after the source-tree 2D recipe is implemented"
}
$desktop2dRecipe = @($productionLoop.recipes | Where-Object { $_.id -eq "2d-desktop-runtime-package" })
if ($desktop2dRecipe.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose exactly one 2d-desktop-runtime-package recipe"
} else {
    foreach ($module in @("MK_core", "MK_platform", "MK_platform_sdl3", "MK_runtime", "MK_runtime_scene", "MK_runtime_host", "MK_runtime_host_sdl3", "MK_runtime_host_sdl3_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_audio", "MK_renderer")) {
        if (@($desktop2dRecipe[0].requiredModules) -notcontains $module) {
            Write-Error "engine manifest 2d-desktop-runtime-package recipe missing required module: $module"
        }
    }
    if (@($desktop2dRecipe[0].allowedTemplates) -notcontains "DesktopRuntime2DPackage") {
        Write-Error "engine manifest 2d-desktop-runtime-package recipe must allow DesktopRuntime2DPackage"
    }
    foreach ($target in @("desktop-game-runtime", "desktop-runtime-release")) {
        if (@($desktop2dRecipe[0].allowedPackagingTargets) -notcontains $target) {
            Write-Error "engine manifest 2d-desktop-runtime-package recipe must allow $target"
        }
    }
    foreach ($validationRecipe in @("desktop-game-runtime", "desktop-runtime-2d-package-proof", "desktop-runtime-2d-vulkan-window-package", "shader-toolchain")) {
        if (@($desktop2dRecipe[0].validationRecipes) -notcontains $validationRecipe) {
            Write-Error "engine manifest 2d-desktop-runtime-package recipe must include $validationRecipe validation"
        }
    }
    foreach ($claim in @("texture atlas cook", "tilemap editor UX", "runtime image decoding", "production sprite batching", "package streaming", "3D playable vertical slice", "editor productization", "public native or RHI handle access", "Metal readiness", "general production renderer quality")) {
        if (-not ((@($desktop2dRecipe[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest 2d-desktop-runtime-package recipe must keep unsupported claim explicit: $claim"
        }
    }
}
$desktop3dRecipe = @($productionLoop.recipes | Where-Object { $_.id -eq "3d-playable-desktop-package" })
if ($desktop3dRecipe.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose exactly one 3d-playable-desktop-package recipe"
} else {
    foreach ($module in @("MK_core", "MK_math", "MK_platform", "MK_platform_sdl3", "MK_runtime", "MK_runtime_rhi", "MK_runtime_scene", "MK_runtime_scene_rhi", "MK_runtime_host", "MK_runtime_host_sdl3", "MK_runtime_host_sdl3_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_renderer")) {
        if (@($desktop3dRecipe[0].requiredModules) -notcontains $module) {
            Write-Error "engine manifest 3d-playable-desktop-package recipe missing required module: $module"
        }
    }
    if (@($desktop3dRecipe[0].allowedTemplates) -notcontains "DesktopRuntime3DPackage") {
        Write-Error "engine manifest 3d-playable-desktop-package recipe must allow DesktopRuntime3DPackage"
    }
    foreach ($target in @("desktop-game-runtime", "desktop-runtime-release")) {
        if (@($desktop3dRecipe[0].allowedPackagingTargets) -notcontains $target) {
            Write-Error "engine manifest 3d-playable-desktop-package recipe must allow $target"
        }
    }
    Assert-ContainsText ([string]$desktop3dRecipe[0].summary) "--require-shadow-morph-composition" "engine manifest 3d-playable-desktop-package summary"
    Assert-ContainsText ([string]$desktop3dRecipe[0].cookedRuntimeAssumptions) "--require-shadow-morph-composition" "engine manifest 3d-playable-desktop-package cooked runtime assumptions"
    Assert-ContainsText ([string]$desktop3dRecipe[0].cookedRuntimeAssumptions) "renderer_morph_descriptor_binds" "engine manifest 3d-playable-desktop-package cooked runtime assumptions"
    Assert-ContainsText ([string]$desktop3dRecipe[0].rendererBackendAssumptions.d3d12) "selected generated graphics morph + directional shadow receiver" "engine manifest 3d-playable-desktop-package d3d12 backend assumptions"
    Assert-ContainsText ([string]$desktop3dRecipe[0].rendererBackendAssumptions.vulkan) "no Vulkan shadow-morph validation recipe is ready" "engine manifest 3d-playable-desktop-package vulkan backend assumptions"
    foreach ($validationRecipe in @("desktop-game-runtime", "desktop-runtime-sample-game-scene-gpu-package", "desktop-runtime-sample-game-vulkan-scene-gpu-package", "installed-d3d12-3d-shadow-morph-composition-smoke", "shader-toolchain")) {
        if (@($desktop3dRecipe[0].validationRecipes) -notcontains $validationRecipe) {
            Write-Error "engine manifest 3d-playable-desktop-package recipe must include $validationRecipe validation"
        }
    }
    foreach ($claim in @("runtime source asset parsing", "material graph", "shader graph", "skeletal animation production path", "GPU skinning", "package streaming", "broad shadow+morph composition beyond the selected receiver smoke", "compute morph + shadow composition", "morph-deformed shadow-caster silhouettes", "Metal ready", "public native or RHI handle access", "general production renderer quality")) {
        if (-not ((@($desktop3dRecipe[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest 3d-playable-desktop-package recipe must keep unsupported claim explicit: $claim"
        }
    }
    if (((@($desktop3dRecipe[0].unsupportedClaims) -join " ").Contains("native GPU HUD or sprite overlay output"))) {
        Write-Error "engine manifest 3d-playable-desktop-package recipe must not keep stale native GPU HUD or sprite overlay unsupported claim after the focused overlay recipe is added"
    }
}
$nativeOverlayRecipe = @($productionLoop.recipes | Where-Object { $_.id -eq "native-gpu-runtime-ui-overlay" })
if ($nativeOverlayRecipe.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose exactly one native-gpu-runtime-ui-overlay recipe"
} else {
    foreach ($module in @("MK_core", "MK_math", "MK_platform", "MK_platform_sdl3", "MK_runtime", "MK_runtime_rhi", "MK_runtime_scene", "MK_runtime_scene_rhi", "MK_runtime_host", "MK_runtime_host_sdl3", "MK_runtime_host_sdl3_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_renderer")) {
        if (@($nativeOverlayRecipe[0].requiredModules) -notcontains $module) {
            Write-Error "engine manifest native-gpu-runtime-ui-overlay recipe missing required module: $module"
        }
    }
    foreach ($validationRecipe in @("desktop-game-runtime", "desktop-runtime-sample-game-native-ui-overlay-package", "desktop-runtime-sample-game-vulkan-native-ui-overlay-package", "shader-toolchain")) {
        if (@($nativeOverlayRecipe[0].validationRecipes) -notcontains $validationRecipe) {
            Write-Error "engine manifest native-gpu-runtime-ui-overlay recipe must include $validationRecipe validation"
        }
    }
    foreach ($claim in @("production text shaping", "font rasterization", "glyph atlas", "image decoding", "real texture atlas", "IME", "OS accessibility bridge", "Metal ready", "public native or RHI handle access", "general production renderer quality")) {
        if (-not ((@($nativeOverlayRecipe[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest native-gpu-runtime-ui-overlay recipe must keep unsupported claim explicit: $claim"
        }
    }
}
$texturedUiRecipe = @($productionLoop.recipes | Where-Object { $_.id -eq "native-ui-textured-sprite-atlas" })
if ($texturedUiRecipe.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose exactly one native-ui-textured-sprite-atlas recipe"
} else {
    foreach ($module in @("MK_core", "MK_math", "MK_platform", "MK_platform_sdl3", "MK_runtime", "MK_runtime_rhi", "MK_runtime_scene", "MK_runtime_scene_rhi", "MK_runtime_host", "MK_runtime_host_sdl3", "MK_runtime_host_sdl3_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_renderer")) {
        if (@($texturedUiRecipe[0].requiredModules) -notcontains $module) {
            Write-Error "engine manifest native-ui-textured-sprite-atlas recipe missing required module: $module"
        }
    }
    foreach ($validationRecipe in @("desktop-game-runtime", "desktop-runtime-sample-game-textured-ui-atlas-package", "desktop-runtime-sample-game-vulkan-textured-ui-atlas-package", "shader-toolchain")) {
        if (@($texturedUiRecipe[0].validationRecipes) -notcontains $validationRecipe) {
            Write-Error "engine manifest native-ui-textured-sprite-atlas recipe must include $validationRecipe validation"
        }
    }
    foreach ($claim in @("source image decoding", "production atlas packing", "production text shaping", "font rasterization", "glyph atlas", "IME", "OS accessibility bridge", "Metal ready", "public native or RHI handle access", "general production UI renderer quality")) {
        if (-not ((@($texturedUiRecipe[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest native-ui-textured-sprite-atlas recipe must keep unsupported claim explicit: $claim"
        }
    }
}
$uiAtlasMetadataRecipe = @($productionLoop.recipes | Where-Object { $_.id -eq "native-ui-atlas-package-metadata" })
if ($uiAtlasMetadataRecipe.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose exactly one native-ui-atlas-package-metadata recipe"
} else {
    foreach ($module in @("MK_core", "MK_math", "MK_platform", "MK_platform_sdl3", "MK_runtime", "MK_runtime_rhi", "MK_runtime_scene", "MK_runtime_scene_rhi", "MK_runtime_host", "MK_runtime_host_sdl3", "MK_runtime_host_sdl3_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_renderer")) {
        if (@($uiAtlasMetadataRecipe[0].requiredModules) -notcontains $module) {
            Write-Error "engine manifest native-ui-atlas-package-metadata recipe missing required module: $module"
        }
    }
    foreach ($validationRecipe in @("desktop-game-runtime", "desktop-runtime-sample-game-ui-atlas-metadata-package", "desktop-runtime-sample-game-vulkan-ui-atlas-metadata-package", "shader-toolchain")) {
        if (@($uiAtlasMetadataRecipe[0].validationRecipes) -notcontains $validationRecipe) {
            Write-Error "engine manifest native-ui-atlas-package-metadata recipe must include $validationRecipe validation"
        }
    }
    foreach ($claim in @("runtime source PNG/JPEG image decoding", "production atlas packing", "production text shaping", "font rasterization", "glyph atlas", "IME", "OS accessibility bridge", "Metal ready", "public native or RHI handle access", "general production UI renderer quality")) {
        if (-not ((@($uiAtlasMetadataRecipe[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest native-ui-atlas-package-metadata recipe must keep unsupported claim explicit: $claim"
        }
    }
}

$expectedCommandSurfaceIds = @(
    "create-game-recipe",
    "create-scene",
    "update-scene-package",
    "migrate-scene-v2-runtime-package",
    "validate-runtime-scene-package",
    "add-scene-node",
    "add-or-update-component",
    "create-prefab",
    "instantiate-prefab",
    "create-material-instance",
    "create-material-from-graph",
    "register-source-asset",
    "cook-registered-source-assets",
    "cook-runtime-package",
    "register-runtime-package-files",
    "update-ui-atlas-metadata-package",
    "update-game-agent-manifest",
    "run-validation-recipe"
)
$knownAuthoringSurfaceIds = @{}
foreach ($authoringSurface in $productionLoop.authoringSurfaces) {
    $knownAuthoringSurfaceIds[$authoringSurface.id] = $true
}
$knownPackageSurfaceIds = @{}
foreach ($packageSurface in $productionLoop.packageSurfaces) {
    $knownPackageSurfaceIds[$packageSurface.id] = $true
}
$knownUnsupportedGapIds = @{}
foreach ($gap in $productionLoop.unsupportedProductionGaps) {
    $knownUnsupportedGapIds[$gap.id] = $true
}
$knownHostGateIds = @{}
foreach ($hostGate in $productionLoop.hostGates) {
    $knownHostGateIds[$hostGate.id] = $true
}
$commandSurfaceIds = @{}
foreach ($commandSurface in $productionLoop.commandSurfaces) {
    Assert-Properties $commandSurface @("id", "schemaVersion", "status", "owner", "summary", "requestModes", "requestShape", "resultShape", "requiredModules", "capabilityGates", "hostGates", "validationRecipes", "unsupportedGapIds", "undoToken", "notes") "engine manifest aiOperableProductionLoop commandSurfaces"
    Assert-ManifestCommandSurfaceHasNoLegacyTopLevelFields -CommandSurface $commandSurface -MessagePrefix "engine manifest aiOperableProductionLoop command surface"
    if ($commandSurface.schemaVersion -ne 1) {
        Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' schemaVersion must be 1"
    }
    if ($commandSurfaceIds.ContainsKey($commandSurface.id)) {
        Write-Error "engine manifest aiOperableProductionLoop command surface id is duplicated: $($commandSurface.id)"
    }
    $commandSurfaceIds[$commandSurface.id] = $true
    if ($allowedProductionStatuses -notcontains $commandSurface.status) {
        Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' has invalid status: $($commandSurface.status)"
    }
    $modeIds = @{}
    foreach ($mode in @($commandSurface.requestModes)) {
        Assert-Properties $mode @("id", "status", "mutates", "requiresDryRun", "notes") "engine manifest aiOperableProductionLoop command surface requestModes"
        $modeIds[$mode.id] = $mode
        if (@("dry-run", "apply", "execute") -notcontains $mode.id) {
            Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' has unknown request mode: $($mode.id)"
        }
        if ($allowedProductionStatuses -notcontains $mode.status) {
            Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' mode '$($mode.id)' has invalid status: $($mode.status)"
        }
    }
    if (-not $modeIds.ContainsKey("dry-run")) {
        Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' must expose a dry-run request mode"
    }
    if ($modeIds.ContainsKey("apply") -and $modeIds["apply"].status -eq "ready" -and $modeIds["dry-run"].status -ne "ready") {
        Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' cannot make apply ready before dry-run is ready"
    }
    if ($modeIds.ContainsKey("execute") -and $modeIds["execute"].status -eq "ready" -and $modeIds["dry-run"].status -ne "ready") {
        Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' cannot make execute ready before dry-run is ready"
    }
    if ($modeIds.ContainsKey("apply") -and $modeIds["apply"].status -eq "ready" -and
        @("register-runtime-package-files", "update-ui-atlas-metadata-package", "create-material-instance", "create-material-from-graph", "update-scene-package", "migrate-scene-v2-runtime-package", "create-scene", "add-scene-node", "add-or-update-component", "create-prefab", "instantiate-prefab", "register-source-asset", "cook-registered-source-assets") -notcontains $commandSurface.id) {
        Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' cannot make apply ready without a focused apply tooling slice"
    }
    if ($modeIds.ContainsKey("execute") -and $modeIds["execute"].status -eq "ready" -and
        @("run-validation-recipe", "validate-runtime-scene-package") -notcontains $commandSurface.id) {
        Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' cannot make execute ready without a focused execution tooling slice"
    }
    Assert-Properties $commandSurface.requestShape @("schema", "requiredFields", "optionalFields", "pathPolicy", "nativeHandlePolicy") "engine manifest aiOperableProductionLoop command surface requestShape"
    if ($commandSurface.requestShape.nativeHandlePolicy -ne "forbidden") {
        Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' requestShape must forbid native handles"
    }
    Assert-Properties $commandSurface.resultShape @("schema", "requiredFields", "diagnosticFields", "dryRunFields") "engine manifest aiOperableProductionLoop command surface resultShape"
    if (@("run-validation-recipe", "validate-runtime-scene-package") -contains $commandSurface.id) {
        Assert-Properties $commandSurface.resultShape @("executeFields") "engine manifest aiOperableProductionLoop run-validation-recipe resultShape"
    } else {
        Assert-Properties $commandSurface.resultShape @("applyFields") "engine manifest aiOperableProductionLoop command surface resultShape"
    }
    foreach ($requiredResultField in @("commandId", "mode", "status", "diagnostics", "validationRecipes", "unsupportedGapIds", "undoToken")) {
        if (@($commandSurface.resultShape.requiredFields) -notcontains $requiredResultField) {
            Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' resultShape missing required result field: $requiredResultField"
        }
    }
    foreach ($diagnosticField in @("severity", "code", "message", "unsupportedGapId", "validationRecipe")) {
        if (@($commandSurface.resultShape.diagnosticFields) -notcontains $diagnosticField) {
            Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' resultShape missing diagnostic field: $diagnosticField"
        }
    }
    foreach ($module in @($commandSurface.requiredModules)) {
        if (-not $moduleNames.ContainsKey($module)) {
            Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown module: $module"
        }
    }
    foreach ($gate in @($commandSurface.capabilityGates)) {
        Assert-Properties $gate @("id", "source", "requiredStatus", "notes") "engine manifest aiOperableProductionLoop command surface capabilityGates"
        switch ($gate.source) {
            "module" { if (-not $moduleNames.ContainsKey($gate.id)) { Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown module capability gate: $($gate.id)" } }
            "recipe" { if (-not $productionRecipeIds.ContainsKey($gate.id)) { Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown recipe capability gate: $($gate.id)" } }
            "authoring-surface" { if (-not $knownAuthoringSurfaceIds.ContainsKey($gate.id)) { Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown authoring surface capability gate: $($gate.id)" } }
            "package-surface" { if (-not $knownPackageSurfaceIds.ContainsKey($gate.id)) { Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown package surface capability gate: $($gate.id)" } }
            "unsupported-gap" { if (-not $knownUnsupportedGapIds.ContainsKey($gate.id)) { Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown unsupported gap capability gate: $($gate.id)" } }
            "host-gate" { if (-not $knownHostGateIds.ContainsKey($gate.id)) { Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown host gate capability gate: $($gate.id)" } }
            "validation-recipe" { if (-not $validationRecipeNames.ContainsKey($gate.id)) { Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown validation recipe capability gate: $($gate.id)" } }
            default { Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' has unknown capability gate source: $($gate.source)" }
        }
    }
    foreach ($hostGate in @($commandSurface.hostGates)) {
        if (-not $knownHostGateIds.ContainsKey($hostGate)) {
            Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown host gate: $hostGate"
        }
    }
    foreach ($validationRecipe in @($commandSurface.validationRecipes)) {
        if (-not $validationRecipeNames.ContainsKey($validationRecipe)) {
            Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown validation recipe: $validationRecipe"
        }
    }
    foreach ($gapId in @($commandSurface.unsupportedGapIds)) {
        if (-not $knownUnsupportedGapIds.ContainsKey($gapId)) {
            Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown unsupported gap: $gapId"
        }
    }
    if (@($commandSurface.unsupportedGapIds).Count -lt 1) {
        Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' must list unsupportedGapIds for diagnostics"
    }
    Assert-Properties $commandSurface.undoToken @("status", "notes") "engine manifest aiOperableProductionLoop command surface undoToken"
    if ($commandSurface.undoToken.status -ne "placeholder-only") {
        Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' undoToken must remain placeholder-only in this slice"
    }
    if ($modeIds.ContainsKey("apply") -and $modeIds["apply"].status -eq "ready" -and @($commandSurface.validationRecipes).Count -lt 1) {
        Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' must list validation recipes when apply=true"
    }
}
$runtimePackageCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "register-runtime-package-files" })
if ($runtimePackageCommand.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose exactly one register-runtime-package-files command surface"
} else {
    $runtimeModes = @{}
    foreach ($mode in @($runtimePackageCommand[0].requestModes)) {
        $runtimeModes[$mode.id] = $mode
    }
    if (-not $runtimeModes.ContainsKey("dry-run") -or $runtimeModes["dry-run"].status -ne "ready" -or
        -not $runtimeModes.ContainsKey("apply") -or $runtimeModes["apply"].status -ne "ready") {
        Write-Error "engine manifest aiOperableProductionLoop register-runtime-package-files must keep dry-run and apply ready"
    }
    if (-not ([string]$runtimeModes["dry-run"].notes).Contains("-DryRun")) {
        Write-Error "engine manifest aiOperableProductionLoop register-runtime-package-files dry-run notes must reference the actual -DryRun switch"
    }
}
$uiAtlasPackageCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "update-ui-atlas-metadata-package" })
if ($uiAtlasPackageCommand.Count -ne 1 -or $uiAtlasPackageCommand[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop must expose one ready update-ui-atlas-metadata-package command surface"
} else {
    $uiAtlasModes = @{}
    foreach ($mode in @($uiAtlasPackageCommand[0].requestModes)) {
        $uiAtlasModes[$mode.id] = $mode
    }
    if (-not $uiAtlasModes.ContainsKey("dry-run") -or $uiAtlasModes["dry-run"].status -ne "ready" -or
        -not $uiAtlasModes.ContainsKey("apply") -or $uiAtlasModes["apply"].status -ne "ready") {
        Write-Error "engine manifest update-ui-atlas-metadata-package must keep dry-run and apply ready"
    }
    $uiAtlasNotes = [string]$uiAtlasPackageCommand[0].notes
    if (-not $uiAtlasNotes.Contains("plan_cooked_ui_atlas_package_update") -or
        -not $uiAtlasNotes.Contains("apply_cooked_ui_atlas_package_update") -or
        -not $uiAtlasNotes.Contains("author_packed_ui_atlas_from_decoded_images") -or
        -not $uiAtlasNotes.Contains("author_packed_ui_glyph_atlas_from_rasterized_glyphs") -or
        -not $uiAtlasNotes.Contains("plan_packed_ui_glyph_atlas_package_update") -or
        -not $uiAtlasNotes.Contains("GameEngine.CookedTexture.v1") -or
        -not $uiAtlasNotes.Contains("renderer texture upload")) {
        Write-Error "engine manifest update-ui-atlas-metadata-package notes must keep cooked helper names, decoded/glyph atlas bridges, and renderer-upload limits explicit"
    }
}
$materialInstanceCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "create-material-instance" })
if ($materialInstanceCommand.Count -ne 1 -or $materialInstanceCommand[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop must expose one ready create-material-instance command surface"
} else {
    $materialModes = @{}
    foreach ($mode in @($materialInstanceCommand[0].requestModes)) {
        $materialModes[$mode.id] = $mode
    }
    if (-not $materialModes.ContainsKey("dry-run") -or $materialModes["dry-run"].status -ne "ready" -or
        -not $materialModes.ContainsKey("apply") -or $materialModes["apply"].status -ne "ready") {
        Write-Error "engine manifest create-material-instance must keep dry-run and apply ready"
    }
    $materialNotes = [string]$materialInstanceCommand[0].notes
    if (-not $materialNotes.Contains("plan_material_instance_package_update") -or
        -not $materialNotes.Contains("apply_material_instance_package_update") -or
        -not $materialNotes.Contains("material graph") -or
        -not $materialNotes.Contains("shader graph") -or
        -not $materialNotes.Contains("live shader generation")) {
        Write-Error "engine manifest create-material-instance notes must keep helper names and unsupported graph/shader limits explicit"
    }
}
$materialGraphCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "create-material-from-graph" })
if ($materialGraphCommand.Count -ne 1 -or $materialGraphCommand[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop must expose one ready create-material-from-graph command surface"
} else {
    $materialGraphModes = @{}
    foreach ($mode in @($materialGraphCommand[0].requestModes)) {
        $materialGraphModes[$mode.id] = $mode
    }
    if (-not $materialGraphModes.ContainsKey("dry-run") -or $materialGraphModes["dry-run"].status -ne "ready" -or
        -not $materialGraphModes.ContainsKey("apply") -or $materialGraphModes["apply"].status -ne "ready") {
        Write-Error "engine manifest create-material-from-graph must keep dry-run and apply ready"
    }
    $materialGraphNotes = [string]$materialGraphCommand[0].notes
    foreach ($needle in @(
            "plan_material_graph_package_update",
            "apply_material_graph_package_update",
            "GameEngine.MaterialGraph.v1",
            "GameEngine.Material.v1",
            "material_texture",
            "shader graph",
            "shader compiler execution",
            "live shader generation",
            "renderer/RHI residency",
            "package streaming"
        )) {
        if (-not $materialGraphNotes.Contains($needle)) {
            Write-Error "engine manifest create-material-from-graph notes must keep helper names and unsupported graph/package limits explicit: $needle"
        }
    }
}
$scenePackageCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "update-scene-package" })
if ($scenePackageCommand.Count -ne 1 -or $scenePackageCommand[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop must expose one ready update-scene-package command surface"
} else {
    $sceneModes = @{}
    foreach ($mode in @($scenePackageCommand[0].requestModes)) {
        $sceneModes[$mode.id] = $mode
    }
    if (-not $sceneModes.ContainsKey("dry-run") -or $sceneModes["dry-run"].status -ne "ready" -or
        -not $sceneModes.ContainsKey("apply") -or $sceneModes["apply"].status -ne "ready") {
        Write-Error "engine manifest update-scene-package must keep dry-run and apply ready"
    }
    $sceneNotes = [string]$scenePackageCommand[0].notes
    foreach ($needle in @(
            "plan_scene_package_update",
            "apply_scene_package_update",
            "scene_mesh",
            "scene_material",
            "scene_sprite",
            "editor productization",
            "prefab mutation",
            "runtime source import",
            "renderer/RHI residency",
            "package streaming",
            "material graph",
            "shader graph",
            "live shader generation"
        )) {
        if (-not $sceneNotes.Contains($needle)) {
            Write-Error "engine manifest update-scene-package notes must keep helper names and unsupported scene/package limits explicit: $needle"
        }
    }
}
$scenePrefabAuthoringCommandIds = @(
    "create-scene",
    "add-scene-node",
    "add-or-update-component",
    "create-prefab",
    "instantiate-prefab"
)
