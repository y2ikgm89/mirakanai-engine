// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/viewport_shader_artifacts.hpp"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace mirakana::editor {
namespace {

[[nodiscard]] int stage_sort_key(ShaderSourceStage stage) noexcept {
    switch (stage) {
    case ShaderSourceStage::vertex:
        return 0;
    case ShaderSourceStage::fragment:
        return 1;
    case ShaderSourceStage::compute:
        return 2;
    case ShaderSourceStage::unknown:
        break;
    }
    return 3;
}

[[nodiscard]] int target_sort_key(ShaderCompileTarget target) noexcept {
    switch (target) {
    case ShaderCompileTarget::d3d12_dxil:
        return 0;
    case ShaderCompileTarget::vulkan_spirv:
        return 1;
    case ShaderCompileTarget::metal_ir:
        return 2;
    case ShaderCompileTarget::metal_library:
        return 3;
    case ShaderCompileTarget::unknown:
        break;
    }
    return 4;
}

[[nodiscard]] bool is_viewport_graphics_stage(ShaderSourceStage stage) noexcept {
    return stage == ShaderSourceStage::vertex || stage == ShaderSourceStage::fragment;
}

[[nodiscard]] bool is_viewport_graphics_target(ShaderCompileTarget target) noexcept {
    return target == ShaderCompileTarget::d3d12_dxil || target == ShaderCompileTarget::vulkan_spirv;
}

[[nodiscard]] ViewportShaderArtifactItem make_item(const IFileSystem& filesystem, const ShaderCompileRequest& request) {
    ViewportShaderArtifactItem item{
        .shader = request.source.id,
        .stage = request.source.stage,
        .target = request.target,
        .output_path = request.output_path,
        .profile = request.profile,
        .entry_point = request.source.entry_point,
        .status = ViewportShaderArtifactStatus::missing,
        .byte_size = 0,
        .diagnostic = {},
    };

    if (!is_viewport_graphics_stage(item.stage)) {
        item.status = ViewportShaderArtifactStatus::unsupported_stage;
        item.diagnostic = "viewport requires vertex and fragment shader stages";
        return item;
    }

    if (!is_viewport_graphics_target(item.target)) {
        item.status = ViewportShaderArtifactStatus::unsupported_target;
        item.diagnostic = "viewport requires d3d12_dxil or vulkan_spirv artifacts";
        return item;
    }

    if (!filesystem.exists(item.output_path)) {
        item.status = ViewportShaderArtifactStatus::missing;
        item.diagnostic = "compiled shader artifact is missing";
        return item;
    }

    std::string bytecode;
    try {
        bytecode = filesystem.read_text(item.output_path);
    } catch (const std::exception& error) {
        item.status = ViewportShaderArtifactStatus::missing;
        item.diagnostic = error.what();
        return item;
    }

    if (bytecode.empty()) {
        item.status = ViewportShaderArtifactStatus::empty;
        item.diagnostic = "compiled shader artifact is empty";
        return item;
    }

    item.status = ViewportShaderArtifactStatus::ready;
    item.byte_size = bytecode.size();
    item.diagnostic = "ready";
    return item;
}

[[nodiscard]] bool item_less(const ViewportShaderArtifactItem& lhs, const ViewportShaderArtifactItem& rhs) noexcept {
    const auto lhs_stage = stage_sort_key(lhs.stage);
    const auto rhs_stage = stage_sort_key(rhs.stage);
    if (lhs_stage != rhs_stage) {
        return lhs_stage < rhs_stage;
    }
    const auto lhs_target = target_sort_key(lhs.target);
    const auto rhs_target = target_sort_key(rhs.target);
    if (lhs_target != rhs_target) {
        return lhs_target < rhs_target;
    }
    if (lhs.output_path != rhs.output_path) {
        return lhs.output_path < rhs.output_path;
    }
    return lhs.shader.value < rhs.shader.value;
}

} // namespace

void ViewportShaderArtifactState::refresh_from(const IFileSystem& filesystem,
                                               const std::vector<ShaderCompileRequest>& requests) {
    items_.clear();
    items_.reserve(requests.size());
    for (const auto& request : requests) {
        items_.push_back(make_item(filesystem, request));
    }
    std::ranges::sort(items_, item_less);
}

