// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/rhi/gpu_debug.hpp"
#include "mirakana/rhi/rhi.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::rhi {
namespace {

using BufferByteDifference = std::vector<std::uint8_t>::difference_type;

[[nodiscard]] BufferByteDifference checked_buffer_byte_difference(std::uint64_t value) {
    const auto max_difference = static_cast<std::uint64_t>(std::numeric_limits<BufferByteDifference>::max());
    if (value > max_difference) {
        throw std::invalid_argument("rhi byte offset is too large for host iterator arithmetic");
    }
    return static_cast<BufferByteDifference>(value);
}

void validate_buffer_desc(const BufferDesc& desc) {
    if (desc.size_bytes == 0) {
        throw std::invalid_argument("rhi buffer size must be non-zero");
    }
    if (desc.usage == BufferUsage::none) {
        throw std::invalid_argument("rhi buffer usage must not be none");
    }
}

void validate_texture_desc(const TextureDesc& desc) {
    if (desc.extent.width == 0 || desc.extent.height == 0 || desc.extent.depth == 0) {
        throw std::invalid_argument("rhi texture extent must be non-zero");
    }
    if (desc.format == Format::unknown) {
        throw std::invalid_argument("rhi texture format must not be unknown");
    }
    if (desc.usage == TextureUsage::none) {
        throw std::invalid_argument("rhi texture usage must not be none");
    }
    const TextureUsage sampled_depth_usage = TextureUsage::depth_stencil | TextureUsage::shader_resource;
    if (desc.format == Format::depth24_stencil8) {
        if (desc.usage != TextureUsage::depth_stencil && desc.usage != sampled_depth_usage) {
            throw std::invalid_argument("rhi depth texture usage is unsupported");
        }
        return;
    }
    if (has_flag(desc.usage, TextureUsage::depth_stencil)) {
        throw std::invalid_argument("rhi depth texture format must be depth24_stencil8");
    }
}

void validate_swapchain_desc(const SwapchainDesc& desc) {
    if (desc.extent.width == 0 || desc.extent.height == 0) {
        throw std::invalid_argument("rhi swapchain extent must be non-zero");
    }
    if (desc.format == Format::unknown) {
        throw std::invalid_argument("rhi swapchain format must not be unknown");
    }
    if (desc.buffer_count < 2) {
        throw std::invalid_argument("rhi swapchain must use at least two buffers");
    }
}

void validate_swapchain_extent(Extent2D extent) {
    if (extent.width == 0 || extent.height == 0) {
        throw std::invalid_argument("rhi swapchain extent must be non-zero");
    }
}

[[nodiscard]] bool valid_queue_kind(QueueKind queue) noexcept {
    switch (queue) {
    case QueueKind::graphics:
    case QueueKind::compute:
    case QueueKind::copy:
        return true;
    }

    return false;
}

[[nodiscard]] std::size_t queue_index(QueueKind queue) noexcept {
    switch (queue) {
    case QueueKind::graphics:
        return 0;
    case QueueKind::compute:
        return 1;
    case QueueKind::copy:
        return 2;
    }

    return 0;
}

void record_queue_submit(RhiStats& stats, QueueKind queue, FenceValue fence) noexcept {
    ++stats.queue_event_sequence;
    stats.last_submitted_fence_queue = fence.queue;
    switch (queue) {
    case QueueKind::graphics:
        ++stats.graphics_queue_submits;
        stats.last_graphics_submitted_fence_value = fence.value;
        stats.last_graphics_submit_sequence = stats.queue_event_sequence;
        return;
    case QueueKind::compute:
        ++stats.compute_queue_submits;
        stats.last_compute_submitted_fence_value = fence.value;
        stats.last_compute_submit_sequence = stats.queue_event_sequence;
        return;
    case QueueKind::copy:
        ++stats.copy_queue_submits;
        stats.last_copy_submitted_fence_value = fence.value;
        stats.last_copy_submit_sequence = stats.queue_event_sequence;
        return;
    }
}

void record_queue_wait(RhiStats& stats, QueueKind queue, FenceValue fence) noexcept {
    ++stats.queue_event_sequence;
    stats.last_queue_wait_fence_value = fence.value;
    stats.last_queue_wait_fence_queue = fence.queue;
    stats.last_queue_wait_sequence = stats.queue_event_sequence;
    switch (queue) {
    case QueueKind::graphics:
        stats.last_graphics_queue_wait_fence_value = fence.value;
        stats.last_graphics_queue_wait_fence_queue = fence.queue;
        stats.last_graphics_queue_wait_sequence = stats.queue_event_sequence;
        return;
    case QueueKind::compute:
        stats.last_compute_queue_wait_fence_value = fence.value;
        stats.last_compute_queue_wait_fence_queue = fence.queue;
        stats.last_compute_queue_wait_sequence = stats.queue_event_sequence;
        return;
    case QueueKind::copy:
        stats.last_copy_queue_wait_fence_value = fence.value;
        stats.last_copy_queue_wait_fence_queue = fence.queue;
        stats.last_copy_queue_wait_sequence = stats.queue_event_sequence;
        return;
    }
}

[[nodiscard]] bool is_color_render_format(Format format) noexcept {
    return format == Format::rgba8_unorm || format == Format::bgra8_unorm;
}

[[nodiscard]] bool is_depth_render_format(Format format) noexcept {
    return format == Format::depth24_stencil8;
}

[[nodiscard]] bool texture_state_supported_for_desc(const TextureDesc& desc, ResourceState state) noexcept {
    switch (state) {
    case ResourceState::undefined:
        return true;
    case ResourceState::copy_source:
        return has_flag(desc.usage, TextureUsage::copy_source);
    case ResourceState::copy_destination:
        return has_flag(desc.usage, TextureUsage::copy_destination);
    case ResourceState::shader_read:
        return has_flag(desc.usage, TextureUsage::shader_resource);
    case ResourceState::render_target:
        return has_flag(desc.usage, TextureUsage::render_target) && is_color_render_format(desc.format);
    case ResourceState::depth_write:
        return has_flag(desc.usage, TextureUsage::depth_stencil) && is_depth_render_format(desc.format);
    case ResourceState::present:
        return has_flag(desc.usage, TextureUsage::present);
    }
    return false;
}

[[nodiscard]] ResourceState initial_texture_state(TextureUsage usage) noexcept {
    if (has_flag(usage, TextureUsage::copy_destination)) {
        return ResourceState::copy_destination;
    }
    if (has_flag(usage, TextureUsage::copy_source)) {
        return ResourceState::copy_source;
    }
    if (has_flag(usage, TextureUsage::render_target)) {
        return ResourceState::render_target;
    }
    if (has_flag(usage, TextureUsage::depth_stencil)) {
        return ResourceState::depth_write;
    }
    if (has_flag(usage, TextureUsage::present)) {
        return ResourceState::present;
    }
    if (has_flag(usage, TextureUsage::shader_resource)) {
        return ResourceState::shader_read;
    }
    return ResourceState::undefined;
}

[[nodiscard]] Extent2D extent_2d(const Extent3D& extent) noexcept {
    return Extent2D{.width = extent.width, .height = extent.height};
}

void validate_compare_op(CompareOp op) {
    switch (op) {
    case CompareOp::never:
    case CompareOp::less:
    case CompareOp::equal:
    case CompareOp::less_equal:
    case CompareOp::greater:
    case CompareOp::not_equal:
    case CompareOp::greater_equal:
    case CompareOp::always:
        return;
    }
    throw std::invalid_argument("rhi depth compare op is invalid");
}

void validate_clear_depth(ClearDepthValue clear_depth) {
    if (!std::isfinite(clear_depth.depth) || clear_depth.depth < 0.0F || clear_depth.depth > 1.0F) {
        throw std::invalid_argument("rhi render pass depth clear value must be finite and in [0, 1]");
    }
}

std::uint64_t checked_mul(std::uint64_t lhs, std::uint64_t rhs, const char* message) {
    if (lhs != 0 && rhs > std::numeric_limits<std::uint64_t>::max() / lhs) {
        throw std::invalid_argument(message);
    }
    return lhs * rhs;
}

std::uint64_t checked_add(std::uint64_t lhs, std::uint64_t rhs, const char* message) {
    if (rhs > std::numeric_limits<std::uint64_t>::max() - lhs) {
        throw std::invalid_argument(message);
    }
    return lhs + rhs;
}

void validate_shader_desc(const ShaderDesc& desc) {
    if (desc.entry_point.empty()) {
        throw std::invalid_argument("rhi shader entry point must not be empty");
    }
    if (desc.bytecode_size == 0) {
        throw std::invalid_argument("rhi shader bytecode size must be non-zero");
    }
}

void validate_pipeline_layout_desc(const PipelineLayoutDesc& desc) {
    if (desc.push_constant_bytes > 256) {
        throw std::invalid_argument("rhi push constants must not exceed 256 bytes");
    }
}

[[nodiscard]] std::uint32_t vertex_format_size(VertexFormat format) {
    switch (format) {
    case VertexFormat::float32x2:
        return 8;
    case VertexFormat::float32x3:
        return 12;
    case VertexFormat::float32x4:
        return 16;
    case VertexFormat::uint16x4:
        return 8;
    case VertexFormat::unknown:
        break;
    }
    throw std::invalid_argument("rhi vertex attribute format must be known");
}

void validate_vertex_input_desc(const GraphicsPipelineDesc& desc) {
    for (const auto& layout : desc.vertex_buffers) {
        if (layout.stride == 0) {
            throw std::invalid_argument("rhi vertex buffer layout stride must be non-zero");
        }
        const auto duplicate =
            std::ranges::count_if(desc.vertex_buffers, [&layout](const VertexBufferLayoutDesc& candidate) {
                return candidate.binding == layout.binding;
            });
        if (duplicate > 1) {
            throw std::invalid_argument("rhi vertex buffer bindings must be unique");
        }
    }

    for (const auto& attribute : desc.vertex_attributes) {
        const auto layout =
            std::ranges::find_if(desc.vertex_buffers, [&attribute](const VertexBufferLayoutDesc& candidate) {
                return candidate.binding == attribute.binding;
            });
        if (layout == desc.vertex_buffers.end()) {
            throw std::invalid_argument("rhi vertex attribute must reference a declared vertex buffer binding");
        }
        const auto duplicate =
            std::ranges::count_if(desc.vertex_attributes, [&attribute](const VertexAttributeDesc& candidate) {
                return candidate.location == attribute.location;
            });
        if (duplicate > 1) {
            throw std::invalid_argument("rhi vertex attribute locations must be unique");
        }
        const auto size = vertex_format_size(attribute.format);
        if (attribute.offset > layout->stride || size > layout->stride - attribute.offset) {
            throw std::invalid_argument("rhi vertex attribute range must fit inside the vertex stride");
        }
    }
}

bool is_buffer_descriptor(DescriptorType type) {
    return type == DescriptorType::uniform_buffer || type == DescriptorType::storage_buffer;
}

bool is_texture_descriptor(DescriptorType type) {
    return type == DescriptorType::sampled_texture || type == DescriptorType::storage_texture;
}

bool is_sampler_descriptor(DescriptorType type) {
    return type == DescriptorType::sampler;
}

void validate_descriptor_type(DescriptorType type) {
    switch (type) {
    case DescriptorType::uniform_buffer:
    case DescriptorType::storage_buffer:
    case DescriptorType::sampled_texture:
    case DescriptorType::storage_texture:
    case DescriptorType::sampler:
        return;
    }
    throw std::invalid_argument("rhi descriptor type is invalid");
}

void validate_sampler_desc(SamplerDesc desc) {
    const auto valid_filter = [](SamplerFilter filter) noexcept {
        return filter == SamplerFilter::nearest || filter == SamplerFilter::linear;
    };
    const auto valid_address = [](SamplerAddressMode mode) noexcept {
        return mode == SamplerAddressMode::repeat || mode == SamplerAddressMode::clamp_to_edge;
    };
    if (!valid_filter(desc.min_filter) || !valid_filter(desc.mag_filter) || !valid_address(desc.address_u) ||
        !valid_address(desc.address_v) || !valid_address(desc.address_w)) {
        throw std::invalid_argument("rhi sampler description is invalid");
    }
}

void validate_descriptor_set_layout_desc(const DescriptorSetLayoutDesc& desc) {
    for (const auto& binding : desc.bindings) {
        validate_descriptor_type(binding.type);
        if (binding.count == 0) {
            throw std::invalid_argument("rhi descriptor binding count must be non-zero");
        }
        if (!has_stage_visibility(binding.stages)) {
            throw std::invalid_argument("rhi descriptor binding shader visibility must not be none");
        }
        const auto duplicate = std::ranges::count_if(
            desc.bindings, [&binding](const DescriptorBindingDesc& other) { return other.binding == binding.binding; });
        if (duplicate > 1) {
            throw std::invalid_argument("rhi descriptor binding numbers must be unique");
        }
    }
}

void validate_graphics_pipeline_desc(const GraphicsPipelineDesc& desc) {
    if (desc.layout.value == 0) {
        throw std::invalid_argument("rhi graphics pipeline layout must be valid");
    }
    if (desc.vertex_shader.value == 0 || desc.fragment_shader.value == 0) {
        throw std::invalid_argument("rhi graphics pipeline shaders must be valid");
    }
    if (!is_color_render_format(desc.color_format)) {
        throw std::invalid_argument("rhi graphics pipeline color format must be a color render format");
    }
    if (desc.depth_format != Format::unknown && !is_depth_render_format(desc.depth_format)) {
        throw std::invalid_argument("rhi graphics pipeline depth format must be a depth render format");
    }
    validate_compare_op(desc.depth_state.depth_compare);
    if (desc.depth_state.depth_write_enabled && !desc.depth_state.depth_test_enabled) {
        throw std::invalid_argument("rhi graphics pipeline depth writes require depth testing");
    }
    if ((desc.depth_state.depth_test_enabled || desc.depth_state.depth_write_enabled) &&
        desc.depth_format == Format::unknown) {
        throw std::invalid_argument("rhi graphics pipeline depth state requires a depth format");
    }
    validate_vertex_input_desc(desc);
}

void validate_compute_pipeline_desc(const ComputePipelineDesc& desc) {
    if (desc.layout.value == 0) {
        throw std::invalid_argument("rhi compute pipeline layout must be valid");
    }
    if (desc.compute_shader.value == 0) {
        throw std::invalid_argument("rhi compute pipeline shader must be valid");
    }
}

const DescriptorBindingDesc& find_binding(const DescriptorSetLayoutDesc& layout, std::uint32_t binding) {
    const auto found = std::ranges::find_if(
        layout.bindings, [binding](const DescriptorBindingDesc& candidate) { return candidate.binding == binding; });
    if (found == layout.bindings.end()) {
        throw std::invalid_argument("rhi descriptor binding is not declared by the set layout");
    }
    return *found;
}

void validate_buffer_copy_region(const BufferDesc& source, const BufferDesc& destination,
                                 const BufferCopyRegion& region) {
    if (region.size_bytes == 0) {
        throw std::invalid_argument("rhi buffer copy size must be non-zero");
    }
    if (region.source_offset > source.size_bytes || region.size_bytes > source.size_bytes - region.source_offset) {
        throw std::invalid_argument("rhi buffer copy source range is outside the source buffer");
    }
    if (region.destination_offset > destination.size_bytes ||
        region.size_bytes > destination.size_bytes - region.destination_offset) {
        throw std::invalid_argument("rhi buffer copy destination range is outside the destination buffer");
    }
}

void validate_buffer_texture_region(const TextureDesc& texture, const BufferTextureCopyRegion& region) {
    if (region.texture_extent.width == 0 || region.texture_extent.height == 0 || region.texture_extent.depth == 0) {
        throw std::invalid_argument("rhi buffer texture copy extent must be non-zero");
    }
    if (region.texture_offset.x > texture.extent.width ||
        region.texture_extent.width > texture.extent.width - region.texture_offset.x) {
        throw std::invalid_argument("rhi buffer texture copy width is outside the texture");
    }
    if (region.texture_offset.y > texture.extent.height ||
        region.texture_extent.height > texture.extent.height - region.texture_offset.y) {
        throw std::invalid_argument("rhi buffer texture copy height is outside the texture");
    }
    if (region.texture_offset.z > texture.extent.depth ||
        region.texture_extent.depth > texture.extent.depth - region.texture_offset.z) {
        throw std::invalid_argument("rhi buffer texture copy depth is outside the texture");
    }
}

} // namespace

