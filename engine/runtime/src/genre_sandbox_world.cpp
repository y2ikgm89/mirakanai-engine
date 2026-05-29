// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/genre_sandbox_world.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::runtime {
namespace {

struct RuntimeSandboxChunkLookupRow {
    std::string chunk_id;
    RuntimeSandboxCellCoord origin;
    RuntimeSandboxCellCoord end_exclusive;
    bool resident{false};
};

struct RuntimeSandboxCellLookupRow {
    std::string chunk_id;
    RuntimeSandboxCellCoord coord;
    std::string block_id;
    bool destructible{false};
    bool protected_cell{false};
};

[[nodiscard]] bool is_valid_id(std::string_view id) {
    return !id.empty() && std::ranges::none_of(id, [](char ch) {
        const auto value = static_cast<unsigned char>(ch);
        return value < 0x20U || ch == '\\' || ch == '/' || ch == ':';
    });
}

[[nodiscard]] bool is_token_char(char ch) noexcept {
    const auto value = static_cast<unsigned char>(ch);
    return (value >= static_cast<unsigned char>('a') && value <= static_cast<unsigned char>('z')) ||
           (value >= static_cast<unsigned char>('A') && value <= static_cast<unsigned char>('Z')) ||
           (value >= static_cast<unsigned char>('0') && value <= static_cast<unsigned char>('9')) || ch == '_';
}

[[nodiscard]] char lower_ascii(char ch) noexcept {
    if (ch >= 'A' && ch <= 'Z') {
        return static_cast<char>(ch - 'A' + 'a');
    }
    return ch;
}

[[nodiscard]] bool is_forbidden_backend_token(std::string_view token) {
    constexpr auto forbidden = std::array<std::string_view, 10U>{
        "backend", "native", "renderer", "rhi", "d3d12", "vulkan", "sdl", "sdl3", "imgui", "gpu",
    };
    return std::ranges::find(forbidden, token) != forbidden.end();
}

[[nodiscard]] bool has_backend_reference(std::string_view value) {
    std::string token;
    for (const auto ch : value) {
        if (is_token_char(ch)) {
            token.push_back(lower_ascii(ch));
            continue;
        }
        if (is_forbidden_backend_token(token)) {
            return true;
        }
        token.clear();
    }
    return is_forbidden_backend_token(token);
}

[[nodiscard]] bool has_game_owned_content_reference(std::string_view value) {
    std::string lower;
    lower.reserve(value.size());
    for (const auto ch : value) {
        lower.push_back(lower_ascii(ch));
    }
    constexpr auto forbidden = std::array<std::string_view, 8U>{
        "boss", "npc", "damage_formula", "damage-formula", "damage.formula", "economy", "price", "currency",
    };
    return std::ranges::any_of(forbidden,
                               [&lower](std::string_view token) { return lower.find(token) != std::string::npos; });
}

[[nodiscard]] bool same_coord(RuntimeSandboxCellCoord lhs, RuntimeSandboxCellCoord rhs) noexcept {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

[[nodiscard]] bool coord_less(RuntimeSandboxCellCoord lhs, RuntimeSandboxCellCoord rhs) noexcept {
    if (lhs.x != rhs.x) {
        return lhs.x < rhs.x;
    }
    if (lhs.y != rhs.y) {
        return lhs.y < rhs.y;
    }
    return lhs.z < rhs.z;
}

[[nodiscard]] std::string make_cell_key(std::string_view chunk_id, RuntimeSandboxCellCoord coord) {
    auto key = std::string{chunk_id};
    key.append("\n");
    key.append(std::to_string(coord.x));
    key.append("\n");
    key.append(std::to_string(coord.y));
    key.append("\n");
    key.append(std::to_string(coord.z));
    return key;
}

[[nodiscard]] std::string make_pair_key(std::string_view first, std::string_view second) {
    std::string key;
    key.reserve(first.size() + second.size() + 1U);
    key.append(first);
    key.push_back('\n');
    key.append(second);
    return key;
}

[[nodiscard]] bool contains_value(const std::vector<std::string>& values, std::string_view value) {
    return std::ranges::any_of(values, [value](const auto& candidate) { return candidate == value; });
}

void add_diagnostic(RuntimeSandboxWorldMutationPlan& plan, RuntimeSandboxDiagnosticCode code,
                    const RuntimeSandboxWorldMutationRequest& request, std::string row_id, std::string chunk_id,
                    RuntimeSandboxCellCoord coord, std::string message, std::uint32_t source_index) {
    plan.diagnostics.push_back(RuntimeSandboxDiagnostic{
        .code = code,
        .world_id = request.world_id,
        .row_id = std::move(row_id),
        .chunk_id = std::move(chunk_id),
        .coord = coord,
        .message = std::move(message),
        .source_index = source_index,
    });
}

void sort_diagnostics(RuntimeSandboxWorldMutationPlan& plan) {
    std::ranges::sort(plan.diagnostics, [](const auto& lhs, const auto& rhs) {
        if (lhs.code != rhs.code) {
            return static_cast<std::uint8_t>(lhs.code) < static_cast<std::uint8_t>(rhs.code);
        }
        if (lhs.world_id != rhs.world_id) {
            return lhs.world_id < rhs.world_id;
        }
        if (lhs.chunk_id != rhs.chunk_id) {
            return lhs.chunk_id < rhs.chunk_id;
        }
        if (lhs.row_id != rhs.row_id) {
            return lhs.row_id < rhs.row_id;
        }
        if (!same_coord(lhs.coord, rhs.coord)) {
            return coord_less(lhs.coord, rhs.coord);
        }
        if (lhs.source_index != rhs.source_index) {
            return lhs.source_index < rhs.source_index;
        }
        return lhs.message < rhs.message;
    });
}

[[nodiscard]] std::size_t request_row_count(const RuntimeSandboxWorldMutationRequest& request) {
    auto provided_cost_count = std::size_t{0U};
    for (const auto& intent : request.placement_intents) {
        provided_cost_count += intent.provided_costs.size();
    }
    return request.chunk_rows.size() + request.existing_cell_rows.size() + request.placement_intents.size() +
           request.destruction_intents.size() + request.construction_cost_rows.size() +
           request.persistence_rows.size() + request.tile_drop_rows.size() + request.tool_effectiveness_rows.size() +
           request.spawn_region_rows.size() + request.day_night_event_rows.size() + request.trigger_rows.size() +
           request.game_content_rule_ids.size() + provided_cost_count;
}

[[nodiscard]] std::size_t output_row_count(const RuntimeSandboxWorldMutationPlan& plan) {
    return plan.chunk_rows.size() + plan.existing_cell_rows.size() + plan.placement_intent_rows.size() +
           plan.destruction_intent_rows.size() + plan.construction_cost_rows.size() + plan.mutation_rows.size() +
           plan.persistence_rows.size() + plan.tile_drop_rows.size() + plan.construction_cost_consumption_rows.size() +
           plan.tool_effectiveness_rows.size() + plan.spawn_region_rows.size() + plan.day_night_event_rows.size() +
           plan.trigger_rows.size();
}

void clear_output_rows(RuntimeSandboxWorldMutationPlan& plan) {
    plan.chunk_rows.clear();
    plan.existing_cell_rows.clear();
    plan.placement_intent_rows.clear();
    plan.destruction_intent_rows.clear();
    plan.construction_cost_rows.clear();
    plan.mutation_rows.clear();
    plan.persistence_rows.clear();
    plan.tile_drop_rows.clear();
    plan.construction_cost_consumption_rows.clear();
    plan.tool_effectiveness_rows.clear();
    plan.spawn_region_rows.clear();
    plan.day_night_event_rows.clear();
    plan.trigger_rows.clear();
    plan.chunk_count = 0U;
    plan.resident_chunk_count = 0U;
    plan.placement_intent_count = 0U;
    plan.accepted_placement_count = 0U;
    plan.rejected_placement_count = 0U;
    plan.destruction_intent_count = 0U;
    plan.accepted_destruction_count = 0U;
    plan.rejected_destruction_count = 0U;
    plan.construction_cost_count = 0U;
    plan.persistence_row_count = 0U;
    plan.repairable_persistence_row_count = 0U;
    plan.tile_drop_count = 0U;
    plan.construction_cost_consumption_count = 0U;
    plan.tool_effectiveness_count = 0U;
    plan.spawn_region_count = 0U;
    plan.day_night_event_count = 0U;
    plan.trigger_count = 0U;
    plan.rejected_unsafe_mutation_count = 0U;
    plan.replay_hash = 0U;
}

[[nodiscard]] bool is_valid_chunk_extent(const RuntimeSandboxChunkRow& row) noexcept {
    if (row.size_x == 0U || row.size_y == 0U || row.size_z == 0U) {
        return false;
    }
    const auto end_x = static_cast<std::int64_t>(row.origin_x) + static_cast<std::int64_t>(row.size_x);
    const auto end_y = static_cast<std::int64_t>(row.origin_y) + static_cast<std::int64_t>(row.size_y);
    const auto end_z = static_cast<std::int64_t>(row.origin_z) + static_cast<std::int64_t>(row.size_z);
    return end_x <= static_cast<std::int64_t>(INT32_MAX) && end_y <= static_cast<std::int64_t>(INT32_MAX) &&
           end_z <= static_cast<std::int64_t>(INT32_MAX);
}

[[nodiscard]] RuntimeSandboxCellCoord end_coord(const RuntimeSandboxChunkRow& row) noexcept {
    return RuntimeSandboxCellCoord{
        .x = static_cast<std::int32_t>(static_cast<std::int64_t>(row.origin_x) + static_cast<std::int64_t>(row.size_x)),
        .y = static_cast<std::int32_t>(static_cast<std::int64_t>(row.origin_y) + static_cast<std::int64_t>(row.size_y)),
        .z = static_cast<std::int32_t>(static_cast<std::int64_t>(row.origin_z) + static_cast<std::int64_t>(row.size_z)),
    };
}

[[nodiscard]] bool contains_coord(const RuntimeSandboxChunkLookupRow& chunk, RuntimeSandboxCellCoord coord) noexcept {
    return coord.x >= chunk.origin.x && coord.y >= chunk.origin.y && coord.z >= chunk.origin.z &&
           coord.x < chunk.end_exclusive.x && coord.y < chunk.end_exclusive.y && coord.z < chunk.end_exclusive.z;
}

[[nodiscard]] const RuntimeSandboxChunkLookupRow* find_chunk(const std::vector<RuntimeSandboxChunkLookupRow>& chunks,
                                                             std::string_view chunk_id) {
    const auto iter =
        std::ranges::find_if(chunks, [chunk_id](const auto& chunk) { return chunk.chunk_id == chunk_id; });
    if (iter == chunks.end()) {
        return nullptr;
    }
    return &(*iter);
}

[[nodiscard]] const RuntimeSandboxCellLookupRow* find_cell(const std::vector<RuntimeSandboxCellLookupRow>& cells,
                                                           std::string_view chunk_id, RuntimeSandboxCellCoord coord) {
    const auto iter = std::ranges::find_if(cells, [chunk_id, coord](const auto& cell) {
        return cell.chunk_id == chunk_id && same_coord(cell.coord, coord);
    });
    if (iter == cells.end()) {
        return nullptr;
    }
    return &(*iter);
}

void validate_top_level(RuntimeSandboxWorldMutationPlan& plan, const RuntimeSandboxWorldMutationRequest& request) {
    if (!is_valid_id(request.world_id)) {
        add_diagnostic(plan, RuntimeSandboxDiagnosticCode::missing_world_id, request, request.world_id, {}, {},
                       "runtime sandbox world id must be non-empty and path-safe", 0U);
    }
    if (has_backend_reference(request.world_id)) {
        add_diagnostic(plan, RuntimeSandboxDiagnosticCode::unsupported_backend_reference, request, request.world_id, {},
                       {}, "runtime sandbox world rows must not encode renderer/platform/backend references", 0U);
    }
    if (request_row_count(request) > request.row_budget) {
        add_diagnostic(plan, RuntimeSandboxDiagnosticCode::row_budget_exceeded, request, request.world_id, {}, {},
                       "runtime sandbox world request exceeds its review row budget", 0U);
    }
    for (std::size_t index = 0U; index < request.game_content_rule_ids.size(); ++index) {
        add_diagnostic(plan, RuntimeSandboxDiagnosticCode::unsupported_game_content_rule, request,
                       request.game_content_rule_ids[index], {}, {},
                       "biome, balance, block-art, and content rules remain game-owned",
                       static_cast<std::uint32_t>(index));
    }
}

[[nodiscard]] std::vector<RuntimeSandboxChunkLookupRow>
validate_chunks(RuntimeSandboxWorldMutationPlan& plan, const RuntimeSandboxWorldMutationRequest& request) {
    std::vector<std::string> chunk_ids;
    std::vector<RuntimeSandboxChunkLookupRow> chunks;
    chunk_ids.reserve(request.chunk_rows.size());
    chunks.reserve(request.chunk_rows.size());
    for (const auto& row : request.chunk_rows) {
        auto valid = true;
        if (!is_valid_id(row.chunk_id) || !is_valid_id(row.region_id) || has_backend_reference(row.chunk_id) ||
            has_backend_reference(row.region_id)) {
            add_diagnostic(plan, RuntimeSandboxDiagnosticCode::invalid_chunk_id, request, row.chunk_id, row.chunk_id,
                           {}, "sandbox chunk and region ids must be path-safe and backend-neutral", row.source_index);
            valid = false;
        }
        if (contains_value(chunk_ids, row.chunk_id)) {
            add_diagnostic(plan, RuntimeSandboxDiagnosticCode::duplicate_chunk, request, row.chunk_id, row.chunk_id, {},
                           "sandbox chunk ids must be unique", row.source_index);
            valid = false;
        } else {
            chunk_ids.push_back(row.chunk_id);
        }
        if (!is_valid_chunk_extent(row)) {
            add_diagnostic(plan, RuntimeSandboxDiagnosticCode::invalid_chunk_extent, request, row.chunk_id,
                           row.chunk_id, {}, "sandbox chunk extents must be non-empty and fit int32 coordinates",
                           row.source_index);
            valid = false;
        }
        if (valid) {
            chunks.push_back(RuntimeSandboxChunkLookupRow{
                .chunk_id = row.chunk_id,
                .origin = RuntimeSandboxCellCoord{.x = row.origin_x, .y = row.origin_y, .z = row.origin_z},
                .end_exclusive = end_coord(row),
                .resident = row.resident,
            });
        }
    }
    return chunks;
}

[[nodiscard]] std::vector<RuntimeSandboxCellLookupRow>
validate_existing_cells(RuntimeSandboxWorldMutationPlan& plan, const RuntimeSandboxWorldMutationRequest& request,
                        const std::vector<RuntimeSandboxChunkLookupRow>& chunks) {
    std::vector<std::string> cell_keys;
    std::vector<RuntimeSandboxCellLookupRow> cells;
    cell_keys.reserve(request.existing_cell_rows.size());
    cells.reserve(request.existing_cell_rows.size());
    for (const auto& row : request.existing_cell_rows) {
        auto valid = true;
        const auto* chunk = find_chunk(chunks, row.chunk_id);
        if (chunk == nullptr) {
            add_diagnostic(plan, RuntimeSandboxDiagnosticCode::unknown_existing_cell_chunk, request, row.block_id,
                           row.chunk_id, row.coord, "existing sandbox cells must reference a declared chunk",
                           row.source_index);
            valid = false;
        } else if (!contains_coord(*chunk, row.coord)) {
            add_diagnostic(plan, RuntimeSandboxDiagnosticCode::invalid_existing_cell, request, row.block_id,
                           row.chunk_id, row.coord, "existing sandbox cell coordinates must be inside their chunk",
                           row.source_index);
            valid = false;
        }
        if (!is_valid_id(row.block_id) || has_backend_reference(row.block_id)) {
            add_diagnostic(plan, RuntimeSandboxDiagnosticCode::invalid_existing_cell, request, row.block_id,
                           row.chunk_id, row.coord, "existing sandbox cells require backend-neutral block ids",
                           row.source_index);
            valid = false;
        }
        const auto cell_key = make_cell_key(row.chunk_id, row.coord);
        if (contains_value(cell_keys, cell_key)) {
            add_diagnostic(plan, RuntimeSandboxDiagnosticCode::duplicate_existing_cell, request, row.block_id,
                           row.chunk_id, row.coord, "existing sandbox cells must be unique per chunk coordinate",
                           row.source_index);
            valid = false;
        } else {
            cell_keys.push_back(cell_key);
        }
        if (valid) {
            cells.push_back(RuntimeSandboxCellLookupRow{
                .chunk_id = row.chunk_id,
                .coord = row.coord,
                .block_id = row.block_id,
                .destructible = row.destructible,
                .protected_cell = row.protected_cell,
            });
        }
    }
    return cells;
}

void validate_cost_rows(RuntimeSandboxWorldMutationPlan& plan, const RuntimeSandboxWorldMutationRequest& request) {
    std::vector<std::string> cost_keys;
    cost_keys.reserve(request.construction_cost_rows.size());
    for (const auto& row : request.construction_cost_rows) {
        if (!is_valid_id(row.block_id) || !is_valid_id(row.item_id) || has_backend_reference(row.block_id) ||
            has_backend_reference(row.item_id) || row.quantity == 0U) {
            add_diagnostic(plan, RuntimeSandboxDiagnosticCode::invalid_construction_cost, request, row.item_id,
                           row.block_id, {}, "sandbox construction costs require block/item ids and positive quantity",
                           row.source_index);
        }
        const auto key = make_pair_key(row.block_id, row.item_id);
        if (contains_value(cost_keys, key)) {
            add_diagnostic(plan, RuntimeSandboxDiagnosticCode::duplicate_construction_cost, request, row.item_id,
                           row.block_id, {}, "sandbox construction cost rows must be unique per block and item",
                           row.source_index);
        } else {
            cost_keys.push_back(key);
        }
    }
}

void validate_intent_ids(RuntimeSandboxWorldMutationPlan& plan, const RuntimeSandboxWorldMutationRequest& request) {
    std::vector<std::string> placement_ids;
    std::vector<std::string> destruction_ids;
    placement_ids.reserve(request.placement_intents.size());
    destruction_ids.reserve(request.destruction_intents.size());
    for (const auto& intent : request.placement_intents) {
        if (!is_valid_id(intent.intent_id) || !is_valid_id(intent.chunk_id) || !is_valid_id(intent.block_id) ||
            has_backend_reference(intent.intent_id) || has_backend_reference(intent.chunk_id) ||
            has_backend_reference(intent.block_id)) {
            add_diagnostic(plan, RuntimeSandboxDiagnosticCode::invalid_placement_intent, request, intent.intent_id,
                           intent.chunk_id, intent.coord,
                           "sandbox placement intents require backend-neutral intent, chunk, and block ids",
                           intent.source_index);
        }
        if (contains_value(placement_ids, intent.intent_id)) {
            add_diagnostic(plan, RuntimeSandboxDiagnosticCode::duplicate_placement_intent, request, intent.intent_id,
                           intent.chunk_id, intent.coord, "sandbox placement intent ids must be unique",
                           intent.source_index);
        } else {
            placement_ids.push_back(intent.intent_id);
        }
        for (const auto& cost : intent.provided_costs) {
            if (!is_valid_id(cost.block_id) || !is_valid_id(cost.item_id) || has_backend_reference(cost.block_id) ||
                has_backend_reference(cost.item_id) || cost.quantity == 0U) {
                add_diagnostic(plan, RuntimeSandboxDiagnosticCode::invalid_construction_cost, request, cost.item_id,
                               cost.block_id, intent.coord,
                               "provided sandbox construction costs require block/item ids and positive quantity",
                               cost.source_index);
            }
        }
    }
    for (const auto& intent : request.destruction_intents) {
        if (!is_valid_id(intent.intent_id) || !is_valid_id(intent.chunk_id) ||
            has_backend_reference(intent.intent_id) || has_backend_reference(intent.chunk_id)) {
            add_diagnostic(plan, RuntimeSandboxDiagnosticCode::invalid_destruction_intent, request, intent.intent_id,
                           intent.chunk_id, intent.coord,
                           "sandbox destruction intents require backend-neutral intent and chunk ids",
                           intent.source_index);
        }
        if (contains_value(destruction_ids, intent.intent_id)) {
            add_diagnostic(plan, RuntimeSandboxDiagnosticCode::duplicate_destruction_intent, request, intent.intent_id,
                           intent.chunk_id, intent.coord, "sandbox destruction intent ids must be unique",
                           intent.source_index);
        } else {
            destruction_ids.push_back(intent.intent_id);
        }
    }
}

void validate_persistence_rows(RuntimeSandboxWorldMutationPlan& plan, const RuntimeSandboxWorldMutationRequest& request,
                               const std::vector<RuntimeSandboxChunkLookupRow>& chunks) {
    std::vector<std::string> keys;
    keys.reserve(request.persistence_rows.size());
    for (const auto& row : request.persistence_rows) {
        if (!is_valid_id(row.key) || has_backend_reference(row.key) || row.expected_schema_version == 0U ||
            row.observed_schema_version == 0U || find_chunk(chunks, row.chunk_id) == nullptr) {
            add_diagnostic(plan, RuntimeSandboxDiagnosticCode::invalid_persistence_row, request, row.key, row.chunk_id,
                           {}, "sandbox persistence rows require known chunk ids, path-safe keys, and schema versions",
                           row.source_index);
        }
        const auto key = make_pair_key(row.chunk_id, row.key);
        if (contains_value(keys, key)) {
            add_diagnostic(plan, RuntimeSandboxDiagnosticCode::duplicate_persistence_key, request, row.key,
                           row.chunk_id, {}, "sandbox persistence rows must be unique per chunk and key",
                           row.source_index);
        } else {
            keys.push_back(key);
        }
    }
}

void validate_gameplay_hook_value(RuntimeSandboxWorldMutationPlan& plan,
                                  const RuntimeSandboxWorldMutationRequest& request, std::string_view value,
                                  std::string row_id, std::uint32_t source_index) {
    if (has_game_owned_content_reference(value)) {
        add_diagnostic(plan, RuntimeSandboxDiagnosticCode::unsupported_game_content_rule, request, std::move(row_id),
                       {}, {},
                       "sandbox gameplay hooks must stay generic and not encode boss, NPC, damage formula, or economy "
                       "balance catalogs",
                       source_index);
    }
}

void validate_gameplay_hook_rows(RuntimeSandboxWorldMutationPlan& plan,
                                 const RuntimeSandboxWorldMutationRequest& request,
                                 const std::vector<RuntimeSandboxChunkLookupRow>& chunks) {
    std::vector<std::string> tile_drop_keys;
    std::vector<std::string> tool_keys;
    std::vector<std::string> spawn_keys;
    std::vector<std::string> event_ids;
    std::vector<std::string> trigger_ids;

    for (const auto& row : request.tile_drop_rows) {
        const auto key = make_pair_key(row.block_id, row.item_id);
        if (!is_valid_id(row.block_id) || !is_valid_id(row.item_id) ||
            (!row.required_tool_category_id.empty() && !is_valid_id(row.required_tool_category_id)) ||
            has_backend_reference(row.block_id) || has_backend_reference(row.item_id) ||
            has_backend_reference(row.required_tool_category_id) || row.min_quantity == 0U ||
            row.max_quantity < row.min_quantity) {
            add_diagnostic(
                plan, RuntimeSandboxDiagnosticCode::invalid_construction_cost, request, row.item_id, row.block_id, {},
                "sandbox tile drops require generic block/item/tool ids and quantity ranges", row.source_index);
        }
        if (contains_value(tile_drop_keys, key)) {
            add_diagnostic(plan, RuntimeSandboxDiagnosticCode::duplicate_construction_cost, request, row.item_id,
                           row.block_id, {}, "sandbox tile drop rows must be unique per block and item",
                           row.source_index);
        } else {
            tile_drop_keys.push_back(key);
        }
        validate_gameplay_hook_value(plan, request, row.block_id, row.block_id, row.source_index);
        validate_gameplay_hook_value(plan, request, row.item_id, row.item_id, row.source_index);
        validate_gameplay_hook_value(plan, request, row.required_tool_category_id, row.required_tool_category_id,
                                     row.source_index);
    }

    for (const auto& row : request.tool_effectiveness_rows) {
        const auto key = make_pair_key(row.tool_category_id, row.block_tag_id);
        if (!is_valid_id(row.tool_category_id) || !is_valid_id(row.block_tag_id) ||
            has_backend_reference(row.tool_category_id) || has_backend_reference(row.block_tag_id) ||
            row.effectiveness_tier == 0U) {
            add_diagnostic(plan, RuntimeSandboxDiagnosticCode::invalid_construction_cost, request, row.tool_category_id,
                           {}, {}, "sandbox tool effectiveness rows require generic tool and block tag ids",
                           row.source_index);
        }
        if (contains_value(tool_keys, key)) {
            add_diagnostic(plan, RuntimeSandboxDiagnosticCode::duplicate_construction_cost, request,
                           row.tool_category_id, {}, {}, "sandbox tool effectiveness rows must be unique",
                           row.source_index);
        } else {
            tool_keys.push_back(key);
        }
        validate_gameplay_hook_value(plan, request, row.tool_category_id, row.tool_category_id, row.source_index);
        validate_gameplay_hook_value(plan, request, row.block_tag_id, row.block_tag_id, row.source_index);
    }

    for (const auto& row : request.spawn_region_rows) {
        const auto key = make_pair_key(row.region_id, row.spawn_group_id);
        if (!is_valid_id(row.region_id) || !is_valid_id(row.spawn_group_id) || has_backend_reference(row.region_id) ||
            has_backend_reference(row.spawn_group_id) || row.max_active == 0U) {
            add_diagnostic(plan, RuntimeSandboxDiagnosticCode::invalid_persistence_row, request, row.spawn_group_id,
                           row.region_id, {}, "sandbox spawn region rows require generic region and spawn group ids",
                           row.source_index);
        }
        if (contains_value(spawn_keys, key)) {
            add_diagnostic(plan, RuntimeSandboxDiagnosticCode::duplicate_persistence_key, request, row.spawn_group_id,
                           row.region_id, {}, "sandbox spawn region rows must be unique", row.source_index);
        } else {
            spawn_keys.push_back(key);
        }
        validate_gameplay_hook_value(plan, request, row.region_id, row.region_id, row.source_index);
        validate_gameplay_hook_value(plan, request, row.spawn_group_id, row.spawn_group_id, row.source_index);
    }

    for (const auto& row : request.day_night_event_rows) {
        if (!is_valid_id(row.event_id) || has_backend_reference(row.event_id) || row.repeat_interval_ticks == 0U) {
            add_diagnostic(plan, RuntimeSandboxDiagnosticCode::invalid_persistence_row, request, row.event_id, {}, {},
                           "sandbox day/night events require generic event ids and repeat intervals", row.source_index);
        }
        if (contains_value(event_ids, row.event_id)) {
            add_diagnostic(plan, RuntimeSandboxDiagnosticCode::duplicate_persistence_key, request, row.event_id, {}, {},
                           "sandbox day/night event ids must be unique", row.source_index);
        } else {
            event_ids.push_back(row.event_id);
        }
        validate_gameplay_hook_value(plan, request, row.event_id, row.event_id, row.source_index);
    }

    for (const auto& row : request.trigger_rows) {
        const auto* chunk = find_chunk(chunks, row.chunk_id);
        if (!is_valid_id(row.trigger_id) || !is_valid_id(row.event_id) || !is_valid_id(row.chunk_id) ||
            !is_valid_id(row.interaction_id) || has_backend_reference(row.trigger_id) ||
            has_backend_reference(row.event_id) || has_backend_reference(row.chunk_id) ||
            has_backend_reference(row.interaction_id) || chunk == nullptr ||
            (chunk != nullptr && !contains_coord(*chunk, row.coord))) {
            add_diagnostic(plan, RuntimeSandboxDiagnosticCode::invalid_placement_intent, request, row.trigger_id,
                           row.chunk_id, row.coord,
                           "sandbox trigger rows require generic ids and known in-chunk coordinates", row.source_index);
        }
        if (contains_value(trigger_ids, row.trigger_id)) {
            add_diagnostic(plan, RuntimeSandboxDiagnosticCode::duplicate_placement_intent, request, row.trigger_id,
                           row.chunk_id, row.coord, "sandbox trigger ids must be unique", row.source_index);
        } else {
            trigger_ids.push_back(row.trigger_id);
        }
        validate_gameplay_hook_value(plan, request, row.trigger_id, row.trigger_id, row.source_index);
        validate_gameplay_hook_value(plan, request, row.event_id, row.event_id, row.source_index);
        validate_gameplay_hook_value(plan, request, row.interaction_id, row.interaction_id, row.source_index);
    }
}

[[nodiscard]] bool provided_costs_cover_required(std::string_view block_id,
                                                 const std::vector<RuntimeSandboxConstructionCostRow>& required,
                                                 const std::vector<RuntimeSandboxConstructionCostRow>& provided) {
    for (const auto& required_cost : required) {
        if (required_cost.block_id != block_id) {
            continue;
        }
        const auto provided_iter = std::ranges::find_if(provided, [&required_cost](const auto& provided_cost) {
            return provided_cost.block_id == required_cost.block_id && provided_cost.item_id == required_cost.item_id &&
                   provided_cost.quantity >= required_cost.quantity;
        });
        if (provided_iter == provided.end()) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] RuntimeSandboxMutationStatus
placement_status_for(const RuntimeSandboxPlacementIntent& intent,
                     const std::vector<RuntimeSandboxChunkLookupRow>& chunks,
                     const std::vector<RuntimeSandboxCellLookupRow>& cells,
                     const std::vector<RuntimeSandboxConstructionCostRow>& costs) {
    const auto* chunk = find_chunk(chunks, intent.chunk_id);
    if (chunk == nullptr || !chunk->resident) {
        return RuntimeSandboxMutationStatus::blocked_missing_chunk;
    }
    if (!contains_coord(*chunk, intent.coord)) {
        return RuntimeSandboxMutationStatus::blocked_out_of_bounds;
    }
    if (find_cell(cells, intent.chunk_id, intent.coord) != nullptr) {
        return RuntimeSandboxMutationStatus::blocked_occupied;
    }
    if (!provided_costs_cover_required(intent.block_id, costs, intent.provided_costs)) {
        return RuntimeSandboxMutationStatus::blocked_missing_cost;
    }
    return RuntimeSandboxMutationStatus::accepted;
}

[[nodiscard]] RuntimeSandboxMutationStatus
destruction_status_for(const RuntimeSandboxDestructionIntent& intent,
                       const std::vector<RuntimeSandboxChunkLookupRow>& chunks,
                       const std::vector<RuntimeSandboxCellLookupRow>& cells) {
    const auto* chunk = find_chunk(chunks, intent.chunk_id);
    if (chunk == nullptr || !chunk->resident) {
        return RuntimeSandboxMutationStatus::blocked_missing_chunk;
    }
    if (!contains_coord(*chunk, intent.coord)) {
        return RuntimeSandboxMutationStatus::blocked_out_of_bounds;
    }
    const auto* existing = find_cell(cells, intent.chunk_id, intent.coord);
    if (existing == nullptr) {
        return RuntimeSandboxMutationStatus::blocked_missing_cell;
    }
    if (existing->protected_cell || !existing->destructible) {
        return RuntimeSandboxMutationStatus::blocked_protected;
    }
    return RuntimeSandboxMutationStatus::accepted;
}

[[nodiscard]] bool rejected(RuntimeSandboxMutationStatus status) noexcept {
    return status != RuntimeSandboxMutationStatus::accepted && status != RuntimeSandboxMutationStatus::invalid;
}

void append_output_rows(RuntimeSandboxWorldMutationPlan& plan, const RuntimeSandboxWorldMutationRequest& request,
                        const std::vector<RuntimeSandboxChunkLookupRow>& chunks,
                        const std::vector<RuntimeSandboxCellLookupRow>& cells) {
    plan.chunk_rows = request.chunk_rows;
    plan.existing_cell_rows = request.existing_cell_rows;
    plan.placement_intent_rows = request.placement_intents;
    plan.destruction_intent_rows = request.destruction_intents;
    plan.construction_cost_rows = request.construction_cost_rows;
    plan.tile_drop_rows = request.tile_drop_rows;
    plan.tool_effectiveness_rows = request.tool_effectiveness_rows;
    plan.spawn_region_rows = request.spawn_region_rows;
    plan.day_night_event_rows = request.day_night_event_rows;
    plan.trigger_rows = request.trigger_rows;
    std::ranges::sort(plan.chunk_rows, [](const auto& lhs, const auto& rhs) { return lhs.chunk_id < rhs.chunk_id; });
    std::ranges::sort(plan.existing_cell_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.chunk_id != rhs.chunk_id) {
            return lhs.chunk_id < rhs.chunk_id;
        }
        return coord_less(lhs.coord, rhs.coord);
    });
    std::ranges::sort(plan.construction_cost_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.block_id != rhs.block_id) {
            return lhs.block_id < rhs.block_id;
        }
        return lhs.item_id < rhs.item_id;
    });
    std::ranges::sort(plan.tile_drop_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.block_id != rhs.block_id) {
            return lhs.block_id < rhs.block_id;
        }
        return lhs.item_id < rhs.item_id;
    });
    std::ranges::sort(plan.tool_effectiveness_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.tool_category_id != rhs.tool_category_id) {
            return lhs.tool_category_id < rhs.tool_category_id;
        }
        return lhs.block_tag_id < rhs.block_tag_id;
    });
    std::ranges::sort(plan.spawn_region_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.region_id != rhs.region_id) {
            return lhs.region_id < rhs.region_id;
        }
        return lhs.spawn_group_id < rhs.spawn_group_id;
    });
    std::ranges::sort(plan.day_night_event_rows,
                      [](const auto& lhs, const auto& rhs) { return lhs.event_id < rhs.event_id; });
    std::ranges::sort(plan.trigger_rows,
                      [](const auto& lhs, const auto& rhs) { return lhs.trigger_id < rhs.trigger_id; });

    for (const auto& intent : request.placement_intents) {
        const auto status = placement_status_for(intent, chunks, cells, request.construction_cost_rows);
        plan.mutation_rows.push_back(RuntimeSandboxMutationRow{
            .kind = RuntimeSandboxMutationKind::placement,
            .status = status,
            .intent_id = intent.intent_id,
            .chunk_id = intent.chunk_id,
            .coord = intent.coord,
            .block_id = intent.block_id,
            .source_index = intent.source_index,
        });
        if (status == RuntimeSandboxMutationStatus::accepted) {
            for (const auto& cost : request.construction_cost_rows) {
                if (cost.block_id != intent.block_id) {
                    continue;
                }
                const auto provided_iter =
                    std::ranges::find_if(intent.provided_costs, [&cost](const auto& provided_cost) {
                        return provided_cost.block_id == cost.block_id && provided_cost.item_id == cost.item_id;
                    });
                const auto consumed_quantity = provided_iter != intent.provided_costs.end()
                                                   ? std::min(provided_iter->quantity, cost.quantity)
                                                   : 0U;
                plan.construction_cost_consumption_rows.push_back(RuntimeSandboxConstructionCostConsumptionRow{
                    .intent_id = intent.intent_id,
                    .block_id = intent.block_id,
                    .item_id = cost.item_id,
                    .required_quantity = cost.quantity,
                    .consumed_quantity = consumed_quantity,
                    .accepted = consumed_quantity >= cost.quantity,
                    .source_index = intent.source_index,
                });
            }
        }
    }
    for (const auto& intent : request.destruction_intents) {
        const auto status = destruction_status_for(intent, chunks, cells);
        const auto* existing = find_cell(cells, intent.chunk_id, intent.coord);
        plan.mutation_rows.push_back(RuntimeSandboxMutationRow{
            .kind = RuntimeSandboxMutationKind::destruction,
            .status = status,
            .intent_id = intent.intent_id,
            .chunk_id = intent.chunk_id,
            .coord = intent.coord,
            .block_id = existing != nullptr ? existing->block_id : std::string{},
            .source_index = intent.source_index,
        });
    }
    for (auto row : request.persistence_rows) {
        if (row.observed_schema_version == row.expected_schema_version) {
            row.status = RuntimeSandboxPersistenceStatus::accepted;
        } else if (row.observed_schema_version < row.expected_schema_version) {
            row.status = RuntimeSandboxPersistenceStatus::repairable;
        } else {
            row.status = RuntimeSandboxPersistenceStatus::rejected;
        }
        plan.persistence_rows.push_back(std::move(row));
    }
}

