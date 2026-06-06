// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "metal_native_private.hpp"

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mirakana::rhi::metal::native {

bool metal_sdk_headers_and_framework_linked() noexcept {
    return true;
}

} // namespace mirakana::rhi::metal::native

namespace mirakana::rhi::metal {

namespace {

// Assigns `-[MTLObject setLabel:]` when supported (GPU Debugger / Instruments object lists).
static void metal_set_gpu_debug_label(id object, NSString* label) {
    if (object == nil || label == nil) {
        return;
    }
    if ([object respondsToSelector:@selector(setLabel:)]) {
        [object setLabel:label];
    }
}

static NSString* metal_next_debug_label(MetalRuntimeDevice::Impl* owner, const char* prefix_utf8) {
    if (owner == nullptr || prefix_utf8 == nullptr) {
        return nil;
    }
    const auto serial = static_cast<unsigned long long>(++owner->debug_label_serial);
    return [NSString stringWithFormat:@"%s.%llu", prefix_utf8, serial];
}

void release_objc_object(void* object) noexcept {
    if (object != nullptr) {
        CFRelease(object);
    }
}

void end_render_encoder_object(void* object) noexcept {
    if (object != nullptr) {
        [(__bridge id<MTLRenderCommandEncoder>)object endEncoding];
    }
}

[[nodiscard]] MTLPixelFormat metal_pixel_format(Format format) noexcept {
    switch (format) {
    case Format::rgba8_unorm:
        return MTLPixelFormatRGBA8Unorm;
    case Format::bgra8_unorm:
        return MTLPixelFormatBGRA8Unorm;
    case Format::unknown:
    case Format::depth24_stencil8:
        return MTLPixelFormatInvalid;
    }

    return MTLPixelFormatInvalid;
}

[[nodiscard]] std::uint64_t bytes_per_pixel(Format format) noexcept {
    switch (format) {
    case Format::rgba8_unorm:
    case Format::bgra8_unorm:
        return 4;
    case Format::unknown:
    case Format::depth24_stencil8:
        return 0;
    }

    return 0;
}

[[nodiscard]] MTLTextureUsage metal_texture_usage(TextureUsage usage) noexcept {
    MTLTextureUsage result = MTLTextureUsageUnknown;
    if (has_flag(usage, TextureUsage::shader_resource)) {
        result |= MTLTextureUsageShaderRead;
    }
    if (has_flag(usage, TextureUsage::render_target)) {
        result |= MTLTextureUsageRenderTarget;
    }
    if (has_flag(usage, TextureUsage::storage)) {
        result |= MTLTextureUsageShaderWrite;
    }

    return result;
}

[[nodiscard]] CAMetalLayer* metal_layer_from_surface(SurfaceHandle surface) noexcept {
    if (surface.value == 0) {
        return nil;
    }

    id surface_object = (__bridge id)reinterpret_cast<void*>(surface.value);
    if (surface_object == nil || ![surface_object isKindOfClass:[CAMetalLayer class]]) {
        return nil;
    }

    return (CAMetalLayer*)surface_object;
}

[[nodiscard]] id<MTLRenderCommandEncoder> create_render_encoder_for_texture(id<MTLCommandBuffer> command_buffer,
                                                                             id<MTLTexture> texture,
                                                                             const MetalRenderEncoderDesc& desc) {
    MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = texture;
    pass.colorAttachments[0].loadAction = desc.clear ? MTLLoadActionClear : MTLLoadActionLoad;
    pass.colorAttachments[0].storeAction = MTLStoreActionStore;
    pass.colorAttachments[0].clearColor =
        MTLClearColorMake(desc.clear_color.red, desc.clear_color.green, desc.clear_color.blue, desc.clear_color.alpha);

    return [command_buffer renderCommandEncoderWithDescriptor:pass];
}

[[nodiscard]] std::string metal_error_message(NSError* error, const char* fallback) {
    if (error != nil && error.localizedDescription != nil && error.localizedDescription.UTF8String != nullptr) {
        return std::string{error.localizedDescription.UTF8String};
    }
    return fallback == nullptr ? std::string{} : std::string{fallback};
}

[[nodiscard]] bool metal_buffer_has_nonzero_byte(id<MTLBuffer> buffer, NSUInteger byte_count) {
    if (buffer == nil || byte_count == 0) {
        return false;
    }

    const auto* bytes = static_cast<const std::uint8_t*>([buffer contents]);
    if (bytes == nullptr) {
        return false;
    }

    for (NSUInteger index = 0; index < byte_count; ++index) {
        if (bytes[index] != 0U) {
            return true;
        }
    }
    return false;
}

} // namespace

MetalRuntimeDeviceQueueCreateResult create_native_device_and_command_queue(const MetalNativeDeviceQueueDesc& desc) {
    MetalRuntimeDeviceQueueCreateResult result;
    const auto host = desc.host == RhiHostPlatform::unknown ? current_rhi_host_platform() : desc.host;
    const auto availability = diagnose_platform_availability(host, true);
    if (!availability.host_supported) {
        result.diagnostic = "Metal native device and command queue require an Apple host";
        result.probe = make_probe_result(host, BackendProbeStatus::unsupported_host, result.diagnostic);
        return result;
    }

    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (device == nil) {
        result.diagnostic = "Metal default device is required";
        result.probe = make_probe_result(host, BackendProbeStatus::no_suitable_device, result.diagnostic);
        return result;
    }

    id<MTLCommandQueue> command_queue = [device newCommandQueue];
    if (command_queue == nil) {
        result.diagnostic = "Metal command queue is required";
        result.probe = make_probe_result(host, BackendProbeStatus::unavailable, result.diagnostic);
        return result;
    }

    metal_set_gpu_debug_label(device, @"GameEngine.RHI.Metal.Device");
    metal_set_gpu_debug_label(command_queue, @"GameEngine.RHI.Metal.CommandQueue");

    auto impl = std::make_shared<MetalRuntimeDevice::Impl>();
    impl->device = (__bridge_retained void*)device;
    impl->command_queue = (__bridge_retained void*)command_queue;
    impl->release_object = release_objc_object;

    result.device = MetalRuntimeDevice{std::move(impl)};
    result.created = true;
    result.diagnostic = "Metal native device and command queue owner ready";
    result.probe = make_probe_result(host, BackendProbeStatus::available, result.diagnostic);
    return result;
}

MetalRuntimeCommandBufferCreateResult create_native_command_buffer(MetalRuntimeDevice& device) {
    MetalRuntimeCommandBufferCreateResult result;
    if (!device.owns_command_queue()) {
        result.diagnostic = "Metal command queue is required before creating a command buffer";
        return result;
    }

    id<MTLCommandQueue> command_queue = (__bridge id<MTLCommandQueue>)device.impl_->command_queue;
    id<MTLCommandBuffer> command_buffer = [command_queue commandBuffer];
    if (command_buffer == nil) {
        result.diagnostic = "Metal command buffer creation failed";
        return result;
    }

    metal_set_gpu_debug_label(command_buffer,
                              metal_next_debug_label(device.impl_.get(), "GameEngine.RHI.Metal.CommandBuffer"));

    auto impl = std::make_shared<MetalRuntimeCommandBuffer::Impl>();
    impl->device_owner = device.impl_;
    impl->command_buffer = (__bridge_retained void*)command_buffer;
    impl->release_object = release_objc_object;

    result.command_buffer = MetalRuntimeCommandBuffer{std::move(impl)};
    result.created = true;
    result.diagnostic = "Metal native command buffer owner ready";
    return result;
}

MetalRuntimeTextureCreateResult create_native_texture_target(MetalRuntimeDevice& device,
                                                             const MetalTextureTargetDesc& desc) {
    MetalRuntimeTextureCreateResult result;
    if (!device.owns_device()) {
        result.diagnostic = "Metal device is required before creating a texture target";
        return result;
    }
    if (desc.extent.width == 0 || desc.extent.height == 0) {
        result.diagnostic = "Metal texture target extent is required";
        return result;
    }
    if (desc.drawable) {
        result.diagnostic = "Metal drawable textures must be acquired from a platform drawable";
        return result;
    }

    const auto pixel_format = metal_pixel_format(desc.format);
    if (pixel_format == MTLPixelFormatInvalid) {
        result.diagnostic = "Metal texture target format is unsupported";
        return result;
    }

    id<MTLDevice> metal_device = (__bridge id<MTLDevice>)device.impl_->device;
    MTLTextureDescriptor* descriptor =
        [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:pixel_format
                                                           width:static_cast<NSUInteger>(desc.extent.width)
                                                          height:static_cast<NSUInteger>(desc.extent.height)
                                                       mipmapped:NO];
    descriptor.usage = metal_texture_usage(desc.usage);
    descriptor.storageMode = MTLStorageModePrivate;

    id<MTLTexture> texture = [metal_device newTextureWithDescriptor:descriptor];
    if (texture == nil) {
        result.diagnostic = "Metal texture target creation failed";
        return result;
    }

    metal_set_gpu_debug_label(texture,
                              metal_next_debug_label(device.impl_.get(), "GameEngine.RHI.Metal.TextureTarget"));

    auto impl = std::make_shared<MetalRuntimeTexture::Impl>();
    impl->device_owner = device.impl_;
    impl->texture = (__bridge_retained void*)texture;
    impl->release_object = release_objc_object;
    impl->extent = desc.extent;
    impl->format = desc.format;
    impl->drawable = false;

    result.texture = MetalRuntimeTexture{std::move(impl)};
    result.created = true;
    result.diagnostic = "Metal native texture target owner ready";
    return result;
}

MetalRuntimeDrawableAcquireResult acquire_native_drawable(MetalRuntimeDevice& device,
                                                          const MetalDrawableAcquireDesc& desc) {
    MetalRuntimeDrawableAcquireResult result;
    if (!device.owns_device()) {
        result.diagnostic = "Metal device is required before acquiring a drawable";
        return result;
    }
    if (desc.surface.value == 0) {
        result.diagnostic = "Metal drawable surface handle is required";
        return result;
    }
    if (desc.extent.width == 0 || desc.extent.height == 0) {
        result.diagnostic = "Metal drawable extent is required";
        return result;
    }

    const auto pixel_format = metal_pixel_format(desc.format);
    if (pixel_format == MTLPixelFormatInvalid) {
        result.diagnostic = "Metal drawable format is unsupported";
        return result;
    }

    CAMetalLayer* layer = metal_layer_from_surface(desc.surface);
    if (layer == nil) {
        result.diagnostic = "Metal drawable surface must reference a CAMetalLayer";
        return result;
    }

    id<MTLDevice> metal_device = (__bridge id<MTLDevice>)device.impl_->device;
    layer.device = metal_device;
    layer.pixelFormat = pixel_format;
    layer.drawableSize = CGSizeMake(desc.extent.width, desc.extent.height);
    layer.framebufferOnly = desc.framebuffer_only ? YES : NO;

    id<CAMetalDrawable> drawable = [layer nextDrawable];
    if (drawable == nil) {
        result.diagnostic = "Metal drawable acquisition failed";
        return result;
    }

    id<MTLTexture> texture = drawable.texture;
    if (texture == nil) {
        result.diagnostic = "Metal drawable texture is required";
        return result;
    }

    metal_set_gpu_debug_label(texture,
                              metal_next_debug_label(device.impl_.get(), "GameEngine.RHI.Metal.DrawableTexture"));

    auto impl = std::make_shared<MetalRuntimeDrawable::Impl>();
    impl->device_owner = device.impl_;
    impl->drawable = (__bridge_retained void*)drawable;
    impl->texture = (__bridge_retained void*)texture;
    impl->release_object = release_objc_object;
    impl->extent = desc.extent;
    impl->format = desc.format;

    result.drawable = MetalRuntimeDrawable{std::move(impl)};
    result.acquired = true;
    result.diagnostic = "Metal native drawable owner ready";
    return result;
}

MetalRuntimeRenderEncoderCreateResult create_native_render_encoder(MetalRuntimeCommandBuffer& command_buffer,
                                                                    MetalRuntimeTexture& texture,
                                                                    const MetalRenderEncoderDesc& desc) {
    MetalRuntimeRenderEncoderCreateResult result;
    if (!command_buffer.owns_command_buffer()) {
        result.diagnostic = "Metal command buffer is required before creating a render encoder";
        return result;
    }
    if (!texture.owns_texture()) {
        result.diagnostic = "Metal texture target is required before creating a render encoder";
        return result;
    }
    if (command_buffer.committed()) {
        result.diagnostic = "Metal command buffer is already committed";
        return result;
    }

    id<MTLCommandBuffer> metal_command_buffer = (__bridge id<MTLCommandBuffer>)command_buffer.impl_->command_buffer;
    id<MTLTexture> metal_texture = (__bridge id<MTLTexture>)texture.impl_->texture;
    id<MTLRenderCommandEncoder> encoder = create_render_encoder_for_texture(metal_command_buffer, metal_texture, desc);
    if (encoder == nil) {
        result.diagnostic = "Metal render command encoder creation failed";
        return result;
    }

    metal_set_gpu_debug_label(
        encoder, metal_next_debug_label(command_buffer.impl_->device_owner.get(), "GameEngine.RHI.Metal.RenderEncoder"));

    auto impl = std::make_shared<MetalRuntimeRenderEncoder::Impl>();
    impl->command_buffer_owner = command_buffer.impl_;
    impl->texture_owner = texture.impl_;
    impl->render_encoder = (__bridge_retained void*)encoder;
    impl->end_encoder = end_render_encoder_object;
    impl->release_object = release_objc_object;

    result.render_encoder = MetalRuntimeRenderEncoder{std::move(impl)};
    result.created = true;
    result.diagnostic = "Metal native render encoder owner ready";
    return result;
}

MetalRuntimeRenderEncoderCreateResult create_native_render_encoder(MetalRuntimeCommandBuffer& command_buffer,
                                                                    MetalRuntimeDrawable& drawable,
                                                                    const MetalRenderEncoderDesc& desc) {
    MetalRuntimeRenderEncoderCreateResult result;
    if (!command_buffer.owns_command_buffer()) {
        result.diagnostic = "Metal command buffer is required before creating a render encoder";
        return result;
    }
    if (!drawable.owns_texture()) {
        result.diagnostic = "Metal drawable texture is required before creating a render encoder";
        return result;
    }
    if (command_buffer.committed()) {
        result.diagnostic = "Metal command buffer is already committed";
        return result;
    }

    id<MTLCommandBuffer> metal_command_buffer = (__bridge id<MTLCommandBuffer>)command_buffer.impl_->command_buffer;
    id<MTLTexture> metal_texture = (__bridge id<MTLTexture>)drawable.impl_->texture;
    id<MTLRenderCommandEncoder> encoder = create_render_encoder_for_texture(metal_command_buffer, metal_texture, desc);
    if (encoder == nil) {
        result.diagnostic = "Metal render command encoder creation failed";
        return result;
    }

    metal_set_gpu_debug_label(
        encoder, metal_next_debug_label(command_buffer.impl_->device_owner.get(), "GameEngine.RHI.Metal.RenderEncoder"));

    auto impl = std::make_shared<MetalRuntimeRenderEncoder::Impl>();
    impl->command_buffer_owner = command_buffer.impl_;
    impl->drawable_owner = drawable.impl_;
    impl->render_encoder = (__bridge_retained void*)encoder;
    impl->end_encoder = end_render_encoder_object;
    impl->release_object = release_objc_object;

    result.render_encoder = MetalRuntimeRenderEncoder{std::move(impl)};
    result.created = true;
    result.diagnostic = "Metal native render encoder owner ready";
    return result;
}

MetalRuntimeCommandBufferSubmitResult commit_and_wait_native_command_buffer(MetalRuntimeCommandBuffer& command_buffer) {
    MetalRuntimeCommandBufferSubmitResult result;
    if (!command_buffer.owns_command_buffer()) {
        result.diagnostic = "Metal command buffer is required before commit";
        return result;
    }
    if (command_buffer.committed()) {
        result.submitted = true;
        result.completed = command_buffer.completed();
        result.diagnostic = command_buffer.completed() ? "Metal command buffer already completed"
                                                       : "Metal command buffer already committed";
        return result;
    }

    id<MTLCommandBuffer> metal_command_buffer = (__bridge id<MTLCommandBuffer>)command_buffer.impl_->command_buffer;
    [metal_command_buffer commit];
    command_buffer.impl_->committed = true;
    result.submitted = true;

    [metal_command_buffer waitUntilCompleted];
    command_buffer.impl_->completed = metal_command_buffer.status == MTLCommandBufferStatusCompleted;
    result.completed = command_buffer.impl_->completed;
    result.diagnostic = result.completed ? "Metal command buffer completed" : "Metal command buffer failed";
    return result;
}

MetalRuntimeTextureReadbackResult read_native_texture_bytes(MetalRuntimeDevice& device, MetalRuntimeTexture& texture) {
    MetalRuntimeTextureReadbackResult result;
    if (!device.owns_command_queue()) {
        result.diagnostic = "Metal command queue is required before texture readback";
        return result;
    }
    if (!texture.owns_texture()) {
        result.diagnostic = "Metal texture is required before readback";
        return result;
    }

    const auto pixel_bytes = bytes_per_pixel(texture.format());
    if (pixel_bytes == 0) {
        result.diagnostic = "Metal texture readback format is unsupported";
        return result;
    }

    const auto extent = texture.extent();
    const auto row_bytes = static_cast<std::uint64_t>(extent.width) * pixel_bytes;
    const auto byte_count = row_bytes * static_cast<std::uint64_t>(extent.height);

    id<MTLDevice> metal_device = (__bridge id<MTLDevice>)device.impl_->device;
    id<MTLCommandQueue> command_queue = (__bridge id<MTLCommandQueue>)device.impl_->command_queue;
    id<MTLTexture> metal_texture = (__bridge id<MTLTexture>)texture.impl_->texture;
    id<MTLBuffer> readback_buffer =
        [metal_device newBufferWithLength:static_cast<NSUInteger>(byte_count) options:MTLResourceStorageModeShared];
    if (readback_buffer == nil) {
        result.diagnostic = "Metal readback buffer creation failed";
        return result;
    }

    id<MTLCommandBuffer> command_buffer = [command_queue commandBuffer];
    id<MTLBlitCommandEncoder> blit_encoder = [command_buffer blitCommandEncoder];
    if (command_buffer == nil || blit_encoder == nil) {
        result.diagnostic = "Metal readback command creation failed";
        release_objc_object((__bridge_retained void*)readback_buffer);
        return result;
    }

    metal_set_gpu_debug_label(readback_buffer,
                              metal_next_debug_label(device.impl_.get(), "GameEngine.RHI.Metal.ReadbackBuffer"));
    metal_set_gpu_debug_label(command_buffer,
                              metal_next_debug_label(device.impl_.get(), "GameEngine.RHI.Metal.ReadbackCommandBuffer"));
    metal_set_gpu_debug_label(blit_encoder,
                              metal_next_debug_label(device.impl_.get(), "GameEngine.RHI.Metal.BlitEncoder"));

    [blit_encoder copyFromTexture:metal_texture
                      sourceSlice:0
                      sourceLevel:0
                     sourceOrigin:MTLOriginMake(0, 0, 0)
                       sourceSize:MTLSizeMake(extent.width, extent.height, 1)
                         toBuffer:readback_buffer
                destinationOffset:0
           destinationBytesPerRow:static_cast<NSUInteger>(row_bytes)
         destinationBytesPerImage:static_cast<NSUInteger>(byte_count)];
    [blit_encoder endEncoding];
    [command_buffer commit];
    [command_buffer waitUntilCompleted];

    if (command_buffer.status != MTLCommandBufferStatusCompleted) {
        result.diagnostic = "Metal texture readback command buffer failed";
        release_objc_object((__bridge_retained void*)readback_buffer);
        return result;
    }

    const auto* contents = static_cast<const std::uint8_t*>([readback_buffer contents]);
    result.bytes.assign(contents, contents + byte_count);
    result.extent = extent;
    result.format = texture.format();
    result.read = true;
    result.diagnostic = "Metal texture readback completed";

    release_objc_object((__bridge_retained void*)readback_buffer);
    return result;
}

MetalRuntimeDrawablePresentResult present_native_drawable(MetalRuntimeCommandBuffer& command_buffer,
                                                          MetalRuntimeDrawable& drawable) {
    MetalRuntimeDrawablePresentResult result;
    if (!command_buffer.owns_command_buffer()) {
        result.diagnostic = "Metal command buffer is required before presenting a drawable";
        return result;
    }
    if (!drawable.owns_drawable()) {
        result.diagnostic = "Metal drawable is required before present";
        return result;
    }
    if (command_buffer.committed()) {
        result.diagnostic = "Metal command buffer is already committed";
        return result;
    }
    if (drawable.presented()) {
        result.scheduled = true;
        result.diagnostic = "Metal drawable already scheduled for present";
        return result;
    }

    id<MTLCommandBuffer> metal_command_buffer = (__bridge id<MTLCommandBuffer>)command_buffer.impl_->command_buffer;
    id<CAMetalDrawable> metal_drawable = (__bridge id<CAMetalDrawable>)drawable.impl_->drawable;
    [metal_command_buffer presentDrawable:metal_drawable];
    drawable.impl_->presented = true;

    result.scheduled = true;
    result.diagnostic = "Metal native drawable scheduled for present";
    return result;
}

MetalNativeEnvironmentFeatureHostEvidenceResult
create_native_environment_feature_host_evidence(const MetalNativeEnvironmentFeatureHostEvidenceDesc& desc) {
    MetalNativeEnvironmentFeatureHostEvidenceResult result;
    const auto host = desc.host == RhiHostPlatform::unknown ? current_rhi_host_platform() : desc.host;
    const auto availability = diagnose_platform_availability(host, true);
    if (!availability.host_supported) {
        result.diagnostic = "Metal environment native feature evidence requires an Apple host";
        return result;
    }
    if (desc.metallib_path.empty()) {
        result.diagnostic = "Metal environment native feature evidence requires a metallib artifact";
        return result;
    }

    auto native_device = create_native_device_and_command_queue(MetalNativeDeviceQueueDesc{.host = host});
    result.runtime_ready = native_device.created;
    result.command_queue_ready = native_device.device.owns_command_queue();
    if (!native_device.created) {
        result.diagnostic = native_device.diagnostic;
        return result;
    }

    id<MTLDevice> metal_device = (__bridge id<MTLDevice>)native_device.device.impl_->device;
    id<MTLCommandQueue> command_queue = (__bridge id<MTLCommandQueue>)native_device.device.impl_->command_queue;

    NSString* metallib_path = [NSString stringWithUTF8String:desc.metallib_path.c_str()];
    if (metallib_path == nil) {
        result.diagnostic = "Metal environment native feature evidence metallib path is not UTF-8";
        return result;
    }

    NSError* error = nil;
    id<MTLLibrary> library = [metal_device newLibraryWithFile:metallib_path error:&error];
    if (library == nil) {
        result.diagnostic = metal_error_message(error, "Metal environment metallib load failed");
        return result;
    }
    result.shader_library_ready = true;

    id<MTLFunction> vertex_function = [library newFunctionWithName:@"mk_environment_feature_vertex"];
    id<MTLFunction> fragment_function = [library newFunctionWithName:@"mk_environment_feature_fragment"];
    id<MTLFunction> compute_function = [library newFunctionWithName:@"mk_environment_feature_compute"];
    if (vertex_function == nil || fragment_function == nil || compute_function == nil) {
        result.diagnostic = "Metal environment metallib is missing feature evidence entry points";
        return result;
    }

    MTLRenderPipelineDescriptor* render_pipeline_desc = [MTLRenderPipelineDescriptor new];
    render_pipeline_desc.vertexFunction = vertex_function;
    render_pipeline_desc.fragmentFunction = fragment_function;
    render_pipeline_desc.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA8Unorm;
    render_pipeline_desc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
    id<MTLRenderPipelineState> render_pipeline =
        [metal_device newRenderPipelineStateWithDescriptor:render_pipeline_desc error:&error];
    if (render_pipeline == nil) {
        result.diagnostic = metal_error_message(error, "Metal environment render pipeline creation failed");
        return result;
    }
    result.render_pipeline_ready = true;

    id<MTLComputePipelineState> compute_pipeline =
        [metal_device newComputePipelineStateWithFunction:compute_function error:&error];
    if (compute_pipeline == nil) {
        result.diagnostic = metal_error_message(error, "Metal environment compute pipeline creation failed");
        return result;
    }
    result.compute_pipeline_ready = true;

    MTLTextureDescriptor* cube_desc = [MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:MTLPixelFormatRGBA16Float
                                                                                           size:4
                                                                                      mipmapped:NO];
    cube_desc.usage = MTLTextureUsageShaderRead;
    id<MTLTexture> cube_texture = [metal_device newTextureWithDescriptor:cube_desc];
    if (cube_texture == nil) {
        result.diagnostic = "Metal environment HDR cube texture creation failed";
        return result;
    }
    metal_set_gpu_debug_label(cube_texture,
                              metal_next_debug_label(native_device.device.impl_.get(),
                                                     "GameEngine.RHI.Metal.EnvironmentCubeTexture"));
    result.cube_map_texture_feature_available = true;
    result.hdr_texture_feature_available = true;

    constexpr NSUInteger cube_size = 4;
    constexpr NSUInteger channels_per_pixel = 4;
    constexpr NSUInteger bytes_per_half = 2;
    constexpr NSUInteger cube_row_bytes = cube_size * channels_per_pixel * bytes_per_half;
    constexpr NSUInteger cube_face_bytes = cube_row_bytes * cube_size;
    std::vector<std::uint16_t> cube_face(cube_size * cube_size * channels_per_pixel, 0x3c00U);
    for (NSUInteger slice = 0; slice < 6; ++slice) {
        [cube_texture replaceRegion:MTLRegionMake2D(0, 0, cube_size, cube_size)
                         mipmapLevel:0
                               slice:slice
                           withBytes:cube_face.data()
                         bytesPerRow:cube_row_bytes
                       bytesPerImage:cube_face_bytes];
    }

    MTLTextureDescriptor* target_desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                           width:4
                                                                                          height:4
                                                                                       mipmapped:NO];
    target_desc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    target_desc.storageMode = MTLStorageModePrivate;
    id<MTLTexture> render_target = [metal_device newTextureWithDescriptor:target_desc];
    if (render_target == nil) {
        result.diagnostic = "Metal environment render target creation failed";
        return result;
    }
    metal_set_gpu_debug_label(render_target,
                              metal_next_debug_label(native_device.device.impl_.get(),
                                                     "GameEngine.RHI.Metal.EnvironmentRenderTarget"));