std::uint32_t bytes_per_texel(Format format) {
    switch (format) {
    case Format::rgba8_unorm:
    case Format::bgra8_unorm:
    case Format::depth24_stencil8:
        return 4;
    case Format::unknown:
        break;
    }
    throw std::invalid_argument("rhi format must have a known texel size");
}

std::uint64_t buffer_texture_copy_required_bytes(Format format, const BufferTextureCopyRegion& region) {
    if (region.texture_extent.width == 0 || region.texture_extent.height == 0 || region.texture_extent.depth == 0) {
        throw std::invalid_argument("rhi buffer texture copy extent must be non-zero");
    }

    const auto texel_bytes = static_cast<std::uint64_t>(bytes_per_texel(format));
    const auto row_length = region.buffer_row_length == 0 ? region.texture_extent.width : region.buffer_row_length;
    const auto image_height =
        region.buffer_image_height == 0 ? region.texture_extent.height : region.buffer_image_height;
    if (row_length < region.texture_extent.width) {
        throw std::invalid_argument("rhi buffer texture copy row length must be zero or at least the copy width");
    }
    if (image_height < region.texture_extent.height) {
        throw std::invalid_argument("rhi buffer texture copy image height must be zero or at least the copy height");
    }

    const auto row_pitch = checked_mul(row_length, texel_bytes, "rhi buffer texture copy row pitch overflowed");
    const auto image_pitch = checked_mul(image_height, row_pitch, "rhi buffer texture copy image pitch overflowed");
    const auto depth_offset =
        checked_mul(region.texture_extent.depth - 1U, image_pitch, "rhi buffer texture copy depth offset overflowed");
    const auto row_offset =
        checked_mul(region.texture_extent.height - 1U, row_pitch, "rhi buffer texture copy row offset overflowed");
    const auto final_row_bytes =
        checked_mul(region.texture_extent.width, texel_bytes, "rhi buffer texture copy final row size overflowed");
    return checked_add(
        checked_add(checked_add(region.buffer_offset, depth_offset, "rhi buffer texture copy size overflowed"),
                    row_offset, "rhi buffer texture copy size overflowed"),
        final_row_bytes, "rhi buffer texture copy size overflowed");
}

class NullRhiCommandList final : public IRhiCommandList {
  public:
    NullRhiCommandList(NullRhiDevice& device, QueueKind queue) : device_(device), queue_(queue) {}
    ~NullRhiCommandList() override {
        release_swapchain_reservations_for_abandonment();
    }

    [[nodiscard]] QueueKind queue_kind() const noexcept override {
        return queue_;
    }

    [[nodiscard]] bool closed() const noexcept override {
        return closed_;
    }

    void transition_texture(TextureHandle texture, ResourceState before, ResourceState after) override {
        require_recording();
        if (!device_.owns_texture(texture)) {
            throw std::invalid_argument("rhi texture handle must belong to this device");
        }
        const auto& desc = device_.texture_desc(texture);
        if (!texture_state_supported_for_desc(desc, before) || !texture_state_supported_for_desc(desc, after) ||
            before == after) {
            throw std::invalid_argument("rhi texture transition states are incompatible with the texture");
        }
        if (before == ResourceState::undefined && device_.texture_state(texture) == after) {
            ++device_.stats_.resource_transitions;
            return;
        }
        if (device_.texture_state(texture) != before) {
            throw std::invalid_argument("rhi texture transition before state must match tracked state");
        }
        device_.set_texture_state(texture, after);
        ++device_.stats_.resource_transitions;
    }

    void texture_aliasing_barrier(TextureHandle before, TextureHandle after) override {
        require_recording();
        require_no_render_pass();
        if (before.value == 0 || after.value == 0 || before.value == after.value || !device_.owns_texture(before) ||
            !device_.owns_texture(after)) {
            throw std::invalid_argument("rhi texture aliasing barrier requires two distinct device texture handles");
        }

        ++device_.stats_.texture_aliasing_barriers;
    }

