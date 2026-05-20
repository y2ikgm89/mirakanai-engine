// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/inventory_items.hpp"

#include <limits>
#include <span>
#include <string>
#include <vector>

namespace {

[[nodiscard]] mirakana::runtime::RuntimeItemCatalogDocument valid_item_catalog_document() {
    using namespace mirakana::runtime;

    return RuntimeItemCatalogDocument{
        .items =
            std::vector<RuntimeItemDesc>{
                RuntimeItemDesc{
                    .id = "wood",
                    .localization_key = "item.wood",
                    .category_id = "material",
                    .tag_ids = {"flammable", "crafting"},
                    .max_stack = 99U,
                    .placement_id = {},
                    .placement_costs = {},
                },
                RuntimeItemDesc{
                    .id = "workbench",
                    .localization_key = "item.workbench",
                    .category_id = "station",
                    .tag_ids = {"placeable", "crafting"},
                    .max_stack = 1U,
                    .placement_id = "grid_2d",
                    .placement_costs =
                        std::vector<RuntimeItemCostDesc>{
                            RuntimeItemCostDesc{
                                .item_id = "wood",
                                .quantity = 3U,
                            },
                        },
                },
            },
    };
}

[[nodiscard]] mirakana::runtime::RuntimeItemCatalogValidationContext validation_context() {
    static const std::vector<std::string> localization_keys{"item.wood", "item.workbench"};
    static const std::vector<std::string> category_ids{"material", "station"};
    static const std::vector<std::string> tag_ids{"flammable", "crafting", "placeable"};
    static const std::vector<std::string> placement_ids{"grid_2d"};

    return mirakana::runtime::RuntimeItemCatalogValidationContext{
        .localization_keys = std::span<const std::string>{localization_keys},
        .supported_category_ids = std::span<const std::string>{category_ids},
        .supported_tag_ids = std::span<const std::string>{tag_ids},
        .supported_placement_ids = std::span<const std::string>{placement_ids},
    };
}

} // namespace

MK_TEST("runtime item catalog validates deterministic rows") {
    const auto document = valid_item_catalog_document();

    const auto first = mirakana::runtime::validate_runtime_item_catalog_document(document, validation_context());
    const auto second = mirakana::runtime::validate_runtime_item_catalog_document(document, validation_context());

    MK_REQUIRE(first.succeeded);
    MK_REQUIRE(first.diagnostics.empty());
    MK_REQUIRE(first.rows == second.rows);
    MK_REQUIRE(first.rows.size() == 2U);
    MK_REQUIRE(first.rows[0].kind == mirakana::runtime::RuntimeItemCatalogValidationRowKind::item);
    MK_REQUIRE(first.rows[0].item_id == "wood");
    MK_REQUIRE(first.rows[1].kind == mirakana::runtime::RuntimeItemCatalogValidationRowKind::item);
    MK_REQUIRE(first.rows[1].item_id == "workbench");
}

MK_TEST("runtime item catalog reports invalid ids metadata and references deterministically") {
    using Code = mirakana::runtime::RuntimeItemCatalogDiagnosticCode;
    using namespace mirakana::runtime;

    auto document = valid_item_catalog_document();
    document.items.push_back(document.items.front());
    document.items.front().localization_key = "item wood unsafe";
    document.items.front().category_id = "unsupported_category";
    document.items.front().tag_ids.push_back("unknown_tag");
    document.items.front().tag_ids.push_back("crafting");
    document.items.front().max_stack = 0U;
    document.items[1].localization_key = "item.missing";
    document.items[1].placement_id = "unsupported_placement";
    document.items[1].placement_costs.push_back(RuntimeItemCostDesc{.item_id = "missing_item", .quantity = 2U});
    document.items[1].placement_costs.push_back(RuntimeItemCostDesc{.item_id = "wood", .quantity = 0U});

    const auto first = validate_runtime_item_catalog_document(document, validation_context());
    const auto second = validate_runtime_item_catalog_document(document, validation_context());

    MK_REQUIRE(!first.succeeded);
    MK_REQUIRE(first.diagnostics == second.diagnostics);
    MK_REQUIRE(first.rows.empty());
    MK_REQUIRE(first.diagnostics.size() == 10U);
    MK_REQUIRE(first.diagnostics[0].code == Code::duplicate_item_id);
    MK_REQUIRE(first.diagnostics[0].item_id == "wood");
    MK_REQUIRE(first.diagnostics[1].code == Code::unsafe_localization_key);
    MK_REQUIRE(first.diagnostics[1].localization_key == "item wood unsafe");
    MK_REQUIRE(first.diagnostics[2].code == Code::invalid_stack_limit);
    MK_REQUIRE(first.diagnostics[2].item_id == "wood");
    MK_REQUIRE(first.diagnostics[3].code == Code::unsupported_category_id);
    MK_REQUIRE(first.diagnostics[3].category_id == "unsupported_category");
    MK_REQUIRE(first.diagnostics[4].code == Code::unsupported_tag_id);
    MK_REQUIRE(first.diagnostics[4].tag_id == "unknown_tag");
    MK_REQUIRE(first.diagnostics[5].code == Code::duplicate_item_tag_id);
    MK_REQUIRE(first.diagnostics[5].tag_id == "crafting");
    MK_REQUIRE(first.diagnostics[6].code == Code::missing_localization_key);
    MK_REQUIRE(first.diagnostics[6].localization_key == "item.missing");
    MK_REQUIRE(first.diagnostics[7].code == Code::unsupported_placement_id);
    MK_REQUIRE(first.diagnostics[7].placement_id == "unsupported_placement");
    MK_REQUIRE(first.diagnostics[8].code == Code::missing_item_reference);
    MK_REQUIRE(first.diagnostics[8].referenced_item_id == "missing_item");
    MK_REQUIRE(first.diagnostics[9].code == Code::invalid_cost_quantity);
    MK_REQUIRE(first.diagnostics[9].referenced_item_id == "wood");
}

