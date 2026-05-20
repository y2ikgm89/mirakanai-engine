// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana::runtime {

struct RuntimeItemCostDesc {
    std::string item_id;
    std::uint32_t quantity{0U};

    [[nodiscard]] bool operator==(const RuntimeItemCostDesc&) const = default;
};

struct RuntimeItemDesc {
    std::string id;
    std::string localization_key;
    std::string category_id;
    std::vector<std::string> tag_ids;
    std::uint32_t max_stack{1U};
    std::string placement_id;
    std::vector<RuntimeItemCostDesc> placement_costs;

    [[nodiscard]] bool operator==(const RuntimeItemDesc&) const = default;
};

struct RuntimeItemCatalogDocument {
    std::vector<RuntimeItemDesc> items;

    [[nodiscard]] bool operator==(const RuntimeItemCatalogDocument&) const = default;
};

struct RuntimeItemCatalogValidationContext {
    std::span<const std::string> localization_keys;
    std::span<const std::string> supported_category_ids;
    std::span<const std::string> supported_tag_ids;
    std::span<const std::string> supported_placement_ids;
};

enum class RuntimeItemCatalogDiagnosticCode : std::uint8_t {
    none,
    invalid_item_id,
    duplicate_item_id,
    unsafe_localization_key,
    missing_localization_key,
    invalid_stack_limit,
    unsupported_category_id,
    unsupported_tag_id,
    duplicate_item_tag_id,
    unsupported_placement_id,
    missing_item_reference,
    invalid_cost_quantity,
};

struct RuntimeItemCatalogDiagnostic {
    RuntimeItemCatalogDiagnosticCode code{RuntimeItemCatalogDiagnosticCode::none};
    std::string item_id;
    std::string referenced_item_id;
    std::string category_id;
    std::string tag_id;
    std::string placement_id;
    std::string localization_key;
    std::uint32_t quantity{0U};

    [[nodiscard]] bool operator==(const RuntimeItemCatalogDiagnostic&) const = default;
};

enum class RuntimeItemCatalogValidationRowKind : std::uint8_t {
    item,
};

struct RuntimeItemCatalogValidationRow {
    RuntimeItemCatalogValidationRowKind kind{RuntimeItemCatalogValidationRowKind::item};
    std::string item_id;

    [[nodiscard]] bool operator==(const RuntimeItemCatalogValidationRow&) const = default;
};

struct RuntimeItemCatalogValidationResult {
    bool succeeded{true};
    std::vector<RuntimeItemCatalogDiagnostic> diagnostics;
    std::vector<RuntimeItemCatalogValidationRow> rows;
};

struct RuntimeInventoryStackDesc {
    std::string item_id;
    std::uint32_t quantity{0U};

    [[nodiscard]] bool operator==(const RuntimeInventoryStackDesc&) const = default;
};

struct RuntimeInventoryState {
    std::vector<RuntimeInventoryStackDesc> stacks;

    [[nodiscard]] bool operator==(const RuntimeInventoryState&) const = default;
};

enum class RuntimeInventoryStateDiagnosticCode : std::uint8_t {
    none,
    invalid_catalog,
    missing_item_reference,
    invalid_quantity,
    stack_limit_exceeded,
};

struct RuntimeInventoryStateDiagnostic {
    RuntimeInventoryStateDiagnosticCode code{RuntimeInventoryStateDiagnosticCode::none};
    std::string item_id;
    std::uint32_t quantity{0U};

    [[nodiscard]] bool operator==(const RuntimeInventoryStateDiagnostic&) const = default;
};

enum class RuntimeInventoryStateRowKind : std::uint8_t {
    stack,
};

struct RuntimeInventoryStateRow {
    RuntimeInventoryStateRowKind kind{RuntimeInventoryStateRowKind::stack};
    std::string item_id;
    std::uint32_t quantity{0U};

    [[nodiscard]] bool operator==(const RuntimeInventoryStateRow&) const = default;
};

struct RuntimeInventoryStateValidationResult {
    bool succeeded{true};
    std::vector<RuntimeInventoryStateDiagnostic> diagnostics;
    std::vector<RuntimeInventoryStateRow> rows;
};

struct RuntimeCraftingRecipeDesc {
    std::string id;
    std::vector<RuntimeItemCostDesc> inputs;
    std::vector<RuntimeItemCostDesc> outputs;

    [[nodiscard]] bool operator==(const RuntimeCraftingRecipeDesc&) const = default;
};

struct RuntimeCraftingRecipeDocument {
    std::vector<RuntimeCraftingRecipeDesc> recipes;

    [[nodiscard]] bool operator==(const RuntimeCraftingRecipeDocument&) const = default;
};

enum class RuntimeInventoryTransitionKind : std::uint8_t {
    add_item,
    remove_item,
    craft_recipe,
};

enum class RuntimeInventoryTransitionStatus : std::uint8_t {
    accepted,
    ignored,
    blocked,
    completed,
    invalid,
};

enum class RuntimeInventoryTransitionDiagnosticCode : std::uint8_t {
    none,
    invalid_catalog,
    invalid_state,
    missing_item,
    missing_recipe,
    invalid_recipe,
    insufficient_items,
};

struct RuntimeInventoryTransitionRequest {
    RuntimeInventoryTransitionKind kind{RuntimeInventoryTransitionKind::add_item};
    std::string item_id;
    std::uint32_t quantity{0U};
    std::string recipe_id;
};