    void copy_buffer(BufferHandle source, BufferHandle destination, const BufferCopyRegion& region) override {
        require_recording();
        require_no_render_pass();
        if (!device_.owns_buffer(source) || !device_.owns_buffer(destination)) {
            throw std::invalid_argument("rhi buffer copy handles must belong to this device");
        }
        const auto& source_desc = device_.buffer_desc(source);
        const auto& destination_desc = device_.buffer_desc(destination);
        if (!has_flag(source_desc.usage, BufferUsage::copy_source)) {
            throw std::invalid_argument("rhi buffer copy source must use copy_source");
        }
        if (!has_flag(destination_desc.usage, BufferUsage::copy_destination)) {
            throw std::invalid_argument("rhi buffer copy destination must use copy_destination");
        }
        validate_buffer_copy_region(source_desc, destination_desc, region);
        const auto source_offset = checked_buffer_byte_difference(region.source_offset);
        const auto destination_offset = checked_buffer_byte_difference(region.destination_offset);
        const auto size = checked_buffer_byte_difference(region.size_bytes);
        const auto& source_bytes = device_.buffer_bytes_.at(source.value - 1U);
        auto& destination_bytes = device_.buffer_bytes_.at(destination.value - 1U);
        std::copy_n(source_bytes.begin() + source_offset, size, destination_bytes.begin() + destination_offset);
        ++device_.stats_.buffer_copies;
        device_.stats_.bytes_copied += region.size_bytes;
    }

    void copy_buffer_to_texture(BufferHandle source, TextureHandle destination,
                                const BufferTextureCopyRegion& region) override {
        require_recording();
        require_no_render_pass();
        if (!device_.owns_buffer(source) || !device_.owns_texture(destination)) {
            throw std::invalid_argument("rhi buffer texture copy handles must belong to this device");
        }
        const auto& source_desc = device_.buffer_desc(source);
        const auto& destination_desc = device_.texture_desc(destination);
        if (!has_flag(source_desc.usage, BufferUsage::copy_source)) {
            throw std::invalid_argument("rhi buffer texture copy source must use copy_source");
        }
        if (!has_flag(destination_desc.usage, TextureUsage::copy_destination)) {
            throw std::invalid_argument("rhi buffer texture copy destination must use copy_destination");
        }
        const auto required_bytes = buffer_texture_copy_required_bytes(destination_desc.format, region);
        if (required_bytes > source_desc.size_bytes) {
            throw std::invalid_argument("rhi buffer texture copy source range is outside the source buffer");
        }
        validate_buffer_texture_region(destination_desc, region);
        ++device_.stats_.buffer_texture_copies;
    }

    void copy_texture_to_buffer(TextureHandle source, BufferHandle destination,
                                const BufferTextureCopyRegion& region) override {
        require_recording();
        require_no_render_pass();
        if (!device_.owns_texture(source) || !device_.owns_buffer(destination)) {
            throw std::invalid_argument("rhi texture buffer copy handles must belong to this device");
        }
        const auto& source_desc = device_.texture_desc(source);
        const auto& destination_desc = device_.buffer_desc(destination);
        if (!has_flag(source_desc.usage, TextureUsage::copy_source)) {
            throw std::invalid_argument("rhi texture buffer copy source must use copy_source");
        }
        if (!has_flag(destination_desc.usage, BufferUsage::copy_destination)) {
            throw std::invalid_argument("rhi texture buffer copy destination must use copy_destination");
        }
        const auto required_bytes = buffer_texture_copy_required_bytes(source_desc.format, region);
        if (required_bytes > destination_desc.size_bytes) {
            throw std::invalid_argument("rhi texture buffer copy destination range is outside the destination buffer");
        }
        validate_buffer_texture_region(source_desc, region);
        ++device_.stats_.texture_buffer_copies;
    }

    void present(SwapchainFrameHandle frame) override {
        require_recording();
        if (queue_ != QueueKind::graphics) {
            throw std::logic_error("rhi swapchain presents require a graphics command list");
        }
        require_no_render_pass();
        if (!device_.owns_swapchain_frame(frame)) {
            throw std::invalid_argument("rhi swapchain frame handle must belong to this device");
        }
        const auto swapchain = device_.swapchain_for_frame(frame);
        if (device_.swapchain_state(swapchain) != ResourceState::present) {
            throw std::invalid_argument("rhi swapchain must be in present state before present command");
        }
        if (!device_.swapchain_presentable(swapchain)) {
            throw std::invalid_argument("rhi swapchain must complete a render pass before present command");
        }
        if (!owns_swapchain_frame(frame)) {
            throw std::invalid_argument("rhi swapchain present must match a frame recorded by this command list");
        }
        if (device_.swapchain_frame_presented(frame)) {
            throw std::invalid_argument("rhi swapchain frame is already presented");
        }
        device_.set_swapchain_presentable(swapchain, false);
        device_.set_swapchain_frame_presented(frame, true);
        pending_present_frames_.push_back(frame);
        ++device_.stats_.present_calls;
    }

    void begin_render_pass(const RenderPassDesc& desc) override {
        require_recording();
        if (queue_ != QueueKind::graphics) {
            throw std::logic_error("rhi render passes require a graphics command list");
        }
        if (render_pass_active_) {
            throw std::logic_error("rhi render pass is already active");
        }
        const bool uses_swapchain = desc.color.swapchain_frame.value != 0;
        const bool uses_texture = desc.color.texture.value != 0;
        if (uses_swapchain == uses_texture) {
            throw std::invalid_argument("rhi render pass requires exactly one color attachment");
        }
        Format active_color_format = Format::unknown;
        Extent2D active_extent{};
        SwapchainHandle swapchain_to_activate{};
        SwapchainFrameHandle swapchain_frame_to_activate{};
        if (uses_swapchain) {
            if (!device_.owns_swapchain_frame(desc.color.swapchain_frame)) {
                throw std::invalid_argument("rhi render pass swapchain frame must belong to this device");
            }
            const auto swapchain = device_.swapchain_for_frame(desc.color.swapchain_frame);
            const auto& swapchain_desc = device_.swapchain_desc(swapchain);
            active_color_format = swapchain_desc.format;
            active_extent = swapchain_desc.extent;
            if (!is_color_render_format(active_color_format)) {
                throw std::invalid_argument("rhi render pass swapchain attachment must use a color format");
            }
            if (device_.swapchain_state(swapchain) != ResourceState::present) {
                throw std::invalid_argument("rhi swapchain render pass attachment must be in present state");
            }
            if (!device_.swapchain_frame_active(desc.color.swapchain_frame) ||
                device_.swapchain_frame_presented(desc.color.swapchain_frame) ||
                device_.swapchain_presentable(swapchain)) {
                throw std::invalid_argument("rhi swapchain already has a pending frame");
            }
            swapchain_to_activate = swapchain;
            swapchain_frame_to_activate = desc.color.swapchain_frame;
        } else {
            if (!device_.owns_texture(desc.color.texture)) {
                throw std::invalid_argument("rhi render pass color attachment must belong to this device");
            }
            const auto& color_desc = device_.texture_desc(desc.color.texture);
            if (!has_flag(color_desc.usage, TextureUsage::render_target)) {
                throw std::invalid_argument("rhi render pass color attachment must use render_target");
            }
            if (!is_color_render_format(color_desc.format)) {
                throw std::invalid_argument("rhi render pass color attachment must use a color format");
            }
            active_color_format = color_desc.format;
            active_extent = extent_2d(color_desc.extent);
        }
        if (desc.depth.texture.value != 0) {
            if (!device_.owns_texture(desc.depth.texture)) {
                throw std::invalid_argument("rhi render pass depth attachment must belong to this device");
            }
            const auto& depth_desc = device_.texture_desc(desc.depth.texture);
            if (!has_flag(depth_desc.usage, TextureUsage::depth_stencil)) {
                throw std::invalid_argument("rhi render pass depth attachment must use depth_stencil");
            }
            if (!is_depth_render_format(depth_desc.format)) {
                throw std::invalid_argument("rhi render pass depth attachment must use a depth format");
            }
            if (extent_2d(depth_desc.extent).width != active_extent.width ||
                extent_2d(depth_desc.extent).height != active_extent.height || depth_desc.extent.depth != 1) {
                throw std::invalid_argument("rhi render pass depth attachment extent must match the color target");
            }
            const auto depth_state = device_.texture_state(desc.depth.texture);
            if (depth_state != ResourceState::undefined && depth_state != ResourceState::depth_write) {
                throw std::invalid_argument("rhi render pass depth attachment must be in depth_write state");
            }
            validate_clear_depth(desc.depth.clear_depth);
            active_depth_format_ = depth_desc.format;
        } else {
            active_depth_format_ = Format::unknown;
        }
        active_color_format_ = active_color_format;
        if (swapchain_frame_to_activate.value != 0) {
            reserved_swapchain_frames_.push_back(swapchain_frame_to_activate);
            device_.set_swapchain_state(swapchain_to_activate, ResourceState::render_target);
            device_.set_swapchain_presentable(swapchain_to_activate, false);
            active_swapchain_frame_ = swapchain_frame_to_activate;
        }
        render_pass_active_ = true;
        graphics_pipeline_bound_ = false;
        bound_graphics_pipeline_ = {};
        ++device_.stats_.render_passes_begun;
    }

    void end_render_pass() override {
        require_recording();
        if (!render_pass_active_) {
            throw std::logic_error("rhi render pass is not active");
        }
        if (active_swapchain_frame_.value != 0) {
            const auto swapchain = device_.swapchain_for_frame(active_swapchain_frame_);
            device_.set_swapchain_state(swapchain, ResourceState::present);
            device_.set_swapchain_presentable(swapchain, true);
            active_swapchain_frame_ = {};
        }
        render_pass_active_ = false;
        graphics_pipeline_bound_ = false;
        bound_graphics_pipeline_ = {};
        active_color_format_ = Format::unknown;
        active_depth_format_ = Format::unknown;
        vertex_buffer_bound_ = false;
        index_buffer_bound_ = false;
    }

    void bind_graphics_pipeline(GraphicsPipelineHandle pipeline) override {
        require_recording();
        if (!render_pass_active_) {
            throw std::logic_error("rhi graphics pipeline must be bound inside a render pass");
        }
        if (!device_.owns_graphics_pipeline(pipeline)) {
            throw std::invalid_argument("rhi graphics pipeline handle must belong to this device");
        }
        if (device_.graphics_pipeline_color_format(pipeline) != active_color_format_) {
            throw std::invalid_argument("rhi graphics pipeline color format must match the active render pass");
        }
        if (device_.graphics_pipeline_depth_format(pipeline) != active_depth_format_) {
            throw std::invalid_argument("rhi graphics pipeline depth format must match the active render pass");
        }
        graphics_pipeline_bound_ = true;
        bound_graphics_pipeline_ = pipeline;
        vertex_buffer_bound_ = false;
        index_buffer_bound_ = false;
        ++device_.stats_.graphics_pipelines_bound;
    }

