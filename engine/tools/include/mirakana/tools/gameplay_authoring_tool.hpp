// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

struct GameplayAuthoringCapabilityProfile {
    std::vector<std::string> supported_capability_ids;
    std::vector<std::string> validation_recipe_ids;
    std::vector<std::string> package_evidence_ids;
};

struct GameplayAuthoringRequestedFeatureRow {
    std::string feature_id;
    std::string gameplay_family;
    std::vector<std::string> required_capability_ids;
    std::vector<std::string> validation_recipe_ids;
    std::vector<std::string> package_evidence_ids;
    std::vector<std::string> claimed_scope_ids;
    std::uint32_t source_index{0};
};

struct GameplayAuthoringAcceptedFeatureRow {
    std::string feature_id;
    std::string gameplay_family;
    std::vector<std::string> required_capability_ids;
    std::vector<std::string> validation_recipe_ids;
    std::vector<std::string> package_evidence_ids;
    std::uint32_t source_index{0};
};

struct GameplayAuthoringRemediationRow {
    std::string feature_id;
    std::string remediation_kind;
    std::string referenced_id;
    std::string message;
    std::uint32_t source_index{0};
};

struct GameplayAuthoringMutationLedgerRow {
    std::string ledger_id;
    std::string feature_id;
    std::string action;
    std::vector<std::string> required_capability_ids;
    std::vector<std::string> validation_recipe_ids;
    std::vector<std::string> package_evidence_ids;
    std::uint32_t source_index{0};
};

struct GameplayAuthoringDiagnostic {
    std::string code;
    std::string message;
    std::string feature_id;
    std::string referenced_id;
    std::uint32_t source_index{0};
};

struct GameplayAuthoringReviewRequest {
    GameplayAuthoringCapabilityProfile profile;
    std::vector<GameplayAuthoringRequestedFeatureRow> features;
};

struct GameplayAuthoringReviewResult {
    std::vector<GameplayAuthoringAcceptedFeatureRow> accepted_features;
    std::vector<GameplayAuthoringRemediationRow> remediation_rows;
    std::vector<GameplayAuthoringMutationLedgerRow> mutation_ledger_rows;
    std::vector<GameplayAuthoringDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] GameplayAuthoringReviewResult
review_gameplay_authoring_request(const GameplayAuthoringReviewRequest& request);

struct EngineCapabilityHandoffRequestRow {
    std::string handoff_id;
    std::string requested_capability_id;
    std::string blocked_feature_id;
    std::string current_workaround;
    std::vector<std::string> affected_game_files;
    std::string desired_public_contract;
    std::vector<std::string> required_evidence_ids;
    std::uint32_t source_index{0};
};

struct EngineCapabilityHandoffRow {
    std::string handoff_id;
    std::string requested_capability_id;
    std::string blocked_feature_id;
    std::string current_workaround;
    std::vector<std::string> affected_game_files;
    std::string desired_public_contract;
    std::vector<std::string> required_evidence_ids;
    std::string owner;
    std::string next_action;
    std::uint32_t source_index{0};
};

struct EngineCapabilityHandoffDiagnostic {
    std::string code;
    std::string message;
    std::string handoff_id;
    std::string referenced_id;
    std::uint32_t source_index{0};
};

struct EngineCapabilityHandoffReviewRequest {
    std::vector<std::string> canonical_capability_ids;
    std::vector<EngineCapabilityHandoffRequestRow> handoffs;
};

struct EngineCapabilityHandoffReviewResult {
    std::vector<EngineCapabilityHandoffRow> accepted_handoffs;
    std::vector<EngineCapabilityHandoffDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] EngineCapabilityHandoffReviewResult
review_engine_capability_handoff_request(const EngineCapabilityHandoffReviewRequest& request);

} // namespace mirakana
