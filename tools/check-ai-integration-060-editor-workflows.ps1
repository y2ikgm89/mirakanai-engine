#requires -Version 7.0
#requires -PSEdition Core

# Chapter 6 for check-ai-integration.ps1 static contracts.

$editorInputRebindingProfilePanelChecks = @(
    @{
        Path = "editor/core/include/mirakana/editor/input_rebinding.hpp"
        Needles = @(
            "EditorInputRebindingProfilePanelStatus",
            "EditorInputRebindingProfileBindingRow",
            "EditorInputRebindingProfilePanelModel",
            "make_editor_input_rebinding_profile_panel_model",
            "make_input_rebinding_profile_panel_ui_model",
            "EditorInputRebindingCaptureStatus",
            "EditorInputRebindingCaptureModel",
            "make_editor_input_rebinding_capture_action_model",
            "make_input_rebinding_capture_action_ui_model",
            "EditorInputRebindingAxisCaptureDesc",
            "EditorInputRebindingAxisCaptureModel",
            "make_editor_input_rebinding_capture_axis_model",
            "make_input_rebinding_capture_axis_ui_model",
            "EditorInputRebindingProfilePersistenceUiModel",
            "validate_editor_input_rebinding_profile_store_path",
            "save_editor_input_rebinding_profile_to_project_store",
            "load_editor_input_rebinding_profile_from_project_store"
        )
    },
    @{
        Path = "editor/core/src/input_rebinding.cpp"
        Needles = @(
            "input_rebinding.profile.id",
            "input_rebinding.bindings.",
            "input_rebinding.review.",
            "input_rebinding.capture",
            "make_editor_input_rebinding_profile_review_model",
            "make_input_rebinding_profile_panel_ui_model",
            "make_editor_input_rebinding_capture_action_model",
            "make_input_rebinding_capture_action_ui_model",
            "input_rebinding.capture.axis",
            "make_editor_input_rebinding_capture_axis_model",
            "make_input_rebinding_capture_axis_ui_model",
            "input_rebinding.persistence"
        )
    },
    @{
        Path = "editor/core/include/mirakana/editor/workspace.hpp"
        Needles = @(
            "input_rebinding"
        )
    },
    @{
        Path = "editor/core/src/workspace.cpp"
        Needles = @(
            'PanelToken{.id = PanelId::input_rebinding, .token = "input_rebinding"}',
            "PanelState{.id = PanelId::input_rebinding, .visible = false}"
        )
    },
    @{
        Path = "editor/CMakeLists.txt"
        Needles = @(
            "Native MK_editor shell is SDL3-free",
            "MK_editor_shell_common",
            "add_executable(MK_editor"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor input rebinding panel model exposes reviewed bindings and ui rows",
            "editor input rebinding capture model captures pressed key candidate",
            "editor input rebinding capture model waits without mutating profile",
            "editor input rebinding capture model blocks unsupported file command and native claims",
            "make_editor_input_rebinding_profile_panel_model",
            "make_editor_input_rebinding_capture_action_model",
            "panel.input_rebinding=visible",
            "input_rebinding.bindings.action.gameplay.confirm.current",
            "input_rebinding.capture.status",
            "input_rebinding.capture.diagnostics",
            "editor input rebinding axis capture model captures gamepad axis candidate",
            "input_rebinding.capture.axis",
            "editor input rebinding profile store path validation rejects unsafe paths",
            "editor input rebinding profile save load roundtrips over memory text store",
            "editor input rebinding profile panel ui exposes persistence rows when requested"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Editor Input Rebinding Profile Panel v1",
            "EditorInputRebindingProfilePanelModel",
            "Editor Input Rebinding Action Capture Panel v1",
            "EditorInputRebindingCaptureModel",
            "Editor Input Rebinding Axis Capture Gamepad v1",
            "make_editor_input_rebinding_capture_axis_model",
            "input_rebinding.capture.axis",
            "panel.input_rebinding=hidden",
            "in-memory profile",
            "Editor Input Rebinding Profile Persistence v1",
            "save_editor_input_rebinding_profile_to_project_store",
            "load_editor_input_rebinding_profile_from_project_store"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Editor Input Rebinding Profile Panel v1",
            "EditorInputRebindingProfilePanelModel",
            "Editor Input Rebinding Action Capture Panel v1",
            "EditorInputRebindingCaptureModel",
            "Editor Input Rebinding Axis Capture Gamepad v1",
            "capture_runtime_input_rebinding_axis",
            "in-memory profile"
        )
    },
    @{
        Path = "docs/ai-game-development.md"
        Needles = @(
            "Editor Input Rebinding Profile Panel v1",
            "EditorInputRebindingProfilePanelModel",
            "make_input_rebinding_profile_panel_ui_model",
            "EditorInputRebindingCaptureModel",
            "make_editor_input_rebinding_capture_action_model",
            "EditorInputRebindingAxisCaptureModel",
            "make_editor_input_rebinding_capture_axis_model",
            "input_rebinding.capture.axis",
            "capture_runtime_input_rebinding_axis",
            "in-memory profile",
            "interactive runtime/game rebinding"
        )
    },
    @{
        Path = "docs/roadmap.md"
        Needles = @(
            "Editor Input Rebinding Profile Panel v1",
            "EditorInputRebindingProfilePanelModel",
            "EditorInputRebindingCaptureModel",
            "EditorInputRebindingAxisCaptureModel",
            "capture_runtime_input_rebinding_axis",
            "in-memory profile",
            "interactive runtime/game rebinding panels"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "Editor Input Rebinding Profile Panel v1",
            "EditorInputRebindingProfilePanelModel",
            "make_input_rebinding_profile_panel_ui_model",
            "panel.input_rebinding=visible"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "2026-05-07-editor-input-rebinding-profile-panel-v1.md",
            "Editor Input Rebinding Profile Panel v1",
            "EditorInputRebindingProfilePanelModel",
            "input_rebinding",
            "2026-05-10-editor-input-rebinding-axis-capture-gamepad-v1.md",
            "Editor Input Rebinding Axis Capture Gamepad v1",
            "capture_runtime_input_rebinding_axis"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
        Needles = @(
            "editor-input-rebinding-profile-panel-v1",
            "EditorInputRebindingProfilePanelModel",
            "editor-input-rebinding-action-capture-panel-v1",
            "EditorInputRebindingCaptureModel",
            "reviewed editor action capture lane"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "Editor Input Rebinding Profile Panel v1",
            "EditorInputRebindingProfilePanelModel",
            "PanelId::input_rebinding",
            "interactive runtime/game rebinding panels"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "Editor Input Rebinding Profile Panel v1",
            "EditorInputRebindingProfilePanelModel",
            "make_input_rebinding_profile_panel_ui_model",
            "Editor Input Rebinding Action Capture Panel v1",
            "EditorInputRebindingCaptureModel",
            "Editor Input Rebinding Axis Capture Gamepad v1",
            "make_editor_input_rebinding_capture_axis_model",
            "make_editor_input_rebinding_capture_action_model",
            "input_rebinding.capture.axis",
            "currentEditorInputRebindingProfiles"
        )
    },
    @{
        Path = ".agents/skills/editor-change/SKILL.md"
        Needles = @(
            "make_editor_input_rebinding_profile_panel_model",
            "make_input_rebinding_profile_panel_ui_model",
            "make_editor_input_rebinding_capture_action_model",
            "make_editor_input_rebinding_capture_axis_model",
            "RuntimeInputStateView",
            "in-memory profile",
            "runtime UI focus",
            "gamepad axis capture",
            "keyboard key-pair axis capture"
        )
    },
    @{
        Path = ".claude/skills/gameengine-editor/SKILL.md"
        Needles = @(
            "make_editor_input_rebinding_profile_panel_model",
            "make_input_rebinding_profile_panel_ui_model",
            "make_editor_input_rebinding_capture_action_model",
            "make_editor_input_rebinding_capture_axis_model",
            "RuntimeInputStateView",
            "in-memory profile",
            "runtime UI focus",
            "gamepad axis capture",
            "keyboard key-pair axis capture"
        )
    }
)
foreach ($check in $editorInputRebindingProfilePanelChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing editor input rebinding profile panel contract: $($missingNeedles -join ', ')"
    }
}

