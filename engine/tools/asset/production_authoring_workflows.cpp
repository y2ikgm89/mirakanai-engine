// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/production_authoring_workflows.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] std::unordered_set<std::string> make_set(const std::vector<std::string>& values) {
    return {values.begin(), values.end()};
}

void add_diagnostic(ProductionAuthoringWorkflowReviewResult& result, std::string code, std::string message,
                    const ProductionAuthoringWorkflowRow& row, std::string referenced_id = {}) {
    result.diagnostics.push_back(ProductionAuthoringWorkflowDiagnostic{
        .code = std::move(code),
        .message = std::move(message),
        .workflow_id = row.workflow_id,
        .referenced_id = std::move(referenced_id),
        .source_index = row.source_index,
    });
}

[[nodiscard]] std::string to_ascii_lowercase(std::string_view value) {
    std::string lowered;
    lowered.reserve(value.size());
    for (const unsigned char character : value) {
        lowered.push_back(static_cast<char>(std::tolower(character)));
    }
    return lowered;
}

[[nodiscard]] bool contains_empty_value(const std::vector<std::string>& values) {
    return std::ranges::any_of(values, [](const auto& value) { return value.empty(); });
}

[[nodiscard]] bool is_game_owned_file_path(std::string_view value) {
    constexpr std::string_view games_prefix = "games/";
    if (!value.starts_with(games_prefix) || value.ends_with("/") || value.find('\\') != std::string_view::npos ||
        value.find("..") != std::string_view::npos || value.find("//") != std::string_view::npos ||
        value.find(';') != std::string_view::npos) {
        return false;
    }

    const auto remaining = value.substr(games_prefix.size());
    const auto separator = remaining.find('/');
    if (separator == std::string_view::npos || separator == 0U || separator + 1U == remaining.size()) {
        return false;
    }

    const auto game_name = remaining.substr(0, separator);
    if (std::islower(static_cast<unsigned char>(game_name.front())) == 0) {
        return false;
    }

    return std::ranges::all_of(game_name, [](const unsigned char character) {
        return std::islower(character) != 0 || std::isdigit(character) != 0 || character == '_';
    });
}

[[nodiscard]] bool contains_native_backend_term(std::string_view value) {
    const auto lowered = to_ascii_lowercase(value);
    constexpr std::string_view terms[] = {
        "native handle",   "native window", "sdl",          "sdl3",
        "dear imgui",      "imgui",         "renderer/rhi", "rhi",
        "d3d12",           "id3d12",        "vulkan",       "vk",
        "metal",           "middleware",    "fastgltf",     "cgltf",
        "parser type",     "parser handle", "idxc",         "dxccompiler",
        "compiler handle", "ktxtexture",    "spng",         "ma_decoder",
    };
    return std::ranges::any_of(terms,
                               [&lowered](std::string_view term) { return lowered.find(term) != std::string::npos; });
}

[[nodiscard]] bool contains_arbitrary_shell_term(std::string_view value) {
    const auto lowered = to_ascii_lowercase(value);
    return lowered.contains("arbitrary shell") || lowered.contains("shell command") || lowered.contains("cmd.exe") ||
           lowered.contains("powershell") || lowered.contains("process execution");
}

[[nodiscard]] bool contains_cooked_package_mutation_term(std::string_view value) {
    const auto lowered = to_ascii_lowercase(value);
    return lowered.contains("cooked package mutation") || lowered.contains("cooked-package mutation") ||
           lowered.contains("package io") || lowered.contains("runtime package write") ||
           lowered.contains("mutate package");
}

void validate_id_vector(ProductionAuthoringWorkflowReviewResult& result, const ProductionAuthoringWorkflowRow& row,
                        const std::vector<std::string>& values, const std::unordered_set<std::string>& supported,
                        std::string_view missing_code, std::string_view unsupported_code,
                        std::string_view missing_message, std::string_view unsupported_message) {
    if (values.empty() || contains_empty_value(values)) {
        add_diagnostic(result, std::string{missing_code}, std::string{missing_message}, row);
        return;
    }

    for (const auto& value : values) {
        if (!supported.contains(value)) {
            add_diagnostic(result, std::string{unsupported_code}, std::string{unsupported_message}, row, value);
        }
    }
}

} // namespace

