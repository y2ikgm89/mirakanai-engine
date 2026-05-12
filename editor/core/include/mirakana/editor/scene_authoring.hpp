// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/editor/history.hpp"
#include "mirakana/editor/io.hpp"
#include "mirakana/editor/scene_edit.hpp"
#include "mirakana/platform/file_dialog.hpp"
#include "mirakana/scene/prefab.hpp"
#include "mirakana/scene/scene.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace mirakana::editor {

struct SceneAuthoringSceneNodeIdHash {
    [[nodiscard]] std::size_t operator()(SceneNodeId id) const noexcept {
        return std::hash<std::uint32_t>{}(id.value);
    }
};

enum class SceneAuthoringDiagnosticKind : std::uint8_t { missing_asset, wrong_asset_kind };

enum class ScenePrefabInstanceSourceLinkStatus : std::uint8_t { linked, stale };

enum class ScenePrefabInstanceRefreshStatus : std::uint8_t { ready, warning, blocked };

enum class ScenePrefabInstanceRefreshRowKind : std::uint8_t {
    preserve_node,
    add_source_node,
    remove_stale_node,
    keep_local_child,
    keep_stale_source_node_as_local,
    keep_nested_prefab_instance,
    invalid_instance_root,
    ambiguous_source_node,
    unsupported_local_child,
    unsupported_stale_source_subtree,
    unsupported_nested_prefab_instance,
};

struct ScenePrefabInstanceRefreshPolicy {
    bool keep_local_children{false};
    bool keep_stale_source_nodes_as_local{false};
    bool keep_nested_prefab_instances{false};
    /// When set, `plan_scene_prefab_instance_refresh` / `apply_scene_prefab_instance_refresh` may chain reviewed
    /// refreshes for `nested_prefab_propagation_preview` roots after the root apply (fail-closed; inner applies clear
    /// this flag). Requires `load_prefab_for_nested_propagation` when the preview list is non-empty.
    bool apply_reviewed_nested_prefab_propagation{false};
    std::function<std::optional<PrefabDefinition>(std::string_view prefab_path)> load_prefab_for_nested_propagation;
};

struct SceneAuthoringNodeRow {
    SceneNodeId node;
    SceneNodeId parent{null_scene_node};
    std::string name;
    std::size_t depth{0};
    bool selected{false};
    bool has_camera{false};
    bool has_light{false};
    bool has_mesh_renderer{false};
    bool has_sprite_renderer{false};
};

/// Valid non-null parent targets for `make_scene_authoring_reparent_node_action`, excluding the node, its subtree,
/// and the node's current parent (no-op). Labels prefix depth indentation for combo display; include unique `##`
/// suffixes for optional editor-shell combo disambiguation ids.
struct SceneReparentParentOption {
    SceneNodeId parent{null_scene_node};
    std::string label;
};

struct ScenePrefabInstanceSourceLinkRow {
    std::string id;
    SceneNodeId node;
    std::string node_name;
    ScenePrefabInstanceSourceLinkStatus status{ScenePrefabInstanceSourceLinkStatus::stale};
    std::string status_label;
    std::string prefab_name;
    std::string prefab_path;
    std::uint32_t source_node_index{0};
    std::string source_node_name;
    std::string diagnostic;
    bool selected{false};
};

struct ScenePrefabInstanceSourceLinkModel {
    std::vector<ScenePrefabInstanceSourceLinkRow> rows;
    std::size_t linked_count{0};
    std::size_t stale_count{0};
    bool has_links{false};
    bool has_diagnostics{false};
};

/// Descendant linked prefab instance root surfaced for propagation planning preview only (scene node iteration order).
struct SceneNestedPrefabPropagationPreviewRow {
    std::size_t preview_order{0};
    SceneNodeId instance_root{null_scene_node};
    std::string node_name;
    std::string prefab_name;
    std::string prefab_path;
};

