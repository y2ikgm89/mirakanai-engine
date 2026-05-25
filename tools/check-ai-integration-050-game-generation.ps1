#requires -Version 7.0
#requires -PSEdition Core

# Chapter 5 for check-ai-integration.ps1 static contracts.

$engineCapabilityHandoffChecks = @(
    @{
        Path = "engine/tools/include/mirakana/tools/gameplay_authoring_tool.hpp"
        Needles = @(
            "EngineCapabilityHandoffRequestRow",
            "EngineCapabilityHandoffReviewRequest",
            "EngineCapabilityHandoffReviewResult",
            "review_engine_capability_handoff_request"
        )
    },
    @{
        Path = "engine/tools/asset/gameplay_authoring_tool.cpp"
        Needles = @(
            "unsupported_capability_id",
            "missing_current_workaround",
            "unsafe_public_contract",
            "developer-owned-engine-feature",
            "create-dated-capability-plan"
        )
    },
    @{
        Path = "tests/unit/tools_tests.cpp"
        Needles = @(
            "engine capability handoff review accepts canonical developer handoff rows",
            "engine capability handoff review fails closed on incomplete invented and native handoffs",
            "EngineCapabilityHandoffRequestRow"
        )
    },
    @{
        Path = "schemas/game-agent.schema.json"
        Needles = @(
            "engineCapabilityHandoffs",
            "requestedCapabilityId",
            "blockedFeatureId",
            "currentWorkaround",
            "desiredPublicContract",
            "requiredEvidenceIds"
        )
    },
    @{
        Path = "docs/ai-game-development.md"
        Needles = @(
            "Engine Capability Handoffs",
            "EngineCapabilityHandoffRequestRow",
            "review_engine_capability_handoff_request",
            "game.agent.json.aiWorkflow.engineCapabilityHandoffs"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "AI Engine Capability Handoff v1",
            "EngineCapabilityHandoffReviewRequest",
            "native handles, SDL3, Dear ImGui, renderer/RHI, backend"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md"
        Needles = @(
            "ai-engine-capability-handoff-v1",
            "implemented-1x-foundation",
            "game.agent.json.aiWorkflow.engineCapabilityHandoffs"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "implemented-ai-engine-capability-handoff",
            "review_engine_capability_handoff_request",
            "EngineCapabilityHandoffRequestRow",
            "aiWorkflow.engineCapabilityHandoffs"
        )
    }
)
foreach ($check in $engineCapabilityHandoffChecks) {
    $engineCapabilityHandoffText = Get-AgentSurfaceText $check.Path
    foreach ($needle in $check.Needles) {
        Assert-ContainsText $engineCapabilityHandoffText $needle $check.Path
    }
}

$aiGameDesignSpecChecks = @(
    @{
        Path = "schemas/game-agent.schema.json"
        Needles = @(
            "gameDesignSpec",
            "ai-game-design-spec-v1",
            "gameplayFamily",
            "qualityGates",
            "unsupportedClaims"
        )
    },
    @{
        Path = "tools/check-json-contracts-060-game-design-spec.ps1"
        Needles = @(
            "Assert-JsonGameDesignSpec",
            "aiWorkflow.gameDesignSpec",
            "packageTargets references undeclared packaging target",
            "validationRecipeIds references undeclared validation recipe",
            "unsupportedClaims missing",
            "native-handles"
        )
    },
    @{
        Path = "tools/new-game-templates.ps1"
        Needles = @(
            "New-DesktopRuntime2DGameDesignSpec",
            "New-DesktopRuntime3DGameDesignSpec",
            "gameDesignSpec = New-DesktopRuntime2DGameDesignSpec",
            "gameDesignSpec = New-DesktopRuntime3DGameDesignSpec"
        )
    },
    @{
        Path = "games/sample_2d_desktop_runtime_package/game.agent.json"
        Needles = @(
            '"capabilityId": "ai-game-design-spec-v1"',
            '"gameplayFamily": "2d-desktop-runtime-package"',
            '"qualityGates"',
            '"engine-internal-edits"'
        )
    },
    @{
        Path = "games/sample_generated_desktop_runtime_3d_package/game.agent.json"
        Needles = @(
            '"capabilityId": "ai-game-design-spec-v1"',
            '"gameplayFamily": "3d-playable-desktop-package"',
            '"systems"',
            '"unreviewed-external-assets"'
        )
    },
    @{
        Path = "docs/ai-game-development.md"
        Needles = @(
            "game.agent.json.aiWorkflow.gameDesignSpec",
            "ai-game-design-spec-v1",
            "gameplay family, template, camera, input map, core loop",
            "same-manifest declarations"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "AI Game Design Spec v1",
            "game.agent.json.aiWorkflow.gameDesignSpec",
            "DesktopRuntime2DPackage",
            "DesktopRuntime3DPackage"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md"
        Needles = @(
            "ai-game-design-spec-v1",
            "implemented-1x-foundation",
            "game.agent.json.aiWorkflow.gameDesignSpec"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "AI Game Design Spec v1",
            "aiWorkflow.gameDesignSpec",
            "gameplay family, template, camera, input map",
            "same-manifest package targets and validation recipes"
        )
    },
    @{
        Path = ".agents/skills/gameengine-game-development/SKILL.md"
        Needles = @(
            "game.agent.json.aiWorkflow.gameDesignSpec",
            "validation recipe ids",
            "same-manifest package targets"
        )
    },
    @{
        Path = ".claude/skills/gameengine-game-development/SKILL.md"
        Needles = @(
            "game.agent.json.aiWorkflow.gameDesignSpec",
            "validation recipe ids",
            "same-manifest package targets"
        )
    },
    @{
        Path = ".cursor/skills/gameengine-game-development/SKILL.md"
        Needles = @(
            "game.agent.json.aiWorkflow.gameDesignSpec",
            "validation recipe ids",
            "same-manifest package targets"
        )
    }
)
foreach ($check in $aiGameDesignSpecChecks) {
    $gameDesignSpecText = Get-AgentSurfaceText $check.Path
    foreach ($needle in $check.Needles) {
        Assert-ContainsText $gameDesignSpecText $needle $check.Path
    }
}

$aiGameGenerationOrchestratorChecks = @(
    @{
        Path = "tools/create-game-recipe.ps1"
        Needles = @(
            "[CmdletBinding(SupportsShouldProcess = `$true)]",
            "GameEngine.AiCommand.CreateGameRecipe.Result",
            "tools/new-game.ps1",
            "aiWorkflow.gameDesignSpec",
            "DesktopRuntime2DPackage",
            "DesktopRuntime3DPackage",
            "arbitrary-shell"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            '"id": "create-game-recipe"',
            '"status": "ready"',
            "tools/create-game-recipe.ps1",
            "tools/new-game.ps1",
            "plannedFiles",
            "changedFiles",
            "external asset generation"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "AI Game Generation Orchestrator v1",
            "tools/create-game-recipe.ps1",
            "plannedFiles",
            "changedFiles",
            "external asset generation"
        )
    },
    @{
        Path = "docs/ai-game-development.md"
        Needles = @(
            "tools/create-game-recipe.ps1 -Mode DryRun",
            "DesktopRuntime2DPackage",
            "DesktopRuntime3DPackage",
            "does not execute arbitrary shell text"
        )
    },
    @{
        Path = "docs/roadmap.md"
        Needles = @(
            "AI Game Generation Orchestrator v1",
            "reviewed dry-run/apply wrapper",
            "no arbitrary shell execution"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md"
        Needles = @(
            "ai-game-generation-orchestrator-v1",
            "implemented-1x-foundation",
            "tools/create-game-recipe.ps1"
        )
    },
    @{
        Path = ".agents/skills/gameengine-game-development/SKILL.md"
        Needles = @(
            "tools/create-game-recipe.ps1",
            "plannedFiles",
            "arbitrary shell text"
        )
    },
    @{
        Path = ".claude/skills/gameengine-game-development/SKILL.md"
        Needles = @(
            "tools/create-game-recipe.ps1",
            "plannedFiles",
            "arbitrary shell text"
        )
    },
    @{
        Path = ".cursor/skills/gameengine-game-development/SKILL.md"
        Needles = @(
            "tools/create-game-recipe.ps1",
            "plannedFiles",
            "arbitrary shell text"
        )
    }
)
foreach ($check in $aiGameGenerationOrchestratorChecks) {
    $gameGenerationOrchestratorText = Get-AgentSurfaceText $check.Path
    foreach ($needle in $check.Needles) {
        Assert-ContainsText $gameGenerationOrchestratorText $needle $check.Path
    }
}

$aiSafeContentMutationLedgerChecks = @(
    @{
        Path = "schemas/game-agent.schema.json"
        Needles = @(
            "contentMutationLedger",
            "ai-safe-content-mutation-ledger-v1",
            "aiOwnedSourceRoots",
            "generatedFiles",
            "reviewedCommandSurfaces",
            "forbiddenSharedPaths",
            "remediationActions"
        )
    },
    @{
        Path = "tools/check-json-contracts-061-content-mutation-ledger.ps1"
        Needles = @(
            "Assert-JsonMutationLedger",
            "aiWorkflow.contentMutationLedger",
            "reviewedCommandSurfaces missing",
            "forbiddenSharedPaths missing",
            "remediationActions missing"
        )
    },
    @{
        Path = "tools/new-game-templates.ps1"
        Needles = @(
            "New-AiContentMutationLedger",
            "contentMutationLedger = New-AiContentMutationLedger",
            "create-game-recipe",
            "register-runtime-package-files",
            "engine-capability-handoff"
        )
    },
    @{
        Path = "games/sample_2d_desktop_runtime_package/game.agent.json"
        Needles = @(
            '"capabilityId": "ai-safe-content-mutation-ledger-v1"',
            '"ledgerId": "sample-2d-desktop-runtime-package-ai-mutation-ledger"',
            '"path": "engine"',
            '"id": "missing-package-file"'
        )
    },
    @{
        Path = "games/sample_generated_desktop_runtime_3d_package/game.agent.json"
        Needles = @(
            '"capabilityId": "ai-safe-content-mutation-ledger-v1"',
            '"ledgerId": "sample-generated-desktop-runtime-3d-package-ai-mutation-ledger"',
            '"path": "engine/agent"',
            '"id": "unsupported-engine-capability"'
        )
    },
    @{
        Path = "docs/ai-game-development.md"
        Needles = @(
            "Content Mutation Ledger",
            "game.agent.json.aiWorkflow.contentMutationLedger",
            "tools/register-runtime-package-files.ps1",
            "forbidden shared repository paths"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "AI Safe Content Mutation Ledger v1",
            "game.agent.json.aiWorkflow.contentMutationLedger",
            "forbidden shared repository paths"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md"
        Needles = @(
            "ai-safe-content-mutation-ledger-v1",
            "implemented-1x-foundation",
            "game.agent.json.aiWorkflow.contentMutationLedger"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "AI Safe Content Mutation Ledger v1",
            "aiWorkflow.contentMutationLedger",
            "forbidden shared paths"
        )
    },
    @{
        Path = ".agents/skills/gameengine-game-development/SKILL.md"
        Needles = @(
            "game.agent.json.aiWorkflow.contentMutationLedger",
            "register-runtime-package-files.ps1",
            "games/CMakeLists.txt"
        )
    },
    @{
        Path = ".claude/skills/gameengine-game-development/SKILL.md"
        Needles = @(
            "game.agent.json.aiWorkflow.contentMutationLedger",
            "register-runtime-package-files.ps1",
            "games/CMakeLists.txt"
        )
    },
    @{
        Path = ".cursor/skills/gameengine-game-development/SKILL.md"
        Needles = @(
            "game.agent.json.aiWorkflow.contentMutationLedger",
            "register-runtime-package-files.ps1",
            "games/CMakeLists.txt"
        )
    }
)
foreach ($check in $aiSafeContentMutationLedgerChecks) {
    $safeMutationLedgerText = Get-AgentSurfaceText $check.Path
    foreach ($needle in $check.Needles) {
        Assert-ContainsText $safeMutationLedgerText $needle $check.Path
    }
}

$aiPlaceholderAssetPipelineChecks = @(
    @{
        Path = "schemas/game-agent.schema.json"
        Needles = @(
            "placeholderAssetPipeline",
            "ai-placeholder-asset-pipeline-v1",
            "plannedAssets",
            "packageHandoff",
            "unsupportedClaims"
        )
    },
    @{
        Path = "tools/check-json-contracts-062-placeholder-asset-pipeline.ps1"
        Needles = @(
            "Assert-JsonPlaceholderPipeline",
            "aiWorkflow.placeholderAssetPipeline",
            "plan-placeholder-asset-cook-package",
            "external-asset-download",
            "broad-art-direction"
        )
    },
    @{
        Path = "tools/new-game-templates.ps1"
        Needles = @(
            "New-AiPlaceholderAssetPipeline",
            "placeholderAssetPipeline = New-AiPlaceholderAssetPipeline",
            "plan-placeholder-asset-bundle",
            "plan-placeholder-asset-cook-package"
        )
    },
    @{
        Path = "games/sample_2d_desktop_runtime_package/game.agent.json"
        Needles = @(
            '"capabilityId": "ai-placeholder-asset-pipeline-v1"',
            '"player-sprite-placeholder"',
            '"jump-audio-placeholder"'
        )
    },
    @{
        Path = "games/sample_generated_desktop_runtime_3d_package/game.agent.json"
        Needles = @(
            '"capabilityId": "ai-placeholder-asset-pipeline-v1"',
            '"packaged-mesh-placeholder"',
            '"hud-atlas-placeholder"',
            '"scene-prop"'
        )
    },
    @{
        Path = "docs/ai-game-development.md"
        Needles = @(
            "Placeholder Asset Pipeline",
            "game.agent.json.aiWorkflow.placeholderAssetPipeline",
            "first-party placeholder source documents",
            "external asset downloads"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "AI Placeholder Asset Pipeline v1",
            "game.agent.json.aiWorkflow.placeholderAssetPipeline",
            "first-party placeholder assets"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md"
        Needles = @(
            "ai-placeholder-asset-pipeline-v1",
            "implemented-1x-foundation",
            "game.agent.json.aiWorkflow.placeholderAssetPipeline"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "AI Placeholder Asset Pipeline v1",
            "aiWorkflow.placeholderAssetPipeline",
            "first-party placeholder"
        )
    },
    @{
        Path = ".agents/skills/gameengine-game-development/SKILL.md"
        Needles = @(
            "game.agent.json.aiWorkflow.placeholderAssetPipeline",
            "first-party placeholder assets",
            "no external asset downloads"
        )
    },
    @{
        Path = ".claude/skills/gameengine-game-development/SKILL.md"
        Needles = @(
            "game.agent.json.aiWorkflow.placeholderAssetPipeline",
            "first-party placeholder assets",
            "no external asset downloads"
        )
    },
    @{
        Path = ".cursor/skills/gameengine-game-development/SKILL.md"
        Needles = @(
            "game.agent.json.aiWorkflow.placeholderAssetPipeline",
            "first-party placeholder assets",
            "no external asset downloads"
        )
    }
)
foreach ($check in $aiPlaceholderAssetPipelineChecks) {
    $placeholderPipelineText = Get-AgentSurfaceText $check.Path
    foreach ($needle in $check.Needles) {
        Assert-ContainsText $placeholderPipelineText $needle $check.Path
    }
}

$aiGeneratedGamePlaytestLoopChecks = @(
    @{
        Path = "schemas/game-agent.schema.json"
        Needles = @(
            "generatedGamePlaytestLoop",
            "ai-generated-game-playtest-loop-v1",
            "reviewedRecipeSurfaces",
            "failureClassifications",
            "remediationPolicy"
        )
    },
    @{
        Path = "tools/check-json-contracts-063-generated-game-playtest-loop.ps1"
        Needles = @(
            "Assert-JsonGeneratedGamePlaytestLoop",
            "aiWorkflow.generatedGamePlaytestLoop",
            "run-validation-recipe-execute",
            "validation-weakening",
            "counter-mismatch"
        )
    },
    @{
        Path = "tools/new-game-templates.ps1"
        Needles = @(
            "New-AiGeneratedGamePlaytestLoop",
            "generatedGamePlaytestLoop = New-AiGeneratedGamePlaytestLoop",
            "package-smoke-evidence-review",
            "mutation-ledger-remediation"
        )
    },
    @{
        Path = "games/sample_2d_desktop_runtime_package/game.agent.json"
        Needles = @(
            '"capabilityId": "ai-generated-game-playtest-loop-v1"',
            '"installed-2d-package-smoke-playtest"',
            '"counter-mismatch"'
        )
    },
    @{
        Path = "games/sample_generated_desktop_runtime_3d_package/game.agent.json"
        Needles = @(
            '"capabilityId": "ai-generated-game-playtest-loop-v1"',
            '"installed-d3d12-3d-package-smoke-playtest"',
            '"host-gated"'
        )
    },
    @{
        Path = "docs/ai-game-development.md"
        Needles = @(
            "Generated Game Playtest Loop",
            "game.agent.json.aiWorkflow.generatedGamePlaytestLoop",
            "failure classification",
            "no validation weakening"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "AI Generated Game Playtest Loop v1",
            "game.agent.json.aiWorkflow.generatedGamePlaytestLoop",
            "failure classification"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md"
        Needles = @(
            "ai-generated-game-playtest-loop-v1",
            "implemented-1x-foundation",
            "game.agent.json.aiWorkflow.generatedGamePlaytestLoop"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "AI Generated Game Playtest Loop v1",
            "aiWorkflow.generatedGamePlaytestLoop",
            "failure classification"
        )
    },
    @{
        Path = ".agents/skills/gameengine-game-development/SKILL.md"
        Needles = @(
            "game.agent.json.aiWorkflow.generatedGamePlaytestLoop",
            "failure classification",
            "no validation weakening"
        )
    },
    @{
        Path = ".claude/skills/gameengine-game-development/SKILL.md"
        Needles = @(
            "game.agent.json.aiWorkflow.generatedGamePlaytestLoop",
            "failure classification",
            "no validation weakening"
        )
    },
    @{
        Path = ".cursor/skills/gameengine-game-development/SKILL.md"
        Needles = @(
            "game.agent.json.aiWorkflow.generatedGamePlaytestLoop",
            "failure classification",
            "no validation weakening"
        )
    }
)
foreach ($check in $aiGeneratedGamePlaytestLoopChecks) {
    $playtestLoopText = Get-AgentSurfaceText $check.Path
    foreach ($needle in $check.Needles) {
        Assert-ContainsText $playtestLoopText $needle $check.Path
    }
}

$aiValidationRemediationRecipesChecks = @(
    @{ Path = "schemas/game-agent.schema.json"; Needles = @("validationRemediationRecipes", "ai-validation-remediation-recipes-v1", "failureClassificationId", "rerun-selected-validation-recipe") },
    @{ Path = "tools/check-json-contracts-064-validation-remediation-recipes.ps1"; Needles = @("Assert-JsonValidationRemediationRecipes", "aiWorkflow.validationRemediationRecipes", "failureClassificationId", "host-gate-bypass") },
    @{ Path = "tools/new-game-templates.ps1"; Needles = @("New-AiValidationRemediationRecipes", "validationRemediationRecipes = New-AiValidationRemediationRecipes", "rerun-selected-validation-recipe", "record-host-gate") },
    @{ Path = "games/sample_2d_desktop_runtime_package/game.agent.json"; Needles = @('"capabilityId": "ai-validation-remediation-recipes-v1"', '"missing-package-file-remediation"', '"host-gated-remediation"') },
    @{ Path = "games/sample_generated_desktop_runtime_3d_package/game.agent.json"; Needles = @('"capabilityId": "ai-validation-remediation-recipes-v1"', '"shader-tool-gap-remediation"', '"counter-mismatch-remediation"') },
    @{ Path = "docs/ai-game-development.md"; Needles = @("Validation Remediation Recipes", "game.agent.json.aiWorkflow.validationRemediationRecipes", "rerun selected validation recipes", "no validation weakening") },
    @{ Path = "docs/current-capabilities.md"; Needles = @("AI Validation Remediation Recipes v1", "game.agent.json.aiWorkflow.validationRemediationRecipes", "common generation failures") },
    @{ Path = "docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md"; Needles = @("ai-validation-remediation-recipes-v1", "implemented-1x-foundation", "game.agent.json.aiWorkflow.validationRemediationRecipes") },
    @{ Path = "engine/agent/manifest.json"; Needles = @("AI Validation Remediation Recipes v1", "aiWorkflow.validationRemediationRecipes", "common missing package file, invalid reference, host gate, shader/tool gap, counter mismatch, and runtime package load failures") },
    @{ Path = ".agents/skills/gameengine-game-development/SKILL.md"; Needles = @("game.agent.json.aiWorkflow.validationRemediationRecipes", "rerun selected validation recipes", "no validation weakening") },
    @{ Path = ".claude/skills/gameengine-game-development/SKILL.md"; Needles = @("game.agent.json.aiWorkflow.validationRemediationRecipes", "rerun selected validation recipes", "no validation weakening") },
    @{ Path = ".cursor/skills/gameengine-game-development/SKILL.md"; Needles = @("game.agent.json.aiWorkflow.validationRemediationRecipes", "rerun selected validation recipes", "no validation weakening") }
)
foreach ($check in $aiValidationRemediationRecipesChecks) {
    $remediationRecipesText = Get-AgentSurfaceText $check.Path
    foreach ($needle in $check.Needles) { Assert-ContainsText $remediationRecipesText $needle $check.Path }
}
$editorResourceCaptureExecutionChecks = @(
    @{
        Path = "editor/core/include/mirakana/editor/resource_panel.hpp"
        Needles = @(
            "EditorResourceCaptureExecutionInput",
            "EditorResourceCaptureExecutionRow",
            "capture_execution_snapshots",
            "capture_execution_rows"
        )
    },
    @{
        Path = "editor/core/src/resource_panel.cpp"
        Needles = @(
            "append_capture_execution",
            "resources.capture_execution",
            "resources.capture_execution.contract_label",
            "resources.capture_execution.operator_validated_launch_workflow_contract_label",
            "ge.editor.resources_capture_execution.v1",
            "ge.editor.resources_capture_operator_validated_launch_workflow.v1",
            "capture_execution_status_label",
            "capture_execution_phase_code",
            "bound_sanitized_field",
            "k_max_capture_execution_artifact_chars",
            "resource capture execution evidence blocked",
            "native handle exposure",
            "resources.capture_execution.pix_gpu_capture.host_helper_hint"
        )
    },
    @{
        Path = "editor/src/main.cpp"
        Needles = @(
            "append_resource_capture_execution_snapshots",
            "draw_resource_capture_execution_rows_table",
            "Resource Capture Execution Evidence",
            "execute_pix_host_helper_reviewed",
            "waiting for external host PIX capture evidence",
            "waiting for external host debug-layer/GPU-validation evidence"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor resource capture execution evidence reports host owned snapshots",
            "EditorResourceCaptureExecutionInput",
            "capture_execution_rows",
            "resources.capture_execution.pix_gpu_capture.phase",
            "resources.capture_execution.pix_gpu_capture.status",
            "resources.capture_execution.pix_gpu_capture.host_helper_hint",
            "resources.capture_execution.contract_label",
            "resources.capture_execution.operator_validated_launch_workflow_contract_label",
            "ge.editor.resources_capture_execution.v1",
            "ge.editor.resources_capture_operator_validated_launch_workflow.v1",
            "unsafe_native_capture"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Editor Resource Capture Execution Evidence v1",
            "EditorResourceCaptureExecutionInput",
            "EditorResourceCaptureExecutionRow",
            "resources.capture_execution",
            "resources.capture_execution.contract_label",
            "resources.capture_execution.operator_validated_launch_workflow_contract_label",
            "ge.editor.resources_capture_execution.v1",
            "ge.editor.resources_capture_operator_validated_launch_workflow.v1",
            "host-owned evidence reporting",
            "Run helper (-SkipLaunch)",
            "Run helper (launch PIX)"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Resource Capture Execution Evidence v1",
            "EditorResourceCaptureExecutionInput",
            "resources.capture_execution",
            "ge.editor.resources_capture_execution.v1",
            "host-owned capture execution evidence"
        )
    },
    @{
        Path = "docs/ai-game-development.md"
        Needles = @(
            "Resource Capture Execution Evidence v1",
            "EditorResourceCaptureExecutionInput",
            "resources.capture_execution",
            "actual capture execution remains a host/tool workflow"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "Resource Capture Execution Evidence v1 coverage",
            "EditorResourceCaptureExecutionRow",
            "blocked unsupported editor-core execution/native-handle claims"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "2026-05-07-editor-resource-capture-execution-evidence-v1.md",
            "Editor Resource Capture Execution Evidence v1",
            "resources.capture_execution"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
        Needles = @(
            "Editor Resource Capture Execution Evidence v1",
            "host-owned Resources capture execution evidence rows",
            "resource management/capture execution beyond host-owned evidence rows"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "currentEditorResourceCaptureRequests",
            "EditorResourceCaptureExecutionInput",
            "resources.capture_execution",
            "does not launch PIX automatically"
        )
    },
    @{
        Path = ".agents/skills/editor-change/SKILL.md"
        Needles = @(
            "host-owned capture execution evidence rows",
            "EditorResourceCaptureExecutionInput",
            "resources.capture_execution",
            "ge.editor.resources_capture_execution.v1",
            "ge.editor.resources_capture_operator_validated_launch_workflow.v1"
        )
    },
    @{
        Path = ".claude/skills/gameengine-editor/SKILL.md"
        Needles = @(
            "host-owned capture execution evidence rows",
            "EditorResourceCaptureExecutionInput",
            "resources.capture_execution",
            "ge.editor.resources_capture_execution.v1",
            "ge.editor.resources_capture_operator_validated_launch_workflow.v1"
        )
    }
)
foreach ($check in $editorResourceCaptureExecutionChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing editor resource capture execution evidence contract: $($missingNeedles -join ', ')"
    }
}

$editorAiCommandPanelChecks = @(
    @{
        Path = "editor/core/include/mirakana/editor/ai_command_panel.hpp"
        Needles = @(
            "EditorAiCommandPanelModel",
            "EditorAiCommandPanelDesc",
            "EditorAiCommandPanelStageRow",
            "EditorAiCommandPanelCommandRow",
            "EditorAiCommandPanelEvidenceRow",
            "make_editor_ai_command_panel_model",
            "make_ai_command_panel_ui_model"
        )
    },
    @{
        Path = "editor/core/src/ai_command_panel.cpp"
        Needles = @(
            "AI command panel input must be read-only",
            "must not execute validation recipes",
            "package scripts",
            "shell commands",
            "external_action_required",
            "ai_commands.commands"
        )
    },
    @{
        Path = "editor/core/include/mirakana/editor/workspace.hpp"
        Needles = @(
            "PanelId",
            "ai_commands"
        )
    },
    @{
        Path = "editor/core/src/workspace.cpp"
        Needles = @(
            'PanelToken{.id = PanelId::ai_commands, .token = "ai_commands"}',
            "PanelState{.id = PanelId::ai_commands, .visible = false}"
        )
    },
    @{
        Path = "editor/src/main.cpp"
        Needles = @(
            "make_ai_command_panel_context",
            "make_reviewed_validation_recipe_dry_run_rows",
            "draw_ai_commands_panel",
            "view.ai_commands",
            "draw_ai_command_rows_table",
            "make_editor_ai_command_panel_model"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor ai command panel summarizes workflow command and evidence rows",
            "editor ai command panel blocks mutating or executing workflow inputs",
            "panel.ai_commands=visible",
            "EditorAiCommandPanelModel",
            "make_ai_command_panel_ui_model"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Editor AI Command Diagnostics Panel v1",
            "EditorAiCommandPanelModel",
            "make_ai_command_panel_ui_model",
            "panel.ai_commands=hidden",
            "raw manifest command strings",
            "free-form manifest edits"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Editor AI Command Diagnostics Panel v1",
            "EditorAiCommandPanelModel",
            "AI Commands panel",
            "validation recipe execution",
            "raw manifest command evaluation"
        )
    },
    @{
        Path = "docs/ai-game-development.md"
        Needles = @(
            "Editor AI Command Diagnostics Panel v1",
            "EditorAiCommandPanelModel",
            "make_ai_command_panel_ui_model",
            "raw manifest command strings",
            "free-form manifest edits"
        )
    },
    @{
        Path = "docs/roadmap.md"
        Needles = @(
            "Editor AI Command Diagnostics Panel v1",
            "EditorAiCommandPanelModel",
            'workspace `ai_commands` panel',
            "dynamic game-module loading"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "Editor AI Command Diagnostics Panel v1",
            "EditorAiCommandPanelModel",
            "make_ai_command_panel_ui_model"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "2026-05-07-editor-ai-command-diagnostics-panel-v1.md",
            "Editor AI Command Diagnostics Panel v1",
            'workspace `ai_commands` panel state',
            "raw manifest command evaluation"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
        Needles = @(
            "Editor AI Command Diagnostics Panel v1",
            "EditorAiCommandPanelModel",
            "unacknowledged or automatic host-gated AI command execution",
            "raw manifest command evaluation"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "Editor AI Command Diagnostics Panel v1",
            "EditorAiCommandPanelModel",
            "workspace ai_commands panel state",
            "read-only AI Commands diagnostics",
            "raw manifest command evaluation",
            "host-gated AI command execution workflows"
        )
    },
    @{
        Path = ".agents/skills/editor-change/SKILL.md"
        Needles = @(
            "make_editor_ai_command_panel_model",
            "make_ai_command_panel_ui_model",
            "raw manifest command strings",
            "free-form manifest edits"
        )
    },
    @{
        Path = ".claude/skills/gameengine-editor/SKILL.md"
        Needles = @(
            "make_editor_ai_command_panel_model",
            "make_ai_command_panel_ui_model",
            "raw manifest command strings",
            "free-form manifest edits"
        )
    }
)
foreach ($check in $editorAiCommandPanelChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing editor AI command diagnostics panel contract: $($missingNeedles -join ', ')"
    }
}

$editorAiEvidenceImportReviewChecks = @(
    @{
        Path = "editor/core/include/mirakana/editor/playtest_package_review.hpp"
        Needles = @(
            "EditorAiPlaytestEvidenceImportStatus",
            "EditorAiPlaytestEvidenceImportReviewRow",
            "EditorAiPlaytestEvidenceImportDesc",
            "EditorAiPlaytestEvidenceImportModel",
            "make_editor_ai_playtest_evidence_import_model",
            "make_editor_ai_playtest_evidence_import_ui_model"
        )
    },
    @{
        Path = "editor/core/src/playtest_package_review.cpp"
        Needles = @(
            "GameEngine.EditorAiPlaytestEvidence",
            "ai_evidence_import",
            "claims_editor_core_execution",
            "evidence-not-externally-supplied",
            "validation execution",
            "package script execution"
        )
    },
    @{
        Path = "editor/src/main.cpp"
        Needles = @(
            "make_ai_evidence_import_model",
            "draw_ai_evidence_import_review_table",
            "ai_evidence_import_text_",
            "ai_playtest_evidence_rows_",
            "Import Evidence",
            "Clear Imported Evidence",
            "reset_ai_evidence_import_state"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor ai playtest evidence import parses external rows without execution",
            "editor ai playtest evidence import blocks malformed rows and unsupported claims",
            "GameEngine.EditorAiPlaytestEvidence",
            "EditorAiPlaytestEvidenceImportReviewRow",
            "ai_evidence_import.rows.agent-contract.status"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Editor AI Evidence Import Review v1",
            "EditorAiPlaytestEvidenceImportModel",
            "GameEngine.EditorAiPlaytestEvidence",
            "ai_evidence_import",
            "transient"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Editor AI Evidence Import Review v1",
            "EditorAiPlaytestEvidenceImportModel",
            "ai_evidence_import",
            "imported evidence rows"
        )
    },
    @{
        Path = "docs/ai-game-development.md"
        Needles = @(
            "Editor AI Evidence Import Review v1",
            "EditorAiPlaytestEvidenceImportReviewRow",
            "GameEngine.EditorAiPlaytestEvidence",
            "transient local editor state"
        )
    },
    @{
        Path = "docs/roadmap.md"
        Needles = @(
            "Editor AI Evidence Import Review v1",
            "EditorAiPlaytestEvidenceImportModel",
            "ai_evidence_import",
            "AI command execution"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "Editor AI Evidence Import Review v1",
            "EditorAiPlaytestEvidenceImportModel",
            "make_editor_ai_playtest_evidence_import_ui_model",
            "ai_evidence_import"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "2026-05-07-editor-ai-evidence-import-review-v1.md",
            "Editor AI Evidence Import Review v1",
            "EditorAiPlaytestEvidenceImportModel",
            "ai_evidence_import"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
        Needles = @(
            "Editor AI Evidence Import Review v1",
            "EditorAiPlaytestEvidenceImportModel",
            "ai_evidence_import",
            "unacknowledged or automatic host-gated AI command execution"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "currentEditorAiEvidenceImportReview",
            "Editor AI Evidence Import Review v1",
            "EditorAiPlaytestEvidenceImportModel",
            "GameEngine.EditorAiPlaytestEvidence",
            "ai_evidence_import",
            "host-gated AI command execution workflows"
        )
    },
    @{
        Path = ".agents/skills/editor-change/SKILL.md"
        Needles = @(
            "make_editor_ai_playtest_evidence_import_model",
            "make_editor_ai_playtest_evidence_import_ui_model",
            "GameEngine.EditorAiPlaytestEvidence",
            "ai_evidence_import"
        )
    },
    @{
        Path = ".claude/skills/gameengine-editor/SKILL.md"
        Needles = @(
            "make_editor_ai_playtest_evidence_import_model",
            "make_editor_ai_playtest_evidence_import_ui_model",
            "GameEngine.EditorAiPlaytestEvidence",
            "ai_evidence_import"
        )
    }
)
foreach ($check in $editorAiEvidenceImportReviewChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing editor AI evidence import review contract: $($missingNeedles -join ', ')"
    }
}

$editorAiReviewedValidationExecutionChecks = @(
    @{
        Path = "editor/core/include/mirakana/editor/ai_command_panel.hpp"
        Needles = @(
            "EditorAiReviewedValidationExecutionStatus",
            "EditorAiReviewedValidationExecutionDesc",
            "EditorAiReviewedValidationExecutionModel",
            "acknowledge_host_gates",
            "acknowledged_host_gates",
            "host_gate_acknowledgement_required",
            "host_gates_acknowledged",
            "make_editor_ai_reviewed_validation_execution_plan",
            "make_ai_reviewed_validation_execution_ui_model",
            "mirakana::ProcessCommand"
        )
    },
    @{
        Path = "editor/core/src/ai_command_panel.cpp"
        Needles = @(
            "reviewed run-validation-recipe argv",
            "-Mode",
            "Execute",
            "-HostGateAcknowledgements",
            "requires host-gate acknowledgement",
            "host_gate_acknowledgement_required",
            "ai_commands.execution",
            "editor core must not execute validation recipes",
            "host-gated"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor ai reviewed validation execution plan converts reviewed dry run argv",
            "editor ai reviewed validation execution plan blocks unsafe or host gated rows",
            "editor ai reviewed validation execution requires host gate acknowledgement",
            "editor ai reviewed validation execution rejects mismatched host gate acknowledgement",
            "host_gate_acknowledgement_required",
            "host_gates_acknowledged",
            "ai_commands.execution.agent-contract.command",
            "mirakana::is_safe_process_command"
        )
    },
    @{
        Path = "editor/src/main.cpp"
        Needles = @(
            "make_ai_reviewed_execution_plan",
            "execute_ai_reviewed_validation_recipe",
            "mirakana::Win32ProcessRunner",
            "apply_ai_validation_process_result",
            "AI Reviewed Validation Execution",
            "ai_playtest_evidence_rows_",
            "ai_acknowledged_host_gate_recipe_ids_",
            "set_ai_host_gate_acknowledged",
            'ImGui::Checkbox("Host gate"'
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Editor AI Reviewed Validation Execution v1",
            "Host-Gated Validation Execution Ack v1",
            "EditorAiReviewedValidationExecutionModel",
            "make_editor_ai_reviewed_validation_execution_plan",
            "-HostGateAcknowledgements",
            "ai_commands.execution",
            "host_gate_acknowledgement_required",
            "Host-gated rows"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Editor AI Reviewed Validation Execution v1",
            "Host-Gated Validation Execution Ack v1",
            "EditorAiReviewedValidationExecutionModel",
            "mirakana::ProcessCommand",
            "-HostGateAcknowledgements",
            "automatic host-gated recipe execution"
        )
    },
    @{
        Path = "docs/ai-game-development.md"
        Needles = @(
            "Editor AI Reviewed Validation Execution v1",
            "Host-Gated Validation Execution Ack v1",
            "EditorAiReviewedValidationExecutionDesc",
            "mirakana::ProcessCommand",
            "-HostGateAcknowledgements",
            "Host-gated recipes"
        )
    },
    @{
        Path = "docs/roadmap.md"
        Needles = @(
            "Editor AI Reviewed Validation Execution v1",
            "Host-Gated Validation Execution Ack v1",
            "EditorAiReviewedValidationExecutionModel",
            "ai_commands.execution",
            "-HostGateAcknowledgements",
            "host-gated recipe execution"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "Editor AI Reviewed Validation Execution v1",
            "Host-Gated Validation Execution Ack v1",
            "EditorAiReviewedValidationExecutionDesc",
            "make_ai_reviewed_validation_execution_ui_model",
            "-HostGateAcknowledgements",
            "broad package script execution"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "2026-05-07-editor-ai-host-gated-validation-execution-ack-v1.md",
            "Host-Gated Validation Execution Ack v1",
            "2026-05-07-editor-ai-reviewed-validation-execution-v1.md",
            "Editor AI Reviewed Validation Execution v1",
            "ai_commands.execution",
            "-HostGateAcknowledgements",
            "host-gate-free reviewed"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "Host-Gated Validation Execution Ack v1",
            "acknowledge_host_gates",
            "host_gate_acknowledgement_required",
            "-HostGateAcknowledgements",
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
        Needles = @(
            "Editor AI Reviewed Validation Execution v1",
            "Host-Gated Validation Execution Ack v1",
            "EditorAiReviewedValidationExecutionModel",
            "editor-ai-reviewed-validation-execution-v1",
            "editor-ai-host-gated-validation-execution-ack-v1",
            "automatic host-gated AI command execution"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "currentEditorAiReviewedValidationExecution",
            "Host-Gated Validation Execution Ack v1",
            "Editor AI Reviewed Validation Execution v1",
            "EditorAiReviewedValidationExecutionModel",
            "EditorAiReviewedValidationExecutionDesc",
            "make_ai_reviewed_validation_execution_ui_model",
            "-HostGateAcknowledgements",
            "ai_commands.execution",
            "automatic host-gated"
        )
    },
    @{
        Path = ".agents/skills/editor-change/SKILL.md"
        Needles = @(
            "make_editor_ai_reviewed_validation_execution_plan",
            "make_ai_reviewed_validation_execution_ui_model",
            "explicit per-row host-gate acknowledgement",
            "-HostGateAcknowledgements",
            "platform process runner"
        )
    },
    @{
        Path = ".claude/skills/gameengine-editor/SKILL.md"
        Needles = @(
            "make_editor_ai_reviewed_validation_execution_plan",
            "make_ai_reviewed_validation_execution_ui_model",
            "explicit per-row host-gate acknowledgement",
            "-HostGateAcknowledgements",
            "platform process runner"
        )
    }
)
foreach ($check in $editorAiReviewedValidationExecutionChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing editor AI reviewed validation execution contract: $($missingNeedles -join ', ')"
    }
}

$editorAiReviewedValidationBatchExecutionChecks = @(
    @{
        Path = "editor/core/include/mirakana/editor/ai_command_panel.hpp"
        Needles = @(
            "EditorAiReviewedValidationExecutionBatchDesc",
            "EditorAiReviewedValidationExecutionBatchModel",
            "acknowledged_host_gate_recipe_ids",
            "executable_plan_indexes",
            "make_editor_ai_reviewed_validation_execution_batch",
            "make_ai_reviewed_validation_execution_batch_ui_model"
        )
    },
    @{
        Path = "editor/core/src/ai_command_panel.cpp"
        Needles = @(
            "make_editor_ai_reviewed_validation_execution_batch",
            "make_editor_ai_reviewed_validation_execution_plan",
            "batch has no reviewed validation rows",
            "batch has no executable reviewed validation rows",
            "ai_commands.execution.batch",
            "ai_commands.execution.batch.executable_count"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor ai reviewed validation batch execution collects ready acknowledged plans",
            "EditorAiReviewedValidationExecutionBatchDesc",
            "expected_executable_plan_indexes",
            "vulkan-package-smoke",
            "ai_commands.execution.batch.ready_count",
            "ai_commands.execution.batch.executable_count"
        )
    },
    @{
        Path = "editor/src/main.cpp"
        Needles = @(
            "make_ai_reviewed_execution_batch",
            "execute_ai_reviewed_validation_batch",
            "ai_acknowledged_host_gate_recipe_ids_",
            "AI validation batch has no executable reviewed rows",
            "Batch: ready",
            "Execute Ready"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Editor AI Reviewed Validation Batch Execution v1",
            "EditorAiReviewedValidationExecutionBatchModel",
            "make_editor_ai_reviewed_validation_execution_batch",
            "ai_commands.execution.batch",
            "Execute Ready",
            "unacknowledged host-gated recipe execution"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Reviewed Validation Batch Execution v1",
            "EditorAiReviewedValidationExecutionBatchModel",
            "make_editor_ai_reviewed_validation_execution_batch",
            "make_ai_reviewed_validation_execution_batch_ui_model",
            "ai_commands.execution.batch",
            "Execute Ready",
            "unacknowledged host-gated recipe execution"
        )
    },
    @{
        Path = "docs/ai-game-development.md"
        Needles = @(
            "Editor AI Reviewed Validation Batch Execution v1",
            "EditorAiReviewedValidationExecutionBatchModel",
            "make_editor_ai_reviewed_validation_execution_batch",
            "ai_commands.execution.batch",
            "Execute Ready",
            "Unacknowledged host-gated execution"
        )
    },
    @{
        Path = "docs/roadmap.md"
        Needles = @(
            "Editor AI Reviewed Validation Batch Execution v1",
            "EditorAiReviewedValidationExecutionBatchModel",
            "ai_commands.execution.batch",
            "Execute Ready",
            "unacknowledged host-gated recipe execution"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "Editor AI Reviewed Validation Batch Execution v1",
            "EditorAiReviewedValidationExecutionBatchModel",
            "make_editor_ai_reviewed_validation_execution_batch",
            "make_ai_reviewed_validation_execution_batch_ui_model",
            "ai_commands.execution.batch",
            "unacknowledged host-gated rows remain host-gated"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "2026-05-07-editor-ai-reviewed-validation-batch-execution-v1.md",
            "Editor AI Reviewed Validation Batch Execution v1",
            "EditorAiReviewedValidationExecutionBatchModel",
            "ai_commands.execution.batch",
            "Execute Ready"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "Editor AI Reviewed Validation Batch Execution v1 Implementation Plan",
            "EditorAiReviewedValidationExecutionBatchModel",
            "make_editor_ai_reviewed_validation_execution_batch",
            "ai_commands.execution.batch",
            "Execute Ready",
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
        Needles = @(
            "Editor AI Reviewed Validation Batch Execution v1",
            "EditorAiReviewedValidationExecutionBatchModel",
            "make_editor_ai_reviewed_validation_execution_batch",
            "ai_commands.execution.batch",
            "Execute Ready",
            "unacknowledged or automatic host-gated AI command execution"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "currentEditorAiReviewedValidationExecution",
            "EditorAiReviewedValidationExecutionBatchModel",
            "make_editor_ai_reviewed_validation_execution_batch",
            "make_ai_reviewed_validation_execution_batch_ui_model",
            "ai_commands.execution.batch",
            "Execute Ready",
            "unacknowledged/automatic host-gated recipe execution"
        )
    },
    @{
        Path = ".agents/skills/editor-change/SKILL.md"
        Needles = @(
            "make_editor_ai_reviewed_validation_execution_batch",
            "make_ai_reviewed_validation_execution_batch_ui_model",
            "ai_commands.execution.batch",
            "Execute Ready",
            "unacknowledged host-gated recipes"
        )
    },
    @{
        Path = ".claude/skills/gameengine-editor/SKILL.md"
        Needles = @(
            "make_editor_ai_reviewed_validation_execution_batch",
            "make_ai_reviewed_validation_execution_batch_ui_model",
            "ai_commands.execution.batch",
            "Execute Ready",
            "unacknowledged host-gated recipes"
        )
    }
)
foreach ($check in $editorAiReviewedValidationBatchExecutionChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing editor AI reviewed validation batch execution contract: $($missingNeedles -join ', ')"
    }
}

$editorContentBrowserImportPanelChecks = @(
    @{
        Path = "editor/core/include/mirakana/editor/content_browser.hpp"
        Needles = @(
            "SourceAssetRegistryDocument",
            "refresh_from(const SourceAssetRegistryDocument& registry)",
            "select(const AssetKey& key)"
        )
    },
    @{
        Path = "editor/core/src/content_browser.cpp"
        Needles = @(
            "make_content_browser_item(const SourceAssetRegistryRow& row)",
            "asset_id_from_key(row.key)",
            "identity_backed = true",
            "ContentBrowserState::refresh_from(const SourceAssetRegistryDocument& registry)"
        )
    },
    @{
        Path = "editor/core/include/mirakana/editor/content_browser_import_panel.hpp"
        Needles = @(
            "EditorContentBrowserImportPanelModel",
            "EditorContentBrowserAssetRow",
            "EditorContentBrowserImportQueueRow",
            "EditorContentBrowserHotReloadSummaryRow",
            "make_editor_content_browser_import_panel_model",
            "make_content_browser_import_panel_ui_model"
        )
    },
    @{
        Path = "editor/core/src/content_browser_import_panel.cpp"
        Needles = @(
            "content_browser_import.assets",
            "content_browser_import.imports",
            "content_browser_import.hot_reload",
            "make_editor_asset_pipeline_panel_model",
            "failed_hot_reload_count"
        )
    },
    @{
        Path = "editor/src/main.cpp"
        Needles = @(
            "make_editor_content_browser_import_panel_model",
            "asset_panel_model.pipeline",
            "asset_panel_model.import_queue",
            "Hot Reload Summary"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor content browser import panel model summarizes assets imports and diagnostics",
            "editor content browser populates source registry rows",
            "editor source registry browser refresh loads project registry into content browser",
            "GameEngine.Project",
            "project.source_registry=source/assets/package.geassets",
            "animation_quaternion_clip",
            "validate_source_asset_registry_document",
            "refresh_content_browser_from_project_source_registry",
            "editor content browser import panel retained ui exposes all diagnostic sections",
            "EditorContentBrowserImportPanelModel",
            "make_content_browser_import_panel_ui_model"
        )
    },
    @{
        Path = "editor/core/include/mirakana/editor/source_registry_browser.hpp"
        Needles = @(
            "EditorSourceRegistryBrowserRefreshResult",
            "refresh_content_browser_from_project_source_registry",
            "AssetImportPlan",
            "ContentBrowserState"
        )
    },
    @{
        Path = "editor/core/src/source_registry_browser.cpp"
        Needles = @(
            "deserialize_source_asset_registry_document",
            "build_source_asset_import_metadata_registry",
            "build_asset_import_plan",
            "browser.refresh_from(registry)",
            "source asset registry not found"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "GameEngine.Project",
            "project.source_registry=source/assets/package.geassets",
            "refresh_content_browser_from_project_source_registry",
            "Reload Source Registry",
            "Editor Content Browser Import Diagnostics v1",
            "EditorContentBrowserImportPanelModel",
            "make_content_browser_import_panel_ui_model",
            "hot-reload summaries",
            "manifest edits"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Editor Source Registry Visible Content Browser v1",
            "GameEngine.Project",
            "project.source_registry",
            "refresh_content_browser_from_project_source_registry",
            "Reload Source Registry",
            "Editor Content Browser Import Diagnostics v1",
            "Source Registry Population v1",
            "EditorContentBrowserImportPanelModel",
            "make_content_browser_import_panel_ui_model",
            "hot-reload summaries",
            "broad editor/importer readiness"
        )
    },
    @{
        Path = "docs/ai-game-development.md"
        Needles = @(
            "Editor Source Registry Visible Content Browser v1",
            "GameEngine.Project",
            "project.source_registry",
            "refresh_content_browser_from_project_source_registry",
            "Reload Source Registry",
            "Editor Content Browser Import Diagnostics v1",
            "Source Registry Population v1",
            "EditorContentBrowserImportPanelModel",
            "make_content_browser_import_panel_ui_model",
            "free-form manifest edits",
            "dynamic game-module Play-In-Editor"
        )
    },
    @{
        Path = "docs/roadmap.md"
        Needles = @(
            "Editor Source Registry Visible Content Browser v1",
            "visible `MK_editor` source registry loading is complete",
            "Editor Content Browser Import Diagnostics v1",
            "Editor Content Browser Source Registry Population v1",
            "EditorContentBrowserImportPanelModel",
            "hot-reload summaries",
            "broad editor/importer readiness"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "Editor Content Browser Import Diagnostics v1",
            "EditorContentBrowserImportPanelModel",
            "make_content_browser_import_panel_ui_model",
            "hot-reload summaries"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "2026-05-12-editor-source-registry-visible-content-browser-v1.md",
            "Reload Source Registry",
            "2026-05-12-editor-content-browser-source-registry-population-v1.md",
            "2026-05-07-editor-content-browser-import-diagnostics-v1.md",
            "Editor Content Browser Import Diagnostics v1",
            "EditorContentBrowserImportPanelModel",
            "hot-reload summaries"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
        Needles = @(
            "2026-05-12-editor-source-registry-visible-content-browser-v1.md",
            "Reload Source Registry",
            "Editor Content Browser Import Diagnostics v1",
            "EditorContentBrowserImportPanelModel",
            "editor-content-browser-and-import-diagnostics-v1",
            "unacknowledged or automatic host-gated AI command execution"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "Editor Source Registry Visible Content Browser v1",
            "GameEngine.Project project.source_registry",
            "source/assets/package.geassets",
            "refresh_content_browser_from_project_source_registry",
            "Reload Source Registry",
            "Editor Content Browser Import Diagnostics v1",
            "Editor Content Browser Source Registry Population v1",
            "SourceAssetRegistryDocument",
            "EditorContentBrowserImportPanelModel",
            "make_content_browser_import_panel_ui_model",
            "hot-reload summaries",
            "import/recook/hot reload execution",
            "currentEditorContentBrowserImportDiagnostics"
        )
    },
    @{
        Path = "editor/src/main.cpp"
        Needles = @(
            "Reload Source Registry",
            "refresh_content_browser_from_project_source_registry",
            "content_browser_source_registry_loaded_",
            "Source Registry Diagnostics",
            "Asset Key"
        )
    },
    @{
        Path = ".agents/skills/editor-change/SKILL.md"
        Needles = @(
            "make_editor_content_browser_import_panel_model",
            "make_content_browser_import_panel_ui_model",
            "hot-reload summaries",
            "free-form manifest edits"
        )
    },
    @{
        Path = ".claude/skills/gameengine-editor/SKILL.md"
        Needles = @(
            "make_editor_content_browser_import_panel_model",
            "make_content_browser_import_panel_ui_model",
            "hot-reload summaries",
            "free-form manifest edits"
        )
    }
)
foreach ($check in $editorContentBrowserImportPanelChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing editor content browser import diagnostics contract: $($missingNeedles -join ', ')"
    }
}

$editorContentBrowserImportNativeDialogChecks = @(
    @{
        Path = "editor/core/include/mirakana/editor/content_browser_import_panel.hpp"
        Needles = @(
            "EditorContentBrowserImportOpenDialogModel",
            "EditorContentBrowserImportOpenDialogRow",
            "make_content_browser_import_open_dialog_request",
            "make_content_browser_import_open_dialog_model",
            "make_content_browser_import_open_dialog_ui_model"
        )
    },
    @{
        Path = "editor/core/src/content_browser_import_panel.cpp"
        Needles = @(
            "content_browser_import.open_dialog",
            "Audio Source",
            "audio_source",
            "import_open_dialog_status",
            "supported codec source paths"
        )
    },
    @{
        Path = "editor/src/main.cpp"
        Needles = @(
            "Browse Import Sources",
            "asset_import_open_dialog_",
            "project_store_relative_asset_import_source_path",
            "rebuild_asset_import_plan_from_sources",
            "make_content_browser_import_open_dialog_request"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor content browser import native file dialog reviews source selections",
            "content_browser_import.open_dialog.status",
            "content_browser_import.open_dialog.paths.1",
            "audio_source"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Editor Content Browser Import Native Dialog v1",
            "EditorContentBrowserImportOpenDialogModel",
            "make_content_browser_import_open_dialog_ui_model",
            "content_browser_import.open_dialog",
            "Browse Import Sources",
            "Editor Content Browser Import External Copy Review v1"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Editor Content Browser Import Diagnostics v1, Native Dialog v1, External Copy Review v1, and Codec Adapter Review v1",
            "EditorContentBrowserImportOpenDialogModel",
            "content_browser_import.open_dialog",
            "Browse Import Sources",
            "broad editor/importer readiness"
        )
    },
    @{
        Path = "docs/roadmap.md"
        Needles = @(
            "Editor Content Browser Import Native Dialog v1",
            "EditorContentBrowserImportOpenDialogModel",
            "Browse Import Sources",
            "ExternalAssetImportAdapters"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "2026-05-07-editor-content-browser-import-native-dialog-v1.md",
            "Editor Content Browser Import Native Dialog v1",
            "make_content_browser_import_open_dialog_ui_model",
            "content_browser_import.open_dialog",
            "external importer claims"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
        Needles = @(
            "Editor Content Browser Import Native Dialog v1",
            "EditorContentBrowserImportOpenDialogModel",
            "content_browser_import.open_dialog",
            "Browse Import Sources",
            "ExternalAssetImportAdapters"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "Editor Content Browser Import Native Dialog v1 Implementation Plan",
            "EditorContentBrowserImportOpenDialogModel",
            "content_browser_import.open_dialog",
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "Editor Content Browser Import Native Dialog v1",
            "EditorContentBrowserImportOpenDialogModel",
            "make_content_browser_import_open_dialog_request",
            "make_content_browser_import_open_dialog_ui_model",
            "content_browser_import.open_dialog",
            "Browse Import Sources"
        )
    },
    @{
        Path = ".agents/skills/editor-change/SKILL.md"
        Needles = @(
            "make_content_browser_import_open_dialog_request",
            "content_browser_import.open_dialog",
            "Browse Import Sources",
            "ExternalAssetImportAdapters"
        )
    },
    @{
        Path = ".claude/skills/gameengine-editor/SKILL.md"
        Needles = @(
            "make_content_browser_import_open_dialog_request",
            "content_browser_import.open_dialog",
            "Browse Import Sources",
            "ExternalAssetImportAdapters"
        )
    }
)
foreach ($check in $editorContentBrowserImportNativeDialogChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing editor content browser import native dialog contract: $($missingNeedles -join ', ')"
    }
}

$editorContentBrowserImportExternalCopyReviewChecks = @(
    @{
        Path = "editor/core/include/mirakana/editor/content_browser_import_panel.hpp"
        Needles = @(
            "EditorContentBrowserImportExternalSourceCopyStatus",
            "EditorContentBrowserImportExternalSourceCopyInput",
            "EditorContentBrowserImportExternalSourceCopyModel",
            "make_content_browser_import_external_source_copy_model",
            "make_content_browser_import_external_source_copy_ui_model"
        )
    },
    @{
        Path = "editor/core/src/content_browser_import_panel.cpp"
        Needles = @(
            "content_browser_import.external_copy",
            "External import source copy ready",
            "safe project-relative path",
            "supported codec source paths",
            "target already exists"
        )
    },
    @{
        Path = "editor/src/main.cpp"
        Needles = @(
            "Copy External Sources",
            "asset_import_external_copy_",
            "asset_import_external_copy_target_path",
            "copy_asset_import_external_sources",
            "imported_sources"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor content browser import external source copy review keeps copying explicit",
            "EditorContentBrowserImportExternalSourceCopyStatus",
            "content_browser_import.external_copy.status",
            "content_browser_import.external_copy.copy_count",
            "content_browser_import.external_copy.rows.1.source",
            "content_browser_import.external_copy.rows.1.target"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Editor Content Browser Import External Copy Review v1",
            "EditorContentBrowserImportExternalSourceCopyModel",
            "make_content_browser_import_external_source_copy_ui_model",
            "content_browser_import.external_copy",
            "Copy External Sources",
            "ExternalAssetImportAdapters"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Editor Content Browser Import Diagnostics v1, Native Dialog v1, External Copy Review v1, and Codec Adapter Review v1",
            "EditorContentBrowserImportExternalSourceCopyModel",
            "content_browser_import.external_copy",
            "Copy External Sources",
            "broad editor/importer readiness"
        )
    },
    @{
        Path = "docs/roadmap.md"
        Needles = @(
            "Editor Content Browser Import External Copy Review v1",
            "EditorContentBrowserImportExternalSourceCopyModel",
            "content_browser_import.external_copy",
            "Copy External Sources",
            "ExternalAssetImportAdapters"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "Editor Content Browser Import External Copy Review v1 Implementation Plan",
            "content_browser_import.external_copy",
            "EditorContentBrowserImportExternalSourceCopyModel",
            "make_content_browser_import_external_source_copy_model",
            "Copy External Sources",
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "2026-05-07-editor-content-browser-import-external-copy-review-v1.md",
            "Editor Content Browser Import External Copy Review v1",
            "content_browser_import.external_copy",
            "Copy External Sources",
            "broad importer readiness"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
        Needles = @(
            "2026-05-07-editor-content-browser-import-external-copy-review-v1.md",
            "Editor Content Browser Import External Copy Review v1",
            "reviewed Content Browser import selections",
            "ExternalAssetImportAdapters",
            "broad importer readiness"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "Editor Content Browser Import Codec Adapter Review v1",
            "Editor Content Browser Import External Copy Review v1",
            "EditorContentBrowserImportExternalSourceCopyModel",
            "make_content_browser_import_external_source_copy_model",
            "content_browser_import.external_copy",
            "Copy External Sources"
        )
    },
    @{
        Path = ".agents/skills/editor-change/SKILL.md"
        Needles = @(
            "make_content_browser_import_external_source_copy_model",
            "content_browser_import.external_copy",
            "Copy External Sources",
            "ExternalAssetImportAdapters"
        )
    },
    @{
        Path = ".claude/skills/gameengine-editor/SKILL.md"
        Needles = @(
            "make_content_browser_import_external_source_copy_model",
            "content_browser_import.external_copy",
            "Copy External Sources",
            "ExternalAssetImportAdapters"
        )
    }
)
foreach ($check in $editorContentBrowserImportExternalCopyReviewChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing editor content browser import external copy review contract: $($missingNeedles -join ', ')"
    }
}

$editorContentBrowserImportCodecAdapterCompletedPlanChecks = @(
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "Editor Content Browser Import Codec Adapter Review v1 Implementation Plan",
            "ExternalAssetImportAdapters",
            ".png",
            ".gltf",
            ".glb",
            ".wav",
            ".mp3",
            ".flac",
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "2026-05-07-editor-content-browser-import-codec-adapter-review-v1.md",
            "Editor Content Browser Import Codec Adapter Review v1",
            "ExternalAssetImportAdapters",
            "broad importer readiness"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
        Needles = @(
            "2026-05-07-editor-content-browser-import-codec-adapter-review-v1.md",
            "editor-content-browser-import-codec-adapter-review-v1",
            "ExternalAssetImportAdapters",
            "asset-importers",
            "broad importer readiness"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "Editor Content Browser Import Codec Adapter Review v1",
            "ExternalAssetImportAdapters",
            ".png/.gltf/.glb/.wav/.mp3/.flac",
            "CI Matrix Contract Check v1"
        )
    },
    @{
        Path = "editor/core/src/content_browser_import_panel.cpp"
        Needles = @(
            "PNG Texture Source",
            "glTF Mesh Source",
            "Common Audio Source",
            ".png",
            ".gltf",
            ".glb",
            ".wav",
            ".mp3",
            ".flac",
            "supported codec source paths"
        )
    },
    @{
        Path = "editor/src/main.cpp"
        Needles = @(
            "ExternalAssetImportAdapters",
            ".png",
            ".gltf",
            ".glb",
            ".wav",
            ".mp3",
            ".flac",
            "execute_asset_import_plan(tool_filesystem_, asset_import_plan_, adapters.options())",
            "execute_asset_import_plan(tool_filesystem_, recook_plan, adapters.options())"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor content browser import codec adapter review accepts supported source formats",
            "PNG Texture Source",
            "glTF Mesh Source",
            "Common Audio Source",
            "hero.png",
            "ship.gltf",
            "hit.wav"
        )
    }
)
foreach ($check in $editorContentBrowserImportCodecAdapterCompletedPlanChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing editor content browser import codec adapter completed plan contract: $($missingNeedles -join ', ')"
    }
}

$ciMatrixContractCheckCompletedPlanChecks = @(
    @{
        Path = "tools/check-ci-matrix.ps1"
        Needles = @(
            ".github/workflows/validate.yml",
            ".github/workflows/ios-validate.yml",
            "tools/classify-pr-validation-tier.ps1",
            "Assert-ValidationTierSelection",
            "docs-only PR",
            "static policy PR",
            "runtime PR",
            "workflow PR",
            "non-PR run",
            "tools/evaluate-cpp23.ps1",
            "Assert-ReleasePackageArtifacts",
            "cpp23-release-preset-eval",
            "windows-packages",
            "linux-coverage",
            "linux-sanitizer-test-logs",
            "static-analysis-tidy-logs",
            "macos-test-logs",
            "ios-simulator-build",
            "ci-matrix-check: ok"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "ciMatrixCheck",
            "tools/check-ci-matrix.ps1",
            "classifyPrValidationTier",
            "tools/classify-pr-validation-tier.ps1"
        )
    },
    @{
        Path = "tools/classify-pr-validation-tier.ps1"
        Needles = @(
            "Test-CiWorkflowPath",
            "Test-RuntimeOrBuildPath",
            "Test-StaticPolicyPath",
            "tools/classify-pr-validation-tier.ps1",
            "tools/check-tidy.ps1",
            "GitHubOutputPath"
        )
    },
    @{
        Path = "tools/validate.ps1"
        Needles = @(
            "check-ci-matrix.ps1"
        )
    },
    @{
        Path = ".github/workflows/validate.yml"
        Needles = @(
            "Select PR validation tier",
            "tools/classify-pr-validation-tier.ps1 -RunAll -GitHubOutputPath",
            "tools/classify-pr-validation-tier.ps1 -ChangedPath",
            "needs.changes.outputs.windows",
            "PR Gate",
            "toJson(needs)",
            "windows-packages",
            "out/build/cpp23-release-preset-eval/*.zip.sha256",
            "linux-coverage",
            "linux-sanitizer-test-logs",
            "static-analysis",
            "tools/check-tidy.ps1 -Strict",
            "static-analysis-tidy-logs",
            "macos-test-logs"
        )
    },
    @{
        Path = "tools/evaluate-cpp23.ps1"
        Needles = @(
            "release-package-artifacts.ps1",
            "Assert-ReleasePackageArtifacts",
            "cpp23-release-preset-eval"
        )
    },
    @{
        Path = ".github/workflows/ios-validate.yml"
        Needles = @(
            "workflow_dispatch",
            "check-mobile-packaging.ps1 -RequireApple",
            "smoke-ios-package.ps1 -Game sample_headless -Configuration Debug",
            "ios-simulator-build"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "CI Matrix Contract Check v1 Implementation Plan",
            "tools/check-ci-matrix.ps1",
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1",
            "Windows/Linux/sanitizer/macOS/iOS",
            "full package/build matrix readiness"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "2026-05-07-ci-matrix-contract-check-v1.md",
            "CI Matrix Contract Check v1",
            "next-production-gap-selection"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
        Needles = @(
            "2026-05-07-ci-matrix-contract-check-v1.md",
            "tools/check-ci-matrix.ps1",
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1",
            "next-production-gap-selection"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "Full Repository Static Analysis CI Contract v1 Implementation Plan",
            "**Status:** Completed.",
            "static-analysis",
            "tools/check-tidy.ps1 -Strict",
            "static-analysis-tidy-logs",
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1",
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "CI Matrix Contract Check v1",
            "Full Repository Static Analysis CI Contract v1",
            "tools/classify-pr-validation-tier.ps1",
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1",
            "without executing CI locally"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1",
            "PR validation tier selector",
            "tools/classify-pr-validation-tier.ps1",
            "PR aggregate gate",
            "Windows/Linux/sanitizer/static-analysis/macOS/iOS",
            "static-analysis",
            "does not execute GitHub Actions locally"
        )
    },
    @{
        Path = "docs/roadmap.md"
        Needles = @(
            "CI Matrix Contract Check v1",
            "C++23 Release Package Artifact CI Evidence v1",
            "Full Repository Static Analysis CI Contract v1",
            "tools/check-ci-matrix.ps1",
            "broader build/package CI execution evidence"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "ciMatrixCheck",
            "classifyPrValidationTier",
            "CI Matrix Contract Check v1",
            # manifest.json is emitted with ASCII escapes for '+' (e.g. language C\u002B\u002B23).
            "C\u002B\u002B23 Release Package Artifact CI Evidence v1",
            "Full Repository Static Analysis CI Contract v1",
            "Assert-ReleasePackageArtifacts",
            "tools/classify-pr-validation-tier.ps1",
            "tools/check-ci-matrix.ps1",
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1",
            "Windows/Linux/sanitizer/macOS/iOS",
            "static-analysis",
            "broader build/package CI execution evidence"
        )
    }
)
foreach ($check in $ciMatrixContractCheckCompletedPlanChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing CI matrix contract completed plan evidence: $($missingNeedles -join ', ')"
    }
}

$cpp23ReleasePackageArtifactCiEvidenceChecks = @(
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "C++23 Release Package Artifact CI Evidence v1 Implementation Plan",
            "tools/evaluate-cpp23.ps1 -Release",
            "Assert-ReleasePackageArtifacts",
            ".zip.sha256",
            "full cross-platform package matrix readiness"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "2026-05-08-cpp23-release-package-artifact-ci-evidence-v1.md",
            "C++23 Release Package Artifact CI Evidence v1",
            "Assert-ReleasePackageArtifacts"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
        Needles = @(
            "2026-05-08-cpp23-release-package-artifact-ci-evidence-v1.md",
            "C++23 Release Package Artifact CI Evidence v1",
            "Assert-ReleasePackageArtifacts",
            "full cross-platform package matrix readiness"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "C++23 Release Package Artifact CI Evidence v1",
            "tools/evaluate-cpp23.ps1 -Release",
            "Assert-ReleasePackageArtifacts",
            ".zip.sha256"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "C++23 release package artifact validation",
            "Assert-ReleasePackageArtifacts",
            "out/build/cpp23-release-preset-eval/*.zip.sha256"
        )
    }
)
foreach ($check in $cpp23ReleasePackageArtifactCiEvidenceChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing C++23 release package artifact CI evidence: $($missingNeedles -join ', ')"
    }
}

$editorMaterialAssetPreviewPanelChecks = @(
    @{
        Path = "editor/core/include/mirakana/editor/material_asset_preview_panel.hpp"
        Needles = @(
            "EditorMaterialAssetPreviewPanelModel",
            "EditorMaterialAssetPreviewTexturePayloadRow",
            "EditorMaterialAssetPreviewShaderRow",
            "EditorMaterialGpuPreviewExecutionSnapshot",
            "EditorMaterialGpuPreviewDisplayParityChecklistRow",
            "gpu_display_parity_checklist_rows",
            "editor_material_gpu_preview_vulkan_visible_refresh_evidence",
            "editor_material_gpu_preview_metal_visible_refresh_evidence",
            "make_editor_material_asset_preview_panel_model",
            "apply_editor_material_gpu_preview_execution_snapshot",
            "make_material_asset_preview_panel_ui_model"
        )
    },
    @{
        Path = "editor/core/src/material_asset_preview_panel.cpp"
        Needles = @(
            "material_asset_preview.material.path",
            "material_asset_preview.gpu.status",
            "material_asset_preview.gpu.execution",
            "material_asset_preview.gpu.execution.parity_checklist",
            "ge.editor.material_gpu_preview_display_parity_checklist.v1",
            "material_asset_preview.gpu.execution.vulkan_visible_refresh",
            "material_asset_preview.gpu.execution.metal_visible_refresh",
            "material_asset_preview.shaders",
            "apply_editor_material_gpu_preview_execution_snapshot",
            "make_editor_selected_material_preview",
            "make_editor_material_gpu_preview_plan"
        )
    },
    @{
        Path = "editor/src/main.cpp"
        Needles = @(
            "make_editor_material_asset_preview_panel_model",
            "material_preview_panel_model",
            "texture_payload_rows",
            "shader_rows",
            "ensure_material_preview_gpu_cache",
            "refresh_material_gpu_preview_execution",
            "make_material_gpu_preview_execution_snapshot",
            "apply_editor_material_gpu_preview_execution_snapshot",
            "gpu_execution_vulkan_visible_refresh",
            "gpu_execution_metal_visible_refresh"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor material asset preview panel summarizes payloads shaders and diagnostics",
            "editor material asset preview panel retained ui exposes diagnostic sections",
            "editor material asset preview panel reports host gpu preview execution snapshot",
            "EditorMaterialAssetPreviewPanelModel",
            "EditorMaterialGpuPreviewExecutionSnapshot",
            "material_asset_preview.gpu.execution.vulkan_visible_refresh",
            "material_asset_preview.gpu.execution.metal_visible_refresh",
            "material_asset_preview.gpu.execution.parity_checklist",
            "make_material_asset_preview_panel_ui_model"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Editor Material Asset Preview Diagnostics v1",
            "Editor Material GPU Preview Execution Evidence v1",
            "EditorMaterialAssetPreviewPanelModel",
            "EditorMaterialGpuPreviewExecutionSnapshot",
            "make_material_asset_preview_panel_ui_model",
            "material_asset_preview.gpu.execution",
            "material_asset_preview.gpu.execution.vulkan_visible_refresh",
            "material_asset_preview.gpu.execution.metal_visible_refresh",
            "material_asset_preview.gpu.execution.parity_checklist",
            "ge.editor.material_gpu_preview_display_parity_checklist.v1",
            "typed GPU payload texture rows",
            "RHI uploads"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Editor Material Asset Preview Diagnostics v1",
            "GPU Preview Execution Evidence v1",
            "EditorMaterialAssetPreviewPanelModel",
            "EditorMaterialGpuPreviewExecutionSnapshot",
            "make_editor_material_asset_preview_panel_model",
            "apply_editor_material_gpu_preview_execution_snapshot",
            "typed GPU payload texture rows",
            "broad editor/renderer readiness"
        )
    },
    @{
        Path = "docs/ai-game-development.md"
        Needles = @(
            "Editor Material Asset Preview Diagnostics v1",
            "Editor Material GPU Preview Execution Evidence v1",
            "EditorMaterialAssetPreviewPanelModel",
            "EditorMaterialGpuPreviewExecutionSnapshot",
            "make_material_asset_preview_panel_ui_model",
            "material_asset_preview.gpu.execution",
            "material_asset_preview.gpu.execution.vulkan_visible_refresh",
            "material_asset_preview.gpu.execution.metal_visible_refresh",
            "RHI upload/display",
            "Vulkan display parity"
        )
    },
    @{
        Path = "docs/roadmap.md"
        Needles = @(
            "Editor Material Asset Preview Diagnostics v1",
            "Editor Material GPU Preview Execution Evidence v1",
            "EditorMaterialAssetPreviewPanelModel",
            "EditorMaterialGpuPreviewExecutionSnapshot",
            "typed GPU payload texture rows",
            "Vulkan display parity"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "Editor Material Asset Preview Diagnostics v1",
            "GPU Preview Execution Evidence v1",
            "EditorMaterialAssetPreviewPanelModel",
            "EditorMaterialGpuPreviewExecutionSnapshot",
            "apply_editor_material_gpu_preview_execution_snapshot",
            "make_material_asset_preview_panel_ui_model",
            "D3D12/Vulkan material-preview shader readiness"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "2026-05-07-editor-material-asset-preview-diagnostics-v1.md",
            "2026-05-07-editor-material-gpu-preview-execution-evidence-v1.md",
            "Editor Material Asset Preview Diagnostics v1",
            "Editor Material GPU Preview Execution Evidence v1",
            "EditorMaterialAssetPreviewPanelModel",
            "EditorMaterialGpuPreviewExecutionSnapshot",
            "Vulkan display parity"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
        Needles = @(
            "Editor Material Asset Preview Diagnostics v1",
            "Editor Material GPU Preview Execution Evidence v1",
            "EditorMaterialAssetPreviewPanelModel",
            "EditorMaterialGpuPreviewExecutionSnapshot",
            "editor-material-asset-preview-diagnostics-v1",
            "editor-material-gpu-preview-execution-evidence-v1",
            "Vulkan/Metal material-preview display parity"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "editor-material-gpu-preview-execution-evidence-v1",
            "EditorMaterialGpuPreviewExecutionSnapshot",
            "apply_editor_material_gpu_preview_execution_snapshot",
            "material_asset_preview.gpu.execution",
            "Vulkan display parity"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "Editor Material Asset Preview Diagnostics v1",
            "GPU Preview Execution Evidence v1",
            "EditorMaterialAssetPreviewPanelModel",
            "EditorMaterialGpuPreviewExecutionSnapshot",
            "EditorMaterialGpuPreviewDisplayParityChecklistRow",
            "make_material_asset_preview_panel_ui_model",
            "apply_editor_material_gpu_preview_execution_snapshot",
            "material_asset_preview.gpu.execution",
            "material_asset_preview.gpu.execution.parity_checklist",
            "ge.editor.material_gpu_preview_display_parity_checklist.v1",
            "material_asset_preview.gpu.execution.vulkan_visible_refresh",
            "material_asset_preview.gpu.execution.metal_visible_refresh",
            "typed GPU payload texture rows",
            "currentEditorMaterialAssetPreviewDiagnostics"
        )
    },
    @{
        Path = ".agents/skills/editor-change/SKILL.md"
        Needles = @(
            "make_editor_material_asset_preview_panel_model",
            "apply_editor_material_gpu_preview_execution_snapshot",
            "make_material_asset_preview_panel_ui_model",
            "material_asset_preview.gpu.execution",
            "material_asset_preview.gpu.execution.parity_checklist",
            "material_asset_preview.gpu.execution.vulkan_visible_refresh",
            "material_asset_preview.gpu.execution.metal_visible_refresh",
            "GPU payload texture readiness",
            "Vulkan display parity"
        )
    },
    @{
        Path = ".claude/skills/gameengine-editor/SKILL.md"
        Needles = @(
            "make_editor_material_asset_preview_panel_model",
            "apply_editor_material_gpu_preview_execution_snapshot",
            "make_material_asset_preview_panel_ui_model",
            "material_asset_preview.gpu.execution",
            "material_asset_preview.gpu.execution.parity_checklist",
            "material_asset_preview.gpu.execution.vulkan_visible_refresh",
            "material_asset_preview.gpu.execution.metal_visible_refresh",
            "GPU payload texture readiness",
            "Vulkan display parity"
        )
    }
)
foreach ($check in $editorMaterialAssetPreviewPanelChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing editor material asset preview diagnostics contract: $($missingNeedles -join ', ')"
    }
}
