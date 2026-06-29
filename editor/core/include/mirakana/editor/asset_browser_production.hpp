// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_import_pipeline.hpp"
#include "mirakana/assets/asset_import_provenance.hpp"
#include "mirakana/assets/asset_source_format.hpp"
#include "mirakana/editor/asset_pipeline.hpp"
#include "mirakana/editor/content_browser.hpp"
#include "mirakana/editor/material_asset_preview_panel.hpp"
#include "mirakana/editor/scene_authoring.hpp"
#include "mirakana/tools/gltf_mesh_inspect.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

enum class EditorAssetBrowserProductionStatus : std::uint8_t { empty, ready, attention };
enum class EditorAssetBrowserQueryStatus : std::uint8_t { empty, ready, blocked };
enum class EditorAssetBrowserCommandKind : std::uint8_t {
    reload_source_registry,
    review_import_sources,
    register_import_sources,
    copy_external_sources,
    execute_reviewed_import_plan,
    reimport_selected,
    recook_stale,
    preview_cooked_package,
    stage_hot_reload,
    inspect_selection,
    apply_package_registration,
};
enum class EditorAssetBrowserCommandMode : std::uint8_t { dry_run, apply };
enum class EditorAssetBrowserCommandStatus : std::uint8_t { ready, blocked, rejected_stale_generation };
enum class EditorAssetBrowserPackageReviewStatus : std::uint8_t {
    add,
    already_registered,
    source_only,
    unsafe_path,
    duplicate,
    missing_cooked_artifact,
    blocked_license,
};

struct EditorAssetBrowserSourcePulseRow {
    AssetId asset;
    AssetKind kind{AssetKind::unknown};
    std::string row_id;
    std::string kind_label;
    std::string asset_key_label;
    std::string source_path;
    std::string imported_path;
    std::string display_name;
    std::string scope_label;
    std::string state_label;
    std::string import_status_label;
    std::string package_status_label;
    std::string provenance_status_label;
    std::string preview_status_label;
    std::string hot_reload_status_label;
    bool selected{false};
    bool identity_backed{false};
    bool source_visible{false};
    bool package_visible{false};
    bool blocked{false};
    bool host_gated{false};
};

struct EditorAssetBrowserPreviewEvidenceRow {
    std::string id;
    std::string asset_key_label;
    std::string preview_kind;
    std::string backend_label;
    std::string display_path_label;
    std::string status_label;
    std::string diagnostic;
    std::uint64_t frame_or_sample_count{0};
    bool host_owned{true};
    bool ready{false};
    bool exposes_native_handles{false};
};

struct EditorAssetBrowserGltfInspectEvidenceInput {
    AssetId asset;
    std::string asset_key_label;
    std::string source_path;
    GltfMeshInspectReport report;
};

struct EditorAssetBrowserAudioSummaryInput {
    AssetId asset;
    std::string asset_key_label;
    std::string source_path;
    AudioSourceDocument audio;
};

struct EditorAssetBrowserPreviewEvidenceDesc {
    std::vector<EditorMaterialAssetPreviewPanelModel> material_previews;
    std::vector<EditorAssetThumbnailRequest> thumbnail_requests;
    std::vector<EditorAssetBrowserGltfInspectEvidenceInput> gltf_inspects;
    std::vector<EditorAssetBrowserAudioSummaryInput> audio_summaries;
    std::vector<SceneAuthoringDiagnostic> scene_reference_diagnostics;
    std::vector<AssetHotReloadRecookRequest> hot_reload_recook_requests;
};

struct EditorAssetBrowserRetainedCommandRow {
    std::string command_id;
    std::string label;
    std::string status_label;
    bool enabled{false};
    bool requires_user_confirmation{false};
    bool mutates_project_files{false};
    bool executes_import_tools{false};
    bool executes_package_scripts{false};
    bool executes_validation_recipes{false};
    bool exposes_native_handles{false};
};

struct EditorAssetBrowserRetainedLegalRow {
    std::string id;
    std::string asset_key_label;
    std::string status_label;
    std::string diagnostic;
    bool blocked{true};
};

struct EditorAssetBrowserRetainedUiDesc {
    std::string query_text;
    std::string query_status_label{"Asset browser query empty"};
    std::vector<EditorAssetBrowserRetainedCommandRow> command_rows;
    std::vector<EditorAssetBrowserRetainedLegalRow> legal_rows;
};

