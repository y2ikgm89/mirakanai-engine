// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/rhi/metal/metal_backend.hpp"

namespace mirakana::rhi::metal {

struct MetalRuntimeDevice::Impl {
    void* device{nullptr};
    void* command_queue{nullptr};
    void (*release_object)(void*) noexcept {nullptr};
    /// Monotonic counter for unique `GameEngine.RHI.Metal.*` GPU debugger labels (Apple host only).
    std::uint64_t debug_label_serial{0};
    bool destroyed{false};

    ~Impl() {
        reset();
    }

    void reset() noexcept {
        if (command_queue != nullptr || device != nullptr) {
            destroyed = true;
        }
        if (release_object != nullptr && command_queue != nullptr) {
            release_object(command_queue);
        }
        if (release_object != nullptr && device != nullptr) {
            release_object(device);
        }
        command_queue = nullptr;
        device = nullptr;
    }
};

struct MetalRuntimeCommandBuffer::Impl {
    std::shared_ptr<MetalRuntimeDevice::Impl> device_owner;
    void* command_buffer{nullptr};
    void (*release_object)(void*) noexcept {nullptr};
    bool committed{false};
    bool completed{false};
    bool destroyed{false};

    ~Impl() {
        reset();
    }

    void reset() noexcept {
        if (command_buffer != nullptr) {
            destroyed = true;
        }
        if (release_object != nullptr && command_buffer != nullptr) {
            release_object(command_buffer);
        }
        command_buffer = nullptr;
        device_owner.reset();
    }
};

struct MetalRuntimeTexture::Impl {
    std::shared_ptr<MetalRuntimeDevice::Impl> device_owner;
    void* texture{nullptr};
    void (*release_object)(void*) noexcept {nullptr};
    Extent2D extent{};
    Format format{Format::unknown};
    bool drawable{false};
    bool destroyed{false};

    ~Impl() {
        reset();
    }

    void reset() noexcept {
        if (texture != nullptr) {
            destroyed = true;
        }
        if (release_object != nullptr && texture != nullptr) {
            release_object(texture);
        }
        texture = nullptr;
        device_owner.reset();
    }
};

struct MetalRuntimeDrawable::Impl {
    std::shared_ptr<MetalRuntimeDevice::Impl> device_owner;
    void* drawable{nullptr};
    void* texture{nullptr};
    void (*release_object)(void*) noexcept {nullptr};
    Extent2D extent{};
    Format format{Format::unknown};
    bool presented{false};
    bool destroyed{false};

    ~Impl() {
        reset();
    }

    void reset() noexcept {
        if (drawable != nullptr || texture != nullptr) {
            destroyed = true;
        }
        if (release_object != nullptr && texture != nullptr) {
            release_object(texture);
        }
        if (release_object != nullptr && drawable != nullptr) {
            release_object(drawable);
        }
        texture = nullptr;
        drawable = nullptr;
        device_owner.reset();
    }
};

struct MetalRuntimeRenderEncoder::Impl {
    std::shared_ptr<MetalRuntimeCommandBuffer::Impl> command_buffer_owner;
    std::shared_ptr<MetalRuntimeTexture::Impl> texture_owner;
    std::shared_ptr<MetalRuntimeDrawable::Impl> drawable_owner;
    void* render_encoder{nullptr};
    void (*end_encoder)(void*) noexcept {nullptr};
    void (*release_object)(void*) noexcept {nullptr};
    bool ended{false};
    bool destroyed{false};

    ~Impl() {
        reset();
    }

    void end() noexcept {
        if (!ended && end_encoder != nullptr && render_encoder != nullptr) {
            end_encoder(render_encoder);
        }
        ended = true;
    }

    void reset() noexcept {
        if (render_encoder != nullptr) {
            end();
            destroyed = true;
        }
        if (release_object != nullptr && render_encoder != nullptr) {
            release_object(render_encoder);
        }
        render_encoder = nullptr;
        command_buffer_owner.reset();
        texture_owner.reset();
        drawable_owner.reset();
    }
};

} // namespace mirakana::rhi::metal
