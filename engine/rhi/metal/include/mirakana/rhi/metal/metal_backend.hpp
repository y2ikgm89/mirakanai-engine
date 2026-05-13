// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/rhi/backend_capabilities.hpp"
#include "mirakana/rhi/rhi.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::rhi::metal {

enum class MetalResourceUsage {
    none,
    render_target_write,
    depth_write,
    shader_read,
    blit_read,
    blit_write,
    drawable_present,
};

enum class MetalSynchronizationStep {
    no_op,
    end_render_encoder,
    begin_render_encoder,
    begin_blit_encoder,
    present_drawable,
};

struct MetalShaderLibraryArtifactDesc {
    const void* bytecode{nullptr};
    std::uint64_t bytecode_size{0};
};

struct MetalShaderLibraryArtifactValidation {
    bool valid{false};
    std::uint64_t bytecode_size{0};
    std::string diagnostic;
};

struct MetalRuntimeReadinessDesc {
    RhiHostPlatform host{RhiHostPlatform::unknown};
    bool runtime_available{false};
    bool default_device_created{false};
    bool command_queue_created{false};
    MetalShaderLibraryArtifactValidation shader_library;
};

struct MetalRuntimeReadinessPlan {
    BackendProbeResult probe;
    bool host_supported{false};
    bool runtime_loaded{false};
    bool default_device_ready{false};
    bool command_queue_ready{false};
    bool shader_library_ready{false};
    std::string diagnostic;
};

struct MetalTextureSynchronizationDesc {
    TextureUsage usage{TextureUsage::none};
    ResourceState before{ResourceState::undefined};
    ResourceState after{ResourceState::undefined};
    bool drawable{false};
};

struct MetalTextureSynchronizationPlan {
    bool supported{false};
    MetalResourceUsage before_usage{MetalResourceUsage::none};
    MetalResourceUsage after_usage{MetalResourceUsage::none};
    bool requires_encoder_boundary{false};
    bool requires_render_encoder{false};
    bool requires_blit_encoder{false};
    bool requires_drawable_present{false};
    std::vector<MetalSynchronizationStep> steps;
    std::string diagnostic;
};

struct MetalPlatformAvailabilityDiagnostics {
    RhiHostPlatform host{RhiHostPlatform::unknown};
    bool host_supported{false};
    bool objcxx_enabled{false};
    bool runtime_probe_required{false};
    bool can_compile_native_sources{false};
    std::string diagnostic;
};

struct MetalNativeDeviceQueueDesc {
    RhiHostPlatform host{RhiHostPlatform::unknown};
};

class MetalRuntimeCommandBuffer;
class MetalRuntimeDrawable;
class MetalRuntimeRenderEncoder;
class MetalRuntimeTexture;

struct MetalRuntimeDeviceQueueCreateResult;
struct MetalRuntimeCommandBufferCreateResult;
struct MetalRuntimeTextureCreateResult;
struct MetalRuntimeDrawableAcquireResult;
struct MetalRuntimeRenderEncoderCreateResult;
struct MetalRuntimeCommandBufferSubmitResult;
struct MetalRuntimeTextureReadbackResult;
struct MetalRuntimeDrawablePresentResult;

struct MetalTextureTargetDesc {
    Extent2D extent{};
    Format format{Format::rgba8_unorm};
    TextureUsage usage{TextureUsage::render_target | TextureUsage::copy_source};
    bool drawable{false};
};

struct MetalDrawableAcquireDesc {
    SurfaceHandle surface{};
    Extent2D extent{};
    Format format{Format::bgra8_unorm};
    bool framebuffer_only{true};
};

struct MetalRenderEncoderDesc {
    ClearColorValue clear_color{};
    bool clear{true};
};

class MetalRuntimeDevice {
  public:
    struct Impl;

    MetalRuntimeDevice() noexcept;
    ~MetalRuntimeDevice();

    MetalRuntimeDevice(MetalRuntimeDevice&& other) noexcept;
    MetalRuntimeDevice& operator=(MetalRuntimeDevice&& other) noexcept;

    MetalRuntimeDevice(const MetalRuntimeDevice&) = delete;
    MetalRuntimeDevice& operator=(const MetalRuntimeDevice&) = delete;

    [[nodiscard]] bool owns_device() const noexcept;
    [[nodiscard]] bool owns_command_queue() const noexcept;
    [[nodiscard]] bool destroyed() const noexcept;
    void reset() noexcept;

