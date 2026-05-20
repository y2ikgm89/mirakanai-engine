// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/inventory_items.hpp"

#include <algorithm>
#include <iterator>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::runtime {
namespace {

struct RuntimeInventoryItemCount {
    std::string item_id;
    std::uint64_t quantity{0U};
};

[[nodiscard]] bool contains_control_character(const std::string_view value) noexcept {
    return std::ranges::any_of(value, [](const char character) {
        const auto byte = static_cast<unsigned char>(character);
        return byte < 0x20U || byte == 0x7FU;
    });
}

[[nodiscard]] bool is_safe_token_character(const char character) noexcept {
    return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
           (character >= '0' && character <= '9') || character == '_' || character == '-' || character == '.' ||
           character == ':' || character == '/';
}

[[nodiscard]] bool is_safe_token(const std::string_view value) noexcept {
    return !value.empty() && !contains_control_character(value) && std::ranges::all_of(value, is_safe_token_character);
}

[[nodiscard]] bool contains_string(const std::span<const std::string> values, const std::string_view value) noexcept {
    return std::ranges::find_if(values, [value](const std::string& candidate) {
               return std::string_view{candidate} == value;
           }) != values.end();
}

[[nodiscard]] bool has_item(const RuntimeItemCatalogDocument& document, const std::string_view item_id) noexcept {
    return std::ranges::find_if(document.items, [item_id](const RuntimeItemDesc& item) {
               return std::string_view{item.id} == item_id;
           }) != document.items.end();
}

[[nodiscard]] const RuntimeItemDesc* find_item(const RuntimeItemCatalogDocument& document,
                                               const std::string_view item_id) noexcept {
    const auto item = std::ranges::find_if(document.items, [item_id](const RuntimeItemDesc& candidate) {
        return std::string_view{candidate.id} == item_id;
    });
    if (item == document.items.end()) {
        return nullptr;
    }
    return &(*item);
}

[[nodiscard]] const RuntimeCraftingRecipeDesc* find_recipe(const RuntimeCraftingRecipeDocument& document,
                                                           const std::string_view recipe_id) noexcept {
    const auto recipe = std::ranges::find_if(document.recipes, [recipe_id](const RuntimeCraftingRecipeDesc& candidate) {
        return std::string_view{candidate.id} == recipe_id;
    });
    if (recipe == document.recipes.end()) {
        return nullptr;
    }
    return &(*recipe);
}

[[nodiscard]] RuntimeInventoryItemCount* find_count(std::vector<RuntimeInventoryItemCount>& counts,
                                                    const std::string_view item_id) noexcept {
    const auto count = std::ranges::find_if(counts, [item_id](const RuntimeInventoryItemCount& candidate) {
        return std::string_view{candidate.item_id} == item_id;
    });
    if (count == counts.end()) {
        return nullptr;
    }
    return &(*count);
}

[[nodiscard]] const RuntimeInventoryItemCount* find_count(const std::vector<RuntimeInventoryItemCount>& counts,
                                                          const std::string_view item_id) noexcept {
    const auto count = std::ranges::find_if(counts, [item_id](const RuntimeInventoryItemCount& candidate) {
        return std::string_view{candidate.item_id} == item_id;
    });
    if (count == counts.end()) {
        return nullptr;
    }
    return &(*count);
}

[[nodiscard]] bool is_catalog_runtime_usable(const RuntimeItemCatalogDocument& document) {
    std::vector<std::string> item_ids;
    for (const auto& item : document.items) {
        if (!is_safe_token(item.id) || item.max_stack == 0U || std::ranges::find(item_ids, item.id) != item_ids.end()) {
            return false;
        }
        item_ids.push_back(item.id);
    }
    return true;
}

[[nodiscard]] bool is_recipe_valid(const RuntimeItemCatalogDocument& catalog, const RuntimeCraftingRecipeDesc& recipe) {
    if (!is_safe_token(recipe.id) || recipe.outputs.empty()) {
        return false;
    }
    for (const auto& input : recipe.inputs) {
        if (input.quantity == 0U || find_item(catalog, input.item_id) == nullptr) {
            return false;
        }
    }
    for (const auto& output : recipe.outputs) {
        if (output.quantity == 0U || find_item(catalog, output.item_id) == nullptr) {
            return false;
        }
    }
    return true;
}

void add_diagnostic(std::vector<RuntimeItemCatalogDiagnostic>& diagnostics, RuntimeItemCatalogDiagnostic diagnostic) {
    diagnostics.push_back(std::move(diagnostic));
}

void add_state_diagnostic(std::vector<RuntimeInventoryStateDiagnostic>& diagnostics,
                          RuntimeInventoryStateDiagnostic diagnostic) {
    diagnostics.push_back(std::move(diagnostic));
}

void add_transition_diagnostic(std::vector<RuntimeInventoryTransitionDiagnostic>& diagnostics,
                               RuntimeInventoryTransitionDiagnostic diagnostic) {
    diagnostics.push_back(std::move(diagnostic));
}

void validate_duplicate_item_ids(const RuntimeItemCatalogDocument& document,
                                 std::vector<RuntimeItemCatalogDiagnostic>& diagnostics) {
    for (auto first = document.items.begin(); first != document.items.end(); ++first) {
        if (first->id.empty()) {
            continue;
        }
        for (auto second = std::next(first); second != document.items.end(); ++second) {
            if (second->id != first->id) {
                continue;
            }

            add_diagnostic(diagnostics, RuntimeItemCatalogDiagnostic{
                                            .code = RuntimeItemCatalogDiagnosticCode::duplicate_item_id,
                                            .item_id = first->id,
                                        });
            return;
        }
    }
}

void validate_localization_key(const RuntimeItemDesc& item, const RuntimeItemCatalogValidationContext& context,
                               std::vector<RuntimeItemCatalogDiagnostic>& diagnostics) {
    if (!is_safe_token(item.localization_key)) {
        add_diagnostic(diagnostics, RuntimeItemCatalogDiagnostic{
                                        .code = RuntimeItemCatalogDiagnosticCode::unsafe_localization_key,
                                        .item_id = item.id,
                                        .localization_key = item.localization_key,
                                    });
        return;
    }
    if (!contains_string(context.localization_keys, item.localization_key)) {
        add_diagnostic(diagnostics, RuntimeItemCatalogDiagnostic{
                                        .code = RuntimeItemCatalogDiagnosticCode::missing_localization_key,
                                        .item_id = item.id,
                                        .localization_key = item.localization_key,
                                    });
    }
}

void validate_tag_ids(const RuntimeItemDesc& item, const RuntimeItemCatalogValidationContext& context,
                      std::vector<RuntimeItemCatalogDiagnostic>& diagnostics) {
    std::vector<std::string> seen_tags;
    for (const auto& tag_id : item.tag_ids) {
        if (!contains_string(context.supported_tag_ids, tag_id)) {
            add_diagnostic(diagnostics, RuntimeItemCatalogDiagnostic{
                                            .code = RuntimeItemCatalogDiagnosticCode::unsupported_tag_id,
                                            .item_id = item.id,
                                            .tag_id = tag_id,
                                        });
        }
        if (std::ranges::find(seen_tags, tag_id) != seen_tags.end()) {
            add_diagnostic(diagnostics, RuntimeItemCatalogDiagnostic{
                                            .code = RuntimeItemCatalogDiagnosticCode::duplicate_item_tag_id,
                                            .item_id = item.id,
                                            .tag_id = tag_id,
                                        });
        }
        seen_tags.push_back(tag_id);
    }
}

void validate_placement_costs(const RuntimeItemCatalogDocument& document, const RuntimeItemDesc& item,
                              std::vector<RuntimeItemCatalogDiagnostic>& diagnostics) {
    for (const auto& cost : item.placement_costs) {
        if (!has_item(document, cost.item_id)) {
            add_diagnostic(diagnostics, RuntimeItemCatalogDiagnostic{
                                            .code = RuntimeItemCatalogDiagnosticCode::missing_item_reference,
                                            .item_id = item.id,
                                            .referenced_item_id = cost.item_id,
                                        });
            continue;
        }
        if (cost.quantity == 0U) {
            add_diagnostic(diagnostics, RuntimeItemCatalogDiagnostic{
                                            .code = RuntimeItemCatalogDiagnosticCode::invalid_cost_quantity,
                                            .item_id = item.id,
                                            .referenced_item_id = cost.item_id,
                                            .quantity = cost.quantity,
                                        });
        }
    }
}

void validate_item(const RuntimeItemCatalogDocument& document, const RuntimeItemDesc& item,
                   const RuntimeItemCatalogValidationContext& context,
                   std::vector<RuntimeItemCatalogDiagnostic>& diagnostics) {
    if (!is_safe_token(item.id)) {
        add_diagnostic(diagnostics, RuntimeItemCatalogDiagnostic{
                                        .code = RuntimeItemCatalogDiagnosticCode::invalid_item_id,
                                        .item_id = item.id,
                                    });
    }
    validate_localization_key(item, context, diagnostics);
    if (item.max_stack == 0U) {
        add_diagnostic(diagnostics, RuntimeItemCatalogDiagnostic{
                                        .code = RuntimeItemCatalogDiagnosticCode::invalid_stack_limit,
                                        .item_id = item.id,
                                    });
    }
    if (!contains_string(context.supported_category_ids, item.category_id)) {
        add_diagnostic(diagnostics, RuntimeItemCatalogDiagnostic{
                                        .code = RuntimeItemCatalogDiagnosticCode::unsupported_category_id,
                                        .item_id = item.id,
                                        .category_id = item.category_id,
                                    });
    }
    validate_tag_ids(item, context, diagnostics);
    if (!item.placement_id.empty() && !contains_string(context.supported_placement_ids, item.placement_id)) {
        add_diagnostic(diagnostics, RuntimeItemCatalogDiagnostic{
                                        .code = RuntimeItemCatalogDiagnosticCode::unsupported_placement_id,
                                        .item_id = item.id,
                                        .placement_id = item.placement_id,
                                    });
    }
    validate_placement_costs(document, item, diagnostics);
}

[[nodiscard]] std::vector<RuntimeInventoryItemCount> aggregate_inventory_state(const RuntimeInventoryState& state) {
    std::vector<RuntimeInventoryItemCount> counts;
    for (const auto& stack : state.stacks) {
        auto* existing = find_count(counts, stack.item_id);
        if (existing == nullptr) {
            counts.push_back(RuntimeInventoryItemCount{.item_id = stack.item_id, .quantity = stack.quantity});
            continue;
        }
        existing->quantity += stack.quantity;
    }
    return counts;
}

[[nodiscard]] RuntimeInventoryState
make_inventory_state_from_counts(const RuntimeItemCatalogDocument& catalog,
                                 const std::vector<RuntimeInventoryItemCount>& counts) {
    RuntimeInventoryState state;
    for (const auto& item : catalog.items) {
        const auto* count = find_count(counts, item.id);
        if (count == nullptr || count->quantity == 0U) {
            continue;
        }

        auto remaining = count->quantity;
        while (remaining > 0U) {
            const auto stack_quantity = static_cast<std::uint32_t>(std::min<std::uint64_t>(remaining, item.max_stack));
            state.stacks.push_back(RuntimeInventoryStackDesc{
                .item_id = item.id,
                .quantity = stack_quantity,
            });
            remaining -= stack_quantity;
        }
    }
    return state;
}

[[nodiscard]] std::uint64_t available_quantity(const std::vector<RuntimeInventoryItemCount>& counts,
                                               const std::string_view item_id) noexcept {
    const auto* count = find_count(counts, item_id);
    if (count == nullptr) {
        return 0U;
    }
    return count->quantity;
}

void add_quantity(std::vector<RuntimeInventoryItemCount>& counts, const std::string& item_id,
                  const std::uint32_t quantity) {
    auto* existing = find_count(counts, item_id);
    if (existing == nullptr) {
        counts.push_back(RuntimeInventoryItemCount{.item_id = item_id, .quantity = quantity});
        return;
    }
    existing->quantity += quantity;
}

void remove_quantity(std::vector<RuntimeInventoryItemCount>& counts, const std::string_view item_id,
                     const std::uint32_t quantity) {
    auto* existing = find_count(counts, item_id);
    if (existing == nullptr) {
        return;
    }
    existing->quantity -= std::min<std::uint64_t>(existing->quantity, quantity);
}

void add_transition_row(RuntimeInventoryTransitionResult& result, RuntimeInventoryTransitionRow row) {
    result.rows.push_back(std::move(row));
}

void mark_transition_invalid(RuntimeInventoryTransitionResult& result, RuntimeInventoryTransitionRow row,
                             RuntimeInventoryTransitionDiagnostic diagnostic) {
    row.status = RuntimeInventoryTransitionStatus::invalid;
    add_transition_row(result, std::move(row));
    add_transition_diagnostic(result.diagnostics, std::move(diagnostic));
    result.succeeded = false;
}

void mark_transition_blocked(RuntimeInventoryTransitionResult& result, RuntimeInventoryTransitionRow row,
                             RuntimeInventoryTransitionDiagnostic diagnostic) {
    row.status = RuntimeInventoryTransitionStatus::blocked;
    add_transition_row(result, std::move(row));
    add_transition_diagnostic(result.diagnostics, std::move(diagnostic));
    result.succeeded = false;
}

} // namespace

