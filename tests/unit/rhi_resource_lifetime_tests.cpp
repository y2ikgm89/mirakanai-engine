// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/rhi/resource_lifetime.hpp"

MK_TEST("rhi resource lifetime registry assigns generations and debug names") {
    mirakana::rhi::RhiResourceLifetimeRegistry registry;

    const auto registration = registry.register_resource(mirakana::rhi::RhiResourceRegistrationDesc{
        .kind = mirakana::rhi::RhiResourceKind::texture,
        .owner = "runtime-scene",
        .debug_name = "player-albedo",
    });
    MK_REQUIRE(registration.succeeded());
    const auto texture = registration.handle;

    MK_REQUIRE(texture.id.value == 1);
    MK_REQUIRE(texture.generation == 1);
    MK_REQUIRE(registry.records().size() == 1);
    MK_REQUIRE(registry.records()[0].debug_name == "player-albedo");

    const auto rename_result = registry.set_debug_name(texture, "player-albedo-streamed");
    MK_REQUIRE(rename_result.succeeded());
    MK_REQUIRE(registry.records()[0].debug_name == "player-albedo-streamed");
}

MK_TEST("rhi resource lifetime registry defers release until retire frame") {
    mirakana::rhi::RhiResourceLifetimeRegistry registry;
    const auto registration = registry.register_resource(mirakana::rhi::RhiResourceRegistrationDesc{
        .kind = mirakana::rhi::RhiResourceKind::buffer,
        .owner = "upload",
        .debug_name = "mesh-vertices",
    });
    MK_REQUIRE(registration.succeeded());
    const auto buffer = registration.handle;

    const auto release = registry.release_resource_deferred(buffer, 12);
    MK_REQUIRE(release.succeeded());
    MK_REQUIRE(!registry.is_live(buffer));
    MK_REQUIRE(registry.records().size() == 1);

    MK_REQUIRE(registry.retire_released_resources(11) == 0);
    MK_REQUIRE(registry.records().size() == 1);
    MK_REQUIRE(registry.retire_released_resources(12) == 1);
    MK_REQUIRE(registry.records().empty());
}

MK_TEST("rhi resource lifetime registry diagnoses duplicate and stale releases") {
    mirakana::rhi::RhiResourceLifetimeRegistry registry;
    const auto registration = registry.register_resource(mirakana::rhi::RhiResourceRegistrationDesc{
        .kind = mirakana::rhi::RhiResourceKind::texture, .owner = "renderer", .debug_name = "shadow-map"});
    MK_REQUIRE(registration.succeeded());
    const auto texture = registration.handle;

    MK_REQUIRE(registry.release_resource_deferred(texture, 4).succeeded());

    const auto duplicate = registry.release_resource_deferred(texture, 5);
    MK_REQUIRE(!duplicate.succeeded());
    MK_REQUIRE(duplicate.diagnostics[0].code == mirakana::rhi::RhiResourceLifetimeDiagnosticCode::duplicate_release);

    const auto stale = registry.set_debug_name(
        mirakana::rhi::RhiResourceHandle{.id = texture.id, .generation = texture.generation + 1U}, "stale-name");
    MK_REQUIRE(!stale.succeeded());
    MK_REQUIRE(stale.diagnostics[0].code == mirakana::rhi::RhiResourceLifetimeDiagnosticCode::stale_generation);
}