MK_TEST("runtime inventory state validates deterministic stack rows") {
    using Code = mirakana::runtime::RuntimeInventoryStateDiagnosticCode;
    using namespace mirakana::runtime;

    const auto document = valid_item_catalog_document();
    const RuntimeInventoryState valid_state{
        .stacks =
            std::vector<RuntimeInventoryStackDesc>{
                RuntimeInventoryStackDesc{.item_id = "wood", .quantity = 99U},
                RuntimeInventoryStackDesc{.item_id = "workbench", .quantity = 1U},
            },
    };

    const auto valid_first = validate_runtime_inventory_state(document, valid_state);
    const auto valid_second = validate_runtime_inventory_state(document, valid_state);

    MK_REQUIRE(valid_first.succeeded);
    MK_REQUIRE(valid_first.diagnostics.empty());
    MK_REQUIRE(valid_first.rows == valid_second.rows);
    MK_REQUIRE(valid_first.rows.size() == 2U);
    MK_REQUIRE(valid_first.rows[0].item_id == "wood");
    MK_REQUIRE(valid_first.rows[0].quantity == 99U);
    MK_REQUIRE(valid_first.rows[1].item_id == "workbench");
    MK_REQUIRE(valid_first.rows[1].quantity == 1U);

    const RuntimeInventoryState invalid_state{
        .stacks =
            std::vector<RuntimeInventoryStackDesc>{
                RuntimeInventoryStackDesc{.item_id = "wood", .quantity = 100U},
                RuntimeInventoryStackDesc{.item_id = "missing", .quantity = 1U},
                RuntimeInventoryStackDesc{.item_id = "workbench", .quantity = 0U},
            },
    };

    const auto invalid_first = validate_runtime_inventory_state(document, invalid_state);
    const auto invalid_second = validate_runtime_inventory_state(document, invalid_state);

    MK_REQUIRE(!invalid_first.succeeded);
    MK_REQUIRE(invalid_first.rows.empty());
    MK_REQUIRE(invalid_first.diagnostics == invalid_second.diagnostics);
    MK_REQUIRE(invalid_first.diagnostics.size() == 3U);
    MK_REQUIRE(invalid_first.diagnostics[0].code == Code::stack_limit_exceeded);
    MK_REQUIRE(invalid_first.diagnostics[0].item_id == "wood");
    MK_REQUIRE(invalid_first.diagnostics[0].quantity == 100U);
    MK_REQUIRE(invalid_first.diagnostics[1].code == Code::missing_item_reference);
    MK_REQUIRE(invalid_first.diagnostics[1].item_id == "missing");
    MK_REQUIRE(invalid_first.diagnostics[2].code == Code::invalid_quantity);
    MK_REQUIRE(invalid_first.diagnostics[2].item_id == "workbench");
}

