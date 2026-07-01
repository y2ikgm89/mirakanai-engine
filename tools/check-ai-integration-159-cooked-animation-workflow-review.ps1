#requires -Version 7.0
#requires -PSEdition Core
# Chapter 159 for check-ai-integration.ps1 cooked animation workflow review gate.

$headerText = Get-AgentSurfaceText "engine/tools/include/mirakana/tools/cooked_animation_workflow_review.hpp"
$sourceText = Get-AgentSurfaceText "engine/tools/asset/cooked_animation_workflow_review.cpp"
$testsText = Get-AgentSurfaceText "tests/unit/tools_cooked_animation_workflow_review_tests.cpp"
$assetCMakeText = Get-AgentSurfaceText "engine/tools/asset/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-07-02-cooked-animation-asset-workflow-review-v1.md"

foreach ($needle in @(
        "CookedAnimationWorkflowEvidenceKind",
        "CookedAnimationWorkflowReviewStatus",
        "CookedAnimationWorkflowDiagnosticCode",
        "CookedAnimationWorkflowEvidenceRow",
        "CookedAnimationWorkflowReviewDesc",
        "CookedAnimationWorkflowReviewResult",
        "review_cooked_animation_asset_workflow",
        "has_cooked_animation_workflow_diagnostic",
        "std::string row_id;",
        "std::string official_source_id;",
        "invalid_row_id",
        "duplicate_row_id",
        "unexpected_official_source",
        "bool cooked_animation_asset_workflow_review_ready{false};",
        "bool cooked_animation_quaternion_clip_workflow_ready{false};",
        "bool cooked_animation_runtime_gltf_parsing_ready{false};",
        "bool cooked_animation_animation_graph_ready{false};",
        "bool cooked_animation_retargeting_ready{false};",
        "bool cooked_animation_renderer_rhi_execution_ready{false};",
        "bool cooked_animation_broad_skeletal_animation_ready{false};",
        "bool cooked_animation_native_handles_exposed{false};",
        "bool cooked_animation_external_engine_compatibility{false};",
        "bool cooked_animation_legal_approval{false};"
    )) {
    Assert-ContainsText $headerText $needle "cooked_animation_workflow_review.hpp public contract"
}

foreach ($forbiddenNeedle in @(
        "std::string_view row_id",
        "std::string_view official_source_id",
        "#include <windows.h>",
        "#include <d3d12.h>",
        "#include <vulkan/",
        "ID3D12",
        "VkDevice",
        "MTLDevice",
        "HANDLE",
        "void*"
    )) {
    Assert-DoesNotContainText $headerText $forbiddenNeedle "cooked_animation_workflow_review.hpp ownership/native boundary"
    Assert-DoesNotContainText $sourceText $forbiddenNeedle "cooked_animation_workflow_review.cpp ownership/native boundary"
}

foreach ($needle in @(
        "expected_row_id",
        "expected_official_source_id",
        "cooked_animation.workflow.gltf_spec",
        "cooked_animation.workflow.gltf_quaternion_trs_import",
        "cooked_animation.workflow.source_document",
        "cooked_animation.workflow.cooked_payload",
        "cooked_animation.workflow.runtime_payload",
        "cooked_animation.workflow.generated_package_smoke",
        "cooked_animation.workflow.clean_room_legal_boundary",
        "khronos-gltf-2.0-spec-animation",
        "mirakanai-gltf-node-animation-import",
        "mirakanai-animation-quaternion-clip-source-v1",
        "mirakanai-cooked-animation-quaternion-clip-v1",
        "mirakanai-runtime-animation-quaternion-clip-payload",
        "mirakanai-generated-3d-quaternion-package-smoke",
        "mirakanai-clean-room-legal-boundary",
        "Cooked animation workflow evidence row id does not match its required row kind",
        "Cooked animation workflow evidence row ids must be unique",
        "Cooked animation workflow evidence official source id does not match its required row kind",
        "Cooked animation workflow review must not claim runtime glTF parsing readiness",
        "Cooked animation workflow review must not claim animation graph readiness",
        "Cooked animation workflow review must not claim retargeting readiness",
        "Cooked animation workflow review must not claim renderer/RHI execution readiness",
        "Cooked animation workflow review must not claim broad skeletal animation readiness",
        "Cooked animation workflow review rows must not expose native handles",
        "Cooked animation workflow review must not claim copied external engine code, assets, or shaders",
        "Cooked animation workflow review must not claim Unity, Unreal, or Godot compatibility",
        "Cooked animation workflow review is engineering evidence and must not claim legal approval"
    )) {
    Assert-ContainsText $sourceText $needle "cooked_animation_workflow_review.cpp fail-closed implementation"
}

foreach ($needle in @(
        "cooked animation workflow review accepts selected quaternion clip evidence",
        "cooked animation workflow review requires official gltf spec obligations",
        "cooked animation workflow review requires exact evidence identities",
        "cooked animation workflow review rejects duplicate and blank evidence identities",
        "cooked animation workflow review requires all selected workflow rows",
        "cooked animation workflow review blocks unsupported broad and external claims",
        "cooked animation workflow review requires clean room legal boundary evidence",
        "unexpected_official_source",
        "invalid_row_id",
        "duplicate_row_id",
        "missing_row_id",
        "missing_official_source",
        "runtime_gltf_parsing_claim_not_allowed",
        "external_engine_claim_not_allowed",
        "legal_approval_claim_not_allowed"
    )) {
    Assert-ContainsText $testsText $needle "MK_tools_cooked_animation_workflow_review_tests coverage"
}

