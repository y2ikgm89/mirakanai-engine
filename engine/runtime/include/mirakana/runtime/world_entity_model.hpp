// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/runtime/session_services.hpp"
#include "mirakana/runtime/world_region_streaming.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::runtime {

struct RuntimeWorldEntityId {
    std::string value;
};

struct RuntimeWorldRegionId {
    std::string value;
};

struct RuntimeWorldComponentSchemaId {
    std::string value;
};

enum class RuntimeWorldEntityLifecycleAction : std::uint8_t {
    spawn_entity = 0,
    despawn_entity,
    move_entity_region,
};

enum class RuntimeWorldEntityLifecycleStatus : std::uint8_t {
    ready = 0,
    no_entities,
    invalid_request,
};

enum class RuntimeWorldEntityDiagnosticCode : std::uint8_t {
    missing_world_id = 0,
    missing_region_id,
    duplicate_region_id,
    missing_entity_id,
    duplicate_entity_id,
    unknown_entity_region,
    missing_component_schema_id,
    duplicate_component_schema_id,
    missing_component_entity,
    unknown_component_entity,
    missing_component_schema,
    unknown_component_schema,
    duplicate_component_for_entity,
    spawn_duplicate_entity,
    despawn_unknown_entity,
    move_unknown_entity,
    move_unknown_region,
    persistence_bridge_not_ready,
    streaming_bridge_not_ready,
    persistence_bridge_world_mismatch,
    persistence_bridge_unknown_entity,
    persistence_bridge_unknown_region,
    streaming_bridge_unknown_region,
    streaming_bridge_diagnostics_present,
};

struct RuntimeWorldRegionRow {
    RuntimeWorldRegionId region_id;
    std::uint32_t source_index{0U};
};

struct RuntimeWorldEntityRow {
    RuntimeWorldEntityId entity_id;
    RuntimeWorldRegionId region_id;
    std::string archetype_id;
    bool active{true};
    std::uint64_t generation{0U};
    std::uint32_t source_index{0U};
};

struct RuntimeWorldComponentSchemaRow {
    RuntimeWorldComponentSchemaId schema_id;
    std::uint32_t version{1U};
    std::uint32_t source_index{0U};
};

struct RuntimeWorldComponentRow {
    RuntimeWorldEntityId entity_id;
    RuntimeWorldComponentSchemaId schema_id;
    std::string state_hash;
    std::uint32_t source_index{0U};
};

struct RuntimeWorldEntityLifecycleIntent {
    RuntimeWorldEntityLifecycleAction action{RuntimeWorldEntityLifecycleAction::spawn_entity};
    RuntimeWorldEntityId entity_id;
    RuntimeWorldRegionId target_region_id;
    std::string archetype_id;
    std::uint32_t source_index{0U};
};

struct RuntimeWorldEntityPersistenceBridgeDesc {
    bool required{false};
    RuntimeSimulationPersistencePlan plan;
};

struct RuntimeWorldEntityStreamingBridgeDesc {
    bool required{false};
    RuntimeWorldRegionStreamingPlan plan;
};

struct RuntimeWorldEntityLifecycleRequest {
    std::string world_id;
    std::vector<RuntimeWorldRegionRow> regions;
    std::vector<RuntimeWorldEntityRow> entities;
    std::vector<RuntimeWorldComponentSchemaRow> component_schemas;
    std::vector<RuntimeWorldComponentRow> components;
    std::vector<RuntimeWorldEntityLifecycleIntent> lifecycle_intents;
    RuntimeWorldEntityPersistenceBridgeDesc persistence_bridge;
    RuntimeWorldEntityStreamingBridgeDesc streaming_bridge;
};

struct RuntimeWorldEntityDiagnostic {
    RuntimeWorldEntityDiagnosticCode code{RuntimeWorldEntityDiagnosticCode::missing_world_id};
    std::string world_id;
    RuntimeWorldEntityId entity_id;
    RuntimeWorldRegionId region_id;
    RuntimeWorldComponentSchemaId schema_id;
    std::string message;
    std::uint32_t source_index{0U};
};

struct RuntimeWorldEntityRegionOwnershipRow {
    std::string world_id;
    RuntimeWorldEntityId entity_id;
    RuntimeWorldRegionId region_id;
    bool active{true};
    std::uint64_t generation{0U};
    std::uint32_t source_index{0U};
};

struct RuntimeWorldEntityLifecycleRow {
    std::string world_id;
    RuntimeWorldEntityLifecycleAction action{RuntimeWorldEntityLifecycleAction::spawn_entity};
    RuntimeWorldEntityId entity_id;
    RuntimeWorldRegionId target_region_id;
    std::string archetype_id;
    std::uint32_t source_index{0U};
};

struct RuntimeWorldEntityPersistenceRow {
    std::string world_id;
    std::uint64_t world_tick{0U};
    RuntimeWorldEntityId entity_id;
    RuntimeWorldRegionId region_id;
    std::string entity_type;
    std::string state_hash;
};

struct RuntimeWorldEntityStreamingRegionRow {
    RuntimeWorldRegionStreamingActionKind action{RuntimeWorldRegionStreamingActionKind::keep_resident};
    RuntimeWorldRegionId region_id;
    std::uint64_t estimated_resident_bytes{0U};
    std::size_t estimated_asset_records{0U};
};

struct RuntimeWorldEntityLifecyclePlan {
    RuntimeWorldEntityLifecycleStatus status{RuntimeWorldEntityLifecycleStatus::invalid_request};
    std::vector<RuntimeWorldEntityDiagnostic> diagnostics;
    std::vector<RuntimeWorldRegionRow> region_rows;
    std::vector<RuntimeWorldEntityRow> entity_rows;
    std::vector<RuntimeWorldComponentSchemaRow> component_schema_rows;
    std::vector<RuntimeWorldComponentRow> component_rows;
    std::vector<RuntimeWorldEntityRegionOwnershipRow> region_ownership_rows;
    std::vector<RuntimeWorldEntityLifecycleRow> lifecycle_rows;
    std::vector<RuntimeWorldEntityPersistenceRow> persistence_rows;
    std::vector<RuntimeWorldEntityStreamingRegionRow> streaming_region_rows;
    std::size_t validated_entity_count{0U};
    std::size_t validated_component_count{0U};
    std::size_t region_ownership_count{0U};
    std::size_t spawn_count{0U};
    std::size_t despawn_count{0U};
    std::size_t move_count{0U};
    std::size_t rejected_duplicate_entity_count{0U};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Reviews stable runtime world/entity/component identity rows plus lifecycle intent rows.
/// This value-only helper does not mutate scenes, execute lifecycle actions, load packages, perform persistence IO,
/// stream regions, upload renderer resources, create threads, or expose native handles.
[[nodiscard]] RuntimeWorldEntityLifecyclePlan
plan_runtime_world_entity_lifecycle(const RuntimeWorldEntityLifecycleRequest& request);

} // namespace mirakana::runtime