$editorAiPackageDiagnosticsChecks = @(
    @{
        Path = "editor/core/include/mirakana/editor/playtest_package_review.hpp"
        Needles = @(
            "EditorAiPackageAuthoringDiagnosticsModel",
            "EditorAiPackageAuthoringDiagnosticsDesc",
            "EditorAiPackageDescriptorDiagnosticRow",
            "EditorAiPackagePayloadDiagnosticRow",
            "EditorAiPackageValidationRecipeDiagnosticRow",
            "make_editor_ai_package_authoring_diagnostics_model",
            "editor_ai_package_authoring_diagnostic_status_label"
        )
    },
    @{
        Path = "editor/core/src/playtest_package_review.cpp"
        Needles = @(
            "diagnostics-only",
            "preflight-only",
            "playtest package review is blocked before runtime scene validation",
            "must not mutate",
            "must not execute"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor ai package authoring diagnostics summarize descriptors payloads and host gates without mutation",
            "editor ai package authoring diagnostics reject mutation and execution claims",
            "EditorAiPackageAuthoringDiagnosticsDesc",
            "EditorAiPackageAuthoringDiagnosticStatus"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Editor AI Package Authoring Diagnostics v1",
            "EditorAiPackageAuthoringDiagnosticsModel",
            "diagnostics-only",
            "runtime package payload diagnostics",
            "must not mutate or execute"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "Editor AI Package Authoring Diagnostics v1",
            "EditorAiPackageAuthoringDiagnosticsModel",
            "diagnostics-only",
            "mutation and execution claims"
        )
    }
)
foreach ($check in $editorAiPackageDiagnosticsChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing editor AI package diagnostics contract: $($missingNeedles -join ', ')"
    }
}

$editorAiValidationRecipePreflightChecks = @(
    @{
        Path = "editor/core/include/mirakana/editor/playtest_package_review.hpp"
        Needles = @(
            "EditorAiValidationRecipePreflightModel",
            "EditorAiValidationRecipePreflightDesc",
            "EditorAiValidationRecipeDryRunPlanRow",
            "EditorAiValidationRecipePreflightRow",
            "make_editor_ai_validation_recipe_preflight_model"
        )
    },
    @{
        Path = "editor/core/src/playtest_package_review.cpp"
        Needles = @(
            "not-declared-in-game-agent-validationRecipes",
            "missing-reviewed-dry-run-plan",
            "preflight-only",
            "raw manifest command",
            "package script execution",
            "renderer/RHI handle exposure"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor ai validation recipe preflight maps manifest recipes to dry run plans",
            "editor ai validation recipe preflight blocks execution and unsupported claims",
            "EditorAiValidationRecipePreflightDesc",
            "EditorAiValidationRecipeDryRunPlanRow"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Editor AI Validation Recipe Preflight v1",
            "EditorAiValidationRecipePreflightModel",
            "run-validation-recipe dry-run",
            "raw manifest command",
            "package script execution"
        )
    },
    @{
        Path = "docs/ai-game-development.md"
        Needles = @(
            "Editor AI Validation Recipe Preflight v1",
            "EditorAiValidationRecipePreflightModel",
            "validationRecipes",
            "run-validation-recipe dry-run",
            "host gates"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Editor AI Validation Recipe Preflight v1",
            "EditorAiValidationRecipePreflightModel",
            "raw manifest command strings",
            "package script execution"
        )
    },
    @{
        Path = "docs/roadmap.md"
        Needles = @(
            "Editor AI Validation Recipe Preflight v1"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "Editor AI Validation Recipe Preflight v1",
            "EditorAiValidationRecipePreflightModel",
            "raw shell execution",
            "renderer/RHI handle exposure"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "editor-ai-validation-recipe-preflight",
            "EditorAiValidationRecipePreflightModel",
            "raw manifest command strings",
            "package script execution"
        )
    }
)
foreach ($check in $editorAiValidationRecipePreflightChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing editor AI validation recipe preflight contract: $($missingNeedles -join ', ')"
    }
}

$editorAiPlaytestReadinessReportChecks = @(
    @{
        Path = "editor/core/include/mirakana/editor/playtest_package_review.hpp"
        Needles = @(
            "EditorAiPlaytestReadinessReportModel",
            "EditorAiPlaytestReadinessReportDesc",
            "EditorAiPlaytestReadinessReportRow",
            "make_editor_ai_playtest_readiness_report_model"
        )
    },
    @{
        Path = "editor/core/src/playtest_package_review.cpp"
        Needles = @(
            "package-authoring-diagnostics",
            "validation-recipe-preflight",
            "ready_for_operator_validation",
            "free-form manifest edits",
            "read-only and must not execute"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor ai playtest readiness report aggregates package diagnostics and validation preflight",
            "editor ai playtest readiness report blocks mutation execution and unsupported claims",
            "EditorAiPlaytestReadinessReportDesc",
            "EditorAiPlaytestReadinessReportModel"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Editor AI Playtest Readiness Report v1",
            "EditorAiPlaytestReadinessReportModel",
            "EditorAiPackageAuthoringDiagnosticsModel",
            "EditorAiValidationRecipePreflightModel",
            "read-only"
        )
    },
    @{
        Path = "docs/ai-game-development.md"
        Needles = @(
            "Editor AI Playtest Readiness Report v1",
            "EditorAiPlaytestReadinessReportModel",
            "operator validation",
            "free-form manifest edits",
            "host gates"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Editor AI Playtest Readiness Report v1",
            "EditorAiPlaytestReadinessReportModel",
            "read-only",
            "validation execution"
        )
    },
    @{
        Path = "docs/roadmap.md"
        Needles = @(
            "Editor AI Playtest Readiness Report v1"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "Editor AI Playtest Readiness Report v1",
            "EditorAiPlaytestReadinessReportModel",
            "read-only readiness",
            "free-form manifest edits"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "editor-ai-playtest-readiness-report",
            "EditorAiPlaytestReadinessReportModel",
            "free-form manifest edits",
            "validation execution"
        )
    }
)
foreach ($check in $editorAiPlaytestReadinessReportChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing editor AI playtest readiness report contract: $($missingNeedles -join ', ')"
    }
}

$editorAiPlaytestOperatorHandoffChecks = @(
    @{
        Path = "editor/core/include/mirakana/editor/playtest_package_review.hpp"
        Needles = @(
            "EditorAiPlaytestOperatorHandoffModel",
            "EditorAiPlaytestOperatorHandoffDesc",
            "EditorAiPlaytestOperatorHandoffCommandRow",
            "make_editor_ai_playtest_operator_handoff_model"
        )
    },
    @{
        Path = "editor/core/src/playtest_package_review.cpp"
        Needles = @(
            "readiness-report-blocked",
            "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
            "validation execution",
            "raw manifest command"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor ai playtest operator handoff lists reviewed commands after readiness report",
            "editor ai playtest operator handoff blocks execution mutation and unsupported claims",
            "EditorAiPlaytestOperatorHandoffDesc",
            "EditorAiPlaytestOperatorHandoffModel"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Editor AI Playtest Operator Handoff v1",
            "EditorAiPlaytestOperatorHandoffModel",
            "reviewed command display",
            "argv plan data"
        )
    },
    @{
        Path = "docs/ai-game-development.md"
        Needles = @(
            "Editor AI Playtest Operator Handoff v1",
            "EditorAiPlaytestOperatorHandoffModel",
            "external operator",
            "reviewed command display"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Editor AI Playtest Operator Handoff v1",
            "EditorAiPlaytestOperatorHandoffModel",
            "argv plan data",
            "validation execution"
        )
    },
    @{
        Path = "docs/roadmap.md"
        Needles = @(
            "Editor AI Playtest Operator Handoff v1"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "Editor AI Playtest Operator Handoff v1",
            "EditorAiPlaytestOperatorHandoffModel",
            "readiness dependency",
            "argv plan data"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "editor-ai-playtest-operator-handoff",
            "EditorAiPlaytestOperatorHandoffModel",
            "reviewed command display",
            "argv plan data"
        )
    }
)
foreach ($check in $editorAiPlaytestOperatorHandoffChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing editor AI playtest operator handoff contract: $($missingNeedles -join ', ')"
    }
}

