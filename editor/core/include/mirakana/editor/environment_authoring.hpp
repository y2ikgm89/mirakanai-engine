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

[[nodiscard]] std::string_view environment_package_candidate_kind_label(EnvironmentPackageCandidateKind kind) noexcept;
[[nodiscard]] std::string_view
environment_package_registration_draft_status_label(EnvironmentPackageRegistrationDraftStatus status) noexcept;

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
