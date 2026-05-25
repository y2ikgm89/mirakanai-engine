// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class ProductionAuthoringWorkflowKind : std::uint8_t {
    scene_placement,
    quest_dialogue,
    item_economy,
    ai_behavior,
    world_region,
    validation_repair,
};

struct ProductionAuthoringWorkflowRow {
    std::string workflow_id;
    ProductionAuthoringWorkflowKind kind{ProductionAuthoringWorkflowKind::scene_placement};
    std::string target_path;
    std::vector<std::string> required_capability_ids;
    std::vector<std::string> validation_recipe_ids;
    std::vector<std::string> package_evidence_ids;
    std::vector<std::string> reviewed_surface_ids;
    std::vector<std::string> claimed_scope_ids;
    bool requests_shared_surface_mutation{false};
    bool requests_arbitrary_shell{false};
    bool requests_cooked_package_mutation{false};
    std::uint32_t source_index{0};
};

struct ProductionAuthoringAcceptedWorkflowRow {
    std::string workflow_id;
    ProductionAuthoringWorkflowKind kind{ProductionAuthoringWorkflowKind::scene_placement};
    std::string target_path;
    std::vector<std::string> required_capability_ids;
    std::vector<std::string> validation_recipe_ids;
    std::vector<std::string> package_evidence_ids;
    std::vector<std::string> reviewed_surface_ids;
    std::uint32_t source_index{0};
};

struct ProductionAuthoringWorkflowMutationLedgerRow {
    std::string ledger_id;
    std::string workflow_id;
    ProductionAuthoringWorkflowKind kind{ProductionAuthoringWorkflowKind::scene_placement};
    std::string action;
    std::string target_path;
    std::vector<std::string> required_capability_ids;
    std::vector<std::string> validation_recipe_ids;
    std::vector<std::string> package_evidence_ids;
    std::vector<std::string> reviewed_surface_ids;
    std::uint32_t source_index{0};
};

struct ProductionAuthoringWorkflowValidationRepairRow {
    std::string workflow_id;
    std::string action;
    std::vector<std::string> validation_recipe_ids;
    std::vector<std::string> package_evidence_ids;
    std::uint32_t source_index{0};
};

struct ProductionAuthoringWorkflowDiagnostic {
    std::string code;
    std::string message;
    std::string workflow_id;
    std::string referenced_id;
    std::uint32_t source_index{0};
};

struct ProductionAuthoringWorkflowRequest {
    std::vector<std::string> supported_capability_ids;
    std::vector<std::string> validation_recipe_ids;
    std::vector<std::string> package_evidence_ids;
    std::vector<std::string> reviewed_surface_ids;
    std::vector<ProductionAuthoringWorkflowRow> workflow_rows;
};

struct ProductionAuthoringWorkflowReviewResult {
    std::vector<ProductionAuthoringAcceptedWorkflowRow> accepted_rows;
    std::vector<ProductionAuthoringWorkflowMutationLedgerRow> mutation_ledger_rows;
    std::vector<ProductionAuthoringWorkflowValidationRepairRow> validation_repair_rows;
    std::vector<ProductionAuthoringWorkflowDiagnostic> diagnostics;
    bool invoked_file_mutation{false};
    bool invoked_package_io{false};
    bool invoked_command_execution{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] ProductionAuthoringWorkflowReviewResult
review_production_authoring_workflow(const ProductionAuthoringWorkflowRequest& request);

} // namespace mirakana
