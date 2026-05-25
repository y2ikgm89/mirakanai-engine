// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/world_entity_model.hpp"

#include <algorithm>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::runtime {
namespace {

[[nodiscard]] bool is_valid_id(std::string_view id) {
    return !id.empty() && std::ranges::none_of(id, [](char ch) {
        const auto value = static_cast<unsigned char>(ch);
        return value < 0x20U || ch == '\\' || ch == '/' || ch == ':';
    });
}

void add_diagnostic(RuntimeWorldEntityLifecyclePlan& plan, RuntimeWorldEntityDiagnosticCode code, std::string world_id,
                    RuntimeWorldEntityId entity_id, RuntimeWorldRegionId region_id,
                    RuntimeWorldComponentSchemaId schema_id, std::string message, std::uint32_t source_index) {
    plan.diagnostics.push_back(RuntimeWorldEntityDiagnostic{
        .code = code,
        .world_id = std::move(world_id),
        .entity_id = std::move(entity_id),
        .region_id = std::move(region_id),
        .schema_id = std::move(schema_id),
        .message = std::move(message),
        .source_index = source_index,
    });
}

[[nodiscard]] bool contains_value(const std::vector<std::string>& values, std::string_view value) {
    return std::ranges::find(values, value) != values.end();
}

void sort_diagnostics(RuntimeWorldEntityLifecyclePlan& plan) {
    std::ranges::sort(plan.diagnostics, [](const auto& lhs, const auto& rhs) {
        if (lhs.code != rhs.code) {
            return static_cast<std::uint8_t>(lhs.code) < static_cast<std::uint8_t>(rhs.code);
        }
        if (lhs.entity_id.value != rhs.entity_id.value) {
            return lhs.entity_id.value < rhs.entity_id.value;
        }
        if (lhs.region_id.value != rhs.region_id.value) {
            return lhs.region_id.value < rhs.region_id.value;
        }
        if (lhs.schema_id.value != rhs.schema_id.value) {
            return lhs.schema_id.value < rhs.schema_id.value;
        }
        return lhs.source_index < rhs.source_index;
    });
}

void sort_plan_rows(RuntimeWorldEntityLifecyclePlan& plan) {
    std::ranges::sort(plan.region_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.region_id.value != rhs.region_id.value) {
            return lhs.region_id.value < rhs.region_id.value;
        }
        return lhs.source_index < rhs.source_index;
    });
    std::ranges::sort(plan.entity_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.entity_id.value != rhs.entity_id.value) {
            return lhs.entity_id.value < rhs.entity_id.value;
        }
        return lhs.source_index < rhs.source_index;
    });
    std::ranges::sort(plan.component_schema_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.schema_id.value != rhs.schema_id.value) {
            return lhs.schema_id.value < rhs.schema_id.value;
        }
        return lhs.source_index < rhs.source_index;
    });
    std::ranges::sort(plan.component_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.entity_id.value != rhs.entity_id.value) {
            return lhs.entity_id.value < rhs.entity_id.value;
        }
        if (lhs.schema_id.value != rhs.schema_id.value) {
            return lhs.schema_id.value < rhs.schema_id.value;
        }
        return lhs.source_index < rhs.source_index;
    });
    std::ranges::sort(plan.lifecycle_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.source_index != rhs.source_index) {
            return lhs.source_index < rhs.source_index;
        }
        if (lhs.entity_id.value != rhs.entity_id.value) {
            return lhs.entity_id.value < rhs.entity_id.value;
        }
        return static_cast<std::uint8_t>(lhs.action) < static_cast<std::uint8_t>(rhs.action);
    });
    std::ranges::sort(plan.region_ownership_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.entity_id.value != rhs.entity_id.value) {
            return lhs.entity_id.value < rhs.entity_id.value;
        }
        if (lhs.region_id.value != rhs.region_id.value) {
            return lhs.region_id.value < rhs.region_id.value;
        }
        return lhs.source_index < rhs.source_index;
    });
    std::ranges::sort(plan.persistence_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.entity_id.value != rhs.entity_id.value) {
            return lhs.entity_id.value < rhs.entity_id.value;
        }
        if (lhs.region_id.value != rhs.region_id.value) {
            return lhs.region_id.value < rhs.region_id.value;
        }
        return lhs.world_tick < rhs.world_tick;
    });
    std::ranges::sort(plan.streaming_region_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.region_id.value != rhs.region_id.value) {
            return lhs.region_id.value < rhs.region_id.value;
        }
        return static_cast<std::uint8_t>(lhs.action) < static_cast<std::uint8_t>(rhs.action);
    });
}

