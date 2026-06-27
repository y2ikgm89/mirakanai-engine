#requires -Version 7.0
#requires -PSEdition Core
# Chapter 144 for renderer commercial readiness final retained-root artifact import.

$importerText = Get-AgentSurfaceText "tools/import-renderer-commercial-readiness-final-retained-root-artifacts.ps1"
$importerCheckText = Get-AgentSurfaceText "tools/check-renderer-commercial-readiness-final-retained-root-artifact-import.ps1"
$runnerPreflightText = Get-AgentSurfaceText "tools/validate-renderer-metal-memory-profiling-capable-host-runner.ps1"
$runnerPreflightCheckText = Get-AgentSurfaceText "tools/check-renderer-metal-memory-profiling-capable-host-runner.ps1"
$finalHandoffText = Get-AgentSurfaceText "tools/validate-renderer-commercial-readiness-final-handoff.ps1"
$finalHandoffCheckText = Get-AgentSurfaceText "tools/check-renderer-commercial-readiness-final-handoff.ps1"
$validateText = Get-AgentSurfaceText "tools/validate.ps1"
$commandsFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/002-commands.json"
$validationRecipesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/009-validationRecipes.json"
$productionLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"
$recipePlansText = Get-AgentSurfaceText "tools/run-validation-recipe-plans.ps1"
$recipeRunnerCheckText = Get-AgentSurfaceText "tools/check-validation-recipe-runner.ps1"
$validateWorkflowText = Get-AgentSurfaceText ".github/workflows/validate.yml"
$finalFromRunsWorkflowText = Get-AgentSurfaceText ".github/workflows/renderer-commercial-readiness-final-from-runs.yml"
$ciMatrixCheckText = Get-AgentSurfaceText "tools/check-ci-matrix.ps1"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-25-renderer-commercial-readiness-evidence-promotion-v1.md"

foreach ($needle in @(
        "renderer-commercial-readiness-final-retained-root-artifact-import",
        "RendererCommercialReadinessFinalRetainedRootArtifactImport",
        "ArtifactListJsonRelative",
        "SupplementalRunIds",
        "SupplementalArtifactNames",
        "RegenerateQualityVfx",
        "RegeneratedQualityVfxOutputRootRelative",
        "workflow_artifact_list",
        "workflow_artifact_list_present",
        "artifact_handoff_strategy",
        "supplemental_import",
        "quality_vfx_regenerate",
        "final_retained_root_artifact",
        "assembler_source_artifacts",
        "missing_workflow_artifact_names",
        "missing_assembler_inputs",
        "assembler_handoff",
        "final_preflight_handoff",
        "AutoAssemble",
        "auto_assemble",
        "auto_assemble_requested",
        "auto_assemble_output_log",
        "assembler_handoff_ready",
        "gh run download",
        "GitHub CLI",
        "GitHub Actions artifacts",
        "renderer-clean-room-legal-review-artifacts",
        "renderer-commercial-readiness-final-retained-root",
        "renderer-metal-memory-profiling-host-artifacts",
        "renderer-d3d12-commercial-quality-host-evidence",
        "renderer-vulkan-strict-commercial-quality-host-evidence",
        "renderer-apple-metal-commercial-quality-host-evidence",
        "renderer-package-commercial-quality-host-evidence",
        "renderer-quality-vfx-commercial-artifacts",
        "host-gate-summary.json",
        "host_gate_summaries",
        "host_gate_summary_reasons",
        "assembler_input_blockers",
        "host_gate_blocked_assembler_inputs",
        "quality_vfx_dependency_blockers",
        "mtlresidencyset_unavailable",
        "D3d12CommercialQualityHostEvidence",
        "VulkanStrictCommercialQualityHostEvidence",
        "AppleMetalCommercialQualityHostEvidence",
        "RendererMetalMemoryProfilingHostEvidence",
        "RendererPackageCommercialQualityHostEvidence",
        "RendererQualityVfxCommercialHostEvidence",
        "RendererCleanRoomLegalReviewInput",
        "renderer_commercial_readiness=0"
    )) {
    Assert-ContainsText $importerText $needle "renderer commercial readiness final retained-root artifact import script"
}

