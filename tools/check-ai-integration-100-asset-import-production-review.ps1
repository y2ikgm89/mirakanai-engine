#requires -Version 7.0
#requires -PSEdition Core

# Chapter 10.0 for check-ai-integration.ps1 Asset Import Production Review contracts.

$assetReviewHeaderText = Get-AgentSurfaceText "engine/assets/include/mirakana/assets/asset_import_production_review.hpp"
$assetReviewSourceText = Get-AgentSurfaceText "engine/assets/src/asset_import_production_review.cpp"
$assetCMakeText = Get-AgentSurfaceText "engine/assets/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$assetReviewTestsText = Get-AgentSurfaceText "tests/unit/asset_import_production_review_tests.cpp"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$importerCapabilitiesText = Get-AgentSurfaceText "engine/agent/manifest.fragments/007-importerCapabilities.json"
$gameCodeGuidanceText = Get-AgentSurfaceText "engine/agent/manifest.fragments/014-gameCodeGuidance.json"
$sample2dManifestText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/game.agent.json"
$sample3dManifestText = Get-AgentSurfaceText "games/sample_generated_desktop_runtime_3d_package/game.agent.json"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$aiGameDevelopmentText = Get-AgentSurfaceText "docs/ai-game-development.md"
$generatedValidationText = Get-AgentSurfaceText "docs/specs/generated-game-validation-scenarios.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$activePlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-05-27-reviewed-importers-codecs-shader-generation-v1.md"
$backlogText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md"
$projectionText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"

foreach ($needle in @(
        "AssetImportProductionStatus",
        "AssetImportProductionExecutionReadiness",
        "AssetImportProductionFeatureKind",
        "AssetImportProductionProofKind",
        "AssetImportProductionDiagnosticCode",
        "AssetImportProductionEvidenceRow",
        "AssetImportProductionReviewRequest",
        "AssetImportProductionExecutionReadinessRow",
        "AssetImportProductionDiagnostic",
        "AssetImportProductionReview",
        "review_asset_import_production_readiness",
        "KtxBasisTextureReviewStatus",
        "KtxBasisTextureReviewFeature",
        "KtxBasisTextureReviewDiagnosticCode",
        "KtxBasisTextureReviewRow",
        "KtxBasisTextureReviewRequest",
        "KtxBasisTextureReviewDiagnostic",
        "KtxBasisTextureReview",
        "review_ktx_basis_texture_readiness",
        "GltfSceneImportReviewStatus",
        "GltfSceneImportReviewFeature",
        "GltfSceneImportReviewDiagnosticCode",
        "GltfSceneImportReviewRow",
        "GltfSceneImportReviewRequest",
        "GltfSceneImportReviewDiagnostic",
        "GltfSceneImportReview",
        "review_gltf_scene_import_readiness",
        "SourceImageAudioCodecReviewStatus",
        "SourceImageAudioCodecReviewFeature",
        "SourceImageAudioCodecReviewDiagnosticCode",
        "SourceImageAudioCodecReviewRow",
        "SourceImageAudioCodecReviewRequest",
        "SourceImageAudioCodecReviewDiagnostic",
        "SourceImageAudioCodecReview",
        "review_source_image_audio_codec_readiness"
    )) {
    Assert-ContainsText $assetReviewHeaderText $needle "engine/assets/include/mirakana/assets/asset_import_production_review.hpp"
}

foreach ($needle in @(
        "missing_review_evidence",
        "missing_host_validation_evidence",
        "missing_source_root_evidence",
        "missing_importer_id",
        "missing_extension_evidence",
        "missing_output_package_row",
        "missing_license_provenance",
        "missing_deterministic_hash",
        "missing_validator_evidence",
        "missing_dependency_legal_record",
        "missing_command_review_evidence",
        "dependency_evidence_required",
        "unsupported_arbitrary_importer_plugin",
        "unsupported_external_download",
        "unsupported_live_shader_generation",
        "unsupported_source_mutation_outside_roots",
        "unsupported_package_mutation",
        "unsupported_native_handle_claim",
        "unsupported_unreviewed_compiler_execution",
        "unsupported_runtime_source_parsing",
        "unsupported_broad_codec_claim",
        "unsupported_runtime_transcoding",
        "unsupported_gpu_upload",
        "unsupported_compression_execution",
        "unsupported_broad_texture_codec_claim",
        "missing_source_root",
        "missing_parser_validation",
        "missing_geometry_payload",
        "missing_material_payload",
        "missing_animation_payload",
        "missing_external_resource_policy",
        "missing_source_provenance",
        "missing_package_output",
        "unsupported_external_network_fetch",
        "unsupported_parser_type_leakage",
        "unsupported_broad_scene_import_claim",
        "missing_pixel_format_diagnostics",
        "missing_sample_format_diagnostics",
        "unsupported_svg_vector_codec",
        "unsupported_broad_image_codec",
        "unsupported_broad_audio_codec",
        "unsupported_background_decode_streaming",
        "unsupported_hrtf_middleware"
    )) {
    Assert-ContainsText $assetReviewHeaderText $needle "asset import production review diagnostics"
}