struct RuntimeWorldEntityValidatedIds {
    std::vector<std::string> region_ids;
    std::vector<std::string> entity_ids;
    std::vector<std::string> component_schema_ids;
};

[[nodiscard]] RuntimeWorldEntityValidatedIds validate_request(RuntimeWorldEntityLifecyclePlan& plan,
                                                              const RuntimeWorldEntityLifecycleRequest& request) {
    RuntimeWorldEntityValidatedIds ids;

    if (!is_valid_id(request.world_id)) {
        add_diagnostic(plan, RuntimeWorldEntityDiagnosticCode::missing_world_id, request.world_id, {}, {}, {},
                       "runtime world id must be non-empty and path-safe", 0U);
    }

    ids.region_ids.reserve(request.regions.size());
    for (const auto& region : request.regions) {
        if (!is_valid_id(region.region_id.value)) {
            add_diagnostic(plan, RuntimeWorldEntityDiagnosticCode::missing_region_id, request.world_id, {},
                           region.region_id, {}, "runtime world region id must be non-empty and path-safe",
                           region.source_index);
        } else if (contains_value(ids.region_ids, region.region_id.value)) {
            add_diagnostic(plan, RuntimeWorldEntityDiagnosticCode::duplicate_region_id, request.world_id, {},
                           region.region_id, {}, "runtime world region ids must be unique", region.source_index);
        } else {
            ids.region_ids.push_back(region.region_id.value);
        }
    }

    ids.entity_ids.reserve(request.entities.size());
    for (const auto& entity : request.entities) {
        if (!is_valid_id(entity.entity_id.value)) {
            add_diagnostic(plan, RuntimeWorldEntityDiagnosticCode::missing_entity_id, request.world_id,
                           entity.entity_id, entity.region_id, {},
                           "runtime world entity id must be non-empty and path-safe", entity.source_index);
        } else if (contains_value(ids.entity_ids, entity.entity_id.value)) {
            add_diagnostic(plan, RuntimeWorldEntityDiagnosticCode::duplicate_entity_id, request.world_id,
                           entity.entity_id, entity.region_id, {}, "runtime world entity ids must be unique",
                           entity.source_index);
            ++plan.rejected_duplicate_entity_count;
        } else {
            ids.entity_ids.push_back(entity.entity_id.value);
        }

        if (is_valid_id(entity.region_id.value) && !contains_value(ids.region_ids, entity.region_id.value)) {
            add_diagnostic(plan, RuntimeWorldEntityDiagnosticCode::unknown_entity_region, request.world_id,
                           entity.entity_id, entity.region_id, {}, "runtime world entity references an unknown region",
                           entity.source_index);
        }
    }

    ids.component_schema_ids.reserve(request.component_schemas.size());
    for (const auto& schema : request.component_schemas) {
        if (!is_valid_id(schema.schema_id.value)) {
            add_diagnostic(plan, RuntimeWorldEntityDiagnosticCode::missing_component_schema_id, request.world_id, {},
                           {}, schema.schema_id, "runtime world component schema id must be non-empty and path-safe",
                           schema.source_index);
        } else if (contains_value(ids.component_schema_ids, schema.schema_id.value)) {
            add_diagnostic(plan, RuntimeWorldEntityDiagnosticCode::duplicate_component_schema_id, request.world_id, {},
                           {}, schema.schema_id, "runtime world component schema ids must be unique",
                           schema.source_index);
        } else {
            ids.component_schema_ids.push_back(schema.schema_id.value);
        }
    }

    std::vector<std::string> component_keys;
    component_keys.reserve(request.components.size());
    for (const auto& component : request.components) {
        const auto entity_valid = is_valid_id(component.entity_id.value);
        const auto schema_valid = is_valid_id(component.schema_id.value);
        if (!entity_valid) {
            add_diagnostic(plan, RuntimeWorldEntityDiagnosticCode::missing_entity_id, request.world_id,
                           component.entity_id, {}, component.schema_id,
                           "runtime world component must reference a non-empty entity id", component.source_index);
        } else if (!contains_value(ids.entity_ids, component.entity_id.value)) {
            add_diagnostic(plan, RuntimeWorldEntityDiagnosticCode::unknown_component_entity, request.world_id,
                           component.entity_id, {}, component.schema_id,
                           "runtime world component references an unknown entity", component.source_index);
        }

        if (!schema_valid) {
            add_diagnostic(plan, RuntimeWorldEntityDiagnosticCode::missing_component_schema_id, request.world_id,
                           component.entity_id, {}, component.schema_id,
                           "runtime world component must reference a non-empty schema id", component.source_index);
        } else if (!contains_value(ids.component_schema_ids, component.schema_id.value)) {
            add_diagnostic(plan, RuntimeWorldEntityDiagnosticCode::unknown_component_schema, request.world_id,
                           component.entity_id, {}, component.schema_id,
                           "runtime world component references an unknown schema", component.source_index);
        }

        if (entity_valid && schema_valid) {
            const auto key = component.entity_id.value + "\n" + component.schema_id.value;
            if (contains_value(component_keys, key)) {
                add_diagnostic(plan, RuntimeWorldEntityDiagnosticCode::duplicate_component_for_entity, request.world_id,
                               component.entity_id, {}, component.schema_id,
                               "runtime world components must be unique per entity and schema", component.source_index);
            } else {
                component_keys.push_back(key);
            }
        }
    }

    std::vector<std::string> spawn_entity_ids;
    for (const auto& intent : request.lifecycle_intents) {
        const auto entity_valid = is_valid_id(intent.entity_id.value);
        if (!entity_valid) {
            add_diagnostic(plan, RuntimeWorldEntityDiagnosticCode::missing_entity_id, request.world_id,
                           intent.entity_id, intent.target_region_id, {},
                           "runtime world lifecycle intent must reference a non-empty entity id", intent.source_index);
            continue;
        }

        switch (intent.action) {
        case RuntimeWorldEntityLifecycleAction::spawn_entity:
            if (!is_valid_id(intent.target_region_id.value) ||
                !contains_value(ids.region_ids, intent.target_region_id.value)) {
                add_diagnostic(plan, RuntimeWorldEntityDiagnosticCode::move_unknown_region, request.world_id,
                               intent.entity_id, intent.target_region_id, {},
                               "runtime world spawn intent targets an unknown region", intent.source_index);
            }
            if (contains_value(ids.entity_ids, intent.entity_id.value) ||
                contains_value(spawn_entity_ids, intent.entity_id.value)) {
                add_diagnostic(plan, RuntimeWorldEntityDiagnosticCode::spawn_duplicate_entity, request.world_id,
                               intent.entity_id, intent.target_region_id, {},
                               "runtime world spawn intent duplicates an existing or pending entity",
                               intent.source_index);
                ++plan.rejected_duplicate_entity_count;
            } else {
                spawn_entity_ids.push_back(intent.entity_id.value);
            }
            break;
        case RuntimeWorldEntityLifecycleAction::despawn_entity:
            if (!contains_value(ids.entity_ids, intent.entity_id.value)) {
                add_diagnostic(plan, RuntimeWorldEntityDiagnosticCode::despawn_unknown_entity, request.world_id,
                               intent.entity_id, {}, {}, "runtime world despawn intent targets an unknown entity",
                               intent.source_index);
            }
            break;
        case RuntimeWorldEntityLifecycleAction::move_entity_region:
            if (!contains_value(ids.entity_ids, intent.entity_id.value)) {
                add_diagnostic(plan, RuntimeWorldEntityDiagnosticCode::move_unknown_entity, request.world_id,
                               intent.entity_id, intent.target_region_id, {},
                               "runtime world move intent targets an unknown entity", intent.source_index);
            }
            if (!is_valid_id(intent.target_region_id.value) ||
                !contains_value(ids.region_ids, intent.target_region_id.value)) {
                add_diagnostic(plan, RuntimeWorldEntityDiagnosticCode::move_unknown_region, request.world_id,
                               intent.entity_id, intent.target_region_id, {},
                               "runtime world move intent targets an unknown region", intent.source_index);
            }
            break;
        }
    }

    const bool persistence_bridge_ready = request.persistence_bridge.plan.ready();
    if (request.persistence_bridge.required && !persistence_bridge_ready) {
        add_diagnostic(plan, RuntimeWorldEntityDiagnosticCode::persistence_bridge_not_ready, request.world_id, {}, {},
                       {}, "runtime world entity persistence bridge requires a ready persistence plan", 0U);
    }
    if (request.persistence_bridge.required && persistence_bridge_ready &&
        request.persistence_bridge.plan.world_id != request.world_id) {
        add_diagnostic(plan, RuntimeWorldEntityDiagnosticCode::persistence_bridge_world_mismatch, request.world_id, {},
                       {}, {}, "runtime world entity persistence bridge world id must match the request world id", 0U);
    }
    if (request.persistence_bridge.required && persistence_bridge_ready) {
        for (const auto& entity : request.persistence_bridge.plan.entity_rows) {
            if (!contains_value(ids.entity_ids, entity.entity_id)) {
                add_diagnostic(plan, RuntimeWorldEntityDiagnosticCode::persistence_bridge_unknown_entity,
                               request.world_id, RuntimeWorldEntityId{.value = entity.entity_id}, {}, {},
                               "runtime world entity persistence bridge references an unknown entity", 0U);
            }
            if (!contains_value(ids.region_ids, entity.region_id)) {
                add_diagnostic(plan, RuntimeWorldEntityDiagnosticCode::persistence_bridge_unknown_region,
                               request.world_id, RuntimeWorldEntityId{.value = entity.entity_id},
                               RuntimeWorldRegionId{.value = entity.region_id}, {},
                               "runtime world entity persistence bridge references an unknown region", 0U);
            }
        }
    }
    const bool streaming_bridge_succeeded = request.streaming_bridge.plan.succeeded();
    if (request.streaming_bridge.required && !streaming_bridge_succeeded) {
        add_diagnostic(plan, RuntimeWorldEntityDiagnosticCode::streaming_bridge_not_ready, request.world_id, {}, {}, {},
                       "runtime world entity streaming bridge requires a successful streaming plan", 0U);
    }
    if (request.streaming_bridge.required && streaming_bridge_succeeded &&
        !request.streaming_bridge.plan.diagnostics.empty()) {
        add_diagnostic(plan, RuntimeWorldEntityDiagnosticCode::streaming_bridge_diagnostics_present, request.world_id,
                       {}, {}, {}, "runtime world entity streaming bridge must not carry diagnostics", 0U);
    }
    if (request.streaming_bridge.required && streaming_bridge_succeeded) {
        for (const auto& row : request.streaming_bridge.plan.rows) {
            if (!contains_value(ids.region_ids, row.region_id)) {
                add_diagnostic(plan, RuntimeWorldEntityDiagnosticCode::streaming_bridge_unknown_region,
                               request.world_id, {}, RuntimeWorldRegionId{.value = row.region_id}, {},
                               "runtime world entity streaming bridge references an unknown region", 0U);
            }
        }
    }

    return ids;
}