void update_counts(RuntimeSandboxWorldMutationPlan& plan) {
    plan.chunk_count = plan.chunk_rows.size();
    plan.resident_chunk_count =
        static_cast<std::size_t>(std::ranges::count_if(plan.chunk_rows, [](const auto& row) { return row.resident; }));
    plan.placement_intent_count = plan.placement_intent_rows.size();
    plan.destruction_intent_count = plan.destruction_intent_rows.size();
    plan.construction_cost_count = plan.construction_cost_rows.size();
    plan.persistence_row_count = plan.persistence_rows.size();
    plan.tile_drop_count = plan.tile_drop_rows.size();
    plan.construction_cost_consumption_count = plan.construction_cost_consumption_rows.size();
    plan.tool_effectiveness_count = plan.tool_effectiveness_rows.size();
    plan.spawn_region_count = plan.spawn_region_rows.size();
    plan.day_night_event_count = plan.day_night_event_rows.size();
    plan.trigger_count = plan.trigger_rows.size();
    for (const auto& row : plan.mutation_rows) {
        if (row.kind == RuntimeSandboxMutationKind::placement) {
            if (row.status == RuntimeSandboxMutationStatus::accepted) {
                ++plan.accepted_placement_count;
            } else if (rejected(row.status)) {
                ++plan.rejected_placement_count;
                ++plan.rejected_unsafe_mutation_count;
            }
        } else {
            if (row.status == RuntimeSandboxMutationStatus::accepted) {
                ++plan.accepted_destruction_count;
            } else if (rejected(row.status)) {
                ++plan.rejected_destruction_count;
                ++plan.rejected_unsafe_mutation_count;
            }
        }
    }
    plan.repairable_persistence_row_count =
        static_cast<std::size_t>(std::ranges::count_if(plan.persistence_rows, [](const auto& row) {
            return row.status == RuntimeSandboxPersistenceStatus::repairable;
        }));
}