$editorAiPlaytestEvidenceSummaryChecks = @(
    @{
        Path = "editor/core/include/mirakana/editor/playtest_package_review.hpp"
        Needles = @(
            "EditorAiPlaytestEvidenceSummaryModel",
            "EditorAiPlaytestEvidenceSummaryDesc",
            "EditorAiPlaytestValidationEvidenceRow",
            "EditorAiPlaytestEvidenceSummaryRow",
            "EditorAiPlaytestEvidenceStatus",
            "make_editor_ai_playtest_evidence_summary_model"
        )
    },
    @{
        Path = "editor/core/src/playtest_package_review.cpp"
        Needles = @(
            "missing-external-validation-evidence",
            "evidence-not-externally-supplied",
            "editor-core-validation-execution-claim",
            "raw manifest command",
            "package script execution",
            "renderer/RHI handle exposure"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor ai playtest evidence summary maps external evidence to operator handoff rows",
            "editor ai playtest evidence summary blocks editor core execution and unsupported claims",
            "EditorAiPlaytestEvidenceSummaryDesc",
            "EditorAiPlaytestEvidenceSummaryModel"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Editor AI Playtest Evidence Summary v1",
            "EditorAiPlaytestEvidenceSummaryModel",
            "externally supplied validation evidence",
            "raw/free-form command text"
        )
    },
    @{
        Path = "docs/ai-game-development.md"
        Needles = @(
            "Editor AI Playtest Evidence Summary v1",
            "EditorAiPlaytestEvidenceSummaryModel",
            "externally supplied validation evidence",
            "host gates"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Editor AI Playtest Evidence Summary v1",
            "EditorAiPlaytestEvidenceSummaryModel",
            "passed",
            "raw/free-form command evaluation"
        )
    },
    @{
        Path = "docs/roadmap.md"
        Needles = @(
            "Editor AI Playtest Evidence Summary v1"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "Editor AI Playtest Evidence Summary v1",
            "EditorAiPlaytestEvidenceSummaryModel",
            "non-external evidence",
            "raw/free-form command evaluation"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "editor-ai-playtest-evidence-summary",
            "EditorAiPlaytestEvidenceSummaryModel",
            "externally supplied run-validation-recipe execute results",
            "status passed failed blocked host-gated missing"
        )
    }
)
foreach ($check in $editorAiPlaytestEvidenceSummaryChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing editor AI playtest evidence summary contract: $($missingNeedles -join ', ')"
    }
}

$editorAiPlaytestRemediationQueueChecks = @(
    @{
        Path = "editor/core/include/mirakana/editor/playtest_package_review.hpp"
        Needles = @(
            "EditorAiPlaytestRemediationQueueDesc",
            "EditorAiPlaytestRemediationQueueModel",
            "EditorAiPlaytestRemediationCategory"
        )
    },
    @{
        Path = "editor/core/src/playtest_package_review.cpp"
        Needles = @(
            "collect_missing_evidence",
            "investigate_failure",
            "resolve_blocker",
            "raw/free-form command evaluation",
            "fix execution"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor ai playtest remediation queue classifies failed blocked missing and host gated evidence",
            "editor ai playtest remediation queue rejects execution mutation fixes and unsupported claims",
            "EditorAiPlaytestRemediationQueueDesc",
            "EditorAiPlaytestRemediationQueueModel"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Editor AI Playtest Remediation Queue v1",
            "EditorAiPlaytestRemediationQueueModel",
            "failed, blocked, missing, and host-gated evidence",
            "fix execution"
        )
    },
    @{
        Path = "docs/ai-game-development.md"
        Needles = @(
            "Editor AI Playtest Remediation Queue v1",
            "EditorAiPlaytestRemediationQueueModel",
            "next-action text",
            "evidence mutation"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Editor AI Playtest Remediation Queue v1",
            "EditorAiPlaytestRemediationQueueModel",
            "remediation category",
            "raw/free-form command evaluation"
        )
    },
    @{
        Path = "docs/roadmap.md"
        Needles = @(
            "Editor AI Playtest Remediation Queue v1"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "Editor AI Playtest Remediation Queue v1",
            "EditorAiPlaytestRemediationQueueModel",
            "fix execution",
            "evidence mutation"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "editor-ai-playtest-remediation-queue",
            "EditorAiPlaytestRemediationQueueModel",
            "failed evidence",
            "next-action text"
        )
    }
)
foreach ($check in $editorAiPlaytestRemediationQueueChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing editor AI playtest remediation queue contract: $($missingNeedles -join ', ')"
    }
}

$editorAiPlaytestRemediationHandoffChecks = @(
    @{
        Path = "editor/core/include/mirakana/editor/playtest_package_review.hpp"
        Needles = @(
            "EditorAiPlaytestRemediationHandoffDesc",
            "EditorAiPlaytestRemediationHandoffModel",
            "EditorAiPlaytestRemediationHandoffActionKind",
            "make_editor_ai_playtest_remediation_handoff_model"
        )
    },
    @{
        Path = "editor/core/src/playtest_package_review.cpp"
        Needles = @(
            "collect_external_evidence",
            "investigate_external_failure",
            "resolve_external_blocker",
            "remediation mutation",
            "outside editor core"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor ai playtest remediation handoff maps queue rows to external operator actions",
            "editor ai playtest remediation handoff rejects execution mutation remediation edits and unsupported claims",
            "EditorAiPlaytestRemediationHandoffDesc",
            "EditorAiPlaytestRemediationHandoffModel"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Editor AI Playtest Remediation Handoff v1",
            "EditorAiPlaytestRemediationHandoffModel",
            "external-owner",
            "remediation mutation"
        )
    },
    @{
        Path = "docs/ai-game-development.md"
        Needles = @(
            "Editor AI Playtest Remediation Handoff v1",
            "EditorAiPlaytestRemediationHandoffModel",
            "external operator",
            "handoff text"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Editor AI Playtest Remediation Handoff v1",
            "EditorAiPlaytestRemediationHandoffModel",
            "external-owner",
            "raw/free-form command evaluation"
        )
    },
    @{
        Path = "docs/roadmap.md"
        Needles = @(
            "Editor AI Playtest Remediation Handoff v1"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "Editor AI Playtest Remediation Handoff v1",
            "EditorAiPlaytestRemediationHandoffModel",
            "remediation mutation",
            "external-owner"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "editor-ai-playtest-remediation-handoff",
            "EditorAiPlaytestRemediationHandoffModel",
            "external-owner",
            "handoff text"
        )
    }
)
foreach ($check in $editorAiPlaytestRemediationHandoffChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing editor AI playtest remediation handoff contract: $($missingNeedles -join ', ')"
    }
}

