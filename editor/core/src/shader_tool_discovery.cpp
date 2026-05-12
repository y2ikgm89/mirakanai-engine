// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/shader_tool_discovery.hpp"

#include <algorithm>
#include <utility>

namespace mirakana::editor {
namespace {

[[nodiscard]] int tool_order(ShaderToolKind kind) noexcept {
    switch (kind) {
    case ShaderToolKind::dxc:
        return 0;
    case ShaderToolKind::spirv_val:
        return 1;
    case ShaderToolKind::metal:
        return 2;
    case ShaderToolKind::metallib:
        return 3;
    case ShaderToolKind::unknown:
        break;
    }
    return 4;
}

} // namespace

void ShaderToolDiscoveryState::refresh_from(std::vector<ShaderToolDescriptor> tools) {
    readiness_ = evaluate_shader_toolchain_readiness(tools);

    std::ranges::sort(tools, [](const ShaderToolDescriptor& lhs, const ShaderToolDescriptor& rhs) {
        if (tool_order(lhs.kind) != tool_order(rhs.kind)) {
            return tool_order(lhs.kind) < tool_order(rhs.kind);
        }
        return lhs.executable_path < rhs.executable_path;
    });

    items_.clear();
    items_.reserve(tools.size());
    for (auto& tool : tools) {
        if (tool.kind == ShaderToolKind::unknown || tool.executable_path.empty()) {
            continue;
        }
        items_.push_back(ShaderToolDiscoveryItem{
            .kind = tool.kind,
            .label = std::string(shader_tool_kind_name(tool.kind)),
            .executable_path = std::move(tool.executable_path),
            .version = std::move(tool.version),
        });
    }
}

void ShaderToolDiscoveryState::clear() noexcept {
    items_.clear();
    readiness_ = ShaderToolchainReadiness{};
}

const std::vector<ShaderToolDiscoveryItem>& ShaderToolDiscoveryState::items() const noexcept {
    return items_;
}

std::size_t ShaderToolDiscoveryState::item_count() const noexcept {
    return items_.size();
}

const ShaderToolDiscoveryItem* ShaderToolDiscoveryState::find_first(ShaderToolKind kind) const noexcept {
    const auto it =
        std::ranges::find_if(items_, [kind](const ShaderToolDiscoveryItem& item) { return item.kind == kind; });
    return it == items_.end() ? nullptr : &(*it);
}

const ShaderToolchainReadiness& ShaderToolDiscoveryState::readiness() const noexcept {
    return readiness_;
}

} // namespace mirakana::editor