struct EditorAssetBrowserPackageReviewRow {
    std::string id;
    ScenePackageCandidateKind kind{ScenePackageCandidateKind::scene_source};
    std::string kind_label;
    std::string candidate_path;
    std::string runtime_package_path;
    EditorAssetBrowserPackageReviewStatus status{EditorAssetBrowserPackageReviewStatus::source_only};
    std::string status_label;
    std::string diagnostic;
    bool runtime_file{false};
    bool can_apply{false};
    bool blocked{true};
};

struct EditorAssetBrowserPackageReviewDesc {
    std::vector<ScenePackageCandidateRow> candidates;
    std::string project_root_path;
    std::string game_manifest_path{"game.agent.json"};
    std::vector<std::string> existing_runtime_package_files;
    std::vector<std::string> available_cooked_artifacts;
    std::vector<std::string> blocked_license_runtime_package_files;
};

struct EditorAssetBrowserPackageReviewModel {
    std::vector<EditorAssetBrowserPackageReviewRow> rows;
    ScenePackageRegistrationApplyPlan apply_plan;
    bool executes_package_scripts{false};
    bool executes_validation_recipes{false};
    bool streams_packages{false};
    bool loads_runtime_game_modules{false};
};

struct EditorAssetBrowserProductionDesc {
    const ContentBrowserState* browser{nullptr};
    const AssetImportPlan* import_plan{nullptr};
    const AssetPipelineState* pipeline_state{nullptr};
    std::string project_root{"."};
    std::string asset_root{"assets"};
    std::string source_registry_path;
    std::uint64_t generation{1};
    const EditorAssetBrowserPreviewEvidenceDesc* preview_evidence{nullptr};
    const EditorAssetBrowserRetainedUiDesc* retained_ui{nullptr};
    const EditorAssetBrowserPackageReviewDesc* package_review{nullptr};
};

struct EditorAssetBrowserProductionModel {
    EditorAssetBrowserProductionStatus status{EditorAssetBrowserProductionStatus::empty};
    std::string status_label{"Asset browser empty"};
    std::string project_root;
    std::string asset_root;
    std::string source_registry_path;
    std::uint64_t generation{1};
    std::size_t total_row_count{0};
    std::size_t visible_row_count{0};
    std::vector<EditorAssetBrowserSourcePulseRow> rows;
    std::vector<EditorAssetBrowserPreviewEvidenceRow> preview_rows;
    std::string query_text;
    std::string query_status_label{"Asset browser query empty"};
    std::vector<EditorAssetBrowserRetainedCommandRow> command_rows;
    std::vector<EditorAssetBrowserRetainedLegalRow> legal_rows;
    EditorAssetBrowserPackageReviewModel package_review;
    std::vector<std::string> diagnostics;
    bool mutates{false};
    bool executes{false};
    bool exposes_native_handles{false};
    bool decodes_source_files{false};
    bool uploads_gpu_resources{false};
    bool executes_shader_compilers{false};
    bool streams_packages{false};
    bool mutates_manifests{false};
    bool executes_package_scripts{false};
    bool executes_validation_recipes{false};
    bool loads_runtime_game_modules{false};
};

struct EditorAssetBrowserQueryTokenRow {
    std::string id;
    std::string key;
    std::string value;
    std::string status_label;
    bool active{false};
    bool blocked{false};
};

struct EditorAssetBrowserQueryDesc {
    std::string query_text;
    std::vector<EditorAssetBrowserSourcePulseRow> rows;
};

struct EditorAssetBrowserQueryResult {
    EditorAssetBrowserQueryStatus status{EditorAssetBrowserQueryStatus::empty};
    std::string status_label{"Asset browser query empty"};
    std::string normalized_query;
    std::vector<EditorAssetBrowserQueryTokenRow> tokens;
    std::vector<EditorAssetBrowserSourcePulseRow> rows;
    std::vector<std::string> diagnostics;
};

struct EditorAssetBrowserCommandRequest {
    EditorAssetBrowserCommandKind kind{EditorAssetBrowserCommandKind::reload_source_registry};
    EditorAssetBrowserCommandMode mode{EditorAssetBrowserCommandMode::dry_run};
    std::uint64_t expected_generation{0};
    std::uint64_t current_generation{0};
    bool user_confirmed{false};
};

