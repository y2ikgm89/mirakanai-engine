// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/project_wizard.hpp"

#include <stdexcept>
#include <utility>

namespace mirakana::editor {
namespace {

bool valid_field(std::string_view value) {
    return !value.empty() && value.find_first_of("\r\n=") == std::string_view::npos;
}

bool has_parent_path_segment(std::string_view value) {
    std::size_t segment_start = 0;
    while (segment_start <= value.size()) {
        const auto separator = value.find_first_of("/\\", segment_start);
        const auto segment_end = separator == std::string_view::npos ? value.size() : separator;
        if (value.substr(segment_start, segment_end - segment_start) == "..") {
            return true;
        }
        if (separator == std::string_view::npos) {
            break;
        }
        segment_start = separator + 1;
    }
    return false;
}

bool valid_project_relative_path(std::string_view value) {
    return valid_field(value) && !value.starts_with('/') && !value.starts_with('\\') &&
           value.find(':') == std::string_view::npos && !has_parent_path_segment(value);
}

void add_error_if_invalid(std::vector<ProjectCreationError>& errors, std::string field, std::string_view value,
                          std::string message) {
    if (!valid_field(value)) {
        errors.push_back(ProjectCreationError{.field = std::move(field), .message = std::move(message)});
    }
}

void add_error_if_invalid_source_registry_path(std::vector<ProjectCreationError>& errors, std::string_view value) {
    if (!valid_project_relative_path(value) || !value.ends_with(".geassets")) {
        errors.push_back(ProjectCreationError{
            .field = "source_registry_path",
            .message = "Source registry path must be a project-relative .geassets path",
        });
    }
}

} // namespace

ProjectCreationWizard ProjectCreationWizard::begin() {
    return ProjectCreationWizard{};
}

ProjectCreationStep ProjectCreationWizard::step() const noexcept {
    return step_;
}

const ProjectCreationDraft& ProjectCreationWizard::draft() const noexcept {
    return draft_;
}

std::vector<ProjectCreationError> ProjectCreationWizard::validation_errors() const {
    std::vector<ProjectCreationError> errors;

    add_error_if_invalid(errors, "name", draft_.name, "Project name is required and must not contain newlines or '='");
    add_error_if_invalid(errors, "root_path", draft_.root_path, "Project root path is required");
    if (step_ == ProjectCreationStep::paths || step_ == ProjectCreationStep::review) {
        add_error_if_invalid(errors, "asset_root", draft_.asset_root, "Asset root is required");
        add_error_if_invalid_source_registry_path(errors, draft_.source_registry_path);
        add_error_if_invalid(errors, "game_manifest_path", draft_.game_manifest_path, "Game manifest path is required");
        add_error_if_invalid(errors, "startup_scene_path", draft_.startup_scene_path, "Startup scene path is required");
    }

    return errors;
}

bool ProjectCreationWizard::can_advance() const {
    return validation_errors().empty();
}

ProjectDocument ProjectCreationWizard::create_project_document() const {
    if (!validation_errors().empty()) {
        throw std::invalid_argument("project creation draft is invalid");
    }

    ProjectDocument document{
        .name = draft_.name,
        .root_path = draft_.root_path,
        .asset_root = draft_.asset_root,
        .source_registry_path = draft_.source_registry_path,
        .game_manifest_path = draft_.game_manifest_path,
        .startup_scene_path = draft_.startup_scene_path,
    };
    validate_project_document(document);
    return document;
}

void ProjectCreationWizard::set_name(std::string value) {
    draft_.name = std::move(value);
}

void ProjectCreationWizard::set_root_path(std::string value) {
    draft_.root_path = std::move(value);
}

void ProjectCreationWizard::set_asset_root(std::string value) {
    draft_.asset_root = std::move(value);
}

void ProjectCreationWizard::set_source_registry_path(std::string value) {
    draft_.source_registry_path = std::move(value);
}

void ProjectCreationWizard::set_game_manifest_path(std::string value) {
    draft_.game_manifest_path = std::move(value);
}

void ProjectCreationWizard::set_startup_scene_path(std::string value) {
    draft_.startup_scene_path = std::move(value);
}

bool ProjectCreationWizard::advance() {
    if (!can_advance()) {
        return false;
    }
    if (step_ == ProjectCreationStep::identity) {
        step_ = ProjectCreationStep::paths;
        return true;
    }
    if (step_ == ProjectCreationStep::paths) {
        step_ = ProjectCreationStep::review;
        return true;
    }
    return false;
}

bool ProjectCreationWizard::back() {
    if (step_ == ProjectCreationStep::review) {
        step_ = ProjectCreationStep::paths;
        return true;
    }
    if (step_ == ProjectCreationStep::paths) {
        step_ = ProjectCreationStep::identity;
        return true;
    }
    return false;
}

void ProjectCreationWizard::reset() {
    step_ = ProjectCreationStep::identity;
    draft_ = ProjectCreationDraft{};
}

std::string_view project_creation_step_name(ProjectCreationStep step) noexcept {
    switch (step) {
    case ProjectCreationStep::identity:
        return "Identity";
    case ProjectCreationStep::paths:
        return "Paths";
    case ProjectCreationStep::review:
        return "Review";
    }
    return "Unknown";
}

} // namespace mirakana::editor
