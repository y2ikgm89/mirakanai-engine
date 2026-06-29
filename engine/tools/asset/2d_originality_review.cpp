// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/2d_originality_review.hpp"

#include <string>
#include <utility>

namespace mirakana {
namespace {

void add_diagnostic(TwoDOriginalityReviewResult& result, const TwoDOriginalitySourceRow& row, std::string code,
                    std::string message) {
    result.diagnostics.push_back(TwoDOriginalityDiagnostic{
        .code = std::move(code),
        .message = std::move(message),
        .source_id = row.id,
        .source_uri = row.source_uri,
    });
}

[[nodiscard]] bool is_prohibited_kind(TwoDOriginalitySourceKind kind) noexcept {
    return kind == TwoDOriginalitySourceKind::prohibited_external_engine_code ||
           kind == TwoDOriginalitySourceKind::prohibited_external_engine_asset ||
           kind == TwoDOriginalitySourceKind::prohibited_external_engine_schema ||
           kind == TwoDOriginalitySourceKind::prohibited_trademark_surface;
}

[[nodiscard]] std::string prohibited_kind_code(TwoDOriginalitySourceKind kind) {
    switch (kind) {
    case TwoDOriginalitySourceKind::prohibited_external_engine_code:
        return "prohibited_external_engine_code";
    case TwoDOriginalitySourceKind::prohibited_external_engine_asset:
        return "prohibited_external_engine_asset";
    case TwoDOriginalitySourceKind::prohibited_external_engine_schema:
        return "prohibited_external_engine_schema";
    case TwoDOriginalitySourceKind::prohibited_trademark_surface:
        return "prohibited_trademark_surface";
    case TwoDOriginalitySourceKind::first_party_design:
    case TwoDOriginalitySourceKind::official_documentation_category:
    case TwoDOriginalitySourceKind::official_platform_sdk:
    case TwoDOriginalitySourceKind::permissive_dependency_notice_record:
    case TwoDOriginalitySourceKind::unknown:
        break;
    }
    return "unknown_source_kind";
}

[[nodiscard]] std::string prohibited_kind_message(TwoDOriginalitySourceKind kind) {
    switch (kind) {
    case TwoDOriginalitySourceKind::prohibited_external_engine_code:
        return "external engine source code must not be used as implementation input";
    case TwoDOriginalitySourceKind::prohibited_external_engine_asset:
        return "external engine assets, samples, themes, templates, or screenshots must not be reused";
    case TwoDOriginalitySourceKind::prohibited_external_engine_schema:
        return "external engine project, scene, or asset schemas must not be imported or copied";
    case TwoDOriginalitySourceKind::prohibited_trademark_surface:
        return "external engine trademarks must not appear in public product, API, or package surfaces";
    case TwoDOriginalitySourceKind::first_party_design:
    case TwoDOriginalitySourceKind::official_documentation_category:
    case TwoDOriginalitySourceKind::official_platform_sdk:
    case TwoDOriginalitySourceKind::permissive_dependency_notice_record:
    case TwoDOriginalitySourceKind::unknown:
        break;
    }
    return "source kind is not accepted by the 2D originality review";
}

void increment_accepted_counter(TwoDOriginalityReviewResult& result, TwoDOriginalitySourceKind kind) noexcept {
    switch (kind) {
    case TwoDOriginalitySourceKind::first_party_design:
        ++result.first_party_design_rows;
        break;
    case TwoDOriginalitySourceKind::official_documentation_category:
        ++result.official_documentation_category_rows;
        break;
    case TwoDOriginalitySourceKind::official_platform_sdk:
        ++result.official_platform_sdk_rows;
        break;
    case TwoDOriginalitySourceKind::permissive_dependency_notice_record:
        ++result.permissive_notice_rows;
        break;
    case TwoDOriginalitySourceKind::prohibited_external_engine_code:
    case TwoDOriginalitySourceKind::prohibited_external_engine_asset:
    case TwoDOriginalitySourceKind::prohibited_external_engine_schema:
    case TwoDOriginalitySourceKind::prohibited_trademark_surface:
    case TwoDOriginalitySourceKind::unknown:
        break;
    }
}

[[nodiscard]] bool review_row(TwoDOriginalityReviewResult& result, const TwoDOriginalitySourceRow& row) {
    bool rejected = false;
    if (row.id.empty()) {
        add_diagnostic(result, row, "missing_source_id", "2D originality source id is required");
        rejected = true;
    }
    if (row.source_uri.empty()) {
        add_diagnostic(result, row, "missing_source_uri", "2D originality source URI is required");
        rejected = true;
    }
    if (row.usage_scope.empty()) {
        add_diagnostic(result, row, "missing_usage_scope", "2D originality source usage scope is required");
        rejected = true;
    }
    if (row.kind == TwoDOriginalitySourceKind::unknown) {
        add_diagnostic(result, row, "unknown_source_kind", "2D originality source kind must be explicit");
        rejected = true;
    }

    if (is_prohibited_kind(row.kind)) {
        add_diagnostic(result, row, prohibited_kind_code(row.kind), prohibited_kind_message(row.kind));
        rejected = true;
    }
    if (row.copied_code) {
        ++result.copied_code_rows;
        if (row.kind != TwoDOriginalitySourceKind::prohibited_external_engine_code) {
            add_diagnostic(result, row, "copied_code", "copied external code is not accepted");
        }
        rejected = true;
    } else if (row.kind == TwoDOriginalitySourceKind::prohibited_external_engine_code) {
        ++result.copied_code_rows;
    }

    if (row.copied_asset) {
        ++result.copied_asset_rows;
        if (row.kind != TwoDOriginalitySourceKind::prohibited_external_engine_asset) {
            add_diagnostic(result, row, "copied_asset", "copied external assets are not accepted");
        }
        rejected = true;
    } else if (row.kind == TwoDOriginalitySourceKind::prohibited_external_engine_asset) {
        ++result.copied_asset_rows;
    }

    if (row.copied_documentation_text) {
        add_diagnostic(result, row, "copied_documentation_text",
                       "documentation may be used only for category research, not copied expression");
        rejected = true;
    }

    if (row.public_surface_uses_external_engine_mark) {
        ++result.trademark_surface_rows;
        if (row.kind != TwoDOriginalitySourceKind::prohibited_trademark_surface) {
            add_diagnostic(result, row, "prohibited_trademark_surface",
                           "external engine marks must not appear in public API or product surfaces");
        }
        rejected = true;
    } else if (row.kind == TwoDOriginalitySourceKind::prohibited_trademark_surface) {
        ++result.trademark_surface_rows;
    }

    if (row.external_engine_compatibility_claim || row.external_engine_equivalence_claim ||
        row.external_engine_parity_claim) {
        ++result.external_engine_claim_rows;
        rejected = true;
        if (row.external_engine_compatibility_claim) {
            add_diagnostic(result, row, "external_engine_compatibility_claim",
                           "Unity, Unreal Engine, Godot, or other external-engine compatibility claims are not "
                           "accepted evidence");
        }
        if (row.external_engine_equivalence_claim) {
            add_diagnostic(result, row, "external_engine_equivalence_claim",
                           "external-engine equivalence claims are not accepted evidence");
        }
        if (row.external_engine_parity_claim) {
            add_diagnostic(result, row, "external_engine_parity_claim",
                           "external-engine parity claims are not accepted evidence");
        }
    }

    return rejected;
}

} // namespace

TwoDOriginalityReviewResult review_2d_originality_sources(std::span<const TwoDOriginalitySourceRow> rows) {
    TwoDOriginalityReviewResult result;
    if (rows.empty()) {
        result.diagnostics.push_back(TwoDOriginalityDiagnostic{
            .code = "missing_source_rows",
            .message = "at least one 2D originality source row is required",
            .source_id = {},
            .source_uri = {},
        });
        return result;
    }

    for (const auto& row : rows) {
        if (review_row(result, row)) {
            result.rejected_rows.push_back(row);
            continue;
        }
        increment_accepted_counter(result, row.kind);
        result.accepted_rows.push_back(row);
    }

    result.clean_room_ready = result.diagnostics.empty();
    return result;
}

TwoDOriginalityReviewResult review_2d_commercial_production_sources(std::span<const TwoDOriginalitySourceRow> rows) {
    auto result = review_2d_originality_sources(rows);

    if (result.first_party_design_rows == 0U) {
        result.diagnostics.push_back(TwoDOriginalityDiagnostic{
            .code = "missing_first_party_design_source",
            .message = "2D commercial production source review requires first-party design evidence",
            .source_id = {},
            .source_uri = {},
        });
    }
    if (result.official_documentation_category_rows == 0U) {
        result.diagnostics.push_back(TwoDOriginalityDiagnostic{
            .code = "missing_official_documentation_category_source",
            .message = "2D commercial production source review requires official documentation category evidence",
            .source_id = {},
            .source_uri = {},
        });
    }
    if (result.official_platform_sdk_rows == 0U) {
        result.diagnostics.push_back(TwoDOriginalityDiagnostic{
            .code = "missing_official_platform_sdk_source",
            .message = "2D commercial production source review requires official platform or SDK evidence",
            .source_id = {},
            .source_uri = {},
        });
    }

    result.clean_room_ready = result.succeeded();
    result.official_source_ledger_ready = result.clean_room_ready && result.first_party_design_rows > 0U &&
                                          result.official_documentation_category_rows > 0U &&
                                          result.official_platform_sdk_rows > 0U;
    result.commercial_production_source_gate_ready =
        result.official_source_ledger_ready && result.external_engine_claim_rows == 0U;
    return result;
}

} // namespace mirakana
