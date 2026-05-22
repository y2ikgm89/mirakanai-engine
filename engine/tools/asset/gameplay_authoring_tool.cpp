// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/gameplay_authoring_tool.hpp"

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

void add_diagnostic(GameplayAuthoringReviewResult& result, std::string code, std::string message,
                    const GameplayAuthoringRequestedFeatureRow& feature, std::string referenced_id = {}) {
    result.diagnostics.push_back(GameplayAuthoringDiagnostic{
        .code = std::move(code),
        .message = std::move(message),
        .feature_id = feature.feature_id,
        .referenced_id = std::move(referenced_id),
        .source_index = feature.source_index,
    });
}

void add_remediation(GameplayAuthoringReviewResult& result, const GameplayAuthoringRequestedFeatureRow& feature,
                     std::string kind, std::string referenced_id, std::string message) {
    result.remediation_rows.push_back(GameplayAuthoringRemediationRow{
        .feature_id = feature.feature_id,
        .remediation_kind = std::move(kind),
        .referenced_id = std::move(referenced_id),
        .message = std::move(message),
        .source_index = feature.source_index,
    });
}

void validate_id_vector(GameplayAuthoringReviewResult& result, const GameplayAuthoringRequestedFeatureRow& feature,
                        const std::vector<std::string>& values, const std::unordered_set<std::string>& supported,
                        std::string_view missing_code, std::string_view remediation_kind,
                        std::string_view missing_message) {
    for (const auto& value : values) {
        if (value.empty() || !supported.contains(value)) {
            add_diagnostic(result, std::string{missing_code}, std::string{missing_message}, feature, value);
            add_remediation(result, feature, std::string{remediation_kind}, value, std::string{missing_message});
        }
    }
}

void add_handoff_diagnostic(EngineCapabilityHandoffReviewResult& result, std::string code, std::string message,
                            const EngineCapabilityHandoffRequestRow& handoff, std::string referenced_id = {}) {
    result.diagnostics.push_back(EngineCapabilityHandoffDiagnostic{
        .code = std::move(code),
        .message = std::move(message),
        .handoff_id = handoff.handoff_id,
        .referenced_id = std::move(referenced_id),
        .source_index = handoff.source_index,
    });
}

[[nodiscard]] bool contains_empty_value(const std::vector<std::string>& values) {
    return std::ranges::any_of(values, [](const auto& value) { return value.empty(); });
}

[[nodiscard]] std::string to_ascii_lowercase(std::string_view value) {
    std::string lowered;
    lowered.reserve(value.size());
    for (const unsigned char character : value) {
        lowered.push_back(static_cast<char>(std::tolower(character)));
    }
    return lowered;
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

[[nodiscard]] bool contains_non_game_owned_file_path(const std::vector<std::string>& values) {
    return std::ranges::any_of(values, [](const auto& value) { return !is_game_owned_file_path(value); });
}

[[nodiscard]] bool contains_unsafe_public_contract_term(std::string_view value) {
    const auto lowered = to_ascii_lowercase(value);
    constexpr std::string_view unsafe_terms[] = {
        "native handle",
        "native handles",
        "native window handle",
        "sdl",
        "sdl3",
        "dear imgui",
        "imgui",
        "irhi",
        "rhi handle",
        "d3d12",
        "id3d12",
        "vulkan",
        "vk",
        "metal",
        "middleware",
        "physx",
        "jolt",
    };
    return std::ranges::any_of(unsafe_terms,
                               [&lowered](std::string_view term) { return lowered.find(term) != std::string::npos; });
}

} // namespace

GameplayAuthoringReviewResult review_gameplay_authoring_request(const GameplayAuthoringReviewRequest& request) {
    GameplayAuthoringReviewResult result;

    const auto supported_capabilities = make_set(request.profile.supported_capability_ids);
    const auto validation_recipes = make_set(request.profile.validation_recipe_ids);
    const auto package_evidence = make_set(request.profile.package_evidence_ids);

    std::unordered_set<std::string> seen_feature_ids;
    for (const auto& feature : request.features) {
        if (feature.feature_id.empty()) {
            add_diagnostic(result, "missing_feature_id", "gameplay authoring feature id is required", feature);
        } else if (!seen_feature_ids.insert(feature.feature_id).second) {
            add_diagnostic(result, "duplicate_feature_id", "gameplay authoring feature id appears more than once",
                           feature, feature.feature_id);
        }

        if (feature.gameplay_family.empty()) {
            add_diagnostic(result, "missing_gameplay_family", "gameplay authoring family is required", feature);
        }
        if (feature.required_capability_ids.empty()) {
            add_diagnostic(result, "missing_required_capability", "at least one supported capability id is required",
                           feature);
            add_remediation(result, feature, "capability", {},
                            "select a supported gameplay capability before authoring this feature");
        }

        validate_id_vector(result, feature, feature.required_capability_ids, supported_capabilities,
                           "missing_required_capability", "capability",
                           "requested gameplay capability is not supported by the active profile");
        validate_id_vector(result, feature, feature.validation_recipe_ids, validation_recipes,
                           "missing_validation_recipe", "validation_recipe",
                           "requested validation recipe is not supported by the active profile");
        validate_id_vector(result, feature, feature.package_evidence_ids, package_evidence, "missing_package_evidence",
                           "package_evidence", "requested package evidence is not supported by the active profile");

        for (const auto& claim : feature.claimed_scope_ids) {
            add_diagnostic(result, "unsupported_claim",
                           "gameplay authoring review does not accept broad or unreviewed authoring claims", feature,
                           claim);
            add_remediation(result, feature, "unsupported_claim", claim,
                            "remove the unsupported authoring claim or create a separate developer-owned plan");
        }
    }

    if (!result.diagnostics.empty()) {
        return result;
    }

    result.accepted_features.reserve(request.features.size());
    result.mutation_ledger_rows.reserve(request.features.size());
    for (const auto& feature : request.features) {
        result.accepted_features.push_back(GameplayAuthoringAcceptedFeatureRow{
            .feature_id = feature.feature_id,
            .gameplay_family = feature.gameplay_family,
            .required_capability_ids = feature.required_capability_ids,
            .validation_recipe_ids = feature.validation_recipe_ids,
            .package_evidence_ids = feature.package_evidence_ids,
            .source_index = feature.source_index,
        });
        result.mutation_ledger_rows.push_back(GameplayAuthoringMutationLedgerRow{
            .ledger_id = "gameplay-authoring:" + feature.feature_id,
            .feature_id = feature.feature_id,
            .action = "reviewed-authoring-plan",
            .required_capability_ids = feature.required_capability_ids,
            .validation_recipe_ids = feature.validation_recipe_ids,
            .package_evidence_ids = feature.package_evidence_ids,
            .source_index = feature.source_index,
        });
    }

    return result;
}