    void bind_compute_pipeline(ComputePipelineHandle pipeline) override {
        require_recording();
        require_no_render_pass();
        if (queue_ != QueueKind::compute) {
            throw std::logic_error("rhi compute pipeline binding requires a compute command list");
        }
        if (!device_.owns_compute_pipeline(pipeline)) {
            throw std::invalid_argument("rhi compute pipeline handle must belong to this device");
        }
        compute_pipeline_bound_ = true;
        bound_compute_pipeline_ = pipeline;
        ++device_.stats_.compute_pipelines_bound;
    }

    void bind_descriptor_set(PipelineLayoutHandle layout, std::uint32_t set_index, DescriptorSetHandle set) override {
        require_recording();
        if (!render_pass_active_ && !compute_pipeline_bound_) {
            throw std::logic_error("rhi descriptor set binding requires a graphics or compute pipeline");
        }
        if (render_pass_active_ && !graphics_pipeline_bound_) {
            throw std::logic_error("rhi descriptor set binding requires a graphics pipeline");
        }
        if (!device_.owns_pipeline_layout(layout)) {
            throw std::invalid_argument("rhi pipeline layout handle must belong to this device");
        }
        if (!device_.owns_descriptor_set(set)) {
            throw std::invalid_argument("rhi descriptor set handle must belong to this device");
        }
        const auto bound_layout = render_pass_active_
                                      ? device_.pipeline_layout_for_pipeline(bound_graphics_pipeline_)
                                      : device_.pipeline_layout_for_compute_pipeline(bound_compute_pipeline_);
        if (bound_layout.value != layout.value) {
            throw std::invalid_argument("rhi descriptor set pipeline layout must match the bound pipeline");
        }
        const auto& pipeline_layout = device_.pipeline_layout_desc(layout);
        if (set_index >= pipeline_layout.descriptor_sets.size()) {
            throw std::invalid_argument("rhi descriptor set index is outside the pipeline layout");
        }
        const auto expected_layout = pipeline_layout.descriptor_sets.at(set_index);
        const auto actual_layout = device_.descriptor_set_layout_for_set(set);
        if (expected_layout.value != actual_layout.value) {
            throw std::invalid_argument("rhi descriptor set layout is not compatible with the pipeline layout");
        }
        ++device_.stats_.descriptor_sets_bound;
    }

    void bind_vertex_buffer(const VertexBufferBinding& binding) override {
        require_recording();
        if (!render_pass_active_) {
            throw std::logic_error("rhi vertex buffer must be bound inside a render pass");
        }
        if (!graphics_pipeline_bound_) {
            throw std::logic_error("rhi vertex buffer binding requires a graphics pipeline");
        }
        if (!device_.owns_buffer(binding.buffer)) {
            throw std::invalid_argument("rhi vertex buffer handle must belong to this device");
        }
        if (binding.stride == 0) {
            throw std::invalid_argument("rhi vertex buffer stride must be non-zero");
        }
        const auto& desc = device_.buffer_desc(binding.buffer);
        if (!has_flag(desc.usage, BufferUsage::vertex)) {
            throw std::invalid_argument("rhi vertex buffer binding requires vertex buffer usage");
        }
        if (binding.offset >= desc.size_bytes) {
            throw std::invalid_argument("rhi vertex buffer offset is outside the buffer");
        }
        vertex_buffer_bound_ = true;
        ++device_.stats_.vertex_buffer_bindings;
    }

    void bind_index_buffer(const IndexBufferBinding& binding) override {
        require_recording();
        if (!render_pass_active_) {
            throw std::logic_error("rhi index buffer must be bound inside a render pass");
        }
        if (!graphics_pipeline_bound_) {
            throw std::logic_error("rhi index buffer binding requires a graphics pipeline");
        }
        if (!device_.owns_buffer(binding.buffer)) {
            throw std::invalid_argument("rhi index buffer handle must belong to this device");
        }
        if (binding.format == IndexFormat::unknown) {
            throw std::invalid_argument("rhi index buffer format must be known");
        }
        const auto& desc = device_.buffer_desc(binding.buffer);
        if (!has_flag(desc.usage, BufferUsage::index)) {
            throw std::invalid_argument("rhi index buffer binding requires index buffer usage");
        }
        if (binding.offset >= desc.size_bytes) {
            throw std::invalid_argument("rhi index buffer offset is outside the buffer");
        }
        index_buffer_bound_ = true;
        ++device_.stats_.index_buffer_bindings;
    }

    void draw(std::uint32_t vertex_count, std::uint32_t instance_count) override {
        require_recording();
        if (!render_pass_active_) {
            throw std::logic_error("rhi draw must be recorded inside a render pass");
        }
        if (!graphics_pipeline_bound_) {
            throw std::logic_error("rhi draw requires a graphics pipeline");
        }
        if (vertex_count == 0 || instance_count == 0) {
            throw std::invalid_argument("rhi draw counts must be non-zero");
        }
        ++device_.stats_.draw_calls;
        device_.stats_.vertices_submitted += static_cast<std::uint64_t>(vertex_count) * instance_count;
    }

    void draw_indexed(std::uint32_t index_count, std::uint32_t instance_count) override {
        require_recording();
        if (!render_pass_active_) {
            throw std::logic_error("rhi indexed draw must be recorded inside a render pass");
        }
        if (!graphics_pipeline_bound_) {
            throw std::logic_error("rhi indexed draw requires a graphics pipeline");
        }
        if (!vertex_buffer_bound_) {
            throw std::logic_error("rhi indexed draw requires a vertex buffer");
        }
        if (!index_buffer_bound_) {
            throw std::logic_error("rhi indexed draw requires an index buffer");
        }
        if (index_count == 0 || instance_count == 0) {
            throw std::invalid_argument("rhi indexed draw counts must be non-zero");
        }
        ++device_.stats_.draw_calls;
        ++device_.stats_.indexed_draw_calls;
        device_.stats_.indices_submitted += static_cast<std::uint64_t>(index_count) * instance_count;
    }

    void dispatch(std::uint32_t group_count_x, std::uint32_t group_count_y, std::uint32_t group_count_z) override {
        require_recording();
        require_no_render_pass();
        if (queue_ != QueueKind::compute) {
            throw std::logic_error("rhi dispatch requires a compute command list");
        }
        if (!compute_pipeline_bound_) {
            throw std::logic_error("rhi dispatch requires a compute pipeline");
        }
        if (group_count_x == 0 || group_count_y == 0 || group_count_z == 0) {
            throw std::invalid_argument("rhi dispatch workgroup counts must be non-zero");
        }
        ++device_.stats_.compute_dispatches;
        device_.stats_.compute_workgroups_x += group_count_x;
        device_.stats_.compute_workgroups_y += group_count_y;
        device_.stats_.compute_workgroups_z += group_count_z;
    }

    void set_viewport(const ViewportDesc& viewport) override {
        require_recording();
        if (!render_pass_active_) {
            throw std::logic_error("rhi set_viewport must be recorded inside a render pass");
        }
        if (!std::isfinite(viewport.x) || !std::isfinite(viewport.y) || !std::isfinite(viewport.width) ||
            !std::isfinite(viewport.height) || !std::isfinite(viewport.min_depth) ||
            !std::isfinite(viewport.max_depth)) {
            throw std::invalid_argument("rhi viewport fields must be finite");
        }
        if (viewport.width <= 0.0F || viewport.height <= 0.0F) {
            throw std::invalid_argument("rhi viewport width and height must be positive");
        }
    }

    void set_scissor(const ScissorRectDesc& scissor) override {
        require_recording();
        if (!render_pass_active_) {
            throw std::logic_error("rhi set_scissor must be recorded inside a render pass");
        }
        if (scissor.width == 0U || scissor.height == 0U) {
            throw std::invalid_argument("rhi scissor width and height must be non-zero");
        }
    }

    void begin_gpu_debug_scope(std::string_view name) override {
        require_recording();
        validate_rhi_debug_label(name);
        ++gpu_debug_scope_depth_;
        ++device_.stats_.gpu_debug_scopes_begun;
    }

    void end_gpu_debug_scope() override {
        require_recording();
        if (gpu_debug_scope_depth_ == 0) {
            throw std::logic_error("rhi gpu debug scope end without matching begin");
        }
        --gpu_debug_scope_depth_;
        ++device_.stats_.gpu_debug_scopes_ended;
    }

    void insert_gpu_debug_marker(std::string_view name) override {
        require_recording();
        validate_rhi_debug_label(name);
        ++device_.stats_.gpu_debug_markers_inserted;
    }

    void close() override {
        require_recording();
        if (gpu_debug_scope_depth_ != 0) {
            throw std::logic_error("rhi command list cannot close with unbalanced gpu debug scopes");
        }
        if (render_pass_active_) {
            throw std::logic_error("rhi command list cannot close with an active render pass");
        }
        if (has_unpresented_swapchain_frame()) {
            throw std::logic_error("rhi command list cannot close with an unpresented swapchain frame");
        }
        closed_ = true;
    }

    void mark_submitted() {
        release_swapchain_reservations_for_submit();
    }

  private:
    void require_recording() const {
        if (closed_) {
            throw std::logic_error("rhi command list is already closed");
        }
    }

    void require_no_render_pass() const {
        if (render_pass_active_) {
            throw std::logic_error("rhi copy and present commands must be recorded outside a render pass");
        }
    }

    [[nodiscard]] bool owns_swapchain_frame(SwapchainFrameHandle frame) const {
        return std::ranges::any_of(reserved_swapchain_frames_,
                                   [frame](SwapchainFrameHandle reserved) { return reserved.value == frame.value; });
    }

    [[nodiscard]] bool has_unpresented_swapchain_frame() const {
        return std::ranges::any_of(reserved_swapchain_frames_, [this](SwapchainFrameHandle frame) {
            return device_.owns_swapchain_frame(frame) && !device_.swapchain_frame_presented(frame);
        });
    }

    void release_swapchain_reservations_for_submit() {
        for (const auto frame : pending_present_frames_) {
            if (device_.owns_swapchain_frame(frame)) {
                device_.complete_swapchain_frame(frame);
            }
        }
        pending_present_frames_.clear();
        reserved_swapchain_frames_.clear();
    }

    void release_swapchain_reservations_for_abandonment() noexcept {
        for (const auto frame : reserved_swapchain_frames_) {
            if (frame.value == 0 || frame.value > device_.swapchain_frame_swapchains_.size()) {
                continue;
            }
            const auto frame_index = frame.value - 1U;
            const auto swapchain = device_.swapchain_frame_swapchains_[frame_index];
            if (swapchain.value > 0 && swapchain.value <= device_.swapchains_.size()) {
                const auto swapchain_index = swapchain.value - 1U;
                device_.swapchain_states_[swapchain_index] = ResourceState::present;
                device_.swapchain_presentable_[swapchain_index] = false;
                device_.swapchain_frame_reserved_[swapchain_index] = false;
            }
            device_.swapchain_frame_active_[frame_index] = false;
            device_.swapchain_frame_presented_[frame_index] = false;
        }
        pending_present_frames_.clear();
        reserved_swapchain_frames_.clear();
    }

