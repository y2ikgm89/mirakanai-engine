// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/runtime_diagnostics.hpp"

#include <algorithm>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::runtime {
namespace {

[[nodiscard]] RuntimeDiagnostic make_error(RuntimeDiagnosticDomain domain, AssetId asset, AssetKind kind,
                                           std::string path, std::string message) {
    return RuntimeDiagnostic{
        .severity = RuntimeDiagnosticSeverity::error,
        .domain = domain,
        .asset = asset,
        .kind = kind,
        .path = std::move(path),
        .message = std::move(message),
    };
}

[[nodiscard]] RuntimeDiagnostic make_record_error(RuntimeDiagnosticDomain domain, const RuntimeAssetRecord& record,
                                                  std::string message) {
    return make_error(domain, record.asset, record.kind, record.path, std::move(message));
}

template <typename Payload>
void append_payload_diagnostic(RuntimeDiagnosticReport& report, const RuntimeAssetRecord& record,
                               const RuntimePayloadAccessResult<Payload>& access) {
    if (!access.succeeded()) {
        report.diagnostics.push_back(make_record_error(RuntimeDiagnosticDomain::payload, record, access.diagnostic));
    }
}

[[nodiscard]] const RuntimeAssetRecord* find_record_or_null(const RuntimeAssetPackage& package,
                                                            AssetId asset) noexcept {
    return asset.value == 0 ? nullptr : package.find(asset);
}

void append_scene_resolution_diagnostic(RuntimeDiagnosticReport& report, const RuntimeAssetPackage& package,
                                        const RuntimeSceneMaterialResolutionFailure& failure) {
    if (const auto* record = find_record_or_null(package, failure.asset); record != nullptr) {
        report.diagnostics.push_back(make_record_error(RuntimeDiagnosticDomain::scene, *record, failure.diagnostic));
        return;
    }
    report.diagnostics.push_back(
        make_error(RuntimeDiagnosticDomain::scene, failure.asset, AssetKind::unknown, {}, failure.diagnostic));
}

[[nodiscard]] bool path_matches_package_uri(std::string_view actual, std::string_view expected) noexcept {
    if (expected.empty()) {
        return actual.empty();
    }
    if (actual == expected) {
        return true;
    }
    if (actual.size() <= expected.size()) {
        return false;
    }
    const auto suffix_offset = actual.size() - expected.size();
    return actual[suffix_offset - 1U] == '/' && actual.substr(suffix_offset) == expected;
}

[[nodiscard]] bool has_tilemap_texture_edge(const RuntimeAssetPackage& package, const RuntimeAssetRecord& record,
                                            AssetId atlas_page) noexcept {
    return std::ranges::any_of(package.dependency_edges(), [&record, atlas_page](const AssetDependencyEdge& edge) {
        return edge.asset == record.asset && edge.dependency == atlas_page &&
               edge.kind == AssetDependencyKind::tilemap_texture && path_matches_package_uri(record.path, edge.path);
    });
}

[[nodiscard]] bool has_sprite_animation_edge(const RuntimeAssetPackage& package, const RuntimeAssetRecord& record,
                                             AssetId dependency, AssetDependencyKind kind) noexcept {
    return std::ranges::any_of(package.dependency_edges(),
                               [&record, dependency, kind](const AssetDependencyEdge& edge) {
                                   return edge.asset == record.asset && edge.dependency == dependency &&
                                          edge.kind == kind && path_matches_package_uri(record.path, edge.path);
                               });
}

void append_tilemap_package_diagnostics(RuntimeDiagnosticReport& report, const RuntimeAssetPackage& package,
                                        const RuntimeAssetRecord& record,
                                        const RuntimePayloadAccessResult<RuntimeTilemapPayload>& access) {
    append_payload_diagnostic(report, record, access);
    if (!access.succeeded()) {
        return;
    }

    const auto* atlas_page_record = package.find(access.payload.atlas_page);
    if (atlas_page_record == nullptr) {
        report.diagnostics.push_back(make_record_error(RuntimeDiagnosticDomain::payload, record,
                                                       "runtime tilemap atlas texture package entry is missing"));
        return;
    }
    if (atlas_page_record->kind != AssetKind::texture) {
        report.diagnostics.push_back(make_record_error(RuntimeDiagnosticDomain::payload, record,
                                                       "runtime tilemap atlas texture package entry is not a texture"));
    }
    if (!path_matches_package_uri(atlas_page_record->path, access.payload.atlas_page_uri)) {
        report.diagnostics.push_back(
            make_record_error(RuntimeDiagnosticDomain::payload, record,
                              "runtime tilemap atlas texture package path does not match atlas page asset_uri"));
    }
    if (!has_tilemap_texture_edge(package, record, access.payload.atlas_page)) {
        report.diagnostics.push_back(
            make_record_error(RuntimeDiagnosticDomain::payload, record,
                              "runtime tilemap package is missing a tilemap_texture dependency edge"));
    }
}

void append_sprite_animation_package_diagnostics(
    RuntimeDiagnosticReport& report, const RuntimeAssetPackage& package, const RuntimeAssetRecord& record,
    const RuntimePayloadAccessResult<RuntimeSpriteAnimationPayload>& access) {
    append_payload_diagnostic(report, record, access);
    if (!access.succeeded()) {
        return;
    }

    std::vector<AssetId> checked_sprites;
    std::vector<AssetId> checked_materials;
    for (const auto& frame : access.payload.frames) {
        if (std::ranges::find(checked_sprites, frame.sprite) == checked_sprites.end()) {
            checked_sprites.push_back(frame.sprite);
            const auto* sprite_record = package.find(frame.sprite);
            if (sprite_record == nullptr) {
                report.diagnostics.push_back(
                    make_record_error(RuntimeDiagnosticDomain::payload, record,
                                      "runtime sprite animation sprite package entry is missing"));
            } else if (sprite_record->kind != AssetKind::texture) {
                report.diagnostics.push_back(
                    make_record_error(RuntimeDiagnosticDomain::payload, record,
                                      "runtime sprite animation sprite package entry is not a texture"));
            }
            if (!has_sprite_animation_edge(package, record, frame.sprite,
                                           AssetDependencyKind::sprite_animation_texture)) {
                report.diagnostics.push_back(make_record_error(
                    RuntimeDiagnosticDomain::payload, record,
                    "runtime sprite animation package is missing a sprite_animation_texture dependency edge"));
            }
        }

        if (std::ranges::find(checked_materials, frame.material) == checked_materials.end()) {
            checked_materials.push_back(frame.material);
            const auto* material_record = package.find(frame.material);
            if (material_record == nullptr) {
                report.diagnostics.push_back(
                    make_record_error(RuntimeDiagnosticDomain::payload, record,
                                      "runtime sprite animation material package entry is missing"));
            } else if (material_record->kind != AssetKind::material) {
                report.diagnostics.push_back(
                    make_record_error(RuntimeDiagnosticDomain::payload, record,
                                      "runtime sprite animation material package entry is not a material"));
            }
            if (!has_sprite_animation_edge(package, record, frame.material,
                                           AssetDependencyKind::sprite_animation_material)) {
                report.diagnostics.push_back(make_record_error(
                    RuntimeDiagnosticDomain::payload, record,
                    "runtime sprite animation package is missing a sprite_animation_material dependency edge"));
            }
        }
    }
}

} // namespace