ProductionAuthoringWorkflowReviewResult
review_production_authoring_workflow(const ProductionAuthoringWorkflowRequest& request) {
    ProductionAuthoringWorkflowReviewResult result;
    const auto supported_capabilities = make_set(request.supported_capability_ids);
    const auto validation_recipes = make_set(request.validation_recipe_ids);
    const auto package_evidence = make_set(request.package_evidence_ids);
    const auto reviewed_surfaces = make_set(request.reviewed_surface_ids);

    std::unordered_set<std::string> seen_workflow_ids;
    for (const auto& row : request.workflow_rows) {
        if (row.workflow_id.empty()) {
            add_diagnostic(result, "missing_workflow_id", "production authoring workflow id is required", row);
        } else if (!seen_workflow_ids.insert(row.workflow_id).second) {
            add_diagnostic(result, "duplicate_workflow_id", "production authoring workflow id appears more than once",
                           row, row.workflow_id);
        }

        if (row.target_path.empty()) {
            add_diagnostic(result, "missing_target_path", "production authoring target path is required", row);
        } else if (!is_game_owned_file_path(row.target_path)) {
            add_diagnostic(result, "invalid_target_path",
                           "production authoring target paths must be repo-relative game-owned paths", row,
                           row.target_path);
        }

        validate_id_vector(result, row, row.required_capability_ids, supported_capabilities,
                           "missing_required_capability", "unsupported_required_capability",
                           "at least one reviewed capability id is required",
                           "requested capability is not supported by the active production authoring profile");
        validate_id_vector(result, row, row.validation_recipe_ids, validation_recipes, "missing_validation_recipe",
                           "unsupported_validation_recipe", "at least one validation recipe id is required",
                           "validation recipe is not supported by the active production authoring profile");
        validate_id_vector(result, row, row.package_evidence_ids, package_evidence, "missing_package_evidence",
                           "unsupported_package_evidence", "at least one package evidence id is required",
                           "package evidence id is not supported by the active production authoring profile");
        validate_id_vector(result, row, row.reviewed_surface_ids, reviewed_surfaces, "missing_reviewed_surface",
                           "unsupported_reviewed_surface", "at least one reviewed command surface id is required",
                           "reviewed command surface id is not supported by the active production authoring profile");

        bool emitted_shell = false;
        bool emitted_cooked_package = false;
        if (row.requests_shared_surface_mutation) {
            add_diagnostic(result, "shared_surface_mutation",
                           "production authoring workflows must not mutate shared engine/editor/tool surfaces", row);
        }
        if (row.requests_arbitrary_shell) {
            add_diagnostic(result, "arbitrary_shell",
                           "production authoring workflows must not execute arbitrary shell commands", row);
            emitted_shell = true;
        }
        if (row.requests_cooked_package_mutation) {
            add_diagnostic(result, "cooked_package_mutation",
                           "production authoring workflows must not mutate cooked package payloads directly", row);
            emitted_cooked_package = true;
        }

        for (const auto& claim : row.claimed_scope_ids) {
            if (contains_native_backend_term(claim)) {
                add_diagnostic(result, "native_backend_term",
                               "production authoring workflows must not expose native, renderer, RHI, or backend terms",
                               row, claim);
            } else if (contains_arbitrary_shell_term(claim)) {
                if (!emitted_shell) {
                    add_diagnostic(result, "arbitrary_shell",
                                   "production authoring workflows must not execute arbitrary shell commands", row,
                                   claim);
                    emitted_shell = true;
                }
            } else if (contains_cooked_package_mutation_term(claim)) {
                if (!emitted_cooked_package) {
                    add_diagnostic(result, "cooked_package_mutation",
                                   "production authoring workflows must not mutate cooked package payloads directly",
                                   row, claim);
                    emitted_cooked_package = true;
                }
            } else {
                add_diagnostic(result, "unsupported_claim",
                               "production authoring workflows reject broad or unreviewed authoring claims", row,
                               claim);
            }
        }
    }

    if (!result.diagnostics.empty()) {
        return result;
    }

    result.accepted_rows.reserve(request.workflow_rows.size());
    result.mutation_ledger_rows.reserve(request.workflow_rows.size());
    result.validation_repair_rows.reserve(request.workflow_rows.size());
    for (const auto& row : request.workflow_rows) {
        result.accepted_rows.push_back(ProductionAuthoringAcceptedWorkflowRow{
            .workflow_id = row.workflow_id,
            .kind = row.kind,
            .target_path = row.target_path,
            .required_capability_ids = row.required_capability_ids,
            .validation_recipe_ids = row.validation_recipe_ids,
            .package_evidence_ids = row.package_evidence_ids,
            .reviewed_surface_ids = row.reviewed_surface_ids,
            .source_index = row.source_index,
        });
        result.mutation_ledger_rows.push_back(ProductionAuthoringWorkflowMutationLedgerRow{
            .ledger_id = "production-authoring:" + row.workflow_id,
            .workflow_id = row.workflow_id,
            .kind = row.kind,
            .action = "reviewed-production-authoring-plan",
            .target_path = row.target_path,
            .required_capability_ids = row.required_capability_ids,
            .validation_recipe_ids = row.validation_recipe_ids,
            .package_evidence_ids = row.package_evidence_ids,
            .reviewed_surface_ids = row.reviewed_surface_ids,
            .source_index = row.source_index,
        });
        if (row.kind == ProductionAuthoringWorkflowKind::validation_repair) {
            result.validation_repair_rows.push_back(ProductionAuthoringWorkflowValidationRepairRow{
                .workflow_id = row.workflow_id,
                .action = "rerun-selected-validation-recipe",
                .validation_recipe_ids = row.validation_recipe_ids,
                .package_evidence_ids = row.package_evidence_ids,
                .source_index = row.source_index,
            });
        }
    }

    return result;
}

} // namespace mirakana
