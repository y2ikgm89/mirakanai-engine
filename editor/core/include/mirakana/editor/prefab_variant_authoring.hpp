// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/editor/history.hpp"
#include "mirakana/editor/io.hpp"
#include "mirakana/platform/file_dialog.hpp"
#include "mirakana/scene/prefab_overrides.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

enum class PrefabVariantAuthoringDiagnosticKind { invalid_variant, missing_asset, wrong_asset_kind };

struct PrefabVariantAuthoringDiagnostic {
    PrefabVariantAuthoringDiagnosticKind kind{PrefabVariantAuthoringDiagnosticKind::invalid_variant};
    std::uint32_t node_index{0};
    PrefabOverrideKind override_kind{PrefabOverrideKind::name};
    AssetId asset;
    std::string field;
    std::string diagnostic;
};

struct PrefabVariantOverrideRow {
    std::uint32_t node_index{0};
    PrefabOverrideKind kind{PrefabOverrideKind::name};
    std::string node_name;
    std::string kind_label;
    bool has_name{false};
    bool has_transform{false};
    bool has_components{false};
    bool has_camera{false};
    bool has_light{false};
    bool has_mesh_renderer{false};
    bool has_sprite_renderer{false};
    std::size_t diagnostic_count{0};
};

struct PrefabVariantAuthoringModel {
    std::string path;
    std::string name;
    std::vector<PrefabVariantOverrideRow> override_rows;
    std::vector<PrefabVariantAuthoringDiagnostic> diagnostics;
    bool dirty{false};

    [[nodiscard]] bool valid() const noexcept {
        return diagnostics.empty();
    }
};

enum class PrefabVariantConflictStatus { ready, warning, blocked };

enum class PrefabVariantConflictKind {
    clean,
    missing_node,
    source_node_mismatch,
    duplicate_override,
    invalid_override,
    redundant_override,
    component_family_replacement,
};

enum class PrefabVariantConflictResolutionKind { none, remove_override, retarget_override, accept_current_node };

struct PrefabVariantConflictBatchResolutionPlan {
    std::string id{"apply_all_reviewed_resolutions"};
    std::string label{"Apply All Reviewed Resolutions"};
    bool available{false};
    bool mutates{false};
    bool executes{false};
    std::size_t resolution_count{0};
    std::size_t blocking_resolution_count{0};
    std::size_t warning_resolution_count{0};
    std::vector<std::string> resolution_ids;
    std::string diagnostic;
};

struct PrefabVariantConflictRow {
    std::string id;
    std::size_t override_index{0};
    std::uint32_t node_index{0};
    PrefabOverrideKind override_kind{PrefabOverrideKind::name};
    PrefabVariantConflictStatus status{PrefabVariantConflictStatus::ready};
    PrefabVariantConflictKind conflict{PrefabVariantConflictKind::clean};
    std::string node_name;
    std::string override_kind_label;
    std::string status_label;
    std::string conflict_label;
    std::string base_value;
    std::string override_value;
    std::string diagnostic;
    bool resolution_available{false};
    std::string resolution_id;
    std::string resolution_label;
    std::string resolution_diagnostic;
    PrefabVariantConflictResolutionKind resolution_kind{PrefabVariantConflictResolutionKind::none};
    std::uint32_t resolution_target_node_index{0};
    std::string resolution_target_node_name;
    bool blocking{false};
};

struct PrefabVariantConflictReviewModel {
    PrefabVariantConflictStatus status{PrefabVariantConflictStatus::ready};
    std::string status_label;
    std::string variant_name;
    std::string base_prefab_name;
    bool can_compose{false};
    bool has_blocking_conflicts{false};
    bool mutates{false};
    bool executes{false};
    std::size_t blocking_count{0};
    std::size_t warning_count{0};
    std::vector<PrefabVariantConflictRow> rows;
    std::vector<std::string> diagnostics;
    PrefabVariantConflictBatchResolutionPlan batch_resolution;
};

struct PrefabVariantConflictResolutionResult {
    bool applied{false};
    bool valid_after_apply{false};
    PrefabVariantDefinition variant;
    std::string diagnostic;
};

struct PrefabVariantConflictBatchResolutionResult {
    bool applied{false};
    bool valid_after_apply{false};
    std::size_t applied_count{0};
    std::size_t remaining_available_count{0};
    std::size_t remaining_blocking_count{0};
    std::vector<std::string> applied_resolution_ids;
    PrefabVariantDefinition variant;
    std::string diagnostic;
};

enum class PrefabVariantBaseRefreshStatus { ready, warning, blocked };

