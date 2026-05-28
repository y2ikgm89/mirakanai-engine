#requires -Version 7.0
#requires -PSEdition Core

# Chapter 10.3 for check-ai-integration.ps1 Audio production playback/streaming/DSP/spatialization evidence.

$audioHeaderText = Get-AgentSurfaceText "engine/audio/include/mirakana/audio/audio_mixer.hpp"
$audioSourceText = Get-AgentSurfaceText "engine/audio/src/audio_mixer.cpp"
$wasapiAudioHeaderText = Get-AgentSurfaceText "engine/audio/wasapi/include/mirakana/audio/wasapi/wasapi_audio_device.hpp"
$wasapiAudioSourceText = Get-AgentSurfaceText "engine/audio/wasapi/src/wasapi_audio_device.cpp"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$audioTestText = Get-AgentSurfaceText "tests/unit/audio_production_tests.cpp"
$wasapiAudioTestText = Get-AgentSurfaceText "tests/unit/wasapi_audio_tests.cpp"
$specText = Get-AgentSurfaceText "docs/specs/2026-05-27-audio-production-playback-streaming-dsp-spatialization.md"
$sampleMainText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/main.cpp"
$sampleManifestText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/game.agent.json"
$sample3dMainText = Get-AgentSurfaceText "games/sample_generated_desktop_runtime_3d_package/main.cpp"
$sample3dManifestText = Get-AgentSurfaceText "games/sample_generated_desktop_runtime_3d_package/game.agent.json"
$installedValidationText = Get-AgentSurfaceText "tools/validate-installed-desktop-runtime.ps1"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-05-26-engine-general-production-quality-expansion-v1.md"
$registryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$readinessFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/006-runtimeBackendReadiness.json"
$recipesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/009-validationRecipes.json"
$loopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$gameGuidanceFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/014-gameCodeGuidance.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"
$retiredGenerated3dPackageProof = Test-RetiredSdl3DesktopRuntimeSamplePath "games/sample_generated_desktop_runtime_3d_package/main.cpp"

foreach ($needle in @(
        "AudioProductionReadinessStatus",
        "AudioProductionReadinessPlan",
        "AudioProductionReviewRequest",
        "AudioProductionDecodedSourceEvidenceRow",
        "AudioProductionStreamingChunkEvidenceRow",
        "AudioProductionFormatConversionPolicyRow",
        "AudioProductionDspGraphRow",
        "AudioProductionDeviceLifecycleRow",
        "AudioProductionUnsupportedClaimRow",
        "AudioProductionUnsupportedClaimKind",
        "review_audio_production_readiness"
    )) {
    Assert-ContainsText $audioHeaderText $needle "engine/audio/include/mirakana/audio/audio_mixer.hpp"
}

foreach ($needle in @(
        "missing_hrtf_host_gate",
        "missing_device_host_evidence",
        "unsupported_audio_claim",
        "side_effect_claim",
        "AudioProductionReadinessStatus::host_evidence_required",
        "AudioProductionReadinessStatus::invalid_request",
        "replay_hash_for_audio_production",
        "plan.selected_package_evidence_ready"
    )) {
    Assert-ContainsText $audioSourceText $needle "engine/audio/src/audio_mixer.cpp"
}

foreach ($needle in @(
        "WasapiAudioRuntime",
        "WasapiAudioDevice",
        "plan_wasapi_audio_runtime",
        "plan_wasapi_shared_mode_stream",
        "plan_wasapi_render_queue",
        "wasapi_audio_device_lifecycle_evidence",
        "AudioProductionDeviceLifecycleRow"
    )) {
    Assert-ContainsText $wasapiAudioHeaderText $needle "engine/audio/wasapi/include/mirakana/audio/wasapi/wasapi_audio_device.hpp"
}

foreach ($needle in @(
        "wasapi_audio_device_lifecycle_evidence",
        "CoInitializeEx",
        "GetDefaultAudioEndpoint",
        "IAudioClient::Initialize",
        "IAudioRenderClient::GetBuffer",
        "AUDCLNT_BUFFERFLAGS_SILENT",
        ".uses_callback = false",
        ".native_handle_exposed = false"
    )) {
    Assert-ContainsText $wasapiAudioSourceText $needle "engine/audio/wasapi/src/wasapi_audio_device.cpp"
}

Assert-ContainsText $rootCMakeText "MK_audio_tests" "CMakeLists.txt"
Assert-ContainsText $rootCMakeText "MK_wasapi_audio_tests" "CMakeLists.txt"

foreach ($needle in @(
        "audio production readiness accepts decoded streaming dsp spatial and device evidence",
        "audio production readiness separates missing device and hrtf host evidence",
        "audio production readiness rejects unsupported codec middleware native handle and background claims",
        "audio production readiness rejects incomplete rows budgets and side effects"
    )) {
    Assert-ContainsText $audioTestText $needle "tests/unit/audio_production_tests.cpp"
}

Assert-ContainsText $wasapiAudioTestText "wasapi audio adapter exposes production stream queue lifecycle evidence" "tests/unit/wasapi_audio_tests.cpp"

