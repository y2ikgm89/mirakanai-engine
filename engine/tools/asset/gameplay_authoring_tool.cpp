// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/gameplay_authoring_tool.hpp"

#include <algorithm>
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

} // namespace mirakana