struct ScenePrefabInstanceRefreshRow {
    std::string id;
    SceneNodeId current_node{null_scene_node};
    std::string current_node_name;
    std::uint32_t refreshed_node_index{0};
    std::string source_node_name;
    ScenePrefabInstanceRefreshStatus status{ScenePrefabInstanceRefreshStatus::ready};
    ScenePrefabInstanceRefreshRowKind kind{ScenePrefabInstanceRefreshRowKind::preserve_node};
    std::string status_label;
    std::string kind_label;
    std::string diagnostic;
    bool blocking{false};
};

struct ScenePrefabInstanceRefreshPlan {
    ScenePrefabInstanceRefreshStatus status{ScenePrefabInstanceRefreshStatus::ready};
    std::string status_label;
    SceneNodeId instance_root{null_scene_node};
    std::string prefab_name;
    std::string prefab_path;
    bool can_apply{false};
    bool mutates{false};
    bool executes{false};
    bool keep_local_children{false};
    bool keep_stale_source_nodes_as_local{false};
    bool keep_nested_prefab_instances{false};
    std::size_t row_count{0};
    std::size_t preserve_count{0};
    std::size_t add_count{0};
    std::size_t remove_count{0};
    std::size_t keep_local_child_count{0};
    std::size_t keep_stale_source_node_count{0};
    std::size_t keep_nested_prefab_instance_count{0};
    std::size_t unsupported_nested_prefab_instance_count{0};
    /// Descendant scene nodes that are linked prefab instance roots (source_node_index == 1) for a prefab asset
    /// different from the outer `instance_root` prefab identity; dry-run metric for chained refresh planning only.
    std::size_t descendant_linked_prefab_instance_root_count{0};
    /// Distinct nested prefab assets (`prefab_name` + `prefab_path`) among
    /// `descendant_linked_prefab_instance_root_count`.
    std::size_t distinct_nested_prefab_asset_count{0};
    std::size_t blocking_count{0};
    std::size_t warning_count{0};
    std::vector<ScenePrefabInstanceRefreshRow> rows;
    std::vector<std::string> diagnostics;
    /// Descendant linked prefab instance roots (scene iteration order). Chained refresh execution is opt-in via
    /// `ScenePrefabInstanceRefreshPolicy::apply_reviewed_nested_prefab_propagation` and a prefab loader.
    std::vector<SceneNestedPrefabPropagationPreviewRow> nested_prefab_propagation_preview;
    /// Mirrors the policy flag at plan time for MK_ui operator policy labels.
    bool apply_reviewed_nested_prefab_propagation_requested{false};
};

struct ScenePrefabInstanceRefreshResult {
    bool applied{false};
    std::size_t preserved_count{0};
    std::size_t added_count{0};
    std::size_t removed_count{0};
    std::size_t kept_local_child_count{0};
    std::size_t kept_stale_source_node_count{0};
    std::size_t kept_nested_prefab_instance_count{0};
    /// When `applied`, maps source-scene node ids to matching node ids in `scene` (subset of nodes; removed
    /// subtree nodes are omitted).
    std::unordered_map<SceneNodeId, SceneNodeId, SceneAuthoringSceneNodeIdHash> source_to_result_node_id;
    std::optional<Scene> scene;
    SceneNodeId selected_node{null_scene_node};
    std::string diagnostic;
};

struct ScenePrefabInstanceRefreshBatchTargetInput {
    SceneNodeId instance_root{null_scene_node};
    PrefabDefinition refreshed_prefab;
    std::string refreshed_prefab_path;
};