$editorAiPlaytestOperatorWorkflowLoop = @($productionLoop.reviewLoops | Where-Object { $_.id -eq "editor-ai-playtest-operator-workflow" })
if ($editorAiPlaytestOperatorWorkflowLoop.Count -ne 1) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must expose one editor-ai-playtest-operator-workflow review loop"
}
if ($editorAiPlaytestOperatorWorkflowLoop.Count -eq 1) {
    Assert-JsonProperty $editorAiPlaytestOperatorWorkflowLoop[0] @("id", "status", "owner", "orderedSteps", "requiredManifestFields", "workflowInputs", "workflowFields", "structuredReportSurface", "closeoutPolicy", "blockedExecution", "unsupportedClaims") "engine/agent/manifest.json editor-ai-playtest-operator-workflow"
    if ($editorAiPlaytestOperatorWorkflowLoop[0].status -ne "ready") {
        Write-Error "engine/agent/manifest.json editor-ai-playtest-operator-workflow must be ready as a read-only consolidated operator workflow"
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
        Assert-JsonProperty $_ @("id", "surface", "status", "mutates", "executes") "engine/agent/manifest.json editor-ai-playtest-operator-workflow ordered step"
        if ($_.mutates -ne $false) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-operator-workflow step '$($_.id)' must not mutate"
        }
        if ($_.executes -ne $false) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-operator-workflow step '$($_.id)' must not execute"
        }
        $_.id
    })
    if (($actualEditorAiOperatorWorkflowSteps -join "|") -ne ($expectedEditorAiOperatorWorkflowSteps -join "|")) {
        Write-Error "engine/agent/manifest.json editor-ai-playtest-operator-workflow orderedSteps must be: $($expectedEditorAiOperatorWorkflowSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "prefabScenePackageAuthoringTargets", "registeredSourceAssetCookTargets", "validationRecipes")) {
        if (@($editorAiPlaytestOperatorWorkflowLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-operator-workflow requiredManifestFields missing: $field"
        }
    }
    foreach ($expectedManifestInput in @("EditorAiPackageAuthoringDiagnosticsModel", "EditorAiValidationRecipePreflightModel", "EditorAiPlaytestReadinessReportModel", "EditorAiPlaytestOperatorHandoffModel", "externally supplied validation evidence", "EditorAiPlaytestEvidenceSummaryModel", "EditorAiPlaytestRemediationQueueModel", "EditorAiPlaytestRemediationHandoffModel")) {
        if (-not ((@($editorAiPlaytestOperatorWorkflowLoop[0].workflowInputs) -join " ").Contains($expectedManifestInput))) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-operator-workflow workflowInputs missing: $expectedManifestInput"
        }
    }
    foreach ($field in @("package diagnostics", "validation preflight", "readiness dependency", "operator command rows", "evidence status", "remediation category", "external-owner", "closeout through existing evidence summary")) {
        if (-not ((@($editorAiPlaytestOperatorWorkflowLoop[0].workflowFields) -join " ").Contains($field))) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-operator-workflow workflowFields missing: $field"
        }
    }
    Assert-JsonProperty $editorAiPlaytestOperatorWorkflowLoop[0].structuredReportSurface @("model", "stageFields", "closeoutFields", "blockedExecution", "unsupportedClaims") "engine/agent/manifest.json editor-ai-playtest-operator-workflow structuredReportSurface"
    if ($editorAiPlaytestOperatorWorkflowLoop[0].structuredReportSurface.model -ne "EditorAiPlaytestOperatorWorkflowReportModel") {
        Write-Error "engine/agent/manifest.json editor-ai-playtest-operator-workflow structuredReportSurface.model must be EditorAiPlaytestOperatorWorkflowReportModel"
    }
    foreach ($field in @("operator workflow stage", "source model", "source row count", "source row ids", "status", "host gates", "blockers", "diagnostic")) {
        if (-not ((@($editorAiPlaytestOperatorWorkflowLoop[0].structuredReportSurface.stageFields) -join " ").Contains($field))) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-operator-workflow structuredReportSurface.stageFields missing: $field"
        }
    }
    foreach ($field in @("all_required_evidence_passed", "remediation_required=false", "handoff_required=false", "closeout_complete", "external_action_required")) {
        if (-not ((@($editorAiPlaytestOperatorWorkflowLoop[0].structuredReportSurface.closeoutFields) -join " ").Contains($field))) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-operator-workflow structuredReportSurface.closeoutFields missing: $field"
        }
    }
    foreach ($claim in @("arbitrary shell", "raw manifest command strings", "raw/free-form command evaluation", "validation execution", "package script execution", "free-form manifest edits", "manifest mutation", "evidence mutation", "remediation mutation", "fix execution", "play-in-editor productization", "renderer/RHI handle exposure", "Metal readiness", "general renderer quality")) {
        if (-not ((@($editorAiPlaytestOperatorWorkflowLoop[0].structuredReportSurface.blockedExecution) -join " ").Contains($claim))) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-operator-workflow structuredReportSurface.blockedExecution missing: $claim"
        }
        if (-not ((@($editorAiPlaytestOperatorWorkflowLoop[0].structuredReportSurface.unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-operator-workflow structuredReportSurface.unsupportedClaims missing: $claim"
        }
    }
    foreach ($policy in @("no separate remediation evidence-review row model", "no separate remediation closeout-report row model", "all_required_evidence_passed", "remediation_required=false", "handoff_required=false")) {
        if (-not ((@($editorAiPlaytestOperatorWorkflowLoop[0].closeoutPolicy) -join " ").Contains($policy))) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-operator-workflow closeoutPolicy missing: $policy"
        }
    }
    foreach ($claim in @("arbitrary shell", "raw manifest command strings", "raw/free-form command evaluation", "validation execution", "package script execution", "free-form manifest edits", "evidence mutation", "remediation mutation", "fix execution", "play-in-editor productization", "renderer/RHI handle exposure", "Metal readiness", "general renderer quality")) {
        if (-not ((@($editorAiPlaytestOperatorWorkflowLoop[0].blockedExecution) -join " ").Contains($claim))) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-operator-workflow blockedExecution missing: $claim"
        }
        if (-not ((@($editorAiPlaytestOperatorWorkflowLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine/agent/manifest.json editor-ai-playtest-operator-workflow unsupportedClaims missing: $claim"
        }
    }
}

$editorAiPlaytestOperatorWorkflowChecks = @(
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Editor AI Playtest Operator Workflow v1",
            "EditorAiPlaytestOperatorWorkflowReportModel",
            "closeout through existing evidence summary",
            "no separate remediation evidence-review row model",
            "no separate remediation closeout-report row model"
        )
    },
    @{
        Path = "docs/ai-game-development.md"
        Needles = @(
            "Editor AI Playtest Operator Workflow v1",
            "EditorAiPlaytestOperatorWorkflowReportModel",
            "package diagnostics -> validation recipe preflight -> readiness -> operator handoff -> evidence summary -> remediation queue -> remediation handoff",
            'rerun `EditorAiPlaytestEvidenceSummaryModel`',
            "no separate remediation closeout-report row model"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Editor AI Playtest Operator Workflow v1",
            "EditorAiPlaytestOperatorWorkflowReportModel",
            "closeout through existing evidence summary",
            "no separate remediation evidence-review row model",
            "no separate remediation closeout-report row model"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "EditorAiPlaytestOperatorWorkflowReportModel",
            "structured report surface",
            "source row count"
        )
    },
    @{
        Path = "docs/roadmap.md"
        Needles = @(
            "Editor AI Playtest Operator Workflow v1",
            "EditorAiPlaytestOperatorWorkflowReportModel",
            "remediation evidence-review and closeout-report candidates were retired"
        )
    },
    @{
        Path = "editor/core/include/mirakana/editor/playtest_package_review.hpp"
        Needles = @(
            "EditorAiPlaytestOperatorWorkflowReportModel",
            "EditorAiPlaytestOperatorWorkflowReportDesc",
            "EditorAiPlaytestOperatorWorkflowStageStatus",
            "make_editor_ai_playtest_operator_workflow_report_model"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor ai playtest operator workflow report surfaces existing model rows",
            "editor ai playtest operator workflow report closes through evidence summary rerun",
            "editor ai playtest operator workflow report rejects execution mutation and unsupported claims"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "Phase 2 decision: retire the separate remediation evidence-review and closeout-report candidates",
            "Phase 3 consolidation: package diagnostics -> validation recipe preflight -> readiness -> operator handoff -> evidence summary -> remediation queue -> remediation handoff",
            "closeout through existing evidence summary"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "Status note (2026-05-02): Retired by",
            "no separate remediation evidence-review row model"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "Status note (2026-05-02): Retired by",
            "no separate remediation closeout-report row model"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "editor-ai-playtest-operator-workflow",
            "EditorAiPlaytestOperatorWorkflowReportModel",
            "closeout through existing evidence summary",
            "no separate remediation evidence-review row model",
            "no separate remediation closeout-report row model"
        )
    }
)
foreach ($check in $editorAiPlaytestOperatorWorkflowChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing consolidated editor AI playtest operator workflow contract: $($missingNeedles -join ', ')"
    }
}