MK_TEST("runtime inventory transitions classify accepted ignored blocked completed and invalid rows") {
    using Status = mirakana::runtime::RuntimeInventoryTransitionStatus;
    using namespace mirakana::runtime;

    const auto document = valid_item_catalog_document();
    const RuntimeCraftingRecipeDocument recipes{
        .recipes =
            std::vector<RuntimeCraftingRecipeDesc>{
                RuntimeCraftingRecipeDesc{
                    .id = "recipe.workbench",
                    .inputs =
                        std::vector<RuntimeItemCostDesc>{
                            RuntimeItemCostDesc{.item_id = "wood", .quantity = 3U},
                        },
                    .outputs =
                        std::vector<RuntimeItemCostDesc>{
                            RuntimeItemCostDesc{.item_id = "workbench", .quantity = 1U},
                        },
                },
            },
    };
    RuntimeInventoryState state{
        .stacks =
            std::vector<RuntimeInventoryStackDesc>{
                RuntimeInventoryStackDesc{.item_id = "wood", .quantity = 2U},
            },
    };

    const auto blocked_craft = advance_runtime_inventory_state(document, recipes, state,
                                                               RuntimeInventoryTransitionRequest{
                                                                   .kind = RuntimeInventoryTransitionKind::craft_recipe,
                                                                   .recipe_id = "recipe.workbench",
                                                               });
    MK_REQUIRE(!blocked_craft.succeeded);
    MK_REQUIRE(blocked_craft.rows.size() == 1U);
    MK_REQUIRE(blocked_craft.rows[0].status == Status::blocked);
    MK_REQUIRE(blocked_craft.state == state);

    const auto ignored_add = advance_runtime_inventory_state(document, recipes, state,
                                                             RuntimeInventoryTransitionRequest{
                                                                 .kind = RuntimeInventoryTransitionKind::add_item,
                                                                 .item_id = "wood",
                                                                 .quantity = 0U,
                                                             });
    MK_REQUIRE(ignored_add.succeeded);
    MK_REQUIRE(ignored_add.rows[0].status == Status::ignored);
    MK_REQUIRE(ignored_add.state == state);

    const auto accepted_add = advance_runtime_inventory_state(document, recipes, state,
                                                              RuntimeInventoryTransitionRequest{
                                                                  .kind = RuntimeInventoryTransitionKind::add_item,
                                                                  .item_id = "wood",
                                                                  .quantity = 120U,
                                                              });
    MK_REQUIRE(accepted_add.succeeded);
    MK_REQUIRE(accepted_add.rows[0].status == Status::accepted);
    MK_REQUIRE(accepted_add.state.stacks.size() == 2U);
    MK_REQUIRE(accepted_add.state.stacks[0].item_id == "wood");
    MK_REQUIRE(accepted_add.state.stacks[0].quantity == 99U);
    MK_REQUIRE(accepted_add.state.stacks[1].item_id == "wood");
    MK_REQUIRE(accepted_add.state.stacks[1].quantity == 23U);

    state = RuntimeInventoryState{
        .stacks =
            std::vector<RuntimeInventoryStackDesc>{
                RuntimeInventoryStackDesc{.item_id = "wood", .quantity = 3U},
            },
    };
    const auto completed_craft =
        advance_runtime_inventory_state(document, recipes, state,
                                        RuntimeInventoryTransitionRequest{
                                            .kind = RuntimeInventoryTransitionKind::craft_recipe,
                                            .recipe_id = "recipe.workbench",
                                        });
    MK_REQUIRE(completed_craft.succeeded);
    MK_REQUIRE(completed_craft.rows[0].status == Status::completed);
    MK_REQUIRE(completed_craft.state.stacks.size() == 1U);
    MK_REQUIRE(completed_craft.state.stacks[0].item_id == "workbench");
    MK_REQUIRE(completed_craft.state.stacks[0].quantity == 1U);

    const auto invalid_add = advance_runtime_inventory_state(document, recipes, state,
                                                             RuntimeInventoryTransitionRequest{
                                                                 .kind = RuntimeInventoryTransitionKind::add_item,
                                                                 .item_id = "missing",
                                                                 .quantity = 1U,
                                                             });
    MK_REQUIRE(!invalid_add.succeeded);
    MK_REQUIRE(invalid_add.rows[0].status == Status::invalid);
    MK_REQUIRE(invalid_add.state == state);
}