struct ScenePrefabInstanceRefreshBatchPlan {
    ScenePrefabInstanceRefreshStatus status{ScenePrefabInstanceRefreshStatus::ready};
    std::string status_label;
    bool can_apply{false};
    std::size_t target_count{0};
    std::size_t blocking_target_count{0};
    std::size_t warning_target_count{0};
    std::size_t ready_target_count{0};
    std::vector<std::string> batch_diagnostics;
    std::vector<ScenePrefabInstanceRefreshPlan> targets;
    /// Sorted instance roots with loaded prefabs; aligned index-wise with `targets` after planning.
    std::vector<ScenePrefabInstanceRefreshBatchTargetInput> ordered_targets;
    /// Mirrors the batch policy flag at plan time for MK_ui labels.
    bool apply_reviewed_nested_prefab_propagation_requested{false};
};

struct ScenePrefabInstanceRefreshBatchResult {
    bool applied{false};
    std::size_t targets_applied{0};
    std::optional<Scene> scene;
    SceneNodeId selected_node{null_scene_node};
    std::string diagnostic;
};

struct SceneAuthoringDiagnostic {
    SceneAuthoringDiagnosticKind kind{SceneAuthoringDiagnosticKind::missing_asset};
    SceneNodeId node;
    AssetId asset;
    std::string field;
    std::string diagnostic;
};

enum class ScenePackageCandidateKind : std::uint8_t { scene_source, scene_cooked, package_index, prefab_source };

struct ScenePackageCandidateRow {
    ScenePackageCandidateKind kind{ScenePackageCandidateKind::scene_source};
    std::string path;
    bool runtime_file{false};
};

enum class ScenePackageRegistrationDraftStatus : std::uint8_t {
    add_runtime_file,
    already_registered,
    rejected_source_file,
    rejected_unsafe_path,
    rejected_duplicate,
};

struct ScenePackageRegistrationDraftRow {
    ScenePackageCandidateKind kind{ScenePackageCandidateKind::scene_source};
    std::string candidate_path;
    std::string runtime_package_path;
    bool runtime_file{false};
    ScenePackageRegistrationDraftStatus status{ScenePackageRegistrationDraftStatus::rejected_source_file};
    std::string diagnostic;
};

struct ScenePackageRegistrationApplyPlan {
    std::string game_manifest_path;
    std::vector<std::string> runtime_package_files;
    bool can_apply{false};
    std::string diagnostic;
};

struct ScenePackageRegistrationApplyResult {
    bool applied{false};
    std::vector<std::string> runtime_package_files;
    std::string diagnostic;
};

enum class EditorSceneFileDialogMode : std::uint8_t { open, save };

struct EditorSceneFileDialogRow {
    std::string id;
    std::string label;
    std::string value;
};

struct EditorSceneFileDialogModel {
    EditorSceneFileDialogMode mode{EditorSceneFileDialogMode::open};
    bool accepted{false};
    std::string status_label;
    std::string selected_path;
    std::vector<EditorSceneFileDialogRow> rows;
    std::vector<std::string> diagnostics;
};

class SceneAuthoringDocument {
  public:
    [[nodiscard]] static SceneAuthoringDocument from_scene(Scene scene, std::string scene_path = {});

    [[nodiscard]] const Scene& scene() const noexcept;
    [[nodiscard]] std::string_view scene_path() const noexcept;
    [[nodiscard]] bool dirty() const;
    [[nodiscard]] SceneNodeId selected_node() const noexcept;
    [[nodiscard]] std::vector<SceneAuthoringNodeRow> hierarchy_rows() const;

    [[nodiscard]] bool select_node(SceneNodeId node) noexcept;
    [[nodiscard]] bool replace_scene(Scene scene, SceneNodeId selected_node);
    void set_scene_path(std::string scene_path);
    void mark_saved();

  private:
    SceneAuthoringDocument(Scene scene, std::string scene_path);

    Scene scene_;
    std::string scene_path_;
    std::string saved_scene_text_;
    SceneNodeId selected_node_{null_scene_node};
};

// Returned undoable actions capture the document by reference; keep the owning UndoStack scoped to this document.
[[nodiscard]] UndoableAction make_scene_authoring_rename_node_action(SceneAuthoringDocument& document, SceneNodeId node,
                                                                     std::string name);
