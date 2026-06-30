// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/asset_import_preset_diff.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <limits>
#include <locale>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace mirakana {
namespace {

enum class PresetDomain : std::uint8_t { unsupported, texture, mesh, audio };

struct EffectiveTexturePreset {
    AssetImportTexturePresetV1 preset;
    std::vector<std::string> diagnostics;
};

struct EffectiveMeshPreset {
    AssetImportMeshPresetV1 preset;
    std::vector<std::string> diagnostics;
};

struct EffectiveAudioPreset {
    AssetImportAudioPresetV1 preset;
    std::vector<std::string> diagnostics;
};

[[nodiscard]] bool clean_text(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos &&
           value.find('\0') == std::string_view::npos;
}

[[nodiscard]] bool contains_parent_segment(std::string_view value) noexcept {
    std::size_t segment_begin = 0U;
    while (segment_begin <= value.size()) {
        const auto segment_end = value.find('/', segment_begin);
        const auto segment =
            value.substr(segment_begin, segment_end == std::string_view::npos ? value.size() - segment_begin
                                                                              : segment_end - segment_begin);
        if (segment == "..") {
            return true;
        }
        if (segment_end == std::string_view::npos) {
            break;
        }
        segment_begin = segment_end + 1U;
    }
    return false;
}

[[nodiscard]] bool safe_token(std::string_view value) noexcept {
    return clean_text(value) && value.find('\\') == std::string_view::npos && value.find(':') == std::string_view::npos;
}

[[nodiscard]] bool safe_relative_path(std::string_view value) noexcept {
    return safe_token(value) && value.front() != '/' && !contains_parent_segment(value);
}

[[nodiscard]] std::string float_text(float value) {
    if (!std::isfinite(value)) {
        return "invalid";
    }
    std::ostringstream output;
    output.imbue(std::locale::classic());
    output << std::setprecision(std::numeric_limits<float>::max_digits10) << value;
    return output.str();
}

[[nodiscard]] std::string bool_text(bool value) {
    return value ? "true" : "false";
}

[[nodiscard]] PresetDomain preset_domain_for_action(AssetImportActionKind kind) noexcept {
    switch (kind) {
    case AssetImportActionKind::texture:
    case AssetImportActionKind::environment_profile:
        return PresetDomain::texture;
    case AssetImportActionKind::mesh:
    case AssetImportActionKind::morph_mesh_cpu:
    case AssetImportActionKind::animation_float_clip:
    case AssetImportActionKind::animation_quaternion_clip:
        return PresetDomain::mesh;
    case AssetImportActionKind::audio:
        return PresetDomain::audio;
    case AssetImportActionKind::material:
    case AssetImportActionKind::scene:
    case AssetImportActionKind::unknown:
        break;
    }
    return PresetDomain::unsupported;
}

[[nodiscard]] AssetKind review_kind_for_domain(PresetDomain domain) noexcept {
    switch (domain) {
    case PresetDomain::texture:
        return AssetKind::texture;
    case PresetDomain::mesh:
        return AssetKind::mesh;
    case PresetDomain::audio:
        return AssetKind::audio;
    case PresetDomain::unsupported:
        break;
    }
    return AssetKind::unknown;
}

[[nodiscard]] bool legal_blocked_code(AssetImportRegressionDiagnosticCode code) noexcept {
    switch (code) {
    case AssetImportRegressionDiagnosticCode::missing_license_provenance:
    case AssetImportRegressionDiagnosticCode::rejected_license:
    case AssetImportRegressionDiagnosticCode::external_engine_material:
        return true;
    case AssetImportRegressionDiagnosticCode::none:
    case AssetImportRegressionDiagnosticCode::invalid_manifest:
    case AssetImportRegressionDiagnosticCode::duplicate_asset_id:
    case AssetImportRegressionDiagnosticCode::unsafe_source_path:
    case AssetImportRegressionDiagnosticCode::missing_source_file:
    case AssetImportRegressionDiagnosticCode::source_hash_mismatch:
    case AssetImportRegressionDiagnosticCode::unsupported_format:
    case AssetImportRegressionDiagnosticCode::parser_error:
    case AssetImportRegressionDiagnosticCode::validator_error:
    case AssetImportRegressionDiagnosticCode::missing_external_resource:
    case AssetImportRegressionDiagnosticCode::unsafe_external_resource_path:
    case AssetImportRegressionDiagnosticCode::unsupported_extension:
    case AssetImportRegressionDiagnosticCode::unsupported_animation_channel:
    case AssetImportRegressionDiagnosticCode::unsupported_skin_or_morph_combination:
    case AssetImportRegressionDiagnosticCode::coordinate_normalization_failed:
    case AssetImportRegressionDiagnosticCode::material_extraction_failed:
    case AssetImportRegressionDiagnosticCode::texture_decode_failed:
    case AssetImportRegressionDiagnosticCode::texture_transcode_failed:
    case AssetImportRegressionDiagnosticCode::cooked_output_mismatch:
    case AssetImportRegressionDiagnosticCode::nondeterministic_output:
    case AssetImportRegressionDiagnosticCode::row_budget_exceeded:
        break;
    }
    return false;
}

[[nodiscard]] const AssetImportPresetOverrideV1* matching_override(const AssetImportPresetsDocumentV1& document,
                                                                   const AssetKeyV2& key) noexcept {
    const auto it = std::ranges::find_if(document.overrides, [&key](const AssetImportPresetOverrideV1& row) {
        return row.asset_key.value == key.value;
    });
    return it == document.overrides.end() ? nullptr : &*it;
}

[[nodiscard]] EffectiveTexturePreset effective_texture(const AssetImportPresetsDocumentV1& document,
                                                       const AssetKeyV2& key) {
    EffectiveTexturePreset result{.preset = document.defaults.texture};
    if (const auto* row = matching_override(document, key); row != nullptr && row->texture.has_value()) {
        result.preset = *row->texture;
    }
    const auto review = review_asset_import_preset_for_asset(document, key, AssetKind::texture);
    result.diagnostics = review.diagnostics;
    return result;
}

[[nodiscard]] EffectiveMeshPreset effective_mesh(const AssetImportPresetsDocumentV1& document, const AssetKeyV2& key) {
    EffectiveMeshPreset result{.preset = document.defaults.mesh};
    if (const auto* row = matching_override(document, key); row != nullptr && row->mesh.has_value()) {
        result.preset = *row->mesh;
    }
    const auto review = review_asset_import_preset_for_asset(document, key, AssetKind::mesh);
    result.diagnostics = review.diagnostics;
    return result;
}

[[nodiscard]] EffectiveAudioPreset effective_audio(const AssetImportPresetsDocumentV1& document,
                                                   const AssetKeyV2& key) {
    EffectiveAudioPreset result{.preset = document.defaults.audio};
    if (const auto* row = matching_override(document, key); row != nullptr && row->audio.has_value()) {
        result.preset = *row->audio;
    }
    const auto review = review_asset_import_preset_for_asset(document, key, AssetKind::audio);
    result.diagnostics = review.diagnostics;
    return result;
}

void add_impact_flags(AssetImportPresetDiffRow& row, const AssetImportPresetDiffFieldChange& change) {
    if (std::ranges::find(change.impacts, AssetImportPresetDiffImpact::source_document) != change.impacts.end()) {
        row.source_review_required = true;
    }
    if (std::ranges::find(change.impacts, AssetImportPresetDiffImpact::cooked_output) != change.impacts.end()) {
        row.cooked_output_changes = true;
    }
    if (std::ranges::find(change.impacts, AssetImportPresetDiffImpact::package_output) != change.impacts.end()) {
        row.package_output_changes = true;
    }
}

void push_change(AssetImportPresetDiffRow& row, AssetImportPresetDiffFieldChange change) {
    add_impact_flags(row, change);
    row.field_changes.push_back(std::move(change));
}

[[nodiscard]] std::vector<AssetImportPresetDiffImpact> cooked_package_impacts() {
    return {AssetImportPresetDiffImpact::cooked_output, AssetImportPresetDiffImpact::package_output};
}

[[nodiscard]] std::vector<AssetImportPresetDiffImpact> source_cooked_package_impacts() {
    return {
        AssetImportPresetDiffImpact::source_document,
        AssetImportPresetDiffImpact::cooked_output,
        AssetImportPresetDiffImpact::package_output,
    };
}

void diff_texture_fields(AssetImportPresetDiffRow& row, const AssetImportTexturePresetV1& before,
                         const AssetImportTexturePresetV1& after) {
    if (before.color_space != after.color_space) {
        push_change(row, AssetImportPresetDiffFieldChange{
                             .field = "texture.color_space",
                             .before_value = std::string{asset_import_texture_color_space_label(before.color_space)},
                             .after_value = std::string{asset_import_texture_color_space_label(after.color_space)},
                             .impacts = source_cooked_package_impacts(),
                         });
    }
    if (before.mipmap_policy != after.mipmap_policy) {
        push_change(row,
                    AssetImportPresetDiffFieldChange{
                        .field = "texture.mipmap_policy",
                        .before_value = std::string{asset_import_texture_mipmap_policy_label(before.mipmap_policy)},
                        .after_value = std::string{asset_import_texture_mipmap_policy_label(after.mipmap_policy)},
                        .impacts = cooked_package_impacts(),
                    });
    }
    if (before.alpha_policy != after.alpha_policy) {
        push_change(row, AssetImportPresetDiffFieldChange{
                             .field = "texture.alpha_policy",
                             .before_value = std::string{asset_import_texture_alpha_policy_label(before.alpha_policy)},
                             .after_value = std::string{asset_import_texture_alpha_policy_label(after.alpha_policy)},
                             .impacts = source_cooked_package_impacts(),
                         });
    }
    if (before.compression_intent != after.compression_intent) {
        push_change(
            row,
            AssetImportPresetDiffFieldChange{
                .field = "texture.compression_intent",
                .before_value = std::string{asset_import_texture_compression_intent_label(before.compression_intent)},
                .after_value = std::string{asset_import_texture_compression_intent_label(after.compression_intent)},
                .impacts = source_cooked_package_impacts(),
            });
    }
}

void diff_mesh_fields(AssetImportPresetDiffRow& row, const AssetImportMeshPresetV1& before,
                      const AssetImportMeshPresetV1& after) {
    if (before.unit_scale != after.unit_scale) {
        push_change(row, AssetImportPresetDiffFieldChange{
                             .field = "mesh.unit_scale",
                             .before_value = float_text(before.unit_scale),
                             .after_value = float_text(after.unit_scale),
                             .impacts = cooked_package_impacts(),
                         });
    }
    if (before.up_axis != after.up_axis) {
        push_change(row, AssetImportPresetDiffFieldChange{
                             .field = "mesh.up_axis",
                             .before_value = std::string{asset_import_mesh_up_axis_label(before.up_axis)},
                             .after_value = std::string{asset_import_mesh_up_axis_label(after.up_axis)},
                             .impacts = cooked_package_impacts(),
                         });
    }
    if (before.triangulate != after.triangulate) {
        push_change(row, AssetImportPresetDiffFieldChange{
                             .field = "mesh.triangulate",
                             .before_value = bool_text(before.triangulate),
                             .after_value = bool_text(after.triangulate),
                             .impacts = cooked_package_impacts(),
                         });
    }
    if (before.generate_normals != after.generate_normals) {
        push_change(row, AssetImportPresetDiffFieldChange{
                             .field = "mesh.generate_normals",
                             .before_value = bool_text(before.generate_normals),
                             .after_value = bool_text(after.generate_normals),
                             .impacts = cooked_package_impacts(),
                         });
    }
    if (before.generate_tangents != after.generate_tangents) {
        push_change(row, AssetImportPresetDiffFieldChange{
                             .field = "mesh.generate_tangents",
                             .before_value = bool_text(before.generate_tangents),
                             .after_value = bool_text(after.generate_tangents),
                             .impacts = cooked_package_impacts(),
                         });
    }
    if (before.material_extraction != after.material_extraction) {
        push_change(
            row,
            AssetImportPresetDiffFieldChange{
                .field = "mesh.material_extraction",
                .before_value = std::string{asset_import_mesh_material_extraction_label(before.material_extraction)},
                .after_value = std::string{asset_import_mesh_material_extraction_label(after.material_extraction)},
                .impacts = source_cooked_package_impacts(),
            });
    }
}

void diff_audio_fields(AssetImportPresetDiffRow& row, const AssetImportAudioPresetV1& before,
                       const AssetImportAudioPresetV1& after) {
    if (before.decode_mode != after.decode_mode) {
        push_change(row, AssetImportPresetDiffFieldChange{
                             .field = "audio.decode_mode",
                             .before_value = std::string{asset_import_audio_decode_mode_label(before.decode_mode)},
                             .after_value = std::string{asset_import_audio_decode_mode_label(after.decode_mode)},
                             .impacts = source_cooked_package_impacts(),
                         });
    }
    if (before.sample_format != after.sample_format) {
        push_change(row, AssetImportPresetDiffFieldChange{
                             .field = "audio.sample_format",
                             .before_value = std::string{asset_import_audio_sample_format_label(before.sample_format)},
                             .after_value = std::string{asset_import_audio_sample_format_label(after.sample_format)},
                             .impacts = cooked_package_impacts(),
                         });
    }
    if (before.loop != after.loop) {
        push_change(row, AssetImportPresetDiffFieldChange{
                             .field = "audio.loop",
                             .before_value = bool_text(before.loop),
                             .after_value = bool_text(after.loop),
                             .impacts = cooked_package_impacts(),
                         });
    }
    if (before.normalize_peak != after.normalize_peak) {
        push_change(row, AssetImportPresetDiffFieldChange{
                             .field = "audio.normalize_peak",
                             .before_value = bool_text(before.normalize_peak),
                             .after_value = bool_text(after.normalize_peak),
                             .impacts = source_cooked_package_impacts(),
                         });
    }
}

void mark_review_blocked(AssetImportPresetDiffRow& row, std::string diagnostic) {
    row.status = AssetImportPresetDiffRowStatus::review_blocked;
    row.review_blocked = true;
    row.diagnostics.push_back(std::move(diagnostic));
}

void finalize_status(AssetImportPresetDiffRow& row) {
    std::ranges::sort(row.field_changes, {}, &AssetImportPresetDiffFieldChange::field);
    if (row.review_blocked) {
        row.status = AssetImportPresetDiffRowStatus::review_blocked;
    } else if (!row.field_changes.empty()) {
        row.status = AssetImportPresetDiffRowStatus::changed;
    } else {
        row.status = AssetImportPresetDiffRowStatus::unchanged;
    }
}

using ActionByAsset = std::unordered_map<AssetId, const AssetImportAction*, AssetIdHash>;

[[nodiscard]] ActionByAsset map_actions(const AssetImportPlan& import_plan, AssetImportPresetDiff& diff) {
    ActionByAsset actions;
    actions.reserve(import_plan.actions.size());
    for (const auto& action : import_plan.actions) {
        if (!is_valid_asset_import_action(action)) {
            diff.diagnostics.push_back("import_plan.invalid_action");
            continue;
        }
        const auto [_, inserted] = actions.emplace(action.id, &action);
        if (!inserted) {
            diff.diagnostics.push_back("import_plan.duplicate_asset_action");
        }
    }
    return actions;
}

using ReportByAsset = std::unordered_map<AssetId, const AssetImportRegressionReportRowV1*, AssetIdHash>;

[[nodiscard]] ReportByAsset map_report_rows(const std::optional<AssetImportRegressionReportV1>& report,
                                            AssetImportPresetDiff& diff) {
    ReportByAsset rows;
    if (!report.has_value()) {
        return rows;
    }

    rows.reserve(report->rows.size());
    for (const auto& row : report->rows) {
        const auto [_, inserted] = rows.emplace(row.asset, &row);
        if (!inserted) {
            diff.diagnostics.push_back("latest_report.duplicate_asset_row");
        }
    }
    return rows;
}

[[nodiscard]] std::vector<AssetImportPresetDiffAssetV1> asset_rows_from_actions(const AssetImportPlan& import_plan) {
    std::vector<AssetImportPresetDiffAssetV1> rows;
    rows.reserve(import_plan.actions.size());
    for (const auto& action : import_plan.actions) {
        rows.push_back(AssetImportPresetDiffAssetV1{
            .asset_id = action.output_path,
            .asset_key = AssetKeyV2{.value = action.output_path},
            .asset = action.id,
        });
    }
    return rows;
}

void append_review_diagnostics(AssetImportPresetDiffRow& row, std::span<const std::string> diagnostics) {
    for (const auto& diagnostic : diagnostics) {
        if (std::ranges::find(row.diagnostics, diagnostic) == row.diagnostics.end()) {
            row.diagnostics.push_back(diagnostic);
        }
    }
    if (!diagnostics.empty()) {
        row.review_blocked = true;
    }
}

void block_from_report_row(AssetImportPresetDiffRow& row, const AssetImportRegressionReportRowV1& report_row) {
    if (legal_blocked_code(report_row.code)) {
        mark_review_blocked(row, "latest_report.legal_policy_blocks_preset_diff");
    } else if (!report_row.succeeded || !report_row.ready_for_commercial_evidence) {
        mark_review_blocked(row, "latest_report.row_not_ready_for_preset_diff");
    }
}

void count_row(AssetImportPresetDiff& diff, const AssetImportPresetDiffRow& row) {
    if (row.status != AssetImportPresetDiffRowStatus::unchanged) {
        ++diff.affected_count;
    }
    if (row.source_review_required) {
        ++diff.source_review_count;
    }
    if (row.cooked_output_changes) {
        ++diff.cooked_output_change_count;
    }
    if (row.package_output_changes) {
        ++diff.package_output_change_count;
    }
    if (row.review_blocked) {
        ++diff.review_blocked_count;
    }
}

} // namespace