struct EditorAssetBrowserCommandPlan {
    std::string command_id;
    std::string label;
    EditorAssetBrowserCommandStatus status{EditorAssetBrowserCommandStatus::blocked};
    std::string status_label;
    std::uint64_t expected_generation{0};
    std::uint64_t current_generation{0};
    bool requires_user_confirmation{false};
    bool mutates_project_files{false};
    bool executes_import_tools{false};
    bool executes_package_scripts{false};
    bool executes_validation_recipes{false};
    bool exposes_native_handles{false};
    std::vector<std::string> report_rows;
    std::vector<std::string> diagnostics;
};

struct EditorAssetBrowserLegalProvenanceRow {
    std::string id;
    std::string asset_key_label;
    std::string source_url;
    std::string retrieved_date;
    std::string version_or_commit;
    std::string copyright_holder;
    std::string license_id;
    std::string modification_status;
    std::string distribution_target;
    std::string status_label;
    std::string diagnostic;
    bool notice_complete{false};
    bool external_engine_material{false};
    bool accepted_for_package{false};
    bool blocked{false};
};

struct EditorAssetBrowserOpenExrSourceReviewRow {
    std::string id;
    std::string asset_key_label;
    std::string source_path;
    std::string display_window;
    std::string data_window;
    std::string pixel_aspect_ratio;
    std::string channels;
    std::string pixel_type_rows;
    std::string compression;
    std::string line_order;
    std::string screen_window_width;
    std::string screen_window_center;
    std::string tiled_policy;
    std::string multipart_policy;
    std::string deep_image_policy;
    std::string declared_color_intent;
    std::string status_label;
    std::string diagnostic;
    bool header_required_attributes_present{false};
    bool chromaticities_present{false};
    bool scene_linear_claimed{false};
    bool optional_importer_feature{false};
    bool blocked{true};
};

struct EditorAssetBrowserKtx2BasisSourceReviewRow {
    std::string id;
    std::string asset_key_label;
    std::string source_path;
    std::string basis_color_model;
    std::string selected_transcode_target;
    std::string backend_format_support_evidence_id;
    std::string dimensions;
    std::string levels;
    std::string layers;
    std::string faces;
    std::string supercompression;
    std::string payload_byte_count;
    std::string status_label;
    std::string diagnostic;
    bool loaded_with_image_data{false};
    bool needs_transcoding{false};
    bool gpu_upload_requested{false};
    bool editor_core_upload_executed{false};
    bool optional_importer_feature{false};
    bool blocked{true};
};

[[nodiscard]] std::string_view
editor_asset_browser_production_status_label(EditorAssetBrowserProductionStatus status) noexcept;
[[nodiscard]] std::string_view editor_asset_browser_command_id(EditorAssetBrowserCommandKind kind) noexcept;
[[nodiscard]] std::string_view
editor_asset_browser_package_review_status_label(EditorAssetBrowserPackageReviewStatus status) noexcept;
[[nodiscard]] std::vector<EditorAssetBrowserPreviewEvidenceRow>
make_editor_asset_browser_preview_evidence_rows(const EditorAssetBrowserPreviewEvidenceDesc& desc);
[[nodiscard]] EditorAssetBrowserPackageReviewModel
make_editor_asset_browser_package_review_model(const EditorAssetBrowserPackageReviewDesc& desc);
[[nodiscard]] EditorAssetBrowserProductionModel
make_editor_asset_browser_production_model(const EditorAssetBrowserProductionDesc& desc);
[[nodiscard]] mirakana::ui::UiDocument
make_editor_asset_browser_production_ui_model(const EditorAssetBrowserProductionModel& model);
[[nodiscard]] EditorAssetBrowserQueryResult plan_editor_asset_browser_query(const EditorAssetBrowserQueryDesc& desc);
[[nodiscard]] EditorAssetBrowserCommandPlan
plan_editor_asset_browser_command(const EditorAssetBrowserCommandRequest& request);
[[nodiscard]] EditorAssetBrowserLegalProvenanceRow
make_editor_asset_browser_legal_provenance_row(const mirakana::AssetImportProvenanceRowV1& row);
[[nodiscard]] EditorAssetBrowserLegalProvenanceRow
review_editor_asset_browser_legal_provenance(const EditorAssetBrowserLegalProvenanceRow& row);
[[nodiscard]] EditorAssetBrowserOpenExrSourceReviewRow
review_editor_asset_browser_open_exr_source(const EditorAssetBrowserOpenExrSourceReviewRow& row);
[[nodiscard]] EditorAssetBrowserKtx2BasisSourceReviewRow
review_editor_asset_browser_ktx2_basis_source(const EditorAssetBrowserKtx2BasisSourceReviewRow& row);

} // namespace mirakana::editor