bool RuntimeDiagnosticReport::succeeded() const noexcept {
    return error_count() == 0;
}

std::size_t RuntimeDiagnosticReport::warning_count() const noexcept {
    return static_cast<std::size_t>(std::ranges::count_if(diagnostics, [](const RuntimeDiagnostic& diagnostic) {
        return diagnostic.severity == RuntimeDiagnosticSeverity::warning;
    }));
}

std::size_t RuntimeDiagnosticReport::error_count() const noexcept {
    return static_cast<std::size_t>(std::ranges::count_if(diagnostics, [](const RuntimeDiagnostic& diagnostic) {
        return diagnostic.severity == RuntimeDiagnosticSeverity::error;
    }));
}

std::string_view runtime_diagnostic_severity_label(RuntimeDiagnosticSeverity severity) noexcept {
    switch (severity) {
    case RuntimeDiagnosticSeverity::info:
        return "Info";
    case RuntimeDiagnosticSeverity::warning:
        return "Warning";
    case RuntimeDiagnosticSeverity::error:
        return "Error";
    case RuntimeDiagnosticSeverity::unknown:
        break;
    }
    return "Unknown";
}

std::string_view runtime_diagnostic_domain_label(RuntimeDiagnosticDomain domain) noexcept {
    switch (domain) {
    case RuntimeDiagnosticDomain::asset_package:
        return "Asset Package";
    case RuntimeDiagnosticDomain::payload:
        return "Payload";
    case RuntimeDiagnosticDomain::scene:
        return "Scene";
    case RuntimeDiagnosticDomain::session:
        return "Session";
    case RuntimeDiagnosticDomain::unknown:
        break;
    }
    return "Unknown";
}

RuntimeDiagnosticReport make_runtime_asset_package_load_diagnostic_report(const RuntimeAssetPackageLoadResult& result) {
    RuntimeDiagnosticReport report;
    report.diagnostics.reserve(result.failures.size());
    for (const auto& failure : result.failures) {
        report.diagnostics.push_back(make_error(RuntimeDiagnosticDomain::asset_package, failure.asset,
                                                AssetKind::unknown, failure.path, failure.diagnostic));
    }
    return report;
}

