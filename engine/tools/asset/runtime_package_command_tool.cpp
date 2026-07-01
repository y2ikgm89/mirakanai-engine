// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/runtime_package_command_tool.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

[[nodiscard]] std::string mode_name(RuntimePackageCommandMode mode) {
    switch (mode) {
    case RuntimePackageCommandMode::dry_run:
        return "dry-run";
    case RuntimePackageCommandMode::apply:
        return "apply";
    }
    return "dry-run";
}

[[nodiscard]] std::string backend_name(RuntimePackageCommandBackend backend) {
    switch (backend) {
    case RuntimePackageCommandBackend::unspecified:
        return "unspecified";
    case RuntimePackageCommandBackend::d3d12:
        return "d3d12";
    case RuntimePackageCommandBackend::vulkan:
        return "vulkan";
    case RuntimePackageCommandBackend::metal:
        return "metal";
    }
    return "unspecified";
}

[[nodiscard]] std::vector<std::string> default_validation_recipes() {
    return {"agent-contract", "public-api-boundary", "desktop-game-runtime", "default"};
}

[[nodiscard]] std::vector<std::string> default_unsupported_gap_ids() {
    return {"runtime-resource-v2", "renderer-rhi-resource-foundation", "editor-productization",
            "3d-playable-vertical-slice"};
}

[[nodiscard]] bool has_control_character(std::string_view value) noexcept {
    return std::ranges::any_of(value, [](char character) {
        const auto code = static_cast<unsigned char>(character);
        return code < 0x20U || code == 0x7FU;
    });
}

[[nodiscard]] bool is_safe_path(std::string_view path) noexcept {
    if (path.empty() || path.front() == '/' || path.front() == '\\' || path.find(':') != std::string_view::npos ||
        path.find('\\') != std::string_view::npos || path.find(';') != std::string_view::npos ||
        has_control_character(path)) {
        return false;
    }

    std::size_t begin = 0;
    while (begin <= path.size()) {
        const auto end = path.find('/', begin);
        const auto token = path.substr(begin, end == std::string_view::npos ? std::string_view::npos : end - begin);
        if (token.empty() || token == "." || token == "..") {
            return false;
        }
        if (end == std::string_view::npos) {
            break;
        }
        begin = end + 1U;
    }
    return true;
}

[[nodiscard]] bool starts_with(std::string_view value, std::string_view prefix) noexcept {
    return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
}

[[nodiscard]] bool ends_with(std::string_view value, std::string_view suffix) noexcept {
    return value.size() >= suffix.size() && value.substr(value.size() - suffix.size()) == suffix;
}

[[nodiscard]] bool is_lower_snake_name(std::string_view value) noexcept {
    if (value.empty() || value.front() < 'a' || value.front() > 'z') {
        return false;
    }
    return std::ranges::all_of(value, [](char character) {
        return (character >= 'a' && character <= 'z') || (character >= '0' && character <= '9') || character == '_';
    });
}