foreach ($needle in @(
        "IAudioClient",
        "IAudioRenderClient",
        "no OpenAL",
        "host_evidence_required",
        "audio_production_*",
        "broad codec support"
    )) {
    Assert-ContainsText $specText $needle "docs/specs/2026-05-27-audio-production-playback-streaming-dsp-spatialization.md"
}

foreach ($needle in @(
        "validate_audio_production_package_evidence",
        "audio_production_status",
        "audio_production_selected_package_ready",
        "audio_production_package_evidence_ready",
        "audio_production_invoked_codec_decode",
        "audio_production_replay_hash"
    )) {
    Assert-ContainsText $sampleMainText $needle "games/sample_2d_desktop_runtime_package/main.cpp"
    Assert-ContainsText $sample3dMainText $needle "games/sample_generated_desktop_runtime_3d_package/main.cpp"
}
Assert-ContainsText $sample3dMainText "--require-audio-production" "games/sample_generated_desktop_runtime_3d_package/main.cpp"

foreach ($needle in @(
        "--require-audio-production",
        "audio_production_status",
        "audio_production_selected_package_ready",
        "audio_production_invoked_codec_decode",
        "audio_production_replay_hash"
    )) {
    Assert-ContainsText $installedValidationText $needle "tools/validate-installed-desktop-runtime.ps1"
}

foreach ($needle in @(
        "audio-production",
        "installed-2d-audio-production-smoke",
        "audio_production_*",
        "broad-audio-codec-support",
        "audio-middleware-parity",
        "hrtf-execution-parity"
    )) {
    Assert-ContainsText $sampleManifestText $needle "games/sample_2d_desktop_runtime_package/game.agent.json"
}

foreach ($needle in @(
        "audio-production",
        "installed-3d-audio-production-smoke",
        "--require-audio-production",
        "audio_production_*",
        "broad codec",
        "middleware",
        "HRTF"
    )) {
    Assert-ContainsText $sample3dManifestText $needle "games/sample_generated_desktop_runtime_3d_package/game.agent.json"
}

foreach ($docSurface in @(
        @{ Text = $planText; Label = "docs/superpowers/plans/2026-05-26-engine-general-production-quality-expansion-v1.md" },
        @{ Text = $registryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $gameGuidanceFragmentText; Label = "engine/agent/manifest.fragments/014-gameCodeGuidance.json" }
    )) {
    Assert-ContainsText $docSurface.Text "review_audio_production_readiness" $docSurface.Label
    Assert-ContainsText $docSurface.Text "audio_production_*" $docSurface.Label
    Assert-ContainsText $docSurface.Text "HRTF" $docSurface.Label
    Assert-ContainsText $docSurface.Text "middleware" $docSurface.Label
    Assert-ContainsText $docSurface.Text "broad codec" $docSurface.Label
}

foreach ($needle in @(
        "AudioProductionReviewRequest",
        "review_audio_production_readiness",
        "wasapi_audio_device_lifecycle_evidence"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json"
    Assert-ContainsText $manifestText $needle "engine/agent/manifest.json"
}

Assert-ContainsText $readinessFragmentText "audio_production_*" "engine/agent/manifest.fragments/006-runtimeBackendReadiness.json"
Assert-ContainsText $readinessFragmentText "wasapi-shared-mode" "engine/agent/manifest.fragments/006-runtimeBackendReadiness.json"
Assert-ContainsText $recipesFragmentText '"audio-production"' "engine/agent/manifest.fragments/009-validationRecipes.json"
Assert-ContainsText $recipesFragmentText '"wasapi-audio"' "engine/agent/manifest.fragments/009-validationRecipes.json"
Assert-ContainsText $recipesFragmentText '"desktop-runtime-2d-audio-production-proof"' "engine/agent/manifest.fragments/009-validationRecipes.json"
Assert-ContainsText $manifestText "desktop-runtime-2d-audio-production-proof" "engine/agent/manifest.json"
if (-not $retiredGenerated3dPackageProof) {
    Assert-ContainsText $recipesFragmentText '"desktop-runtime-3d-audio-production-proof"' "engine/agent/manifest.fragments/009-validationRecipes.json"
    Assert-ContainsText $manifestText "desktop-runtime-3d-audio-production-proof" "engine/agent/manifest.json"
}
else {
    Assert-DoesNotContainText $recipesFragmentText '"desktop-runtime-3d-audio-production-proof"' "engine/agent/manifest.fragments/009-validationRecipes.json retired generated 3D audio proof"
    Assert-DoesNotContainText $manifestText "desktop-runtime-3d-audio-production-proof" "engine/agent/manifest.json retired generated 3D audio proof"
    Assert-ContainsText $currentCapabilitiesText 'retired `sample_generated_desktop_runtime_3d_package` rows remain historical audio-production evidence' "docs/current-capabilities.md retired generated 3D audio proof"
}
Assert-ContainsText $loopFragmentText "Audio Production Playback/Streaming/DSP/Spatialization Evidence v1" "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
Assert-ContainsText $gameGuidanceFragmentText "currentAudioProductionEvidence" "engine/agent/manifest.fragments/014-gameCodeGuidance.json"
