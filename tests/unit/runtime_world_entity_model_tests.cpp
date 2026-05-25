// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/world_entity_model.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace {

[[nodiscard]] mirakana::runtime::RuntimeWorldRegionRow make_region(std::string region_id, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeWorldRegionRow{
        .region_id = mirakana::runtime::RuntimeWorldRegionId{.value = std::move(region_id)},
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeWorldEntityRow
make_entity(std::string entity_id, std::string region_id, std::string archetype_id, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeWorldEntityRow{
        .entity_id = mirakana::runtime::RuntimeWorldEntityId{.value = std::move(entity_id)},
        .region_id = mirakana::runtime::RuntimeWorldRegionId{.value = std::move(region_id)},
        .archetype_id = std::move(archetype_id),
        .active = true,
        .generation = 7U,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeWorldComponentSchemaRow make_schema(std::string schema_id,
                                                                            std::uint32_t source_index) {
    return mirakana::runtime::RuntimeWorldComponentSchemaRow{
        .schema_id = mirakana::runtime::RuntimeWorldComponentSchemaId{.value = std::move(schema_id)},
        .version = 1U,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeWorldComponentRow
make_component(std::string entity_id, std::string schema_id, std::string state_hash, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeWorldComponentRow{
        .entity_id = mirakana::runtime::RuntimeWorldEntityId{.value = std::move(entity_id)},
        .schema_id = mirakana::runtime::RuntimeWorldComponentSchemaId{.value = std::move(schema_id)},
        .state_hash = std::move(state_hash),
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeWorldEntityLifecycleIntent
make_intent(mirakana::runtime::RuntimeWorldEntityLifecycleAction action, std::string entity_id,
            std::string target_region_id, std::string archetype_id, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeWorldEntityLifecycleIntent{
        .action = action,
        .entity_id = mirakana::runtime::RuntimeWorldEntityId{.value = std::move(entity_id)},
        .target_region_id = mirakana::runtime::RuntimeWorldRegionId{.value = std::move(target_region_id)},
        .archetype_id = std::move(archetype_id),
        .source_index = source_index,
    };
}

[[nodiscard]] std::size_t diagnostic_count(const mirakana::runtime::RuntimeWorldEntityLifecyclePlan& plan,
                                           mirakana::runtime::RuntimeWorldEntityDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

} // namespace

MK_TEST("runtime world entity model validates stable identity and region ownership rows") {
    using Action = mirakana::runtime::RuntimeWorldEntityLifecycleAction;
    using Status = mirakana::runtime::RuntimeWorldEntityLifecycleStatus;

    const auto request = mirakana::runtime::RuntimeWorldEntityLifecycleRequest{
        .world_id = "campaign_world",
        .regions = {make_region("field", 2U), make_region("town", 1U)},
        .entities =
            {
                make_entity("player", "field", "hero", 1U),
                make_entity("npc.vendor", "town", "vendor", 2U),
            },
        .component_schemas = {make_schema("inventory", 2U), make_schema("transform", 1U)},
        .components =
            {
                make_component("npc.vendor", "transform", "hash.vendor.transform", 3U),
                make_component("player", "inventory", "hash.player.inventory", 2U),
                make_component("player", "transform", "hash.player.transform", 1U),
            },
        .lifecycle_intents =
            {
                make_intent(Action::spawn_entity, "quest.item", "town", "pickup", 1U),
                make_intent(Action::move_entity_region, "player", "town", {}, 2U),
                make_intent(Action::despawn_entity, "npc.vendor", {}, {}, 3U),
            },
    };

    const auto plan = mirakana::runtime::plan_runtime_world_entity_lifecycle(request);

    MK_REQUIRE(plan.status == Status::ready);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.region_rows.size() == 2U);
    MK_REQUIRE(plan.entity_rows.size() == 2U);
    MK_REQUIRE(plan.component_rows.size() == 3U);
    MK_REQUIRE(plan.region_ownership_rows.size() == 2U);
    MK_REQUIRE(plan.lifecycle_rows.size() == 3U);
    MK_REQUIRE(plan.validated_entity_count == 2U);
    MK_REQUIRE(plan.validated_component_count == 3U);
    MK_REQUIRE(plan.region_ownership_count == 2U);
    MK_REQUIRE(plan.spawn_count == 1U);
    MK_REQUIRE(plan.move_count == 1U);
    MK_REQUIRE(plan.despawn_count == 1U);
    MK_REQUIRE(plan.rejected_duplicate_entity_count == 0U);
    MK_REQUIRE(plan.entity_rows[0].entity_id.value == "npc.vendor");
    MK_REQUIRE(plan.entity_rows[1].entity_id.value == "player");
    MK_REQUIRE(plan.component_rows[0].entity_id.value == "npc.vendor");
    MK_REQUIRE(plan.component_rows[0].schema_id.value == "transform");
    MK_REQUIRE(plan.component_rows[1].entity_id.value == "player");
    MK_REQUIRE(plan.component_rows[1].schema_id.value == "inventory");
    MK_REQUIRE(plan.component_rows[2].schema_id.value == "transform");
    MK_REQUIRE(plan.region_ownership_rows[0].entity_id.value == "npc.vendor");
    MK_REQUIRE(plan.region_ownership_rows[1].entity_id.value == "player");
}

MK_TEST("runtime world entity model rejects duplicate and unresolved references") {
    using Code = mirakana::runtime::RuntimeWorldEntityDiagnosticCode;
    using Status = mirakana::runtime::RuntimeWorldEntityLifecycleStatus;

    const auto request = mirakana::runtime::RuntimeWorldEntityLifecycleRequest{
        .world_id = "",
        .regions = {make_region("field", 1U), make_region("field", 2U), make_region("", 3U)},
        .entities =
            {
                make_entity("player", "field", "hero", 1U),
                make_entity("player", "field", "hero.copy", 2U),
                make_entity("orphan", "missing_region", "ghost", 3U),
                make_entity("", "field", "broken", 4U),
            },
        .component_schemas = {make_schema("transform", 1U), make_schema("transform", 2U), make_schema("", 3U)},
        .components =
            {
                make_component("player", "transform", "hash.transform", 1U),
                make_component("player", "transform", "hash.duplicate", 2U),
                make_component("missing_entity", "transform", "hash.missing.entity", 3U),
                make_component("player", "missing_schema", "hash.missing.schema", 4U),
                make_component("", "transform", "hash.empty.entity", 5U),
                make_component("player", "", "hash.empty.schema", 6U),
            },
    };

    const auto plan = mirakana::runtime::plan_runtime_world_entity_lifecycle(request);

    MK_REQUIRE(plan.status == Status::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.entity_rows.empty());
    MK_REQUIRE(plan.component_rows.empty());
    MK_REQUIRE(plan.rejected_duplicate_entity_count == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::missing_world_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::missing_region_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::duplicate_region_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::missing_entity_id) == 2U);
    MK_REQUIRE(diagnostic_count(plan, Code::duplicate_entity_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::unknown_entity_region) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::missing_component_schema_id) == 2U);
    MK_REQUIRE(diagnostic_count(plan, Code::duplicate_component_schema_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::unknown_component_entity) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::unknown_component_schema) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::duplicate_component_for_entity) == 1U);
}

MK_TEST("runtime world entity model reviews persistence and streaming bridge rows") {
    using PersistStatus = mirakana::runtime::RuntimeSimulationPersistenceStatus;
    using RegionAction = mirakana::runtime::RuntimeWorldRegionStreamingActionKind;
    using RegionStatus = mirakana::runtime::RuntimeWorldRegionStreamingPlanStatus;
    using Status = mirakana::runtime::RuntimeWorldEntityLifecycleStatus;

    const auto persistence_plan = mirakana::runtime::RuntimeSimulationPersistencePlan{
        .status = PersistStatus::ready,
        .world_id = "campaign_world",
        .world_tick = 9001U,
        .entity_rows =
            {
                mirakana::runtime::RuntimeSimulationPersistentEntityRow{
                    .entity_id = "player",
                    .entity_type = "hero",
                    .region_id = "field",
                    .state_hash = "hash.player",
                },
                mirakana::runtime::RuntimeSimulationPersistentEntityRow{
                    .entity_id = "npc.vendor",
                    .entity_type = "vendor",
                    .region_id = "town",
                    .state_hash = "hash.vendor",
                },
            },
    };
    const auto streaming_plan = mirakana::runtime::RuntimeWorldRegionStreamingPlan{
        .status = RegionStatus::planned,
        .rows =
            {
                mirakana::runtime::RuntimeWorldRegionStreamingPlanRow{
                    .action = RegionAction::load_region,
                    .region_id = "town",
                    .estimated_resident_bytes = 8192U,
                    .estimated_asset_records = 16U,
                },
                mirakana::runtime::RuntimeWorldRegionStreamingPlanRow{
                    .action = RegionAction::load_region,
                    .region_id = "field",
                    .estimated_resident_bytes = 4096U,
                    .estimated_asset_records = 8U,
                },
            },
    };

    const auto request = mirakana::runtime::RuntimeWorldEntityLifecycleRequest{
        .world_id = "campaign_world",
        .regions = {make_region("field", 1U), make_region("town", 2U)},
        .entities =
            {
                make_entity("player", "field", "hero", 1U),
                make_entity("npc.vendor", "town", "vendor", 2U),
            },
        .component_schemas = {make_schema("transform", 1U)},
        .components =
            {
                make_component("player", "transform", "hash.transform", 1U),
                make_component("npc.vendor", "transform", "hash.vendor.transform", 2U),
            },
        .persistence_bridge =
            mirakana::runtime::RuntimeWorldEntityPersistenceBridgeDesc{
                .required = true,
                .plan = persistence_plan,
            },
        .streaming_bridge =
            mirakana::runtime::RuntimeWorldEntityStreamingBridgeDesc{
                .required = true,
                .plan = streaming_plan,
            },
    };

    const auto plan = mirakana::runtime::plan_runtime_world_entity_lifecycle(request);

    MK_REQUIRE(plan.status == Status::ready);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.persistence_rows.size() == 2U);
    MK_REQUIRE(plan.persistence_rows[0].entity_id.value == "npc.vendor");
    MK_REQUIRE(plan.persistence_rows[1].entity_id.value == "player");
    MK_REQUIRE(plan.persistence_rows[1].world_tick == 9001U);
    MK_REQUIRE(plan.streaming_region_rows.size() == 2U);
    MK_REQUIRE(plan.streaming_region_rows[0].region_id.value == "field");
    MK_REQUIRE(plan.streaming_region_rows[0].estimated_resident_bytes == 4096U);
    MK_REQUIRE(plan.streaming_region_rows[1].region_id.value == "town");
}

MK_TEST("runtime world entity model rejects non-ready persistence and streaming bridges") {
    using Code = mirakana::runtime::RuntimeWorldEntityDiagnosticCode;
    using PersistStatus = mirakana::runtime::RuntimeSimulationPersistenceStatus;
    using RegionStatus = mirakana::runtime::RuntimeWorldRegionStreamingPlanStatus;
    using Status = mirakana::runtime::RuntimeWorldEntityLifecycleStatus;

    const auto request = mirakana::runtime::RuntimeWorldEntityLifecycleRequest{
        .world_id = "campaign_world",
        .regions = {make_region("field", 1U)},
        .entities = {make_entity("player", "field", "hero", 1U)},
        .component_schemas = {make_schema("transform", 1U)},
        .components = {make_component("player", "transform", "hash.transform", 1U)},
        .persistence_bridge =
            mirakana::runtime::RuntimeWorldEntityPersistenceBridgeDesc{
                .required = true,
                .plan =
                    mirakana::runtime::RuntimeSimulationPersistencePlan{
                        .status = PersistStatus::blocked,
                        .diagnostics =
                            {
                                mirakana::runtime::RuntimeSimulationPersistenceDiagnostic{},
                            },
                    },
            },
        .streaming_bridge =
            mirakana::runtime::RuntimeWorldEntityStreamingBridgeDesc{
                .required = true,
                .plan =
                    mirakana::runtime::RuntimeWorldRegionStreamingPlan{
                        .status = RegionStatus::invalid_request,
                    },
            },
    };

    const auto plan = mirakana::runtime::plan_runtime_world_entity_lifecycle(request);

    MK_REQUIRE(plan.status == Status::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.region_ownership_rows.empty());
    MK_REQUIRE(diagnostic_count(plan, Code::persistence_bridge_not_ready) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::streaming_bridge_not_ready) == 1U);
}

MK_TEST("runtime world entity model rejects mismatched bridge rows") {
    using Code = mirakana::runtime::RuntimeWorldEntityDiagnosticCode;
    using PersistStatus = mirakana::runtime::RuntimeSimulationPersistenceStatus;
    using RegionAction = mirakana::runtime::RuntimeWorldRegionStreamingActionKind;
    using RegionCode = mirakana::runtime::RuntimeWorldRegionStreamingDiagnosticCode;
    using RegionStatus = mirakana::runtime::RuntimeWorldRegionStreamingPlanStatus;
    using Status = mirakana::runtime::RuntimeWorldEntityLifecycleStatus;

    const auto
        request =
            mirakana::runtime::RuntimeWorldEntityLifecycleRequest{
                .world_id = "campaign_world",
                .regions = {make_region("field", 1U)},
                .entities = {make_entity("player", "field", "hero", 1U)},
                .component_schemas = {make_schema("transform", 1U)},
                .components = {make_component("player", "transform", "hash.transform", 1U)},
                .persistence_bridge =
                    mirakana::runtime::RuntimeWorldEntityPersistenceBridgeDesc{
                        .required = true,
                        .plan =
                            mirakana::runtime::RuntimeSimulationPersistencePlan{
                                .status = PersistStatus::ready,
                                .world_id = "other_world",
                                .world_tick = 42U,
                                .entity_rows =
                                    {
                                        mirakana::runtime::RuntimeSimulationPersistentEntityRow{
                                            .entity_id = "ghost",
                                            .entity_type = "npc",
                                            .region_id = "missing_region",
                                            .state_hash = "hash.ghost",
                                        },
                                    },
                            },
                    },
                .streaming_bridge =
                    mirakana::runtime::RuntimeWorldEntityStreamingBridgeDesc{
                        .required = true,
                        .plan =
                            mirakana::runtime::RuntimeWorldRegionStreamingPlan{
                                .status = RegionStatus::planned,
                                .diagnostics =
                                    {
                                        mirakana::runtime::RuntimeWorldRegionStreamingDiagnostic{
                                            .code = RegionCode::missing_desired_region,
                                            .region_id = "missing_region",
                                            .message = "synthetic bridge diagnostic",
                                        },
                                    },
                                .rows =
                                    {
                                        mirakana::runtime::RuntimeWorldRegionStreamingPlanRow{
                                            .action = RegionAction::load_region,
                                            .region_id = "missing_region",
                                        },
                                    },
                            },
                    },
            };

    const auto plan = mirakana::runtime::plan_runtime_world_entity_lifecycle(request);

    MK_REQUIRE(plan.status == Status::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.persistence_rows.empty());
    MK_REQUIRE(plan.streaming_region_rows.empty());
    MK_REQUIRE(diagnostic_count(plan, Code::persistence_bridge_world_mismatch) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::persistence_bridge_unknown_entity) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::persistence_bridge_unknown_region) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::streaming_bridge_diagnostics_present) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::streaming_bridge_unknown_region) == 1U);
}

int main() {
    return mirakana::test::run_all();
}