    NullRhiDevice& device_;
    QueueKind queue_;
    bool closed_{false};
    bool render_pass_active_{false};
    bool graphics_pipeline_bound_{false};
    bool compute_pipeline_bound_{false};
    bool vertex_buffer_bound_{false};
    bool index_buffer_bound_{false};
    GraphicsPipelineHandle bound_graphics_pipeline_{};
    ComputePipelineHandle bound_compute_pipeline_{};
    SwapchainFrameHandle active_swapchain_frame_{};
    Format active_color_format_{Format::unknown};
    Format active_depth_format_{Format::unknown};
    std::vector<SwapchainFrameHandle> reserved_swapchain_frames_;
    std::vector<SwapchainFrameHandle> pending_present_frames_;
    std::uint32_t gpu_debug_scope_depth_{0};
};

namespace {

[[nodiscard]] std::string null_resource_debug_name(std::string_view prefix, std::uint32_t id) {
    std::string out;
    out.reserve(prefix.size() + 16U);
    out.append(prefix);
    out.push_back('-');
    out.append(std::to_string(id));
    return out;
}

} // namespace

NullRhiDevice::~NullRhiDevice() {
    const auto flush_fence = std::numeric_limits<std::uint64_t>::max();
    for (std::size_t i = 0; i < buffer_active_.size(); ++i) {
        if (buffer_active_[i]) {
            (void)resource_lifetime_.release_resource_deferred(buffer_lifetime_[i], 0);
            buffer_active_[i] = false;
        }
    }
    for (std::size_t i = 0; i < texture_active_.size(); ++i) {
        if (texture_active_[i]) {
            (void)resource_lifetime_.release_resource_deferred(texture_lifetime_[i], 0);
            texture_active_[i] = false;
        }
    }
    for (std::size_t i = 0; i < sampler_active_.size(); ++i) {
        if (sampler_active_[i]) {
            (void)resource_lifetime_.release_resource_deferred(sampler_lifetime_[i], 0);
            sampler_active_[i] = false;
        }
    }
    for (std::size_t i = 0; i < shader_active_.size(); ++i) {
        if (shader_active_[i]) {
            (void)resource_lifetime_.release_resource_deferred(shader_lifetime_[i], 0);
            shader_active_[i] = false;
        }
    }
    for (std::size_t i = 0; i < descriptor_set_layout_active_.size(); ++i) {
        if (descriptor_set_layout_active_[i]) {
            (void)resource_lifetime_.release_resource_deferred(descriptor_set_layout_lifetime_[i], 0);
            descriptor_set_layout_active_[i] = false;
        }
    }
    for (std::size_t i = 0; i < descriptor_set_active_.size(); ++i) {
        if (descriptor_set_active_[i]) {
            (void)resource_lifetime_.release_resource_deferred(descriptor_set_lifetime_[i], 0);
            descriptor_set_active_[i] = false;
        }
    }
    for (std::size_t i = 0; i < pipeline_layout_active_.size(); ++i) {
        if (pipeline_layout_active_[i]) {
            (void)resource_lifetime_.release_resource_deferred(pipeline_layout_lifetime_[i], 0);
            pipeline_layout_active_[i] = false;
        }
    }
    for (std::size_t i = 0; i < graphics_pipeline_active_.size(); ++i) {
        if (graphics_pipeline_active_[i]) {
            (void)resource_lifetime_.release_resource_deferred(graphics_pipeline_lifetime_[i], 0);
            graphics_pipeline_active_[i] = false;
        }
    }
    for (std::size_t i = 0; i < compute_pipeline_active_.size(); ++i) {
        if (compute_pipeline_active_[i]) {
            (void)resource_lifetime_.release_resource_deferred(compute_pipeline_lifetime_[i], 0);
            compute_pipeline_active_[i] = false;
        }
    }
    (void)resource_lifetime_.retire_released_resources(flush_fence);
}

void NullRhiDevice::retire_resource_lifetime_to_completed_fence() noexcept {
    (void)resource_lifetime_.retire_released_resources(completed_.value);
}

BackendKind NullRhiDevice::backend_kind() const noexcept {
    return BackendKind::null;
}

std::string_view NullRhiDevice::backend_name() const noexcept {
    return "null";
}

RhiStats NullRhiDevice::stats() const noexcept {
    return stats_;
}

std::uint64_t NullRhiDevice::gpu_timestamp_ticks_per_second() const noexcept {
    return 0;
}

RhiDeviceMemoryDiagnostics NullRhiDevice::memory_diagnostics() const {
    RhiDeviceMemoryDiagnostics diagnostics{};
    std::uint64_t total = 0;
    for (std::size_t i = 0; i < buffers_.size(); ++i) {
        if (i < buffer_active_.size() && buffer_active_.at(i)) {
            total += buffers_.at(i).size_bytes;
        }
    }
    for (std::size_t i = 0; i < textures_.size(); ++i) {
        if (i >= texture_active_.size() || !texture_active_.at(i)) {
            continue;
        }
        const auto& desc = textures_.at(i);
        if (desc.format == Format::unknown) {
            continue;
        }
        const auto texels = static_cast<std::uint64_t>(desc.extent.width) *
                            static_cast<std::uint64_t>(desc.extent.height) *
                            static_cast<std::uint64_t>(desc.extent.depth);
        total += texels * static_cast<std::uint64_t>(bytes_per_texel(desc.format));
    }
    diagnostics.committed_resources_byte_estimate = total;
    diagnostics.committed_resources_byte_estimate_available = true;
    return diagnostics;
}

FenceValue NullRhiDevice::completed_fence() const noexcept {
    return completed_;
}

bool NullRhiDevice::owns_texture(TextureHandle texture) const noexcept {
    return texture.value > 0 && texture.value < next_texture_ && texture_active_.at(texture.value - 1U);
}

bool NullRhiDevice::owns_sampler(SamplerHandle sampler) const noexcept {
    return sampler.value > 0 && sampler.value < next_sampler_ && sampler_active_.at(sampler.value - 1U);
}

bool NullRhiDevice::owns_buffer(BufferHandle buffer) const noexcept {
    return buffer.value > 0 && buffer.value < next_buffer_ && buffer_active_.at(buffer.value - 1U);
}

bool NullRhiDevice::owns_swapchain(SwapchainHandle swapchain) const noexcept {
    return swapchain.value > 0 && swapchain.value < next_swapchain_;
}

bool NullRhiDevice::owns_shader(ShaderHandle shader) const noexcept {
    return shader.value > 0 && shader.value < next_shader_ && shader_active_.at(shader.value - 1U);
}

bool NullRhiDevice::owns_descriptor_set_layout(DescriptorSetLayoutHandle layout) const noexcept {
    return layout.value > 0 && layout.value < next_descriptor_set_layout_ &&
           descriptor_set_layout_active_.at(layout.value - 1U);
}

bool NullRhiDevice::owns_descriptor_set(DescriptorSetHandle set) const noexcept {
    return set.value > 0 && set.value < next_descriptor_set_ && descriptor_set_active_.at(set.value - 1U);
}

bool NullRhiDevice::owns_pipeline_layout(PipelineLayoutHandle layout) const noexcept {
    return layout.value > 0 && layout.value < next_pipeline_layout_ && pipeline_layout_active_.at(layout.value - 1U);
}

bool NullRhiDevice::owns_graphics_pipeline(GraphicsPipelineHandle pipeline) const noexcept {
    return pipeline.value > 0 && pipeline.value < next_graphics_pipeline_ &&
           graphics_pipeline_active_.at(pipeline.value - 1U);
}

bool NullRhiDevice::owns_compute_pipeline(ComputePipelineHandle pipeline) const noexcept {
    return pipeline.value > 0 && pipeline.value < next_compute_pipeline_ &&
           compute_pipeline_active_.at(pipeline.value - 1U);
}

const BufferDesc& NullRhiDevice::buffer_desc(BufferHandle buffer) const {
    return buffers_.at(buffer.value - 1U);
}

const TextureDesc& NullRhiDevice::texture_desc(TextureHandle texture) const {
    return textures_.at(texture.value - 1U);
}

ResourceState NullRhiDevice::texture_state(TextureHandle texture) const {
    if (!owns_texture(texture)) {
        throw std::invalid_argument("rhi texture handle must belong to this device");
    }
    return texture_states_.at(texture.value - 1U);
}

void NullRhiDevice::set_texture_state(TextureHandle texture, ResourceState state) {
    if (!owns_texture(texture)) {
        throw std::invalid_argument("rhi texture handle must belong to this device");
    }
    texture_states_.at(texture.value - 1U) = state;
}

const SwapchainDesc& NullRhiDevice::swapchain_desc(SwapchainHandle swapchain) const {
    return swapchains_.at(swapchain.value - 1U);
}

bool NullRhiDevice::owns_swapchain_frame(SwapchainFrameHandle frame) const noexcept {
    return frame.value > 0 && frame.value < next_swapchain_frame_ && swapchain_frame_active_.at(frame.value - 1U);
}

SwapchainHandle NullRhiDevice::swapchain_for_frame(SwapchainFrameHandle frame) const {
    if (!owns_swapchain_frame(frame)) {
        throw std::invalid_argument("rhi swapchain frame handle must belong to this device");
    }
    return swapchain_frame_swapchains_.at(frame.value - 1U);
}

bool NullRhiDevice::swapchain_frame_active(SwapchainFrameHandle frame) const {
    if (frame.value == 0 || frame.value >= next_swapchain_frame_) {
        throw std::invalid_argument("rhi swapchain frame handle must belong to this device");
    }
    return swapchain_frame_active_.at(frame.value - 1U);
}

bool NullRhiDevice::swapchain_frame_presented(SwapchainFrameHandle frame) const {
    if (!owns_swapchain_frame(frame)) {
        throw std::invalid_argument("rhi swapchain frame handle must belong to this device");
    }
    return swapchain_frame_presented_.at(frame.value - 1U);
}

void NullRhiDevice::set_swapchain_frame_presented(SwapchainFrameHandle frame, bool presented) {
    if (!owns_swapchain_frame(frame)) {
        throw std::invalid_argument("rhi swapchain frame handle must belong to this device");
    }
    swapchain_frame_presented_.at(frame.value - 1U) = presented;
}

