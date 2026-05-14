#requires -Version 7.0
#requires -PSEdition Core

# Chapter 4 for check-ai-integration.ps1 static contracts.

$editorPackageRegistrationDraftChecks = @(
    @{
        Path = "editor/core/include/mirakana/editor/scene_authoring.hpp"
        Needles = @(
            "ScenePackageRegistrationDraftStatus",
            "ScenePackageRegistrationDraftRow",
            "ScenePackageRegistrationApplyPlan",
            "ScenePackageRegistrationApplyResult",
            "make_scene_package_registration_draft_rows",
            "scene_package_registration_draft_status_label",
            "make_scene_package_registration_apply_plan",
            "apply_scene_package_registration_to_manifest"
        )
    },
    @{
        Path = "editor/core/src/scene_authoring.cpp"
        Needles = @(
            "ScenePackageRegistrationDraftStatus::add_runtime_file",
            "ScenePackageRegistrationDraftStatus::already_registered",
            "ScenePackageRegistrationDraftStatus::rejected_source_file",
            "ScenePackageRegistrationDraftStatus::rejected_unsafe_path",
            "ScenePackageRegistrationDraftStatus::rejected_duplicate",
            "make_scene_package_registration_draft_rows",
            "make_scene_package_registration_apply_plan",
            "apply_scene_package_registration_to_manifest",
            "runtimePackageFiles must be an array"
        )
    },
    @{
        Path = "editor/src/main.cpp"
        Needles = @(
            "make_scene_package_registration_draft_rows",
            "Package Registration Draft",
            "scene_package_registration_draft_status_label",
            "make_scene_package_registration_apply_plan",
            "Apply Package Registration",
            "apply_package_registration",
            "current_runtime_package_files",
            "manifest_runtime_package_files_"
        )
    }
)
foreach ($check in $editorPackageRegistrationDraftChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing editor package registration draft contract: $($missingNeedles -join ', ')"
    }
}

$editorPlaytestReviewChecks = @(
    @{
        Path = "editor/core/include/mirakana/editor/playtest_package_review.hpp"
        Needles = @(
            "EditorPlaytestPackageReviewModel",
            "EditorPlaytestPackageReviewDesc",
            "RuntimeSceneValidationTargetRow",
            "make_editor_playtest_package_review_model",
            "editor_playtest_review_step_status_label"
        )
    },
    @{
        Path = "editor/core/src/playtest_package_review.cpp"
        Needles = @(
            "review-editor-package-candidates",
            "apply-reviewed-runtime-package-files",
            "select-runtime-scene-validation-target",
            "validate-runtime-scene-package",
            "run-host-gated-desktop-smoke",
            "Apply Package Registration before selecting runtimeSceneValidationTargets"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor playtest package review model orders validation before host smoke",
            "editor playtest package review model blocks validation until reviewed package files are applied",
            "editor playtest package review model rejects missing validation targets before smoke",
            "EditorPlaytestPackageReviewDesc",
            "RuntimeSceneValidationTargetRow"
        )
    },
    @{
        Path = "engine/scene/include/mirakana/scene/prefab_overrides.hpp"
        Needles = @(
            "deserialize_prefab_variant_definition_for_review"
        )
    },
    @{
        Path = "editor/core/src/prefab_variant_authoring.cpp"
        Needles = @(
            "repairable_for_prefab_variant_authoring",
            "Remove missing-node override",
            "deserialize_prefab_variant_definition_for_review"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor prefab variant missing node cleanup resolves stale raw variants",
            "editor prefab variant authoring loads stale variants for reviewed missing node cleanup",
            "Remove missing-node override"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Editor Playtest Package Review Loop v1",
            "EditorPlaytestPackageReviewModel",
            "runtimeSceneValidationTargets",
            "validate-runtime-scene-package",
            "host-gated desktop smoke"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "Editor Playtest Package Review Loop v1",
            "review-editor-package-candidates -> apply-reviewed-runtime-package-files -> select-runtime-scene-validation-target -> validate-runtime-scene-package -> run-host-gated-desktop-smoke",
            "blocking runtime scene validation until reviewed package files are applied"
        )
    }
)
foreach ($check in $editorPlaytestReviewChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing editor playtest package review contract: $($missingNeedles -join ', ')"
    }
}

$editorPlayInEditorIsolationChecks = @(
    @{
        Path = "editor/core/include/mirakana/editor/play_in_editor.hpp"
        Needles = @(
            "EditorPlaySession",
            "EditorPlaySessionReport",
            "EditorPlaySessionState",
            "EditorPlaySessionActionStatus",
            "EditorPlaySessionTickContext",
            "IEditorPlaySessionDriver",
            "EditorPlaySessionControlCommand",
            "EditorPlaySessionControlsModel",
            "make_editor_play_session_report",
            "make_editor_play_session_controls_model",
            "editor_play_session_control_command_id"
        )
    },
    @{
        Path = "editor/core/src/play_in_editor.cpp"
        Needles = @(
            "source_document.scene()",
            "isolated simulation scene",
            "gameplay_driver_",
            "on_play_tick",
            "rejected_invalid_delta",
            "source_has_scene",
            "viewport_uses_simulation_scene",
            "EditorPlaySessionControlCommand::play",
            "simulation_frame_count_",
            "simulation_scene_.reset()"
        )
    },
    @{
        Path = "editor/src/main.cpp"
        Needles = @(
            "play_session_controls_model",
            "viewport_scene()",
            "tick_play_session_from_viewport_frame",
            "source_scene_edits_blocked",
            "Scene authoring is blocked while Play-In-Editor is active"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor play in editor session isolates simulation scene from source document",
            "editor play in editor session controls ticks pause resume stop and rejects invalid transitions",
            "editor play in editor gameplay driver mutates only isolated simulation scene",
            "editor play in editor gameplay driver rejects paused and invalid ticks",
            "editor play in editor controls model exposes edit and empty source controls",
            "editor play in editor controls model exposes play paused and stopped controls",
            "EditorPlaySession",
            "IEditorPlaySessionDriver",
            "EditorPlaySessionActionStatus",
            "EditorPlaySessionControlsModel"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Play-In-Editor Session Isolation v1",
            "Play-In-Editor Gameplay Driver v1",
            "Play-In-Editor Visible Viewport Wiring v1",
            "EditorPlaySessionControlsModel",
            "EditorPlaySession",
            "IEditorPlaySessionDriver",
            "isolated simulation scene",
            "dynamic game modules"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Play-In-Editor Session Isolation v1",
            "Play-In-Editor Gameplay Driver v1",
            "Play-In-Editor Visible Viewport Wiring v1",
            "EditorPlaySessionControlsModel",
            "EditorPlaySession",
            "IEditorPlaySessionDriver",
            "Editor Runtime Host Playtest Launch v1"
        )
    }
)
foreach ($check in $editorPlayInEditorIsolationChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing play-in-editor visible viewport contract: $($missingNeedles -join ', ')"
    }
}

