// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/core/application.hpp"
#include "mirakana/core/log.hpp"
#include "mirakana/core/version.hpp"
#include "mirakana/tools/asset_import_adapters.hpp"
#include "mirakana/ui/ui.hpp"

#if defined(MIRAKANAI_INSTALLED_CONSUMER_HAS_PHYSICS_JOLT)
#include "mirakana/physics/jolt/jolt_physics_adapter.hpp"
#endif

#if defined(MIRAKANAI_INSTALLED_CONSUMER_HAS_NETWORK_ENET)
#include "mirakana/runtime/network/enet/enet_network_transport_adapter.hpp"
#endif

#include <iostream>

class InstalledConsumerApp final : public mirakana::GameApp {
  public:
    bool on_update(mirakana::EngineContext&, double) override {
        ++frames_;
        return frames_ < 1;
    }

  private:
    int frames_{0};
};

int main() {
    mirakana::RingBufferLogger logger(8);
    mirakana::Registry registry;
    mirakana::HeadlessRunner runner(logger, registry);
    InstalledConsumerApp app;

    mirakana::ui::UiDocument document;
    const bool added_root = document.try_add_element(mirakana::ui::ElementDesc{
        mirakana::ui::ElementId{"root"},
        mirakana::ui::ElementId{},
        mirakana::ui::SemanticRole::root,
        mirakana::ui::Rect{0.0F, 0.0F, 320.0F, 180.0F},
    });

    const auto result = runner.run(app, mirakana::RunConfig{1, 1.0 / 60.0});
    constexpr auto version = mirakana::engine_version();
#if defined(MIRAKANAI_INSTALLED_CONSUMER_HAS_PHYSICS_JOLT)
    const auto physics_capabilities = mirakana::jolt_physics_3d_adapter_capabilities();
    const bool physics_jolt_ready = physics_capabilities.available && !physics_capabilities.exposes_native_handles;
#else
    const bool physics_jolt_ready = true;
#endif
#if defined(MIRAKANAI_INSTALLED_CONSUMER_HAS_NETWORK_ENET)
    const auto network_adapter = mirakana::runtime::make_enet_network_transport_adapter();
    const auto network_capabilities = network_adapter->capabilities();
    const bool network_enet_ready = network_capabilities.available && network_capabilities.supports_loopback_exchange &&
                                    !network_capabilities.exposes_native_handles;
#else
    const bool network_enet_ready = true;
#endif
    std::cout << version.name << " installed-consumer frames=" << result.frames_run
              << " ui_elements=" << document.size()
              << " external_importers=" << (mirakana::external_asset_importers_available() ? "available" : "disabled")
#if defined(MIRAKANAI_INSTALLED_CONSUMER_HAS_PHYSICS_JOLT)
              << " physics_jolt=" << (physics_jolt_ready ? "available" : "invalid")
#else
              << " physics_jolt=disabled"
#endif
#if defined(MIRAKANAI_INSTALLED_CONSUMER_HAS_NETWORK_ENET)
              << " network_enet=" << (network_enet_ready ? "available" : "invalid")
#else
              << " network_enet=disabled"
#endif
              << '\n';

    return added_root && result.frames_run == 1 && document.size() == 1 && physics_jolt_ready && network_enet_ready ? 0
                                                                                                                    : 1;
}