void NullRhiDevice::complete_swapchain_frame(SwapchainFrameHandle frame) {
    if (!owns_swapchain_frame(frame)) {
        throw std::invalid_argument("rhi swapchain frame handle must belong to this device");
    }
    const auto frame_index = frame.value - 1U;
    const auto swapchain = swapchain_frame_swapchains_.at(frame_index);
    release_swapchain_reservation(swapchain);
    set_swapchain_state(swapchain, ResourceState::present);
    set_swapchain_presentable(swapchain, false);
    swapchain_frame_active_.at(frame_index) = false;
    swapchain_frame_presented_.at(frame_index) = false;
    ++stats_.swapchain_frames_released;
}

ResourceState NullRhiDevice::swapchain_state(SwapchainHandle swapchain) const {
    if (!owns_swapchain(swapchain)) {
        throw std::invalid_argument("rhi swapchain handle must belong to this device");
    }
    return swapchain_states_.at(swapchain.value - 1U);
}

void NullRhiDevice::set_swapchain_state(SwapchainHandle swapchain, ResourceState state) {
    if (!owns_swapchain(swapchain)) {
        throw std::invalid_argument("rhi swapchain handle must belong to this device");
    }
    swapchain_states_.at(swapchain.value - 1U) = state;
}

bool NullRhiDevice::swapchain_presentable(SwapchainHandle swapchain) const {
    if (!owns_swapchain(swapchain)) {
        throw std::invalid_argument("rhi swapchain handle must belong to this device");
    }
    return swapchain_presentable_.at(swapchain.value - 1U);
}

void NullRhiDevice::set_swapchain_presentable(SwapchainHandle swapchain, bool presentable) {
    if (!owns_swapchain(swapchain)) {
        throw std::invalid_argument("rhi swapchain handle must belong to this device");
    }
    swapchain_presentable_.at(swapchain.value - 1U) = presentable;
}

bool NullRhiDevice::swapchain_frame_reserved(SwapchainHandle swapchain) const {
    if (!owns_swapchain(swapchain)) {
        throw std::invalid_argument("rhi swapchain handle must belong to this device");
    }
    return swapchain_frame_reserved_.at(swapchain.value - 1U);
}

void NullRhiDevice::reserve_swapchain_frame(SwapchainHandle swapchain) {
    if (!owns_swapchain(swapchain)) {
        throw std::invalid_argument("rhi swapchain handle must belong to this device");
    }
    const auto index = swapchain.value - 1U;
    if (swapchain_frame_reserved_.at(index)) {
        throw std::invalid_argument("rhi swapchain already has a pending frame");
    }
    swapchain_frame_reserved_.at(index) = true;
}

void NullRhiDevice::release_swapchain_reservation(SwapchainHandle swapchain) {
    if (!owns_swapchain(swapchain)) {
        throw std::invalid_argument("rhi swapchain handle must belong to this device");
    }
    swapchain_frame_reserved_.at(swapchain.value - 1U) = false;
}

ShaderStage NullRhiDevice::shader_stage(ShaderHandle shader) const {
    return shader_stages_.at(shader.value - 1U);
}

const DescriptorSetLayoutDesc& NullRhiDevice::descriptor_set_layout_desc(DescriptorSetLayoutHandle layout) const {
    return descriptor_set_layouts_.at(layout.value - 1U);
}

DescriptorSetLayoutHandle NullRhiDevice::descriptor_set_layout_for_set(DescriptorSetHandle set) const {
    return descriptor_set_layout_for_sets_.at(set.value - 1U);
}

const PipelineLayoutDesc& NullRhiDevice::pipeline_layout_desc(PipelineLayoutHandle layout) const {
    return pipeline_layouts_.at(layout.value - 1U);
}

PipelineLayoutHandle NullRhiDevice::pipeline_layout_for_pipeline(GraphicsPipelineHandle pipeline) const {
    return graphics_pipeline_layouts_.at(pipeline.value - 1U);
}

PipelineLayoutHandle NullRhiDevice::pipeline_layout_for_compute_pipeline(ComputePipelineHandle pipeline) const {
    return compute_pipeline_layouts_.at(pipeline.value - 1U);
}

Format NullRhiDevice::graphics_pipeline_color_format(GraphicsPipelineHandle pipeline) const {
    return graphics_pipeline_color_formats_.at(pipeline.value - 1U);
}

Format NullRhiDevice::graphics_pipeline_depth_format(GraphicsPipelineHandle pipeline) const {
    return graphics_pipeline_depth_formats_.at(pipeline.value - 1U);
}

BufferHandle NullRhiDevice::create_buffer(const BufferDesc& desc) {
    validate_buffer_desc(desc);
    const auto handle = BufferHandle{next_buffer_++};
    buffers_.push_back(desc);
    buffer_active_.push_back(true);
    buffer_bytes_.emplace_back(static_cast<std::size_t>(desc.size_bytes), std::uint8_t{0});
    const auto lifetime_registration = resource_lifetime_.register_resource(RhiResourceRegistrationDesc{
        .kind = RhiResourceKind::buffer,
        .owner = "null-rhi",
        .debug_name = null_resource_debug_name("buffer", handle.value),
    });
    if (!lifetime_registration.succeeded()) {
        buffers_.pop_back();
        buffer_active_.pop_back();
        buffer_bytes_.pop_back();
        --next_buffer_;
        throw std::logic_error("null rhi device buffer lifetime registration failed");
    }
    buffer_lifetime_.push_back(lifetime_registration.handle);
    ++stats_.buffers_created;
    return handle;
}

TextureHandle NullRhiDevice::create_texture(const TextureDesc& desc) {
    validate_texture_desc(desc);
    const auto handle = TextureHandle{next_texture_++};
    textures_.push_back(desc);
    texture_active_.push_back(true);
    texture_states_.push_back(initial_texture_state(desc.usage));
    const auto lifetime_registration = resource_lifetime_.register_resource(RhiResourceRegistrationDesc{
        .kind = RhiResourceKind::texture,
        .owner = "null-rhi",
        .debug_name = null_resource_debug_name("texture", handle.value),
    });
    if (!lifetime_registration.succeeded()) {
        textures_.pop_back();
        texture_active_.pop_back();
        texture_states_.pop_back();
        --next_texture_;
        throw std::logic_error("null rhi device texture lifetime registration failed");
    }
    texture_lifetime_.push_back(lifetime_registration.handle);
    ++stats_.textures_created;
    return handle;
}

SamplerHandle NullRhiDevice::create_sampler(const SamplerDesc& desc) {
    validate_sampler_desc(desc);
    const auto handle = SamplerHandle{next_sampler_++};
    samplers_.push_back(desc);
    sampler_active_.push_back(true);
    const auto lifetime_registration = resource_lifetime_.register_resource(RhiResourceRegistrationDesc{
        .kind = RhiResourceKind::sampler,
        .owner = "null-rhi",
        .debug_name = null_resource_debug_name("sampler", handle.value),
    });
    if (!lifetime_registration.succeeded()) {
        samplers_.pop_back();
        sampler_active_.pop_back();
        --next_sampler_;
        throw std::logic_error("null rhi device sampler lifetime registration failed");
    }
    sampler_lifetime_.push_back(lifetime_registration.handle);
    ++stats_.samplers_created;
    return handle;
}

SwapchainHandle NullRhiDevice::create_swapchain(const SwapchainDesc& desc) {
    validate_swapchain_desc(desc);
    const auto handle = SwapchainHandle{next_swapchain_++};
    swapchains_.push_back(desc);
    swapchain_states_.push_back(ResourceState::present);
    swapchain_presentable_.push_back(false);
    swapchain_frame_reserved_.push_back(false);
    ++stats_.swapchains_created;
    return handle;
}

void NullRhiDevice::resize_swapchain(SwapchainHandle swapchain, Extent2D extent) {
    if (!owns_swapchain(swapchain)) {
        throw std::invalid_argument("rhi swapchain handle must belong to this device");
    }
    validate_swapchain_extent(extent);
    if (swapchain_state(swapchain) != ResourceState::present || swapchain_presentable(swapchain) ||
        swapchain_frame_reserved(swapchain)) {
        throw std::invalid_argument("rhi swapchain cannot be resized while a frame is pending presentation");
    }

    swapchains_.at(swapchain.value - 1U).extent = extent;
    set_swapchain_state(swapchain, ResourceState::present);
    set_swapchain_presentable(swapchain, false);
    ++stats_.swapchain_resizes;
}

SwapchainFrameHandle NullRhiDevice::acquire_swapchain_frame(SwapchainHandle swapchain) {
    if (!owns_swapchain(swapchain)) {
        throw std::invalid_argument("rhi swapchain handle must belong to this device");
    }
    if (swapchain_state(swapchain) != ResourceState::present || swapchain_presentable(swapchain) ||
        swapchain_frame_reserved(swapchain)) {
        throw std::invalid_argument("rhi swapchain already has a pending frame");
    }

    reserve_swapchain_frame(swapchain);
    const auto frame = SwapchainFrameHandle{next_swapchain_frame_++};
    swapchain_frame_swapchains_.push_back(swapchain);
    swapchain_frame_active_.push_back(true);
    swapchain_frame_presented_.push_back(false);
    ++stats_.swapchain_frames_acquired;
    return frame;
}

void NullRhiDevice::release_swapchain_frame(SwapchainFrameHandle frame) {
    if (!owns_swapchain_frame(frame)) {
        throw std::invalid_argument("rhi swapchain frame handle must belong to this device");
    }
    if (swapchain_frame_presented(frame)) {
        throw std::invalid_argument("rhi swapchain frame cannot be manually released after present recording");
    }
    complete_swapchain_frame(frame);
}

TransientBuffer NullRhiDevice::acquire_transient_buffer(const BufferDesc& desc) {
    const auto buffer = create_buffer(desc);
    const auto lease = TransientResourceHandle{next_transient_resource_++};
    transient_leases_.push_back(TransientLeaseRecord{
        .kind = TransientResourceKind::buffer,
        .buffer = buffer,
        .texture = TextureHandle{},
        .active = true,
    });
    ++stats_.transient_resources_acquired;
    ++stats_.transient_resources_active;
    return TransientBuffer{.lease = lease, .buffer = buffer};
}

TransientTexture NullRhiDevice::acquire_transient_texture(const TextureDesc& desc) {
    const auto texture = create_texture(desc);
    const auto lease = TransientResourceHandle{next_transient_resource_++};
    transient_leases_.push_back(TransientLeaseRecord{
        .kind = TransientResourceKind::texture,
        .buffer = BufferHandle{},
        .texture = texture,
        .active = true,
    });
    ++stats_.transient_resources_acquired;
    ++stats_.transient_resources_active;
    return TransientTexture{.lease = lease, .texture = texture};
}