foreach ($needle in @(
        "row_is_host_gated",
        "row_is_dependency_gated",
        "execution_readiness_for_row",
        "row_is_ready",
        "feature_requires_validator",
        "feature_requires_dependency_legal_record",
        "feature_requires_command_review",
        "row_requests_broad_codec_claim",
        "row_has_required_dependency_evidence",
        "has_exact_dependency_id",
        "row_has_exact_dependency_ids",
        "ktx_row_has_required_dependency_evidence",
        "review_ktx_basis_texture_readiness",
        "ktx_basis_review_ready",
        "broad_texture_codec_ready",
        "gltf_row_has_required_dependency_evidence",
        "gltf_row_has_unsupported_claim",
        "review_gltf_scene_import_readiness",
        "gltf_scene_import_ready",
        "broad_scene_import_ready",
        "source_codec_row_has_required_dependency_evidence",
        "source_codec_row_has_unsupported_claim",
        "review_source_image_audio_codec_readiness",
        "source_image_audio_codec_ready",
        "broad_image_codec_ready",
        "broad_audio_codec_ready",
        "contains_ascii_case_insensitive",
        "vcpkg.ktx",
        "vcpkg.asset-importers",
        "vcpkg.libspng",
        "vcpkg.miniaudio",
        "toolchain.dxc",
        "toolchain.spirv-tools",
        "all_extensions_are_selected",
        "build_replay_hash",
        "build_gltf_replay_hash",
        "valid_token_list",
        "contains_unsafe_token",
        "fastgltf",
        "IDxc",
        "request_arbitrary_importer_plugin",
        "request_external_download",
        "request_live_shader_generation",
        "request_source_mutation_outside_roots",
        "request_package_mutation",
        "request_native_handle_access",
        "request_unreviewed_compiler_execution",
        "request_runtime_source_parsing",
        "request_broad_codec_claim",
        "request_external_network_fetch",
        "request_parser_type_access",
        "request_broad_scene_import_claim",
        "request_svg_vector_codec",
        "request_broad_image_codec",
        "request_broad_audio_codec",
        "request_background_decode_streaming",
        "request_hrtf_middleware",
        "AssetImportProductionStatus::host_evidence_required",
        "AssetImportProductionStatus::dependency_evidence_required",
        "AssetImportProductionStatus::no_rows",
        "GltfSceneImportReviewStatus::dependency_evidence_required",
        "GltfSceneImportReviewStatus::ready",
        "SourceImageAudioCodecReviewStatus::dependency_evidence_required",
        "SourceImageAudioCodecReviewStatus::ready"
    )) {
    Assert-ContainsText $assetReviewSourceText $needle "engine/assets/src/asset_import_production_review.cpp"
}
Assert-DoesNotContainText $assetReviewSourceText "vcpkg.ktx-software" "engine/assets/src/asset_import_production_review.cpp"

foreach ($needle in @(
        "src/asset_import_production_review.cpp",
        "asset_import_production_review.hpp",
        "MK_asset_import_production_review_tests",
        "tests/unit/asset_import_production_review_tests.cpp"
    )) {
    Assert-ContainsText ($assetCMakeText + $rootCMakeText + $modulesFragmentText) $needle "asset import production review build and manifest registration"
}