    MTLTextureDescriptor* depth_desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                                                                          width:4
                                                                                         height:4
                                                                                      mipmapped:NO];
    depth_desc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    depth_desc.storageMode = MTLStorageModePrivate;
    id<MTLTexture> depth_texture = [metal_device newTextureWithDescriptor:depth_desc];
    if (depth_texture == nil) {
        result.diagnostic = "Metal environment depth texture creation failed";
        return result;
    }
    metal_set_gpu_debug_label(depth_texture,
                              metal_next_debug_label(native_device.device.impl_.get(),
                                                     "GameEngine.RHI.Metal.EnvironmentDepthTexture"));
    result.depth_texture_ready = true;

    MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = render_target;
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].storeAction = MTLStoreActionStore;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    pass.depthAttachment.texture = depth_texture;
    pass.depthAttachment.loadAction = MTLLoadActionClear;
    pass.depthAttachment.storeAction = MTLStoreActionDontCare;
    pass.depthAttachment.clearDepth = 1.0;

    id<MTLCommandBuffer> render_command = [command_queue commandBuffer];
    id<MTLRenderCommandEncoder> render_encoder = [render_command renderCommandEncoderWithDescriptor:pass];
    if (render_command == nil || render_encoder == nil) {
        result.diagnostic = "Metal environment render command creation failed";
        return result;
    }
    metal_set_gpu_debug_label(render_command,
                              metal_next_debug_label(native_device.device.impl_.get(),
                                                     "GameEngine.RHI.Metal.EnvironmentRenderCommandBuffer"));
    metal_set_gpu_debug_label(render_encoder,
                              metal_next_debug_label(native_device.device.impl_.get(),
                                                     "GameEngine.RHI.Metal.EnvironmentRenderEncoder"));
    [render_encoder setRenderPipelineState:render_pipeline];
    [render_encoder setFragmentTexture:cube_texture atIndex:0];
    [render_encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
    [render_encoder endEncoding];
    [render_command commit];
    [render_command waitUntilCompleted];
    if (render_command.status != MTLCommandBufferStatusCompleted) {
        result.diagnostic = "Metal environment render command buffer failed";
        return result;
    }
    result.render_pass_ready = true;

    constexpr NSUInteger render_row_bytes = 4 * 4;
    constexpr NSUInteger render_byte_count = render_row_bytes * 4;
    id<MTLBuffer> render_readback =
        [metal_device newBufferWithLength:render_byte_count options:MTLResourceStorageModeShared];
    id<MTLCommandBuffer> blit_command = [command_queue commandBuffer];
    id<MTLBlitCommandEncoder> blit_encoder = [blit_command blitCommandEncoder];
    if (render_readback == nil || blit_command == nil || blit_encoder == nil) {
        result.diagnostic = "Metal environment render readback command creation failed";
        return result;
    }
    [blit_encoder copyFromTexture:render_target
                      sourceSlice:0
                      sourceLevel:0
                     sourceOrigin:MTLOriginMake(0, 0, 0)
                       sourceSize:MTLSizeMake(4, 4, 1)
                         toBuffer:render_readback
                destinationOffset:0
           destinationBytesPerRow:render_row_bytes
         destinationBytesPerImage:render_byte_count];
    [blit_encoder endEncoding];
    [blit_command commit];
    [blit_command waitUntilCompleted];
    if (blit_command.status != MTLCommandBufferStatusCompleted) {
        result.diagnostic = "Metal environment render readback command buffer failed";
        return result;
    }
    result.render_readback_nonzero = metal_buffer_has_nonzero_byte(render_readback, render_byte_count);

    id<MTLBuffer> compute_output =
        [metal_device newBufferWithLength:sizeof(std::uint32_t) options:MTLResourceStorageModeShared];
    id<MTLCommandBuffer> compute_command = [command_queue commandBuffer];
    id<MTLComputeCommandEncoder> compute_encoder = [compute_command computeCommandEncoder];
    if (compute_output == nil || compute_command == nil || compute_encoder == nil) {
        result.diagnostic = "Metal environment compute command creation failed";
        return result;
    }
    metal_set_gpu_debug_label(compute_output,
                              metal_next_debug_label(native_device.device.impl_.get(),
                                                     "GameEngine.RHI.Metal.EnvironmentParticleBuffer"));
    [compute_encoder setComputePipelineState:compute_pipeline];
    [compute_encoder setBuffer:compute_output offset:0 atIndex:0];
    [compute_encoder dispatchThreads:MTLSizeMake(1, 1, 1) threadsPerThreadgroup:MTLSizeMake(1, 1, 1)];
    [compute_encoder endEncoding];
    [compute_command commit];
    [compute_command waitUntilCompleted];
    if (compute_command.status != MTLCommandBufferStatusCompleted) {
        result.diagnostic = "Metal environment compute command buffer failed";
        return result;
    }
    result.particle_buffer_ready = true;
    result.compute_readback_nonzero = metal_buffer_has_nonzero_byte(compute_output, sizeof(std::uint32_t));
    result.synchronization_evidence_ready = result.render_readback_nonzero && result.compute_readback_nonzero;
    result.ready = result.runtime_ready && result.command_queue_ready && result.shader_library_ready &&
                   result.render_pass_ready && result.render_pipeline_ready && result.compute_pipeline_ready &&
                   result.depth_texture_ready && result.particle_buffer_ready &&
                   result.cube_map_texture_feature_available && result.hdr_texture_feature_available &&
                   result.synchronization_evidence_ready && !result.native_handle_access;
    result.diagnostic =
        result.ready ? "Metal environment native feature evidence ready" : "Metal environment native evidence incomplete";
    return result;
}

} // namespace mirakana::rhi::metal
