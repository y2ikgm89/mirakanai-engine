// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/inventory_items.hpp"

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

int main() {
    return mirakana::test::run_all();
}
