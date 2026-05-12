// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/tools/shader_toolchain.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace mirakana::editor {

struct ShaderToolDiscoveryItem {
    ShaderToolKind kind{ShaderToolKind::unknown};
    std::string label;
    std::string executable_path;
    std::string version;
};

class ShaderToolDiscoveryState {
  public:
    void refresh_from(std::vector<ShaderToolDescriptor> tools);
    void clear() noexcept;

    [[nodiscard]] const std::vector<ShaderToolDiscoveryItem>& items() const noexcept;
    [[nodiscard]] std::size_t item_count() const noexcept;
    [[nodiscard]] const ShaderToolDiscoveryItem* find_first(ShaderToolKind kind) const noexcept;
    [[nodiscard]] const ShaderToolchainReadiness& readiness() const noexcept;

  private:
    std::vector<ShaderToolDiscoveryItem> items_;
    ShaderToolchainReadiness readiness_;
};

} // namespace mirakana::editor
