// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana {

enum class TwoDOriginalitySourceKind : std::uint8_t {
    first_party_design,
    official_documentation_category,
    official_platform_sdk,
    permissive_dependency_notice_record,
    prohibited_external_engine_code,
    prohibited_external_engine_asset,
    prohibited_external_engine_schema,
    prohibited_trademark_surface,
    unknown,
};

struct TwoDOriginalitySourceRow {
    std::string id;
    TwoDOriginalitySourceKind kind{TwoDOriginalitySourceKind::unknown};
    std::string source_uri;
    std::string usage_scope;
    bool copied_code{false};
    bool copied_asset{false};
    bool copied_documentation_text{false};
    bool public_surface_uses_external_engine_mark{false};
    bool external_engine_compatibility_claim{false};
    bool external_engine_equivalence_claim{false};
    bool external_engine_parity_claim{false};
};

struct TwoDOriginalityDiagnostic {
    std::string code;
    std::string message;
    std::string source_id;
    std::string source_uri;
};

struct TwoDOriginalityReviewResult {
    std::vector<TwoDOriginalitySourceRow> accepted_rows;
    std::vector<TwoDOriginalitySourceRow> rejected_rows;
    std::vector<TwoDOriginalityDiagnostic> diagnostics;
    std::size_t official_documentation_category_rows{0};
    std::size_t official_platform_sdk_rows{0};
    std::size_t first_party_design_rows{0};
    std::size_t permissive_notice_rows{0};
    std::size_t copied_code_rows{0};
    std::size_t copied_asset_rows{0};
    std::size_t trademark_surface_rows{0};
    std::size_t external_engine_claim_rows{0};
    // True when a commercial review has clean first-party, official-doc, and platform-SDK source evidence.
    bool official_source_ledger_ready{false};
    // True when the commercial production source ledger is ready and contains no external-engine product claims.
    bool commercial_production_source_gate_ready{false};
    // True when the selected review mode has no diagnostics.
    bool clean_room_ready{false};
    // This review is an engineering gate; final release decisions still require counsel review.
    bool requires_legal_counsel_review{true};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] TwoDOriginalityReviewResult review_2d_originality_sources(std::span<const TwoDOriginalitySourceRow> rows);
[[nodiscard]] TwoDOriginalityReviewResult
review_2d_commercial_production_sources(std::span<const TwoDOriginalitySourceRow> rows);

} // namespace mirakana