void mix_hash(std::uint64_t& hash, std::uint64_t value) noexcept {
    hash ^= value + 0x9e3779b97f4a7c15ULL + (hash << 6U) + (hash >> 2U);
}

void mix_hash(std::uint64_t& hash, std::string_view value) noexcept {
    for (const auto ch : value) {
        mix_hash(hash, static_cast<std::uint64_t>(static_cast<unsigned char>(ch)));
    }
    mix_hash(hash, 0xffU);
}

void mix_hash(std::uint64_t& hash, RuntimeSandboxCellCoord coord) noexcept {
    mix_hash(hash, static_cast<std::uint64_t>(static_cast<std::int64_t>(coord.x)));
    mix_hash(hash, static_cast<std::uint64_t>(static_cast<std::int64_t>(coord.y)));
    mix_hash(hash, static_cast<std::uint64_t>(static_cast<std::int64_t>(coord.z)));
}

[[nodiscard]] std::uint64_t compute_replay_hash(const RuntimeSandboxWorldMutationRequest& request,
                                                const RuntimeSandboxWorldMutationPlan& plan) {
    auto hash = std::uint64_t{1469598103934665603ULL};
    mix_hash(hash, request.world_id);
    mix_hash(hash, request.world_tick);
    mix_hash(hash, request.seed);
    mix_hash(hash, static_cast<std::uint64_t>(plan.status));
    mix_hash(hash, plan.chunk_count);
    mix_hash(hash, plan.resident_chunk_count);
    mix_hash(hash, plan.placement_intent_count);
    mix_hash(hash, plan.accepted_placement_count);
    mix_hash(hash, plan.rejected_placement_count);
    mix_hash(hash, plan.destruction_intent_count);
    mix_hash(hash, plan.accepted_destruction_count);
    mix_hash(hash, plan.rejected_destruction_count);
    mix_hash(hash, plan.construction_cost_count);
    mix_hash(hash, plan.persistence_row_count);
    mix_hash(hash, plan.repairable_persistence_row_count);
    mix_hash(hash, plan.tile_drop_count);
    mix_hash(hash, plan.construction_cost_consumption_count);
    mix_hash(hash, plan.tool_effectiveness_count);
    mix_hash(hash, plan.spawn_region_count);
    mix_hash(hash, plan.day_night_event_count);
    mix_hash(hash, plan.trigger_count);
    mix_hash(hash, plan.rejected_unsafe_mutation_count);
    for (const auto& row : plan.chunk_rows) {
        mix_hash(hash, row.chunk_id);
        mix_hash(hash, row.region_id);
        mix_hash(hash, static_cast<std::uint64_t>(static_cast<std::int64_t>(row.origin_x)));
        mix_hash(hash, static_cast<std::uint64_t>(static_cast<std::int64_t>(row.origin_y)));
        mix_hash(hash, static_cast<std::uint64_t>(static_cast<std::int64_t>(row.origin_z)));
        mix_hash(hash, row.size_x);
        mix_hash(hash, row.size_y);
        mix_hash(hash, row.size_z);
        mix_hash(hash, row.resident ? 1U : 0U);
        mix_hash(hash, row.persistent ? 1U : 0U);
        mix_hash(hash, row.source_index);
    }
    for (const auto& row : plan.existing_cell_rows) {
        mix_hash(hash, row.chunk_id);
        mix_hash(hash, row.coord);
        mix_hash(hash, row.block_id);
        mix_hash(hash, row.destructible ? 1U : 0U);
        mix_hash(hash, row.protected_cell ? 1U : 0U);
        mix_hash(hash, row.source_index);
    }
    for (const auto& row : plan.placement_intent_rows) {
        mix_hash(hash, row.intent_id);
        mix_hash(hash, row.chunk_id);
        mix_hash(hash, row.coord);
        mix_hash(hash, row.block_id);
        mix_hash(hash, row.provided_costs.size());
        for (const auto& cost : row.provided_costs) {
            mix_hash(hash, cost.block_id);
            mix_hash(hash, cost.item_id);
            mix_hash(hash, cost.quantity);
            mix_hash(hash, cost.source_index);
        }
        mix_hash(hash, row.source_index);
    }
    for (const auto& row : plan.destruction_intent_rows) {
        mix_hash(hash, row.intent_id);
        mix_hash(hash, row.chunk_id);
        mix_hash(hash, row.coord);
        mix_hash(hash, row.source_index);
    }
    for (const auto& row : plan.construction_cost_rows) {
        mix_hash(hash, row.block_id);
        mix_hash(hash, row.item_id);
        mix_hash(hash, row.quantity);
        mix_hash(hash, row.source_index);
    }
    for (const auto& row : plan.mutation_rows) {
        mix_hash(hash, static_cast<std::uint64_t>(row.kind));
        mix_hash(hash, static_cast<std::uint64_t>(row.status));
        mix_hash(hash, row.intent_id);
        mix_hash(hash, row.chunk_id);
        mix_hash(hash, row.coord);
        mix_hash(hash, row.block_id);
        mix_hash(hash, row.source_index);
    }
    for (const auto& row : plan.persistence_rows) {
        mix_hash(hash, row.chunk_id);
        mix_hash(hash, row.key);
        mix_hash(hash, row.expected_schema_version);
        mix_hash(hash, row.observed_schema_version);
        mix_hash(hash, static_cast<std::uint64_t>(row.status));
        mix_hash(hash, row.source_index);
    }
    for (const auto& row : plan.tile_drop_rows) {
        mix_hash(hash, row.block_id);
        mix_hash(hash, row.item_id);
        mix_hash(hash, row.min_quantity);
        mix_hash(hash, row.max_quantity);
        mix_hash(hash, row.required_tool_category_id);
        mix_hash(hash, row.source_index);
    }
    for (const auto& row : plan.construction_cost_consumption_rows) {
        mix_hash(hash, row.intent_id);
        mix_hash(hash, row.block_id);
        mix_hash(hash, row.item_id);
        mix_hash(hash, row.required_quantity);
        mix_hash(hash, row.consumed_quantity);
        mix_hash(hash, row.accepted ? 1U : 0U);
        mix_hash(hash, row.source_index);
    }
    for (const auto& row : plan.tool_effectiveness_rows) {
        mix_hash(hash, row.tool_category_id);
        mix_hash(hash, row.block_tag_id);
        mix_hash(hash, row.effectiveness_tier);
        mix_hash(hash, row.source_index);
    }
    for (const auto& row : plan.spawn_region_rows) {
        mix_hash(hash, row.region_id);
        mix_hash(hash, row.spawn_group_id);
        mix_hash(hash, row.max_active);
        mix_hash(hash, row.source_index);
    }
    for (const auto& row : plan.day_night_event_rows) {
        mix_hash(hash, row.event_id);
        mix_hash(hash, static_cast<std::uint64_t>(row.phase));
        mix_hash(hash, row.first_tick);
        mix_hash(hash, row.repeat_interval_ticks);
        mix_hash(hash, row.source_index);
    }
    for (const auto& row : plan.trigger_rows) {
        mix_hash(hash, row.trigger_id);
        mix_hash(hash, static_cast<std::uint64_t>(row.kind));
        mix_hash(hash, row.event_id);
        mix_hash(hash, row.chunk_id);
        mix_hash(hash, row.coord);
        mix_hash(hash, row.interaction_id);
        mix_hash(hash, row.source_index);
    }
    mix_hash(hash, plan.invoked_world_mutation ? 1U : 0U);
    mix_hash(hash, plan.invoked_persistence_io ? 1U : 0U);
    mix_hash(hash, plan.invoked_package_io ? 1U : 0U);
    return hash == 0U ? 1U : hash;
}

} // namespace

