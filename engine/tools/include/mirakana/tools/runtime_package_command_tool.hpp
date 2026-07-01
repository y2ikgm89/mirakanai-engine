// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class RuntimePackageCommandMode : std::uint8_t {
    dry_run,
    apply,
};

enum class RuntimePackageCommandBackend : std::uint8_t {
    unspecified,
    d3d12,
    vulkan,
    metal,
};

struct RuntimePackageCommandPlannedChange {
    std::string kind;
    std::string path;
    std::string detail;
};

struct RuntimePackageCommandBlockedBy {
    std::string id;
    std::string reason;
    std::string path;
};

struct RuntimePackageCommandCapabilityGate {
    std::string id;
    std::string source;
    std::string status;
    std::string notes;
};

struct RuntimePackageCommandChangedFile {
    std::string path;
    std::string document_kind;
    std::string content;
    std::uint64_t content_hash{0};
};

struct RuntimePackageCommandDiagnostic {
    std::string severity{"error"};
    std::string code;
    std::string message;
    std::string path;
    std::string unsupported_gap_id;
    std::string validation_recipe;
};

struct RuntimePackageCommandRequest {
    RuntimePackageCommandMode mode{RuntimePackageCommandMode::dry_run};
    std::string game_manifest_path;
    std::string package_target;
    std::vector<std::string> runtime_package_files;
    RuntimePackageCommandBackend backend{RuntimePackageCommandBackend::unspecified};
    std::vector<std::string> smoke_args;
    std::vector<std::string> shader_artifact_requirements;
    std::vector<std::string> validation_recipes;

    std::string package_build_execution{"unsupported"};
    std::string package_scripts{"unsupported"};
    std::string shader_compiler_execution{"unsupported"};
    std::string runtime_package_load_execution{"unsupported"};
    std::string renderer_rhi_residency{"unsupported"};
    std::string package_streaming{"unsupported"};
    std::string public_native_rhi_handles{"unsupported"};
    std::string legal_approval{"unsupported"};
    std::string external_engine_compatibility{"unsupported"};
    std::string external_engine_project_import{"unsupported"};
    std::string external_engine_assets{"unsupported"};
    std::string arbitrary_shell{"unsupported"};
    std::string free_form_edit{"unsupported"};
};

struct RuntimePackageCommandResult {
    std::string schema{"GameEngine.AiCommand.CookRuntimePackage.Result.v1"};
    std::string command_id{"cook-runtime-package"};
    std::string mode{"dry-run"};
    std::string status{"ready"};
    bool would_change{false};
    std::vector<RuntimePackageCommandPlannedChange> planned_changes;
    std::vector<RuntimePackageCommandBlockedBy> blocked_by;
    std::vector<RuntimePackageCommandCapabilityGate> capability_gates;
    std::vector<RuntimePackageCommandChangedFile> changed_files;
    std::vector<RuntimePackageCommandDiagnostic> diagnostics;
    std::vector<std::string> validation_recipes;
    std::vector<std::string> unsupported_gap_ids;
    std::string undo_token{"placeholder-only"};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] RuntimePackageCommandResult plan_runtime_package_command(const RuntimePackageCommandRequest& request);

} // namespace mirakana