MK_TEST("runtime construction placement validates deterministic candidate rows") {
    using namespace mirakana::runtime;

    const auto catalog = valid_item_catalog_document();
    static const std::vector<std::string> placement_ids{"grid_2d"};
    static const std::vector<RuntimeConstructionPlacementSurfaceDesc> surfaces{
        RuntimeConstructionPlacementSurfaceDesc{.id = "floor", .placement_id = "grid_2d"},
    };
    const RuntimeConstructionPlacementValidationContext context{
        .supported_placement_ids = std::span<const std::string>{placement_ids},
        .supported_surfaces = std::span<const RuntimeConstructionPlacementSurfaceDesc>{surfaces},
    };
    const std::vector<RuntimeConstructionPlacementCandidateDesc> candidates{
        RuntimeConstructionPlacementCandidateDesc{
            .item_id = "workbench",
            .surface_id = "floor",
            .grid_x = 4.0F,
            .grid_y = 7.0F,
            .grid_z = 0.0F,
            .world_x = 4.5F,
            .world_y = 7.5F,
            .world_z = 0.0F,
            .footprint_width = 2U,
            .footprint_height = 1U,
            .footprint_depth = 1U,
            .occupied_cells =
                std::vector<RuntimeConstructionPlacementCellDesc>{
                    RuntimeConstructionPlacementCellDesc{.x = 4, .y = 7, .z = 0},
                    RuntimeConstructionPlacementCellDesc{.x = 5, .y = 7, .z = 0},
                },
            .provided_costs =
                std::vector<RuntimeItemCostDesc>{
                    RuntimeItemCostDesc{.item_id = "wood", .quantity = 3U},
                },
        },
    };

    const auto first = validate_runtime_construction_placement(catalog, candidates, context);
    const auto second = validate_runtime_construction_placement(catalog, candidates, context);

    MK_REQUIRE(first.succeeded);
    MK_REQUIRE(first.diagnostics.empty());
    MK_REQUIRE(first.rows == second.rows);
    MK_REQUIRE(first.rows.size() == 3U);
    MK_REQUIRE(first.rows[0].kind == RuntimeConstructionPlacementValidationRowKind::candidate);
    MK_REQUIRE(first.rows[0].candidate_index == 0U);
    MK_REQUIRE(first.rows[0].item_id == "workbench");
    MK_REQUIRE(first.rows[0].surface_id == "floor");
    MK_REQUIRE(first.rows[0].placement_id == "grid_2d");
    MK_REQUIRE(first.rows[0].grid_x == 4.0F);
    MK_REQUIRE(first.rows[0].grid_y == 7.0F);
    MK_REQUIRE(first.rows[0].grid_z == 0.0F);
    MK_REQUIRE(first.rows[0].world_x == 4.5F);
    MK_REQUIRE(first.rows[0].world_y == 7.5F);
    MK_REQUIRE(first.rows[0].world_z == 0.0F);
    MK_REQUIRE(first.rows[0].footprint_width == 2U);
    MK_REQUIRE(first.rows[1].kind == RuntimeConstructionPlacementValidationRowKind::occupied_cell);
    MK_REQUIRE(first.rows[1].world_x == 4.5F);
    MK_REQUIRE(first.rows[1].world_y == 7.5F);
    MK_REQUIRE(first.rows[1].cell_x == 4);
    MK_REQUIRE(first.rows[1].cell_y == 7);
    MK_REQUIRE(first.rows[2].kind == RuntimeConstructionPlacementValidationRowKind::occupied_cell);
    MK_REQUIRE(first.rows[2].cell_x == 5);
    MK_REQUIRE(first.rows[2].cell_y == 7);
}

