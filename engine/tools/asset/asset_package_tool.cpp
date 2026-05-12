// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/asset_package_tool.hpp"

#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace mirakana {
namespace {

using ActionByAsset = std::unordered_map<AssetId, const AssetImportAction*, AssetIdHash>;

[[nodiscard]] AssetKind asset_kind_from_import_action(AssetImportActionKind kind) noexcept {
    switch (kind) {
    case AssetImportActionKind::texture:
        return AssetKind::texture;
    case AssetImportActionKind::mesh:
        return AssetKind::mesh;
    case AssetImportActionKind::morph_mesh_cpu:
        return AssetKind::morph_mesh_cpu;
    case AssetImportActionKind::animation_float_clip:
        return AssetKind::animation_float_clip;
    case AssetImportActionKind::material:
        return AssetKind::material;
    case AssetImportActionKind::scene:
        return AssetKind::scene;
    case AssetImportActionKind::audio:
        return AssetKind::audio;
    case AssetImportActionKind::unknown:
        break;
    }
    return AssetKind::unknown;
}

void add_failure(std::vector<AssetCookedPackageAssemblyFailure>& failures, AssetId asset, AssetImportActionKind kind,
                 std::string path, std::string diagnostic) {
    failures.push_back(AssetCookedPackageAssemblyFailure{
        .asset = asset,
        .kind = kind,
        .path = std::move(path),
        .diagnostic = std::move(diagnostic),
    });
}

void sort_failures(std::vector<AssetCookedPackageAssemblyFailure>& failures) {
    std::ranges::sort(failures,
                      [](const AssetCookedPackageAssemblyFailure& lhs, const AssetCookedPackageAssemblyFailure& rhs) {
                          if (lhs.path != rhs.path) {
                              return lhs.path < rhs.path;
                          }
                          if (lhs.asset.value != rhs.asset.value) {
                              return lhs.asset.value < rhs.asset.value;
                          }
                          return lhs.diagnostic < rhs.diagnostic;
                      });
}

[[nodiscard]] ActionByAsset map_plan_actions(const AssetImportPlan& plan,
                                             std::vector<AssetCookedPackageAssemblyFailure>& failures) {
    ActionByAsset actions;
    actions.reserve(plan.actions.size());
    for (const auto& action : plan.actions) {
        if (!is_valid_asset_import_action(action)) {
            add_failure(failures, action.id, action.kind, action.output_path,
                        "asset import plan action is invalid for package assembly");
            continue;
        }
        const auto [_, inserted] = actions.emplace(action.id, &action);
        if (!inserted) {
            add_failure(failures, action.id, action.kind, action.output_path,
                        "asset import plan action asset is duplicated");
        }
    }
    return actions;
}

[[nodiscard]] bool imported_contains(const std::unordered_set<AssetId, AssetIdHash>& imported, AssetId asset) {
    return imported.find(asset) != imported.end();
}

[[nodiscard]] bool has_valid_dependency_edge(const std::vector<AssetDependencyEdge>& edges, AssetId asset,
                                             AssetId dependency) {
    return std::ranges::any_of(edges, [asset, dependency](const AssetDependencyEdge& edge) {
        return edge.asset == asset && edge.dependency == dependency && is_valid_asset_dependency_edge(edge);
    });
}

void validate_all_plan_actions_imported(const AssetImportPlan& plan,
                                        const std::unordered_set<AssetId, AssetIdHash>& imported_assets,
                                        std::vector<AssetCookedPackageAssemblyFailure>& failures) {
    for (const auto& action : plan.actions) {
        if (!is_valid_asset_import_action(action)) {
            continue;
        }
        if (!imported_contains(imported_assets, action.id)) {
            add_failure(failures, action.id, action.kind, action.output_path,
                        "asset import result is missing imported artifact for plan action");
        }
    }
}

void validate_imported_dependencies(const ActionByAsset& actions,
                                    const std::unordered_set<AssetId, AssetIdHash>& imported_assets,
                                    const std::vector<AssetDependencyEdge>& dependency_edges,
                                    std::vector<AssetCookedPackageAssemblyFailure>& failures) {
    for (const auto& [asset, action] : actions) {
        if (action == nullptr || !imported_contains(imported_assets, asset)) {
            continue;
        }
        for (const auto dependency : action->dependencies) {
            if (!imported_contains(imported_assets, dependency)) {
                add_failure(failures, asset, action->kind, action->output_path,
                            "asset package dependency was not imported into the package");
            }
            if (!has_valid_dependency_edge(dependency_edges, asset, dependency)) {
                add_failure(failures, asset, action->kind, action->output_path,
                            "asset package dependency edge is missing for imported dependency");
            }
        }
    }
}

} // namespace