EngineCapabilityHandoffReviewResult
review_engine_capability_handoff_request(const EngineCapabilityHandoffReviewRequest& request) {
    EngineCapabilityHandoffReviewResult result;
    const auto canonical_capabilities = make_set(request.canonical_capability_ids);

    std::unordered_set<std::string> seen_handoff_ids;
    for (const auto& handoff : request.handoffs) {
        if (handoff.handoff_id.empty()) {
            add_handoff_diagnostic(result, "missing_handoff_id", "engine capability handoff id is required", handoff);
        } else if (!seen_handoff_ids.insert(handoff.handoff_id).second) {
            add_handoff_diagnostic(result, "duplicate_handoff_id",
                                   "engine capability handoff id appears more than once", handoff, handoff.handoff_id);
        }

        if (handoff.requested_capability_id.empty()) {
            add_handoff_diagnostic(result, "missing_requested_capability",
                                   "requested canonical capability id is required", handoff);
        } else if (!canonical_capabilities.contains(handoff.requested_capability_id)) {
            add_handoff_diagnostic(result, "unsupported_capability_id",
                                   "requested capability must reference a canonical developer-owned backlog row",
                                   handoff, handoff.requested_capability_id);
        }
        if (handoff.blocked_feature_id.empty()) {
            add_handoff_diagnostic(result, "missing_blocked_feature", "blocked game feature id is required", handoff);
        }
        if (handoff.current_workaround.empty()) {
            add_handoff_diagnostic(result, "missing_current_workaround",
                                   "current supported workaround must be recorded", handoff);
        }
        if (handoff.affected_game_files.empty() || contains_empty_value(handoff.affected_game_files)) {
            add_handoff_diagnostic(result, "missing_affected_game_file",
                                   "at least one affected game-owned file must be recorded", handoff);
        } else if (contains_non_game_owned_file_path(handoff.affected_game_files)) {
            for (const auto& affected_file : handoff.affected_game_files) {
                if (!is_game_owned_file_path(affected_file)) {
                    add_handoff_diagnostic(result, "invalid_affected_game_file",
                                           "affected files must be repo-relative paths under games/", handoff,
                                           affected_file);
                }
            }
        }
        if (handoff.desired_public_contract.empty()) {
            add_handoff_diagnostic(result, "missing_desired_public_contract",
                                   "desired first-party public contract must be recorded", handoff);
        } else if (contains_unsafe_public_contract_term(handoff.desired_public_contract)) {
            add_handoff_diagnostic(result, "unsafe_public_contract",
                                   "desired public contract must stay first-party and backend-neutral", handoff,
                                   handoff.desired_public_contract);
        }
        if (handoff.required_evidence_ids.empty() || contains_empty_value(handoff.required_evidence_ids)) {
            add_handoff_diagnostic(result, "missing_required_evidence",
                                   "at least one required evidence id must be recorded", handoff);
        }
    }

    if (!result.diagnostics.empty()) {
        return result;
    }

    result.accepted_handoffs.reserve(request.handoffs.size());
    for (const auto& handoff : request.handoffs) {
        result.accepted_handoffs.push_back(EngineCapabilityHandoffRow{
            .handoff_id = handoff.handoff_id,
            .requested_capability_id = handoff.requested_capability_id,
            .blocked_feature_id = handoff.blocked_feature_id,
            .current_workaround = handoff.current_workaround,
            .affected_game_files = handoff.affected_game_files,
            .desired_public_contract = handoff.desired_public_contract,
            .required_evidence_ids = handoff.required_evidence_ids,
            .owner = "developer-owned-engine-feature",
            .next_action = "create-dated-capability-plan",
            .source_index = handoff.source_index,
        });
    }

    return result;
}

} // namespace mirakana