void NullRhiDevice::release_transient(TransientResourceHandle lease) {
    if (lease.value == 0 || lease.value >= next_transient_resource_) {
        throw std::invalid_argument("rhi transient resource lease must belong to this device");
    }
    auto& record = transient_leases_.at(lease.value - 1U);
    if (!record.active) {
        throw std::invalid_argument("rhi transient resource lease is already released");
    }

    record.active = false;
    if (record.kind == TransientResourceKind::buffer) {
        const auto buffer = record.buffer;
        const auto index = buffer.value - 1U;
        if (buffer_active_.at(index)) {
            (void)resource_lifetime_.release_resource_deferred(buffer_lifetime_.at(index), completed_.value);
            buffer_active_[index] = false;
        }
    } else {
        const auto texture = record.texture;
        const auto index = texture.value - 1U;
        if (texture_active_.at(index)) {
            (void)resource_lifetime_.release_resource_deferred(texture_lifetime_.at(index), completed_.value);
            texture_active_[index] = false;
        }
    }
    retire_resource_lifetime_to_completed_fence();
    ++stats_.transient_resources_released;
    --stats_.transient_resources_active;
}

ShaderHandle NullRhiDevice::create_shader(const ShaderDesc& desc) {
    validate_shader_desc(desc);
    const auto handle = ShaderHandle{next_shader_++};
    shader_stages_.push_back(desc.stage);
    shader_active_.push_back(true);
    const auto lifetime_registration = resource_lifetime_.register_resource(RhiResourceRegistrationDesc{
        .kind = RhiResourceKind::shader,
        .owner = "null-rhi",
        .debug_name = null_resource_debug_name("shader", handle.value),
    });
    if (!lifetime_registration.succeeded()) {
        shader_stages_.pop_back();
        shader_active_.pop_back();
        --next_shader_;
        throw std::logic_error("null rhi device shader lifetime registration failed");
    }
    shader_lifetime_.push_back(lifetime_registration.handle);
    ++stats_.shader_modules_created;
    return handle;
}

DescriptorSetLayoutHandle NullRhiDevice::create_descriptor_set_layout(const DescriptorSetLayoutDesc& desc) {
    validate_descriptor_set_layout_desc(desc);
    const auto handle = DescriptorSetLayoutHandle{next_descriptor_set_layout_++};
    descriptor_set_layouts_.push_back(desc);
    descriptor_set_layout_active_.push_back(true);
    const auto lifetime_registration = resource_lifetime_.register_resource(RhiResourceRegistrationDesc{
        .kind = RhiResourceKind::descriptor_set_layout,
        .owner = "null-rhi",
        .debug_name = null_resource_debug_name("descriptor-set-layout", handle.value),
    });
    if (!lifetime_registration.succeeded()) {
        descriptor_set_layouts_.pop_back();
        descriptor_set_layout_active_.pop_back();
        --next_descriptor_set_layout_;
        throw std::logic_error("null rhi device descriptor set layout lifetime registration failed");
    }
    descriptor_set_layout_lifetime_.push_back(lifetime_registration.handle);
    ++stats_.descriptor_set_layouts_created;
    return handle;
}

DescriptorSetHandle NullRhiDevice::allocate_descriptor_set(DescriptorSetLayoutHandle layout) {
    if (!owns_descriptor_set_layout(layout)) {
        throw std::invalid_argument("rhi descriptor set layout handle must belong to this device");
    }
    const auto handle = DescriptorSetHandle{next_descriptor_set_++};
    descriptor_set_layout_for_sets_.push_back(layout);
    descriptor_set_active_.push_back(true);
    const auto lifetime_registration = resource_lifetime_.register_resource(RhiResourceRegistrationDesc{
        .kind = RhiResourceKind::descriptor_set,
        .owner = "null-rhi",
        .debug_name = null_resource_debug_name("descriptor-set", handle.value),
    });
    if (!lifetime_registration.succeeded()) {
        descriptor_set_layout_for_sets_.pop_back();
        descriptor_set_active_.pop_back();
        --next_descriptor_set_;
        throw std::logic_error("null rhi device descriptor set lifetime registration failed");
    }
    descriptor_set_lifetime_.push_back(lifetime_registration.handle);
    ++stats_.descriptor_sets_allocated;
    return handle;
}

void NullRhiDevice::update_descriptor_set(const DescriptorWrite& write) {
    if (!owns_descriptor_set(write.set)) {
        throw std::invalid_argument("rhi descriptor set handle must belong to this device");
    }
    if (write.resources.empty()) {
        throw std::invalid_argument("rhi descriptor write must contain at least one resource");
    }

    const auto layout_handle = descriptor_set_layout_for_set(write.set);
    const auto& layout = descriptor_set_layout_desc(layout_handle);
    const auto& binding = find_binding(layout, write.binding);
    const auto resource_count = static_cast<std::uint32_t>(write.resources.size());
    if (write.array_element >= binding.count || resource_count > binding.count - write.array_element) {
        throw std::invalid_argument("rhi descriptor write exceeds the declared binding range");
    }

    for (const auto& resource : write.resources) {
        if (resource.type != binding.type) {
            throw std::invalid_argument("rhi descriptor resource type must match the binding type");
        }
        if (is_buffer_descriptor(binding.type)) {
            if (!owns_buffer(resource.buffer_handle)) {
                throw std::invalid_argument("rhi descriptor buffer handle must belong to this device");
            }
            const auto usage = buffer_desc(resource.buffer_handle).usage;
            if (binding.type == DescriptorType::uniform_buffer && !has_flag(usage, BufferUsage::uniform)) {
                throw std::invalid_argument("rhi uniform buffer descriptor requires uniform buffer usage");
            }
            if (binding.type == DescriptorType::storage_buffer && !has_flag(usage, BufferUsage::storage)) {
                throw std::invalid_argument("rhi storage buffer descriptor requires storage buffer usage");
            }
        }
        if (is_texture_descriptor(binding.type)) {
            if (!owns_texture(resource.texture_handle)) {
                throw std::invalid_argument("rhi descriptor texture handle must belong to this device");
            }
            const auto usage = texture_desc(resource.texture_handle).usage;
            if (binding.type == DescriptorType::sampled_texture && !has_flag(usage, TextureUsage::shader_resource)) {
                throw std::invalid_argument("rhi sampled texture descriptor requires shader_resource texture usage");
            }
            if (binding.type == DescriptorType::storage_texture && !has_flag(usage, TextureUsage::storage)) {
                throw std::invalid_argument("rhi storage texture descriptor requires storage texture usage");
            }
        }
        if (is_sampler_descriptor(binding.type) && !owns_sampler(resource.sampler_handle)) {
            throw std::invalid_argument("rhi descriptor sampler handle must belong to this device");
        }
    }

    ++stats_.descriptor_writes;
}

PipelineLayoutHandle NullRhiDevice::create_pipeline_layout(const PipelineLayoutDesc& desc) {
    validate_pipeline_layout_desc(desc);
    for (const auto set_layout : desc.descriptor_sets) {
        if (!owns_descriptor_set_layout(set_layout)) {
            throw std::invalid_argument("rhi pipeline layout descriptor set layout must belong to this device");
        }
    }
    const auto handle = PipelineLayoutHandle{next_pipeline_layout_++};
    pipeline_layouts_.push_back(desc);
    pipeline_layout_active_.push_back(true);
    const auto lifetime_registration = resource_lifetime_.register_resource(RhiResourceRegistrationDesc{
        .kind = RhiResourceKind::pipeline_layout,
        .owner = "null-rhi",
        .debug_name = null_resource_debug_name("pipeline-layout", handle.value),
    });
    if (!lifetime_registration.succeeded()) {
        pipeline_layouts_.pop_back();
        pipeline_layout_active_.pop_back();
        --next_pipeline_layout_;
        throw std::logic_error("null rhi device pipeline layout lifetime registration failed");
    }
    pipeline_layout_lifetime_.push_back(lifetime_registration.handle);
    ++stats_.pipeline_layouts_created;
    return handle;
}

GraphicsPipelineHandle NullRhiDevice::create_graphics_pipeline(const GraphicsPipelineDesc& desc) {
    validate_graphics_pipeline_desc(desc);
    if (!owns_pipeline_layout(desc.layout)) {
        throw std::invalid_argument("rhi graphics pipeline layout must belong to this device");
    }
    if (!owns_shader(desc.vertex_shader) || !owns_shader(desc.fragment_shader)) {
        throw std::invalid_argument("rhi graphics pipeline shaders must belong to this device");
    }
    if (shader_stage(desc.vertex_shader) != ShaderStage::vertex) {
        throw std::invalid_argument("rhi graphics pipeline vertex shader stage must be vertex");
    }
    if (shader_stage(desc.fragment_shader) != ShaderStage::fragment) {
        throw std::invalid_argument("rhi graphics pipeline fragment shader stage must be fragment");
    }
    const auto handle = GraphicsPipelineHandle{next_graphics_pipeline_++};
    graphics_pipeline_layouts_.push_back(desc.layout);
    graphics_pipeline_color_formats_.push_back(desc.color_format);
    graphics_pipeline_depth_formats_.push_back(desc.depth_format);
    graphics_pipeline_active_.push_back(true);
    const auto lifetime_registration = resource_lifetime_.register_resource(RhiResourceRegistrationDesc{
        .kind = RhiResourceKind::graphics_pipeline,
        .owner = "null-rhi",
        .debug_name = null_resource_debug_name("graphics-pipeline", handle.value),
    });
    if (!lifetime_registration.succeeded()) {
        graphics_pipeline_layouts_.pop_back();
        graphics_pipeline_color_formats_.pop_back();
        graphics_pipeline_depth_formats_.pop_back();
        graphics_pipeline_active_.pop_back();
        --next_graphics_pipeline_;
        throw std::logic_error("null rhi device graphics pipeline lifetime registration failed");
    }
    graphics_pipeline_lifetime_.push_back(lifetime_registration.handle);
    ++stats_.graphics_pipelines_created;
    return handle;
}

