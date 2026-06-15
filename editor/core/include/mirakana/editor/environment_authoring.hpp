// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/editor/history.hpp"
#include "mirakana/editor/io.hpp"
#include "mirakana/editor/ui_model.hpp"
#include "mirakana/environment/cloud_layer.hpp"
#include "mirakana/environment/environment_preset_pack.hpp"
#include "mirakana/environment/environment_profile.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

enum class EnvironmentAuthoringStatus : std::uint8_t {
    blocked = 0,
    ready,
};

struct EnvironmentAuthoringInspectorRow {
    std::string id;
    std::string section;
    std::string label;
    std::string value;
    bool editable{true};
};

struct EnvironmentAuthoringDiagnosticRow {
    EnvironmentProfileDiagnosticCode profile_code{EnvironmentProfileDiagnosticCode::none};
    std::string field;
    std::string profile_id;
    std::string message;
};

struct EnvironmentAuthoringValidationModel {
    EnvironmentAuthoringStatus status{EnvironmentAuthoringStatus::blocked};
    std::vector<EnvironmentAuthoringDiagnosticRow> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

struct EnvironmentAuthoringCommandRequest;

class EnvironmentAuthoringDocument {
  public:
    [[nodiscard]] static EnvironmentAuthoringDocument from_profile(EnvironmentProfileDesc profile, std::string path);
    [[nodiscard]] static EnvironmentAuthoringDocument from_profile_document_v2(EnvironmentProfileDocumentV2 profile,
                                                                               std::string path);

    [[nodiscard]] const EnvironmentProfileDesc& profile() const noexcept;
    [[nodiscard]] const EnvironmentProfileDocumentV2& profile_document_v2() const noexcept;
    [[nodiscard]] std::string_view path() const noexcept;
    [[nodiscard]] bool dirty() const noexcept;
    [[nodiscard]] std::uint64_t revision() const noexcept;
    [[nodiscard]] std::uint64_t saved_revision() const noexcept;

  private:
    struct Snapshot {
        EnvironmentProfileDocumentV2 profile;
        DocumentDirtyState dirty;
    };

    [[nodiscard]] Snapshot snapshot() const;
    void restore(Snapshot snapshot);
    void replace_profile(EnvironmentProfileDesc profile);
    void replace_profile_document_v2(EnvironmentProfileDocumentV2 profile);
    void mark_saved() noexcept;
    void set_path(std::string path);

    EnvironmentProfileDocumentV2 profile_;
    std::string path_;
    DocumentDirtyState dirty_;

