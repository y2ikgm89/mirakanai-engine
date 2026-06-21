// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime/resource_runtime.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace mirakana::runtime {

enum class RuntimeSpriteAtlasResidencyStatus : std::uint8_t {
    ready = 0,
    invalid_request,
    diagnostics,
    budget_exceeded,
};

enum class RuntimeSpriteAtlasResidencyDiagnosticCode : std::uint8_t {
    missing_atlas_id = 0,
    missing_package,
    missing_catalog,
    missing_page,
    missing_page_id,
    duplicate_page_id,
    missing_texture_asset,
    missing_cooked_texture_payload,
    invalid_cooked_texture_payload,
    stale_resource_handle,
    invalid_page_dimensions,
    invalid_uv_rect,
    invalid_padding,
    resident_byte_budget_exceeded,
    runtime_source_decode_requested,
    renderer_residency_ownership_requested,
    public_native_handle_requested,
};

struct RuntimeSpriteAtlasPageRow {
    std::string page_id;
    AssetId texture_asset;
    RuntimeResourceHandleV2 texture_resource;
    std::string texture_uri;
    std::uint32_t width{0U};
    std::uint32_t height{0U};
    std::uint32_t padding_pixels{0U};
    float u0{0.0F};
    float v0{0.0F};
    float u1{1.0F};
    float v1{1.0F};
    std::uint64_t resident_bytes{0U};
};

struct RuntimeSpriteAtlasResidencyRequest {
    std::string atlas_id;
    const RuntimeAssetPackage* package{nullptr};
    const RuntimeResourceCatalogV2* catalog{nullptr};
    std::vector<RuntimeSpriteAtlasPageRow> pages;
    std::optional<std::uint64_t> resident_byte_budget;
    bool request_runtime_source_decode{false};
    bool request_renderer_residency_ownership{false};
    bool request_public_native_handle{false};
};

struct RuntimeSpriteAtlasResidencyUploadHandoffRow {
    std::string handoff_id;
    std::string page_id;
    AssetId texture_asset;
    RuntimeResourceHandleV2 texture_resource;
    std::string texture_uri;
    std::uint32_t width{0U};
    std::uint32_t height{0U};
    std::uint32_t padding_pixels{0U};
    float u0{0.0F};
    float v0{0.0F};
    float u1{1.0F};
    float v1{1.0F};
    std::uint64_t resident_bytes{0U};
};

struct RuntimeSpriteAtlasResidencyDiagnostic {
    RuntimeSpriteAtlasResidencyDiagnosticCode code{RuntimeSpriteAtlasResidencyDiagnosticCode::missing_atlas_id};
    std::string atlas_id;
    std::string page_id;
    AssetId texture_asset;
    std::string message;
};

struct RuntimeSpriteAtlasResidencyPlan {
    RuntimeSpriteAtlasResidencyStatus status{RuntimeSpriteAtlasResidencyStatus::invalid_request};
    std::vector<RuntimeSpriteAtlasPageRow> page_rows;
    std::vector<RuntimeSpriteAtlasResidencyUploadHandoffRow> upload_handoff_rows;
    std::vector<RuntimeSpriteAtlasResidencyDiagnostic> diagnostics;
    std::uint64_t resident_bytes{0U};
    std::size_t page_count{0U};
    std::size_t upload_handoff_count{0U};
    bool invoked_runtime_source_decode{false};
    bool requested_renderer_residency_ownership{false};
    bool requested_public_native_handle{false};
    bool invoked_renderer_upload{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Plans cooked 2D sprite atlas page residency from caller-reviewed runtime package rows.
/// The plan is value-only: it does not decode source images, upload renderer resources, own renderer/RHI residency,
/// mutate packages, create threads, or expose native handles. Upload handoff rows are descriptors for the host-owned
/// renderer/RHI boundary to consume later.
[[nodiscard]] RuntimeSpriteAtlasResidencyPlan
plan_runtime_sprite_atlas_residency(const RuntimeSpriteAtlasResidencyRequest& request);

} // namespace mirakana::runtime
