// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/editor/history.hpp"
#include "mirakana/editor/io.hpp"
#include "mirakana/editor/ui_model.hpp"
#include "mirakana/environment/cloud_layer.hpp"
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

enum class EnvironmentAuthoringQualityTier : std::uint8_t {
    low = 0,
    medium,
    high,
    cinematic,
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

class EnvironmentAuthoringDocument {
  public:
    [[nodiscard]] static EnvironmentAuthoringDocument from_profile(EnvironmentProfileDesc profile, std::string path);

    [[nodiscard]] const EnvironmentProfileDesc& profile() const noexcept;
    [[nodiscard]] std::string_view path() const noexcept;
    [[nodiscard]] bool dirty() const noexcept;
    [[nodiscard]] std::uint64_t revision() const noexcept;
    [[nodiscard]] std::uint64_t saved_revision() const noexcept;

  private:
    struct Snapshot {
        EnvironmentProfileDesc profile;
        DocumentDirtyState dirty;
    };

    [[nodiscard]] Snapshot snapshot() const;
    void restore(Snapshot snapshot);
    void replace_profile(EnvironmentProfileDesc profile);
    void mark_saved() noexcept;
    void set_path(std::string path);

    EnvironmentProfileDesc profile_;
    std::string path_;
    DocumentDirtyState dirty_;

    friend UndoableAction make_environment_authoring_profile_edit_action(EnvironmentAuthoringDocument& document,
                                                                         EnvironmentProfileDesc profile,
                                                                         std::string label);
    friend void save_environment_authoring_document(ITextStore& store, std::string_view path,
                                                    EnvironmentAuthoringDocument& document);
};

struct EnvironmentAuthoringInspectorDesc {
    EnvironmentAuthoringDocument document;
    EnvironmentCloudLayerDesc cloud_layer;
    bool volumetric_clouds_policy_available{false};
    EnvironmentAuthoringQualityTier quality_tier{EnvironmentAuthoringQualityTier::high};
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
};

struct EnvironmentPackageCandidateRow {
    EnvironmentPackageCandidateKind kind{EnvironmentPackageCandidateKind::profile_source};
    std::string path;
    bool runtime_file{false};
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

[[nodiscard]] std::string_view environment_authoring_quality_tier_label(EnvironmentAuthoringQualityTier tier) noexcept;
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

[[nodiscard]] EnvironmentAuthoringValidationModel
make_environment_authoring_validation_model(const EnvironmentAuthoringDocument& document);
[[nodiscard]] EnvironmentAuthoringInspectorModel
make_environment_authoring_inspector_model(const EnvironmentAuthoringInspectorDesc& desc);
[[nodiscard]] std::vector<EditorPropertyRow>
make_environment_authoring_editor_property_rows(const EnvironmentAuthoringInspectorModel& model);
[[nodiscard]] mirakana::ui::UiDocument
make_environment_authoring_ui_model(const EnvironmentAuthoringInspectorModel& model);

[[nodiscard]] std::vector<EnvironmentPackageCandidateRow>
make_environment_package_candidate_rows(const EnvironmentAuthoringDocument& document,
                                        std::string_view cooked_profile_path, std::string_view package_index_path);
[[nodiscard]] std::vector<EnvironmentPackageRegistrationDraftRow>
make_environment_package_registration_draft_rows(std::span<const EnvironmentPackageCandidateRow> candidates,
                                                 std::string_view project_root_path,
                                                 std::span<const std::string> existing_runtime_files);

} // namespace mirakana::editor