MK_TEST("rhi resource lifetime registry rejects invalid registrations and debug names") {
    mirakana::rhi::RhiResourceLifetimeRegistry registry;

    const auto invalid = registry.register_resource(mirakana::rhi::RhiResourceRegistrationDesc{
        .kind = mirakana::rhi::RhiResourceKind::unknown, .owner = "renderer", .debug_name = "shadow-map"});
    MK_REQUIRE(!invalid.succeeded());
    MK_REQUIRE(invalid.handle.id.value == 0);
    MK_REQUIRE(invalid.handle.generation == 0);
    MK_REQUIRE(invalid.diagnostics[0].code == mirakana::rhi::RhiResourceLifetimeDiagnosticCode::invalid_registration);
    MK_REQUIRE(registry.records().empty());
    MK_REQUIRE(registry.events().size() == 1);
    MK_REQUIRE(registry.events()[0].kind == mirakana::rhi::RhiResourceLifetimeEventKind::invalid_registration);

    const auto registration = registry.register_resource(mirakana::rhi::RhiResourceRegistrationDesc{
        .kind = mirakana::rhi::RhiResourceKind::buffer, .owner = "renderer", .debug_name = "constant-buffer"});
    MK_REQUIRE(registration.succeeded());
    const auto buffer = registration.handle;
    const auto invalid_name = registry.set_debug_name(buffer, "bad\nname");
    MK_REQUIRE(!invalid_name.succeeded());
    MK_REQUIRE(invalid_name.diagnostics[0].code ==
               mirakana::rhi::RhiResourceLifetimeDiagnosticCode::invalid_debug_name);
    MK_REQUIRE(registry.records()[0].debug_name == "constant-buffer");
}

MK_TEST("rhi resource lifetime registry rejects ops on handles retired from the registry") {
    mirakana::rhi::RhiResourceLifetimeRegistry registry;
    const auto registration = registry.register_resource(mirakana::rhi::RhiResourceRegistrationDesc{
        .kind = mirakana::rhi::RhiResourceKind::buffer,
        .owner = "renderer",
        .debug_name = "mesh-vb",
    });
    MK_REQUIRE(registration.succeeded());
    const auto handle = registration.handle;

    MK_REQUIRE(registry.release_resource_deferred(handle, 3).succeeded());
    MK_REQUIRE(registry.retire_released_resources(3) == 1);
    MK_REQUIRE(registry.records().empty());

    const auto rename_after_retire = registry.set_debug_name(handle, "too-late");
    MK_REQUIRE(!rename_after_retire.succeeded());
    MK_REQUIRE(rename_after_retire.diagnostics[0].code ==
               mirakana::rhi::RhiResourceLifetimeDiagnosticCode::invalid_resource);

    const auto release_after_retire = registry.release_resource_deferred(handle, 4);
    MK_REQUIRE(!release_after_retire.succeeded());
    MK_REQUIRE(release_after_retire.diagnostics[0].code ==
               mirakana::rhi::RhiResourceLifetimeDiagnosticCode::invalid_resource);
}

MK_TEST("rhi resource lifetime registry records marker-style event rows") {
    mirakana::rhi::RhiResourceLifetimeRegistry registry;
    const auto registration = registry.register_resource(mirakana::rhi::RhiResourceRegistrationDesc{
        .kind = mirakana::rhi::RhiResourceKind::graphics_pipeline, .owner = "scene", .debug_name = "lit-pipeline"});
    MK_REQUIRE(registration.succeeded());
    const auto pipeline = registration.handle;
    MK_REQUIRE(registry.set_debug_name(pipeline, "lit-pipeline-v2").succeeded());
    MK_REQUIRE(registry.release_resource_deferred(pipeline, 8).succeeded());
    MK_REQUIRE(registry.retire_released_resources(8) == 1);

    MK_REQUIRE(registry.events().size() == 4);
    MK_REQUIRE(registry.events()[0].kind == mirakana::rhi::RhiResourceLifetimeEventKind::register_resource);
    MK_REQUIRE(registry.events()[1].kind == mirakana::rhi::RhiResourceLifetimeEventKind::rename);
    MK_REQUIRE(registry.events()[2].kind == mirakana::rhi::RhiResourceLifetimeEventKind::defer_release);
    MK_REQUIRE(registry.events()[3].kind == mirakana::rhi::RhiResourceLifetimeEventKind::retire);
}

int main() {
    return mirakana::test::run_all();
}