$prefabVariantAuthoringChecks = @(
    @{
        Path = "engine/scene/include/mirakana/scene/scene.hpp"
        Needles = @(
            "ScenePrefabSourceLink",
            "prefab_source",
            "is_valid_scene_prefab_source_link"
        )
    },
    @{
        Path = "engine/scene/include/mirakana/scene/prefab.hpp"
        Needles = @(
            "PrefabInstantiateDesc",
            "source_path",
            "instantiate_prefab"
        )
    },
    @{
        Path = "engine/scene/include/mirakana/scene/prefab_overrides.hpp"
        Needles = @(
            "PrefabVariantDefinition",
            "PrefabNodeOverride",
            "source_node_name",
            "invalid_override_kind",
            "invalid_source_node_name",
            "invalid_transform",
            "compose_prefab_variant",
            "serialize_prefab_variant_definition",
            "deserialize_prefab_variant_definition_for_review"
        )
    },
    @{
        Path = "engine/scene/src/prefab_overrides.cpp"
        Needles = @(
            "source_node_name",
            "invalid_source_node_name",
            "serialize_prefab_variant_definition",
            "deserialize_prefab_variant_definition_for_review"
        )
    },
    @{
        Path = "editor/core/include/mirakana/editor/prefab_variant_authoring.hpp"
        Needles = @(
            "PrefabVariantAuthoringDocument",
            "PrefabVariantOverrideRow",
            "PrefabVariantConflictReviewModel",
            "PrefabVariantConflictRow",
            "source_node_mismatch",
            "PrefabVariantConflictResolutionKind",
            "prefab_variant_conflict_resolution_kind_label",
            "retarget_override",
            "accept_current_node",
            "resolution_target_node_index",
            "PrefabVariantConflictResolutionResult",
            "PrefabVariantBaseRefreshPlan",
            "PrefabVariantBaseRefreshStatus",
            "PrefabVariantBaseRefreshRow",
            "PrefabVariantBaseRefreshResult",
            "EditorPrefabVariantFileDialogModel",
            "validate_prefab_variant_authoring_document",
            "make_prefab_variant_conflict_review_model",
            "make_prefab_variant_conflict_review_ui_model",
            "resolve_prefab_variant_conflict",
            "make_prefab_variant_conflict_resolution_action",
            "plan_prefab_variant_base_refresh",
            "apply_prefab_variant_base_refresh",
            "make_prefab_variant_base_refresh_action",
            "make_prefab_variant_base_refresh_ui_model",
            "make_prefab_variant_open_dialog_request",
            "make_prefab_variant_save_dialog_request",
            "make_prefab_variant_open_dialog_model",
            "make_prefab_variant_save_dialog_model",
            "make_prefab_variant_file_dialog_ui_model",
            "make_prefab_variant_name_override_action",
            "save_prefab_variant_authoring_document"
        )
    },
    @{
        Path = "editor/core/include/mirakana/editor/scene_authoring.hpp"
        Needles = @(
            "ScenePrefabInstanceSourceLinkStatus",
            "ScenePrefabInstanceSourceLinkRow",
            "ScenePrefabInstanceSourceLinkModel",
            "make_scene_prefab_instance_source_link_model",
            "make_scene_prefab_instance_source_link_ui_model",
            "ScenePrefabInstanceRefreshStatus",
            "ScenePrefabInstanceRefreshRowKind",
            "ScenePrefabInstanceRefreshPolicy",
            "keep_stale_source_nodes_as_local",
            "keep_nested_prefab_instances",
            "ScenePrefabInstanceRefreshRow",
            "ScenePrefabInstanceRefreshPlan",
            "SceneNestedPrefabPropagationPreviewRow",
            "nested_prefab_propagation_preview",
            "ScenePrefabInstanceRefreshResult",
            "ScenePrefabInstanceRefreshBatchPlan",
            "ScenePrefabInstanceRefreshBatchTargetInput",
            "ScenePrefabInstanceRefreshBatchResult",
            "kept_stale_source_node_count",
            "kept_nested_prefab_instance_count",
            "plan_scene_prefab_instance_refresh",
            "plan_scene_prefab_instance_refresh_batch",
            "apply_scene_prefab_instance_refresh",
            "apply_scene_prefab_instance_refresh_batch",
            "make_scene_prefab_instance_refresh_action",
            "make_scene_prefab_instance_refresh_batch_action",
            "make_scene_prefab_instance_refresh_ui_model",
            "make_scene_prefab_instance_refresh_batch_ui_model"
        )
    },
    @{
        Path = "editor/core/src/scene_authoring.cpp"
        Needles = @(
            "scene_prefab_instance_refresh",
            "scene_prefab_instance_refresh_batch",
            "scene_prefab_instance_refresh_row_kind_label",
            "unsupported_local_child",
            "keep_local_child",
            "keep_stale_source_node_as_local",
            "keep_nested_prefab_instance",
            "unsupported_nested_prefab_instance",
            "nested_prefab_variant_alignment",
            "ge.editor.scene_prefab_nested_variant_alignment.v1",
            "local_child_variant_alignment",
            "ge.editor.scene_prefab_local_child_variant_alignment.v1",
            "stale_source_variant_alignment",
            "ge.editor.scene_prefab_stale_source_variant_alignment.v1",
            "source_node_variant_alignment",
            "ge.editor.scene_prefab_source_node_variant_alignment.v1",
            "propagation_preview",
            "ge.editor.scene_nested_prefab_propagation_preview.v1",
            "preview_only_no_chained_refresh",
            "reviewed_chained_prefab_refresh_after_root",
            "apply_reviewed_nested_prefab_propagation",
            "prefab_variant_conflict_resolution_kind_label",
            "unsupported_stale_source_subtree",
            "ambiguous_source_node",
            "preserve_node",
            "add_source_node",
            "remove_stale_node",
            "apply_scene_prefab_instance_refresh",
            "apply_scene_prefab_instance_refresh_batch",
            "make_scene_prefab_instance_refresh_batch_action"
        )
    },
    @{
        Path = "editor/core/src/prefab_variant_authoring.cpp"
        Needles = @(
            "Retarget override to node",
            "retarget_resolution_id_for",
            "accept_current_resolution_id_for",
            "Accept current node",
            "accept_current_node",
            "prefab_variant_conflict_resolution_kind_label",
            "resolution_kind",
            "resolution_target_node",
            "source_node_name_mismatch",
            "source_node_mismatch",
            "source_node_name",
            "prefab_variant_base_refresh",
            "missing_source_node_hint",
            "duplicate_target_override",
            "Refresh Prefab Variant Base",
            "plan_prefab_variant_base_refresh",
            "apply_prefab_variant_base_refresh",
            "make_prefab_variant_dialog_request",
            "prefab_variant_file_dialog.",
            "prefab_variant_file_dialog_status",
            "accepted"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor scene prefab source link model reviews instantiated prefab nodes",
            "editor scene prefab source link model reports stale link diagnostics",
            "scene_prefab_source_links",
            "editor prefab variant authoring saves loads rows and instantiates composed prefab",
            "editor prefab variant authoring reports registry backed component override diagnostics",
            "editor prefab variant authoring undo actions edit name transform and components",
            "editor prefab variant conflict review reports blocking and warning rows",
            "editor prefab variant reviewed resolution removes redundant overrides with undo",
            "editor prefab variant reviewed resolution exposes duplicate and missing cleanup while blocked",
            "editor prefab variant missing node cleanup resolves stale raw variants",
            "editor prefab variant authoring loads stale variants for reviewed missing node cleanup",
            "editor prefab variant reviewed retarget resolves unique source node hints",
            "editor prefab variant reviewed retarget falls back when source node hints are ambiguous",
            "editor prefab variant source mismatch retargets existing stale node hints",
            "editor prefab variant source mismatch accepts current indexed node hints",
            "editor prefab variant batch resolution applies reviewed rows with one undo",
            "editor prefab variant base refresh retargets overrides by source node names",
            "editor prefab variant base refresh blocks unsafe source mappings",
            "editor prefab variant base refresh blocks duplicate target override keys",
            "editor prefab variant native file dialogs review results and retained rows",
            "editor scene prefab instance refresh review preserves linked nodes and applies with undo",
            "scene_prefab_instance_refresh_batch.source_node_variant_alignment",
            "ge.editor.scene_prefab_source_node_variant_alignment.v1",
            "source_node_variant_alignment.resolution_kind",
            "editor scene prefab instance refresh review blocks unsafe mappings",
            "editor scene prefab instance refresh review blocks multi root source prefabs",
            "editor scene prefab instance refresh review can keep local child subtrees",
            "editor scene prefab instance refresh review can keep nested prefab instances",
            "editor scene prefab instance refresh review can apply nested prefab propagation chain",
            "editor scene prefab instance refresh batch keeps recreated nested source node selected",
            "editor scene prefab instance refresh batch keeps local child during nested prefab propagation",
            "editor scene prefab instance refresh batch stays atomic when nested loader drifts during apply",
            "editor scene prefab instance refresh batch stays atomic when later nested loader drifts during apply",
            "ge.editor.scene_nested_prefab_propagation_preview.v1",
            "propagation_preview.contract",
            "preview_only_no_chained_refresh",
            "reviewed_chained_prefab_refresh_after_root",
            "apply_reviewed_nested_prefab_propagation",
            "scene_prefab_instance_refresh",
            "plan_scene_prefab_instance_refresh",
            "make_scene_prefab_instance_refresh_action",
            "unsupported_local_child",
            "keep_local_child",
            "unsupported_nested_prefab_instance",
            "keep_nested_prefab_instance",
            "ambiguous_source_node",
            "source_node_mismatch",
            "PrefabVariantBaseRefreshStatus",
            "prefab_variant_base_refresh.rows.override.1.node.2.transform.status",
            "missing_source_node_hint",
            "duplicate_target_override",
            "accept_current.node.1.transform",
            "accept_current_node",
            "Accept current node 1",
            "Retarget override to node 2",
            "retarget.node.1.transform.to.2",
            "Apply All Reviewed Resolutions",
            "prefab_variant_conflicts.rows.node.1.name.resolution",
            "prefab_variant_conflicts.rows.node.2.name.resolution",
            "prefab_variant_conflicts.rows.node.1.transform.resolution_kind",
            "prefab_variant_conflicts.rows.node.1.transform.resolution_target_node",
            "prefab_variant_conflicts.rows.node.3.transform.resolution_kind",
            "prefab_variant_conflicts.rows.node.3.transform.resolution_target_node",
            "prefab_variant_conflicts.batch_resolution.apply_all",
            "prefab_variant_file_dialog.open.status.value",
            "prefab_variant_file_dialog.save.status.value",
            "make_prefab_variant_open_dialog_request",
            "make_prefab_variant_save_dialog_model",
            "remove.node.1.name.duplicate.2",
            "prefab_variant_conflicts.rows.node.1.name.status"
        )
    },
    @{
        Path = "tests/unit/core_tests.cpp"
        Needles = @(
            "prefab instantiation records durable source links",
            "scene serializes restores and validates prefab source links",
            "ScenePrefabSourceLink",
            "prefab variant validation rejects unsupported kinds and malformed transforms",
            "prefab variant deserialization rejects fields outside the declared override kind",
            "prefab variant preserves source node name hints"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Editor Scene Prefab Instance Refresh Review v1",
            "Editor Prefab Instance Local Child Refresh Resolution v1",
            "Editor Prefab Instance Stale Node Refresh Resolution v1",
            "Editor Nested Prefab Refresh Resolution v1",
            "ScenePrefabInstanceRefreshPlan",
            "ScenePrefabInstanceRefreshPolicy",
            "make_scene_prefab_instance_refresh_action",
            "scene_prefab_instance_refresh",
            "Refresh Prefab Instance",
            "Keep Local Children",
            "Keep Stale Source Nodes",
            "Keep Nested Prefab Instances",
            "unsupported local children",
            "keep_local_child",
            "keep_stale_source_node_as_local",
            "keep_nested_prefab_instance",
            "unsupported_nested_prefab_instance",
            "automatic nested prefab refresh"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Editor Scene Prefab Instance Refresh Review v1",
            "Editor Prefab Instance Local Child Refresh Resolution v1",
            "Editor Prefab Instance Stale Node Refresh Resolution v1",
            "Editor Nested Prefab Refresh Resolution v1",
            "ScenePrefabInstanceRefreshRow",
            "ScenePrefabInstanceRefreshPolicy",
            "scene_prefab_instance_refresh",
            "keep_local_child",
            "keep_stale_source_node_as_local",
            "keep_nested_prefab_instance",
            "explicit scene prefab instance refresh rows"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "Editor Scene Prefab Instance Refresh Review v1 coverage",
            "Editor Prefab Instance Local Child Refresh Resolution v1 coverage",
            "Editor Prefab Instance Stale Node Refresh Resolution v1 coverage",
            "Editor Nested Prefab Refresh Resolution v1 coverage",
            "ScenePrefabInstanceRefreshPlan",
            "ScenePrefabInstanceRefreshPolicy",
            "apply_scene_prefab_instance_refresh",
            "unsupported local-child mappings",
            "keep_local_child",
            "keep_stale_source_node_as_local",
            "keep_nested_prefab_instance"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
        Needles = @(
            "2026-05-09-editor-scene-prefab-instance-refresh-review-v1.md",
            "2026-05-09-editor-prefab-instance-local-child-refresh-resolution-v1.md",
            "2026-05-09-editor-prefab-instance-stale-node-refresh-resolution-v1.md",
            "2026-05-09-editor-nested-prefab-refresh-resolution-v1.md",
            "Editor Scene Prefab Instance Refresh Review v1",
            "Editor Prefab Instance Local Child Refresh Resolution v1",
            "Editor Prefab Instance Stale Node Refresh Resolution v1",
            "Editor Nested Prefab Refresh Resolution v1",
            "scene_prefab_instance_refresh",
            "keep_local_child",
            "keep_stale_source_node_as_local",
            "keep_nested_prefab_instance",
            "selected linked prefab root"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "2026-05-09-editor-scene-prefab-instance-refresh-review-v1.md",
            "2026-05-09-editor-prefab-instance-local-child-refresh-resolution-v1.md",
            "2026-05-09-editor-prefab-instance-stale-node-refresh-resolution-v1.md",
            "2026-05-09-editor-nested-prefab-refresh-resolution-v1.md",
            "Active slice",
            "scene_prefab_instance_refresh",
            "Refresh Prefab Instance",
            "keep_local_child",
            "keep_stale_source_node_as_local",
            "keep_nested_prefab_instance"
        )
    },
    @{
        Path = ".agents/skills/editor-change/SKILL.md"
        Needles = @(
            "ScenePrefabInstanceRefreshPlan",
            "ScenePrefabInstanceRefreshPolicy",
            "make_scene_prefab_instance_refresh_action",
            "scene_prefab_instance_refresh",
            "Refresh Prefab Instance",
            "Keep Local Children",
            "Keep Stale Source Nodes",
            "Keep Nested Prefab Instances",
            "keep_stale_source_node_as_local",
            "keep_nested_prefab_instance"
        )
    },
    @{
        Path = ".claude/skills/gameengine-editor/SKILL.md"
        Needles = @(
            "ScenePrefabInstanceRefreshPlan",
            "ScenePrefabInstanceRefreshPolicy",
            "make_scene_prefab_instance_refresh_action",
            "scene_prefab_instance_refresh",
            "Refresh Prefab Instance",
            "Keep Local Children",
            "Keep Stale Source Nodes",
            "Keep Nested Prefab Instances",
            "keep_stale_source_node_as_local",
            "keep_nested_prefab_instance"
        )
    }
)
foreach ($check in $prefabVariantAuthoringChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing prefab variant authoring contract: $($missingNeedles -join ', ')"
    }
}