foreach ($needle in @(
        "asset import production review accepts explicit broad source and cook evidence",
        "asset import production review reports host gated rows without broad readiness",
        "asset import production review reports dependency gated execution matrix without broad readiness",
        "asset import production review rejects ktx2 basis readiness without selected dependency evidence",
        "asset import production review rejects gltf scene import without package hash and source root evidence",
        "asset import production review rejects broad source image and audio codec extension claims",
        "asset import production review requires shader toolchain dependency and legal evidence",
        "asset import production review keeps shader compiler rows host gated without host validation",
        "asset import production review rejects parser compiler and native handle leakage tokens",
        "asset import production review rejects missing manifest and package evidence",
        "asset import production review rejects unsupported execution and unsafe claims",
        "asset import production review distinguishes package mutation from unsupported importer execution",
        "asset import production review rejects missing required features duplicate rows and unsafe tokens",
        "asset import production review reports no rows without broad import claims",
        "ktx2 basis texture review accepts selected dependency and host gated offline tool policy",
        "ktx2 basis texture review reports dependency gated selected package evidence",
        "ktx2 basis texture review rejects runtime transcode upload and missing target policy",
        "gltf scene import review accepts selected fastgltf package handoff evidence",
        "gltf scene import review reports dependency gated selected package evidence",
        "gltf scene import review rejects unsupported broad scene import claims and missing evidence",
        "source image audio codec review accepts selected libspng and miniaudio package evidence",
        "source image audio codec review reports dependency gated selected codec evidence",
        "source image audio codec review rejects broad codec claims and missing format evidence"
    )) {
    Assert-ContainsText $assetReviewTestsText $needle "tests/unit/asset_import_production_review_tests.cpp"
}

foreach ($needle in @(
        "broad-reviewed-asset-import-production-review-v1",
        "implemented-value-contract",
        "mirakana::review_asset_import_production_readiness",
        "first-party-source-documents",
        "png",
        "gltf",
        "common-audio",
        "KTX2/Basis evidence rows may be reviewed when supplied",
        "exact vcpkg.ktx dependency/legal evidence",
        "Selected glTF scene import review rows may be reviewed when supplied",
        "exact vcpkg.asset-importers dependency/legal evidence",
        "gltf_scene_import_review",
        "review_gltf_scene_import_readiness",
        "mirakana::review_source_image_audio_codec_readiness",
        "selected-libspng-png-rgba8",
        "selected-miniaudio-source-audio-pcm",
        "exact vcpkg.libspng and vcpkg.miniaudio dependency/legal evidence",
        "SVG/vector decoding",
        "background decode streaming",
        "HRTF/middleware execution",
        "mirakana::review_shader_generation_cache_execution",
        "Selected shader generation/cache rows may be reviewed",
        "DXC D3D12 DXIL",
        "spirv-val --target-env vulkan1.3",
        "runtime shader compiler execution",
        "native shader cache handle exposure",
        "renderer/RHI shader residency",
        "Metal shader library generation",
        "parser/compiler adapter type leakage",
        "executionReadinessRows",
        "dependency_evidence_required",
        "package_mutation_request_count",
        "arbitrary importer plugin execution",
        "external asset download",
        "live shader generation",
        "package mutation from importer review rows",
        "runtime source parsing",
        "broad codec readiness",
        "glTF external network fetch",
        "glTF runtime source parsing",
        "glTF parser type leakage",
        "glTF native handle exposure",
        "glTF package mutation",
        "broad glTF scene import readiness"
    )) {
    Assert-ContainsText ($importerCapabilitiesText + $manifestText) $needle "engine agent importer capability production review contract"
}