$editorRuntimeHostPlaytestLaunchChecks = @(
    @{
        Path = "editor/core/include/mirakana/editor/play_in_editor.hpp"
        Needles = @(
            "EditorRuntimeHostPlaytestLaunchDesc",
            "EditorRuntimeHostPlaytestLaunchModel",
            "EditorRuntimeHostPlaytestLaunchStatus",
            "make_editor_runtime_host_playtest_launch_model",
            "make_editor_runtime_host_playtest_launch_ui_model"
        )
    },
    @{
        Path = "editor/core/src/play_in_editor.cpp"
        Needles = @(
            "play_in_editor.runtime_host",
            "unsafe-argv-token",
            "missing-reviewed-argv",
            "request_dynamic_game_module_loading",
            "is_safe_process_command"
        )
    },
    @{
        Path = "editor/src/main.cpp"
        Needles = @(
            "make_runtime_host_playtest_launch_model",
            "Execute Runtime Host",
            "runtime_host_playtest_host_gate_acknowledged_",
            "mirakana::Win32ProcessRunner",
            "apply_runtime_host_playtest_result"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor runtime host playtest launch reviews safe external process commands",
            "EditorRuntimeHostPlaytestLaunchDesc",
            "d3d12-windows-primary",
            "play_in_editor.runtime_host",
            "unsafe-argv-token"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Editor Runtime Host Playtest Launch v1",
            "EditorRuntimeHostPlaytestLaunchDesc",
            "EditorRuntimeHostPlaytestLaunchModel",
            "make_editor_runtime_host_playtest_launch_model",
            "play_in_editor.runtime_host",
            "Win32ProcessRunner",
            "in-process runtime-host embedding"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Editor Runtime Host Playtest Launch v1",
            "EditorRuntimeHostPlaytestLaunchModel",
            "make_editor_runtime_host_playtest_launch_ui_model",
            "play_in_editor.runtime_host",
            "d3d12-windows-primary",
            "dynamic game-module loading"
        )
    },
    @{
        Path = "docs/roadmap.md"
        Needles = @(
            "Editor Runtime Host Playtest Launch v1",
            "EditorRuntimeHostPlaytestLaunchDesc",
            "Execute Runtime Host",
            "Win32ProcessRunner",
            "in-process runtime-host embedding"
        )
    },
    @{
        Path = "docs/superpowers/plans/README.md"
        Needles = @(
            "2026-05-07-editor-runtime-host-playtest-launch-v1.md",
            "Editor Runtime Host Playtest Launch v1",
            "EditorRuntimeHostPlaytestLaunchModel",
            "play_in_editor.runtime_host",
            "Editor Content Browser Import External Copy Review v1"
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md"
        Needles = @(
            "Editor Runtime Host Playtest Launch v1",
            "EditorRuntimeHostPlaytestLaunchModel",
            "play_in_editor.runtime_host",
            "Win32ProcessRunner",
            "dynamic game-module loading"
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-07-editor-runtime-host-playtest-launch-v1.md"
        Needles = @(
            "Editor Runtime Host Playtest Launch v1 Implementation Plan",
            "EditorRuntimeHostPlaytestLaunchModel",
            "play_in_editor.runtime_host",
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "currentEditorRuntimeHostPlaytestLaunch",
            "EditorRuntimeHostPlaytestLaunchDesc",
            "EditorRuntimeHostPlaytestLaunchModel",
            "make_editor_runtime_host_playtest_launch_model",
            "play_in_editor.runtime_host",
            "Win32ProcessRunner",
            "in-process runtime-host embedding",
            "Editor Content Browser Import External Copy Review v1"
        )
    },
    @{
        Path = ".agents/skills/editor-change/SKILL.md"
        Needles = @(
            "EditorRuntimeHostPlaytestLaunchDesc",
            "make_editor_runtime_host_playtest_launch_model",
            "play_in_editor.runtime_host",
            "Win32ProcessRunner",
            "editor-core process execution"
        )
    },
    @{
        Path = ".claude/skills/gameengine-editor/SKILL.md"
        Needles = @(
            "EditorRuntimeHostPlaytestLaunchDesc",
            "make_editor_runtime_host_playtest_launch_model",
            "play_in_editor.runtime_host",
            "Win32ProcessRunner",
            "editor-core process execution"
        )
    }
)
foreach ($check in $editorRuntimeHostPlaytestLaunchChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing editor runtime host playtest launch contract: $($missingNeedles -join ', ')"
    }
}

$editorInProcessRuntimeHostReviewChecks = @(
    @{
        Path = "editor/core/include/mirakana/editor/play_in_editor.hpp"
        Needles = @(
            "EditorInProcessRuntimeHostDesc",
            "EditorInProcessRuntimeHostModel",
            "EditorInProcessRuntimeHostBeginResult",
            "begin_editor_in_process_runtime_host_session",
            "make_editor_in_process_runtime_host_ui_model"
        )
    },
    @{
        Path = "editor/core/src/play_in_editor.cpp"
        Needles = @(
            "play_in_editor.in_process_runtime_host",
            "request_dynamic_game_module_loading",
            "request_renderer_rhi_handle_exposure",
            "request_package_streaming",
            "linked-gameplay-driver"
        )
    },
    @{
        Path = "editor/src/main.cpp"
        Needles = @(
            "make_in_process_runtime_host_model",
            "make_in_process_runtime_host_desc",
            "game_module_driver_ != nullptr",
            "In-Process Runtime Host",
            "Begin In-Process Runtime Host",
            "begin_in_process_runtime_host"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor in process runtime host review starts linked gameplay driver sessions",
            "EditorInProcessRuntimeHostDesc",
            "begin_editor_in_process_runtime_host_session",
            "play_in_editor.in_process_runtime_host",
            "request_renderer_rhi_handle_exposure"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Editor In-Process Runtime Host Review v1",
            "EditorInProcessRuntimeHostDesc",
            "begin_editor_in_process_runtime_host_session",
            "play_in_editor.in_process_runtime_host",
            "Begin In-Process Runtime Host",
            "already-linked"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Editor In-Process Runtime Host Review v1",
            "EditorInProcessRuntimeHostModel",
            "make_editor_in_process_runtime_host_ui_model",
            "play_in_editor.in_process_runtime_host",
            "IEditorPlaySessionDriver",
            "dynamic game-module loading"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "Editor In-Process Runtime Host Review v1 coverage",
            "EditorInProcessRuntimeHostBeginResult",
            "begin_editor_in_process_runtime_host_session",
            "play_in_editor.in_process_runtime_host",
            "missing-driver diagnostics"
        )
    },
    @{
        Path = "docs/roadmap.md"
        Needles = @(
            "Editor In-Process Runtime Host Review v1",
            "EditorInProcessRuntimeHostDesc",
            "Begin In-Process Runtime Host",
            'already-linked `IEditorPlaySessionDriver`',
            "DesktopGameRunner"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "currentEditorInProcessRuntimeHost",
            "EditorInProcessRuntimeHostDesc",
            "EditorInProcessRuntimeHostModel",
            "begin_editor_in_process_runtime_host_session",
            "play_in_editor.in_process_runtime_host",
            "Begin In-Process Runtime Host",
            "linked-driver handoff evidence"
        )
    },
    @{
        Path = ".agents/skills/editor-change/SKILL.md"
        Needles = @(
            "EditorInProcessRuntimeHostDesc",
            "begin_editor_in_process_runtime_host_session",
            "play_in_editor.in_process_runtime_host",
            "Begin In-Process Runtime Host",
            "DesktopGameRunner"
        )
    },
    @{
        Path = ".claude/skills/gameengine-editor/SKILL.md"
        Needles = @(
            "EditorInProcessRuntimeHostDesc",
            "begin_editor_in_process_runtime_host_session",
            "play_in_editor.in_process_runtime_host",
            "Begin In-Process Runtime Host",
            "DesktopGameRunner"
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-09-editor-in-process-runtime-host-review-v1.md"
        Needles = @(
            "Editor In-Process Runtime Host Review Implementation Plan",
            "EditorInProcessRuntimeHostModel",
            "play_in_editor.in_process_runtime_host",
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1"
        )
    }
)
foreach ($check in $editorInProcessRuntimeHostReviewChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing editor in-process runtime host review contract: $($missingNeedles -join ', ')"
    }
}

$editorGameModuleDriverLoadChecks = @(
    @{
        Path = "engine/platform/include/mirakana/platform/dynamic_library.hpp"
        Needles = @(
            "DynamicLibrary",
            "DynamicLibraryLoadStatus",
            "DynamicLibrarySymbolStatus",
            "is_safe_dynamic_library_symbol_name",
            "load_dynamic_library",
            "resolve_dynamic_library_symbol"
        )
    },
    @{
        Path = "CMakeLists.txt"
        Needles = @(
            "MK_editor_game_module_driver_probe",
            "MK_editor_game_module_driver_load_tests",
            "MK_EDITOR_GAME_MODULE_DRIVER_PROBE_PATH"
        )
    },
    @{
        Path = "engine/platform/src/dynamic_library.cpp"
        Needles = @(
            "LoadLibraryExW",
            "LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR",
            "LOAD_LIBRARY_SEARCH_SYSTEM32",
            "GetProcAddress",
            "FreeLibrary",
            "dynamic library path must be absolute"
        )
    },
    @{
        Path = "editor/core/include/mirakana/editor/game_module_driver.hpp"
        Needles = @(
            "EditorGameModuleDriverApi",
            "GameEngine.EditorGameModuleDriver.v1",
            "editor_game_module_driver_factory_symbol_v1",
            "mirakana_create_editor_game_module_driver_v1",
            "EditorGameModuleDriverContractMetadataRow",
            "EditorGameModuleDriverContractMetadataModel",
            "EditorGameModuleDriverLoadDesc",
            "EditorGameModuleDriverReloadDesc",
            "EditorGameModuleDriverReloadModel",
            "EditorGameModuleDriverUnloadDesc",
            "EditorGameModuleDriverUnloadModel",
            "EditorGameModuleDriverCtestProbeEvidenceModel",
            "EditorGameModuleDriverCreateResult",
            "make_editor_game_module_driver_contract_metadata_model",
            "make_editor_game_module_driver_contract_metadata_ui_model",
            "make_editor_game_module_driver_ctest_probe_evidence_model",
            "make_editor_game_module_driver_ctest_probe_evidence_ui_model",
            "make_editor_game_module_driver_unload_model",
            "make_editor_game_module_driver_unload_ui_model",
            "EditorGameModuleDriverHostSessionPhase",
            "EditorGameModuleDriverHostSessionSnapshot",
            "make_editor_game_module_driver_host_session_snapshot",
            "make_editor_game_module_driver_host_session_ui_model",
            "editor_game_module_driver_host_session_contract_v1",
            "editor_game_module_driver_host_session_dll_barriers_contract_v1"
        )
    },
    @{
        Path = "editor/core/src/game_module_driver.cpp"
        Needles = @(
            "play_in_editor.game_module_driver",
            "play_in_editor.game_module_driver.reload",
            "play_in_editor.game_module_driver.contract",
            "play_in_editor.game_module_driver.unload",
            "play_in_editor.game_module_driver.ctest_probe_evidence",
            "play_in_editor.game_module_driver.session",
            "play_in_editor.game_module_driver.session.barriers_contract_label",
            "play_in_editor.game_module_driver.session.barrier.play_dll_surface_mutation.status",
            "play_in_editor.game_module_driver.session.policy.active_session_hot_reload",
            "play_in_editor.game_module_driver.session.policy.stopped_state_reload_scope",
            "play_in_editor.game_module_driver.session.policy.dll_mutation_order_guidance",
            "make_editor_game_module_driver_contract_metadata_model",
            "make_editor_game_module_driver_ctest_probe_evidence_model",
            "same-engine-build",
            "loaded-driver-required",
            "driver-already-loaded",
            "active-session reload",
            "stable third-party ABI",
            "DesktopGameRunner embedding",
            "package streaming",
            "missing-destroy-callback",
            "invalid-abi-version"
        )
    },
    @{
        Path = "editor/src/main.cpp"
        Needles = @(
            "Game Module Driver",
            "Load Game Module Driver",
            "Reload Game Module Driver",
            "Unload Game Module Driver",
            "mirakana::load_dynamic_library",
            "mirakana::resolve_dynamic_library_symbol",
            "make_editor_game_module_driver_from_symbol",
            "make_editor_game_module_driver_contract_metadata_model",
            "same-engine-build only",
            "make_editor_game_module_driver_ctest_probe_evidence_model",
            "make_editor_game_module_driver_reload_transaction_recipe_evidence_model",
            "dev-windows-editor-game-module-driver-load-tests",
            "ge.editor.editor_game_module_driver_reload_transaction_recipe_evidence.v1",
            "MK_editor_game_module_driver_probe",
            "game_module_driver_library_",
            "DLL mutation order guidance:",
            "begin_in_process_runtime_host"
        )
    },
    @{
        Path = "tests/fixtures/editor_game_module_driver_probe.cpp"
        Needles = @(
            "mirakana_create_editor_game_module_driver_v1",
            "MK_editor_game_module_driver_probe_reset",
            "MK_editor_game_module_driver_probe_tick_count",
            "EditorGameModuleDriverApi"
        )
    },
    @{
        Path = "tests/unit/editor_game_module_driver_load_tests.cpp"
        Needles = @(
            "editor game module driver loads real dynamic probe and ticks isolated session",
            "MK_EDITOR_GAME_MODULE_DRIVER_PROBE_PATH",
            "MK_editor_game_module_driver_probe_reset",
            "mirakana_create_editor_game_module_driver_v1",
            "make_editor_game_module_driver_from_symbol"
        )
    },
    @{
        Path = "tests/unit/platform_process_tests.cpp"
        Needles = @(
            "dynamic library loads absolute module and resolves symbol",
            "MK_dynamic_library_probe_add",
            "dynamic library rejects relative module paths",
            "dynamic library rejects unsafe symbol names",
            "dynamic library reports missing symbols"
        )
    },
    @{
        Path = "tests/fixtures/dynamic_library_probe.cpp"
        Needles = @(
            "MK_dynamic_library_probe_add"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor game module function table driver mutates only isolated simulation scene",
            "editor game module driver rejects invalid function tables",
            "editor game module driver reload model reviews stopped safe reload",
            "editor game module driver contract metadata documents same engine ABI boundary",
            "editor game module driver unload model gates loaded driver and active play session",
            "editor game module driver ctest probe evidence ui exposes retained ids",
            "editor game module driver host session snapshot classifies play and residency phases",
            "play_in_editor.game_module_driver",
            "play_in_editor.game_module_driver.reload",
            "play_in_editor.game_module_driver.contract",
            "play_in_editor.game_module_driver.unload",
            "play_in_editor.game_module_driver.ctest_probe_evidence",
            "play_in_editor.game_module_driver.session",
            "ge.editor.editor_game_module_driver_host_session.v1",
            "stable third-party ABI"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Editor Dynamic Game Module Driver Load v1",
            "Editor Game Module Driver Safe Reload Review v1",
            "Editor Game Module Driver Contract Metadata Review v1",
            "Editor Game Module Driver Dynamic Probe v1",
            "Editor Game Module Driver CTest Probe Evidence UI v1",
            "Editor Game Module Driver Play Session Gate Review v1",
            "MK_editor_game_module_driver_probe",
            "MK_editor_game_module_driver_load_tests",
            "play_in_editor.game_module_driver",
            "play_in_editor.game_module_driver.reload",
            "play_in_editor.game_module_driver.contract",
            "play_in_editor.game_module_driver.unload",
            "play_in_editor.game_module_driver.ctest_probe_evidence",
            "play_in_editor.game_module_driver.session",
            "Load Game Module Driver",
            "Reload Game Module Driver",
            "Unload Game Module Driver",
            "mirakana_create_editor_game_module_driver_v1"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Editor Dynamic Game Module Driver Load v1",
            "Editor Game Module Driver Safe Reload Review v1",
            "Editor Game Module Driver Contract Metadata Review v1",
            "Editor Game Module Driver Dynamic Probe v1",
            "MK_editor_game_module_driver_probe",
            "MK_editor_game_module_driver_load_tests",
            "play_in_editor.game_module_driver",
            "play_in_editor.game_module_driver.reload",
            "play_in_editor.game_module_driver.contract",
            "Editor Game Module Driver CTest Probe Evidence UI v1",
            "play_in_editor.game_module_driver.ctest_probe_evidence",
            "Load Game Module Driver",
            "Reload Game Module Driver",
            "mirakana_create_editor_game_module_driver_v1"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "Editor Dynamic Game Module Driver Load v1",
            "Editor Game Module Driver Safe Reload Review v1",
            "Editor Game Module Driver Contract Metadata Review v1",
            "Editor Game Module Driver Dynamic Probe v1",
            "LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR",
            "MK_editor_game_module_driver_probe",
            "MK_editor_game_module_driver_load_tests",
            "play_in_editor.game_module_driver",
            "play_in_editor.game_module_driver.reload",
            "play_in_editor.game_module_driver.contract",
            "Editor Game Module Driver CTest Probe Evidence UI v1",
            "play_in_editor.game_module_driver.ctest_probe_evidence",
            "mirakana_create_editor_game_module_driver_v1"
        )
    },
    @{
        Path = "docs/roadmap.md"
        Needles = @(
            "Editor Dynamic Game Module Driver Load v1",
            "Editor Game Module Driver Safe Reload Review v1",
            "Editor Game Module Driver Contract Metadata Review v1",
            "Editor Game Module Driver Dynamic Probe v1",
            "MK_editor_game_module_driver_probe",
            "MK_editor_game_module_driver_load_tests",
            "Reload Game Module Driver",
            "DesktopGameRunner"
        )
    },
    @{
        Path = "docs/architecture.md"
        Needles = @(
            "DynamicLibrary",
            "LoadLibraryExW",
            "LOAD_LIBRARY_SEARCH_SYSTEM32"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "currentEditorGameModuleDriverLoad",
            "GameEngine.EditorGameModuleDriver.v1",
            "play_in_editor.game_module_driver",
            "play_in_editor.game_module_driver.reload",
            "play_in_editor.game_module_driver.contract",
            "play_in_editor.game_module_driver.unload",
            "play_in_editor.game_module_driver.ctest_probe_evidence",
            "play_in_editor.game_module_driver.session",
            "EditorGameModuleDriverContractMetadataModel",
            "EditorGameModuleDriverReloadModel",
            "EditorGameModuleDriverCtestProbeEvidenceModel",
            "EditorGameModuleDriverHostSessionPhase",
            "MK_editor_game_module_driver_probe",
            "MK_editor_game_module_driver_load_tests",
            "Reload Game Module Driver"
        )
    },
    @{
        Path = ".agents/skills/editor-change/SKILL.md"
        Needles = @(
            "EditorGameModuleDriverApi",
            "play_in_editor.game_module_driver",
            "play_in_editor.game_module_driver.reload",
            "play_in_editor.game_module_driver.contract",
            "play_in_editor.game_module_driver.ctest_probe_evidence",
            "make_editor_game_module_driver_contract_metadata_model",
            "MK_editor_game_module_driver_probe",
            "MK_editor_game_module_driver_load_tests",
            "Reload Game Module Driver",
            "Game Module Driver"
        )
    },
    @{
        Path = ".claude/skills/gameengine-editor/SKILL.md"
        Needles = @(
            "EditorGameModuleDriverApi",
            "play_in_editor.game_module_driver",
            "play_in_editor.game_module_driver.reload",
            "play_in_editor.game_module_driver.contract",
            "play_in_editor.game_module_driver.ctest_probe_evidence",
            "make_editor_game_module_driver_contract_metadata_model",
            "MK_editor_game_module_driver_probe",
            "MK_editor_game_module_driver_load_tests",
            "Reload Game Module Driver",
            "Game Module Driver"
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-09-editor-game-module-driver-dynamic-probe-v1.md"
        Needles = @(
            "Editor Game Module Driver Dynamic Probe Implementation Plan",
            "MK_editor_game_module_driver_probe",
            "MK_editor_game_module_driver_load_tests",
            "mirakana_create_editor_game_module_driver_v1",
            "stable third-party ABI"
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-09-editor-dynamic-game-module-driver-load-v1.md"
        Needles = @(
            "Editor Dynamic Game Module Driver Load Implementation Plan",
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1",
            "LoadLibraryExW",
            "play_in_editor.game_module_driver"
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-09-editor-game-module-driver-safe-reload-review-v1.md"
        Needles = @(
            "Editor Game Module Driver Safe Reload Review Implementation Plan",
            "EditorGameModuleDriverReloadModel",
            "make_editor_game_module_driver_reload_model",
            "play_in_editor.game_module_driver.reload",
            "Reload Game Module Driver",
            "active-session hot reload"
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-09-editor-game-module-driver-contract-metadata-review-v1.md"
        Needles = @(
            "Editor Game Module Driver Contract Metadata Review Implementation Plan",
            "EditorGameModuleDriverContractMetadataModel",
            "make_editor_game_module_driver_contract_metadata_model",
            "play_in_editor.game_module_driver.contract",
            "same-engine-build",
            "stable third-party ABI"
        )
    }
)
foreach ($check in $editorGameModuleDriverLoadChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing editor dynamic game-module driver load contract: $($missingNeedles -join ', ')"
    }
}

$editorRuntimeScenePackageValidationExecutionChecks = @(
    @{
        Path = "editor/core/include/mirakana/editor/playtest_package_review.hpp"
        Needles = @(
            "EditorRuntimeScenePackageValidationExecutionDesc",
            "EditorRuntimeScenePackageValidationExecutionModel",
            "EditorRuntimeScenePackageValidationExecutionResult",
            "make_editor_runtime_scene_package_validation_execution_model",
            "execute_editor_runtime_scene_package_validation",
            "make_editor_runtime_scene_package_validation_execution_ui_model"
        )
    },
    @{
        Path = "editor/core/src/playtest_package_review.cpp"
        Needles = @(
            "playtest_package_review.runtime_scene_validation",
            "plan_runtime_scene_package_validation",
            "execute_runtime_scene_package_validation",
            "request_arbitrary_shell_execution",
            "request_renderer_rhi_residency"
        )
    },
    @{
        Path = "editor/src/main.cpp"
        Needles = @(
            "make_runtime_scene_package_validation_execution_model",
            "Validate Runtime Scene Package",
            "execute_editor_runtime_scene_package_validation",
            "runtime_scene_package_validation_result_",
            "tool_filesystem_"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor runtime scene package validation execution records reviewed evidence",
            "EditorRuntimeScenePackageValidationExecutionDesc",
            "execute_editor_runtime_scene_package_validation",
            "playtest_package_review.runtime_scene_validation",
            "request_arbitrary_shell_execution"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Editor Runtime Scene Package Validation Execution v1",
            "EditorRuntimeScenePackageValidationExecutionModel",
            "execute_editor_runtime_scene_package_validation",
            "Validate Runtime Scene Package",
            "playtest_package_review.runtime_scene_validation"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Editor Runtime Scene Package Validation Execution v1",
            "EditorRuntimeScenePackageValidationExecutionModel",
            "Validate Runtime Scene Package",
            "contentRoot must not duplicate"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "Editor Runtime Scene Package Validation Execution v1 coverage",
            "EditorRuntimeScenePackageValidationExecutionResult",
            "playtest_package_review.runtime_scene_validation",
            "failed-package diagnostics"
        )
    },
    @{
        Path = "docs/roadmap.md"
        Needles = @(
            "Editor Runtime Scene Package Validation Execution v1",
            "EditorRuntimeScenePackageValidationExecutionModel",
            "Validate Runtime Scene Package",
            "host-gated desktop smoke"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "currentEditorRuntimeScenePackageValidationExecution",
            "EditorRuntimeScenePackageValidationExecutionDesc",
            "execute_editor_runtime_scene_package_validation",
            "playtest_package_review.runtime_scene_validation",
            "Validate Runtime Scene Package",
            "content roots that prefix cooked package entry paths"
        )
    },
    @{
        Path = ".agents/skills/editor-change/SKILL.md"
        Needles = @(
            "make_editor_runtime_scene_package_validation_execution_model",
            "execute_editor_runtime_scene_package_validation",
            "Validate Runtime Scene Package",
            "caller-owned filesystem"
        )
    },
    @{
        Path = ".claude/skills/gameengine-editor/SKILL.md"
        Needles = @(
            "make_editor_runtime_scene_package_validation_execution_model",
            "execute_editor_runtime_scene_package_validation",
            "Validate Runtime Scene Package",
            "caller-owned filesystem"
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-09-editor-runtime-scene-package-validation-execution-v1.md"
        Needles = @(
            "Editor Runtime Scene Package Validation Execution Implementation Plan",
            "EditorRuntimeScenePackageValidationExecutionModel",
            "playtest_package_review.runtime_scene_validation",
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1"
        )
    }
)
foreach ($check in $editorRuntimeScenePackageValidationExecutionChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing editor runtime scene package validation execution contract: $($missingNeedles -join ', ')"
    }
}

$editorProfilerTraceExportChecks = @(
    @{
        Path = "engine/core/include/mirakana/core/diagnostics.hpp"
        Needles = @(
            "DiagnosticsTraceImportReview",
            "DiagnosticsTraceImportResult",
            "DiagnosticCapture",
            "trace_event_count",
            "metadata_event_count",
            "instant_event_count",
            "counter_event_count",
            "profile_event_count",
            "unknown_event_count",
            "review_diagnostics_trace_json",
            "import_diagnostics_trace_json"
        )
    },
    @{
        Path = "engine/core/src/diagnostics.cpp"
        Needles = @(
            "TraceJsonReviewParser",
            "review_diagnostics_trace_json",
            "import_diagnostics_trace_json",
            "reconstruct_trace_event",
            "trace event phase '",
            "cannot be reconstructed",
            "trace import review requires non-empty JSON",
            "trace import review requires a traceEvents array",
            "traceEvents entries must be objects",
            "metadata_event_count",
            "unknown_event_count"
        )
    },
    @{
        Path = "editor/core/include/mirakana/editor/profiler.hpp"
        Needles = @(
            "EditorProfilerTraceExportModel",
            "EditorProfilerTraceFileSaveRequest",
            "EditorProfilerTraceFileSaveResult",
            "EditorProfilerTraceSaveDialogModel",
            "EditorProfilerTelemetryHandoffModel",
            "EditorProfilerTraceImportReviewModel",
            "capture_reconstructed",
            "reconstructed_profile_rows",
            "reconstructed_counter_rows",
            "reconstructed_event_rows",
            "EditorProfilerTraceFileImportRequest",
            "EditorProfilerTraceFileImportResult",
            "EditorProfilerTraceOpenDialogModel",
            "trace_export",
            "trace_file_save",
            "trace_save_dialog",
            "telemetry",
            "trace_import",
            "reconstructed_profile_rows",
            "trace_file_import",
            "trace_open_dialog",
            "payload_bytes",
            "make_editor_profiler_trace_export_model",
            "make_editor_profiler_trace_save_dialog_request",
            "make_editor_profiler_trace_save_dialog_model",
            "make_editor_profiler_telemetry_handoff_model",
            "make_editor_profiler_trace_import_review_model",
            "save_editor_profiler_trace_json",
            "import_editor_profiler_trace_json",
            "make_editor_profiler_trace_open_dialog_request",
            "make_editor_profiler_trace_open_dialog_model",
            "DiagnosticsOpsPlanOptions",
            "DiagnosticsTraceExportOptions"
        )
    },
    @{
        Path = "editor/core/src/profiler.cpp"
        Needles = @(
            "make_editor_profiler_trace_export_model",
            "save_editor_profiler_trace_json",
            "mirakana::export_diagnostics_trace_json",
            "Trace export ready",
            "Trace file saved",
            "trace export requires at least one diagnostic sample",
            "trace file save requires at least one diagnostic sample",
            "make_editor_profiler_trace_save_dialog_request",
            "make_editor_profiler_trace_save_dialog_model",
            "Trace save dialog accepted",
            "trace save dialog requires exactly one selected path",
            "make_editor_profiler_telemetry_handoff_model",
            "mirakana::build_diagnostics_ops_plan",
            "telemetry_upload",
            "make_editor_profiler_trace_import_review_model",
            "mirakana::import_diagnostics_trace_json",
            "import_editor_profiler_trace_json",
            "reconstructed_profile_rows",
            "profiler.trace_import.reconstructed_profiles",
            "trace file import path",
            "Trace file import ready",
            "Trace file import blocked",
            "make_editor_profiler_trace_open_dialog_request",
            "make_editor_profiler_trace_open_dialog_model",
            "Trace open dialog accepted",
            "trace open dialog requires exactly one selected path",
            "Trace import ready",
            "Trace import blocked",
            "profiler.trace_export",
            "profiler.trace_export.diagnostics",
            "profiler.trace_file_save",
            "profiler.trace_file_save.diagnostics",
            "profiler.trace_save_dialog",
            "profiler.trace_save_dialog.diagnostics",
            "profiler.telemetry",
            "profiler.telemetry.diagnostics",
            "profiler.trace_import",
            "profiler.trace_import.diagnostics",
            "profiler.trace_file_import",
            "profiler.trace_file_import.diagnostics",
            "profiler.trace_open_dialog",
            "profiler.trace_open_dialog.diagnostics"
        )
    },
    @{
        Path = "editor/src/main.cpp"
        Needles = @(
            "Profiler Trace Export",
            "Copy Trace JSON",
            "Save Trace JSON",
            "Browse Save Trace JSON",
            "Trace Path",
            "Profiler Telemetry Handoff",
            "Trace JSON Review",
            "Review Trace JSON",
            "Trace Import Path",
            "Import Trace JSON",
            "Browse Trace JSON",
            "SdlFileDialogService",
            "make_editor_profiler_trace_save_dialog_request",
            "make_editor_profiler_trace_save_dialog_model",
            "make_editor_profiler_trace_open_dialog_request",
            "make_editor_profiler_trace_open_dialog_model",
            "Profiler Trace Open Dialog",
            "ImGui::SetClipboardText",
            "save_editor_profiler_trace_json",
            "import_editor_profiler_trace_json",
            "make_editor_profiler_trace_import_review_model",
            "profiler_trace_export_path_",
            "profiler_trace_export_status_",
            "profiler_trace_save_dialog_id_",
            "profiler_trace_save_dialog_",
            "profiler_trace_import_path_",
            "profiler_trace_open_dialog_id_",
            "profiler_trace_open_dialog_",
            "profiler_trace_file_import_",
            "profiler_trace_import_payload_",
            "profiler_trace_import_review_",
            "Trace JSON copied",
            "Trace JSON saved",
            "Trace JSON imported",
            "Trace JSON review ready"
        )
    },
    @{
        Path = "tests/unit/core_tests.cpp"
        Needles = @(
            "diagnostics trace import review classifies trace event json",
            "review_diagnostics_trace_json",
            "trace_event_count",
            "metadata_event_count",
            "trace import review requires non-empty JSON",
            "trace import review requires a traceEvents array",
            "traceEvents entries must be objects"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor profiler trace export model exposes deterministic trace json",
            "editor profiler trace file save writes project relative json",
            "editor profiler native trace save dialog reviews selection",
            "editor profiler telemetry handoff reports backend readiness",
            "editor profiler trace import review model reports pasted trace json",
            "editor profiler trace file import review reads project relative json",
            "editor profiler native trace open dialog reviews selection",
            "make_editor_profiler_trace_export_model",
            "make_editor_profiler_telemetry_handoff_model",
            "make_editor_profiler_trace_import_review_model",
            "save_editor_profiler_trace_json",
            "make_editor_profiler_trace_save_dialog_request",
            "make_editor_profiler_trace_save_dialog_model",
            "import_editor_profiler_trace_json",
            "make_editor_profiler_trace_open_dialog_request",
            "make_editor_profiler_trace_open_dialog_model",
            "profiler.trace_export.payload_bytes.value",
            "profiler.trace_file_save.output_path.value",
            "profiler.trace_save_dialog.status.value",
            "profiler.telemetry.status.value",
            "profiler.trace_import.events.value",
            "profiler.trace_import.metadata.value",
            "profiler.trace_file_import.events.value",
            "profiler.trace_file_import.input_path.value",
            "profiler.trace_open_dialog.status.value",
            "trace export requires at least one diagnostic sample",
            "trace file save requires at least one diagnostic sample",
            "trace file import failed",
            "Trace open dialog accepted",
            "Trace save dialog accepted",
            "renderer.frames_started"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Editor Profiler Trace Export v1",
            "Editor Profiler Trace File Save v1",
            "Editor Profiler Native Trace Save Dialog v1",
            "Editor Profiler Telemetry Handoff v1",
            "Editor Profiler Trace Import Review v1",
            "Editor Profiler Trace File Import Review v1",
            "Editor Profiler Native Trace Open Dialog v1",
            "Trace Import Reconstruction v1",
            "EditorProfilerTraceExportModel",
            "EditorProfilerTraceFileSaveRequest",
            "EditorProfilerTraceSaveDialogModel",
            "EditorProfilerTelemetryHandoffModel",
            "EditorProfilerTraceImportReviewModel",
            "EditorProfilerTraceFileImportRequest",
            "EditorProfilerTraceFileImportResult",
            "EditorProfilerTraceOpenDialogModel",
            "make_editor_profiler_trace_export_model",
            "make_editor_profiler_trace_save_dialog_request",
            "make_editor_profiler_trace_save_dialog_model",
            "make_editor_profiler_telemetry_handoff_model",
            "make_editor_profiler_trace_import_review_model",
            "make_editor_profiler_trace_open_dialog_request",
            "make_editor_profiler_trace_open_dialog_model",
            "save_editor_profiler_trace_json",
            "import_editor_profiler_trace_json",
            "import_diagnostics_trace_json",
            "profiler.trace_import.reconstructed_*",
            "review_diagnostics_trace_json",
            "profiler.trace_export",
            "profiler.trace_file_save",
            "profiler.trace_save_dialog",
            "profiler.telemetry",
            "profiler.trace_import",
            "profiler.trace_file_import",
            "profiler.trace_open_dialog",
            "Copy Trace JSON",
            "Save Trace JSON",
            "Browse Save Trace JSON",
            "Profiler Telemetry Handoff",
            "Review Trace JSON",
            "Import Trace JSON",
            "Browse Trace JSON"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Editor Profiler Trace Export v1, Trace File Save v1, Native Trace Save Dialog v1, Telemetry Handoff v1, Trace Import Review v1, Trace File Import Review v1, Native Trace Open Dialog v1, and Trace Import Reconstruction v1",
            "DiagnosticsTraceImportReview",
            "DiagnosticsTraceImportResult",
            "review_diagnostics_trace_json",
            "import_diagnostics_trace_json",
            "EditorProfilerTraceExportModel",
            "EditorProfilerTraceFileSaveRequest",
            "EditorProfilerTraceSaveDialogModel",
            "EditorProfilerTelemetryHandoffModel",
            "EditorProfilerTraceImportReviewModel",
            "EditorProfilerTraceFileImportRequest",
            "EditorProfilerTraceFileImportResult",
            "EditorProfilerTraceOpenDialogModel",
            "make_editor_profiler_trace_export_model",
            "make_editor_profiler_trace_save_dialog_request",
            "make_editor_profiler_trace_save_dialog_model",
            "make_editor_profiler_telemetry_handoff_model",
            "make_editor_profiler_trace_import_review_model",
            "make_editor_profiler_trace_open_dialog_request",
            "make_editor_profiler_trace_open_dialog_model",
            "save_editor_profiler_trace_json",
            "import_editor_profiler_trace_json",
            "profiler.trace_export",
            "profiler.trace_file_save",
            "profiler.trace_save_dialog",
            "profiler.telemetry",
            "profiler.trace_import",
            "profiler.trace_import.reconstructed_*",
            "profiler.trace_file_import",
            "profiler.trace_open_dialog",
            "Copy Trace JSON",
            "Save Trace JSON",
            "Browse Save Trace JSON",
            "Profiler Telemetry Handoff",
            "Review Trace JSON",
            "Import Trace JSON",
            "Browse Trace JSON",
            'broader editor native save/open dialogs outside this Profiler Trace JSON path, the Scene `.scene` browse path, and the Prefab Variant `.prefabvariant` browse path',
            "arbitrary JSON conversion",
            "first-party exported Trace Event JSON"
        )
    },
    @{
        Path = "docs/roadmap.md"
        Needles = @(
            "Editor Profiler Trace Export v1",
            "Editor Profiler Trace File Save v1",
            "Editor Profiler Native Trace Save Dialog v1",
            "Editor Profiler Telemetry Handoff v1",
            "Editor Profiler Trace Import Review v1",
            "Editor Profiler Trace File Import Review v1",
            "Editor Profiler Native Trace Open Dialog v1",
            "Editor Profiler Trace Import Reconstruction v1",
            "DiagnosticsTraceImportReview",
            "DiagnosticsTraceImportResult",
            "review_diagnostics_trace_json",
            "import_diagnostics_trace_json",
            "EditorProfilerTraceExportModel",
            "EditorProfilerTraceFileSaveRequest",
            "EditorProfilerTraceSaveDialogModel",
            "EditorProfilerTelemetryHandoffModel",
            "EditorProfilerTraceImportReviewModel",
            "EditorProfilerTraceFileImportRequest",
            "EditorProfilerTraceFileImportResult",
            "EditorProfilerTraceOpenDialogModel",
            "make_editor_profiler_trace_export_model",
            "make_editor_profiler_trace_save_dialog_request",
            "make_editor_profiler_trace_save_dialog_model",
            "make_editor_profiler_telemetry_handoff_model",
            "make_editor_profiler_trace_import_review_model",
            "make_editor_profiler_trace_open_dialog_request",
            "make_editor_profiler_trace_open_dialog_model",
            "save_editor_profiler_trace_json",
            "import_editor_profiler_trace_json",
            "profiler.trace_export",
            "profiler.trace_file_save",
            "profiler.trace_save_dialog",
            "profiler.telemetry",
            "profiler.trace_import",
            "profiler.trace_import.reconstructed_*",
            "profiler.trace_file_import",
            "profiler.trace_open_dialog",
            "Copy Trace JSON",
            "Save Trace JSON",
            "Browse Save Trace JSON",
            "Profiler Telemetry Handoff",
            "Review Trace JSON",
            "Import Trace JSON",
            "Browse Trace JSON"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "Editor Profiler Trace Export v1, Trace File Save v1, Native Trace Save Dialog v1, Telemetry Handoff v1, Trace Import Review v1, Trace File Import Review v1, Native Trace Open Dialog v1, and Trace Import Reconstruction v1",
            "DiagnosticsTraceImportReview",
            "DiagnosticsTraceImportResult",
            "review_diagnostics_trace_json",
            "import_diagnostics_trace_json",
            "EditorProfilerTraceExportModel",
            "EditorProfilerTraceFileSaveRequest",
            "EditorProfilerTraceSaveDialogModel",
            "EditorProfilerTelemetryHandoffModel",
            "EditorProfilerTraceImportReviewModel",
            "EditorProfilerTraceFileImportRequest",
            "EditorProfilerTraceFileImportResult",
            "EditorProfilerTraceOpenDialogModel",
            "make_editor_profiler_trace_export_model",
            "make_editor_profiler_trace_save_dialog_request",
            "make_editor_profiler_trace_save_dialog_model",
            "make_editor_profiler_telemetry_handoff_model",
            "make_editor_profiler_trace_import_review_model",
            "make_editor_profiler_trace_open_dialog_request",
            "make_editor_profiler_trace_open_dialog_model",
            "save_editor_profiler_trace_json",
            "import_editor_profiler_trace_json",
            "profiler.trace_export",
            "profiler.trace_file_save",
            "profiler.trace_save_dialog",
            "profiler.telemetry",
            "profiler.trace_import",
            "profiler.trace_import.reconstructed_*",
            "profiler.trace_file_import",
            "profiler.trace_open_dialog",
            "mirakana::export_diagnostics_trace_json",
            "Save Trace JSON",
            "Browse Save Trace JSON",
            "Review Trace JSON",
            "Import Trace JSON",
            "Browse Trace JSON",
            "first-party exported"
        )
    },
    @{
        Path = "docs/superpowers/plans/README.md"
        Needles = @(
            "2026-05-07-editor-profiler-trace-import-reconstruction-v1.md",
            "Editor Profiler Trace Import Reconstruction v1",
            "DiagnosticsTraceImportResult",
            "import_diagnostics_trace_json",
            "profiler.trace_import.reconstructed_*",
            "2026-05-07-editor-profiler-native-trace-save-dialog-v1.md",
            "Editor Profiler Native Trace Save Dialog v1",
            "EditorProfilerTraceSaveDialogModel",
            "make_editor_profiler_trace_save_dialog_request",
            "make_editor_profiler_trace_save_dialog_model",
            "profiler.trace_save_dialog",
            "Browse Save Trace JSON",
            "2026-05-07-editor-profiler-native-trace-open-dialog-v1.md",
            "Editor Profiler Native Trace Open Dialog v1",
            "EditorProfilerTraceOpenDialogModel",
            "make_editor_profiler_trace_open_dialog_request",
            "make_editor_profiler_trace_open_dialog_model",
            "profiler.trace_open_dialog",
            "Browse Trace JSON",
            "2026-05-07-editor-profiler-trace-file-import-review-v1.md",
            "Editor Profiler Trace File Import Review v1",
            "EditorProfilerTraceFileImportRequest",
            "import_editor_profiler_trace_json",
            "profiler.trace_file_import",
            "Import Trace JSON",
            "2026-05-07-editor-profiler-trace-import-review-v1.md",
            "Editor Profiler Trace Import Review v1",
            "DiagnosticsTraceImportReview",
            "profiler.trace_import",
            "Review Trace JSON",
            "2026-05-07-editor-profiler-telemetry-handoff-v1.md",
            "Editor Profiler Telemetry Handoff v1",
            "EditorProfilerTelemetryHandoffModel",
            "profiler.telemetry",
            "Profiler Telemetry Handoff",
            "2026-05-07-editor-profiler-trace-file-save-v1.md",
            "Editor Profiler Trace File Save v1",
            "EditorProfilerTraceFileSaveRequest",
            "profiler.trace_file_save",
            "Save Trace JSON",
            "2026-05-07-editor-profiler-trace-export-v1.md",
            "Editor Profiler Trace Export v1",
            "EditorProfilerTraceExportModel",
            "profiler.trace_export",
            "Copy Trace JSON"
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-07-editor-profiler-trace-import-reconstruction-v1.md"
        Needles = @(
            "Editor Profiler Trace Import Reconstruction v1 Implementation Plan",
            "DiagnosticsTraceImportResult",
            "import_diagnostics_trace_json",
            "EditorProfilerTraceImportReviewModel",
            "EditorProfilerTraceFileImportResult",
            "profiler.trace_import.reconstructed_*",
            "first-party exported",
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1"
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-07-editor-profiler-native-trace-save-dialog-v1.md"
        Needles = @(
            "Editor Profiler Native Trace Save Dialog v1 Implementation Plan",
            "EditorProfilerTraceSaveDialogModel",
            "make_editor_profiler_trace_save_dialog_request",
            "make_editor_profiler_trace_save_dialog_model",
            "profiler.trace_save_dialog",
            "Browse Save Trace JSON",
            "SdlFileDialogService",
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1"
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-07-editor-profiler-native-trace-open-dialog-v1.md"
        Needles = @(
            "Editor Profiler Native Trace Open Dialog v1 Implementation Plan",
            "EditorProfilerTraceOpenDialogModel",
            "make_editor_profiler_trace_open_dialog_request",
            "make_editor_profiler_trace_open_dialog_model",
            "profiler.trace_open_dialog",
            "Browse Trace JSON",
            "SdlFileDialogService",
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1"
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-07-editor-profiler-trace-file-import-review-v1.md"
        Needles = @(
            "Editor Profiler Trace File Import Review v1 Implementation Plan",
            "EditorProfilerTraceFileImportRequest",
            "EditorProfilerTraceFileImportResult",
            "import_editor_profiler_trace_json",
            "profiler.trace_file_import",
            "Trace Import Path",
            "Import Trace JSON",
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1"
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-07-editor-profiler-trace-import-review-v1.md"
        Needles = @(
            "Editor Profiler Trace Import Review v1 Implementation Plan",
            "DiagnosticsTraceImportReview",
            "review_diagnostics_trace_json",
            "EditorProfilerTraceImportReviewModel",
            "make_editor_profiler_trace_import_review_model",
            "profiler.trace_import",
            "Review Trace JSON",
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1"
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-07-editor-profiler-telemetry-handoff-v1.md"
        Needles = @(
            "Editor Profiler Telemetry Handoff v1 Implementation Plan",
            "EditorProfilerTelemetryHandoffModel",
            "make_editor_profiler_telemetry_handoff_model",
            "profiler.telemetry",
            "Profiler Telemetry Handoff",
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1"
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-07-editor-profiler-trace-file-save-v1.md"
        Needles = @(
            "Editor Profiler Trace File Save v1 Implementation Plan",
            "EditorProfilerTraceFileSaveRequest",
            "EditorProfilerTraceFileSaveResult",
            "save_editor_profiler_trace_json",
            "profiler.trace_file_save",
            "Save Trace JSON",
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1"
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-07-editor-profiler-trace-export-v1.md"
        Needles = @(
            "Editor Profiler Trace Export v1 Implementation Plan",
            "EditorProfilerTraceExportModel",
            "make_editor_profiler_trace_export_model",
            "profiler.trace_export",
            "Copy Trace JSON",
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1"
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md"
        Needles = @(
            "Profiler trace JSON copy export",
            "project-relative Profiler trace JSON file save",
            "native Profiler Trace JSON save dialog",
            "read-only Profiler telemetry handoff rows",
            "pasted Profiler Trace JSON review",
            "project-relative Profiler Trace JSON file import review/reconstruction",
            "native Profiler Trace JSON open dialog",
            "broader editor native save/open dialogs outside Profiler",
            "arbitrary trace conversion beyond first-party exported Trace Event JSON reconstruction",
            "telemetry SDK/upload/backend follow-ups"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "currentEditorProfilerTraceExport",
            "DiagnosticsTraceImportReview",
            "DiagnosticsTraceImportResult",
            "review_diagnostics_trace_json",
            "import_diagnostics_trace_json",
            "EditorProfilerTraceExportModel",
            "EditorProfilerTraceFileSaveRequest",
            "EditorProfilerTraceSaveDialogModel",
            "EditorProfilerTraceImportReviewModel",
            "EditorProfilerTraceFileImportRequest",
            "EditorProfilerTraceFileImportResult",
            "EditorProfilerTraceOpenDialogModel",
            "make_editor_profiler_trace_export_model",
            "make_editor_profiler_trace_save_dialog_request",
            "make_editor_profiler_trace_save_dialog_model",
            "make_editor_profiler_telemetry_handoff_model",
            "make_editor_profiler_trace_import_review_model",
            "make_editor_profiler_trace_open_dialog_request",
            "make_editor_profiler_trace_open_dialog_model",
            "save_editor_profiler_trace_json",
            "import_editor_profiler_trace_json",
            "profiler.trace_export",
            "profiler.trace_file_save",
            "profiler.trace_save_dialog",
            "profiler.telemetry",
            "profiler.trace_import",
            "profiler.trace_import.reconstructed_*",
            "profiler.trace_file_import",
            "profiler.trace_open_dialog",
            "mirakana::export_diagnostics_trace_json",
            "Copy Trace JSON",
            "Save Trace JSON",
            "Browse Save Trace JSON",
            "Profiler Telemetry Handoff",
            "Review Trace JSON",
            "Import Trace JSON",
            "Browse Trace JSON",
            "SdlFileDialogService",
            "broader editor native save/open dialogs outside Profiler",
            "arbitrary JSON conversion",
            "first-party exported Trace Event JSON subset"
        )
    },
    @{
        Path = ".agents/skills/editor-change/SKILL.md"
        Needles = @(
            "make_editor_profiler_trace_export_model",
            "make_editor_profiler_telemetry_handoff_model",
            "make_editor_profiler_trace_import_review_model",
            "make_editor_profiler_trace_open_dialog_request",
            "make_editor_profiler_trace_open_dialog_model",
            "save_editor_profiler_trace_json",
            "import_editor_profiler_trace_json",
            "profiler.trace_export",
            "profiler.trace_file_save",
            "profiler.trace_save_dialog",
            "profiler.telemetry",
            "profiler.trace_import",
            "profiler.trace_import.reconstructed_*",
            "profiler.trace_file_import",
            "profiler.trace_open_dialog",
            "mirakana::export_diagnostics_trace_json",
            "mirakana::import_diagnostics_trace_json",
            "Copy Trace JSON",
            "Save Trace JSON",
            "Browse Save Trace JSON",
            "Review Trace JSON",
            "Import Trace JSON",
            "Browse Trace JSON",
            "arbitrary JSON conversion beyond first-party exported Trace Event JSON reconstruction"
        )
    },
    @{
        Path = ".claude/skills/gameengine-editor/SKILL.md"
        Needles = @(
            "make_editor_profiler_trace_export_model",
            "make_editor_profiler_trace_save_dialog_request",
            "make_editor_profiler_trace_save_dialog_model",
            "make_editor_profiler_telemetry_handoff_model",
            "make_editor_profiler_trace_import_review_model",
            "make_editor_profiler_trace_open_dialog_request",
            "make_editor_profiler_trace_open_dialog_model",
            "save_editor_profiler_trace_json",
            "import_editor_profiler_trace_json",
            "profiler.trace_export",
            "profiler.trace_file_save",
            "profiler.trace_save_dialog",
            "profiler.telemetry",
            "profiler.trace_import",
            "profiler.trace_import.reconstructed_*",
            "profiler.trace_file_import",
            "profiler.trace_open_dialog",
            "mirakana::export_diagnostics_trace_json",
            "mirakana::import_diagnostics_trace_json",
            "Copy Trace JSON",
            "Save Trace JSON",
            "Browse Save Trace JSON",
            "Review Trace JSON",
            "Import Trace JSON",
            "Browse Trace JSON",
            "arbitrary JSON conversion beyond first-party exported Trace Event JSON reconstruction"
        )
    }
)
foreach ($check in $editorProfilerTraceExportChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing editor profiler trace export contract: $($missingNeedles -join ', ')"
    }
}

$editorResourcePanelChecks = @(
    @{
        Path = "editor/core/include/mirakana/editor/resource_panel.hpp"
        Needles = @(
            "EditorResourcePanelInput",
            "EditorResourceMemoryInput",
            "EditorResourceLifetimeInput",
            "EditorResourcePanelModel",
            "make_editor_resource_panel_model",
            "make_resource_panel_ui_model"
        )
    },
    @{
        Path = "editor/core/src/resource_panel.cpp"
        Needles = @(
            "format_budget_usage",
            "local_video_memory",
            "committed_resources",
            "No RHI device diagnostics available",
            'append_rows(document, root, "memory"'
        )
    },
    @{
        Path = "editor/core/include/mirakana/editor/workspace.hpp"
        Needles = @(
            "PanelId",
            "resources"
        )
    },
    @{
        Path = "editor/core/src/workspace.cpp"
        Needles = @(
            'PanelToken{.id = PanelId::resources, .token = "resources"}',
            "PanelState{.id = PanelId::resources, .visible = false}"
        )
    },
    @{
        Path = "editor/src/main.cpp"
        Needles = @(
            "make_resource_panel_input",
            "draw_resources_panel",
            "view.resources",
            "viewport_device_->memory_diagnostics",
            "resource_lifetime_registry",
            "make_editor_resource_panel_model"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor resource panel model summarizes ready rhi diagnostics",
            "editor resource panel model reports no device explicitly",
            "panel.resources=visible",
            "EditorResourcePanelInput",
            "make_editor_resource_panel_model"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Editor Resource Panel Diagnostics v1",
            "EditorResourcePanelModel",
            "make_editor_resource_panel_model",
            "Resources panel",
            "residency enforcement"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Editor Resource Panel Diagnostics v1",
            "EditorResourcePanelModel",
            "memory_diagnostics",
            "residency enforcement"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "Editor Resource Panel Diagnostics v1",
            "EditorResourcePanelModel",
            "workspace resources panel state",
            "read-only Resources diagnostics",
            "residency enforcement"
        )
    }
)
foreach ($check in $editorResourcePanelChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing editor resource panel diagnostics contract: $($missingNeedles -join ', ')"
    }
}

$editorResourceCaptureRequestChecks = @(
    @{
        Path = "editor/core/include/mirakana/editor/resource_panel.hpp"
        Needles = @(
            "EditorResourceCaptureRequestInput",
            "EditorResourceCaptureRequestRow",
            "capture_requests",
            "capture_request_rows",
            "host_gate_acknowledgement_required"
        )
    },
    @{
        Path = "editor/core/src/resource_panel.cpp"
        Needles = @(
            "append_capture_requests",
            "resources.capture_requests",
            "host-gated external workflow",
            "external host workflow",
            "capture_request_input_less"
        )
    },
    @{
        Path = "editor/src/main.cpp"
        Needles = @(
            "append_resource_capture_requests",
            "draw_resource_capture_request_rows_table",
            "resource_acknowledged_capture_request_ids_",
            "PIX GPU Capture",
            "D3D12 Debug Layer / GPU Validation",
            "resource_capture."
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor resource capture request model exposes reviewed host gated requests",
            "editor resource capture request model blocks unavailable device requests",
            "resources.capture_requests.pix_gpu_capture.action",
            "EditorResourceCaptureRequestInput",
            "capture_request_rows"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Editor Resource Capture Request v1",
            "resources.capture_requests",
            "PIX GPU capture",
            "D3D12 debug-layer",
            "does not launch PIX automatically"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Resource Capture Request v1",
            "EditorResourcePanelModel",
            "resources.capture_requests",
            "does not launch PIX automatically"
        )
    },
    @{
        Path = "docs/ai-game-development.md"
        Needles = @(
            "Editor Resource Capture Request v1",
            "EditorResourceCaptureRequestInput",
            "resources.capture_requests",
            "Generated games must not depend"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "Editor Resource Capture Request v1 coverage",
            "EditorResourceCaptureRequestRow",
            "blocked no-device diagnostics"
        )
    },
    @{
        Path = "docs/superpowers/plans/README.md"
        Needles = @(
            "2026-05-07-editor-resource-capture-request-v1.md",
            "Editor Resource Capture Request v1",
            "resources.capture_requests"
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md"
        Needles = @(
            "Editor Resource Capture Request v1",
            "reviewed Resources capture request handoff rows",
            "resource management/capture execution beyond host-owned evidence rows"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "currentEditorResourceCaptureRequests",
            "EditorResourceCaptureRequestInput",
            "resources.capture_requests",
            "does not launch PIX automatically"
        )
    },
    @{
        Path = ".agents/skills/editor-change/SKILL.md"
        Needles = @(
            "reviewed capture request handoff rows",
            "EditorResourceCaptureRequestInput",
            "resources.capture_execution.pix_gpu_capture.host_helper_hint"
        )
    },
    @{
        Path = ".claude/skills/gameengine-editor/SKILL.md"
        Needles = @(
            "reviewed capture request handoff rows",
            "EditorResourceCaptureRequestInput",
            "resources.capture_execution.pix_gpu_capture.host_helper_hint"
        )
    }
)
foreach ($check in $editorResourceCaptureRequestChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing editor resource capture request contract: $($missingNeedles -join ', ')"
    }
}
