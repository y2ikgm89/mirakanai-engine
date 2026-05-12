// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "rhi_native_ui_overlay.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>

namespace mirakana {
namespace {

struct OverlayVertex {
    float x{0.0F};
    float y{0.0F};
    float r{1.0F};
    float g{1.0F};
    float b{1.0F};
    float a{1.0F};
    float u{0.0F};
    float v{0.0F};
    float use_texture{0.0F};
    float reserved{0.0F};
};

struct OverlaySpriteAppendResult {
    bool appended{false};
    bool textured{false};
};

[[nodiscard]] constexpr std::uint32_t overlay_atlas_texture_binding() noexcept {
    return 0;
}

[[nodiscard]] constexpr std::uint32_t overlay_atlas_sampler_binding() noexcept {
    return 1;
}

[[nodiscard]] bool is_positive_finite(float value) noexcept {
    return std::isfinite(value) && value > 0.0F;
}

[[nodiscard]] bool valid_uv_rect(SpriteUvRect rect) noexcept {
    return std::isfinite(rect.u0) && std::isfinite(rect.v0) && std::isfinite(rect.u1) && std::isfinite(rect.v1) &&
           rect.u0 >= 0.0F && rect.v0 >= 0.0F && rect.u1 <= 1.0F && rect.v1 <= 1.0F && rect.u0 < rect.u1 &&
           rect.v0 < rect.v1;
}

[[nodiscard]] float to_ndc_x(float pixel_x, std::uint32_t width) noexcept {
    return ((pixel_x / static_cast<float>(width)) * 2.0F) - 1.0F;
}

[[nodiscard]] float to_ndc_y(float pixel_y, std::uint32_t height) noexcept {
    return 1.0F - ((pixel_y / static_cast<float>(height)) * 2.0F);
}

[[nodiscard]] OverlaySpriteAppendResult append_sprite_vertices(std::vector<OverlayVertex>& vertices,
                                                               const SpriteCommand& sprite, Extent2D extent,
                                                               AssetId atlas_page, bool atlas_ready) {
    const auto width = sprite.transform.scale.x;
    const auto height = sprite.transform.scale.y;
    if (!is_positive_finite(width) || !is_positive_finite(height) || !std::isfinite(sprite.transform.position.x) ||
        !std::isfinite(sprite.transform.position.y)) {
        return {};
    }

    const auto half_width = width * 0.5F;
    const auto half_height = height * 0.5F;
    const auto left = sprite.transform.position.x - half_width;
    const auto right = sprite.transform.position.x + half_width;
    const auto top = sprite.transform.position.y - half_height;
    const auto bottom = sprite.transform.position.y + half_height;

    const auto x0 = to_ndc_x(left, extent.width);
    const auto x1 = to_ndc_x(right, extent.width);
    const auto y0 = to_ndc_y(top, extent.height);
    const auto y1 = to_ndc_y(bottom, extent.height);
    const auto color = std::array<float, 4>{sprite.color.r, sprite.color.g, sprite.color.b, sprite.color.a};
    const bool use_texture = sprite.texture.enabled && atlas_ready && sprite.texture.atlas_page == atlas_page &&
                             valid_uv_rect(sprite.texture.uv_rect);
    const auto uv = use_texture ? sprite.texture.uv_rect : SpriteUvRect{};
    const auto texture_flag = use_texture ? 1.0F : 0.0F;
    const auto make_vertex = [&color, texture_flag](float x, float y, float u, float v) {
        return OverlayVertex{.x = x,
                             .y = y,
                             .r = color[0],
                             .g = color[1],
                             .b = color[2],
                             .a = color[3],
                             .u = u,
                             .v = v,
                             .use_texture = texture_flag,
                             .reserved = 0.0F};
    };

    vertices.push_back(make_vertex(x0, y0, uv.u0, uv.v0));
    vertices.push_back(make_vertex(x1, y0, uv.u1, uv.v0));
    vertices.push_back(make_vertex(x0, y1, uv.u0, uv.v1));
    vertices.push_back(make_vertex(x0, y1, uv.u0, uv.v1));
    vertices.push_back(make_vertex(x1, y0, uv.u1, uv.v0));
    vertices.push_back(make_vertex(x1, y1, uv.u1, uv.v1));
    return OverlaySpriteAppendResult{.appended = true, .textured = use_texture};
}

[[nodiscard]] SpriteCommand make_planned_executed_sprite(const SpriteCommand& sprite, bool textured,
                                                         AssetId atlas_page) {
    auto planned = sprite;
    if (textured) {
        planned.texture.enabled = true;
        planned.texture.atlas_page = atlas_page;
    } else {
        planned.texture = {};
    }
    return planned;
}

[[nodiscard]] std::uint64_t next_capacity(std::uint64_t required) {
    std::uint64_t capacity = 4096;
    while (capacity < required) {
        if (capacity > (std::numeric_limits<std::uint64_t>::max)() / 2U) {
            throw std::overflow_error("native ui overlay vertex buffer capacity overflowed");
        }
        capacity *= 2U;
    }
    return capacity;
}

} // namespace

RhiNativeUiOverlay::RhiNativeUiOverlay(const RhiNativeUiOverlayDesc& desc)
    : device_(desc.device), extent_(desc.extent), color_format_(desc.color_format), atlas_page_(desc.atlas.atlas_page),
      texture_sampling_enabled_(desc.enable_textures) {
    if (device_ == nullptr) {
        throw std::invalid_argument("native ui overlay requires an rhi device");
    }
    if (extent_.width == 0 || extent_.height == 0) {
        throw std::invalid_argument("native ui overlay extent must be non-zero");
    }
    if (color_format_ == rhi::Format::unknown || color_format_ == rhi::Format::depth24_stencil8) {
        throw std::invalid_argument("native ui overlay requires a color format");
    }
    if (desc.vertex_shader.value == 0 || desc.fragment_shader.value == 0) {
        throw std::invalid_argument("native ui overlay requires vertex and fragment shaders");
    }
    if (texture_sampling_enabled_) {
        if (desc.atlas.atlas_page.value == 0 || desc.atlas.texture.value == 0 || desc.atlas.sampler.value == 0) {
            throw std::invalid_argument("native ui overlay textured sprites require an atlas page texture and sampler");
        }
        if (desc.atlas.owner_device == nullptr) {
            throw std::invalid_argument("native ui overlay atlas binding requires an owner rhi device");
        }
        if (desc.atlas.owner_device != device_) {
            throw std::invalid_argument("native ui overlay atlas binding uses a different rhi device");
        }
        atlas_texture_ = desc.atlas.texture;
        atlas_sampler_ = desc.atlas.sampler;
    } else {
        atlas_page_ = {};
        fallback_texture_ = device_->create_texture(rhi::TextureDesc{
            .extent = rhi::Extent3D{.width = 1, .height = 1, .depth = 1},
            .format = rhi::Format::rgba8_unorm,
            .usage = rhi::TextureUsage::shader_resource,
        });
        fallback_sampler_ = device_->create_sampler(rhi::SamplerDesc{});
        atlas_texture_ = fallback_texture_;
        atlas_sampler_ = fallback_sampler_;
    }

    atlas_descriptor_set_layout_ = device_->create_descriptor_set_layout(rhi::DescriptorSetLayoutDesc{{
        rhi::DescriptorBindingDesc{
            .binding = overlay_atlas_texture_binding(),
            .type = rhi::DescriptorType::sampled_texture,
            .count = 1,
            .stages = rhi::ShaderStageVisibility::fragment,
        },
        rhi::DescriptorBindingDesc{
            .binding = overlay_atlas_sampler_binding(),
            .type = rhi::DescriptorType::sampler,
            .count = 1,
            .stages = rhi::ShaderStageVisibility::fragment,
        },
    }});
    atlas_descriptor_set_ = device_->allocate_descriptor_set(atlas_descriptor_set_layout_);
    device_->update_descriptor_set(rhi::DescriptorWrite{
        .set = atlas_descriptor_set_,
        .binding = overlay_atlas_texture_binding(),
        .array_element = 0,
        .resources = {rhi::DescriptorResource::texture(rhi::DescriptorType::sampled_texture, atlas_texture_)},
    });
    device_->update_descriptor_set(rhi::DescriptorWrite{
        .set = atlas_descriptor_set_,
        .binding = overlay_atlas_sampler_binding(),
        .array_element = 0,
        .resources = {rhi::DescriptorResource::sampler(atlas_sampler_)},
    });

    pipeline_layout_ = device_->create_pipeline_layout(
        rhi::PipelineLayoutDesc{.descriptor_sets = {atlas_descriptor_set_layout_}, .push_constant_bytes = 0});
    pipeline_ = device_
                    ->create_graphics_pipeline(
                        rhi::GraphicsPipelineDesc{
                            .layout = pipeline_layout_,
                            .vertex_shader = desc.vertex_shader,
                            .fragment_shader = desc.fragment_shader,
                            .color_format = color_format_,
                            .depth_format = rhi::Format::unknown,
                            .topology = rhi::PrimitiveTopology::triangle_list,
                            .vertex_buffers = {rhi::VertexBufferLayoutDesc{.binding = 0,
                                                                           .stride = sizeof(OverlayVertex),
                                                                           .input_rate = rhi::VertexInputRate::vertex}},
                            .vertex_attributes =
                                {
                                    rhi::VertexAttributeDesc{.location = 0,
                                                             .binding = 0,
                                                             .offset = 0,
                                                             .format = rhi::VertexFormat::float32x2,
                                                             .semantic = rhi::VertexSemantic::position,
                                                             .semantic_index = 0},
                                    rhi::VertexAttributeDesc{.location = 1,
                                                             .binding = 0,
                                                             .offset = 8,
                                                             .format = rhi::VertexFormat::float32x4,
                                                             .semantic = rhi::VertexSemantic::color,
                                                             .semantic_index = 0},
                                    rhi::VertexAttributeDesc{.location = 2,
                                                             .binding = 0,
                                                             .offset = 24,
                                                             .format = rhi::VertexFormat::float32x2,
                                                             .semantic = rhi::VertexSemantic::texcoord,
                                                             .semantic_index = 0},
                                    rhi::
                                        VertexAttributeDesc{.location = 3,
                                                            .binding = 0,
                                                            .offset = 32,
                                                            .format = rhi::VertexFormat::float32x2,
                                                            .semantic = rhi::VertexSemantic::custom,
                                                            .semantic_index = 1},
                                },
                        });
}

bool RhiNativeUiOverlay::ready() const noexcept {
    return device_ != nullptr && atlas_descriptor_set_layout_.value != 0 && atlas_descriptor_set_.value != 0 &&
           atlas_texture_.value != 0 && atlas_sampler_.value != 0 && pipeline_layout_.value != 0 &&
           pipeline_.value != 0;
}

bool RhiNativeUiOverlay::atlas_ready() const noexcept {
    return texture_sampling_enabled_ && atlas_page_.value != 0 && atlas_texture_.value != 0 &&
           atlas_sampler_.value != 0 && atlas_descriptor_set_.value != 0;
}

void RhiNativeUiOverlay::resize(Extent2D extent) noexcept {
    if (extent.width != 0 && extent.height != 0) {
        extent_ = extent;
    }
}

RhiNativeUiOverlayPreparedDraw RhiNativeUiOverlay::prepare(std::span<const SpriteCommand> sprites,
                                                           rhi::IRhiCommandList& /*unused*/) {
    if (sprites.empty()) {
        return {};
    }

    std::vector<OverlayVertex> vertices;
    vertices.reserve(sprites.size() * 6U);
    std::vector<SpriteCommand> planned_sprites;
    planned_sprites.reserve(sprites.size());
    for (const auto& sprite : sprites) {
        const auto append_result = append_sprite_vertices(vertices, sprite, extent_, atlas_page_, atlas_ready());
        if (append_result.appended) {
            planned_sprites.push_back(make_planned_executed_sprite(sprite, append_result.textured, atlas_page_));
        }
    }
    if (vertices.empty()) {
        return {};
    }

    const auto required_bytes = static_cast<std::uint64_t>(vertices.size()) * sizeof(OverlayVertex);
    ensure_vertex_capacity(required_bytes);

    const auto bytes = std::span<const std::uint8_t>{
        reinterpret_cast<const std::uint8_t*>(vertices.data()),
        static_cast<std::size_t>(required_bytes),
    };
    device_->write_buffer(vertex_buffer_, 0, bytes);

    auto plan = plan_sprite_batches(planned_sprites);
    const auto textured_batch_count =
        static_cast<std::uint64_t>(std::count_if(plan.batches.begin(), plan.batches.end(), [](const auto& batch) {
            return batch.kind == SpriteBatchKind::textured;
        }));

    return RhiNativeUiOverlayPreparedDraw{
        .vertex_count = static_cast<std::uint32_t>(vertices.size()),
        .sprite_count = plan.sprite_count,
        .textured_sprite_count = plan.textured_sprite_count,
        .texture_bind_count = plan.texture_bind_count,
        .batch_count = static_cast<std::uint64_t>(plan.batches.size()),
        .textured_batch_count = textured_batch_count,
        .batches = std::move(plan.batches),
    };
}

void RhiNativeUiOverlay::record_draw(const RhiNativeUiOverlayPreparedDraw& prepared, rhi::IRhiCommandList& commands) {
    if (prepared.vertex_count == 0) {
        return;
    }

    commands.bind_graphics_pipeline(pipeline_);
    commands.bind_descriptor_set(pipeline_layout_, 0, atlas_descriptor_set_);
    for (const auto& batch : prepared.batches) {
        if (batch.sprite_count == 0) {
            continue;
        }
        const auto vertex_offset = batch.first_sprite * 6U * sizeof(OverlayVertex);
        const auto vertex_count = static_cast<std::uint32_t>(batch.sprite_count * 6U);
        commands.bind_vertex_buffer(rhi::VertexBufferBinding{
            .buffer = vertex_buffer_, .offset = vertex_offset, .stride = sizeof(OverlayVertex), .binding = 0});
        commands.draw(vertex_count, 1);
    }
}

void RhiNativeUiOverlay::ensure_vertex_capacity(std::uint64_t required_bytes) {
    if (required_bytes == 0 || vertex_capacity_bytes_ >= required_bytes) {
        return;
    }

    vertex_capacity_bytes_ = next_capacity(required_bytes);
    vertex_buffer_ = device_->create_buffer(rhi::BufferDesc{
        .size_bytes = vertex_capacity_bytes_, .usage = rhi::BufferUsage::vertex | rhi::BufferUsage::copy_source});
}

} // namespace mirakana