RuntimeItemCatalogValidationResult
validate_runtime_item_catalog_document(const RuntimeItemCatalogDocument& document,
                                       const RuntimeItemCatalogValidationContext& context) {
    RuntimeItemCatalogValidationResult result;

    validate_duplicate_item_ids(document, result.diagnostics);
    for (const auto& item : document.items) {
        validate_item(document, item, context, result.diagnostics);
    }

    if (!result.diagnostics.empty()) {
        result.succeeded = false;
        result.rows.clear();
        return result;
    }

    result.rows.reserve(document.items.size());
    for (const auto& item : document.items) {
        result.rows.push_back(RuntimeItemCatalogValidationRow{
            .kind = RuntimeItemCatalogValidationRowKind::item,
            .item_id = item.id,
        });
    }

    return result;
}

RuntimeInventoryStateValidationResult validate_runtime_inventory_state(const RuntimeItemCatalogDocument& catalog,
                                                                       const RuntimeInventoryState& state) {
    RuntimeInventoryStateValidationResult result;
    if (!is_catalog_runtime_usable(catalog)) {
        result.succeeded = false;
        add_state_diagnostic(result.diagnostics, RuntimeInventoryStateDiagnostic{
                                                     .code = RuntimeInventoryStateDiagnosticCode::invalid_catalog,
                                                 });
        return result;
    }

    for (const auto& stack : state.stacks) {
        const auto* item = find_item(catalog, stack.item_id);
        if (item == nullptr) {
            add_state_diagnostic(result.diagnostics,
                                 RuntimeInventoryStateDiagnostic{
                                     .code = RuntimeInventoryStateDiagnosticCode::missing_item_reference,
                                     .item_id = stack.item_id,
                                     .quantity = stack.quantity,
                                 });
            continue;
        }
        if (stack.quantity == 0U) {
            add_state_diagnostic(result.diagnostics, RuntimeInventoryStateDiagnostic{
                                                         .code = RuntimeInventoryStateDiagnosticCode::invalid_quantity,
                                                         .item_id = stack.item_id,
                                                         .quantity = stack.quantity,
                                                     });
            continue;
        }
        if (stack.quantity > item->max_stack) {
            add_state_diagnostic(result.diagnostics,
                                 RuntimeInventoryStateDiagnostic{
                                     .code = RuntimeInventoryStateDiagnosticCode::stack_limit_exceeded,
                                     .item_id = stack.item_id,
                                     .quantity = stack.quantity,
                                 });
        }
    }

    if (!result.diagnostics.empty()) {
        result.succeeded = false;
        result.rows.clear();
        return result;
    }

    result.rows.reserve(state.stacks.size());
    for (const auto& stack : state.stacks) {
        result.rows.push_back(RuntimeInventoryStateRow{
            .kind = RuntimeInventoryStateRowKind::stack,
            .item_id = stack.item_id,
            .quantity = stack.quantity,
        });
    }
    return result;
}

