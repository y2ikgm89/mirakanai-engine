// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/asset_axis_unit_preview.hpp"

#include "mirakana/tools/asset_coordinate_normalization.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace mirakana {
namespace {

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

[[nodiscard]] bool safe_relative_path(std::string_view value) noexcept {
    return clean_text(value) && value.front() != '/' && value.find('\\') == std::string_view::npos &&
           value.find(':') == std::string_view::npos && !contains_parent_segment(value);
}

[[nodiscard]] bool supported_action_kind(AssetImportActionKind kind) noexcept {
    switch (kind) {
    case AssetImportActionKind::mesh:
    case AssetImportActionKind::morph_mesh_cpu:
    case AssetImportActionKind::animation_float_clip:
    case AssetImportActionKind::animation_quaternion_clip:
        return true;
    case AssetImportActionKind::unknown:
    case AssetImportActionKind::texture:
    case AssetImportActionKind::material:
    case AssetImportActionKind::scene:
    case AssetImportActionKind::audio:
    case AssetImportActionKind::environment_profile:
        break;
    }
    return false;
}

[[nodiscard]] bool finite_vec3(Vec3 value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

void extend_bounds(AssetAxisUnitPreviewBounds& bounds, Vec3 value) {
    if (!finite_vec3(value)) {
        return;
    }
    if (!bounds.valid) {
        bounds.min = value;
        bounds.max = value;
        bounds.valid = true;
        return;
    }
    bounds.min.x = std::min(bounds.min.x, value.x);
    bounds.min.y = std::min(bounds.min.y, value.y);
    bounds.min.z = std::min(bounds.min.z, value.z);
    bounds.max.x = std::max(bounds.max.x, value.x);
    bounds.max.y = std::max(bounds.max.y, value.y);
    bounds.max.z = std::max(bounds.max.z, value.z);
}

[[nodiscard]] AssetAxisUnitPreviewBasisAxis make_basis_axis(const AssetCoordinateNormalizationPlan& plan,
                                                            std::string label, Vec3 axis) {
    return AssetAxisUnitPreviewBasisAxis{
        .label = std::move(label),
        .source_axis = axis,
        .project_axis = normalize_asset_direction(plan, axis),
    };
}

[[nodiscard]] AssetAxisUnitPreviewRow make_row(const AssetCoordinateNormalizationPlan& plan,
                                               AssetAxisUnitPreviewSampleKind kind,
                                               const AssetAxisUnitPreviewSample& sample) {
    AssetAxisUnitPreviewRow row{
        .kind = kind,
        .label = sample.label,
        .source_position = sample.position,
        .project_position = normalize_asset_position(plan, sample.position),
        .source_direction = sample.direction,
        .project_direction = sample.has_direction ? normalize_asset_direction(plan, sample.direction) : Vec3{},
        .source_rotation = sample.rotation,
        .project_rotation = sample.has_rotation ? normalize_asset_rotation(plan, sample.rotation) : Quat::identity(),
        .has_direction = sample.has_direction,
        .has_rotation = sample.has_rotation,
    };
    return row;
}

void append_rows(AssetAxisUnitPreview& preview, const AssetCoordinateNormalizationPlan& plan,
                 AssetAxisUnitPreviewSampleKind kind, const std::vector<AssetAxisUnitPreviewSample>& samples) {
    for (const auto& sample : samples) {
        preview.rows.push_back(make_row(plan, kind, sample));
    }
}

void append_bounds_from_rows(AssetAxisUnitPreview& preview, AssetAxisUnitPreviewSampleKind preferred_kind) {
    bool saw_preferred = false;
    for (const auto& row : preview.rows) {
        if (row.kind != preferred_kind) {
            continue;
        }
        saw_preferred = true;
        extend_bounds(preview.source_bounds, row.source_position);
        extend_bounds(preview.project_bounds, row.project_position);
    }
    if (saw_preferred) {
        return;
    }
    for (const auto& row : preview.rows) {
        extend_bounds(preview.source_bounds, row.source_position);
        extend_bounds(preview.project_bounds, row.project_position);
    }
}

} // namespace

AssetAxisUnitPreview build_asset_axis_unit_preview(const AssetAxisUnitPreviewDesc& desc) {
    AssetAxisUnitPreview preview{
        .asset_id = desc.asset_id,
        .source_path = desc.source_path,
        .up_axis = desc.mesh_preset.up_axis,
        .unit_scale = desc.mesh_preset.unit_scale,
    };

    if (!clean_text(desc.asset_id)) {
        preview.diagnostics.push_back("asset.invalid_asset_id");
    }
    if (!safe_relative_path(desc.source_path)) {
        preview.diagnostics.push_back("source.invalid_path");
    }
    if (!supported_action_kind(desc.action_kind)) {
        preview.diagnostics.push_back("source.unsupported_for_axis_unit_preview");
    }
    const auto sample_count = desc.vertex_samples.size() + desc.joint_samples.size();
    if (sample_count == 0U) {
        preview.diagnostics.push_back("samples.required");
    }
    if (desc.max_sample_rows == 0U || sample_count > desc.max_sample_rows) {
        preview.diagnostics.push_back("samples.max_sample_rows_exceeded");
    }
    if (!preview.diagnostics.empty()) {
        return preview;
    }

    AssetCoordinateNormalizationPlan normalization;
    try {
        normalization = make_asset_coordinate_normalization_plan(desc.mesh_preset);
    } catch (const std::invalid_argument&) {
        preview.diagnostics.push_back("mesh_preset.invalid_for_axis_unit_preview");
        return preview;
    }

    preview.changes_coordinates = normalization.changes_coordinates;
    preview.basis = {
        make_basis_axis(normalization, "x", Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F}),
        make_basis_axis(normalization, "y", Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F}),
        make_basis_axis(normalization, "z", Vec3{.x = 0.0F, .y = 0.0F, .z = 1.0F}),
    };

    preview.rows.reserve(sample_count);
    append_rows(preview, normalization, AssetAxisUnitPreviewSampleKind::vertex, desc.vertex_samples);
    append_rows(preview, normalization, AssetAxisUnitPreviewSampleKind::joint, desc.joint_samples);
    append_bounds_from_rows(preview, AssetAxisUnitPreviewSampleKind::vertex);
    return preview;
}

} // namespace mirakana