$visiblePrefabVariantGuiChecks = @(
    @{
        Path = "editor/CMakeLists.txt"
        Needles = @(
            "Native MK_editor shell is SDL3-free",
            "MK_editor_shell_common",
            "add_executable(MK_editor"
        )
    }
)
foreach ($check in $visiblePrefabVariantGuiChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing visible prefab variant GUI contract: $($missingNeedles -join ', ')"
    }
    if ($check.Path -ne "editor/CMakeLists.txt") {
        Assert-MatchesText $fileText "project_paths_\s*=\s*mirakana::editor::ProjectBundlePaths\s*\{[^}]*\}\s*;\s*reset_prefab_variant_document\s*\(\s*\)\s*;\s*(?:reset_ai_evidence_import_state\s*\(\s*\)\s*;\s*)?(?:reset_resource_capture_request_state\s*\(\s*\)\s*;\s*)?replace_scene_document" "visible prefab variant GUI create-project reset contract"
        Assert-MatchesText $fileText "workspace_\s*=\s*bundle\.workspace\s*;\s*reset_prefab_variant_document\s*\(\s*\)\s*;\s*(?:reset_ai_evidence_import_state\s*\(\s*\)\s*;\s*)?(?:reset_resource_capture_request_state\s*\(\s*\)\s*;\s*)?reset_project_settings_inputs_from_project" "visible prefab variant GUI open-project reset contract"
        Assert-MatchesText $fileText "if\s*\(\s*!\s*prefab_variant_document_->model\s*\(\s*assets_\s*\)\.valid\s*\(\s*\)\s*\)\s*\{[^}]*Prefab variant has unresolved diagnostics[^}]*return\s*;\s*\}\s*auto\s+composed\s*=\s*prefab_variant_document_->composed_prefab\s*\(\s*\)" "visible prefab variant GUI registry-diagnostic instantiate gate"
    }
}

