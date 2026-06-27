#requires -Version 7.0
#requires -PSEdition Core
# Chapter 75 for check-json-contracts.ps1 renderer commercial readiness final retained-root artifact import.

$importerText = Get-JsonContractSurfaceText "tools/import-renderer-commercial-readiness-final-retained-root-artifacts.ps1"
$importerCheckText = Get-JsonContractSurfaceText "tools/check-renderer-commercial-readiness-final-retained-root-artifact-import.ps1"
$runnerPreflightText = Get-JsonContractSurfaceText "tools/validate-renderer-metal-memory-profiling-capable-host-runner.ps1"
$runnerPreflightCheckText = Get-JsonContractSurfaceText "tools/check-renderer-metal-memory-profiling-capable-host-runner.ps1"
$publicRunnerReviewText = Get-JsonContractSurfaceText "tools/generate-renderer-public-runner-security-review.ps1"
$publicRunnerReviewCheckText = Get-JsonContractSurfaceText "tools/check-renderer-public-runner-security-review.ps1"
$finalHandoffText = Get-JsonContractSurfaceText "tools/validate-renderer-commercial-readiness-final-handoff.ps1"
$finalHandoffCheckText = Get-JsonContractSurfaceText "tools/check-renderer-commercial-readiness-final-handoff.ps1"
$commandsFragmentText = Get-JsonContractSurfaceText "engine/agent/manifest.fragments/002-commands.json"
$validationRecipesFragmentText = Get-JsonContractSurfaceText "engine/agent/manifest.fragments/009-validationRecipes.json"
$productionLoopFragmentText = Get-JsonContractSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$validateWorkflowText = Get-JsonContractSurfaceText ".github/workflows/validate.yml"
$finalFromRunsWorkflowText = Get-JsonContractSurfaceText ".github/workflows/renderer-commercial-readiness-final-from-runs.yml"
$ciMatrixCheckText = Get-JsonContractSurfaceText "tools/check-ci-matrix.ps1"

foreach ($needle in @(
        "renderer-commercial-readiness-final-retained-root-artifact-import",
        "GameEngine.RendererCommercialReadinessFinalRetainedRootArtifactImport.v1",
        "ArtifactListJsonRelative",
        "SupplementalRunIds",
        "SupplementalArtifactNames",
        "RegenerateQualityVfx",
        "RegeneratedQualityVfxOutputRootRelative",
        "workflow_artifact_list",
        "artifact_handoff_strategy",
        "supplemental_import",
        "quality_vfx_regenerate",
        "final_retained_root_artifact",
        "assembler_source_artifacts",
        "assembler_handoff",
        "final_preflight_handoff",
        "AutoAssemble",
        "auto_assemble",
        "auto_assemble_requested",
        "auto_assemble_output_log",
        "renderer_commercial_readiness_final_retained_root_artifact_import_workflow_artifact_list_present",
        "renderer_commercial_readiness_final_retained_root_artifact_import_missing_workflow_artifact_names",
        "renderer_commercial_readiness_final_retained_root_artifact_import_final_root_workflow_artifact_available",
        "renderer_commercial_readiness_final_retained_root_artifact_import_missing_assembler_source_workflow_artifact_names",
        "renderer_commercial_readiness_final_retained_root_artifact_import_supplemental_run_count",
        "renderer_commercial_readiness_final_retained_root_artifact_import_supplemental_artifact_names",
        "renderer_commercial_readiness_final_retained_root_artifact_import_quality_vfx_regenerate_ready",
        "missing_assembler_inputs",
        "renderer_commercial_readiness_final_retained_root_artifact_import_assembler_handoff_ready",
        "renderer_commercial_readiness_final_retained_root_artifact_import_final_preflight_handoff_ready",
        "gh run download",
        "https://cli.github.com/manual/gh_run_download",
        "https://docs.github.com/en/rest/actions/artifacts",
        "renderer-clean-room-legal-review-artifacts",
        "renderer-commercial-readiness-final-retained-root",
        "renderer-metal-memory-profiling-host-artifacts",
        "renderer-d3d12-commercial-quality-host-evidence",
        "renderer-vulkan-strict-commercial-quality-host-evidence",
        "renderer-apple-metal-commercial-quality-host-evidence",
        "renderer-package-commercial-quality-host-evidence",
        "renderer-quality-vfx-commercial-artifacts",
        "metal-host-optimization-artifacts",
        "linux-vulkan-host-evidence",
        "windows-packages",
        "GameEngine.RendererD3d12CommercialQualityHostEvidence.v1",
        "GameEngine.RendererVulkanStrictCommercialQualityHostEvidence.v1",
        "GameEngine.RendererAppleMetalCommercialQualityHostEvidence.v1",
        "GameEngine.RendererMetalMemoryProfilingHostEvidence.v1",
        "GameEngine.RendererPackageCommercialQualityHostEvidence.v1",
        "GameEngine.RendererQualityVfxCommercialHostEvidence.v1",
        "GameEngine.RendererCleanRoomLegalReviewInput.v1",
        "host-gate-summary.json",
        "host_gate_summaries",
        "host_gate_summary_reasons",
        "assembler_input_blockers",
        "host_gate_blocked_assembler_inputs",
        "quality_vfx_dependency_blockers",
        "renderer_commercial_readiness_final_retained_root_artifact_import_ready",
        "renderer_commercial_readiness=0",
        "renderer_environment_ready=0"
    )) {
    Assert-ContainsText $importerText $needle "renderer commercial readiness final retained-root artifact import script"
}