  private:
    explicit MetalRuntimeDevice(std::shared_ptr<Impl> impl) noexcept;

    std::shared_ptr<Impl> impl_;

    friend struct MetalRuntimeDeviceQueueCreateResult;
    friend struct MetalRuntimeCommandBufferCreateResult;
    friend struct MetalRuntimeTextureCreateResult;
    friend struct MetalRuntimeTextureReadbackResult;
    friend MetalRuntimeDeviceQueueCreateResult
    create_native_device_and_command_queue(const MetalNativeDeviceQueueDesc& desc);
    friend MetalRuntimeCommandBufferCreateResult create_native_command_buffer(MetalRuntimeDevice& device);
    friend MetalRuntimeTextureCreateResult create_native_texture_target(MetalRuntimeDevice& device,
                                                                        const MetalTextureTargetDesc& desc);
    friend MetalRuntimeDrawableAcquireResult acquire_native_drawable(MetalRuntimeDevice& device,
                                                                     const MetalDrawableAcquireDesc& desc);
    friend MetalRuntimeTextureReadbackResult read_native_texture_bytes(MetalRuntimeDevice& device,
                                                                       MetalRuntimeTexture& texture);
};

class MetalRuntimeCommandBuffer {
  public:
    struct Impl;

    MetalRuntimeCommandBuffer() noexcept;
    ~MetalRuntimeCommandBuffer();

    MetalRuntimeCommandBuffer(MetalRuntimeCommandBuffer&& other) noexcept;
    MetalRuntimeCommandBuffer& operator=(MetalRuntimeCommandBuffer&& other) noexcept;

    MetalRuntimeCommandBuffer(const MetalRuntimeCommandBuffer&) = delete;
    MetalRuntimeCommandBuffer& operator=(const MetalRuntimeCommandBuffer&) = delete;

    [[nodiscard]] bool owns_command_buffer() const noexcept;
    [[nodiscard]] bool committed() const noexcept;
    [[nodiscard]] bool completed() const noexcept;
    [[nodiscard]] bool destroyed() const noexcept;
    void reset() noexcept;

  private:
    explicit MetalRuntimeCommandBuffer(std::shared_ptr<Impl> impl) noexcept;

    std::shared_ptr<Impl> impl_;

    friend struct MetalRuntimeCommandBufferCreateResult;
    friend struct MetalRuntimeRenderEncoderCreateResult;
    friend struct MetalRuntimeCommandBufferSubmitResult;
    friend struct MetalRuntimeDrawablePresentResult;
    friend MetalRuntimeCommandBufferCreateResult create_native_command_buffer(MetalRuntimeDevice& device);
    friend MetalRuntimeRenderEncoderCreateResult create_native_render_encoder(MetalRuntimeCommandBuffer& command_buffer,
                                                                              MetalRuntimeTexture& texture,
                                                                              const MetalRenderEncoderDesc& desc);
    friend MetalRuntimeRenderEncoderCreateResult create_native_render_encoder(MetalRuntimeCommandBuffer& command_buffer,
                                                                              MetalRuntimeDrawable& drawable,
                                                                              const MetalRenderEncoderDesc& desc);
    friend MetalRuntimeCommandBufferSubmitResult
    commit_and_wait_native_command_buffer(MetalRuntimeCommandBuffer& command_buffer);
    friend MetalRuntimeDrawablePresentResult present_native_drawable(MetalRuntimeCommandBuffer& command_buffer,
                                                                     MetalRuntimeDrawable& drawable);
};

class MetalRuntimeTexture {
  public:
    struct Impl;

    MetalRuntimeTexture() noexcept;
    ~MetalRuntimeTexture();

    MetalRuntimeTexture(MetalRuntimeTexture&& other) noexcept;
    MetalRuntimeTexture& operator=(MetalRuntimeTexture&& other) noexcept;

    MetalRuntimeTexture(const MetalRuntimeTexture&) = delete;
    MetalRuntimeTexture& operator=(const MetalRuntimeTexture&) = delete;

    [[nodiscard]] bool owns_texture() const noexcept;
    [[nodiscard]] Extent2D extent() const noexcept;
    [[nodiscard]] Format format() const noexcept;
    [[nodiscard]] bool drawable() const noexcept;
    [[nodiscard]] bool destroyed() const noexcept;
    void reset() noexcept;