enum class PrefabVariantBaseRefreshRowKind {
    preserve_index,
    retarget_by_source_name,
    missing_source_node_hint,
    missing_source_node,
    ambiguous_source_node,
    duplicate_target_override,
};

struct PrefabVariantBaseRefreshRow {
    std::string id;
    std::size_t override_index{0};
    std::uint32_t old_node_index{0};
    std::string old_node_name;
    std::string source_node_name;
    std::uint32_t refreshed_node_index{0};
    std::string refreshed_node_name;
    PrefabOverrideKind override_kind{PrefabOverrideKind::name};
    std::string override_kind_label;
    PrefabVariantBaseRefreshStatus status{PrefabVariantBaseRefreshStatus::ready};
    PrefabVariantBaseRefreshRowKind kind{PrefabVariantBaseRefreshRowKind::preserve_index};
    std::string status_label;
    std::string kind_label;
    std::string diagnostic;
    bool blocking{false};
};

struct PrefabVariantBaseRefreshPlan {
    PrefabVariantBaseRefreshStatus status{PrefabVariantBaseRefreshStatus::ready};
    std::string status_label;
    std::string variant_name;
    std::string current_base_prefab_name;
    std::string refreshed_base_prefab_name;
    bool can_apply{false};
    bool mutates{false};
    bool executes{false};
    std::size_t row_count{0};
    std::size_t retarget_count{0};
    std::size_t blocking_count{0};
    std::size_t warning_count{0};
    std::vector<PrefabVariantBaseRefreshRow> rows;
    std::vector<std::string> diagnostics;
};

struct PrefabVariantBaseRefreshResult {
    bool applied{false};
    bool valid_after_apply{false};
    std::size_t retargeted_count{0};
    PrefabVariantDefinition variant;
    std::string diagnostic;
};

enum class EditorPrefabVariantFileDialogMode { open, save };

struct EditorPrefabVariantFileDialogRow {
    std::string id;
    std::string label;
    std::string value;
};

struct EditorPrefabVariantFileDialogModel {
    EditorPrefabVariantFileDialogMode mode{EditorPrefabVariantFileDialogMode::open};
    bool accepted{false};
    std::string status_label;
    std::string selected_path;
    std::vector<EditorPrefabVariantFileDialogRow> rows;
    std::vector<std::string> diagnostics;
};

class PrefabVariantAuthoringDocument {
  public:
    [[nodiscard]] static PrefabVariantAuthoringDocument
    from_base_prefab(PrefabDefinition base_prefab, std::string variant_name, std::string path = {});
    [[nodiscard]] static PrefabVariantAuthoringDocument from_variant(PrefabVariantDefinition variant,
                                                                     std::string path = {});

    [[nodiscard]] const PrefabVariantDefinition& variant() const noexcept;
    [[nodiscard]] const PrefabDefinition& base_prefab() const noexcept;
    [[nodiscard]] std::string_view path() const noexcept;
    [[nodiscard]] bool dirty() const;
    [[nodiscard]] PrefabDefinition composed_prefab() const;
    [[nodiscard]] PrefabVariantAuthoringModel model() const;
    [[nodiscard]] PrefabVariantAuthoringModel model(const AssetRegistry& registry) const;

    [[nodiscard]] bool replace_variant(PrefabVariantDefinition variant);
    [[nodiscard]] bool set_name_override(std::uint32_t node_index, std::string name);
    [[nodiscard]] bool set_transform_override(std::uint32_t node_index, Transform3D transform);
    [[nodiscard]] bool set_component_override(std::uint32_t node_index, SceneNodeComponents components);
    void mark_saved();

  private:
    PrefabVariantDefinition variant_;
    std::string path_;
    std::string saved_variant_text_;
};

[[nodiscard]] std::vector<PrefabVariantAuthoringDiagnostic>
validate_prefab_variant_authoring_document(const PrefabVariantAuthoringDocument& document);
[[nodiscard]] std::vector<PrefabVariantAuthoringDiagnostic>
validate_prefab_variant_authoring_document(const PrefabVariantAuthoringDocument& document,
                                           const AssetRegistry& registry);
