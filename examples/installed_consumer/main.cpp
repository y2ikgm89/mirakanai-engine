// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/core/application.hpp"
#include "mirakana/core/log.hpp"
#include "mirakana/core/version.hpp"
#include "mirakana/tools/asset_import_adapters.hpp"
#include "mirakana/ui/ui.hpp"

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
    std::cout << version.name << " installed-consumer frames=" << result.frames_run
              << " ui_elements=" << document.size()
              << " external_importers=" << (mirakana::external_asset_importers_available() ? "available" : "disabled")
              << '\n';

    return added_root && result.frames_run == 1 && document.size() == 1 ? 0 : 1;
}