    friend UndoableAction make_environment_authoring_profile_edit_action(EnvironmentAuthoringDocument& document,
                                                                         EnvironmentProfileDesc profile,
                                                                         std::string label);
    friend UndoableAction make_environment_authoring_command_action(EnvironmentAuthoringDocument& document,
                                                                    const EnvironmentAuthoringCommandRequest& request);
    friend void save_environment_authoring_document(ITextStore& store, std::string_view path,
                                                    EnvironmentAuthoringDocument& document);
};

struct EnvironmentAuthoringInspectorDesc {
    EnvironmentAuthoringDocument document;
    EnvironmentCloudLayerDesc cloud_layer;
    bool volumetric_clouds_policy_available{false};
};

struct EnvironmentAuthoringInspectorModel {
    EnvironmentAuthoringStatus status{EnvironmentAuthoringStatus::blocked};
    std::string profile_id;
    std::string path;
    bool dirty{false};
    std::uint64_t revision{0};
    std::uint64_t saved_revision{0};
    bool invokes_backend{false};
    bool exposes_native_handles{false};
    bool executes_package_scripts{false};
    std::vector<EnvironmentAuthoringInspectorRow> rows;
    std::vector<EnvironmentAuthoringDiagnosticRow> diagnostics;
};

enum class EnvironmentPackageCandidateKind : std::uint8_t {
    profile_source = 0,
    profile_cooked,
    package_index,
    preset_pack,
};

struct EnvironmentPackageCandidateRow {
    EnvironmentPackageCandidateKind kind{EnvironmentPackageCandidateKind::profile_source};
    std::string path;
    bool runtime_file{false};
};

struct EnvironmentPresetLibraryDesc {
    EnvironmentPresetPackDocumentV1 pack;
    std::string path;
    std::string runtime_package_path;
    bool package_index_registered{false};
    bool sample_consumption_evidence{false};
};

struct EnvironmentPresetLibraryModel : EnvironmentAuthoringInspectorModel {
    std::string pack_id;
};

enum class EnvironmentPackageRegistrationDraftStatus : std::uint8_t {
    add_runtime_file = 0,
    already_registered,
    rejected_source_file,
    rejected_unsafe_path,
    rejected_duplicate,
};

struct EnvironmentPackageRegistrationDraftRow {
    EnvironmentPackageCandidateKind kind{EnvironmentPackageCandidateKind::profile_source};
    std::string candidate_path;
    std::string runtime_package_path;
    bool runtime_file{false};
    EnvironmentPackageRegistrationDraftStatus status{EnvironmentPackageRegistrationDraftStatus::rejected_source_file};
    std::string diagnostic;
};

enum class EnvironmentAuthoringCommandKind : std::uint8_t {
    add_volume = 0,
    remove_volume,
    reorder_volume,
    edit_weather_keyframe,
    select_quality_preset,
    request_cubemap_capture,
};

enum class EnvironmentAuthoringCommandStatus : std::uint8_t {
    accepted = 0,
    rejected_invalid_request,
    rejected_not_found,
    rejected_unsafe_execution,
};

struct EnvironmentAuthoringCommandDiagnosticRow {
    std::string code;
    std::string message;
};

struct EnvironmentAuthoringCommandRequest {
    EnvironmentAuthoringCommandKind kind{EnvironmentAuthoringCommandKind::add_volume};
    EnvironmentVolumeDesc volume;
    std::string volume_id;
    std::uint32_t source_index{0U};
    std::uint32_t target_index{0U};
    std::uint32_t weather_keyframe_index{0U};
    EnvironmentWeatherKeyframeDesc weather_keyframe;
    EnvironmentQualityPreset quality_preset{EnvironmentQualityPreset::medium};
    bool request_backend_execution{false};
    bool request_package_script_execution{false};
    bool request_native_handle_access{false};
    std::string label;
};

struct EnvironmentAuthoringCommandPlan {
    EnvironmentAuthoringCommandStatus status{EnvironmentAuthoringCommandStatus::rejected_invalid_request};
    std::string command_id;
    std::string label;
    bool mutates_document{false};
    bool requests_cubemap_capture{false};
    bool invokes_backend{false};
    bool exposes_native_handles{false};
    bool executes_package_scripts{false};
    std::vector<EnvironmentAuthoringCommandDiagnosticRow> diagnostics;
};

enum class EnvironmentArtistWorkflowCommandKind : std::uint8_t {
    preset_import = 0,
    source_asset_review,
    cook_preview,
    profile_graph_edit,
    weather_timeline_edit,
    local_volume_edit,
    simulation_parameter_edit,
    quality_budget_edit,
    package_preview,
    validation_remediation,
    publish_package,
};

enum class EnvironmentArtistWorkflowCommandMode : std::uint8_t {
    dry_run = 0,
    apply,
};

enum class EnvironmentArtistWorkflowCommandStatus : std::uint8_t {
    accepted = 0,
    rejected_invalid_request,
    rejected_stale_revision,
    rejected_unsafe_execution,
};

struct EnvironmentArtistWorkflowCommandRequest {
    EnvironmentArtistWorkflowCommandKind kind{EnvironmentArtistWorkflowCommandKind::preset_import};
    EnvironmentArtistWorkflowCommandMode mode{EnvironmentArtistWorkflowCommandMode::dry_run};
    std::uint64_t expected_revision{0U};
    bool user_confirmed{false};
    bool request_backend_execution{false};
    bool request_package_script_execution{false};
    bool request_native_handle_access{false};
};

struct EnvironmentArtistWorkflowCommandRow {
    EnvironmentArtistWorkflowCommandKind kind{EnvironmentArtistWorkflowCommandKind::preset_import};
    std::string command_id;
    std::string label;
    bool mutates_document{false};
    bool supports_dry_run{true};
    bool supports_revision_checked_apply{true};
    bool supports_undo_metadata{false};
    bool requires_confirmation{false};
    bool invokes_backend{false};
    bool exposes_native_handles{false};
    bool executes_package_scripts{false};
};

struct EnvironmentArtistWorkflowCommandReportRow {
    std::string id;
    std::string label;
    std::string value;
};

struct EnvironmentArtistWorkflowCommandDiagnosticRow {
    std::string code;
    std::string message;
};

struct EnvironmentArtistWorkflowCommandCatalog {
    std::uint64_t revision{0U};
    std::vector<EnvironmentArtistWorkflowCommandRow> commands;
};

struct EnvironmentArtistWorkflowCommandPlan {
    EnvironmentArtistWorkflowCommandStatus status{EnvironmentArtistWorkflowCommandStatus::rejected_invalid_request};
    std::string command_id;
    std::string label;
    bool dry_run{false};
    bool apply{false};
    bool mutates_document{false};
    bool revision_checked{false};
    bool undo_supported{false};
    bool rollback_metadata_available{false};
    bool requires_confirmation{false};
    bool invokes_backend{false};
    bool exposes_native_handles{false};
    bool executes_package_scripts{false};
    std::uint64_t before_revision{0U};
    std::uint64_t after_revision{0U};
    std::vector<EnvironmentArtistWorkflowCommandReportRow> report_rows;
    std::vector<EnvironmentArtistWorkflowCommandDiagnosticRow> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

enum class EnvironmentArtistWorkflowAssetKind : std::uint8_t {
    preset_library = 0,
    openexr_source,
    ktx2_basis_source,
    cooked_texture,
    environment_profile,
    simulation_preset,
    validation_report,
    package_artifact,
};

enum class EnvironmentArtistWorkflowAssetRowStatus : std::uint8_t {
    ready = 0,
    missing,
    blocked,
};

struct EnvironmentArtistWorkflowAssetBrowserInputRow {
    EnvironmentArtistWorkflowAssetKind kind{EnvironmentArtistWorkflowAssetKind::preset_library};
    std::string path;
    bool available{false};
    bool package_visible{false};
    bool provenance_recorded{false};
    bool budget_recorded{false};
    bool requires_host_gate{false};
    std::string host_gate;
    std::string validation_recipe_id;
};

struct EnvironmentArtistWorkflowAssetBrowserDesc {
    std::vector<EnvironmentArtistWorkflowAssetBrowserInputRow> assets;
    bool request_backend_execution{false};
    bool request_package_script_execution{false};
    bool request_native_handle_access{false};
};

struct EnvironmentArtistWorkflowAssetBrowserRow {
    EnvironmentArtistWorkflowAssetKind kind{EnvironmentArtistWorkflowAssetKind::preset_library};
    EnvironmentArtistWorkflowAssetRowStatus status{EnvironmentArtistWorkflowAssetRowStatus::missing};
    std::string row_id;
    std::string label;
    std::string path;
    bool available{false};
    bool package_visible{false};
    bool provenance_recorded{false};
    bool budget_recorded{false};
    bool requires_host_gate{false};
    std::string host_gate;
    std::string validation_recipe_id;
    bool invokes_backend{false};
    bool exposes_native_handles{false};
    bool executes_package_scripts{false};
};

struct EnvironmentArtistWorkflowAssetBrowserDiagnosticRow {
    std::string code;
    std::string message;
};

struct EnvironmentArtistWorkflowAssetBrowserModel {
    EnvironmentAuthoringStatus status{EnvironmentAuthoringStatus::blocked};
    std::size_t ready_rows{0U};
    bool complete_artist_workflow_ready_claimed{false};
    bool invokes_backend{false};
    bool exposes_native_handles{false};
    bool executes_package_scripts{false};
    std::vector<EnvironmentArtistWorkflowAssetBrowserRow> rows;
    std::vector<EnvironmentArtistWorkflowAssetBrowserDiagnosticRow> diagnostics;
};

enum class EnvironmentArtistWorkflowPreviewRowKind : std::uint8_t {
    selected_backend = 0,
    quality_tier,
    missing_host_gate,
    package_budget,
    memory_budget,
    diagnostics,
    unsupported_claim_reason,
};

enum class EnvironmentArtistWorkflowPreviewRowStatus : std::uint8_t {
    ready = 0,
    blocked,
};

struct EnvironmentArtistWorkflowPreviewDesc {
    std::string selected_backend;
    std::string quality_tier;
    std::string missing_host_gate;
    std::uint64_t package_budget_bytes{0U};
    std::uint64_t memory_budget_bytes{0U};
    std::uint32_t diagnostics{0U};
    std::string unsupported_claim_reason;
    bool request_backend_execution{false};
    bool request_package_script_execution{false};
    bool request_native_handle_access{false};
};

struct EnvironmentArtistWorkflowPreviewRow {
    EnvironmentArtistWorkflowPreviewRowKind kind{EnvironmentArtistWorkflowPreviewRowKind::selected_backend};
    EnvironmentArtistWorkflowPreviewRowStatus status{EnvironmentArtistWorkflowPreviewRowStatus::blocked};
    std::string row_id;
    std::string label;
    std::string value;
    bool invokes_backend{false};
    bool exposes_native_handles{false};
    bool executes_package_scripts{false};
};

struct EnvironmentArtistWorkflowPreviewDiagnosticRow {
    std::string code;
    std::string message;
};

struct EnvironmentArtistWorkflowPreviewModel {
    EnvironmentAuthoringStatus status{EnvironmentAuthoringStatus::blocked};
    std::size_t ready_rows{0U};
    bool complete_artist_workflow_ready_claimed{false};
    bool invokes_backend{false};
    bool exposes_native_handles{false};
    bool executes_package_scripts{false};
    std::vector<EnvironmentArtistWorkflowPreviewRow> rows;
    std::vector<EnvironmentArtistWorkflowPreviewDiagnosticRow> diagnostics;
};

enum class EnvironmentArtistWorkflowWalkthroughStepKind : std::uint8_t {
    import_source_assets = 0,
    cook_assets,
    assemble_preset,
    edit_weather_timeline,
    run_simulation_preview,
    package_sample,
    run_installed_validation,
    inspect_report,
};

enum class EnvironmentArtistWorkflowWalkthroughStepStatus : std::uint8_t {
    ready = 0,
    missing,
    blocked,
};

struct EnvironmentArtistWorkflowWalkthroughStepInputRow {
    EnvironmentArtistWorkflowWalkthroughStepKind kind{
        EnvironmentArtistWorkflowWalkthroughStepKind::import_source_assets};
    std::string evidence_id;
    bool completed{false};
    bool reviewed{false};
    bool package_visible{false};
    bool requires_host_gate{false};
    std::string host_gate;
    std::string validation_recipe_id;
};

struct EnvironmentArtistWorkflowWalkthroughDesc {
    std::vector<EnvironmentArtistWorkflowWalkthroughStepInputRow> steps;
    bool request_backend_execution{false};
    bool request_package_script_execution{false};
    bool request_native_handle_access{false};
};

struct EnvironmentArtistWorkflowWalkthroughStepRow {
    EnvironmentArtistWorkflowWalkthroughStepKind kind{
        EnvironmentArtistWorkflowWalkthroughStepKind::import_source_assets};
    EnvironmentArtistWorkflowWalkthroughStepStatus status{EnvironmentArtistWorkflowWalkthroughStepStatus::missing};
    std::string row_id;
    std::string label;
    std::string command_id;
    std::string evidence_id;
    bool completed{false};
    bool reviewed{false};
    bool package_visible{false};
    bool requires_host_gate{false};
    std::string host_gate;
    std::string validation_recipe_id;
    bool invokes_backend{false};
    bool exposes_native_handles{false};
    bool executes_package_scripts{false};
};

struct EnvironmentArtistWorkflowWalkthroughDiagnosticRow {
    std::string code;
    std::string message;
};

struct EnvironmentArtistWorkflowWalkthroughModel {
    EnvironmentAuthoringStatus status{EnvironmentAuthoringStatus::blocked};
    std::size_t ready_rows{0U};
    bool complete_artist_workflow_ready_claimed{false};
    bool invokes_backend{false};
    bool exposes_native_handles{false};
    bool executes_package_scripts{false};
    std::vector<EnvironmentArtistWorkflowWalkthroughStepRow> rows;
    std::vector<EnvironmentArtistWorkflowWalkthroughDiagnosticRow> diagnostics;
};

[[nodiscard]] std::string_view environment_package_candidate_kind_label(EnvironmentPackageCandidateKind kind) noexcept;
[[nodiscard]] std::string_view
environment_package_registration_draft_status_label(EnvironmentPackageRegistrationDraftStatus status) noexcept;
[[nodiscard]] std::string_view
environment_artist_workflow_command_id(EnvironmentArtistWorkflowCommandKind kind) noexcept;
[[nodiscard]] std::string_view
environment_artist_workflow_asset_kind_id(EnvironmentArtistWorkflowAssetKind kind) noexcept;
[[nodiscard]] std::string_view
environment_artist_workflow_preview_row_id(EnvironmentArtistWorkflowPreviewRowKind kind) noexcept;
[[nodiscard]] std::string_view
environment_artist_workflow_walkthrough_step_id(EnvironmentArtistWorkflowWalkthroughStepKind kind) noexcept;

[[nodiscard]] EnvironmentAuthoringDocument load_environment_authoring_document(ITextStore& store,
                                                                               std::string_view path);
void save_environment_authoring_document(ITextStore& store, std::string_view path,
                                         EnvironmentAuthoringDocument& document);
[[nodiscard]] UndoableAction make_environment_authoring_profile_edit_action(EnvironmentAuthoringDocument& document,
                                                                            EnvironmentProfileDesc profile,
                                                                            std::string label = "Edit Environment");
[[nodiscard]] EnvironmentAuthoringCommandPlan
plan_environment_authoring_command(const EnvironmentAuthoringDocument& document,
                                   const EnvironmentAuthoringCommandRequest& request);
[[nodiscard]] UndoableAction
make_environment_authoring_command_action(EnvironmentAuthoringDocument& document,
                                          const EnvironmentAuthoringCommandRequest& request);
[[nodiscard]] EnvironmentArtistWorkflowCommandCatalog
make_environment_artist_workflow_command_catalog(const EnvironmentAuthoringDocument& document);
[[nodiscard]] EnvironmentArtistWorkflowCommandPlan
plan_environment_artist_workflow_command(const EnvironmentAuthoringDocument& document,
                                         const EnvironmentArtistWorkflowCommandRequest& request);
[[nodiscard]] EnvironmentArtistWorkflowAssetBrowserModel
make_environment_artist_workflow_asset_browser_model(const EnvironmentArtistWorkflowAssetBrowserDesc& desc);
[[nodiscard]] mirakana::ui::UiDocument
make_environment_artist_workflow_asset_browser_ui_model(const EnvironmentArtistWorkflowAssetBrowserModel& model);
[[nodiscard]] EnvironmentArtistWorkflowPreviewModel
make_environment_artist_workflow_preview_model(const EnvironmentArtistWorkflowPreviewDesc& desc);
[[nodiscard]] mirakana::ui::UiDocument
make_environment_artist_workflow_preview_ui_model(const EnvironmentArtistWorkflowPreviewModel& model);
[[nodiscard]] EnvironmentArtistWorkflowWalkthroughModel
make_environment_artist_workflow_walkthrough_model(const EnvironmentArtistWorkflowWalkthroughDesc& desc);
[[nodiscard]] mirakana::ui::UiDocument
make_environment_artist_workflow_walkthrough_ui_model(const EnvironmentArtistWorkflowWalkthroughModel& model);

[[nodiscard]] EnvironmentAuthoringValidationModel
make_environment_authoring_validation_model(const EnvironmentAuthoringDocument& document);
[[nodiscard]] EnvironmentAuthoringInspectorModel
make_environment_authoring_inspector_model(const EnvironmentAuthoringInspectorDesc& desc);
[[nodiscard]] std::vector<EditorPropertyRow>
make_environment_authoring_editor_property_rows(const EnvironmentAuthoringInspectorModel& model);
[[nodiscard]] mirakana::ui::UiDocument
make_environment_authoring_ui_model(const EnvironmentAuthoringInspectorModel& model);

[[nodiscard]] EnvironmentPresetLibraryModel
make_environment_preset_library_model(const EnvironmentPresetLibraryDesc& desc);
[[nodiscard]] mirakana::ui::UiDocument
make_environment_preset_library_ui_model(const EnvironmentPresetLibraryModel& model);

[[nodiscard]] std::vector<EnvironmentPackageCandidateRow>
make_environment_package_candidate_rows(const EnvironmentAuthoringDocument& document,
                                        std::string_view cooked_profile_path, std::string_view package_index_path);
[[nodiscard]] std::vector<EnvironmentPackageCandidateRow>
make_environment_preset_library_package_candidate_rows(std::string_view runtime_preset_pack_path);
[[nodiscard]] std::vector<EnvironmentPackageRegistrationDraftRow>
make_environment_package_registration_draft_rows(std::span<const EnvironmentPackageCandidateRow> candidates,
                                                 std::string_view project_root_path,
                                                 std::span<const std::string> existing_runtime_files);

} // namespace mirakana::editor
