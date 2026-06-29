// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/tools/2d_originality_review.hpp"

#include <algorithm>
#include <string_view>
#include <vector>

namespace {

[[nodiscard]] bool has_diagnostic(const std::vector<mirakana::TwoDOriginalityDiagnostic>& diagnostics,
                                  std::string_view code, std::string_view source_id = {}) {
    return std::ranges::any_of(diagnostics, [code, source_id](const auto& diagnostic) {
        return diagnostic.code == code && (source_id.empty() || diagnostic.source_id == source_id);
    });
}

[[nodiscard]] bool has_rejected_source(const mirakana::TwoDOriginalityReviewResult& result, std::string_view id) {
    return std::ranges::any_of(result.rejected_rows, [id](const auto& row) { return row.id == id; });
}

} // namespace

MK_TEST("2d originality review accepts first party official category docs and platform sdk rows") {
    const std::vector<mirakana::TwoDOriginalitySourceRow> rows{
        mirakana::TwoDOriginalitySourceRow{
            .id = "first-party-source-pulse-design",
            .kind = mirakana::TwoDOriginalitySourceKind::first_party_design,
            .source_uri = "docs/superpowers/plans/2026-06-24-original-2d-commercial-authoring-live-iteration-v1.md",
            .usage_scope = "first-party Source Pulse architecture and API plan",
        },
        mirakana::TwoDOriginalitySourceRow{
            .id = "unity-category-reference",
            .kind = mirakana::TwoDOriginalitySourceKind::official_documentation_category,
            .source_uri = "https://unity.com/legal/terms-of-service",
            .usage_scope = "category and legal-boundary research only",
        },
        mirakana::TwoDOriginalitySourceRow{
            .id = "unreal-category-reference",
            .kind = mirakana::TwoDOriginalitySourceKind::official_documentation_category,
            .source_uri = "https://www.unrealengine.com/eula/unreal",
            .usage_scope = "category and legal-boundary research only",
        },
        mirakana::TwoDOriginalitySourceRow{
            .id = "godot-category-reference",
            .kind = mirakana::TwoDOriginalitySourceKind::official_documentation_category,
            .source_uri = "https://godotengine.org/license/",
            .usage_scope = "category and license-boundary research only",
        },
        mirakana::TwoDOriginalitySourceRow{
            .id = "windows-file-watch-sdk",
            .kind = mirakana::TwoDOriginalitySourceKind::official_platform_sdk,
            .source_uri =
                "https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-readdirectorychangesw",
            .usage_scope = "official platform API adapter guidance",
        },
    };

    const auto result = mirakana::review_2d_originality_sources(rows);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.clean_room_ready);
    MK_REQUIRE(result.requires_legal_counsel_review);
    MK_REQUIRE(result.accepted_rows.size() == 5U);
    MK_REQUIRE(result.rejected_rows.empty());
    MK_REQUIRE(result.first_party_design_rows == 1U);
    MK_REQUIRE(result.official_documentation_category_rows == 3U);
    MK_REQUIRE(result.official_platform_sdk_rows == 1U);
    MK_REQUIRE(result.permissive_notice_rows == 0U);
    MK_REQUIRE(result.copied_code_rows == 0U);
    MK_REQUIRE(result.copied_asset_rows == 0U);
    MK_REQUIRE(result.trademark_surface_rows == 0U);
}

MK_TEST("2d originality review rejects unity unreal godot code assets schema or copied docs") {
    const std::vector<mirakana::TwoDOriginalitySourceRow> rows{
        mirakana::TwoDOriginalitySourceRow{
            .id = "unity-source-copy",
            .kind = mirakana::TwoDOriginalitySourceKind::prohibited_external_engine_code,
            .source_uri = "https://example.invalid/unity/source",
            .usage_scope = "copied implementation",
            .copied_code = true,
        },
        mirakana::TwoDOriginalitySourceRow{
            .id = "unreal-sample-asset",
            .kind = mirakana::TwoDOriginalitySourceKind::prohibited_external_engine_asset,
            .source_uri = "https://example.invalid/unreal/asset",
            .usage_scope = "sample asset reuse",
            .copied_asset = true,
        },
        mirakana::TwoDOriginalitySourceRow{
            .id = "godot-scene-schema",
            .kind = mirakana::TwoDOriginalitySourceKind::prohibited_external_engine_schema,
            .source_uri = "https://example.invalid/godot/schema",
            .usage_scope = "external scene schema parser",
        },
        mirakana::TwoDOriginalitySourceRow{
            .id = "copied-doc-example",
            .kind = mirakana::TwoDOriginalitySourceKind::official_documentation_category,
            .source_uri = "https://example.invalid/docs",
            .usage_scope = "copied documentation example",
            .copied_documentation_text = true,
        },
    };

    const auto result = mirakana::review_2d_originality_sources(rows);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.clean_room_ready);
    MK_REQUIRE(result.accepted_rows.empty());
    MK_REQUIRE(result.rejected_rows.size() == 4U);
    MK_REQUIRE(result.copied_code_rows == 1U);
    MK_REQUIRE(result.copied_asset_rows == 1U);
    MK_REQUIRE(has_rejected_source(result, "unity-source-copy"));
    MK_REQUIRE(has_rejected_source(result, "unreal-sample-asset"));
    MK_REQUIRE(has_rejected_source(result, "godot-scene-schema"));
    MK_REQUIRE(has_rejected_source(result, "copied-doc-example"));
    MK_REQUIRE(has_diagnostic(result.diagnostics, "prohibited_external_engine_code", "unity-source-copy"));
    MK_REQUIRE(has_diagnostic(result.diagnostics, "prohibited_external_engine_asset", "unreal-sample-asset"));
    MK_REQUIRE(has_diagnostic(result.diagnostics, "prohibited_external_engine_schema", "godot-scene-schema"));
    MK_REQUIRE(has_diagnostic(result.diagnostics, "copied_documentation_text", "copied-doc-example"));
}