foreach ($needle in @(
        "check-renderer-commercial-readiness-final-retained-root-artifact-import.ps1",
        "renderer-commercial-readiness-final-retained-root-artifact-import-check: ok",
        "renderer_commercial_readiness_final_retained_root_artifact_import_workflow_artifact_list_present=1",
        "renderer_commercial_readiness_final_retained_root_artifact_import_available_workflow_artifacts=9",
        "renderer_commercial_readiness_final_retained_root_artifact_import_missing_workflow_artifact_names=renderer-clean-room-legal-review-artifacts,renderer-commercial-readiness-final-retained-root",
        "renderer_commercial_readiness_final_retained_root_artifact_import_final_root_workflow_artifact_available=0",
        "renderer_commercial_readiness_final_retained_root_artifact_import_assembler_source_workflow_artifacts=9",
        "renderer_commercial_readiness_final_retained_root_artifact_import_missing_assembler_source_workflow_artifact_names=renderer-clean-room-legal-review-artifacts",
        "renderer_commercial_readiness_final_retained_root_artifact_import_missing_assembler_inputs=7",
        "renderer_commercial_readiness_final_retained_root_artifact_import_missing_assembler_input_names=d3d12_host_evidence,vulkan_strict_host_evidence,apple_metal_host_evidence,metal_memory_profiling_host_evidence,package_host_evidence,quality_vfx_host_evidence,clean_room_legal_review",
        "renderer_commercial_readiness_final_retained_root_artifact_import_host_gate_summaries=3",
        "renderer_commercial_readiness_final_retained_root_artifact_import_host_gate_summary_reasons=mtlresidencyset_unavailable,metal_memory_profiling_host_evidence_required",
        "renderer_commercial_readiness_final_retained_root_artifact_import_host_gate_blocked_assembler_inputs=2",
        "renderer_commercial_readiness_final_retained_root_artifact_import_host_gate_blocked_assembler_input_names=metal_memory_profiling_host_evidence,quality_vfx_host_evidence",
        "renderer_commercial_readiness_final_retained_root_artifact_import_quality_vfx_dependency_blockers=metal_memory_profiling_host_evidence",
        "renderer_commercial_readiness_final_retained_root_artifact_import_supplemental_run_count=1",
        "renderer_commercial_readiness_final_retained_root_artifact_import_supplemental_artifact_names=renderer-metal-memory-profiling-host-artifacts",
        "renderer_commercial_readiness_final_retained_root_artifact_import_quality_vfx_regenerate_requested=1",
        "renderer_commercial_readiness_final_retained_root_artifact_import_quality_vfx_regenerate_ran=1",
        "renderer_commercial_readiness_final_retained_root_artifact_import_quality_vfx_regenerate_ready=1",
        "renderer_commercial_readiness_final_retained_root_artifact_import_present_assembler_inputs=7",
        "renderer_commercial_readiness_final_retained_root_artifact_import_assembler_handoff_ready=1",
        "renderer_commercial_readiness_final_retained_root_artifact_import_final_preflight_handoff_ready=0",
        "renderer_commercial_readiness_final_retained_root_artifact_import_auto_assemble_requested=1",
        "renderer_commercial_readiness_final_retained_root_artifact_import_auto_assemble_ran=1",
        "renderer_commercial_readiness_final_retained_root_artifact_import_auto_assemble_ready=0",
        "renderer_commercial_readiness_final_retained_root_artifact_import_auto_assemble_output_root=",
        "renderer_commercial_readiness_final_retained_root_artifact_import_ready=1",
        "renderer_commercial_readiness=0"
    )) {
    Assert-ContainsText $importerCheckText $needle "renderer commercial readiness final retained-root artifact import check"
    Assert-ContainsText $validateText "check-renderer-commercial-readiness-final-retained-root-artifact-import.ps1" `
        "renderer commercial readiness final retained-root artifact import validate task"
}

foreach ($needle in @(
        "validation_recipe=renderer-metal-memory-profiling-capable-host-runner-preflight",
        "renderer_metal_memory_profiling_capable_host_runner_api_endpoint=",
        "/repos/{owner}/{repo}/actions/runners",
        "renderer_metal_memory_profiling_capable_host_runner_status=",
        "renderer_metal_memory_profiling_capable_host_runner_available=",
        "renderer_metal_memory_profiling_capable_host_runner_required_labels=",
        "self-hosted",
        "macOS",
        "ARM64",
        "metal-residency-set",
        "gh",
        "actions/runners?per_page=100",
        "renderer_commercial_readiness=0"
    )) {
    Assert-ContainsText $runnerPreflightText $needle "renderer Metal memory/profiling capable-host runner preflight"
}

foreach ($needle in @(
        "renderer-metal-memory-profiling-capable-host-runner-check: ok",
        "renderer_metal_memory_profiling_capable_host_runner_api_endpoint=/repos/{owner}/{repo}/actions/runners",
        "renderer_metal_memory_profiling_capable_host_runner_status=ready",
        "renderer_metal_memory_profiling_capable_host_runner_status=host_evidence_required",
        "renderer_metal_memory_profiling_capable_host_runner_available=1",
        "renderer_metal_memory_profiling_capable_host_runner_available=0",
        "renderer_metal_memory_profiling_capable_host_runner_total=2",
        "renderer_metal_memory_profiling_capable_host_runner_total=0",
        "Renderer Metal memory profiling capable host runner is not available",
        "renderer_commercial_readiness=0"
    )) {
    Assert-ContainsText $runnerPreflightCheckText $needle "renderer Metal memory/profiling capable-host runner preflight check"
}
Assert-ContainsText $validateText "check-renderer-metal-memory-profiling-capable-host-runner.ps1" `
    "renderer Metal memory/profiling capable-host runner preflight validate task"

foreach ($needle in @(
        "validation_recipe=renderer-commercial-readiness-final-handoff",
        "renderer_commercial_readiness_final_handoff_status=",
        "renderer_commercial_readiness_final_handoff_next_action=",
        "renderer_commercial_readiness_final_handoff_source_run_ready=",
        "renderer_commercial_readiness_final_handoff_runner_available=",
        "renderer_commercial_readiness_final_handoff_missing_assembler_inputs=",
        "renderer_commercial_readiness_final_handoff_quality_vfx_dependency_blockers=",
        "renderer_commercial_readiness_final_handoff_capable_host_workflow_command=",
        "renderer_commercial_readiness_final_handoff_final_from_runs_workflow_command=",
        "renderer_commercial_readiness_final_handoff_runner_required_labels=",
        "renderer_commercial_readiness_final_handoff_runner_repository_metadata_known=",
        "renderer_commercial_readiness_final_handoff_runner_repository_visibility=",
        "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_confirmed=",
        "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_confirmation_required=",
        "public-repo-self-hosted-runner-risk-reviewed",
        "renderer_commercial_readiness_final_handoff_runner_public_repo_registration_blocked=",
        "renderer_commercial_readiness_final_handoff_runner_registration_token_endpoint=",
        "renderer_commercial_readiness_final_handoff_runner_registration_token_command=",
        "renderer_commercial_readiness_final_handoff_runner_registration_token_expires_minutes=60",
        "renderer_commercial_readiness_final_handoff_runner_config_command_template=",
        "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_required=",
        "renderer_commercial_readiness_final_handoff_runner_label_truth_requires_metal_probe=1",
        "gh workflow run renderer-metal-memory-profiling-capable-host.yml",
        "gh workflow run renderer-commercial-readiness-final-from-runs.yml",
        "actions/runners/registration-token",
        "Invoke-GitHubRepositoryMetadata",
        "complete_public_runner_security_review",
        "./config.sh --url",
        "tools/assemble-renderer-commercial-readiness-final-retained-root.ps1",
        "tools/validate-renderer-commercial-readiness-final-promotion-preflight.ps1",
        "renderer_commercial_readiness=0"
    )) {
    Assert-ContainsText $finalHandoffText $needle "renderer commercial readiness final handoff planner"
}

foreach ($needle in @(
        "renderer-commercial-readiness-final-handoff-check: ok",
        "renderer_commercial_readiness_final_handoff_status=capable_host_runner_required",
        "renderer_commercial_readiness_final_handoff_next_action=provision_capable_host_runner",
        "renderer_commercial_readiness_final_handoff_status=public_runner_security_review_required",
        "renderer_commercial_readiness_final_handoff_next_action=complete_public_runner_security_review",
        "renderer_commercial_readiness_final_handoff_runner_required_labels=self-hosted,macOS,ARM64,metal-residency-set",
        "renderer_commercial_readiness_final_handoff_runner_repository_metadata_known=1",
        "renderer_commercial_readiness_final_handoff_runner_repository_visibility=public",
        "renderer_commercial_readiness_final_handoff_runner_repository_private=0",
        "renderer_commercial_readiness_final_handoff_runner_registration_token_endpoint=/repos/owner/repo/actions/runners/registration-token",
        "renderer_commercial_readiness_final_handoff_runner_registration_token_command=gh api -X POST",
        "renderer_commercial_readiness_final_handoff_runner_registration_token_expires_minutes=60",
        "renderer_commercial_readiness_final_handoff_runner_config_command_template=./config.sh --url https://github.com/owner/repo --token <registration-token> --labels metal-residency-set",
        "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_required=1",
        "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_confirmed=0",
        "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_confirmation_required=public-repo-self-hosted-runner-risk-reviewed",
        "renderer_commercial_readiness_final_handoff_runner_public_repo_registration_blocked=1",
        "renderer_commercial_readiness_final_handoff_runner_label_truth_requires_metal_probe=1",
        "renderer_commercial_readiness_final_handoff_status=metal_memory_profiling_run_required",
        "renderer_commercial_readiness_final_handoff_next_action=run_metal_memory_profiling_capable_host_workflow",
        "renderer_commercial_readiness_final_handoff_status=ready_for_final_from_runs_workflow",
        "renderer_commercial_readiness_final_handoff_final_from_runs_workflow_command=gh workflow run renderer-commercial-readiness-final-from-runs.yml",
        "renderer_commercial_readiness_final_handoff_status=ready_for_final_preflight",
        "renderer_commercial_readiness_final_handoff_final_preflight_command=pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-renderer-commercial-readiness-final-promotion-preflight.ps1",
        "renderer_commercial_readiness=0"
    )) {
    Assert-ContainsText $finalHandoffCheckText $needle "renderer commercial readiness final handoff planner check"
}
Assert-ContainsText $validateText "check-renderer-commercial-readiness-final-handoff.ps1" `
    "renderer commercial readiness final handoff validate task"

foreach ($needle in @(
        "rendererCommercialReadinessFinalRetainedRootArtifactImport",
        "rendererCommercialReadinessFinalRetainedRootFromRunsWorkflow",
        "rendererMetalMemoryProfilingCapableHostRunnerPreflight",
        "rendererCommercialReadinessFinalHandoff",
        "validate-renderer-commercial-readiness-final-handoff.ps1",
        "validate-renderer-metal-memory-profiling-capable-host-runner.ps1",
        "gh workflow run renderer-commercial-readiness-final-from-runs.yml",
        "confirm_final_retained_root_handoff=renderer-commercial-final-retained-root",
        "tools/import-renderer-commercial-readiness-final-retained-root-artifacts.ps1"
    )) {
    Assert-ContainsText $commandsFragmentText $needle "renderer commercial readiness final retained-root artifact import command"
}

foreach ($needle in @(
        "renderer-metal-memory-profiling-capable-host-runner-preflight",
        "tools/validate-renderer-metal-memory-profiling-capable-host-runner.ps1",
        "/repos/{owner}/{repo}/actions/runners",
        "/repos/{owner}/{repo}/actions/runners/registration-token",
        "GET /repos/{owner}/{repo}",
        "RepositoryJsonPath",
        "ConfirmPublicRepoSelfHostedRunnerSecurityReview",
        "public-repo-self-hosted-runner-risk-reviewed",
        "complete_public_runner_security_review",
        "self-hosted, macOS, ARM64, and metal-residency-set",
        "registration token expires after one hour",
        "public repository security review",
        "Metal probe truth",
        "renderer-commercial-readiness-final-handoff",
        "tools/validate-renderer-commercial-readiness-final-handoff.ps1",
        "final-handoff-plan-only",
        "renderer-commercial-readiness-final-retained-root-artifact-import",
        "tools/import-renderer-commercial-readiness-final-retained-root-artifacts.ps1",
        "GitHub Actions artifacts",
        "seven explicit assembler inputs",
        "assembler_handoff",
        "final_preflight_handoff",
        "renderer_commercial_readiness=0"
    )) {
    Assert-ContainsText $validationRecipesFragmentText $needle "renderer commercial readiness final retained-root artifact import validation recipe"
}

foreach ($needle in @(
        "renderer-metal-memory-profiling-capable-host-runner-preflight",
        "validate-renderer-metal-memory-profiling-capable-host-runner.ps1",
        "host-evidence-required",
        "renderer-commercial-readiness-final-handoff",
        "validate-renderer-commercial-readiness-final-handoff.ps1",
        "final-handoff-plan-only",
        "renderer-commercial-readiness-final-promotion-preflight",
        "renderer-commercial-readiness-final-retained-root-assembler",
        "renderer-commercial-readiness-final-retained-root-artifact-import",
        "renderer-clean-room-legal-review-input",
        "assemble-renderer-commercial-readiness-final-retained-root.ps1",
        "generate-renderer-clean-room-legal-review-input.ps1",
        "import-renderer-commercial-readiness-final-retained-root-artifacts.ps1",
        "artifact-intake-plan-only",
        "clean-room-review-input-plan-only",
        "final-retained-root-plan-only"
    )) {
    Assert-ContainsText $recipePlansText $needle "renderer commercial readiness final retained-root validation recipe runner plan"
}

foreach ($needle in @(
        'Assert-DryRunRecipe -Recipe "renderer-metal-memory-profiling-capable-host-runner-preflight"',
        'Assert-DryRunRecipe -Recipe "renderer-commercial-readiness-final-handoff"',
        'Assert-DryRunRecipe -Recipe "renderer-commercial-readiness-final-promotion-preflight"',
        'Assert-DryRunRecipe -Recipe "renderer-commercial-readiness-final-retained-root-assembler"',
        'Assert-DryRunRecipe -Recipe "renderer-clean-room-legal-review-input"',
        'Assert-DryRunRecipe -Recipe "renderer-commercial-readiness-final-retained-root-artifact-import"',
        'Assert-ArgvDoesNotContainText -Result $rendererFinalArtifactImportDryRun -Unexpected "-RunId"'
    )) {
    Assert-ContainsText $recipeRunnerCheckText $needle "renderer commercial readiness final retained-root validation recipe runner check"
}

foreach ($surface in @(
        @{ Text = $productionLoopFragmentText; Label = "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json" },
        @{ Text = $manifestText; Label = "engine/agent/manifest.json" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $planText; Label = "renderer commercial readiness evidence promotion plan" }
    )) {
    foreach ($needle in @(
            "renderer-commercial-readiness-final-retained-root-artifact-import",
            "renderer-metal-memory-profiling-capable-host-runner-preflight",
            "renderer-commercial-readiness-final-handoff",
            "rendererCommercialReadinessFinalHandoff",
            "tools/validate-renderer-metal-memory-profiling-capable-host-runner.ps1",
            "tools/validate-renderer-commercial-readiness-final-handoff.ps1",
            "tools/check-renderer-commercial-readiness-final-handoff.ps1",
            "/repos/{owner}/{repo}/actions/runners",
            "/repos/{owner}/{repo}/actions/runners/registration-token",
            "GET /repos/{owner}/{repo}",
            "RepositoryJsonPath",
            "ConfirmPublicRepoSelfHostedRunnerSecurityReview",
            "public-repo-self-hosted-runner-risk-reviewed",
            "complete_public_runner_security_review",
            "self-hosted, macOS, ARM64, metal-residency-set",
            "registration token expires after one hour",
            "public repository security review",
            "Metal probe truth",
            "renderer_metal_memory_profiling_capable_host_runner_available",
            "tools/import-renderer-commercial-readiness-final-retained-root-artifacts.ps1",
            "tools/check-renderer-commercial-readiness-final-retained-root-artifact-import.ps1",
            "GitHub Actions artifacts",
            "artifact_handoff_strategy",
            "missing_assembler_inputs",
            "host_gate_summaries",
            "assembler_input_blockers",
            "auto_assemble",
            ".github/workflows/renderer-commercial-readiness-final-from-runs.yml",
            "renderer-commercial-readiness-final-from-runs",
            "renderer-commercial-readiness-final-from-runs-artifact-intake",
            "source_artifact_run_id",
            "metal_memory_profiling_run_id",
            "RegenerateQualityVfx",
            "AutoAssemble",
            "quality_vfx_dependency_blockers",
            "next_action",
            "renderer_commercial_readiness=0"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) final retained-root artifact import"
    }
}

foreach ($surface in @(
        @{ Text = $finalFromRunsWorkflowText; Label = ".github/workflows/renderer-commercial-readiness-final-from-runs.yml" },
        @{ Text = $ciMatrixCheckText; Label = "tools/check-ci-matrix.ps1" }
    )) {
    foreach ($needle in @(
            "Renderer Commercial Readiness Final From Runs",
            "renderer-commercial-final-from-runs",
            "source_artifact_run_id",
            "metal_memory_profiling_run_id",
            "confirm_final_retained_root_handoff",
            "renderer-commercial-final-retained-root",
            "actions: read",
            'actions/runs/${{ inputs.source_artifact_run_id }}/artifacts?per_page=100',
            "renderer-commercial-readiness-final-from-runs-artifact-intake",
            "renderer-commercial-readiness-final-retained-root",
            "SupplementalRunIds",
            "renderer-metal-memory-profiling-host-artifacts",
            "RegenerateQualityVfx",
            "AutoAssemble",
            "renderer_commercial_readiness=0"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) final-from-runs workflow"
    }
}

foreach ($surface in @(
        @{ Text = $validateWorkflowText; Label = ".github/workflows/validate.yml" },
        @{ Text = $ciMatrixCheckText; Label = "tools/check-ci-matrix.ps1" }
    )) {
    foreach ($needle in @(
            "renderer-commercial-artifact-intake",
            "Renderer Commercial Artifact Intake",
            "Collect renderer quality/VFX commercial host evidence",
            "tools/collect-renderer-quality-vfx-commercial-host-evidence.ps1",
            "renderer-quality-vfx-commercial-artifacts",
            "gh api -H",
            'actions/runs/${{ github.run_id }}/artifacts?per_page=100',
            "current-run-artifact-intake",
            "renderer-commercial-readiness-current-run-artifact-intake",
            "actions: read",
            "renderer_commercial_readiness=0"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) current-run artifact intake"
    }
}