  private:
    explicit MetalRuntimeTexture(std::shared_ptr<Impl> impl) noexcept;

    std::shared_ptr<Impl> impl_;

    friend struct MetalRuntimeTextureCreateResult;
    friend struct MetalRuntimeRenderEncoderCreateResult;
    friend struct MetalRuntimeTextureReadbackResult;
    friend MetalRuntimeTextureCreateResult create_native_texture_target(MetalRuntimeDevice& device,
                                                                        const MetalTextureTargetDesc& desc);
    friend MetalRuntimeRenderEncoderCreateResult create_native_render_encoder(MetalRuntimeCommandBuffer& command_buffer,
                                                                              MetalRuntimeTexture& texture,
                                                                              const MetalRenderEncoderDesc& desc);
    friend MetalRuntimeTextureReadbackResult read_native_texture_bytes(MetalRuntimeDevice& device,
                                                                       MetalRuntimeTexture& texture);
};

class MetalRuntimeDrawable {
  public:
    struct Impl;

    MetalRuntimeDrawable() noexcept;
    ~MetalRuntimeDrawable();

    MetalRuntimeDrawable(MetalRuntimeDrawable&& other) noexcept;
    MetalRuntimeDrawable& operator=(MetalRuntimeDrawable&& other) noexcept;

    MetalRuntimeDrawable(const MetalRuntimeDrawable&) = delete;
    MetalRuntimeDrawable& operator=(const MetalRuntimeDrawable&) = delete;

    [[nodiscard]] bool owns_drawable() const noexcept;
    [[nodiscard]] bool owns_texture() const noexcept;
    [[nodiscard]] Extent2D extent() const noexcept;
    [[nodiscard]] Format format() const noexcept;
    [[nodiscard]] bool presented() const noexcept;
    [[nodiscard]] bool destroyed() const noexcept;
    void reset() noexcept;

  private:
    explicit MetalRuntimeDrawable(std::shared_ptr<Impl> impl) noexcept;

    std::shared_ptr<Impl> impl_;

    friend struct MetalRuntimeDrawableAcquireResult;
    friend struct MetalRuntimeRenderEncoderCreateResult;
    friend struct MetalRuntimeDrawablePresentResult;
    friend MetalRuntimeDrawableAcquireResult acquire_native_drawable(MetalRuntimeDevice& device,
                                                                     const MetalDrawableAcquireDesc& desc);
    friend MetalRuntimeRenderEncoderCreateResult create_native_render_encoder(MetalRuntimeCommandBuffer& command_buffer,
                                                                              MetalRuntimeDrawable& drawable,
                                                                              const MetalRenderEncoderDesc& desc);
    friend MetalRuntimeDrawablePresentResult present_native_drawable(MetalRuntimeCommandBuffer& command_buffer,
                                                                     MetalRuntimeDrawable& drawable);
};

class MetalRuntimeRenderEncoder {
  public:
    struct Impl;

    MetalRuntimeRenderEncoder() noexcept;
    ~MetalRuntimeRenderEncoder();

    MetalRuntimeRenderEncoder(MetalRuntimeRenderEncoder&& other) noexcept;
    MetalRuntimeRenderEncoder& operator=(MetalRuntimeRenderEncoder&& other) noexcept;

    MetalRuntimeRenderEncoder(const MetalRuntimeRenderEncoder&) = delete;
    MetalRuntimeRenderEncoder& operator=(const MetalRuntimeRenderEncoder&) = delete;

    [[nodiscard]] bool owns_render_encoder() const noexcept;
    [[nodiscard]] bool ended() const noexcept;
    [[nodiscard]] bool destroyed() const noexcept;
    void end() noexcept;
    void reset() noexcept;

  private:
    explicit MetalRuntimeRenderEncoder(std::shared_ptr<Impl> impl) noexcept;

    std::shared_ptr<Impl> impl_;

    friend struct MetalRuntimeRenderEncoderCreateResult;
    friend MetalRuntimeRenderEncoderCreateResult create_native_render_encoder(MetalRuntimeCommandBuffer& command_buffer,
                                                                              MetalRuntimeTexture& texture,
                                                                              const MetalRenderEncoderDesc& desc);
    friend MetalRuntimeRenderEncoderCreateResult create_native_render_encoder(MetalRuntimeCommandBuffer& command_buffer,
                                                                              MetalRuntimeDrawable& drawable,
                                                                              const MetalRenderEncoderDesc& desc);
};

