// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/rhi/d3d12/d3d12_backend.hpp"

struct ID3D12CommandQueue;
struct ID3D12Device;
struct ID3D12Fence;
struct ID3D12Resource;

namespace mirakana::rhi::d3d12::detail {

struct MavgGpuCullingDeviceBinding {
    ID3D12Device* device{nullptr};
    ID3D12CommandQueue* command_queue{nullptr};
    ID3D12Fence* fence{nullptr};
};

[[nodiscard]] MavgGpuCullingDeviceBinding mavg_gpu_culling_device_binding(DeviceContext& context) noexcept;
[[nodiscard]] ID3D12Resource* committed_resource(DeviceContext& context, NativeResourceHandle handle) noexcept;

} // namespace mirakana::rhi::d3d12::detail