[[nodiscard]] bool is_safe_package_target(std::string_view value) noexcept {
    if (value.empty()) {
        return false;
    }
    const auto first = value.front();
    const auto starts_with_valid_character =
        (first >= 'A' && first <= 'Z') || (first >= 'a' && first <= 'z') || first == '_';
    if (!starts_with_valid_character) {
        return false;
    }
    return std::ranges::all_of(value, [](char character) {
        return (character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z') ||
               (character >= '0' && character <= '9') || character == '_';
    });
}

[[nodiscard]] bool is_safe_typed_value(std::string_view value) noexcept {
    if (value.empty() || has_control_character(value)) {
        return false;
    }
    return value.find(';') == std::string_view::npos && value.find('|') == std::string_view::npos &&
           value.find('&') == std::string_view::npos && value.find('<') == std::string_view::npos &&
           value.find('>') == std::string_view::npos && value.find('\\') == std::string_view::npos &&
           value.find('"') == std::string_view::npos && value.find('\'') == std::string_view::npos;
}

[[nodiscard]] std::vector<std::string> split_path(std::string_view path) {
    std::vector<std::string> segments;
    std::size_t begin = 0;
    while (begin <= path.size()) {
        const auto end = path.find('/', begin);
        segments.emplace_back(path.substr(begin, end == std::string_view::npos ? std::string_view::npos : end - begin));
        if (end == std::string_view::npos) {
            break;
        }
        begin = end + 1U;
    }
    return segments;
}

[[nodiscard]] bool is_safe_game_manifest_path(std::string_view path) {
    if (!is_safe_path(path) || !starts_with(path, "games/") || !ends_with(path, "/game.agent.json")) {
        return false;
    }
    const auto segments = split_path(path);
    return segments.size() == 3U && segments[0] == "games" && is_lower_snake_name(segments[1]) &&
           segments[2] == "game.agent.json";
}

void add_gap_id(RuntimePackageCommandResult& result, std::string gap_id) {
    if (std::ranges::find(result.unsupported_gap_ids, gap_id) == result.unsupported_gap_ids.end()) {
        result.unsupported_gap_ids.push_back(std::move(gap_id));
    }
}

void add_diagnostic(RuntimePackageCommandResult& result, std::string code, std::string message, std::string path = {},
                    std::string unsupported_gap_id = {}, std::string validation_recipe = {}) {
    if (!unsupported_gap_id.empty()) {
        add_gap_id(result, unsupported_gap_id);
    }
    result.diagnostics.push_back(RuntimePackageCommandDiagnostic{
        .severity = "error",
        .code = std::move(code),
        .message = std::move(message),
        .path = std::move(path),
        .unsupported_gap_id = std::move(unsupported_gap_id),
        .validation_recipe = std::move(validation_recipe),
    });
}

void sort_diagnostics(std::vector<RuntimePackageCommandDiagnostic>& diagnostics) {
    std::ranges::sort(diagnostics, [](const auto& lhs, const auto& rhs) {
        if (lhs.path != rhs.path) {
            return lhs.path < rhs.path;
        }
        if (lhs.code != rhs.code) {
            return lhs.code < rhs.code;
        }
        return lhs.message < rhs.message;
    });
}

void validate_claim(RuntimePackageCommandResult& result, std::string_view value, std::string code, std::string message,
                    std::string gap_id) {
    if (value != "unsupported") {
        add_diagnostic(result, std::move(code), std::move(message), {}, std::move(gap_id));
    }
}

void validate_unsupported_claims(RuntimePackageCommandResult& result, const RuntimePackageCommandRequest& request) {
    validate_claim(result, request.package_build_execution, "unsupported_package_build_execution",
                   "package build execution is not command-owned by cook-runtime-package dry-run",
                   "runtime-resource-v2");
    validate_claim(result, request.package_scripts, "unsupported_package_scripts",
                   "package script execution is not supported by cook-runtime-package dry-run",
                   "editor-productization");
    validate_claim(result, request.shader_compiler_execution, "unsupported_shader_compiler_execution",
                   "shader compiler execution is not supported by cook-runtime-package dry-run",
                   "renderer-rhi-resource-foundation");
    validate_claim(result, request.runtime_package_load_execution, "unsupported_runtime_package_load_execution",
                   "runtime package load execution is not supported by cook-runtime-package dry-run",
                   "runtime-resource-v2");
    validate_claim(result, request.renderer_rhi_residency, "unsupported_renderer_rhi_residency",
                   "renderer/RHI residency is not supported by cook-runtime-package dry-run",
                   "renderer-rhi-resource-foundation");
    validate_claim(result, request.package_streaming, "unsupported_package_streaming",
                   "package streaming is not supported by cook-runtime-package dry-run", "runtime-resource-v2");
    validate_claim(result, request.public_native_rhi_handles, "unsupported_public_native_rhi_handles",
                   "public native/RHI handles are not supported by cook-runtime-package dry-run",
                   "renderer-rhi-resource-foundation");
    validate_claim(result, request.legal_approval, "unsupported_legal_approval",
                   "legal approval is not provided by cook-runtime-package dry-run", "editor-productization");
    validate_claim(result, request.external_engine_compatibility, "unsupported_external_engine_compatibility",
                   "external engine compatibility is not supported by cook-runtime-package dry-run",
                   "editor-productization");
    validate_claim(result, request.external_engine_project_import, "unsupported_external_engine_project_import",
                   "external engine project import is not supported by cook-runtime-package dry-run",
                   "editor-productization");
    validate_claim(result, request.external_engine_assets, "unsupported_external_engine_assets",
                   "external engine assets are not supported by cook-runtime-package dry-run", "editor-productization");
    validate_claim(result, request.arbitrary_shell, "unsupported_arbitrary_shell",
                   "arbitrary shell execution is not supported by cook-runtime-package dry-run",
                   "editor-productization");
    validate_claim(result, request.free_form_edit, "unsupported_free_form_edit",
                   "free-form edits are not supported by cook-runtime-package dry-run", "editor-productization");
}

void validate_runtime_package_files(RuntimePackageCommandResult& result, const RuntimePackageCommandRequest& request) {
    if (request.runtime_package_files.empty()) {
        add_diagnostic(result, "missing_runtime_package_files", "at least one runtime package file must be selected",
                       request.game_manifest_path);
        return;
    }

    std::unordered_set<std::string> seen;
    seen.reserve(request.runtime_package_files.size());
    for (const auto& path : request.runtime_package_files) {
        if (!is_safe_path(path) || starts_with(path, "games/") || !starts_with(path, "runtime/")) {
            add_diagnostic(result, "unsafe_runtime_package_file",
                           "runtime package file must be game-relative, safe, and under runtime/", path);
        }
        if (!seen.insert(path).second) {
            add_diagnostic(result, "duplicate_runtime_package_file", "runtime package file path is duplicated", path);
        }
    }
}

void validate_typed_values(RuntimePackageCommandResult& result, const RuntimePackageCommandRequest& request) {
    std::unordered_set<std::string> smoke_args;
    smoke_args.reserve(request.smoke_args.size());
    for (const auto& arg : request.smoke_args) {
        if (!is_safe_typed_value(arg)) {
            add_diagnostic(result, "unsafe_smoke_arg", "smoke argument must be a safe typed value", arg);
        }
        if (!smoke_args.insert(arg).second) {
            add_diagnostic(result, "duplicate_smoke_arg", "smoke argument is duplicated", arg);
        }
    }

    std::unordered_set<std::string> shader_requirements;
    shader_requirements.reserve(request.shader_artifact_requirements.size());
    for (const auto& requirement : request.shader_artifact_requirements) {
        if (!is_safe_typed_value(requirement)) {
            add_diagnostic(result, "unsafe_shader_artifact_requirement",
                           "shader artifact requirement must be a safe typed value", requirement);
        }
        if (!shader_requirements.insert(requirement).second) {
            add_diagnostic(result, "duplicate_shader_artifact_requirement", "shader artifact requirement is duplicated",
                           requirement);
        }
    }
}

void validate_recipes(RuntimePackageCommandResult& result) {
    if (result.validation_recipes.empty()) {
        add_diagnostic(result, "missing_validation_recipes",
                       "at least one validation recipe is required for cook-runtime-package dry-run");
        return;
    }

    std::unordered_set<std::string> seen;
    seen.reserve(result.validation_recipes.size());
    for (const auto& recipe : result.validation_recipes) {
        if (!is_safe_typed_value(recipe) || recipe.find('/') != std::string::npos) {
            add_diagnostic(result, "unsafe_validation_recipe", "validation recipe must be a safe recipe id", {}, {},
                           recipe);
        }
        if (!seen.insert(recipe).second) {
            add_diagnostic(result, "duplicate_validation_recipe", "validation recipe is duplicated", {}, {}, recipe);
        }
    }
}

void validate_request_shape(RuntimePackageCommandResult& result, const RuntimePackageCommandRequest& request) {
    if (!is_safe_game_manifest_path(request.game_manifest_path)) {
        add_diagnostic(result, "unsafe_game_manifest_path",
                       "game manifest path must be games/<game_name>/game.agent.json with a lowercase snake_case game "
                       "name",
                       request.game_manifest_path);
    }
    if (!is_safe_package_target(request.package_target)) {
        add_diagnostic(result, "invalid_package_target", "package target must match ^[A-Za-z_][A-Za-z0-9_]*$",
                       request.package_target);
    }
    validate_runtime_package_files(result, request);
    validate_typed_values(result, request);
    validate_recipes(result);
    validate_unsupported_claims(result, request);
}

void append_capability_gates(RuntimePackageCommandResult& result, RuntimePackageCommandBackend backend) {
    result.capability_gates.push_back(RuntimePackageCommandCapabilityGate{
        .id = "MK_runtime",
        .source = "module",
        .status = "ready-safe-point-controller",
        .notes = "Reviewed runtime package handles and safe-point/controller APIs are available; package execution "
                 "remains outside this dry-run command.",
    });
    result.capability_gates.push_back(RuntimePackageCommandCapabilityGate{
        .id = "desktop-runtime-cooked-scene-package",
        .source = "package-surface",
        .status = "host-gated",
        .notes = "Desktop runtime package cooking stays validation-recipe-gated and is not executed by dry-run.",
    });

    switch (backend) {
    case RuntimePackageCommandBackend::d3d12:
        result.capability_gates.push_back(RuntimePackageCommandCapabilityGate{
            .id = "d3d12-windows-primary",
            .source = "host-gate",
            .status = "host-gated",
            .notes = "D3D12 package smoke requires a ready Windows host and selected shader artifacts.",
        });
        break;
    case RuntimePackageCommandBackend::vulkan:
        result.capability_gates.push_back(RuntimePackageCommandCapabilityGate{
            .id = "vulkan-strict",
            .source = "host-gate",
            .status = "host-gated",
            .notes = "Strict Vulkan package smoke requires Vulkan SDK tooling, validation layers, and SPIR-V evidence.",
        });
        break;
    case RuntimePackageCommandBackend::metal:
        result.capability_gates.push_back(RuntimePackageCommandCapabilityGate{
            .id = "metal-apple",
            .source = "host-gate",
            .status = "host-gated",
            .notes =
                "Metal package proof requires Apple host evidence and does not infer readiness from other backends.",
        });
        break;
    case RuntimePackageCommandBackend::unspecified:
        break;
    }
}

void append_planned_changes(RuntimePackageCommandResult& result, const RuntimePackageCommandRequest& request) {
    result.planned_changes.push_back(RuntimePackageCommandPlannedChange{
        .kind = "review_game_manifest",
        .path = request.game_manifest_path,
        .detail =
            "Review game.agent.json package target, runtimePackageFiles, packagingTargets, and validationRecipes.",
    });

    std::vector<std::string> runtime_files = request.runtime_package_files;
    std::ranges::sort(runtime_files);
    for (const auto& path : runtime_files) {
        result.planned_changes.push_back(RuntimePackageCommandPlannedChange{
            .kind = "runtime_package_file",
            .path = path,
            .detail = "Validate game-relative runtime package payload row before recipe execution.",
        });
    }

    for (const auto& recipe : result.validation_recipes) {
        result.planned_changes.push_back(RuntimePackageCommandPlannedChange{
            .kind = "validation_recipe",
            .path = request.game_manifest_path,
            .detail = "Select validation recipe " + recipe + " for package proof.",
        });
    }

    for (const auto& requirement : request.shader_artifact_requirements) {
        result.planned_changes.push_back(RuntimePackageCommandPlannedChange{
            .kind = "shader_artifact_requirement",
            .path = request.game_manifest_path,
            .detail = "Require reviewed shader artifact row " + requirement + " before host package smoke.",
        });
    }

    auto command =
        std::string{"pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget "};
    command += request.package_target;
    for (const auto& arg : request.smoke_args) {
        command += " ";
        command += arg;
    }
    result.planned_changes.push_back(RuntimePackageCommandPlannedChange{
        .kind = "package_command",
        .path = "tools/package-desktop-runtime.ps1",
        .detail = command + " (planned only; not executed by dry-run)",
    });

    if (request.backend != RuntimePackageCommandBackend::unspecified) {
        result.planned_changes.push_back(RuntimePackageCommandPlannedChange{
            .kind = "backend_host_gate",
            .path = request.game_manifest_path,
            .detail = "Selected backend " + backend_name(request.backend) + " remains host-gated.",
        });
    }
}

void append_apply_blockers(RuntimePackageCommandResult& result, const RuntimePackageCommandRequest& request) {
    result.blocked_by.push_back(RuntimePackageCommandBlockedBy{
        .id = "package_build_execution_not_command_owned",
        .reason = "package build execution is not command-owned by cook-runtime-package",
        .path = request.game_manifest_path,
    });
    result.blocked_by.push_back(RuntimePackageCommandBlockedBy{
        .id = "package_apply_not_ready",
        .reason = "apply mode is blocked until package cooking and file mutation have a reviewed command contract",
        .path = request.game_manifest_path,
    });
    add_diagnostic(result, "apply_blocked",
                   "apply mode is blocked until package cooking and file mutation are command-owned",
                   request.game_manifest_path, "runtime-resource-v2");
}

[[nodiscard]] RuntimePackageCommandResult make_base_result(const RuntimePackageCommandRequest& request) {
    RuntimePackageCommandResult result;
    result.mode = mode_name(request.mode);
    result.validation_recipes =
        request.validation_recipes.empty() ? default_validation_recipes() : request.validation_recipes;
    result.unsupported_gap_ids = default_unsupported_gap_ids();
    result.would_change = !request.runtime_package_files.empty();
    return result;
}

} // namespace

RuntimePackageCommandResult plan_runtime_package_command(const RuntimePackageCommandRequest& request) {
    auto result = make_base_result(request);
    validate_request_shape(result, request);
    if (!result.succeeded()) {
        result.status = "invalid";
        sort_diagnostics(result.diagnostics);
        return result;
    }

    append_capability_gates(result, request.backend);
    append_planned_changes(result, request);

    if (request.mode == RuntimePackageCommandMode::apply) {
        result.status = "blocked";
        append_apply_blockers(result, request);
        sort_diagnostics(result.diagnostics);
        return result;
    }

    result.status = "ready";
    return result;
}

} // namespace mirakana