MK_TEST("2d originality review rejects external engine trademarks in public surfaces") {
    const std::vector<mirakana::TwoDOriginalitySourceRow> rows{
        mirakana::TwoDOriginalitySourceRow{
            .id = "unity-compatible-public-api",
            .kind = mirakana::TwoDOriginalitySourceKind::prohibited_trademark_surface,
            .source_uri = "docs/api/source_pulse.md",
            .usage_scope = "Unity-compatible importer public API",
            .public_surface_uses_external_engine_mark = true,
        },
    };

    const auto result = mirakana::review_2d_originality_sources(rows);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.clean_room_ready);
    MK_REQUIRE(result.trademark_surface_rows == 1U);
    MK_REQUIRE(has_rejected_source(result, "unity-compatible-public-api"));
    MK_REQUIRE(has_diagnostic(result.diagnostics, "prohibited_trademark_surface", "unity-compatible-public-api"));
}

MK_TEST("2d originality review requires legal counsel review even when engineering gate is ready") {
    const std::vector<mirakana::TwoDOriginalitySourceRow> rows{
        mirakana::TwoDOriginalitySourceRow{
            .id = "first-party-plan",
            .kind = mirakana::TwoDOriginalitySourceKind::first_party_design,
            .source_uri = "docs/superpowers/plans/2026-06-24-original-2d-commercial-authoring-live-iteration-v1.md",
            .usage_scope = "first-party plan",
        },
    };

    const auto result = mirakana::review_2d_originality_sources(rows);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.clean_room_ready);
    MK_REQUIRE(result.requires_legal_counsel_review);
}

MK_TEST("2d originality review treats godot mit notice as future notice only when no code is copied") {
    const std::vector<mirakana::TwoDOriginalitySourceRow> rows{
        mirakana::TwoDOriginalitySourceRow{
            .id = "godot-license-category-only",
            .kind = mirakana::TwoDOriginalitySourceKind::permissive_dependency_notice_record,
            .source_uri = "https://godotengine.org/license/",
            .usage_scope = "MIT license boundary research only; no source or asset copied",
        },
    };

    const auto result = mirakana::review_2d_originality_sources(rows);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.clean_room_ready);
    MK_REQUIRE(result.permissive_notice_rows == 1U);
    MK_REQUIRE(result.copied_code_rows == 0U);
    MK_REQUIRE(result.copied_asset_rows == 0U);
    MK_REQUIRE(result.trademark_surface_rows == 0U);
    MK_REQUIRE(result.requires_legal_counsel_review);
}

MK_TEST("2d commercial production source review requires first party official docs and platform sdk rows") {
    const std::vector<mirakana::TwoDOriginalitySourceRow> rows{
        mirakana::TwoDOriginalitySourceRow{
            .id = "first-party-commercial-plan",
            .kind = mirakana::TwoDOriginalitySourceKind::first_party_design,
            .source_uri = "docs/superpowers/plans/2026-06-29-2d-commercial-production-excellence-v1.md",
            .usage_scope = "first-party commercial production plan",
        },
    };

    const auto result = mirakana::review_2d_commercial_production_sources(rows);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.clean_room_ready);
    MK_REQUIRE(!result.official_source_ledger_ready);
    MK_REQUIRE(!result.commercial_production_source_gate_ready);
    MK_REQUIRE(result.first_party_design_rows == 1U);
    MK_REQUIRE(result.official_documentation_category_rows == 0U);
    MK_REQUIRE(result.official_platform_sdk_rows == 0U);
    MK_REQUIRE(has_diagnostic(result.diagnostics, "missing_official_documentation_category_source"));
    MK_REQUIRE(has_diagnostic(result.diagnostics, "missing_official_platform_sdk_source"));
}