RuntimeDiagnosticReport inspect_runtime_asset_package(const RuntimeAssetPackage& package) {
    RuntimeDiagnosticReport report;
    std::vector<AssetId> scenes;

    for (const auto& record : package.records()) {
        switch (record.kind) {
        case AssetKind::texture:
            append_payload_diagnostic(report, record, runtime_texture_payload(record));
            break;
        case AssetKind::mesh:
            append_payload_diagnostic(report, record, runtime_mesh_payload(record));
            break;
        case AssetKind::morph_mesh_cpu:
            append_payload_diagnostic(report, record, runtime_morph_mesh_cpu_payload(record));
            break;
        case AssetKind::animation_float_clip:
            append_payload_diagnostic(report, record, runtime_animation_float_clip_payload(record));
            break;
        case AssetKind::animation_quaternion_clip:
            append_payload_diagnostic(report, record, runtime_animation_quaternion_clip_payload(record));
            break;
        case AssetKind::sprite_animation:
            append_sprite_animation_package_diagnostics(report, package, record,
                                                        runtime_sprite_animation_payload(record));
            break;
        case AssetKind::skinned_mesh:
            append_payload_diagnostic(report, record, runtime_skinned_mesh_payload(record));
            break;
        case AssetKind::audio:
            append_payload_diagnostic(report, record, runtime_audio_payload(record));
            break;
        case AssetKind::material:
            append_payload_diagnostic(report, record, runtime_material_payload(record));
            break;
        case AssetKind::ui_atlas:
            append_payload_diagnostic(report, record, runtime_ui_atlas_payload(record));
            break;
        case AssetKind::tilemap:
            append_tilemap_package_diagnostics(report, package, record, runtime_tilemap_payload(record));
            break;
        case AssetKind::physics_collision_scene:
            append_payload_diagnostic(report, record, runtime_physics_collision_scene_3d_payload(record));
            break;
        case AssetKind::scene: {
            const auto scene_payload = runtime_scene_payload(record);
            append_payload_diagnostic(report, record, scene_payload);
            if (scene_payload.succeeded()) {
                scenes.push_back(record.asset);
            }
            break;
        }
        case AssetKind::script:
        case AssetKind::shader:
        case AssetKind::unknown:
            report.diagnostics.push_back(make_record_error(RuntimeDiagnosticDomain::payload, record,
                                                           "runtime asset record kind is unsupported"));
            break;
        }
    }

    for (const auto scene : scenes) {
        const auto resolved = resolve_runtime_scene_materials(package, scene);
        for (const auto& failure : resolved.failures) {
            append_scene_resolution_diagnostic(report, package, failure);
        }
    }

    return report;
}

RuntimeDiagnosticReport make_runtime_session_diagnostic_report(const RuntimeSaveDataLoadResult& result,
                                                               std::string_view path) {
    RuntimeDiagnosticReport report;
    if (!result.succeeded()) {
        report.diagnostics.push_back(make_error(RuntimeDiagnosticDomain::session, AssetId{}, AssetKind::unknown,
                                                std::string(path), result.diagnostic));
    }
    return report;
}

RuntimeDiagnosticReport make_runtime_session_diagnostic_report(const RuntimeSettingsLoadResult& result,
                                                               std::string_view path) {
    RuntimeDiagnosticReport report;
    if (!result.succeeded()) {
        report.diagnostics.push_back(make_error(RuntimeDiagnosticDomain::session, AssetId{}, AssetKind::unknown,
                                                std::string(path), result.diagnostic));
    }
    return report;
}

RuntimeDiagnosticReport make_runtime_session_diagnostic_report(const RuntimeLocalizationCatalogLoadResult& result,
                                                               std::string_view path) {
    RuntimeDiagnosticReport report;
    if (!result.succeeded()) {
        report.diagnostics.push_back(make_error(RuntimeDiagnosticDomain::session, AssetId{}, AssetKind::unknown,
                                                std::string(path), result.diagnostic));
    }
    return report;
}

RuntimeDiagnosticReport make_runtime_session_diagnostic_report(const RuntimeInputActionMapLoadResult& result,
                                                               std::string_view path) {
    RuntimeDiagnosticReport report;
    if (!result.succeeded()) {
        report.diagnostics.push_back(make_error(RuntimeDiagnosticDomain::session, AssetId{}, AssetKind::unknown,
                                                std::string(path), result.diagnostic));
    }
    return report;
}

} // namespace mirakana::runtime