$editorSceneNativeDialogChecks = @(
    @{
        Path = "editor/core/include/mirakana/editor/scene_authoring.hpp"
        Needles = @(
            "EditorSceneFileDialogMode",
            "EditorSceneFileDialogModel",
            "EditorSceneFileDialogRow",
            "set_scene_path",
            "make_scene_open_dialog_request",
            "make_scene_save_dialog_request",
            "make_scene_open_dialog_model",
            "make_scene_save_dialog_model",
            "make_scene_file_dialog_ui_model"
        )
    },
    @{
        Path = "editor/core/src/scene_authoring.cpp"
        Needles = @(
            "scene_file_dialog_status",
            "scene_file_dialog_action",
            "scene_file_dialog_mode_id",
            "requires exactly one selected path",
            "selection must end with .scene",
            "scene file dialog ui element could not be added",
            "scene_file_dialog.",
            '.name = "Scene", .pattern = "scene"',
            "make_scene_open_dialog_request",
            "make_scene_save_dialog_request",
            "make_scene_file_dialog_ui_model"
        )
    },
    @{
        Path = "editor/CMakeLists.txt"
        Needles = @(
            "Native MK_editor shell is SDL3-free",
            "MK_editor_shell_common",
            "add_executable(MK_editor"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor scene native file dialogs review results and retained rows",
            "editor scene authoring document updates scene path for save as",
            "scene_file_dialog.open.status.value",
            "scene_file_dialog.save.status.value",
            "make_scene_open_dialog_request",
            "make_scene_save_dialog_model",
            "set_scene_path",
            "at least one path",
            "exactly one",
            ".scene"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Editor Scene Native Dialog v1",
            "EditorSceneFileDialogModel",
            "make_scene_open_dialog_request",
            "make_scene_save_dialog_request",
            "make_scene_open_dialog_model",
            "make_scene_save_dialog_model",
            "make_scene_file_dialog_ui_model",
            "scene_file_dialog.open",
            "scene_file_dialog.save",
            "Open Scene...",
            "Save Scene As...",
            "Browse Open Scene",
            "Browse Save Scene As",
            "SceneAuthoringDocument::set_scene_path"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Editor Scene Native Dialog v1",
            "EditorSceneFileDialogModel",
            "make_scene_open_dialog_request",
            "make_scene_save_dialog_request",
            "make_scene_open_dialog_model",
            "make_scene_save_dialog_model",
            "make_scene_file_dialog_ui_model",
            "scene_file_dialog.open",
            "scene_file_dialog.save",
            "Open Scene...",
            "Save Scene As...",
            "Browse Open Scene",
            "Browse Save Scene As",
            "SceneAuthoringDocument::set_scene_path"
        )
    },
    @{
        Path = "docs/roadmap.md"
        Needles = @(
            "Editor Scene Native Dialog v1",
            "EditorSceneFileDialogModel",
            "make_scene_open_dialog_request",
            "make_scene_save_dialog_request",
            "make_scene_open_dialog_model",
            "make_scene_save_dialog_model",
            "make_scene_file_dialog_ui_model",
            "scene_file_dialog.open",
            "scene_file_dialog.save",
            "Open Scene...",
            "Save Scene As...",
            "Browse Open Scene",
            "Browse Save Scene As",
            "SceneAuthoringDocument::set_scene_path",
            "outside Profiler, Scene, and Prefab Variant"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "2026-05-07-editor-scene-native-dialog-v1.md",
            "EditorSceneFileDialogModel",
            "scene_file_dialog.open",
            "scene_file_dialog.save",
            "Open Scene...",
            "Save Scene As..."
        )
    },
    @{
        Path = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
        Needles = @(
            "Editor Scene Native Dialog v1",
            "EditorSceneFileDialogModel",
            "make_scene_open_dialog_request",
            "make_scene_save_dialog_request",
            "make_scene_open_dialog_model",
            "make_scene_save_dialog_model",
            "make_scene_file_dialog_ui_model",
            "scene_file_dialog.open",
            "scene_file_dialog.save",
            "Open Scene...",
            "Save Scene As...",
            "Browse Open Scene",
            "Browse Save Scene As",
            "SceneAuthoringDocument::set_scene_path",
            "outside Profiler, Scene, and Prefab Variant"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
        Needles = @(
            "Editor Scene Native Dialog v1 Implementation Plan",
            "EditorSceneFileDialogModel",
            "make_scene_open_dialog_request",
            "make_scene_save_dialog_request",
            "make_scene_open_dialog_model",
            "make_scene_save_dialog_model",
            "make_scene_file_dialog_ui_model",
            "SdlFileDialogService",
            "broader editor native save/open"
        )
    },
    @{
        Path = ".agents/skills/editor-change/SKILL.md"
        Needles = @(
            "make_scene_open_dialog_request",
            "make_scene_save_dialog_request",
            "make_scene_file_dialog_ui_model",
            "SceneAuthoringDocument::set_scene_path",
            "scene_file_dialog.open",
            "scene_file_dialog.save",
            "Browse Open Scene",
            "Browse Save Scene As"
        )
    },
    @{
        Path = ".claude/skills/gameengine-editor/SKILL.md"
        Needles = @(
            "make_scene_open_dialog_request",
            "make_scene_save_dialog_request",
            "make_scene_file_dialog_ui_model",
            "SceneAuthoringDocument::set_scene_path",
            "scene_file_dialog.open",
            "scene_file_dialog.save",
            "Browse Open Scene",
            "Browse Save Scene As"
        )
    }
)
foreach ($check in $editorSceneNativeDialogChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing editor scene native dialog contract: $($missingNeedles -join ', ')"
    }
}

$nativeEditorServiceChecks = @(
    @{
        Path = "editor/src/native_editor_app.hpp"
        Needles = @(
            "NativeEditorServiceStatus",
            "NativeEditorServiceBindings",
            "NativeEditorReviewedProcessRequest",
            "NativeEditorReviewedProcessResult",
            "file_dialog_service_id",
            "clipboard_service_id",
            "reviewed_process_runner_id",
            "user_confirmation_required_for_process_execution",
            "bind_native_services",
            "show_file_dialog",
            "poll_file_dialog_result",
            "write_clipboard_text",
            "read_clipboard_text",
            "reviewed_validation_execution_plan",
            "run_reviewed_process"
        )
    },
    @{
        Path = "editor/src/native_editor_app.cpp"
        Needles = @(
            "MemoryFileDialogService",
            "MemoryClipboard",
            "NativeEditorClipboardTextAdapter",
            "RecordingProcessRunner",
            "make_editor_ai_reviewed_validation_execution_plan",
            "is_allowed_process_command",
            "run_process_command",
            "reviewed process execution requires user confirmation before launch"
        )
    },
    @{
        Path = "editor/src/native_editor_win32_services.hpp"
        Needles = @(
            "NativeEditorWin32Services",
            "Win32FileDialogService",
            "Win32Clipboard",
            "Win32ClipboardTextAdapter",
            "Win32ProcessRunner"
        )
    },
    @{
        Path = "editor/src/native_editor_win32_services.cpp"
        Needles = @(
            "file_dialogs_(owner_window_token)",
            "clipboard_adapter_(clipboard_)",
            '.file_dialog_service_id = "win32"',
            '.clipboard_service_id = "win32"',
            '.reviewed_process_runner_id = "win32"'
        )
    },
    @{
        Path = "editor/src/win32_imgui_d3d12_host.cpp"
        Needles = @(
            "native_editor_win32_services.hpp",
            "NativeEditorWin32Services",
            "window->native_window_token()",
            "services->bind(app)"
        )
    },
    @{
        Path = "editor/src/native_editor_panels.cpp"
        Needles = @(
            "Reviewed process runner",
            "Native services",
            "file dialog requests",
            "clipboard operations",
            "reviewed plans",
            "executions"
        )
    },
    @{
        Path = "editor/src/main.cpp"
        Needles = @(
            "editor_shell_file_dialog_service=",
            "editor_shell_clipboard_service=",
            "editor_shell_reviewed_process_runner="
        )
    },
    @{
        Path = "CMakeLists.txt"
        Needles = @(
            "editor_shell_file_dialog_service=win32",
            "editor_shell_clipboard_service=win32",
            "editor_shell_reviewed_process_runner=win32"
        )
    },
    @{
        Path = "tests/unit/editor_native_shell_tests.cpp"
        Needles = @(
            "editor native shell routes file dialog requests through bound service",
            "editor native shell routes clipboard text through bound adapter",
            "editor native shell reviewed process execution requires confirmation",
            "editor native shell service status defaults stay deterministic"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "editor_shell_file_dialog_service=win32",
            "editor_shell_clipboard_service=win32",
            "editor_shell_reviewed_process_runner=win32",
            "native shell file-dialog service"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Win32 file-dialog, clipboard, and reviewed process-runner service adapters",
            "editor_shell_file_dialog_service=win32",
            "editor_shell_clipboard_service=win32",
            "editor_shell_reviewed_process_runner=win32"
        )
    },
    @{
        Path = "docs/roadmap.md"
        Needles = @(
            "Win32 file-dialog, clipboard, reviewed process-runner service adapters",
            "editor_shell_file_dialog_service=win32",
            "editor_shell_clipboard_service=win32",
            "editor_shell_reviewed_process_runner=win32"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "Win32 file-dialog, clipboard, reviewed process-runner service adapters",
            "editor_shell_file_dialog_service=win32",
            "editor_shell_clipboard_service=win32",
            "editor_shell_reviewed_process_runner=win32"
        )
    }
)
foreach ($check in $nativeEditorServiceChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing native editor service contract: $($missingNeedles -join ', ')"
    }
}

