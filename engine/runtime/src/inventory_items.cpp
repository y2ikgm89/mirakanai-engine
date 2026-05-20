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

void add_diagnostic(std::vector<RuntimeItemCatalogDiagnostic>& diagnostics, RuntimeItemCatalogDiagnostic diagnostic) {
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

} // namespace mirakana::runtime