bool RuntimeSandboxWorldMutationPlan::succeeded() const noexcept {
    return status == RuntimeSandboxWorldStatus::ready || status == RuntimeSandboxWorldStatus::no_rows;
}

RuntimeSandboxWorldMutationPlan plan_runtime_sandbox_world_mutation(const RuntimeSandboxWorldMutationRequest& request) {
    RuntimeSandboxWorldMutationPlan plan;

    validate_top_level(plan, request);
    const auto chunks = validate_chunks(plan, request);
    const auto cells = validate_existing_cells(plan, request, chunks);
    validate_cost_rows(plan, request);
    validate_intent_ids(plan, request);
    validate_persistence_rows(plan, request, chunks);
    validate_gameplay_hook_rows(plan, request, chunks);

    if (!plan.diagnostics.empty()) {
        sort_diagnostics(plan);
        plan.status = RuntimeSandboxWorldStatus::invalid_request;
        clear_output_rows(plan);
        return plan;
    }

    if (request_row_count(request) == 0U) {
        plan.status = RuntimeSandboxWorldStatus::no_rows;
        plan.replay_hash = compute_replay_hash(request, plan);
        return plan;
    }

    append_output_rows(plan, request, chunks, cells);
    update_counts(plan);
    plan.status = RuntimeSandboxWorldStatus::ready;

    if (output_row_count(plan) > request.row_budget) {
        add_diagnostic(plan, RuntimeSandboxDiagnosticCode::row_budget_exceeded, request, request.world_id, {}, {},
                       "runtime sandbox world output rows exceed the review row budget", 0U);
        sort_diagnostics(plan);
        plan.status = RuntimeSandboxWorldStatus::invalid_request;
        clear_output_rows(plan);
        return plan;
    }

    plan.replay_hash = compute_replay_hash(request, plan);
    return plan;
}

} // namespace mirakana::runtime