$nativeEditorViewportChecks = @(
    @{
        Path = "editor/src/native_viewport_surface.hpp"
        Needles = @(
            "NativeViewportDisplayDesc",
            "NativeViewportDisplayPlan",
            "d3d12_host_available",
            "renderer_output_available",
            "texture_display_ready",
            "native_texture_handles_exposed",
            "native_texture_handle_policy",
            "plan_native_viewport_display"
        )
    },
    @{
        Path = "editor/src/native_viewport_surface.cpp"
        Needles = @(
            "host_unavailable",
            "invalid_extent",
            "diagnostic_only",
            "renderer output unavailable",
            "native D3D12 texture display adapter is private"
        )
    },
    @{
        Path = "editor/src/native_editor_app.hpp"
        Needles = @(
            "native_viewport_surface.hpp",
            "viewport()",
            "viewport_display()",
            "record_native_viewport_d3d12_host_ready"
        )
    },
    @{
        Path = "editor/src/native_editor_app.cpp"
        Needles = @(
            'NativePanelToken{.id = "viewport"}',
            "plan_native_viewport_display",
            "ViewportState viewport",
            "NativeViewportDisplayPlan viewport_display",
            "record_native_viewport_d3d12_host_ready",
            'set_renderer("d3d12")'
        )
    },
    @{
        Path = "editor/src/native_editor_panels.cpp"
        Needles = @(
            "render_native_editor_viewport_panel",
            "PanelId::viewport",
            "native_viewport_surface",
            "diagnostic only",
            "native handles"
        )
    },
    @{
        Path = "editor/src/win32_imgui_d3d12_host.cpp"
        Needles = @(
            "record_native_viewport_d3d12_host_ready"
        )
    },
    @{
        Path = "editor/src/main.cpp"
        Needles = @(
            "editor_shell_viewport_status=",
            "editor_shell_viewport_native_handles_exposed="
        )
    },
    @{
        Path = "CMakeLists.txt"
        Needles = @(
            "editor_shell_panels=11",
            "editor_shell_viewport_status=diagnostic_only",
            "editor_shell_viewport_native_handles_exposed=0"
        )
    },
    @{
        Path = "editor/CMakeLists.txt"
        Needles = @(
            "src/native_viewport_surface.cpp"
        )
    },
    @{
        Path = "tests/unit/editor_native_shell_tests.cpp"
        Needles = @(
            "editor native viewport display plan rejects missing d3d12 host",
            "editor native viewport display plan records diagnostic-only viewport when renderer output is unavailable",
            "editor native viewport display plan does not expose native texture handles",
            "native_texture_handle_policy"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Viewport",
            "editor_shell_panels=11",
            "editor_shell_viewport_status=diagnostic_only",
            "editor_shell_viewport_native_handles_exposed=0",
            "full D3D12 texture display"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "diagnostic-only native Viewport panel",
            "editor_shell_panels=11",
            "editor_shell_viewport_status=diagnostic_only",
            "editor_shell_viewport_native_handles_exposed=0",
            "Full Viewport D3D12 texture display"
        )
    },
    @{
        Path = "docs/roadmap.md"
        Needles = @(
            "diagnostic-only viewport display",
            "editor_shell_panels=11",
            "editor_shell_viewport_status=diagnostic_only",
            "editor_shell_viewport_native_handles_exposed=0",
            "full viewport texture display"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "diagnostic-only viewport display",
            "editor_shell_panels=11",
            "editor_shell_viewport_status=diagnostic_only",
            "editor_shell_viewport_native_handles_exposed=0",
            "private D3D12 texture adapter"
        )
    }
)
foreach ($check in $nativeEditorViewportChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing native editor viewport contract: $($missingNeedles -join ', ')"
    }
}

$nativeEditorMaterialPreviewChecks = @(
    @{
        Path = "editor/src/native_material_preview_cache.hpp"
        Needles = @(
            "NativeMaterialPreviewDisplayDesc",
            "NativeMaterialPreviewDisplayPlan",
            "shader_artifacts_available",
            "gpu_payload_available",
            "texture_display_ready",
            "native_texture_handles_exposed",
            "native_texture_handle_policy",
            "EditorMaterialGpuPreviewExecutionSnapshot",
            "plan_native_material_preview_display"
        )
    },
    @{
        Path = "editor/src/native_material_preview_cache.cpp"
        Needles = @(
            "shader_artifacts_missing",
            "diagnostic_only",
            "host-private-native",
            "GPU payload unavailable",
            "native material preview texture display adapter is private",
            "exposes_native_handles = false"
        )
    },
    @{
        Path = "editor/core/include/mirakana/editor/material_asset_preview_panel.hpp"
        Needles = @(
            "EditorMaterialGpuPreviewExecutionSnapshot",
            "bool executes",
            "bool exposes_native_handles"
        )
    },
    @{
        Path = "editor/core/src/material_asset_preview_panel.cpp"
        Needles = @(
            "host-private-native",
            "material GPU preview execution snapshot must not claim editor-core execution or native handle exposure",
            "material_asset_preview.gpu.execution.native_handles"
        )
    },
    @{
        Path = "editor/src/native_editor_app.hpp"
        Needles = @(
            "native_material_preview_cache.hpp",
            "material_preview()",
            "material_preview_display()",
            "record_native_material_preview_d3d12_host_ready"
        )
    },
    @{
        Path = "editor/src/native_editor_app.cpp"
        Needles = @(
            "make_default_material_preview_panel_model",
            "make_material_preview_shader_compile_requests",
            "EditorMaterialAssetPreviewPanelModel material_preview",
            "NativeMaterialPreviewDisplayPlan material_preview_display",
            "record_native_material_preview_d3d12_host_ready",
            "apply_editor_material_gpu_preview_execution_snapshot"
        )
    },
    @{
        Path = "editor/src/native_editor_panels.cpp"
        Needles = @(
            "render_material_preview_summary",
            "native_material_preview_surface",
            "diagnostic only",
            "native handles"
        )
    },
    @{
        Path = "editor/src/win32_imgui_d3d12_host.cpp"
        Needles = @(
            "record_native_material_preview_d3d12_host_ready"
        )
    },
    @{
        Path = "editor/src/main.cpp"
        Needles = @(
            "editor_shell_material_preview_status=",
            "editor_shell_material_preview_native_handles_exposed="
        )
    },
    @{
        Path = "CMakeLists.txt"
        Needles = @(
            "editor_shell_material_preview_status=diagnostic_only",
            "editor_shell_material_preview_native_handles_exposed=0"
        )
    },
    @{
        Path = "editor/CMakeLists.txt"
        Needles = @(
            "src/native_material_preview_cache.cpp",
            'target_include_directories(MK_editor_shell_common'
        )
    },
    @{
        Path = "tests/unit/editor_native_shell_tests.cpp"
        Needles = @(
            "editor native material preview plan rejects missing shader artifacts",
            "editor native material preview plan reports diagnostic-only preview without gpu display",
            "editor native material preview plan keeps d3d12 handles private"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor material asset preview panel rejects unsafe gpu execution snapshot claims",
            "unsafe_snapshot.executes = true",
            "unsafe_snapshot.exposes_native_handles = true"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "editor_shell_material_preview_status=diagnostic_only",
            "editor_shell_material_preview_native_handles_exposed=0",
            "host-private-native",
            "material-preview GPU parity remains unsupported"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "editor_shell_material_preview_status=diagnostic_only",
            "editor_shell_material_preview_native_handles_exposed=0",
            "host-private-native",
            "material-preview GPU parity remains unsupported"
        )
    }
)
foreach ($check in $nativeEditorMaterialPreviewChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing native editor material preview contract: $($missingNeedles -join ', ')"
    }
}

$materialPreviewCoreText = Get-AgentSurfaceText "editor/core/src/material_asset_preview_panel.cpp"
if ($materialPreviewCoreText.Contains("d3d12-shared-texture")) {
    Write-Error "ai-integration-check: material preview core display path contract must stay backend-neutral"
}