struct MetalRuntimeDeviceQueueCreateResult {
    MetalRuntimeDevice device;
    BackendProbeResult probe;
    bool created{false};
    std::string diagnostic;
};

struct MetalRuntimeCommandBufferCreateResult {
    MetalRuntimeCommandBuffer command_buffer;
    bool created{false};
    std::string diagnostic;
};

struct MetalRuntimeTextureCreateResult {
    MetalRuntimeTexture texture;
    bool created{false};
    std::string diagnostic;
};

struct MetalRuntimeDrawableAcquireResult {
    MetalRuntimeDrawable drawable;
    bool acquired{false};
    std::string diagnostic;
};

struct MetalRuntimeRenderEncoderCreateResult {
    MetalRuntimeRenderEncoder render_encoder;
    bool created{false};
    std::string diagnostic;
};

struct MetalRuntimeCommandBufferSubmitResult {
    bool submitted{false};
    bool completed{false};
    std::string diagnostic;
};

struct MetalRuntimeTextureReadbackResult {
    bool read{false};
    Extent2D extent{};
    Format format{Format::unknown};
    std::vector<std::uint8_t> bytes;
    std::string diagnostic;
};

struct MetalRuntimeDrawablePresentResult {
    bool scheduled{false};
    std::string diagnostic;
};

[[nodiscard]] BackendKind backend_kind() noexcept;
[[nodiscard]] std::string_view backend_name() noexcept;
[[nodiscard]] BackendCapabilityProfile capabilities() noexcept;
[[nodiscard]] BackendProbePlan probe_plan() noexcept;
[[nodiscard]] bool supports_host(RhiHostPlatform host) noexcept;
[[nodiscard]] BackendProbeResult make_probe_result(RhiHostPlatform host, BackendProbeStatus status,
                                                   std::string_view diagnostic = {});
[[nodiscard]] MetalShaderLibraryArtifactValidation
validate_shader_library_artifact(const MetalShaderLibraryArtifactDesc& desc);
[[nodiscard]] MetalRuntimeReadinessPlan build_runtime_readiness_plan(const MetalRuntimeReadinessDesc& desc);
[[nodiscard]] MetalTextureSynchronizationPlan
build_texture_synchronization_plan(const MetalTextureSynchronizationDesc& desc);
[[nodiscard]] MetalPlatformAvailabilityDiagnostics diagnose_platform_availability(RhiHostPlatform host,
                                                                                  bool objcxx_enabled);
[[nodiscard]] MetalRuntimeDeviceQueueCreateResult
create_native_device_and_command_queue(const MetalNativeDeviceQueueDesc& desc = {});
[[nodiscard]] MetalRuntimeCommandBufferCreateResult create_native_command_buffer(MetalRuntimeDevice& device);
[[nodiscard]] MetalRuntimeTextureCreateResult create_native_texture_target(MetalRuntimeDevice& device,
                                                                           const MetalTextureTargetDesc& desc);
[[nodiscard]] MetalRuntimeDrawableAcquireResult acquire_native_drawable(MetalRuntimeDevice& device,
                                                                        const MetalDrawableAcquireDesc& desc);
[[nodiscard]] MetalRuntimeRenderEncoderCreateResult
create_native_render_encoder(MetalRuntimeCommandBuffer& command_buffer, MetalRuntimeTexture& texture,
                             const MetalRenderEncoderDesc& desc = {});
[[nodiscard]] MetalRuntimeRenderEncoderCreateResult
create_native_render_encoder(MetalRuntimeCommandBuffer& command_buffer, MetalRuntimeDrawable& drawable,
                             const MetalRenderEncoderDesc& desc = {});
[[nodiscard]] MetalRuntimeCommandBufferSubmitResult
commit_and_wait_native_command_buffer(MetalRuntimeCommandBuffer& command_buffer);
[[nodiscard]] MetalRuntimeTextureReadbackResult read_native_texture_bytes(MetalRuntimeDevice& device,
                                                                          MetalRuntimeTexture& texture);
[[nodiscard]] MetalRuntimeDrawablePresentResult present_native_drawable(MetalRuntimeCommandBuffer& command_buffer,
                                                                        MetalRuntimeDrawable& drawable);

} // namespace mirakana::rhi::metal
