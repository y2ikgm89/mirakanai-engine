// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/rhi/d3d12/d3d12_backend.hpp"
#include "mirakana/rhi/rhi.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <d3d12.h>
#include <wrl/client.h>

#include <cstdint>
#include <string_view>

namespace mirakana::rhi::d3d12::native {

enum class D3d12DirectStorageBufferDestinationStatus : std::uint8_t {
    ready = 0,
    unsupported_backend,
    invalid_buffer,
    incompatible_buffer,
    resource_unavailable,
};

struct D3d12DirectStorageBufferDestination {
    D3d12DirectStorageBufferDestinationStatus status{D3d12DirectStorageBufferDestinationStatus::unsupported_backend};
    BackendKind backend{BackendKind::null};
    BufferHandle buffer;
    BufferDesc desc;
    std::uint64_t size_bytes{0};
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    bool copy_destination_compatible{false};
    bool default_heap_resource{false};
    bool exposed_native_handles{false};
    std::string_view diagnostic;

    [[nodiscard]] bool succeeded() const noexcept {
        return status == D3d12DirectStorageBufferDestinationStatus::ready;
    }
};

[[nodiscard]] D3d12DirectStorageBufferDestination
resolve_directstorage_buffer_destination(IRhiDevice& device, BufferHandle buffer) noexcept;

} // namespace mirakana::rhi::d3d12::native