foreach ($needle in @(
        "renderer-commercial-readiness-final-retained-root-artifact-import-check: ok",
        "renderer_commercial_readiness_final_retained_root_artifact_import_required_workflow_artifacts=11",
        "renderer_commercial_readiness_final_retained_root_artifact_import_required_assembler_inputs=7",
        "renderer_commercial_readiness_final_retained_root_artifact_import_workflow_artifact_list_present=1",
        "renderer_commercial_readiness_final_retained_root_artifact_import_available_workflow_artifacts=9",
        "renderer_commercial_readiness_final_retained_root_artifact_import_missing_workflow_artifact_names=renderer-clean-room-legal-review-artifacts,renderer-commercial-readiness-final-retained-root",
        "renderer_commercial_readiness_final_retained_root_artifact_import_final_root_workflow_artifact_available=0",
        "renderer_commercial_readiness_final_retained_root_artifact_import_assembler_source_workflow_artifacts=9",
        "renderer_commercial_readiness_final_retained_root_artifact_import_missing_assembler_source_workflow_artifact_names=renderer-clean-room-legal-review-artifacts",
        "renderer_commercial_readiness_final_retained_root_artifact_import_missing_assembler_inputs=7",
        "renderer_commercial_readiness_final_retained_root_artifact_import_missing_assembler_input_names=d3d12_host_evidence,vulkan_strict_host_evidence,apple_metal_host_evidence,metal_memory_profiling_host_evidence,package_host_evidence,quality_vfx_host_evidence,clean_room_legal_review",
        "renderer_commercial_readiness_final_retained_root_artifact_import_metal_host_gate_reason=mtlresidencyset_unavailable",
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
}

foreach ($needle in @(
        "validation_recipe=renderer-metal-memory-profiling-capable-host-runner-preflight",
        "renderer_metal_memory_profiling_capable_host_runner_api_endpoint=",
        "/repos/{owner}/{repo}/actions/runners",
        "renderer_metal_memory_profiling_capable_host_runner_available=",
        "renderer_metal_memory_profiling_capable_host_runner_total=",
        "renderer_metal_memory_profiling_capable_host_runner_required_labels=",
        "actions/runners?per_page=100",
        "renderer_commercial_readiness=0"
    )) {
    Assert-ContainsText $runnerPreflightText $needle "renderer Metal memory/profiling capable-host runner preflight script"
}

foreach ($needle in @(
        "renderer-metal-memory-profiling-capable-host-runner-check: ok",
        "renderer_metal_memory_profiling_capable_host_runner_api_endpoint=/repos/{owner}/{repo}/actions/runners",
        "renderer_metal_memory_profiling_capable_host_runner_available=1",
        "renderer_metal_memory_profiling_capable_host_runner_available=0",
        "Renderer Metal memory profiling capable host runner is not available"
    )) {
    Assert-ContainsText $runnerPreflightCheckText $needle "renderer Metal memory/profiling capable-host runner preflight check"
}

foreach ($needle in @(
        "validation_recipe=renderer-public-runner-security-review",
        "GameEngine.RendererPublicSelfHostedRunnerSecurityReview.v1",
        "ApprovePublicRepoSelfHostedRunnerReview",
        "public-repo-self-hosted-runner-risk-reviewed",
        "ReviewedPublicForkPrRisk",
        "ReviewedRunnerIsolation",
        "ReviewedSecretExposure",
        "ReviewedMetalProbeTruth",
        "renderer_public_runner_security_review_status=",
        "renderer_public_runner_security_review_ready=",
        "renderer_public_runner_security_review_blockers=",
        "renderer_public_runner_security_review_artifact_written=",
        "renderer_public_runner_security_review_registration_token_endpoint=",
        "renderer_public_runner_security_review_registration_token_expires_minutes=60",
        "renderer_public_runner_security_review_registration_token_fetched=0",
        "renderer_public_runner_security_review_registration_token_printed=0",
        "renderer_public_runner_security_review_workflow_dispatched=0",
        "reviewed_allowed_workflows",
        "reviewed_required_labels",
        "reviewed_metal_probe_truth",
        "registration_token_fetched",
        "workflow_dispatched",
        "renderer_commercial_readiness=0"
    )) {
    Assert-ContainsText $publicRunnerReviewText $needle "renderer public runner security review producer"
}

foreach ($needle in @(
        "renderer-public-runner-security-review-check: ok",
        "renderer_public_runner_security_review_status=review_required",
        "renderer_public_runner_security_review_status=approved",
        "renderer_public_runner_security_review_ready=1",
        "renderer_public_runner_security_review_blockers=approval_confirmation_required,public_fork_pr_risk_review_required,runner_isolation_review_required,secret_exposure_review_required,metal_probe_truth_review_required",
        "renderer_public_runner_security_review_artifact_written=1",
        "renderer_public_runner_security_review_registration_token_fetched=0",
        "renderer_public_runner_security_review_workflow_dispatched=0",
        "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_artifact_valid=1",
        "renderer_commercial_readiness=0"
    )) {
    Assert-ContainsText $publicRunnerReviewCheckText $needle "renderer public runner security review producer check"
}

foreach ($needle in @(
        "validation_recipe=renderer-commercial-readiness-final-handoff",
        "renderer_commercial_readiness_final_handoff_status=",
        "renderer_commercial_readiness_final_handoff_next_action=",
        "renderer_commercial_readiness_final_handoff_source_run_ready=",
        "renderer_commercial_readiness_final_handoff_metal_memory_profiling_run_ready=",
        "renderer_commercial_readiness_final_handoff_runner_available=",
        "renderer_commercial_readiness_final_handoff_missing_assembler_inputs=",
        "renderer_commercial_readiness_final_handoff_quality_vfx_dependency_blockers=",
        "renderer_commercial_readiness_final_handoff_capable_host_workflow_command=",
        "renderer_commercial_readiness_final_handoff_final_from_runs_workflow_command=",
        "renderer_commercial_readiness_final_handoff_final_preflight_command=",
        "renderer_commercial_readiness_final_handoff_runner_required_labels=",
        "renderer_commercial_readiness_final_handoff_runner_repository_metadata_known=",
        "renderer_commercial_readiness_final_handoff_runner_repository_visibility=",
        "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_confirmed=",
        "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_confirmation_required=",
        "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_artifact_present=",
        "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_artifact_valid=",
        "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_artifact_status=",
        "public-repo-self-hosted-runner-risk-reviewed",
        "PublicRepoRunnerSecurityReviewRelative",
        "GameEngine.RendererPublicSelfHostedRunnerSecurityReview.v1",
        "reviewed_public_fork_pr_risk",
        "reviewed_runner_isolation",
        "reviewed_secret_exposure",
        "reviewed_allowed_workflows",
        "reviewed_required_labels",
        "reviewed_metal_probe_truth",
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
        "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_artifact_present=1",
        "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_artifact_valid=1",
        "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_artifact_status=approved",
        "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_artifact_valid=0",
        "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_artifact_status=pending",
        "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_confirmation_required=public-repo-self-hosted-runner-risk-reviewed",
        "renderer_commercial_readiness_final_handoff_runner_public_repo_registration_blocked=1",
        "renderer_commercial_readiness_final_handoff_runner_label_truth_requires_metal_probe=1",
        "renderer_commercial_readiness_final_handoff_status=metal_memory_profiling_run_required",
        "renderer_commercial_readiness_final_handoff_status=ready_for_final_from_runs_workflow",
        "renderer_commercial_readiness_final_handoff_status=ready_for_final_preflight",
        "renderer_commercial_readiness_final_handoff_next_action=run_final_from_runs_workflow",
        "renderer_commercial_readiness_final_handoff_final_from_runs_workflow_command=gh workflow run renderer-commercial-readiness-final-from-runs.yml",
        "renderer_commercial_readiness=0"
    )) {
    Assert-ContainsText $finalHandoffCheckText $needle "renderer commercial readiness final handoff planner check"
}

foreach ($needle in @(
        "rendererCommercialReadinessFinalRetainedRootArtifactImport",
        "rendererCommercialReadinessFinalRetainedRootFromRunsWorkflow",
        "rendererMetalMemoryProfilingCapableHostRunnerPreflight",
        "rendererPublicRunnerSecurityReviewGenerator",
        "rendererCommercialReadinessFinalHandoff",
        "validate-renderer-commercial-readiness-final-handoff.ps1",
        "generate-renderer-public-runner-security-review.ps1",
        "PublicRepoRunnerSecurityReviewRelative",
        "validate-renderer-metal-memory-profiling-capable-host-runner.ps1",
        "gh workflow run renderer-commercial-readiness-final-from-runs.yml",
        "tools/import-renderer-commercial-readiness-final-retained-root-artifacts.ps1"
    )) {
    Assert-ContainsText $commandsFragmentText $needle "renderer commercial readiness final retained-root artifact import command"
}

foreach ($needle in @(
        "renderer-metal-memory-profiling-capable-host-runner-preflight",
        "tools/validate-renderer-metal-memory-profiling-capable-host-runner.ps1",
        "renderer-public-runner-security-review",
        "tools/generate-renderer-public-runner-security-review.ps1",
        "/repos/{owner}/{repo}/actions/runners",
        "/repos/{owner}/{repo}/actions/runners/registration-token",
        "GET /repos/{owner}/{repo}",
        "RepositoryJsonPath",
        "ConfirmPublicRepoSelfHostedRunnerSecurityReview",
        "PublicRepoRunnerSecurityReviewRelative",
        "GameEngine.RendererPublicSelfHostedRunnerSecurityReview.v1",
        "ApprovePublicRepoSelfHostedRunnerReview",
        "ReviewedPublicForkPrRisk",
        "ReviewedRunnerIsolation",
        "ReviewedSecretExposure",
        "ReviewedMetalProbeTruth",
        "registration_token_fetched=false",
        "registration_token_printed=false",
        "workflow_dispatched=false",
        "public-repo-self-hosted-runner-risk-reviewed",
        "review_status=approved",
        "reviewed_public_fork_pr_risk=true",
        "reviewed_runner_isolation=true",
        "reviewed_secret_exposure=true",
        "reviewed_metal_probe_truth=true",
        "reviewed_allowed_workflows including renderer-metal-memory-profiling-capable-host.yml",
        "reviewed_required_labels exactly self-hosted,macOS,ARM64,metal-residency-set",
        "runner_public_repo_security_review_artifact_present",
        "runner_public_repo_security_review_artifact_valid",
        "runner_public_repo_security_review_artifact_status",
        "complete_public_runner_security_review",
        "self-hosted, macOS, ARM64, and metal-residency-set",
        "registration token expires after one hour",
        "public repository security review",
        "Metal probe truth",
        "renderer-commercial-readiness-final-handoff",
        "tools/validate-renderer-commercial-readiness-final-handoff.ps1",
        "final-handoff-plan-only",
        "renderer-commercial-readiness-final-retained-root-artifact-import",
        "renderer-clean-room-legal-review-input",
        "tools/import-renderer-commercial-readiness-final-retained-root-artifacts.ps1",
        "tools/generate-renderer-clean-room-legal-review-input.ps1",
        ".github/workflows/renderer-commercial-readiness-final-from-runs.yml",
        "renderer-commercial-readiness-final-from-runs",
        "renderer-commercial-readiness-final-from-runs-artifact-intake",
        "renderer-commercial-readiness-final-handoff",
        "rendererCommercialReadinessFinalHandoff",
        "validate-renderer-commercial-readiness-final-handoff.ps1",
        "quality_vfx_dependency_blockers",
        "next_action",
        "source_artifact_run_id",
        "metal_memory_profiling_run_id",
        "RegenerateQualityVfx",
        "AutoAssemble",
        "GitHub Actions artifacts",
        "renderer-clean-room-legal-review-artifacts",
        "final retained root",
        "seven explicit assembler inputs",
        "assembler_handoff",
        "final_preflight_handoff"
    )) {
    Assert-ContainsText $validationRecipesFragmentText $needle "renderer commercial readiness final retained-root artifact import validation recipe"
    Assert-ContainsText $productionLoopFragmentText $needle "renderer commercial readiness final retained-root artifact import production loop"
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