[[nodiscard]] UndoableAction make_scene_authoring_create_node_action(SceneAuthoringDocument& document, std::string name,
                                                                     SceneNodeId parent = null_scene_node);
[[nodiscard]] UndoableAction make_scene_authoring_reparent_node_action(SceneAuthoringDocument& document,
                                                                       SceneNodeId node, SceneNodeId parent);

[[nodiscard]] std::vector<SceneReparentParentOption>
make_scene_authoring_reparent_parent_options(const SceneAuthoringDocument& document, SceneNodeId node);
[[nodiscard]] UndoableAction make_scene_authoring_delete_node_action(SceneAuthoringDocument& document,
                                                                     SceneNodeId node);
[[nodiscard]] UndoableAction make_scene_authoring_duplicate_subtree_action(SceneAuthoringDocument& document,
                                                                           SceneNodeId root, std::string root_name);
[[nodiscard]] UndoableAction make_scene_authoring_transform_edit_action(SceneAuthoringDocument& document,
                                                                        const SceneNodeTransformDraft& draft);
[[nodiscard]] UndoableAction make_scene_authoring_component_edit_action(SceneAuthoringDocument& document,
                                                                        const SceneNodeComponentDraft& draft);
[[nodiscard]] UndoableAction make_scene_authoring_instantiate_prefab_action(SceneAuthoringDocument& document,
                                                                            PrefabDefinition prefab);
[[nodiscard]] UndoableAction make_scene_authoring_instantiate_prefab_action(SceneAuthoringDocument& document,
                                                                            PrefabDefinition prefab,
                                                                            std::string prefab_path);

[[nodiscard]] std::optional<PrefabDefinition> build_prefab_from_selected_node(const SceneAuthoringDocument& document,
                                                                              std::string name);
[[nodiscard]] std::string_view
scene_prefab_instance_source_link_status_label(ScenePrefabInstanceSourceLinkStatus status) noexcept;
[[nodiscard]] ScenePrefabInstanceSourceLinkModel
make_scene_prefab_instance_source_link_model(const SceneAuthoringDocument& document);
[[nodiscard]] mirakana::ui::UiDocument
make_scene_prefab_instance_source_link_ui_model(const ScenePrefabInstanceSourceLinkModel& model);
[[nodiscard]] std::string_view
scene_prefab_instance_refresh_status_label(ScenePrefabInstanceRefreshStatus status) noexcept;
[[nodiscard]] std::string_view
scene_prefab_instance_refresh_row_kind_label(ScenePrefabInstanceRefreshRowKind kind) noexcept;
[[nodiscard]] ScenePrefabInstanceRefreshPlan plan_scene_prefab_instance_refresh(
    const SceneAuthoringDocument& document, SceneNodeId instance_root, const PrefabDefinition& refreshed_prefab,
    std::string_view refreshed_prefab_path = {}, const ScenePrefabInstanceRefreshPolicy& policy = {});
[[nodiscard]] ScenePrefabInstanceRefreshResult apply_scene_prefab_instance_refresh(
    const SceneAuthoringDocument& document, SceneNodeId instance_root, const PrefabDefinition& refreshed_prefab,
    std::string_view refreshed_prefab_path = {}, const ScenePrefabInstanceRefreshPolicy& policy = {});
[[nodiscard]] UndoableAction make_scene_prefab_instance_refresh_action(
    SceneAuthoringDocument& document, SceneNodeId instance_root, const PrefabDefinition& refreshed_prefab,
    const std::string& refreshed_prefab_path = {}, const ScenePrefabInstanceRefreshPolicy& policy = {});
[[nodiscard]] ScenePrefabInstanceRefreshBatchPlan
plan_scene_prefab_instance_refresh_batch(const SceneAuthoringDocument& document,
                                         std::vector<ScenePrefabInstanceRefreshBatchTargetInput> targets,
                                         const ScenePrefabInstanceRefreshPolicy& policy = {});