[[nodiscard]] std::string_view prefab_variant_conflict_status_label(PrefabVariantConflictStatus status) noexcept;
[[nodiscard]] std::string_view prefab_variant_conflict_kind_label(PrefabVariantConflictKind kind) noexcept;
[[nodiscard]] std::string_view
prefab_variant_conflict_resolution_kind_label(PrefabVariantConflictResolutionKind kind) noexcept;
[[nodiscard]] PrefabVariantConflictReviewModel
make_prefab_variant_conflict_review_model(const PrefabVariantDefinition& variant);
[[nodiscard]] PrefabVariantConflictReviewModel
make_prefab_variant_conflict_review_model(const PrefabVariantAuthoringDocument& document);
[[nodiscard]] mirakana::ui::UiDocument
make_prefab_variant_conflict_review_ui_model(const PrefabVariantConflictReviewModel& model);
[[nodiscard]] PrefabVariantConflictBatchResolutionPlan
make_prefab_variant_conflict_batch_resolution_plan(const PrefabVariantConflictReviewModel& model);
[[nodiscard]] PrefabVariantConflictResolutionResult
resolve_prefab_variant_conflict(const PrefabVariantDefinition& variant, std::string_view resolution_id);
[[nodiscard]] PrefabVariantConflictBatchResolutionResult
resolve_prefab_variant_conflicts(const PrefabVariantDefinition& variant);
[[nodiscard]] UndoableAction make_prefab_variant_conflict_resolution_action(PrefabVariantAuthoringDocument& document,
                                                                            std::string_view resolution_id);
[[nodiscard]] UndoableAction
make_prefab_variant_conflict_batch_resolution_action(PrefabVariantAuthoringDocument& document);
[[nodiscard]] std::string_view prefab_variant_base_refresh_status_label(PrefabVariantBaseRefreshStatus status) noexcept;
[[nodiscard]] std::string_view
prefab_variant_base_refresh_row_kind_label(PrefabVariantBaseRefreshRowKind kind) noexcept;
[[nodiscard]] PrefabVariantBaseRefreshPlan plan_prefab_variant_base_refresh(const PrefabVariantDefinition& variant,
                                                                            const PrefabDefinition& refreshed_base);
[[nodiscard]] PrefabVariantBaseRefreshPlan
plan_prefab_variant_base_refresh(const PrefabVariantAuthoringDocument& document,
                                 const PrefabDefinition& refreshed_base);
[[nodiscard]] mirakana::ui::UiDocument
make_prefab_variant_base_refresh_ui_model(const PrefabVariantBaseRefreshPlan& plan);
[[nodiscard]] PrefabVariantBaseRefreshResult apply_prefab_variant_base_refresh(const PrefabVariantDefinition& variant,
                                                                               const PrefabDefinition& refreshed_base);
[[nodiscard]] UndoableAction make_prefab_variant_base_refresh_action(PrefabVariantAuthoringDocument& document,
                                                                     const PrefabDefinition& refreshed_base);
[[nodiscard]] mirakana::FileDialogRequest
make_prefab_variant_open_dialog_request(std::string_view default_location = "assets/prefabs");
[[nodiscard]] mirakana::FileDialogRequest
make_prefab_variant_save_dialog_request(std::string_view default_location = "assets/prefabs/selected.prefabvariant");
[[nodiscard]] EditorPrefabVariantFileDialogModel
make_prefab_variant_open_dialog_model(const mirakana::FileDialogResult& result);
[[nodiscard]] EditorPrefabVariantFileDialogModel
make_prefab_variant_save_dialog_model(const mirakana::FileDialogResult& result);
[[nodiscard]] mirakana::ui::UiDocument
make_prefab_variant_file_dialog_ui_model(const EditorPrefabVariantFileDialogModel& model);

// Returned actions capture the document by reference; keep the owning UndoStack scoped to this document.
[[nodiscard]] UndoableAction make_prefab_variant_name_override_action(PrefabVariantAuthoringDocument& document,
                                                                      std::uint32_t node_index, std::string name);
[[nodiscard]] UndoableAction make_prefab_variant_transform_override_action(PrefabVariantAuthoringDocument& document,
                                                                           std::uint32_t node_index,
                                                                           Transform3D transform);
[[nodiscard]] UndoableAction make_prefab_variant_component_override_action(PrefabVariantAuthoringDocument& document,
                                                                           std::uint32_t node_index,
                                                                           SceneNodeComponents components);

void save_prefab_variant_authoring_document(ITextStore& store, std::string_view path,
                                            PrefabVariantAuthoringDocument& document);
void save_prefab_variant_authoring_document(ITextStore& store, std::string_view path,
                                            PrefabVariantAuthoringDocument& document, const AssetRegistry& registry);
[[nodiscard]] PrefabVariantAuthoringDocument load_prefab_variant_authoring_document(ITextStore& store,
                                                                                    std::string_view path);

} // namespace mirakana::editor