ComputePipelineHandle NullRhiDevice::create_compute_pipeline(const ComputePipelineDesc& desc) {
    validate_compute_pipeline_desc(desc);
    if (!owns_pipeline_layout(desc.layout)) {
        throw std::invalid_argument("rhi compute pipeline layout must belong to this device");
    }
    if (!owns_shader(desc.compute_shader)) {
        throw std::invalid_argument("rhi compute pipeline shader must belong to this device");
    }
    if (shader_stage(desc.compute_shader) != ShaderStage::compute) {
        throw std::invalid_argument("rhi compute pipeline shader stage must be compute");
    }

    const auto handle = ComputePipelineHandle{next_compute_pipeline_++};
    compute_pipeline_layouts_.push_back(desc.layout);
    compute_pipeline_active_.push_back(true);
    const auto lifetime_registration = resource_lifetime_.register_resource(RhiResourceRegistrationDesc{
        .kind = RhiResourceKind::compute_pipeline,
        .owner = "null-rhi",
        .debug_name = null_resource_debug_name("compute-pipeline", handle.value),
    });
    if (!lifetime_registration.succeeded()) {
        compute_pipeline_layouts_.pop_back();
        compute_pipeline_active_.pop_back();
        --next_compute_pipeline_;
        throw std::logic_error("null rhi device compute pipeline lifetime registration failed");
    }
    compute_pipeline_lifetime_.push_back(lifetime_registration.handle);
    ++stats_.compute_pipelines_created;
    return handle;
}

std::unique_ptr<IRhiCommandList> NullRhiDevice::begin_command_list(QueueKind queue) {
    ++stats_.command_lists_begun;
    return std::unique_ptr<IRhiCommandList>(new NullRhiCommandList(*this, queue));
}

FenceValue NullRhiDevice::submit(IRhiCommandList& commands) {
    auto* null_commands = dynamic_cast<NullRhiCommandList*>(&commands);
    if (null_commands == nullptr) {
        throw std::invalid_argument("rhi command list must belong to this device");
    }
    if (!commands.closed()) {
        throw std::logic_error("rhi command list must be closed before submit");
    }

    null_commands->mark_submitted();
    ++stats_.command_lists_submitted;
    ++stats_.fences_signaled;
    const auto queue = commands.queue_kind();
    auto& last_signaled_for_queue = last_signaled_by_queue_.at(queue_index(queue));
    last_signaled_for_queue.queue = queue;
    last_signaled_for_queue.value += 1;
    last_signaled_ = last_signaled_for_queue;
    completed_by_queue_.at(queue_index(queue)) = last_signaled_for_queue;
    completed_ = last_signaled_for_queue;
    stats_.last_submitted_fence_value = last_signaled_.value;
    stats_.last_completed_fence_value = completed_.value;
    record_queue_submit(stats_, queue, last_signaled_);
    retire_resource_lifetime_to_completed_fence();
    return last_signaled_;
}

void NullRhiDevice::wait(FenceValue fence) {
    ++stats_.fence_waits;
    if (!valid_queue_kind(fence.queue) || fence.value > last_signaled_by_queue_.at(queue_index(fence.queue)).value) {
        ++stats_.fence_wait_failures;
        throw std::invalid_argument("rhi fence was not submitted by this device");
    }
    completed_by_queue_.at(queue_index(fence.queue)) = fence;
    completed_ = fence;
    stats_.last_completed_fence_value = completed_.value;
    retire_resource_lifetime_to_completed_fence();
}

void NullRhiDevice::wait_for_queue(QueueKind queue, FenceValue fence) {
    ++stats_.queue_waits;
    if (!valid_queue_kind(queue) || !valid_queue_kind(fence.queue) ||
        fence.value > last_signaled_by_queue_.at(queue_index(fence.queue)).value) {
        ++stats_.queue_wait_failures;
        throw std::invalid_argument("rhi queue wait fence was not submitted by this device");
    }
    record_queue_wait(stats_, queue, fence);
}

void NullRhiDevice::write_buffer(BufferHandle buffer, std::uint64_t offset, std::span<const std::uint8_t> bytes) {
    if (!owns_buffer(buffer)) {
        throw std::invalid_argument("rhi write buffer handle must belong to this device");
    }
    if (bytes.empty()) {
        throw std::invalid_argument("rhi write buffer byte span must be non-empty");
    }

    const auto& desc = buffer_desc(buffer);
    if (!has_flag(desc.usage, BufferUsage::copy_source)) {
        throw std::invalid_argument("rhi write buffer requires a copy_source upload buffer");
    }
    if (offset > desc.size_bytes || bytes.size() > desc.size_bytes - offset) {
        throw std::invalid_argument("rhi write buffer range is outside the buffer");
    }

    auto& storage = buffer_bytes_.at(buffer.value - 1U);
    std::ranges::copy(bytes, storage.begin() + checked_buffer_byte_difference(offset));
    ++stats_.buffer_writes;
    stats_.bytes_written += static_cast<std::uint64_t>(bytes.size());
}

std::vector<std::uint8_t> NullRhiDevice::read_buffer(BufferHandle buffer, std::uint64_t offset,
                                                     std::uint64_t size_bytes) {
    if (!owns_buffer(buffer)) {
        throw std::invalid_argument("rhi read buffer handle must belong to this device");
    }
    if (size_bytes == 0) {
        throw std::invalid_argument("rhi read buffer size must be non-zero");
    }

    const auto& desc = buffer_desc(buffer);
    if (!has_flag(desc.usage, BufferUsage::copy_destination)) {
        throw std::invalid_argument("rhi read buffer requires copy_destination usage");
    }
    if (offset > desc.size_bytes || size_bytes > desc.size_bytes - offset) {
        throw std::invalid_argument("rhi read buffer range is outside the buffer");
    }

    ++stats_.buffer_reads;
    stats_.bytes_read += size_bytes;
    const auto& storage = buffer_bytes_.at(buffer.value - 1U);
    const auto begin = storage.begin() + checked_buffer_byte_difference(offset);
    return std::vector<std::uint8_t>(begin, begin + checked_buffer_byte_difference(size_bytes));
}

bool NullRhiDevice::null_mark_buffer_released(BufferHandle buffer) noexcept {
    if (buffer.value == 0 || buffer.value >= next_buffer_) {
        return false;
    }
    const auto index = buffer.value - 1U;
    if (!buffer_active_.at(index)) {
        return false;
    }
    (void)resource_lifetime_.release_resource_deferred(buffer_lifetime_.at(index), completed_.value);
    buffer_active_[index] = false;
    retire_resource_lifetime_to_completed_fence();
    return true;
}

bool NullRhiDevice::null_mark_texture_released(TextureHandle texture) noexcept {
    if (texture.value == 0 || texture.value >= next_texture_) {
        return false;
    }
    const auto index = texture.value - 1U;
    if (!texture_active_.at(index)) {
        return false;
    }
    (void)resource_lifetime_.release_resource_deferred(texture_lifetime_.at(index), completed_.value);
    texture_active_[index] = false;
    retire_resource_lifetime_to_completed_fence();
    return true;
}

bool NullRhiDevice::null_mark_sampler_released(SamplerHandle sampler) noexcept {
    if (sampler.value == 0 || sampler.value >= next_sampler_) {
        return false;
    }
    const auto index = sampler.value - 1U;
    if (!sampler_active_.at(index)) {
        return false;
    }
    (void)resource_lifetime_.release_resource_deferred(sampler_lifetime_.at(index), completed_.value);
    sampler_active_[index] = false;
    retire_resource_lifetime_to_completed_fence();
    return true;
}

bool NullRhiDevice::null_mark_descriptor_set_released(DescriptorSetHandle set) noexcept {
    if (set.value == 0 || set.value >= next_descriptor_set_) {
        return false;
    }
    const auto index = set.value - 1U;
    if (!descriptor_set_active_.at(index)) {
        return false;
    }
    (void)resource_lifetime_.release_resource_deferred(descriptor_set_lifetime_.at(index), completed_.value);
    descriptor_set_active_[index] = false;
    retire_resource_lifetime_to_completed_fence();
    return true;
}

bool NullRhiDevice::null_mark_descriptor_set_layout_released(DescriptorSetLayoutHandle layout) noexcept {
    if (layout.value == 0 || layout.value >= next_descriptor_set_layout_) {
        return false;
    }
    const auto index = layout.value - 1U;
    if (!descriptor_set_layout_active_.at(index)) {
        return false;
    }
    (void)resource_lifetime_.release_resource_deferred(descriptor_set_layout_lifetime_.at(index), completed_.value);
    descriptor_set_layout_active_[index] = false;
    retire_resource_lifetime_to_completed_fence();
    return true;
}

bool NullRhiDevice::null_mark_pipeline_layout_released(PipelineLayoutHandle layout) noexcept {
    if (layout.value == 0 || layout.value >= next_pipeline_layout_) {
        return false;
    }
    const auto index = layout.value - 1U;
    if (!pipeline_layout_active_.at(index)) {
        return false;
    }
    (void)resource_lifetime_.release_resource_deferred(pipeline_layout_lifetime_.at(index), completed_.value);
    pipeline_layout_active_[index] = false;
    retire_resource_lifetime_to_completed_fence();
    return true;
}

bool NullRhiDevice::null_mark_shader_released(ShaderHandle shader) noexcept {
    if (shader.value == 0 || shader.value >= next_shader_) {
        return false;
    }
    const auto index = shader.value - 1U;
    if (!shader_active_.at(index)) {
        return false;
    }
    (void)resource_lifetime_.release_resource_deferred(shader_lifetime_.at(index), completed_.value);
    shader_active_[index] = false;
    retire_resource_lifetime_to_completed_fence();
    return true;
}

bool NullRhiDevice::null_mark_graphics_pipeline_released(GraphicsPipelineHandle pipeline) noexcept {
    if (pipeline.value == 0 || pipeline.value >= next_graphics_pipeline_) {
        return false;
    }
    const auto index = pipeline.value - 1U;
    if (!graphics_pipeline_active_.at(index)) {
        return false;
    }
    (void)resource_lifetime_.release_resource_deferred(graphics_pipeline_lifetime_.at(index), completed_.value);
    graphics_pipeline_active_[index] = false;
    retire_resource_lifetime_to_completed_fence();
    return true;
}

bool NullRhiDevice::null_mark_compute_pipeline_released(ComputePipelineHandle pipeline) noexcept {
    if (pipeline.value == 0 || pipeline.value >= next_compute_pipeline_) {
        return false;
    }
    const auto index = pipeline.value - 1U;
    if (!compute_pipeline_active_.at(index)) {
        return false;
    }
    (void)resource_lifetime_.release_resource_deferred(compute_pipeline_lifetime_.at(index), completed_.value);
    compute_pipeline_active_[index] = false;
    retire_resource_lifetime_to_completed_fence();
    return true;
}

} // namespace mirakana::rhi