void append_plan_rows(RuntimeWorldEntityLifecyclePlan& plan, const RuntimeWorldEntityLifecycleRequest& request) {
    plan.region_rows = request.regions;
    plan.entity_rows = request.entities;
    plan.component_schema_rows = request.component_schemas;
    plan.component_rows = request.components;

    for (const auto& entity : plan.entity_rows) {
        if (entity.active) {
            plan.region_ownership_rows.push_back(RuntimeWorldEntityRegionOwnershipRow{
                .world_id = request.world_id,
                .entity_id = entity.entity_id,
                .region_id = entity.region_id,
                .active = true,
                .generation = entity.generation,
                .source_index = entity.source_index,
            });
        }
    }

    for (const auto& intent : request.lifecycle_intents) {
        plan.lifecycle_rows.push_back(RuntimeWorldEntityLifecycleRow{
            .world_id = request.world_id,
            .action = intent.action,
            .entity_id = intent.entity_id,
            .target_region_id = intent.target_region_id,
            .archetype_id = intent.archetype_id,
            .source_index = intent.source_index,
        });

        switch (intent.action) {
        case RuntimeWorldEntityLifecycleAction::spawn_entity:
            ++plan.spawn_count;
            break;
        case RuntimeWorldEntityLifecycleAction::despawn_entity:
            ++plan.despawn_count;
            break;
        case RuntimeWorldEntityLifecycleAction::move_entity_region:
            ++plan.move_count;
            break;
        }
    }

    if (request.persistence_bridge.required) {
        for (const auto& entity : request.persistence_bridge.plan.entity_rows) {
            plan.persistence_rows.push_back(RuntimeWorldEntityPersistenceRow{
                .world_id = request.persistence_bridge.plan.world_id,
                .world_tick = request.persistence_bridge.plan.world_tick,
                .entity_id = RuntimeWorldEntityId{.value = entity.entity_id},
                .region_id = RuntimeWorldRegionId{.value = entity.region_id},
                .entity_type = entity.entity_type,
                .state_hash = entity.state_hash,
            });
        }
    }

    if (request.streaming_bridge.required) {
        for (const auto& row : request.streaming_bridge.plan.rows) {
            plan.streaming_region_rows.push_back(RuntimeWorldEntityStreamingRegionRow{
                .action = row.action,
                .region_id = RuntimeWorldRegionId{.value = row.region_id},
                .estimated_resident_bytes = row.estimated_resident_bytes,
                .estimated_asset_records = row.estimated_asset_records,
            });
        }
    }

    sort_plan_rows(plan);
    plan.validated_entity_count = plan.entity_rows.size();
    plan.validated_component_count = plan.component_rows.size();
    plan.region_ownership_count = plan.region_ownership_rows.size();
}

} // namespace

bool RuntimeWorldEntityLifecyclePlan::succeeded() const noexcept {
    return status == RuntimeWorldEntityLifecycleStatus::ready ||
           status == RuntimeWorldEntityLifecycleStatus::no_entities;
}

RuntimeWorldEntityLifecyclePlan plan_runtime_world_entity_lifecycle(const RuntimeWorldEntityLifecycleRequest& request) {
    RuntimeWorldEntityLifecyclePlan plan;

    static_cast<void>(validate_request(plan, request));
    if (!plan.diagnostics.empty()) {
        sort_diagnostics(plan);
        plan.status = RuntimeWorldEntityLifecycleStatus::invalid_request;
        return plan;
    }

    append_plan_rows(plan, request);
    plan.status = plan.entity_rows.empty() && plan.lifecycle_rows.empty()
                      ? RuntimeWorldEntityLifecycleStatus::no_entities
                      : RuntimeWorldEntityLifecycleStatus::ready;
    return plan;
}

} // namespace mirakana::runtime