foreach ($needle in @(
        "currentAssetImportProductionReview",
        "currentKtxBasisTextureReviewPackageSmoke",
        "currentGltfSceneImportReviewPackageSmoke",
        "currentSourceImageAudioCodecReviewPackageSmoke",
        "AssetImportProductionReviewRequest",
        "AssetImportProductionEvidenceRow",
        "selected extensions",
        'exact `vcpkg.ktx` dependency evidence',
        'exact `vcpkg.asset-importers` dependency/legal records',
        "--require-ktx2-basis-texture-review",
        "ktx_basis_texture_review_status=host_evidence_required",
        "--require-gltf-scene-import-review",
        "gltf_scene_import_review_status=ready",
        "gltf_scene_import_review_ready=1",
        "--require-source-image-audio-codec-review",
        "source_image_audio_codec_review_status=ready",
        "source_image_audio_codec_review_ready=1",
        "source_image_audio_codec_review_replay_hash",
        "Generated 2D/3D Source Image Audio Codec Review Package Smoke v1",
        "zero runtime transcoding",
        "zero GPU upload",
        "zero compression tool invocation",
        "broad texture codec readiness not claimed",
        "zero external network fetch",
        "zero runtime source parsing",
        "zero parser type leakage",
        "broad scene import readiness not claimed",
        "broad image codec readiness not claimed",
        "broad audio codec readiness not claimed",
        "zero SVG/vector decode",
        "zero background decode streaming",
        "zero HRTF/middleware",
        "does not parse glTF/source assets at runtime",
        "Generated 3D glTF Scene Import Review Package Smoke v1",
        'exact `toolchain.dxc` plus `toolchain.spirv-tools` dependency evidence',
        "parser/compiler/native",
        "missing review or host validation evidence",
        "does not execute importers",
        "does not execute importers, download assets, run shader compilers",
        'descriptor-only `game.agent.json.importerRequirements.productionImportReview`',
        "Broad Reviewed Asset Import And Cook Pipeline",
        "broad-reviewed-asset-import-production-review-v1"
    )) {
    Assert-ContainsText ($gameCodeGuidanceText + $currentCapabilitiesText + $roadmapText + $aiGameDevelopmentText + $generatedValidationText + $planRegistryText + $activePlanText + $backlogText + $projectionText) $needle "asset import production review docs"
}

foreach ($manifestTextForSample in @($sample2dManifestText, $sample3dManifestText)) {
    foreach ($needle in @(
            '"productionImportReview"',
            '"broad-reviewed-asset-import-production-review-v1"',
            '"reviewApi": "mirakana::review_asset_import_production_readiness"',
            '"implementedExternalAdaptersRequired": []',
            '"arbitrary-importer-plugin"',
            '"external-asset-download"',
            '"live-shader-generation"',
            '"runtime-source-parsing"',
            '"native-handle-access"',
            '"svg-vector-codec"',
            '"broad-image-codec-readiness"',
            '"broad-audio-codec-readiness"',
            '"background-decode-streaming"',
            '"hrtf-middleware"',
            '"broad-codec-readiness"'
        )) {
        Assert-ContainsText $manifestTextForSample $needle "sample game importer production review descriptor"
    }
}

Assert-ContainsText $sample2dManifestText '"first-party-source-documents"' "2D sample production import review descriptor"
Assert-ContainsText $sample2dManifestText '"selected-libspng-png-rgba8"' "2D sample production import review descriptor"
Assert-ContainsText $sample2dManifestText '"selected-miniaudio-source-audio-pcm"' "2D sample production import review descriptor"
Assert-ContainsText $sample2dManifestText '"installed-2d-source-image-audio-codec-review-smoke"' "2D sample production import review descriptor"
Assert-ContainsText $sample3dManifestText '"gltf"' "3D sample production import review descriptor"
Assert-ContainsText $sample3dManifestText '"ktx2-basis-review"' "3D sample production import review descriptor"
Assert-ContainsText $sample3dManifestText '"gltf-scene-import-review"' "3D sample production import review descriptor"
Assert-ContainsText $sample3dManifestText '"selected-libspng-png-rgba8"' "3D sample production import review descriptor"
Assert-ContainsText $sample3dManifestText '"selected-miniaudio-source-audio-pcm"' "3D sample production import review descriptor"
Assert-ContainsText $sample3dManifestText '"installed-generated-3d-source-image-audio-codec-review-smoke"' "3D sample production import review descriptor"
Assert-ContainsText $sample3dManifestText '"gltf-external-network-fetch"' "3D sample production import review descriptor"
Assert-ContainsText $sample3dManifestText '"gltf-runtime-source-parsing"' "3D sample production import review descriptor"
Assert-ContainsText $sample3dManifestText '"broad-gltf-scene-import-readiness"' "3D sample production import review descriptor"
Assert-ContainsText $sample3dManifestText '"hlsl-offline-compile-request"' "3D sample production import review descriptor"
Assert-ContainsText $sample3dManifestText '"runtime-ktx2-basis-transcoding"' "3D sample production import review descriptor"
Assert-ContainsText $sample3dManifestText '"ktx2-basis-gpu-upload"' "3D sample production import review descriptor"
Assert-ContainsText $sample3dManifestText '"ktx2-basis-compression-execution"' "3D sample production import review descriptor"