RuntimeInventoryTransitionResult advance_runtime_inventory_state(const RuntimeItemCatalogDocument& catalog,
                                                                 const RuntimeCraftingRecipeDocument& recipes,
                                                                 const RuntimeInventoryState& state,
                                                                 const RuntimeInventoryTransitionRequest& request) {
    RuntimeInventoryTransitionResult result{.succeeded = true, .state = state};
    RuntimeInventoryTransitionRow row{
        .kind = request.kind,
        .status = RuntimeInventoryTransitionStatus::accepted,
        .item_id = request.item_id,
        .recipe_id = request.recipe_id,
        .quantity = request.quantity,
    };

    if (!is_catalog_runtime_usable(catalog)) {
        mark_transition_invalid(result, std::move(row),
                                RuntimeInventoryTransitionDiagnostic{
                                    .code = RuntimeInventoryTransitionDiagnosticCode::invalid_catalog,
                                });
        return result;
    }
    if (!validate_runtime_inventory_state(catalog, state).succeeded) {
        mark_transition_invalid(result, std::move(row),
                                RuntimeInventoryTransitionDiagnostic{
                                    .code = RuntimeInventoryTransitionDiagnosticCode::invalid_state,
                                });
        return result;
    }

    auto counts = aggregate_inventory_state(state);
    switch (request.kind) {
    case RuntimeInventoryTransitionKind::add_item: {
        if (request.quantity == 0U) {
            row.status = RuntimeInventoryTransitionStatus::ignored;
            add_transition_row(result, std::move(row));
            return result;
        }
        if (find_item(catalog, request.item_id) == nullptr) {
            mark_transition_invalid(result, std::move(row),
                                    RuntimeInventoryTransitionDiagnostic{
                                        .code = RuntimeInventoryTransitionDiagnosticCode::missing_item,
                                        .item_id = request.item_id,
                                        .quantity = request.quantity,
                                    });
            return result;
        }
        add_quantity(counts, request.item_id, request.quantity);
        result.state = make_inventory_state_from_counts(catalog, counts);
        add_transition_row(result, std::move(row));
        return result;
    }
    case RuntimeInventoryTransitionKind::remove_item: {
        if (request.quantity == 0U) {
            row.status = RuntimeInventoryTransitionStatus::ignored;
            add_transition_row(result, std::move(row));
            return result;
        }
        if (find_item(catalog, request.item_id) == nullptr) {
            mark_transition_invalid(result, std::move(row),
                                    RuntimeInventoryTransitionDiagnostic{
                                        .code = RuntimeInventoryTransitionDiagnosticCode::missing_item,
                                        .item_id = request.item_id,
                                        .quantity = request.quantity,
                                    });
            return result;
        }
        if (available_quantity(counts, request.item_id) < request.quantity) {
            mark_transition_blocked(result, std::move(row),
                                    RuntimeInventoryTransitionDiagnostic{
                                        .code = RuntimeInventoryTransitionDiagnosticCode::insufficient_items,
                                        .item_id = request.item_id,
                                        .quantity = request.quantity,
                                    });
            return result;
        }
        remove_quantity(counts, request.item_id, request.quantity);
        result.state = make_inventory_state_from_counts(catalog, counts);
        add_transition_row(result, std::move(row));
        return result;
    }
    case RuntimeInventoryTransitionKind::craft_recipe: {
        const auto* recipe = find_recipe(recipes, request.recipe_id);
        if (recipe == nullptr) {
            mark_transition_invalid(result, std::move(row),
                                    RuntimeInventoryTransitionDiagnostic{
                                        .code = RuntimeInventoryTransitionDiagnosticCode::missing_recipe,
                                        .recipe_id = request.recipe_id,
                                    });
            return result;
        }
        if (!is_recipe_valid(catalog, *recipe)) {
            mark_transition_invalid(result, std::move(row),
                                    RuntimeInventoryTransitionDiagnostic{
                                        .code = RuntimeInventoryTransitionDiagnosticCode::invalid_recipe,
                                        .recipe_id = request.recipe_id,
                                    });
            return result;
        }
        for (const auto& input : recipe->inputs) {
            if (available_quantity(counts, input.item_id) < input.quantity) {
                mark_transition_blocked(result, std::move(row),
                                        RuntimeInventoryTransitionDiagnostic{
                                            .code = RuntimeInventoryTransitionDiagnosticCode::insufficient_items,
                                            .item_id = input.item_id,
                                            .recipe_id = request.recipe_id,
                                            .quantity = input.quantity,
                                        });
                return result;
            }
        }
        for (const auto& input : recipe->inputs) {
            remove_quantity(counts, input.item_id, input.quantity);
        }
        for (const auto& output : recipe->outputs) {
            add_quantity(counts, output.item_id, output.quantity);
        }
        row.status = RuntimeInventoryTransitionStatus::completed;
        result.state = make_inventory_state_from_counts(catalog, counts);
        add_transition_row(result, std::move(row));
        return result;
    }
    }

    mark_transition_invalid(result, std::move(row),
                            RuntimeInventoryTransitionDiagnostic{
                                .code = RuntimeInventoryTransitionDiagnosticCode::invalid_recipe,
                            });
    return result;
}

} // namespace mirakana::runtime