AssetCookedPackageAssemblyResult assemble_asset_cooked_package(IFileSystem& filesystem, const AssetImportPlan& plan,
                                                               const AssetImportExecutionResult& import_result,
                                                               AssetCookedPackageAssemblyDesc desc) {
    AssetCookedPackageAssemblyResult result;
    if (desc.source_revision == 0) {
        add_failure(result.failures, {}, AssetImportActionKind::unknown, {},
                    "asset cooked package source revision must be non-zero");
        return result;
    }

    for (const auto& failure : import_result.failures) {
        add_failure(result.failures, failure.asset, failure.kind, failure.output_path,
                    "asset import failed before package assembly: " + failure.diagnostic);
    }
    if (!result.failures.empty()) {
        sort_failures(result.failures);
        return result;
    }

    const auto actions = map_plan_actions(plan, result.failures);
    std::unordered_set<AssetId, AssetIdHash> imported_assets;
    imported_assets.reserve(import_result.imported.size());

    std::vector<AssetCookedArtifact> artifacts;
    artifacts.reserve(import_result.imported.size());
    for (const auto& imported : import_result.imported) {
        if (!imported_assets.insert(imported.asset).second) {
            add_failure(result.failures, imported.asset, imported.kind, imported.output_path,
                        "asset import result asset is duplicated");
            continue;
        }

        const auto action = actions.find(imported.asset);
        if (action == actions.end() || action->second == nullptr) {
            add_failure(result.failures, imported.asset, imported.kind, imported.output_path,
                        "asset import result has no matching plan action");
            continue;
        }

        const auto& planned = *action->second;
        if (planned.kind != imported.kind) {
            add_failure(result.failures, imported.asset, imported.kind, imported.output_path,
                        "asset import result kind does not match plan action");
            continue;
        }
        if (planned.output_path != imported.output_path) {
            add_failure(result.failures, imported.asset, imported.kind, imported.output_path,
                        "asset import result output path does not match plan action");
            continue;
        }

        const auto kind = asset_kind_from_import_action(imported.kind);
        if (kind == AssetKind::unknown) {
            add_failure(result.failures, imported.asset, imported.kind, imported.output_path,
                        "asset import result kind is unsupported for package assembly");
            continue;
        }
        try {
            if (!filesystem.exists(imported.output_path)) {
                add_failure(result.failures, imported.asset, imported.kind, imported.output_path,
                            "missing cooked artifact for package assembly");
                continue;
            }
            artifacts.push_back(AssetCookedArtifact{
                .asset = imported.asset,
                .kind = kind,
                .path = imported.output_path,
                .content = filesystem.read_text(imported.output_path),
                .source_revision = desc.source_revision,
                .dependencies = planned.dependencies,
            });
        } catch (const std::exception& error) {
            add_failure(result.failures, imported.asset, imported.kind, imported.output_path,
                        std::string("failed to read cooked artifact for package assembly: ") + error.what());
        }
    }

    validate_all_plan_actions_imported(plan, imported_assets, result.failures);
    validate_imported_dependencies(actions, imported_assets, plan.dependencies, result.failures);

    std::vector<AssetDependencyEdge> dependency_edges;
    dependency_edges.reserve(plan.dependencies.size());
    for (const auto& edge : plan.dependencies) {
        if (!imported_contains(imported_assets, edge.asset)) {
            continue;
        }
        if (!is_valid_asset_dependency_edge(edge)) {
            add_failure(result.failures, edge.asset, AssetImportActionKind::unknown, edge.path,
                        "asset package dependency edge is invalid");
            continue;
        }
        if (!imported_contains(imported_assets, edge.dependency)) {
            add_failure(result.failures, edge.asset, AssetImportActionKind::unknown, edge.path,
                        "asset package dependency was not imported into the package");
            continue;
        }
        dependency_edges.push_back(edge);
    }

    if (!result.failures.empty()) {
        sort_failures(result.failures);
        return result;
    }

    try {
        result.index = build_asset_cooked_package_index(std::move(artifacts), std::move(dependency_edges));
    } catch (const std::exception& error) {
        add_failure(result.failures, {}, AssetImportActionKind::unknown, {},
                    std::string("asset cooked package index assembly failed: ") + error.what());
    }
    sort_failures(result.failures);
    return result;
}

void write_asset_cooked_package_index(IFileSystem& filesystem, std::string_view path,
                                      const AssetCookedPackageIndex& index) {
    filesystem.write_text(path, serialize_asset_cooked_package_index(index));
}

} // namespace mirakana