AssetImportPresetDiff diff_asset_import_presets(const AssetImportPresetDiffDesc& desc) {
    AssetImportPresetDiff diff;

    const auto before_diagnostics = validate_asset_import_presets_document(desc.before);
    const auto after_diagnostics = validate_asset_import_presets_document(desc.after);
    for (const auto& diagnostic : before_diagnostics) {
        diff.diagnostics.push_back("before." + diagnostic);
    }
    for (const auto& diagnostic : after_diagnostics) {
        diff.diagnostics.push_back("after." + diagnostic);
    }

    const auto actions = map_actions(desc.import_plan, diff);
    const auto report_rows = map_report_rows(desc.latest_report, diff);
    const auto generated_asset_rows = desc.assets.empty() ? asset_rows_from_actions(desc.import_plan) : desc.assets;

    std::unordered_set<AssetId, AssetIdHash> seen_assets;
    seen_assets.reserve(generated_asset_rows.size());
    std::vector<AssetImportPresetDiffRow> rows;
    rows.reserve(generated_asset_rows.size());

    for (const auto& asset : generated_asset_rows) {
        AssetImportPresetDiffRow row{
            .asset_id = asset.asset_id,
            .asset_key = asset.asset_key,
            .asset = asset.asset,
            .corpus_kind = asset.corpus_kind,
        };

        if (!safe_token(row.asset_id)) {
            mark_review_blocked(row, "asset.invalid_asset_id");
        }
        if (!safe_token(row.asset_key.value)) {
            mark_review_blocked(row, "asset.invalid_asset_key");
        }
        if (row.asset.value == 0U || !seen_assets.insert(row.asset).second) {
            mark_review_blocked(row, "asset.duplicate_or_invalid_asset_id");
        }

        const auto action = actions.find(row.asset);
        if (action == actions.end() || action->second == nullptr) {
            mark_review_blocked(row, "import_plan.missing_action");
        } else {
            row.action_kind = action->second->kind;
            row.source_path = action->second->source_path;
            row.output_path = action->second->output_path;
            if (!safe_relative_path(row.source_path) || !safe_relative_path(row.output_path)) {
                mark_review_blocked(row, "import_plan.unsafe_source_or_output_path");
            }
        }

        if (const auto report = report_rows.find(row.asset); report != report_rows.end() && report->second != nullptr) {
            block_from_report_row(row, *report->second);
        }

        const auto domain = preset_domain_for_action(row.action_kind);
        if (domain == PresetDomain::unsupported) {
            mark_review_blocked(row, "preset_diff.unsupported_action_kind");
            finalize_status(row);
            if (desc.include_unchanged_rows || row.status != AssetImportPresetDiffRowStatus::unchanged) {
                rows.push_back(std::move(row));
            }
            continue;
        }

        const auto review_kind = review_kind_for_domain(domain);
        const auto before_review = review_asset_import_preset_for_asset(desc.before, row.asset_key, review_kind);
        const auto after_review = review_asset_import_preset_for_asset(desc.after, row.asset_key, review_kind);
        append_review_diagnostics(row, before_review.diagnostics);
        append_review_diagnostics(row, after_review.diagnostics);

        if (domain == PresetDomain::texture) {
            diff_texture_fields(row, effective_texture(desc.before, row.asset_key).preset,
                                effective_texture(desc.after, row.asset_key).preset);
        } else if (domain == PresetDomain::mesh) {
            diff_mesh_fields(row, effective_mesh(desc.before, row.asset_key).preset,
                             effective_mesh(desc.after, row.asset_key).preset);
        } else if (domain == PresetDomain::audio) {
            diff_audio_fields(row, effective_audio(desc.before, row.asset_key).preset,
                              effective_audio(desc.after, row.asset_key).preset);
        }

        if (desc.latest_report.has_value() && !row.field_changes.empty() &&
            report_rows.find(row.asset) == report_rows.end()) {
            mark_review_blocked(row, "latest_report.missing_asset_row");
        }

        finalize_status(row);
        if (desc.include_unchanged_rows || row.status != AssetImportPresetDiffRowStatus::unchanged) {
            rows.push_back(std::move(row));
        }
    }

    std::ranges::sort(rows, [](const AssetImportPresetDiffRow& lhs, const AssetImportPresetDiffRow& rhs) {
        if (lhs.asset_id != rhs.asset_id) {
            return lhs.asset_id < rhs.asset_id;
        }
        return lhs.asset.value < rhs.asset.value;
    });

    for (const auto& row : rows) {
        count_row(diff, row);
    }
    diff.rows = std::move(rows);
    return diff;
}

} // namespace mirakana