Assert-ContainsText $assetCMakeText "cooked_animation_workflow_review.cpp" "engine/tools/asset/CMakeLists.txt cooked animation workflow review source registration"
Assert-ContainsText $rootCMakeText "MK_tools_cooked_animation_workflow_review_tests" "root CMake cooked animation workflow review test target"
Assert-ContainsText $modulesFragmentText "engine/tools/include/mirakana/tools/cooked_animation_workflow_review.hpp" "engine/agent/manifest.fragments/004-modules.json MK_tools public header"

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $planText; Label = "Cooked Animation Asset Workflow Review plan" },
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" },
        @{ Text = $manifestText; Label = "engine/agent/manifest.json" }
    )) {
    foreach ($needle in @(
            "cooked-animation-asset-workflow-review-v1",
            "cooked_animation_workflow_review.hpp",
            "CookedAnimationWorkflowEvidenceRow",
            "CookedAnimationWorkflowReviewResult",
            "review_cooked_animation_asset_workflow",
            "khronos-gltf-2.0-spec-animation",
            "mirakanai-gltf-node-animation-import",
            "mirakanai-animation-quaternion-clip-source-v1",
            "mirakanai-cooked-animation-quaternion-clip-v1",
            "mirakanai-runtime-animation-quaternion-clip-payload",
            "mirakanai-generated-3d-quaternion-package-smoke",
            "mirakanai-clean-room-legal-boundary",
            "cooked_animation_asset_workflow_review_ready=1",
            "cooked_animation_quaternion_clip_workflow_ready=1",
            "cooked_animation_gltf_animation_spec_reviewed=1",
            "cooked_animation_gltf_quaternion_import_reviewed=1",
            "cooked_animation_source_document_ready=1",
            "cooked_animation_cooked_payload_ready=1",
            "cooked_animation_runtime_payload_ready=1",
            "cooked_animation_generated_package_smoke_ready=1",
            "cooked_animation_clean_room_legal_boundary_ready=1",
            "cooked_animation_runtime_gltf_parsing_ready=0",
            "cooked_animation_animation_graph_ready=0",
            "cooked_animation_retargeting_ready=0",
            "cooked_animation_renderer_rhi_execution_ready=0",
            "cooked_animation_broad_skeletal_animation_ready=0",
            "cooked_animation_native_handles_exposed=0",
            "cooked_animation_external_engine_compatibility=0",
            "cooked_animation_legal_approval=0",
            "Unity",
            "Unreal",
            "Godot",
            "legal approval"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) cooked animation workflow review evidence"
    }

    foreach ($forbiddenNeedle in @(
            "cooked_animation_runtime_gltf_parsing_ready=1",
            "cooked_animation_animation_graph_ready=1",
            "cooked_animation_retargeting_ready=1",
            "cooked_animation_renderer_rhi_execution_ready=1",
            "cooked_animation_broad_skeletal_animation_ready=1",
            "cooked_animation_native_handles_exposed=1",
            "cooked_animation_external_engine_compatibility=1",
            "cooked_animation_legal_approval=1"
        )) {
        Assert-DoesNotContainText $surface.Text $forbiddenNeedle "$($surface.Label) forbidden cooked animation ready/legal claim"
    }
}

foreach ($needle in @(
        "cooked-animation-asset-workflow-review-v1",
        "cooked_animation_workflow_review.hpp",
        "review_cooked_animation_asset_workflow",
        "Khronos glTF 2.0 animation spec",
        "runtime glTF parsing",
        "animation graphs",
        "retargeting",
        "renderer/RHI execution",
        "broad skeletal animation readiness",
        "native handles",
        "Unity/Unreal/Godot compatibility",
        "copied external-engine material",
        "legal approval"
    )) {
    Assert-ContainsText $roadmapText $needle "docs/roadmap.md cooked animation workflow review summary"
}
foreach ($forbiddenNeedle in @(
        "cooked_animation_runtime_gltf_parsing_ready=1",
        "cooked_animation_animation_graph_ready=1",
        "cooked_animation_retargeting_ready=1",
        "cooked_animation_renderer_rhi_execution_ready=1",
        "cooked_animation_broad_skeletal_animation_ready=1",
        "cooked_animation_native_handles_exposed=1",
        "cooked_animation_external_engine_compatibility=1",
        "cooked_animation_legal_approval=1"
    )) {
    Assert-DoesNotContainText $roadmapText $forbiddenNeedle "docs/roadmap.md forbidden cooked animation ready/legal claim"
}

$manifest = $manifestText | ConvertFrom-Json
$toolsModule = @($manifest.modules | Where-Object { $_.name -eq "MK_tools" })
if ($toolsModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_tools module"
}
$toolsManifestText = ((@($toolsModule[0].publicHeaders) -join " "),
    (@($toolsModule[0].recentEvidence) -join " "),
    [string]$toolsModule[0].purpose) -join " "
foreach ($needle in @(
        "engine/tools/include/mirakana/tools/cooked_animation_workflow_review.hpp",
        "CookedAnimationWorkflowReviewResult",
        "review_cooked_animation_asset_workflow",
        "cooked_animation_asset_workflow_review_ready=1",
        "cooked_animation_runtime_gltf_parsing_ready=0",
        "cooked_animation_broad_skeletal_animation_ready=0",
        "cooked_animation_external_engine_compatibility=0",
        "cooked_animation_legal_approval=0"
    )) {
    Assert-ContainsText $toolsManifestText $needle "engine/agent/manifest.json MK_tools cooked animation workflow evidence"
}