MK_TEST("2d commercial production source review accepts official ledger while preserving counsel review") {
    const std::vector<mirakana::TwoDOriginalitySourceRow> rows{
        mirakana::TwoDOriginalitySourceRow{
            .id = "first-party-commercial-plan",
            .kind = mirakana::TwoDOriginalitySourceKind::first_party_design,
            .source_uri = "docs/superpowers/plans/2026-06-29-2d-commercial-production-excellence-v1.md",
            .usage_scope = "first-party commercial production plan",
        },
        mirakana::TwoDOriginalitySourceRow{
            .id = "vulkan-official-sync-source",
            .kind = mirakana::TwoDOriginalitySourceKind::official_platform_sdk,
            .source_uri = "https://docs.vulkan.org/",
            .usage_scope = "official graphics backend synchronization and timestamp-query guidance only",
        },
        mirakana::TwoDOriginalitySourceRow{
            .id = "unity-legal-boundary",
            .kind = mirakana::TwoDOriginalitySourceKind::official_documentation_category,
            .source_uri = "https://unity.com/legal/branding-trademarks",
            .usage_scope = "legal and category boundary research only; no implementation source",
        },
        mirakana::TwoDOriginalitySourceRow{
            .id = "unreal-legal-boundary",
            .kind = mirakana::TwoDOriginalitySourceKind::official_documentation_category,
            .source_uri = "https://www.unrealengine.com/eula/unreal",
            .usage_scope = "legal and category boundary research only; no implementation source",
        },
        mirakana::TwoDOriginalitySourceRow{
            .id = "godot-license-boundary",
            .kind = mirakana::TwoDOriginalitySourceKind::official_documentation_category,
            .source_uri = "https://godotengine.org/license/",
            .usage_scope = "license boundary research only; no code or asset copied",
        },
    };

    const auto result = mirakana::review_2d_commercial_production_sources(rows);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.clean_room_ready);
    MK_REQUIRE(result.official_source_ledger_ready);
    MK_REQUIRE(result.commercial_production_source_gate_ready);
    MK_REQUIRE(result.requires_legal_counsel_review);
    MK_REQUIRE(result.first_party_design_rows == 1U);
    MK_REQUIRE(result.official_platform_sdk_rows == 1U);
    MK_REQUIRE(result.official_documentation_category_rows == 3U);
    MK_REQUIRE(result.external_engine_claim_rows == 0U);
}

MK_TEST("2d commercial production source review rejects compatibility equivalence and parity claims") {
    const std::vector<mirakana::TwoDOriginalitySourceRow> rows{
        mirakana::TwoDOriginalitySourceRow{
            .id = "unity-compatible-package-output",
            .kind = mirakana::TwoDOriginalitySourceKind::official_documentation_category,
            .source_uri = "https://unity.com/legal/branding-trademarks",
            .usage_scope = "claims Unity compatibility in public package output",
            .external_engine_compatibility_claim = true,
        },
        mirakana::TwoDOriginalitySourceRow{
            .id = "unreal-equivalent-editor-flow",
            .kind = mirakana::TwoDOriginalitySourceKind::first_party_design,
            .source_uri = "docs/superpowers/plans/2026-06-29-2d-commercial-production-excellence-v1.md",
            .usage_scope = "claims Unreal equivalence in public editor flow",
            .external_engine_equivalence_claim = true,
        },
        mirakana::TwoDOriginalitySourceRow{
            .id = "godot-parity-runtime-api",
            .kind = mirakana::TwoDOriginalitySourceKind::official_documentation_category,
            .source_uri = "https://godotengine.org/license/",
            .usage_scope = "claims Godot parity in public runtime API",
            .external_engine_parity_claim = true,
        },
    };

    const auto result = mirakana::review_2d_commercial_production_sources(rows);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.clean_room_ready);
    MK_REQUIRE(!result.official_source_ledger_ready);
    MK_REQUIRE(!result.commercial_production_source_gate_ready);
    MK_REQUIRE(result.external_engine_claim_rows == 3U);
    MK_REQUIRE(has_rejected_source(result, "unity-compatible-package-output"));
    MK_REQUIRE(has_rejected_source(result, "unreal-equivalent-editor-flow"));
    MK_REQUIRE(has_rejected_source(result, "godot-parity-runtime-api"));
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, "external_engine_compatibility_claim", "unity-compatible-package-output"));
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, "external_engine_equivalence_claim", "unreal-equivalent-editor-flow"));
    MK_REQUIRE(has_diagnostic(result.diagnostics, "external_engine_parity_claim", "godot-parity-runtime-api"));
}

int main() {
    return mirakana::test::run_all();
}