MK_TEST("runtime construction placement reports invalid candidates deterministically") {
    using Code = mirakana::runtime::RuntimeConstructionPlacementDiagnosticCode;
    using namespace mirakana::runtime;

    auto catalog = valid_item_catalog_document();
    catalog.items.push_back(RuntimeItemDesc{
        .id = "turret",
        .localization_key = "item.workbench",
        .category_id = "station",
        .tag_ids = {"placeable"},
        .max_stack = 1U,
        .placement_id = "orbital",
        .placement_costs = {},
    });

    static const std::vector<std::string> placement_ids{"grid_2d", "wall_mount"};
    static const std::vector<RuntimeConstructionPlacementSurfaceDesc> surfaces{
        RuntimeConstructionPlacementSurfaceDesc{.id = "floor", .placement_id = "grid_2d"},
        RuntimeConstructionPlacementSurfaceDesc{.id = "ceiling", .placement_id = "wall_mount"},
    };
    const RuntimeConstructionPlacementValidationContext context{
        .supported_placement_ids = std::span<const std::string>{placement_ids},
        .supported_surfaces = std::span<const RuntimeConstructionPlacementSurfaceDesc>{surfaces},
    };
    const std::vector<RuntimeConstructionPlacementCandidateDesc> candidates{
        RuntimeConstructionPlacementCandidateDesc{.item_id = "missing", .surface_id = "floor"},
        RuntimeConstructionPlacementCandidateDesc{.item_id = "wood", .surface_id = "floor"},
        RuntimeConstructionPlacementCandidateDesc{.item_id = "turret", .surface_id = "floor"},
        RuntimeConstructionPlacementCandidateDesc{.item_id = "workbench", .surface_id = "missing_surface"},
        RuntimeConstructionPlacementCandidateDesc{.item_id = "workbench", .surface_id = "ceiling"},
        RuntimeConstructionPlacementCandidateDesc{
            .item_id = "workbench",
            .surface_id = "floor",
            .grid_x = 1.0F,
            .grid_y = std::numeric_limits<float>::quiet_NaN(),
            .grid_z = 0.0F,
            .world_x = std::numeric_limits<float>::infinity(),
            .world_y = 1.0F,
            .world_z = 0.0F,
            .footprint_width = 0U,
            .footprint_height = 1U,
            .footprint_depth = 1U,
            .occupied_cells =
                std::vector<RuntimeConstructionPlacementCellDesc>{
                    RuntimeConstructionPlacementCellDesc{.x = 1, .y = 1, .z = 0},
                    RuntimeConstructionPlacementCellDesc{.x = 1, .y = 1, .z = 0},
                },
        },
    };

    const auto first = validate_runtime_construction_placement(catalog, candidates, context);
    const auto second = validate_runtime_construction_placement(catalog, candidates, context);

    MK_REQUIRE(!first.succeeded);
    MK_REQUIRE(first.rows.empty());
    MK_REQUIRE(first.diagnostics == second.diagnostics);
    MK_REQUIRE(first.diagnostics.size() == 10U);
    MK_REQUIRE(first.diagnostics[0].code == Code::missing_item_reference);
    MK_REQUIRE(first.diagnostics[0].candidate_index == 0U);
    MK_REQUIRE(first.diagnostics[0].item_id == "missing");
    MK_REQUIRE(first.diagnostics[1].code == Code::item_not_placeable);
    MK_REQUIRE(first.diagnostics[1].candidate_index == 1U);
    MK_REQUIRE(first.diagnostics[1].item_id == "wood");
    MK_REQUIRE(first.diagnostics[2].code == Code::unsupported_placement_id);
    MK_REQUIRE(first.diagnostics[2].candidate_index == 2U);
    MK_REQUIRE(first.diagnostics[2].placement_id == "orbital");
    MK_REQUIRE(first.diagnostics[3].code == Code::missing_surface);
    MK_REQUIRE(first.diagnostics[3].candidate_index == 3U);
    MK_REQUIRE(first.diagnostics[3].surface_id == "missing_surface");
    MK_REQUIRE(first.diagnostics[4].code == Code::unsupported_placement_surface);
    MK_REQUIRE(first.diagnostics[4].candidate_index == 4U);
    MK_REQUIRE(first.diagnostics[4].surface_id == "ceiling");
    MK_REQUIRE(first.diagnostics[5].code == Code::invalid_grid_position);
    MK_REQUIRE(first.diagnostics[5].candidate_index == 5U);
    MK_REQUIRE(first.diagnostics[6].code == Code::invalid_world_position);
    MK_REQUIRE(first.diagnostics[6].candidate_index == 5U);
    MK_REQUIRE(first.diagnostics[7].code == Code::invalid_footprint);
    MK_REQUIRE(first.diagnostics[7].candidate_index == 5U);
    MK_REQUIRE(first.diagnostics[8].code == Code::duplicate_occupied_cell);
    MK_REQUIRE(first.diagnostics[8].candidate_index == 5U);
    MK_REQUIRE(first.diagnostics[8].cell_x == 1);
    MK_REQUIRE(first.diagnostics[8].cell_y == 1);
    MK_REQUIRE(first.diagnostics[9].code == Code::missing_cost);
    MK_REQUIRE(first.diagnostics[9].candidate_index == 5U);
    MK_REQUIRE(first.diagnostics[9].referenced_item_id == "wood");
    MK_REQUIRE(first.diagnostics[9].required_quantity == 3U);
}

int main() {
    return mirakana::test::run_all();
}