[[nodiscard]] ScenePrefabInstanceRefreshBatchResult
apply_scene_prefab_instance_refresh_batch(const SceneAuthoringDocument& document,
                                          std::vector<ScenePrefabInstanceRefreshBatchTargetInput> targets,
                                          const ScenePrefabInstanceRefreshPolicy& policy = {});
[[nodiscard]] UndoableAction
make_scene_prefab_instance_refresh_batch_action(SceneAuthoringDocument& document,
                                                std::vector<ScenePrefabInstanceRefreshBatchTargetInput> targets,
                                                const ScenePrefabInstanceRefreshPolicy& policy = {});
[[nodiscard]] mirakana::ui::UiDocument
make_scene_prefab_instance_refresh_ui_model(const ScenePrefabInstanceRefreshPlan& plan);
[[nodiscard]] mirakana::ui::UiDocument
make_scene_prefab_instance_refresh_batch_ui_model(const ScenePrefabInstanceRefreshBatchPlan& plan);

void save_scene_authoring_document(ITextStore& store, std::string_view path, SceneAuthoringDocument& document);
[[nodiscard]] SceneAuthoringDocument load_scene_authoring_document(ITextStore& store, std::string_view path);
[[nodiscard]] mirakana::FileDialogRequest make_scene_open_dialog_request(std::string_view default_location = "scenes");
[[nodiscard]] mirakana::FileDialogRequest
make_scene_save_dialog_request(std::string_view default_location = "scenes/start.scene");
[[nodiscard]] EditorSceneFileDialogModel make_scene_open_dialog_model(const mirakana::FileDialogResult& result);
[[nodiscard]] EditorSceneFileDialogModel make_scene_save_dialog_model(const mirakana::FileDialogResult& result);
[[nodiscard]] mirakana::ui::UiDocument make_scene_file_dialog_ui_model(const EditorSceneFileDialogModel& model);
void save_prefab_authoring_document(ITextStore& store, std::string_view path, const PrefabDefinition& prefab);
[[nodiscard]] PrefabDefinition load_prefab_authoring_document(ITextStore& store, std::string_view path);

[[nodiscard]] std::vector<SceneAuthoringDiagnostic> validate_scene_authoring_references(const Scene& scene,
                                                                                        const AssetRegistry& registry);

[[nodiscard]] std::vector<ScenePackageCandidateRow>
make_scene_package_candidate_rows(const SceneAuthoringDocument& document, std::string_view cooked_scene_path,
                                  std::string_view package_index_path,
                                  std::initializer_list<std::string_view> prefab_source_paths = {});

[[nodiscard]] std::string_view scene_package_candidate_kind_label(ScenePackageCandidateKind kind) noexcept;
[[nodiscard]] std::string_view
scene_package_registration_draft_status_label(ScenePackageRegistrationDraftStatus status) noexcept;

[[nodiscard]] std::vector<ScenePackageRegistrationDraftRow>
make_scene_package_registration_draft_rows(const std::vector<ScenePackageCandidateRow>& candidates,
                                           std::string_view project_root_path,
                                           std::initializer_list<std::string_view> existing_runtime_package_files = {});
[[nodiscard]] std::vector<ScenePackageRegistrationDraftRow>
make_scene_package_registration_draft_rows(const std::vector<ScenePackageCandidateRow>& candidates,
                                           std::string_view project_root_path,
                                           const std::vector<std::string>& existing_runtime_package_files);

[[nodiscard]] ScenePackageRegistrationApplyPlan
make_scene_package_registration_apply_plan(const std::vector<ScenePackageRegistrationDraftRow>& draft_rows,
                                           std::string_view project_root_path, std::string_view game_manifest_path);

[[nodiscard]] ScenePackageRegistrationApplyResult
apply_scene_package_registration_to_manifest(ITextStore& store, const ScenePackageRegistrationApplyPlan& plan);

} // namespace mirakana::editor