void ViewportShaderArtifactState::clear() noexcept {
    items_.clear();
}

const std::vector<ViewportShaderArtifactItem>& ViewportShaderArtifactState::items() const noexcept {
    return items_;
}

std::size_t ViewportShaderArtifactState::item_count() const noexcept {
    return items_.size();
}

std::size_t ViewportShaderArtifactState::ready_count() const noexcept {
    return static_cast<std::size_t>(std::ranges::count_if(
        items_, [](const auto& item) { return item.status == ViewportShaderArtifactStatus::ready; }));
}

bool ViewportShaderArtifactState::ready_for_d3d12() const noexcept {
    const auto* vertex = find(ShaderSourceStage::vertex, ShaderCompileTarget::d3d12_dxil);
    const auto* fragment = find(ShaderSourceStage::fragment, ShaderCompileTarget::d3d12_dxil);
    return vertex != nullptr && fragment != nullptr && vertex->status == ViewportShaderArtifactStatus::ready &&
           fragment->status == ViewportShaderArtifactStatus::ready;
}

bool ViewportShaderArtifactState::ready_for_d3d12(AssetId shader) const noexcept {
    const auto* vertex = find(shader, ShaderSourceStage::vertex, ShaderCompileTarget::d3d12_dxil);
    const auto* fragment = find(shader, ShaderSourceStage::fragment, ShaderCompileTarget::d3d12_dxil);
    return vertex != nullptr && fragment != nullptr && vertex->status == ViewportShaderArtifactStatus::ready &&
           fragment->status == ViewportShaderArtifactStatus::ready;
}

bool ViewportShaderArtifactState::ready_for_vulkan() const noexcept {
    const auto* vertex = find(ShaderSourceStage::vertex, ShaderCompileTarget::vulkan_spirv);
    const auto* fragment = find(ShaderSourceStage::fragment, ShaderCompileTarget::vulkan_spirv);
    return vertex != nullptr && fragment != nullptr && vertex->status == ViewportShaderArtifactStatus::ready &&
           fragment->status == ViewportShaderArtifactStatus::ready;
}

bool ViewportShaderArtifactState::ready_for_vulkan(AssetId shader) const noexcept {
    const auto* vertex = find(shader, ShaderSourceStage::vertex, ShaderCompileTarget::vulkan_spirv);
    const auto* fragment = find(shader, ShaderSourceStage::fragment, ShaderCompileTarget::vulkan_spirv);
    return vertex != nullptr && fragment != nullptr && vertex->status == ViewportShaderArtifactStatus::ready &&
           fragment->status == ViewportShaderArtifactStatus::ready;
}

const ViewportShaderArtifactItem* ViewportShaderArtifactState::find(ShaderSourceStage stage) const noexcept {
    const auto found = std::ranges::find_if(items_, [stage](const auto& item) { return item.stage == stage; });
    return found == items_.end() ? nullptr : &*found;
}

const ViewportShaderArtifactItem* ViewportShaderArtifactState::find(ShaderSourceStage stage,
                                                                    ShaderCompileTarget target) const noexcept {
    const auto found = std::ranges::find_if(
        items_, [stage, target](const auto& item) { return item.stage == stage && item.target == target; });
    return found == items_.end() ? nullptr : &*found;
}

const ViewportShaderArtifactItem* ViewportShaderArtifactState::find(AssetId shader, ShaderSourceStage stage,
                                                                    ShaderCompileTarget target) const noexcept {
    const auto found = std::ranges::find_if(items_, [shader, stage, target](const auto& item) {
        return item.shader == shader && item.stage == stage && item.target == target;
    });
    return found == items_.end() ? nullptr : &*found;
}

std::string_view viewport_shader_artifact_status_label(ViewportShaderArtifactStatus status) noexcept {
    switch (status) {
    case ViewportShaderArtifactStatus::missing:
        return "Missing";
    case ViewportShaderArtifactStatus::empty:
        return "Empty";
    case ViewportShaderArtifactStatus::ready:
        return "Ready";
    case ViewportShaderArtifactStatus::unsupported_stage:
        return "Unsupported Stage";
    case ViewportShaderArtifactStatus::unsupported_target:
        return "Unsupported Target";
    }
    return "Unknown";
}

} // namespace mirakana::editor
