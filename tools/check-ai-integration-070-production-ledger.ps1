#requires -Version 7.0
#requires -PSEdition Core

# Chapter 7 for check-ai-integration.ps1 static contracts.

$editorProjectNativeDialogChecks = @(
    @{
        Path = "editor/core/include/mirakana/editor/project.hpp"
        Needles = @(
            "EditorProjectFileDialogMode",
            "EditorProjectFileDialogModel",
            "EditorProjectFileDialogRow",
            "make_project_open_dialog_request",
            "make_project_save_dialog_request",
            "make_project_open_dialog_model",
            "make_project_save_dialog_model",
            "make_project_file_dialog_ui_model"
        )
    },
    @{
        Path = "editor/core/src/project.cpp"
        Needles = @(
            "project_file_dialog_status",
            "project_file_dialog_action",
            "project_file_dialog_mode_id",
            "requires exactly one selected path",
            "selection must end with .geproject",
            "project file dialog ui element could not be added",
            "project_file_dialog.",
            "geproject",
            "make_project_open_dialog_request",
            "make_project_save_dialog_request",
            "make_project_file_dialog_ui_model"
        )
    },
    @{
        Path = "editor/src/main.cpp"
        Needles = @(
            "Open Project...",
            "Save Project As...",
            "Project Open Dialog",
            "Project Save Dialog",
            "show_project_open_dialog",
            "show_project_save_dialog",
            "poll_project_file_dialogs",
            "project_store_relative_project_path",
            "workspace_path_for_project_path",
            "scene_path_for_project_document",
            "open_project_bundle_from_paths",
            "save_project_bundle_to_paths",
            "project_open_dialog_id_",
            "project_save_dialog_id_",
            "make_project_open_dialog_request",
            "make_project_save_dialog_request",
            "make_project_open_dialog_model",
            "make_project_save_dialog_model",
            "mirakana::IFileDialogService",
            "mirakana::SdlFileDialogService"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor project native file dialogs review results and retained rows",
            "project_file_dialog.open.status.value",
            "project_file_dialog.open.selected_path.value",
            "project_file_dialog.save.status.value",
            "make_project_open_dialog_request",
            "make_project_save_dialog_model",
            "at least one path",
            "exactly one",
            ".geproject"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Editor Project Native Dialog v1",
            "EditorProjectFileDialogModel",
            "EditorProjectFileDialogMode",
            "make_project_open_dialog_request",
            "make_project_save_dialog_request",
            "make_project_open_dialog_model",
            "make_project_save_dialog_model",
            "make_project_file_dialog_ui_model",
            "project_file_dialog.open",
            "project_file_dialog.save",
            "Open Project...",
            "Save Project As...",
            "load_project_bundle",
            "save_project_bundle"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Editor Project Native Dialog v1",
            "2026-05-07-editor-project-native-dialog-v1.md",
            "EditorProjectFileDialogModel",
            "EditorProjectFileDialogMode",
            "make_project_open_dialog_request",
            "make_project_save_dialog_request",
            "make_project_open_dialog_model",
            "make_project_save_dialog_model",
            "make_project_file_dialog_ui_model",
            "project_file_dialog.open",
            "project_file_dialog.save",
            "Open Project...",
            "Save Project As...",
            "Project Settings",
            "load_project_bundle",
            "save_project_bundle"
        )
    },
    @{
        Path = "docs/roadmap.md"
        Needles = @(
            "Editor Project Native Dialog v1",
            "EditorProjectFileDialogModel",
            "EditorProjectFileDialogMode",
            "make_project_open_dialog_request",
            "make_project_save_dialog_request",
            "make_project_open_dialog_model",
            "make_project_save_dialog_model",
            "make_project_file_dialog_ui_model",
            "project_file_dialog.open",
            "project_file_dialog.save",
            "Open Project...",
            "Save Project As...",
            "load_project_bundle",
            "save_project_bundle",
            "outside Profiler, Scene, Prefab Variant, and Project"
        )
    },
    @{
        Path = "docs/superpowers/plans/README.md"
        Needles = @(
            "2026-05-07-editor-project-native-dialog-v1.md",
            "EditorProjectFileDialogModel",
            "make_project_open_dialog_request",
            "make_project_save_dialog_request",
            "project_file_dialog.open",
            "project_file_dialog.save",
            "Open Project...",
            "Save Project As..."
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md"
        Needles = @(
            "Editor Project Native Dialog v1",
            "EditorProjectFileDialogModel",
            "EditorProjectFileDialogMode",
            "make_project_open_dialog_request",
            "make_project_save_dialog_request",
            "make_project_open_dialog_model",
            "make_project_save_dialog_model",
            "make_project_file_dialog_ui_model",
            "project_file_dialog.open",
            "project_file_dialog.save",
            "Open Project...",
            "Save Project As...",
            "load_project_bundle",
            "save_project_bundle",
            "outside Profiler, Scene, Prefab Variant, and Project"
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-07-editor-project-native-dialog-v1.md"
        Needles = @(
            "Editor Project Native Dialog v1 Implementation Plan",
            "EditorProjectFileDialogModel",
            "make_project_open_dialog_request",
            "make_project_save_dialog_request",
            "make_project_open_dialog_model",
            "make_project_save_dialog_model",
            "make_project_file_dialog_ui_model",
            "SdlFileDialogService",
            "project_file_dialog.open",
            "project_file_dialog.save"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "currentEditorProjectFileDialogs",
            "Editor Project Native Dialog v1",
            "EditorProjectFileDialogModel",
            "EditorProjectFileDialogMode",
            "make_project_open_dialog_request",
            "make_project_save_dialog_request",
            "make_project_open_dialog_model",
            "make_project_save_dialog_model",
            "make_project_file_dialog_ui_model",
            "project_file_dialog.open",
            "project_file_dialog.save",
            "Open Project...",
            "Save Project As...",
            "ProjectBundlePaths",
            "load_project_bundle",
            "save_project_bundle"
        )
    },
    @{
        Path = ".agents/skills/editor-change/SKILL.md"
        Needles = @(
            "make_project_open_dialog_request",
            "make_project_save_dialog_request",
            "make_project_file_dialog_ui_model",
            "project_file_dialog.open",
            "project_file_dialog.save",
            "Open Project...",
            "Save Project As...",
            "load_project_bundle",
            "save_project_bundle"
        )
    },
    @{
        Path = ".claude/skills/gameengine-editor/SKILL.md"
        Needles = @(
            "make_project_open_dialog_request",
            "make_project_save_dialog_request",
            "make_project_file_dialog_ui_model",
            "project_file_dialog.open",
            "project_file_dialog.save",
            "Open Project...",
            "Save Project As...",
            "load_project_bundle",
            "save_project_bundle"
        )
    }
)
foreach ($check in $editorProjectNativeDialogChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing editor project native dialog contract: $($missingNeedles -join ', ')"
    }
}

$prefabVariantConflictReviewChecks = @(
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Editor Prefab Variant Conflict Review v1",
            "Editor Prefab Variant Reviewed Resolution v1",
            "Editor Prefab Variant Missing Node Cleanup v1",
            "Editor Prefab Variant Node Retarget Review v1",
            "Editor Prefab Variant Batch Resolution Review v1",
            "Editor Prefab Variant Source Mismatch Retarget Review v1",
            "Editor Prefab Variant Source Mismatch Accept Current Review v1",
            "Editor Prefab Variant Native Dialog v1",
            "Editor Prefab Variant Base Refresh Merge Review v1",
            "PrefabVariantConflictReviewModel",
            "PrefabVariantConflictBatchResolutionPlan",
            "PrefabVariantBaseRefreshPlan",
            "PrefabVariantBaseRefreshRow",
            "EditorPrefabVariantFileDialogModel",
            "PrefabNodeOverride::source_node_name",
            "source_node_mismatch",
            "accept_current_node",
            "deserialize_prefab_variant_definition_for_review",
            "resolve_prefab_variant_conflict",
            "resolve_prefab_variant_conflicts",
            "make_prefab_variant_conflict_resolution_action",
            "make_prefab_variant_conflict_batch_resolution_action",
            "make_prefab_variant_conflict_review_ui_model",
            "make_prefab_variant_open_dialog_request",
            "make_prefab_variant_save_dialog_request",
            "make_prefab_variant_open_dialog_model",
            "make_prefab_variant_save_dialog_model",
            "make_prefab_variant_file_dialog_ui_model",
            "plan_prefab_variant_base_refresh",
            "apply_prefab_variant_base_refresh",
            "make_prefab_variant_base_refresh_ui_model",
            "prefab_variant_conflicts",
            "prefab_variant_conflicts.batch_resolution",
            "prefab_variant_base_refresh",
            "prefab_variant_file_dialog.open",
            "prefab_variant_file_dialog.save",
            "Apply All Reviewed",
            "Browse Load Variant",
            "Browse Save Variant",
            "Remove missing-node override",
            "Retarget override to node N",
            "Accept current node N",
            "composition index-based",
            "automatic merge/rebase/resolution UX"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Editor Prefab Variant Conflict Review v1",
            "Reviewed Resolution v1",
            "Missing Node Cleanup v1",
            "Node Retarget Review v1",
            "Source Mismatch Retarget Review v1",
            "Source Mismatch Accept Current Review v1",
            "Native Dialog v1",
            "Base Refresh Merge Review v1",
            "editor-prefab-variant-batch-resolution-review-v1.md",
            "editor-prefab-variant-source-mismatch-retarget-review-v1.md",
            "editor-prefab-variant-source-mismatch-accept-current-review-v1.md",
            "editor-prefab-variant-native-dialog-v1.md",
            "editor-prefab-variant-base-refresh-merge-review-v1.md",
            "PrefabVariantConflictRow",
            "PrefabVariantConflictBatchResolutionPlan",
            "PrefabVariantBaseRefreshPlan",
            "EditorPrefabVariantFileDialogModel",
            "PrefabNodeOverride::source_node_name",
            "source_node_mismatch",
            "accept_current_node",
            "deserialize_prefab_variant_definition_for_review",
            "resolve_prefab_variant_conflicts",
            "make_prefab_variant_conflict_resolution_action",
            "make_prefab_variant_conflict_batch_resolution_action",
            "make_prefab_variant_open_dialog_request",
            "make_prefab_variant_save_dialog_request",
            "plan_prefab_variant_base_refresh",
            "apply_prefab_variant_base_refresh",
            "make_prefab_variant_base_refresh_ui_model",
            "prefab_variant_conflicts",
            "prefab_variant_conflicts.batch_resolution",
            "prefab_variant_base_refresh",
            "prefab_variant_file_dialog.open",
            "prefab_variant_file_dialog.save",
            "Browse Load Variant",
            "Browse Save Variant",
            "Apply All Reviewed",
            "unique source-node retarget",
            "existing-node",
            "accept-current hint repair",
            "composition remains index-based",
            "nested prefab propagation",
            "automatic merge/rebase/resolution UX"
        )
    },
    @{
        Path = "docs/ai-game-development.md"
        Needles = @(
            "Editor Prefab Variant Conflict Review v1",
            "Editor Prefab Variant Reviewed Resolution v1",
            "Editor Prefab Variant Missing Node Cleanup v1",
            "Editor Prefab Variant Node Retarget Review v1",
            "Editor Prefab Variant Batch Resolution Review v1",
            "Editor Prefab Variant Source Mismatch Retarget Review v1",
            "Editor Prefab Variant Source Mismatch Accept Current Review v1",
            "Editor Prefab Variant Native Dialog v1",
            "plan_prefab_variant_base_refresh",
            "apply_prefab_variant_base_refresh",
            "make_prefab_variant_base_refresh_ui_model",
            "PrefabVariantConflictReviewModel",
            "PrefabVariantConflictBatchResolutionPlan",
            "EditorPrefabVariantFileDialogModel",
            "PrefabNodeOverride::source_node_name",
            "source_node_mismatch",
            "accept_current_node",
            "deserialize_prefab_variant_definition_for_review",
            "resolve_prefab_variant_conflict",
            "resolve_prefab_variant_conflicts",
            "make_prefab_variant_open_dialog_request",
            "make_prefab_variant_save_dialog_model",
            "prefab_variant_conflicts",
            "prefab_variant_conflicts.batch_resolution",
            "prefab_variant_base_refresh",
            "prefab_variant_file_dialog.open",
            "prefab_variant_file_dialog.save",
            "Apply All Reviewed",
            "Browse Load Variant",
            "Browse Save Variant",
            "Retarget override to node N",
            "Accept current node N",
            "composition stays index-based",
            "not an execution surface"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "Editor Prefab Variant Conflict Review v1, Reviewed Resolution v1, Missing Node Cleanup v1, Node Retarget Review v1, Batch Resolution Review v1, Source Mismatch Retarget Review v1, Source Mismatch Accept Current Review v1, Native Dialog v1, and Base Refresh Merge Review v1 coverage",
            "Reviewed Resolution v1",
            "PrefabVariantConflictBatchResolutionPlan",
            "PrefabVariantBaseRefreshPlan",
            "EditorPrefabVariantFileDialogModel",
            "make_prefab_variant_conflict_review_model",
            "make_prefab_variant_conflict_resolution_action",
            "make_prefab_variant_conflict_batch_resolution_action",
            "resolve_prefab_variant_conflicts",
            "plan_prefab_variant_base_refresh",
            "apply_prefab_variant_base_refresh",
            "make_prefab_variant_base_refresh_ui_model",
            "make_prefab_variant_open_dialog_request",
            "make_prefab_variant_save_dialog_model",
            "plan_prefab_variant_base_refresh",
            "apply_prefab_variant_base_refresh",
            "make_prefab_variant_base_refresh_ui_model",
            "PrefabNodeOverride::source_node_name",
            "source_node_mismatch",
            "accept_current_node",
            "deserialize_prefab_variant_definition_for_review",
            "prefab_variant_conflicts",
            "prefab_variant_conflicts.batch_resolution",
            "prefab_variant_base_refresh",
            "prefab_variant_file_dialog.open",
            "prefab_variant_file_dialog.save"
        )
    },
    @{
        Path = "docs/superpowers/plans/README.md"
        Needles = @(
            "2026-05-07-editor-prefab-variant-conflict-review-v1.md",
            "2026-05-07-editor-prefab-variant-reviewed-resolution-v1.md",
            "2026-05-07-editor-prefab-variant-missing-node-cleanup-v1.md",
            "2026-05-07-editor-prefab-variant-node-retarget-review-v1.md",
            "2026-05-07-editor-prefab-variant-batch-resolution-review-v1.md",
            "2026-05-07-editor-prefab-variant-source-mismatch-retarget-review-v1.md",
            "2026-05-07-editor-prefab-variant-source-mismatch-accept-current-review-v1.md",
            "2026-05-07-editor-prefab-variant-native-dialog-v1.md",
            "2026-05-09-editor-prefab-variant-base-refresh-merge-review-v1.md",
            "PrefabVariantConflictReviewModel",
            "PrefabVariantConflictBatchResolutionPlan",
            "PrefabVariantBaseRefreshPlan",
            "EditorPrefabVariantFileDialogModel",
            "PrefabNodeOverride::source_node_name",
            "source_node_mismatch",
            "accept_current_node",
            "make_prefab_variant_conflict_resolution_action",
            "make_prefab_variant_conflict_batch_resolution_action",
            "make_prefab_variant_open_dialog_request",
            "make_prefab_variant_save_dialog_request",
            "plan_prefab_variant_base_refresh",
            "apply_prefab_variant_base_refresh",
            "make_prefab_variant_base_refresh_ui_model",
            "resolve_prefab_variant_conflicts",
            "deserialize_prefab_variant_definition_for_review",
            "Retarget override to node",
            "Accept current node",
            "prefab_variant_conflicts",
            "prefab_variant_conflicts.batch_resolution",
            "prefab_variant_base_refresh",
            "prefab_variant_file_dialog.open",
            "prefab_variant_file_dialog.save",
            "Browse Load Variant",
            "Browse Save Variant",
            "Apply All Reviewed"
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md"
        Needles = @(
            "Editor Prefab Variant Conflict Review v1",
            "Editor Prefab Variant Reviewed Resolution v1",
            "Editor Prefab Variant Missing Node Cleanup v1",
            "Editor Prefab Variant Node Retarget Review v1",
            "Editor Prefab Variant Batch Resolution Review v1",
            "Editor Prefab Variant Source Mismatch Retarget Review v1",
            "Editor Prefab Variant Source Mismatch Accept Current Review v1",
            "Editor Prefab Variant Native Dialog v1",
            "Editor Prefab Variant Base Refresh Merge Review v1",
            "PrefabVariantConflictReviewModel",
            "PrefabVariantConflictBatchResolutionPlan",
            "PrefabVariantBaseRefreshPlan",
            "EditorPrefabVariantFileDialogModel",
            "PrefabNodeOverride::source_node_name",
            "source_node_mismatch",
            "accept_current_node",
            "deserialize_prefab_variant_definition_for_review",
            "resolve_prefab_variant_conflict",
            "resolve_prefab_variant_conflicts",
            "make_prefab_variant_open_dialog_request",
            "make_prefab_variant_save_dialog_model",
            "plan_prefab_variant_base_refresh",
            "apply_prefab_variant_base_refresh",
            "make_prefab_variant_base_refresh_ui_model",
            "prefab_variant_conflicts",
            "prefab_variant_conflicts.batch_resolution",
            "prefab_variant_base_refresh",
            "prefab_variant_file_dialog.open",
            "prefab_variant_file_dialog.save",
            "Apply All Reviewed",
            "Browse Load Variant",
            "Browse Save Variant",
            "source-node hint uniquely matches",
            "source-node-mismatch rows",
            "accept-current",
            "embedded-base refresh",
            "automatic merge/rebase/resolution UX"
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-07-editor-prefab-variant-native-dialog-v1.md"
        Needles = @(
            "Editor Prefab Variant Native Dialog v1 Implementation Plan",
            "EditorPrefabVariantFileDialogModel",
            "make_prefab_variant_open_dialog_request",
            "make_prefab_variant_save_dialog_request",
            "make_prefab_variant_open_dialog_model",
            "make_prefab_variant_save_dialog_model",
            "make_prefab_variant_file_dialog_ui_model",
            "Browse Load Variant",
            "Browse Save Variant",
            "SdlFileDialogService",
            "broader editor native save/open dialogs"
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-07-editor-prefab-variant-conflict-review-v1.md"
        Needles = @(
            "nested-prefab-conflict-ux-v1",
            "PrefabVariantConflictReviewModel",
            "prefab_variant_conflicts",
            "automatic merge/rebase/resolution UX"
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-07-editor-prefab-variant-reviewed-resolution-v1.md"
        Needles = @(
            "editor-prefab-variant-reviewed-resolution-v1",
            "resolve_prefab_variant_conflict",
            "make_prefab_variant_conflict_resolution_action",
            "prefab_variant_conflicts",
            "missing-node resolution",
            "automatic merge/rebase/resolution UX"
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-07-editor-prefab-variant-missing-node-cleanup-v1.md"
        Needles = @(
            "editor-prefab-variant-missing-node-cleanup-v1",
            "deserialize_prefab_variant_definition_for_review",
            "Remove missing-node override",
            "node remapping",
            "automatic merge/rebase/resolution UX"
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-07-editor-prefab-variant-node-retarget-review-v1.md"
        Needles = @(
            "editor-prefab-variant-node-retarget-review-v1",
            "PrefabNodeOverride::source_node_name",
            "override.N.source_node_name",
            "Retarget override to node",
            "automatic merge/rebase/resolution UX"
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-07-editor-prefab-variant-batch-resolution-review-v1.md"
        Needles = @(
            "Editor Prefab Variant Batch Resolution Review v1",
            "PrefabVariantConflictBatchResolutionPlan",
            "resolve_prefab_variant_conflicts",
            "make_prefab_variant_conflict_batch_resolution_action",
            "prefab_variant_conflicts.batch_resolution",
            "Apply All Reviewed",
            "automatic merge/rebase"
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-07-editor-prefab-variant-source-mismatch-retarget-review-v1.md"
        Needles = @(
            "Editor Prefab Variant Source Mismatch Retarget Review v1",
            "source_node_mismatch",
            "Retarget override to node N",
            "prefab_variant_conflicts.rows.node.1.transform.resolution_kind",
            "resolve_prefab_variant_conflict",
            "resolve_prefab_variant_conflicts",
            "composition index-based",
            "automatic merge/rebase"
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-07-editor-prefab-variant-source-mismatch-accept-current-review-v1.md"
        Needles = @(
            "Editor Prefab Variant Source Mismatch Accept Current Review v1",
            "source_node_mismatch",
            "accept_current_node",
            "Accept current node N",
            "accept_current.node.1.transform",
            "updates only",
            "resolve_prefab_variant_conflict",
            "resolve_prefab_variant_conflicts",
            'strict `MK_scene` prefab variant validation and composition index-based',
            "automatic merge/rebase"
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-09-editor-prefab-variant-base-refresh-merge-review-v1.md"
        Needles = @(
            "Editor Prefab Variant Base Refresh Merge Review Implementation Plan",
            "PrefabVariantBaseRefreshPlan",
            "plan_prefab_variant_base_refresh",
            "apply_prefab_variant_base_refresh",
            "make_prefab_variant_base_refresh_ui_model",
            "prefab_variant_base_refresh",
            "missing hints",
            "duplicate target",
            "full nested prefab propagation"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "Editor Prefab Variant Conflict Review v1",
            "Reviewed Resolution v1",
            "Missing Node Cleanup v1",
            "Node Retarget Review v1",
            "Batch Resolution Review v1",
            "Source Mismatch Retarget Review v1",
            "Source Mismatch Accept Current Review v1",
            "Native Dialog v1",
            "Base Refresh Merge Review v1",
            "PrefabVariantConflictReviewModel",
            "PrefabVariantConflictBatchResolutionPlan",
            "PrefabVariantBaseRefreshPlan",
            "EditorPrefabVariantFileDialogModel",
            "PrefabNodeOverride::source_node_name",
            "source_node_mismatch",
            "accept_current_node",
            "override.N.source_node_name",
            "deserialize_prefab_variant_definition_for_review",
            "make_prefab_variant_conflict_resolution_action",
            "make_prefab_variant_conflict_batch_resolution_action",
            "plan_prefab_variant_base_refresh",
            "apply_prefab_variant_base_refresh",
            "make_prefab_variant_base_refresh_ui_model",
            "resolve_prefab_variant_conflicts",
            "make_prefab_variant_conflict_review_ui_model",
            "make_prefab_variant_open_dialog_request",
            "make_prefab_variant_save_dialog_model",
            "Retarget override to node N",
            "Accept current node N",
            "resolution kind/target",
            "prefab_variant_conflicts",
            "prefab_variant_conflicts.batch_resolution",
            "prefab_variant_base_refresh",
            "prefab_variant_file_dialog.open",
            "prefab_variant_file_dialog.save",
            "Browse Load Variant",
            "Browse Save Variant",
            "Apply All Reviewed",
            "reviewed missing-node/source-mismatch retarget, accept-current hint repair, batch cleanup, explicit base-refresh apply, source-link diagnostics, explicit scene prefab instance refresh, reviewed local child refresh preservation, and reviewed stale source-node keep-as-local preservation"
        )
    },
    @{
        Path = ".agents/skills/editor-change/SKILL.md"
        Needles = @(
            "make_prefab_variant_conflict_review_model",
            "make_prefab_variant_conflict_resolution_action",
            "make_prefab_variant_conflict_batch_resolution_action",
            "resolve_prefab_variant_conflicts",
            "make_prefab_variant_open_dialog_request",
            "make_prefab_variant_save_dialog_request",
            "make_prefab_variant_file_dialog_ui_model",
            "PrefabNodeOverride::source_node_name",
            "source_node_mismatch",
            "accept_current_node",
            "deserialize_prefab_variant_definition_for_review",
            "source-node retarget",
            "Accept current node N",
            "accept-current hint repair",
            "prefab_variant_base_refresh",
            "prefab_variant_conflicts",
            "prefab_variant_conflicts.batch_resolution",
            "prefab_variant_file_dialog.open",
            "prefab_variant_file_dialog.save",
            "Browse Load Variant",
            "Browse Save Variant",
            "broader editor native save/open dialogs outside Profiler, Scene, and Prefab Variant",
            "Apply All Reviewed",
            "automatic merge/rebase/resolution UX"
        )
    },
    @{
        Path = ".claude/skills/gameengine-editor/SKILL.md"
        Needles = @(
            "make_prefab_variant_conflict_review_model",
            "make_prefab_variant_conflict_resolution_action",
            "make_prefab_variant_conflict_batch_resolution_action",
            "resolve_prefab_variant_conflicts",
            "plan_prefab_variant_base_refresh",
            "apply_prefab_variant_base_refresh",
            "make_prefab_variant_base_refresh_ui_model",
            "make_prefab_variant_open_dialog_request",
            "make_prefab_variant_save_dialog_request",
            "make_prefab_variant_file_dialog_ui_model",
            "PrefabNodeOverride::source_node_name",
            "source_node_mismatch",
            "accept_current_node",
            "deserialize_prefab_variant_definition_for_review",
            "source-node retarget",
            "Accept current node N",
            "accept-current hint repair",
            "prefab_variant_base_refresh",
            "prefab_variant_conflicts",
            "prefab_variant_conflicts.batch_resolution",
            "prefab_variant_file_dialog.open",
            "prefab_variant_file_dialog.save",
            "Browse Load Variant",
            "Browse Save Variant",
            "broader editor native save/open dialogs outside Profiler, Scene, and Prefab Variant",
            "Apply All Reviewed",
            "automatic merge/rebase/resolution UX"
        )
    }
)
foreach ($check in $prefabVariantConflictReviewChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing prefab variant conflict review contract: $($missingNeedles -join ', ')"
    }
}

Get-ChildItem -Path (Join-Path $root "games") -Recurse -Filter "game.agent.json" | ForEach-Object {
    $gameManifest = Get-Content -LiteralPath $_.FullName -Raw | ConvertFrom-Json
    foreach ($field in @("schemaVersion", "name", "entryPoint", "target", "aiWorkflow", "gameplayContract", "backendReadiness", "importerRequirements", "packagingTargets", "validationRecipes")) {
        if (-not $gameManifest.PSObject.Properties.Name.Contains($field)) {
            Write-Error "Game manifest missing required field '$field': $($_.FullName)"
        }
    }
    $entryPath = Join-Path $root $gameManifest.entryPoint
    if (-not (Test-Path $entryPath)) {
        Write-Error "Game manifest entryPoint does not exist: $($gameManifest.entryPoint)"
    }
}

$sample2dManifestPath = "games/sample_2d_playable_foundation/game.agent.json"
$sample2dManifestFullPath = Resolve-RequiredAgentPath $sample2dManifestPath
$sample2dManifest = Get-Content -LiteralPath $sample2dManifestFullPath -Raw | ConvertFrom-Json
if ($sample2dManifest.target -ne "sample_2d_playable_foundation") {
    Write-Error "$sample2dManifestPath target must be sample_2d_playable_foundation"
}
if ($sample2dManifest.gameplayContract.productionRecipe -ne "2d-playable-source-tree") {
    Write-Error "$sample2dManifestPath gameplayContract.productionRecipe must be 2d-playable-source-tree"
}
foreach ($module in @("MK_runtime", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_audio", "MK_renderer")) {
    if (@($sample2dManifest.engineModules) -notcontains $module) {
        Write-Error "$sample2dManifestPath engineModules missing $module"
    }
}
if (@($sample2dManifest.packagingTargets) -notcontains "source-tree-default") {
    Write-Error "$sample2dManifestPath must use source-tree-default packaging target"
}
if (@($sample2dManifest.packagingTargets) -contains "desktop-game-runtime") {
    Write-Error "$sample2dManifestPath must not claim desktop-game-runtime readiness in this source-tree 2D slice"
}

$sample2dDesktopManifestPath = "games/sample_2d_desktop_runtime_package/game.agent.json"
$sample2dDesktopManifestFullPath = Resolve-RequiredAgentPath $sample2dDesktopManifestPath
$sample2dDesktopManifest = Get-Content -LiteralPath $sample2dDesktopManifestFullPath -Raw | ConvertFrom-Json
if ($sample2dDesktopManifest.target -ne "sample_2d_desktop_runtime_package") {
    Write-Error "$sample2dDesktopManifestPath target must be sample_2d_desktop_runtime_package"
}
if ($sample2dDesktopManifest.gameplayContract.productionRecipe -ne "2d-desktop-runtime-package") {
    Write-Error "$sample2dDesktopManifestPath gameplayContract.productionRecipe must be 2d-desktop-runtime-package"
}
foreach ($module in @("MK_runtime", "MK_runtime_scene", "MK_runtime_host", "MK_runtime_host_sdl3", "MK_runtime_host_sdl3_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_audio", "MK_renderer")) {
    if (@($sample2dDesktopManifest.engineModules) -notcontains $module) {
        Write-Error "$sample2dDesktopManifestPath engineModules missing $module"
    }
}
foreach ($target in @("desktop-game-runtime", "desktop-runtime-release")) {
    if (@($sample2dDesktopManifest.packagingTargets) -notcontains $target) {
        Write-Error "$sample2dDesktopManifestPath packagingTargets missing $target"
    }
}
foreach ($packageFile in @(
    "runtime/sample_2d_desktop_runtime_package.config",
    "runtime/sample_2d_desktop_runtime_package.geindex",
    "runtime/assets/2d/player.texture.geasset",
    "runtime/assets/2d/player.material",
    "runtime/assets/2d/jump.audio.geasset",
    "runtime/assets/2d/level.tilemap",
    "runtime/assets/2d/player.sprite_animation",
    "runtime/assets/2d/playable.scene"
)) {
    if (@($sample2dDesktopManifest.runtimePackageFiles) -notcontains $packageFile) {
        Write-Error "$sample2dDesktopManifestPath runtimePackageFiles missing $packageFile"
    }
}
$sample2dDesktopGitAttributes = Get-Content -LiteralPath (Join-Path $root "games/sample_2d_desktop_runtime_package/runtime/.gitattributes") -Raw
foreach ($attributeRule in @(
    "*.geindex text eol=lf",
    "*.geasset text eol=lf",
    "*.material text eol=lf",
    "*.scene text eol=lf",
    "*.tilemap text eol=lf",
    "*.sprite_animation text eol=lf"
)) {
    Assert-ContainsText $sample2dDesktopGitAttributes $attributeRule "games/sample_2d_desktop_runtime_package/runtime/.gitattributes"
}
$sample2dDesktopManifestText = Get-Content -LiteralPath $sample2dDesktopManifestFullPath -Raw
foreach ($needle in @(
    "native 2D sprite package proof",
    "installed-native-2d-sprite-smoke",
    "installed-2d-sprite-animation-smoke",
    "installed-2d-tilemap-runtime-ux-smoke",
    "--require-native-2d-sprites",
    "--require-sprite-animation",
    "--require-tilemap-runtime-ux",
    "sprite_animation_frames_sampled",
    "tilemap_cells_sampled",
    "public native or RHI handle access remains unsupported",
    "broad production sprite batching readiness remains unsupported",
    "general production renderer quality remains unsupported"
)) {
    Assert-ContainsText $sample2dDesktopManifestText $needle $sample2dDesktopManifestPath
}
$sample2dDesktopMainText = Get-Content -LiteralPath (Join-Path $root "games/sample_2d_desktop_runtime_package/main.cpp") -Raw
foreach ($needle in @(
    "mirakana/renderer/sprite_batch.hpp",
    "mirakana/runtime/runtime_diagnostics.hpp",
    "--require-native-2d-sprites",
    "--require-sprite-animation",
    "--require-tilemap-runtime-ux",
    "runtime_sprite_animation_payload",
    "runtime_tilemap_payload",
    "sample_runtime_tilemap_visible_cells",
    "sample_and_apply_runtime_scene_render_sprite_animation",
    "sample_2d_desktop_runtime_package_native_sprite_overlay.vs.dxil",
    "sample_2d_desktop_runtime_package_native_sprite_overlay.ps.dxil",
    "sample_2d_desktop_runtime_package_native_sprite_overlay.vs.spv",
    "sample_2d_desktop_runtime_package_native_sprite_overlay.ps.spv",
    "native_2d_sprites_status",
    "native_2d_textured_sprites_submitted",
    "native_2d_texture_binds",
    "native_2d_sprite_batches_executed",
    "native_2d_sprite_batch_sprites_executed",
    "native_2d_sprite_batch_textured_sprites_executed",
    "native_2d_sprite_batch_texture_binds",
    "plan_scene_sprite_batches",
    "sprite_batch_plan_draws",
    "sprite_batch_plan_texture_binds",
    "sprite_batch_plan_diagnostics",
    "sprite_animation_frames_sampled",
    "sprite_animation_frames_applied",
    "sprite_animation_diagnostics",
    "tilemap_cells_sampled",
    "tilemap_diagnostics",
    "required_native_2d_sprites_unavailable",
    "required_sprite_animation_unavailable",
    "required_tilemap_runtime_ux_unavailable"
)) {
    Assert-ContainsText $sample2dDesktopMainText $needle "games/sample_2d_desktop_runtime_package/main.cpp"
}
$sample2dInstalledRuntimeValidationText = Get-AgentSurfaceText "tools/validate-installed-desktop-runtime.ps1"
foreach ($needle in @(
    "native_2d_sprite_batches_executed",
    "native_2d_sprite_batch_sprites_executed",
    "native_2d_sprite_batch_textured_sprites_executed",
    "native_2d_sprite_batch_texture_binds",
    "sprite_animation_frames_sampled",
    "sprite_animation_frames_applied",
    "sprite_animation_diagnostics",
    "tilemap_cells_sampled",
    "tilemap_diagnostics"
)) {
    Assert-ContainsText $sample2dInstalledRuntimeValidationText $needle "tools/validate-installed-desktop-runtime.ps1"
}
$sample2dDesktopCMakeText = Get-Content -LiteralPath (Join-Path $root "games/CMakeLists.txt") -Raw
foreach ($needle in @(
    "sample_2d_desktop_runtime_package_native_sprite_overlay.vs.dxil",
    "sample_2d_desktop_runtime_package_native_sprite_overlay.ps.dxil",
    "sample_2d_desktop_runtime_package_native_sprite_overlay.vs.spv",
    "sample_2d_desktop_runtime_package_native_sprite_overlay.ps.spv",
    "--require-native-2d-sprites",
    "--require-sprite-animation",
    "--require-tilemap-runtime-ux"
)) {
    Assert-ContainsText $sample2dDesktopCMakeText $needle "games/CMakeLists.txt"
}
Assert-RuntimeSceneValidationTarget `
    $sample2dDesktopManifest `
    $sample2dDesktopManifestPath `
    "packaged-2d-scene" `
    "runtime/sample_2d_desktop_runtime_package.geindex" `
    "sample/2d-desktop-runtime-package/scene"
Assert-AtlasTilemapAuthoringTarget `
    $sample2dDesktopManifest `
    $sample2dDesktopManifestPath `
    "packaged-2d-tilemap" `
    "runtime/sample_2d_desktop_runtime_package.geindex" `
    "runtime/assets/2d/level.tilemap" `
    "runtime/assets/2d/player.texture.geasset"

$sample3dManifestPath = "games/sample_desktop_runtime_game/game.agent.json"
$sample3dManifestFullPath = Resolve-RequiredAgentPath $sample3dManifestPath
$sample3dManifest = Get-Content -LiteralPath $sample3dManifestFullPath -Raw | ConvertFrom-Json
if ($sample3dManifest.target -ne "sample_desktop_runtime_game") {
    Write-Error "$sample3dManifestPath target must be sample_desktop_runtime_game"
}
if ($sample3dManifest.gameplayContract.productionRecipe -ne "3d-playable-desktop-package") {
    Write-Error "$sample3dManifestPath gameplayContract.productionRecipe must be 3d-playable-desktop-package"
}
foreach ($module in @("MK_animation", "MK_runtime", "MK_runtime_rhi", "MK_runtime_scene", "MK_runtime_scene_rhi", "MK_runtime_host", "MK_runtime_host_sdl3", "MK_runtime_host_sdl3_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_renderer")) {
    if (@($sample3dManifest.engineModules) -notcontains $module) {
        Write-Error "$sample3dManifestPath engineModules missing $module"
    }
}
foreach ($target in @("desktop-game-runtime", "desktop-runtime-release")) {
    if (@($sample3dManifest.packagingTargets) -notcontains $target) {
        Write-Error "$sample3dManifestPath packagingTargets missing $target"
    }
}
foreach ($packageFile in @(
    "runtime/sample_desktop_runtime_game.config",
    "runtime/sample_desktop_runtime_game.geindex",
    "runtime/assets/desktop_runtime/base_color.texture.geasset",
    "runtime/assets/desktop_runtime/triangle.mesh",
    "runtime/assets/desktop_runtime/packaged_pose.animation_quaternion_clip",
    "runtime/assets/desktop_runtime/unlit.material",
    "runtime/assets/desktop_runtime/packaged_scene.scene"
)) {
    if (@($sample3dManifest.runtimePackageFiles) -notcontains $packageFile) {
        Write-Error "$sample3dManifestPath runtimePackageFiles missing $packageFile"
    }
}
Assert-RuntimeSceneValidationTarget `
    $sample3dManifest `
    $sample3dManifestPath `
    "packaged-3d-scene" `
    "runtime/sample_desktop_runtime_game.geindex" `
    "sample/desktop-runtime/scene"
Assert-PrefabScenePackageAuthoringTarget `
    $sample3dManifest `
    $sample3dManifestPath `
    "packaged-3d-prefab-scene" `
    "runtime/sample_desktop_runtime_game.geindex" `
    "runtime/assets/desktop_runtime/packaged_scene.scene" `
    "packaged-3d-scene"
Assert-RegisteredSourceAssetCookTarget `
    $sample3dManifest `
    $sample3dManifestPath `
    "packaged-3d-registered-source-cook" `
    "packaged-3d-prefab-scene" `
    "source/assets/package.geassets" `
    "runtime/sample_desktop_runtime_game.geindex" `
    @("sample/desktop-runtime/material") `
    "registered_source_registry_closure" `
    "registry_closure"
foreach ($recipe in @("desktop-game-runtime", "desktop-runtime-release-target", "installed-d3d12-scene-gpu-smoke", "installed-vulkan-scene-gpu-smoke")) {
    if (@($sample3dManifest.validationRecipes | ForEach-Object { $_.name }) -notcontains $recipe) {
        Write-Error "$sample3dManifestPath validationRecipes missing $recipe"
    }
}
$sample3dManifestText = Get-Content -LiteralPath $sample3dManifestFullPath -Raw
foreach ($needle in @(
    "material instance intent",
    "camera/controller movement",
    "HUD diagnostics",
    "runtime source asset parsing remains unsupported",
    "material graph remains unsupported",
    "skeletal animation production path remains unsupported",
    "GPU skinning is host-proven",
    "package streaming remains unsupported",
    "native GPU runtime UI overlay",
    "textured UI sprite atlas",
    "production text/font/image/atlas/accessibility remains unsupported",
    "public native or RHI handle access remains unsupported",
    "general production renderer quality remains unsupported"
)) {
    Assert-ContainsText $sample3dManifestText $needle $sample3dManifestPath
}
if ($sample3dManifestText.Contains("native GPU HUD or sprite overlay output remains unsupported")) {
    Write-Error "$sample3dManifestPath keeps a stale native GPU HUD or sprite overlay unsupported claim"
}
Assert-ContainsText $sample3dManifestText "runtime/assets/desktop_runtime/hud.uiatlas" $sample3dManifestPath
$sample3dUiAtlasText = Get-AgentSurfaceText "games/sample_desktop_runtime_game/runtime/assets/desktop_runtime/hud.uiatlas"
foreach ($needle in @(
    "format=GameEngine.UiAtlas.v1",
    "source.decoding=unsupported",
    "atlas.packing=unsupported",
    "page.count=1",
    "image.count=1"
)) {
    Assert-ContainsText $sample3dUiAtlasText $needle "games/sample_desktop_runtime_game/runtime/assets/desktop_runtime/hud.uiatlas"
}
$sample3dIndexText = Get-AgentSurfaceText "games/sample_desktop_runtime_game/runtime/sample_desktop_runtime_game.geindex"
Assert-ContainsText $sample3dIndexText "kind=ui_atlas" "games/sample_desktop_runtime_game/runtime/sample_desktop_runtime_game.geindex"
Assert-ContainsText $sample3dIndexText "kind=ui_atlas_texture" "games/sample_desktop_runtime_game/runtime/sample_desktop_runtime_game.geindex"
Assert-ContainsText $sample3dIndexText "kind=animation_quaternion_clip" "games/sample_desktop_runtime_game/runtime/sample_desktop_runtime_game.geindex"
$sample3dMainText = Get-AgentSurfaceText "games/sample_desktop_runtime_game/main.cpp"
foreach ($needle in @(
    "mirakana/ui/ui.hpp",
    "mirakana/ui_renderer/ui_renderer.hpp",
    "mirakana/animation/skeleton.hpp",
    "--require-native-ui-overlay",
    "--require-native-ui-textured-sprite-atlas",
    "--require-quaternion-animation",
    "runtime_animation_quaternion_clip_payload",
    "make_animation_joint_tracks_3d_from_f32_bytes",
    "sample_animation_local_pose_3d",
    "sample_and_apply_runtime_scene_render_animation_pose_3d",
    "quaternion_animation=",
    "quaternion_animation_ticks=",
    "quaternion_animation_tracks=",
    "quaternion_animation_failures=",
    "quaternion_animation_scene_applied=",
    "quaternion_animation_scene_rotation_z=",
    "plan_scene_mesh_draws",
    "camera_primary=",
    "camera_controller_ticks=",
    "scene_mesh_plan_draws=",
    "scene_mesh_plan_unique_materials=",
    "scene_mesh_plan_diagnostics=",
    "hud_boxes=",
    "ui_overlay_requested=",
    "ui_overlay_status=",
    "ui_overlay_ready=",
    "ui_overlay_sprites_submitted=",
    "ui_overlay_draws=",
    "ui_texture_overlay_requested=",
    "ui_texture_overlay_status=",
    "ui_texture_overlay_atlas_ready=",
    "ui_texture_overlay_sprites_submitted=",
    "ui_texture_overlay_texture_binds=",
    "ui_texture_overlay_draws=",
    "ui_atlas_metadata_status=",
    "ui_atlas_metadata_pages=",
    "ui_atlas_metadata_bindings=",
    "--require-renderer-quality-gates",
    "renderer_quality_status=",
    "renderer_quality_ready=",
    "renderer_quality_diagnostics=",
    "renderer_quality_expected_framegraph_passes=",
    "renderer_quality_framegraph_passes_ok=",
    "renderer_quality_framegraph_execution_budget_ok=",
    "renderer_quality_scene_gpu_ready=",
    "renderer_quality_postprocess_ready=",
    "renderer_quality_postprocess_depth_input_ready=",
    "renderer_quality_directional_shadow_ready=",
    "renderer_quality_directional_shadow_filter_ready=",
    "primary_camera_seen_",
    "hud_boxes_submitted_"
)) {
    Assert-ContainsText $sample3dMainText $needle "games/sample_desktop_runtime_game/main.cpp"
}
$sceneRendererHeaderText = Get-AgentSurfaceText "engine/scene_renderer/include/mirakana/scene_renderer/scene_renderer.hpp"
foreach ($needle in @(
    "SceneMeshDrawPlan",
    "SceneMeshDrawPlanDiagnosticCode",
    "plan_scene_mesh_draws"
)) {
    Assert-ContainsText $sceneRendererHeaderText $needle "engine/scene_renderer/include/mirakana/scene_renderer/scene_renderer.hpp"
}
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
Assert-ContainsText $planRegistryText "3D Scene Mesh Package Telemetry v1" "docs/superpowers/plans/README.md"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
Assert-ContainsText $currentCapabilitiesText "3D Scene Mesh Package Telemetry v1" "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
Assert-ContainsText $roadmapText "3D Scene Mesh Package Telemetry v1" "docs/roadmap.md"
$engineManifestText = Get-AgentSurfaceText "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "prepare-worktree.ps1" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "normalized-configure-environment" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "normalized-build-environment" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "PATH/Path variants" "engine/agent/manifest.json"
foreach ($needle in @(
    "3d-scene-mesh-package-telemetry",
    "plan_scene_mesh_draws",
    "scene_mesh_plan_*"
)) {
    Assert-ContainsText $engineManifestText $needle "engine/agent/manifest.json"
}
foreach ($unsupportedClaim in @(
    "broad generated 3D game production readiness",
    "skeletal animation production path",
    "GPU skinning",
    "material graph",
    "shader graph",
    "package streaming",
    "Metal ready",
    "public native or RHI handle access",
    "general production renderer quality"
)) {
    Assert-ContainsText $engineManifestText $unsupportedClaim "engine/agent/manifest.json"
}

Get-ChildItem -Path (Resolve-RequiredAgentPath ".agents/skills") -Recurse -Filter "SKILL.md" | ForEach-Object {
    Assert-SkillFrontmatter $_.FullName
}

Get-ChildItem -Path (Resolve-RequiredAgentPath ".claude/skills") -Recurse -Filter "SKILL.md" | ForEach-Object {
    Assert-SkillFrontmatter $_.FullName
}

foreach ($requiredCodexSkill in @(
    ".agents/skills/cmake-build-system/SKILL.md",
    ".agents/skills/cpp-engine-debugging/SKILL.md",
    ".agents/skills/editor-change/SKILL.md",
    ".agents/skills/gameengine-agent-integration/SKILL.md",
    ".agents/skills/gameengine-feature/SKILL.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".agents/skills/license-audit/SKILL.md",
    ".agents/skills/rendering-change/SKILL.md"
)) {
    Resolve-RequiredAgentPath $requiredCodexSkill | Out-Null
}

foreach ($requiredClaudeSkill in @(
    ".claude/skills/gameengine-agent-integration/SKILL.md",
    ".claude/skills/gameengine-cmake-build-system/SKILL.md",
    ".claude/skills/gameengine-debugging/SKILL.md",
    ".claude/skills/gameengine-editor/SKILL.md",
    ".claude/skills/gameengine-feature/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-license-audit/SKILL.md",
    ".claude/skills/gameengine-rendering/SKILL.md"
)) {
    Resolve-RequiredAgentPath $requiredClaudeSkill | Out-Null
}

foreach ($quaternionPackageSmokeGuidance in @(
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md",
    ".codex/agents/gameplay-builder.toml",
    ".claude/agents/gameplay-builder.md",
    ".codex/agents/engine-architect.toml",
    ".claude/agents/engine-architect.md"
)) {
    $quaternionPackageSmokeGuidanceText = Get-AgentSurfaceText $quaternionPackageSmokeGuidance
    Assert-ContainsText $quaternionPackageSmokeGuidanceText "quaternion package smoke is limited to cooked" $quaternionPackageSmokeGuidance
    Assert-ContainsText $quaternionPackageSmokeGuidanceText "sample_animation_local_pose_3d" $quaternionPackageSmokeGuidance
    Assert-DoesNotContainText $quaternionPackageSmokeGuidanceText "generated-game quaternion animation package smoke" $quaternionPackageSmokeGuidance
}

foreach ($packageStreamingSmokeGuidance in @(
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md"
)) {
    $packageStreamingSmokeGuidanceText = Get-AgentSurfaceText $packageStreamingSmokeGuidance
    Assert-ContainsText $packageStreamingSmokeGuidanceText "--require-package-streaming-safe-point" $packageStreamingSmokeGuidance
    Assert-ContainsText $packageStreamingSmokeGuidanceText "execute_selected_runtime_package_streaming_safe_point" $packageStreamingSmokeGuidance
    Assert-ContainsText $packageStreamingSmokeGuidanceText "package_streaming_status" $packageStreamingSmokeGuidance
    Assert-ContainsText $packageStreamingSmokeGuidanceText "broad async/background streaming" $packageStreamingSmokeGuidance
    Assert-ContainsText $packageStreamingSmokeGuidanceText "--require-renderer-quality-gates" $packageStreamingSmokeGuidance
    Assert-ContainsText $packageStreamingSmokeGuidanceText "evaluate_sdl_desktop_presentation_quality_gate" $packageStreamingSmokeGuidance
    Assert-ContainsText $packageStreamingSmokeGuidanceText "renderer_quality_expected_framegraph_passes=2" $packageStreamingSmokeGuidance
    Assert-ContainsText $packageStreamingSmokeGuidanceText "depth-input postprocess" $packageStreamingSmokeGuidance
    Assert-ContainsText $packageStreamingSmokeGuidanceText "--require-playable-3d-slice" $packageStreamingSmokeGuidance
    Assert-ContainsText $packageStreamingSmokeGuidanceText "playable_3d_status=ready" $packageStreamingSmokeGuidance
    Assert-ContainsText $packageStreamingSmokeGuidanceText "broad generated 3D production readiness" $packageStreamingSmokeGuidance
}

foreach ($featureSkill in @(
    ".agents/skills/gameengine-feature/SKILL.md",
    ".claude/skills/gameengine-feature/SKILL.md"
)) {
    $featureSkillText = Get-AgentSurfaceText $featureSkill
    Assert-ContainsText $featureSkillText "C++23" $featureSkill
    Assert-ContainsText $featureSkillText "docs/README.md" $featureSkill
    Assert-ContainsText $featureSkillText "docs/superpowers/plans/README.md" $featureSkill
    Assert-ContainsText $featureSkillText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1" $featureSkill
    Assert-ContainsText $featureSkillText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1" $featureSkill
    Assert-ContainsText $featureSkillText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1" $featureSkill
}

foreach ($cmakeSkill in @(
    ".agents/skills/cmake-build-system/SKILL.md",
    ".claude/skills/gameengine-cmake-build-system/SKILL.md"
)) {
    $cmakeSkillText = Get-AgentSurfaceText $cmakeSkill
    Assert-ContainsText $cmakeSkillText "target_compile_features" $cmakeSkill
    Assert-ContainsText $cmakeSkillText "FILE_SET CXX_MODULES" $cmakeSkill
    Assert-ContainsText $cmakeSkillText "MK_MSVC_CXX23_STANDARD_OPTION" $cmakeSkill
    Assert-ContainsText $cmakeSkillText "PACKAGE_FILES_FROM_MANIFEST" $cmakeSkill
    Assert-ContainsText $cmakeSkillText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1" $cmakeSkill
    Assert-ContainsText $cmakeSkillText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1" $cmakeSkill
    Assert-ContainsText $cmakeSkillText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1" $cmakeSkill
    Assert-ContainsText $cmakeSkillText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1 -RequireDirectCMake" $cmakeSkill
    Assert-ContainsText $cmakeSkillText "normalized-configure-environment" $cmakeSkill
    Assert-ContainsText $cmakeSkillText "normalized-build-environment" $cmakeSkill
    Assert-ContainsText $cmakeSkillText 'child `PATH`' $cmakeSkill
    Assert-ContainsText $cmakeSkillText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1" $cmakeSkill
    Assert-ContainsText $cmakeSkillText "direct-clang-format-status" $cmakeSkill
    Assert-ContainsText $cmakeSkillText "CMake File API" $cmakeSkill
    Assert-ContainsText $cmakeSkillText "HeaderFilterRegex" $cmakeSkill
    Assert-ContainsText $cmakeSkillText "--warnings-as-errors=*" $cmakeSkill
    Assert-ContainsText $cmakeSkillText "NN warnings generated." $cmakeSkill
    Assert-ContainsText $cmakeSkillText "-Jobs 0" $cmakeSkill
    Assert-ContainsText $cmakeSkillText "VCPKG_MANIFEST_INSTALL=OFF" $cmakeSkill
    Assert-ContainsText $cmakeSkillText 'VCPKG_INSTALLED_DIR=${sourceDir}/vcpkg_installed' $cmakeSkill
    Assert-ContainsText $cmakeSkillText "VCPKG_MANIFEST_FEATURES" $cmakeSkill
    Assert-ContainsText $cmakeSkillText "lcov --ignore-errors unused" $cmakeSkill
    Assert-ContainsText $cmakeSkillText "tools/check-coverage-thresholds.ps1" $cmakeSkill
    Assert-ContainsText $cmakeSkillText "Debugging Tools for Windows" $cmakeSkill
    Assert-ContainsText $cmakeSkillText "PIX on Windows" $cmakeSkill
    Assert-ContainsText $cmakeSkillText "Windows Performance Toolkit" $cmakeSkill
}

foreach ($debuggingSkill in @(
    ".agents/skills/cpp-engine-debugging/SKILL.md",
    ".claude/skills/gameengine-debugging/SKILL.md"
)) {
    $debuggingSkillText = Get-AgentSurfaceText $debuggingSkill
    Assert-ContainsText $debuggingSkillText "Debugging Tools for Windows" $debuggingSkill
    Assert-ContainsText $debuggingSkillText "cdb -version" $debuggingSkill
    Assert-ContainsText $debuggingSkillText "_NT_SYMBOL_PATH" $debuggingSkill
}

foreach ($renderingSkill in @(
    ".agents/skills/rendering-change/SKILL.md",
    ".claude/skills/gameengine-rendering/SKILL.md"
)) {
    $renderingSkillText = Get-AgentSurfaceText $renderingSkill
    Assert-ContainsText $renderingSkillText "Windows Graphics Tools" $renderingSkill
    Assert-ContainsText $renderingSkillText "d3d12SDKLayers.dll" $renderingSkill
    Assert-ContainsText $renderingSkillText "PIX on Windows" $renderingSkill
    Assert-ContainsText $renderingSkillText "Windows Performance Toolkit" $renderingSkill
}

foreach ($licenseSkill in @(
    ".agents/skills/license-audit/SKILL.md",
    ".claude/skills/gameengine-license-audit/SKILL.md"
)) {
    $licenseSkillText = Get-AgentSurfaceText $licenseSkill
    Assert-ContainsText $licenseSkillText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1" $licenseSkill
    Assert-ContainsText $licenseSkillText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1" $licenseSkill
}

foreach ($agentIntegrationSkill in @(
    ".agents/skills/gameengine-agent-integration/SKILL.md",
    ".claude/skills/gameengine-agent-integration/SKILL.md"
)) {
    $agentIntegrationSkillText = Get-AgentSurfaceText $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "docs/README.md" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "docs/superpowers/plans/README.md" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "tools/static-contract-ledger.ps1" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "thin static-contract ledger entrypoints" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "agent-surface drift check" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "targeted drift checks" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "Context7" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/remove-merged-worktree.ps1" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1 -RequireDirectCMake" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "normalized-configure-environment" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "normalized-build-environment" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "direct-clang-format-status" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "CMake File API" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "VCPKG_MANIFEST_INSTALL=OFF" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "phase-gated milestone plan" $agentIntegrationSkill
    foreach ($planVolumeNeedle in @("live plan stack shallow", "active gap burn-down or milestone", "behavior/API/validation boundary", "validation-only follow-up", "historical implementation evidence")) {
        Assert-ContainsText $agentIntegrationSkillText $planVolumeNeedle $agentIntegrationSkill
    }
    foreach ($productionPromptNeedle in @("currentActivePlan", "recommendedNextPlan", "unsupportedProductionGaps", "clean breaking greenfield designs", "official documentation", "focused validation")) {
        Assert-ContainsText $agentIntegrationSkillText $productionPromptNeedle $agentIntegrationSkill
    }
    Assert-ContainsText $agentIntegrationSkillText ".codex/rules" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText ".claude/settings.json" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText ".claude/settings.local.json" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText ".mcp.json" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "AGENTS.override.md" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "specific, concise, verifiable" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "machine-readable status claims" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "OpenAI developer documentation MCP" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "official Anthropic documentation" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "Windows host diagnostics guidance" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "Debugging Tools for Windows" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "PIX on Windows" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "Git/GitHub publishing workflow changes" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "merge/delete-branch" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "auto-merge registration" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "official GitHub Flow" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "final completion report must not stop after local validation" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "Direct default-branch pushes are forbidden" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText 'pending-only `UNSTABLE` / `BLOCKED`' $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "mergeStateStatus" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "--match-head-commit <headRefOid>" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "Hosted PR failure hardening" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "CI check selection changes" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "path-filtered required checks" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "always-running aggregate gate" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "Docs/agent/rules/subagent-only changes run formatting plus agent/static guards" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "docs/agent-only PRs use lightweight static validation" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "Static-analysis drift includes" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "HeaderFilterRegex" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "NN warnings generated." $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "not troubleshooting playbooks" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "post-merge remote-tracking cleanup" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "post-merge worktree cleanup" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "policy reload" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "GITHUB_TOKEN" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "credential-manager-core" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "approval-capable session" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "Codex app Worktree/Handoff" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText "isolation: worktree" $agentIntegrationSkill
    Assert-ContainsText $agentIntegrationSkillText 'worktree.baseRef = "head"' $agentIntegrationSkill
}

$gitignoreText = Get-AgentSurfaceText ".gitignore"
Assert-ContainsText $gitignoreText ".worktrees/" ".gitignore"
Assert-ContainsText $gitignoreText ".claude/worktrees/" ".gitignore"

$codexRuleFile = Resolve-RequiredAgentPath ".codex/rules/gameengine.rules"
$codexRuleText = Get-Content -LiteralPath $codexRuleFile -Raw
Assert-ContainsText $codexRuleText "prefix_rule" ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRuleText 'decision = "prompt"' ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRuleText 'decision = "forbidden"' ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRuleText "match =" ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRuleText "not_match =" ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRuleText "git commit" ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRuleText "git push" ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRuleText "git push origin main" ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRuleText "git push -u origin main" ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRuleText "git push origin HEAD:main" ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRuleText "git push --force origin main" ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRuleText "GitHub Flow" ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRuleText "git push --force" ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRuleText "policy reload" ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRuleText "git restore" ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRuleText "git checkout" ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRuleText "git worktree remove" ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRuleText "git worktree prune" ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRuleText "tools/remove-merged-worktree.ps1" ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRuleText 'decision = "allow"' ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRuleText "gh pr view" ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRuleText "gh pr create" ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRuleText "gh pr merge" ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRuleText "gh pr merge --merge --delete-branch" ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRuleText "gh pr merge --auto --merge --delete-branch" ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRuleText "--match-head-commit" ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRuleText "Remove-Item" ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRuleText "Invoke-WebRequest" ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRuleText "Invoke-RestMethod" ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRuleText "Add-WindowsCapability" ".codex/rules/gameengine.rules"
Assert-ContainsText $codexRuleText "msiexec" ".codex/rules/gameengine.rules"

$claudeSettingsFile = Resolve-RequiredAgentPath ".claude/settings.json"
$claudeSettings = Get-Content -LiteralPath $claudeSettingsFile -Raw | ConvertFrom-Json
if (-not $claudeSettings.PSObject.Properties.Name.Contains('$schema')) {
    Write-Error ".claude/settings.json must include the official schema"
}
if (-not $claudeSettings.PSObject.Properties.Name.Contains("permissions")) {
    Write-Error ".claude/settings.json must define permissions"
}
if (-not $claudeSettings.PSObject.Properties.Name.Contains("worktree") -or $claudeSettings.worktree.baseRef -ne "head") {
    Write-Error ".claude/settings.json must set worktree.baseRef to head for project subagent worktree isolation"
}
foreach ($allowRule in @("Bash(gh pr view:*)", "Bash(gh pr create:*)", "Bash(gh pr merge --auto --merge --delete-branch:*)", "Bash(pwsh -NoProfile -ExecutionPolicy Bypass -File tools/remove-merged-worktree.ps1:*)")) {
    if (@($claudeSettings.permissions.allow) -notcontains $allowRule) {
        Write-Error ".claude/settings.json permissions.allow missing $allowRule"
    }
}
foreach ($askRule in @("Bash(git push --force:*)", "Bash(git push --force-with-lease:*)", "Bash(git restore:*)", "Bash(git checkout:*)", "Bash(git worktree remove:*)", "Bash(git worktree prune:*)", "Bash(git worktree repair:*)", "Bash(gh pr edit:*)", "Bash(gh pr merge --merge:*)", "Bash(gh pr merge --squash:*)", "Bash(gh pr merge --rebase:*)", "Bash(gh pr merge --admin:*)", "Bash(gh pr ready:*)", "Bash(gh pr close:*)", "Bash(gh pr reopen:*)", "Bash(Remove-Item:*)", "Bash(pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1:*)", "Bash(pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-android-release-package.ps1:*)", "Bash(curl:*)", "Bash(Invoke-WebRequest:*)", "Bash(Invoke-RestMethod:*)", "Bash(Add-WindowsCapability:*)", "Bash(dism:*)", "Bash(msiexec:*)")) {
    if (@($claudeSettings.permissions.ask) -notcontains $askRule) {
        Write-Error ".claude/settings.json permissions.ask missing $askRule"
    }
}
foreach ($automaticGitRule in @("Bash(git commit:*)", "Bash(git push:*)", "Bash(gh pr view:*)", "Bash(gh pr create:*)", "Bash(gh pr merge --auto --merge --delete-branch:*)", "Bash(pwsh -NoProfile -ExecutionPolicy Bypass -File tools/remove-merged-worktree.ps1:*)")) {
    if (@($claudeSettings.permissions.ask) -contains $automaticGitRule) {
        Write-Error ".claude/settings.json permissions.ask should not prompt routine automatic checkpoint command $automaticGitRule"
    }
}
foreach ($denyRule in @("Bash(git push origin main:*)", "Bash(git push origin master:*)", "Bash(git push -u origin main:*)", "Bash(git push -u origin master:*)", "Bash(git push --set-upstream origin main:*)", "Bash(git push --set-upstream origin master:*)", "Bash(git push --force origin main:*)", "Bash(git push --force origin master:*)", "Bash(git push --force-with-lease origin main:*)", "Bash(git push --force-with-lease origin master:*)", "Bash(git push origin HEAD:main:*)", "Bash(git push origin HEAD:master:*)", "Read(./.env)", "Read(./.env.*)", "Read(./.mcp.json)", "Read(./secrets/**)", "Read(./.claude/settings.local.json)", "Read(./**/*.p12)")) {
    if (@($claudeSettings.permissions.deny) -notcontains $denyRule) {
        Write-Error ".claude/settings.json permissions.deny missing $denyRule"
    }
}

foreach ($ruleFile in @(
    ".claude/rules/ai-agent-integration.md",
    ".claude/rules/cpp-engine.md"
)) {
    $ruleText = Get-AgentSurfaceText $ruleFile
    Assert-ContainsText $ruleText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1" $ruleFile
    Assert-ContainsText $ruleText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1" $ruleFile
    Assert-ContainsText $ruleText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1" $ruleFile
    Assert-ContainsText $ruleText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1 -RequireDirectCMake" $ruleFile
    Assert-ContainsText $ruleText "normalized-configure-environment" $ruleFile
    Assert-ContainsText $ruleText "normalized-build-environment" $ruleFile
    Assert-ContainsText $ruleText "direct-clang-format-status" $ruleFile
    Assert-ContainsText $ruleText "VCPKG_MANIFEST_INSTALL=OFF" $ruleFile
    Assert-ContainsText $ruleText "Debugging Tools for Windows" $ruleFile
    Assert-ContainsText $ruleText "Windows Graphics Tools" $ruleFile
}

$aiAgentRuleText = Get-AgentSurfaceText ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText "phase-gated milestone plan" ".claude/rules/ai-agent-integration.md"
foreach ($planVolumeNeedle in @("live plan stack shallow", "active gap burn-down or milestone", "behavior/API/validation boundary", "validation-only follow-up", "completed plan evidence")) {
    Assert-ContainsText $aiAgentRuleText $planVolumeNeedle ".claude/rules/ai-agent-integration.md"
}
Assert-ContainsText $aiAgentRuleText ".codex/rules" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText "policy reload" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText "GITHUB_TOKEN" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText "credential-manager-core" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText "approval-capable session" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText "Codex app Worktree/Handoff" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText "isolation: worktree" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/remove-merged-worktree.ps1" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText ".claude/settings.json" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText ".claude/settings.local.json" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText ".mcp.json" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText "AGENTS.override.md" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText "specific, concise, verifiable" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText "MCP connection state" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText "agent-surface drift check" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText "no durable guidance changed" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText "PR validation cost proportional to risk" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText "docs, skills, rules, settings, subagents" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText "lightweight static validation" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText "OpenAI developer documentation MCP" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText "official Anthropic docs" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText "mergeStateStatus" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText "--match-head-commit <headRefOid>" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText "final completion report must not stop after local validation" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText "Rules/permissions stay narrow command gates" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText "hosted PR check failure" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText "HeaderFilterRegex" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText "NN warnings generated." ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText "Debugging Tools for Windows" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText "Windows Graphics Tools" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText "PIX on Windows" ".claude/rules/ai-agent-integration.md"
Assert-ContainsText $aiAgentRuleText "Windows Performance Toolkit" ".claude/rules/ai-agent-integration.md"

Get-ChildItem -Path (Resolve-RequiredAgentPath ".claude/agents") -Filter "*.md" | ForEach-Object {
    Assert-ClaudeAgentFrontmatter $_.FullName
}

Get-ChildItem -Path (Resolve-RequiredAgentPath ".codex/agents") -Filter "*.toml" | ForEach-Object {
    $content = Get-Content -LiteralPath $_.FullName -Raw
    foreach ($field in @("name", "description", "developer_instructions")) {
        if ($content -notmatch "(?m)^$field\s*=") {
            Write-Error "Codex agent missing required field '$field': $($_.FullName)"
        }
    }
}

foreach ($relativePath in @(
    ".codex/agents/build-fixer.toml",
    ".codex/agents/cpp-reviewer.toml",
    ".codex/agents/engine-architect.toml",
    ".codex/agents/explorer.toml",
    ".codex/agents/gameplay-builder.toml",
    ".codex/agents/rendering-auditor.toml",
    ".claude/agents/build-fixer.md",
    ".claude/agents/cpp-reviewer.md",
    ".claude/agents/engine-architect.md",
    ".claude/agents/explorer.md",
    ".claude/agents/gameplay-builder.md",
    ".claude/agents/rendering-auditor.md"
)) {
    Assert-ContainsText (Get-AgentSurfaceText $relativePath) "register auto-merge" $relativePath
    Assert-ContainsText (Get-AgentSurfaceText $relativePath) "stale or missing agent guidance" $relativePath
}

foreach ($relativePath in @(
    ".codex/agents/build-fixer.toml",
    ".codex/agents/gameplay-builder.toml",
    ".claude/agents/build-fixer.md",
    ".claude/agents/gameplay-builder.md"
)) {
    Assert-ContainsText (Get-AgentSurfaceText $relativePath) "same parent task can finish synchronization before completion" $relativePath
}

foreach ($relativePath in @(
    ".codex/agents/cpp-reviewer.toml",
    ".codex/agents/explorer.toml",
    ".codex/agents/engine-architect.toml",
    ".codex/agents/rendering-auditor.toml"
)) {
    Assert-CodexReadOnlyAgent $relativePath
}

foreach ($relativePath in @(
    ".claude/agents/cpp-reviewer.md",
    ".claude/agents/explorer.md",
    ".claude/agents/engine-architect.md",
    ".claude/agents/rendering-auditor.md"
)) {
    Assert-ClaudeReadOnlyAgent $relativePath
}

foreach ($relativePath in @(
    ".claude/agents/build-fixer.md",
    ".claude/agents/gameplay-builder.md"
)) {
    Assert-ContainsText (Get-AgentSurfaceText $relativePath) "isolation: worktree" $relativePath
}

foreach ($buildFixerAgent in @(
    ".codex/agents/build-fixer.toml",
    ".claude/agents/build-fixer.md"
)) {
    $buildFixerText = Get-AgentSurfaceText $buildFixerAgent
    Assert-ContainsText $buildFixerText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1" $buildFixerAgent
    Assert-ContainsText $buildFixerText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1" $buildFixerAgent
    Assert-ContainsText $buildFixerText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1" $buildFixerAgent
    Assert-ContainsText $buildFixerText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1 -RequireDirectCMake" $buildFixerAgent
    Assert-ContainsText $buildFixerText "normalized-configure-environment" $buildFixerAgent
    Assert-ContainsText $buildFixerText "normalized-build-environment" $buildFixerAgent
    Assert-ContainsText $buildFixerText 'CL.exe` command-line switch error' $buildFixerAgent
    Assert-ContainsText $buildFixerText "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1" $buildFixerAgent
    Assert-ContainsText $buildFixerText "direct-clang-format-status" $buildFixerAgent
    Assert-ContainsText $buildFixerText "CMake File API" $buildFixerAgent
    Assert-ContainsText $buildFixerText "CreateFileW stdin failed with 5" $buildFixerAgent
    Assert-ContainsText $buildFixerText "VCPKG_MANIFEST_INSTALL=OFF" $buildFixerAgent
    Assert-ContainsText $buildFixerText "latest PR head SHA" $buildFixerAgent
    Assert-ContainsText $buildFixerText "HeaderFilterRegex" $buildFixerAgent
    Assert-ContainsText $buildFixerText "--warnings-as-errors=*" $buildFixerAgent
    Assert-ContainsText $buildFixerText "NN warnings generated." $buildFixerAgent
    Assert-ContainsText $buildFixerText "-Jobs 0" $buildFixerAgent
    Assert-ContainsText $buildFixerText "lcov --ignore-errors unused" $buildFixerAgent
    Assert-ContainsText $buildFixerText "runtime/.gitattributes" $buildFixerAgent
    Assert-ContainsText $buildFixerText "tools/static-contract-ledger.ps1" $buildFixerAgent
    Assert-ContainsText $buildFixerText "GitHub account billing/spending-limit" $buildFixerAgent
    Assert-ContainsText $buildFixerText "cdb -version" $buildFixerAgent
    Assert-ContainsText $buildFixerText "pixtool --help" $buildFixerAgent
    Assert-ContainsText $buildFixerText "Windows Graphics Tools" $buildFixerAgent
    Assert-ContainsText $buildFixerText "Windows Performance Toolkit" $buildFixerAgent
}

foreach ($explorerAgent in @(
    ".codex/agents/explorer.toml",
    ".claude/agents/explorer.md"
)) {
    $explorerAgentText = Get-AgentSurfaceText $explorerAgent
    Assert-ContainsText $explorerAgentText "docs/README.md" $explorerAgent
    Assert-ContainsText $explorerAgentText "docs/roadmap.md" $explorerAgent
    Assert-ContainsText $explorerAgentText "docs/superpowers/plans/README.md" $explorerAgent
    Assert-ContainsText $explorerAgentText "Do not edit files" $explorerAgent
    Assert-ContainsText $explorerAgentText "active milestone" $explorerAgent
}

foreach ($architectAgent in @(
    ".codex/agents/engine-architect.toml",
    ".claude/agents/engine-architect.md"
)) {
    $architectAgentText = Get-AgentSurfaceText $architectAgent
    Assert-ContainsText $architectAgentText "phase-gated milestone plan" $architectAgent
    Assert-ContainsText $architectAgentText "Runtime RHI Compute Morph Calibrated Overlap Diagnostics D3D12 v1" $architectAgent
    Assert-ContainsText $architectAgentText "Runtime RHI Compute Morph Submitted Overlap Diagnostics D3D12 v1" $architectAgent
    Assert-ContainsText $architectAgentText "Runtime RHI Compute Morph Vulkan Proof v1" $architectAgent
    Assert-ContainsText $architectAgentText "Runtime RHI Compute Morph Renderer Consumption Vulkan v1" $architectAgent
    Assert-ContainsText $architectAgentText "Runtime RHI Compute Morph NORMAL/TANGENT Output Vulkan v1" $architectAgent
    Assert-ContainsText $architectAgentText "MK_VULKAN_TEST_COMPUTE_MORPH_TANGENT_FRAME_SPV" $architectAgent
    Assert-ContainsText $architectAgentText "Generated 3D Compute Morph Package Smoke Vulkan v1" $architectAgent
}

foreach ($gameplayBuilderAgent in @(
    ".codex/agents/gameplay-builder.toml",
    ".claude/agents/gameplay-builder.md"
)) {
    $gameplayBuilderText = Get-AgentSurfaceText $gameplayBuilderAgent
    Assert-ContainsText $gameplayBuilderText "Runtime RHI Compute Morph NORMAL/TANGENT Output Vulkan v1" $gameplayBuilderAgent
    Assert-ContainsText $gameplayBuilderText "Generated 3D Compute Morph Package Smoke Vulkan v1" $gameplayBuilderAgent
    Assert-ContainsText $gameplayBuilderText "SdlDesktopPresentationVulkanSceneRendererDesc" $gameplayBuilderAgent
    Assert-ContainsText $gameplayBuilderText "--require-compute-morph" $gameplayBuilderAgent
}

foreach ($renderingAuditorAgent in @(
    ".codex/agents/rendering-auditor.toml",
    ".claude/agents/rendering-auditor.md"
)) {
    $renderingAuditorText = Get-AgentSurfaceText $renderingAuditorAgent
    Assert-ContainsText $renderingAuditorText "Windows Graphics Tools" $renderingAuditorAgent
    Assert-ContainsText $renderingAuditorText "PIX on Windows" $renderingAuditorAgent
    Assert-ContainsText $renderingAuditorText "Windows Performance Toolkit" $renderingAuditorAgent
    Assert-ContainsText $renderingAuditorText "D3D12 debug layer" $renderingAuditorAgent
    Assert-ContainsText $renderingAuditorText "Runtime RHI Compute Morph Calibrated Overlap Diagnostics D3D12 v1" $renderingAuditorAgent
    Assert-ContainsText $renderingAuditorText "Runtime RHI Compute Morph Submitted Overlap Diagnostics D3D12 v1" $renderingAuditorAgent
    Assert-ContainsText $renderingAuditorText "Runtime RHI Compute Morph Vulkan Proof v1" $renderingAuditorAgent
    Assert-ContainsText $renderingAuditorText "Runtime RHI Compute Morph Renderer Consumption Vulkan v1" $renderingAuditorAgent
    Assert-ContainsText $renderingAuditorText "Runtime RHI Compute Morph NORMAL/TANGENT Output Vulkan v1" $renderingAuditorAgent
    Assert-ContainsText $renderingAuditorText "MK_VULKAN_TEST_COMPUTE_MORPH_TANGENT_FRAME_SPV" $renderingAuditorAgent
    Assert-ContainsText $renderingAuditorText "Generated 3D Compute Morph Package Smoke Vulkan v1" $renderingAuditorAgent
    Assert-ContainsText $renderingAuditorText "output_tangent_usage" $renderingAuditorAgent
}

$contextJson = & (Join-Path $PSScriptRoot "agent-context.ps1")
$context = $contextJson | ConvertFrom-Json
if ($context.manifest.engine.name -ne "GameEngine") {
    Write-Error "agent-context output did not include the GameEngine manifest"
}
foreach ($field in @("publicHeaders", "moduleOwnership", "sampleGames", "assets", "platformTargets", "validationRecipes", "windowsDiagnosticsToolchain", "productionLoop", "productionRecipes", "aiCommandSurfaces", "unsupportedProductionGaps", "hostGates", "recommendedNextPlan", "codexRules", "claudeSettings")) {
    if (-not $context.PSObject.Properties.Name.Contains($field)) {
        Write-Error "agent-context output missing required section: $field"
    }
}
if ($context.windowsDiagnosticsToolchain.graphicsTools.source -ne "Windows optional capability Tools.Graphics.DirectX~~~~0.0.1.0") {
    Write-Error "agent-context windowsDiagnosticsToolchain did not expose Windows Graphics Tools capability"
}
if (@($context.codexRules) -notcontains ".codex/rules/gameengine.rules") {
    Write-Error "agent-context codexRules missing .codex/rules/gameengine.rules"
}
if (@($context.claudeSettings) -notcontains ".claude/settings.json") {
    Write-Error "agent-context claudeSettings missing .claude/settings.json"
}
foreach ($recipeId in $expectedProductionRecipeIds) {
    if (@($context.productionRecipes | Where-Object { $_.id -eq $recipeId }).Count -ne 1) {
        Write-Error "agent-context productionRecipes missing recipe id: $recipeId"
    }
}
foreach ($commandId in $expectedCommandSurfaceIds) {
    if (@($context.aiCommandSurfaces | Where-Object { $_.id -eq $commandId }).Count -ne 1) {
        Write-Error "agent-context aiCommandSurfaces missing command id: $commandId"
    }
}
if (-not $context.PSObject.Properties.Name.Contains("documentation")) {
    Write-Error "agent-context output missing required section: documentation"
}
foreach ($field in @("entrypoint", "currentStatus", "workflows", "planRegistry", "activeRoadmap")) {
    if (-not $context.documentation.PSObject.Properties.Name.Contains($field)) {
        Write-Error "agent-context documentation missing required field: $field"
    }
}
foreach ($module in $manifest.modules) {
    foreach ($header in $module.publicHeaders) {
        if ($context.publicHeaders -notcontains $header) {
            Write-Error "agent-context publicHeaders missing module header '$header' from module '$($module.name)'"
        }
    }
}
foreach ($module in $manifest.modules) {
    $ownedModule = @($context.moduleOwnership | Where-Object { $_.name -eq $module.name })
    if ($ownedModule.Count -ne 1) {
        Write-Error "agent-context moduleOwnership missing module '$($module.name)'"
    }
}
foreach ($target in $manifest.packagingTargets) {
    $contextTarget = @($context.platformTargets | Where-Object { $_.name -eq $target.name })
    if ($contextTarget.Count -ne 1) {
        Write-Error "agent-context platformTargets missing packaging target '$($target.name)'"
    }
}

$aiGameDevelopmentText = Get-AgentSurfaceText "docs/ai-game-development.md"
$aiIntegrationText = Get-AgentSurfaceText "docs/ai-integration.md"
$generatedScenariosText = Get-AgentSurfaceText "docs/specs/generated-game-validation-scenarios.md"
$promptPackText = Get-AgentSurfaceText "docs/specs/game-prompt-pack.md"
$handoffPromptText = Get-AgentSurfaceText "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md"
$architectureText = Get-AgentSurfaceText "docs/architecture.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$rhiText = Get-AgentSurfaceText "docs/rhi.md"
$authoredRuntimeWorkflowRequiredText = @(
    "validated authored-to-runtime workflow",
    "register-source-asset -> cook-registered-source-assets -> migrate-scene-v2-runtime-package -> mirakana::runtime::load_runtime_asset_package -> mirakana::runtime_scene::instantiate_runtime_scene"
)
foreach ($workflowDoc in @(
    @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
    @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
    @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
    @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
    @{ Text = $roadmapText; Label = "docs/roadmap.md" }
)) {
    foreach ($requiredText in $authoredRuntimeWorkflowRequiredText) {
        Assert-ContainsText $workflowDoc.Text $requiredText $workflowDoc.Label
    }
}
$runtimeScenePackageValidationRequiredText = @(
    "validate-runtime-scene-package",
    "plan_runtime_scene_package_validation",
    "execute_runtime_scene_package_validation",
    "non-mutating runtime scene package validation"
)
foreach ($runtimeSceneValidationDoc in @(
    @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
    @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
    @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
    @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
    @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
    @{ Text = $roadmapText; Label = "docs/roadmap.md" }
)) {
    foreach ($requiredText in $runtimeScenePackageValidationRequiredText) {
        Assert-ContainsText $runtimeSceneValidationDoc.Text $requiredText $runtimeSceneValidationDoc.Label
    }
}
foreach ($recipeId in $expectedProductionRecipeIds) {
    Assert-ContainsText $aiGameDevelopmentText $recipeId "docs/ai-game-development.md"
    Assert-ContainsText $generatedScenariosText $recipeId "docs/specs/generated-game-validation-scenarios.md"
    Assert-ContainsText $promptPackText $recipeId "docs/specs/game-prompt-pack.md"
    Assert-ContainsText $handoffPromptText $recipeId "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md"
}
foreach ($sceneSchemaDoc in @(
    @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
    @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
    @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
    @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" }
)) {
    Assert-ContainsText $sceneSchemaDoc.Text "Scene/Component/Prefab Schema v2" $sceneSchemaDoc.Label
    Assert-ContainsText $sceneSchemaDoc.Text "contract-only" $sceneSchemaDoc.Label
}
foreach ($assetResourceDoc in @(
    @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
    @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
    @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
    @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
    @{ Text = $architectureText; Label = "docs/architecture.md" },
    @{ Text = $roadmapText; Label = "docs/roadmap.md" }
)) {
    Assert-ContainsText $assetResourceDoc.Text "Asset Identity v2" $assetResourceDoc.Label
    Assert-ContainsText $assetResourceDoc.Text "Runtime Resource v2" $assetResourceDoc.Label
    Assert-ContainsText $assetResourceDoc.Text "foundation-only" $assetResourceDoc.Label
}
foreach ($rendererResourceDoc in @(
    @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
    @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
    @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
    @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
    @{ Text = $architectureText; Label = "docs/architecture.md" },
    @{ Text = $roadmapText; Label = "docs/roadmap.md" },
    @{ Text = $rhiText; Label = "docs/rhi.md" }
)) {
    Assert-ContainsText $rendererResourceDoc.Text "Renderer/RHI Resource Foundation v1" $rendererResourceDoc.Label
    Assert-ContainsText $rendererResourceDoc.Text "RhiResourceLifetimeRegistry" $rendererResourceDoc.Label
    Assert-ContainsText $rendererResourceDoc.Text "foundation-only" $rendererResourceDoc.Label
    Assert-ContainsText $rendererResourceDoc.Text "upload/staging" $rendererResourceDoc.Label
    Assert-ContainsText $rendererResourceDoc.Text "Frame Graph and Upload/Staging Foundation v1" $rendererResourceDoc.Label
    Assert-ContainsText $rendererResourceDoc.Text "RhiUploadStagingPlan" $rendererResourceDoc.Label
}
foreach ($commandSurfaceDoc in @(
    @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
    @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
    @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
    @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
    @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" }
)) {
    Assert-ContainsText $commandSurfaceDoc.Text "AI Command Surface" $commandSurfaceDoc.Label
    Assert-ContainsText $commandSurfaceDoc.Text "requestModes" $commandSurfaceDoc.Label
    Assert-ContainsText $commandSurfaceDoc.Text "requiredModules" $commandSurfaceDoc.Label
    Assert-ContainsText $commandSurfaceDoc.Text "capabilityGates" $commandSurfaceDoc.Label
    Assert-ContainsText $commandSurfaceDoc.Text "undoToken" $commandSurfaceDoc.Label
    Assert-ContainsText $commandSurfaceDoc.Text "register-runtime-package-files" $commandSurfaceDoc.Label
}
foreach ($validationRunnerDoc in @(
    @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
    @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
    @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
    @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" }
)) {
    Assert-ContainsText $validationRunnerDoc.Text "run-validation-recipe" $validationRunnerDoc.Label
    Assert-ContainsText $validationRunnerDoc.Text "tools/run-validation-recipe.ps1" $validationRunnerDoc.Label
    Assert-ContainsText $validationRunnerDoc.Text "allowlisted validation recipes" $validationRunnerDoc.Label
    Assert-ContainsText $validationRunnerDoc.Text "does not evaluate arbitrary shell" $validationRunnerDoc.Label
    Assert-ContainsText $validationRunnerDoc.Text "free-form validation commands are unsupported" $validationRunnerDoc.Label
}
foreach ($forbiddenValidationRunnerClaim in @(
    "run-validation-recipe can run arbitrary shell",
    "free-form validation commands are supported",
    "manifest command strings are evaluated",
    "raw manifest command eval is allowed",
    "run-validation-recipe provides broad CI orchestration"
)) {
    foreach ($doc in @(
        @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
        @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
        @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" }
    )) {
        if ($doc.Text.Contains($forbiddenValidationRunnerClaim)) {
            Write-Error "$($doc.Label) contains forbidden validation runner claim: $forbiddenValidationRunnerClaim"
        }
    }
}
foreach ($forbiddenScenePrefabAuthoringClaim in @(
    "Scene/Prefab v2 authoring makes Scene v2 runtime package migration ready",
    "Scene/Prefab v2 authoring alone makes Scene v2 package migration ready",
    "editor productization is ready",
    "nested prefab merge/resolution UX is ready",
    "arbitrary free-form scene edits are supported",
    "arbitrary free-form prefab edits are supported",
    "Scene/Prefab v2 authoring runs arbitrary shell"
)) {
    foreach ($doc in @(
        @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
        @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
        @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" }
    )) {
        if ($doc.Text.Contains($forbiddenScenePrefabAuthoringClaim)) {
            Write-Error "$($doc.Label) contains forbidden Scene/Prefab v2 authoring claim: $forbiddenScenePrefabAuthoringClaim"
        }
    }
}
foreach ($forbiddenSceneMigrationClaim in @(
    "migrate-scene-v2-runtime-package executes external importers",
    "Scene v2 runtime package migration executes external importers",
    "migrate-scene-v2-runtime-package cooks dependent assets",
    "Scene v2 runtime package migration cooks dependent assets",
    "migrate-scene-v2-runtime-package performs broad package cooking",
    "Scene v2 runtime package migration performs broad package cooking",
    "migrate-scene-v2-runtime-package makes renderer/RHI residency ready",
    "Scene v2 runtime package migration makes renderer/RHI residency ready",
    "migrate-scene-v2-runtime-package makes package streaming ready",
    "Scene v2 runtime package migration makes package streaming ready",
    "migrate-scene-v2-runtime-package supports material graphs",
    "Scene v2 runtime package migration supports material graphs",
    "migrate-scene-v2-runtime-package supports shader graphs",
    "Scene v2 runtime package migration supports shader graphs",
    "migrate-scene-v2-runtime-package supports live shader generation",
    "Scene v2 runtime package migration supports live shader generation",
    "migrate-scene-v2-runtime-package makes editor productization ready",
    "Scene v2 runtime package migration makes editor productization ready",
    "migrate-scene-v2-runtime-package makes Metal ready",
    "Scene v2 runtime package migration makes Metal ready",
    "migrate-scene-v2-runtime-package exposes public native/RHI handles",
    "Scene v2 runtime package migration exposes public native/RHI handles",
    "migrate-scene-v2-runtime-package makes general production renderer quality ready",
    "Scene v2 runtime package migration makes general production renderer quality ready"
)) {
    foreach ($doc in @(
        @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
        @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
        @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" }
    )) {
        if ($doc.Text.Contains($forbiddenSceneMigrationClaim)) {
            Write-Error "$($doc.Label) contains forbidden Scene v2 runtime package migration claim: $forbiddenSceneMigrationClaim"
        }
    }
}
foreach ($forbiddenSourceAssetRegistrationClaim in @(
    "source asset registration executes external importers",
    "register-source-asset executes external importers",
    "source asset registration cooks runtime packages",
    "register-source-asset cooks runtime packages",
    "source asset registration makes renderer/RHI residency ready",
    "source asset registration makes package streaming ready",
    "source asset registration supports material graphs",
    "source asset registration supports shader graphs",
    "source asset registration supports live shader generation",
    "source asset registration makes editor productization ready"
)) {
    foreach ($doc in @(
        @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
        @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
        @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" }
    )) {
        if ($doc.Text.Contains($forbiddenSourceAssetRegistrationClaim)) {
            Write-Error "$($doc.Label) contains forbidden source asset registration claim: $forbiddenSourceAssetRegistrationClaim"
        }
    }
}
foreach ($forbiddenRegisteredCookClaim in @(
    "cook-registered-source-assets performs broad dependency cooking",
    "cook-registered-source-assets cooks unselected dependencies",
    "cook-registered-source-assets makes renderer/RHI residency ready",
    "cook-registered-source-assets makes package streaming ready",
    "cook-registered-source-assets supports material graphs",
    "cook-registered-source-assets supports shader graphs",
    "cook-registered-source-assets supports live shader generation",
    "cook-registered-source-assets makes editor productization ready",
    "cook-registered-source-assets makes Metal ready",
    "cook-registered-source-assets exposes public native/RHI handles",
    "cook-registered-source-assets makes general production renderer quality ready",
    "cook-registered-source-assets executes arbitrary shell",
    "cook-registered-source-assets supports free-form edits"
)) {
    foreach ($doc in @(
        @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
        @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
        @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" }
    )) {
        if ($doc.Text.Contains($forbiddenRegisteredCookClaim)) {
            Write-Error "$($doc.Label) contains forbidden registered source asset cook/package claim: $forbiddenRegisteredCookClaim"
        }
    }
}
foreach ($forbiddenAuthoredRuntimeWorkflowClaim in @(
    "authored-to-runtime workflow performs broad package cooking",
    "authored-to-runtime workflow cooks unselected dependencies",
    "authored-to-runtime workflow parses source assets at runtime",
    "authored-to-runtime workflow makes renderer/RHI residency ready",
    "authored-to-runtime workflow makes package streaming ready",
    "authored-to-runtime workflow supports material graphs",
    "authored-to-runtime workflow supports shader graphs",
    "authored-to-runtime workflow supports live shader generation",
    "authored-to-runtime workflow makes editor productization ready",
    "authored-to-runtime workflow makes Metal ready",
    "authored-to-runtime workflow exposes public native/RHI handles",
    "authored-to-runtime workflow makes general production renderer quality ready"
)) {
    foreach ($doc in @(
        @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
        @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
        @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" }
    )) {
        if ($doc.Text.Contains($forbiddenAuthoredRuntimeWorkflowClaim)) {
            Write-Error "$($doc.Label) contains forbidden authored-to-runtime workflow claim: $forbiddenAuthoredRuntimeWorkflowClaim"
        }
    }
}
foreach ($forbiddenRuntimeSceneValidationClaim in @(
    "validate-runtime-scene-package performs package cooking",
    "validate-runtime-scene-package parses source assets at runtime",
    "validate-runtime-scene-package executes external importers",
    "validate-runtime-scene-package makes renderer/RHI residency ready",
    "validate-runtime-scene-package makes package streaming ready",
    "validate-runtime-scene-package supports material graphs",
    "validate-runtime-scene-package supports shader graphs",
    "validate-runtime-scene-package supports live shader generation",
    "validate-runtime-scene-package makes editor productization ready",
    "validate-runtime-scene-package makes Metal ready",
    "validate-runtime-scene-package exposes public native/RHI handles",
    "validate-runtime-scene-package makes general production renderer quality ready",
    "validate-runtime-scene-package executes arbitrary shell",
    "validate-runtime-scene-package supports free-form edits"
)) {
    foreach ($doc in @(
        @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
        @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
        @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" }
    )) {
        if ($doc.Text.Contains($forbiddenRuntimeSceneValidationClaim)) {
            Write-Error "$($doc.Label) contains forbidden runtime scene package validation claim: $forbiddenRuntimeSceneValidationClaim"
        }
    }
}
foreach ($staleScenarioClaim in @(
    "Native RHI-backed desktop game-window presentation requires",
    "Visible Vulkan draw/readback game proof requires Wave 3",
    "Installed SDK consumer validation requires Wave 10",
    "does not implement the production renderer, Scene v2",
    "future-2d-playable-vertical-slice",
    "2d-playable-source-tree`: planned",
    "2d-playable-source-tree`: host-gated",
    "future-2d-playable-vertical-slice`: ready",
    "future-3d-playable-vertical-slice`: ready",
    "3d-playable-desktop-package`: ready",
    "3d-playable-desktop-package makes runtime source asset parsing ready",
    "3d-playable-desktop-package makes material graph ready",
    "3d-playable-desktop-package makes skeletal animation ready",
    "3d-playable-desktop-package makes GPU skinning ready",
    "3d-playable-desktop-package makes package streaming ready",
    "3d-playable-desktop-package makes production renderer ready",
    "prefab variants no longer need nested prefab propagation/merge resolution follow-up work",
    "Asset Identity v2 and Runtime Resource v2 remain planned gaps",
    "Runtime Resource v2 makes renderer residency ready",
    "Asset Identity v2 makes 2D/3D playable vertical slices ready",
    "Renderer/RHI Resource Foundation remains planned",
    "Renderer/RHI Resource Foundation makes GPU allocator ready",
    "Renderer/RHI Resource Foundation makes upload/staging ready",
    "Renderer/RHI Resource Foundation makes Frame Graph v1 ready",
    "Renderer/RHI Resource Foundation makes 2D/3D playable vertical slices ready",
    "Frame Graph v1 remains planned",
    "Upload/Staging v1 remains planned",
    "Frame Graph v1 makes production renderer ready",
    "Upload/Staging v1 makes package streaming ready",
    "Frame Graph and Upload/Staging Foundation v1 makes 2D/3D playable vertical slices ready",
    "all command surfaces are ready",
    "scene command apply is ready",
    "AI command surfaces may use legacy dryRun",
    "AI Command Surface Foundation v1 makes 2D/3D playable vertical slices ready"
)) {
    foreach ($doc in @(
        @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
        @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
        @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" }
    )) {
        if ($doc.Text.Contains($staleScenarioClaim)) {
            Write-Error "$($doc.Label) contains stale unsupported claim: $staleScenarioClaim"
        }
    }
}

$headlessScaffoldRoot = New-ScaffoldCheckRoot