struct RuntimeInventoryTransitionRow {
    RuntimeInventoryTransitionKind kind{RuntimeInventoryTransitionKind::add_item};
    RuntimeInventoryTransitionStatus status{RuntimeInventoryTransitionStatus::accepted};
    std::string item_id;
    std::string recipe_id;
    std::uint32_t quantity{0U};

    [[nodiscard]] bool operator==(const RuntimeInventoryTransitionRow&) const = default;
};

struct RuntimeInventoryTransitionDiagnostic {
    RuntimeInventoryTransitionDiagnosticCode code{RuntimeInventoryTransitionDiagnosticCode::none};
    std::string item_id;
    std::string recipe_id;
    std::uint32_t quantity{0U};

    [[nodiscard]] bool operator==(const RuntimeInventoryTransitionDiagnostic&) const = default;
};

struct RuntimeInventoryTransitionResult {
    bool succeeded{true};
    RuntimeInventoryState state;
    std::vector<RuntimeInventoryTransitionRow> rows;
    std::vector<RuntimeInventoryTransitionDiagnostic> diagnostics;
};

struct RuntimeConstructionPlacementSurfaceDesc {
    std::string id;
    std::string placement_id;

    [[nodiscard]] bool operator==(const RuntimeConstructionPlacementSurfaceDesc&) const = default;
};

struct RuntimeConstructionPlacementCellDesc {
    std::int32_t x{0};
    std::int32_t y{0};
    std::int32_t z{0};

    [[nodiscard]] bool operator==(const RuntimeConstructionPlacementCellDesc&) const = default;
};

struct RuntimeConstructionPlacementCandidateDesc {
    std::string item_id;
    std::string surface_id;
    float grid_x{0.0F};
    float grid_y{0.0F};
    float grid_z{0.0F};
    float world_x{0.0F};
    float world_y{0.0F};
    float world_z{0.0F};
    std::uint32_t footprint_width{1U};
    std::uint32_t footprint_height{1U};
    std::uint32_t footprint_depth{1U};
    std::vector<RuntimeConstructionPlacementCellDesc> occupied_cells;
    std::vector<RuntimeItemCostDesc> provided_costs;

    [[nodiscard]] bool operator==(const RuntimeConstructionPlacementCandidateDesc&) const = default;
};

struct RuntimeConstructionPlacementValidationContext {
    std::span<const std::string> supported_placement_ids;
    std::span<const RuntimeConstructionPlacementSurfaceDesc> supported_surfaces;
};

enum class RuntimeConstructionPlacementDiagnosticCode : std::uint8_t {
    none,
    invalid_catalog,
    missing_item_reference,
    item_not_placeable,
    unsupported_placement_id,
    missing_surface,
    unsupported_placement_surface,
    invalid_grid_position,
    invalid_world_position,
    invalid_footprint,
    duplicate_occupied_cell,
    missing_cost,
};

struct RuntimeConstructionPlacementDiagnostic {
    RuntimeConstructionPlacementDiagnosticCode code{RuntimeConstructionPlacementDiagnosticCode::none};
    std::uint32_t candidate_index{0U};
    std::string item_id;
    std::string referenced_item_id;
    std::string placement_id;
    std::string surface_id;
    std::int32_t cell_x{0};
    std::int32_t cell_y{0};
    std::int32_t cell_z{0};
    std::uint32_t required_quantity{0U};

    [[nodiscard]] bool operator==(const RuntimeConstructionPlacementDiagnostic&) const = default;
};

enum class RuntimeConstructionPlacementValidationRowKind : std::uint8_t {
    candidate,
    occupied_cell,
};

struct RuntimeConstructionPlacementValidationRow {
    RuntimeConstructionPlacementValidationRowKind kind{RuntimeConstructionPlacementValidationRowKind::candidate};
    std::uint32_t candidate_index{0U};
    std::string item_id;
    std::string placement_id;
    std::string surface_id;
    std::uint32_t footprint_width{0U};
    std::uint32_t footprint_height{0U};
    std::uint32_t footprint_depth{0U};
    std::int32_t cell_x{0};
    std::int32_t cell_y{0};
    std::int32_t cell_z{0};

    [[nodiscard]] bool operator==(const RuntimeConstructionPlacementValidationRow&) const = default;
};

struct RuntimeConstructionPlacementValidationResult {
    bool succeeded{true};
    std::vector<RuntimeConstructionPlacementDiagnostic> diagnostics;
    std::vector<RuntimeConstructionPlacementValidationRow> rows;
};

[[nodiscard]] RuntimeItemCatalogValidationResult
validate_runtime_item_catalog_document(const RuntimeItemCatalogDocument& document,
                                       const RuntimeItemCatalogValidationContext& context);

[[nodiscard]] RuntimeInventoryStateValidationResult
validate_runtime_inventory_state(const RuntimeItemCatalogDocument& catalog, const RuntimeInventoryState& state);

[[nodiscard]] RuntimeInventoryTransitionResult
advance_runtime_inventory_state(const RuntimeItemCatalogDocument& catalog, const RuntimeCraftingRecipeDocument& recipes,
                                const RuntimeInventoryState& state, const RuntimeInventoryTransitionRequest& request);

[[nodiscard]] RuntimeConstructionPlacementValidationResult
validate_runtime_construction_placement(const RuntimeItemCatalogDocument& catalog,
                                        std::span<const RuntimeConstructionPlacementCandidateDesc> candidates,
                                        const RuntimeConstructionPlacementValidationContext& context);

} // namespace mirakana::runtime
