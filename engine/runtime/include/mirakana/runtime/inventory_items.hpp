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

[[nodiscard]] RuntimeItemCatalogValidationResult
validate_runtime_item_catalog_document(const RuntimeItemCatalogDocument& document,
                                       const RuntimeItemCatalogValidationContext& context);

} // namespace mirakana::runtime
