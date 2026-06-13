// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/asset_source_format.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

[[nodiscard]] mirakana::TextureSourceDocumentV2 make_openexr_source() {
    return mirakana::TextureSourceDocumentV2{
        .source_path = "source/textures/environment/studio.exr",
        .source_hash = "sha256:00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff",
        .provenance_id = "provenance.environment.studio",
        .license_id = "LicenseRef-Proprietary",
        .source_kind = mirakana::TextureSourceKindV2::openexr,
        .width = 2048,
        .height = 1024,
        .color_space = mirakana::TextureColorSpaceV2::scene_linear,
        .sampler_class = mirakana::TextureSamplerClassV2::environment_radiance,
        .openexr =
            mirakana::TextureOpenExrSourceReviewV2{
                .data_window = mirakana::TextureSourceWindowV2{.min_x = 0, .min_y = 0, .max_x = 2047, .max_y = 1023},
                .display_window = mirakana::TextureSourceWindowV2{.min_x = 0, .min_y = 0, .max_x = 2047, .max_y = 1023},
                .channel_list = "R:half,G:half,B:half",
                .pixel_encoding = mirakana::TexturePixelEncodingV2::float16,
                .channel_count = 3,
                .chromaticities_recorded = true,
                .scene_linear_intent = true,
                .multipart = false,
                .deep = false,
                .tiled = false,
            },
        .ktx2_basis = {},
    };
}

[[nodiscard]] mirakana::TextureSourceDocumentV2 make_ktx2_basis_source() {
    return mirakana::TextureSourceDocumentV2{
        .source_path = "source/textures/environment/skybox.ktx2",
        .source_hash = "sha256:ffeeddccbbaa99887766554433221100ffeeddccbbaa99887766554433221100",
        .provenance_id = "provenance.environment.skybox",
        .license_id = "LicenseRef-Proprietary",
        .source_kind = mirakana::TextureSourceKindV2::ktx2_basis,
        .width = 1024,
        .height = 1024,
        .color_space = mirakana::TextureColorSpaceV2::srgb,
        .sampler_class = mirakana::TextureSamplerClassV2::color,
        .openexr = {},
        .ktx2_basis =
            mirakana::TextureKtx2BasisSourceReviewV2{
                .vk_format = "VK_FORMAT_UNDEFINED",
                .level_count = 11,
                .layer_count = 1,
                .face_count = 6,
                .supercompression = mirakana::TextureCompressionKindV2::basis_lz,
                .basis_codec = mirakana::TexturePixelEncodingV2::basis_uastc,
                .requires_transcoding = true,
            },
    };
}

[[nodiscard]] std::vector<mirakana::TextureCookBackendDecisionV1> make_backend_decisions() {
    return {
        mirakana::TextureCookBackendDecisionV1{
            .backend = mirakana::TextureCookBackendV1::d3d12,
            .device_format = "DXGI_FORMAT_BC6H_UF16",
            .compression = mirakana::TextureCompressionKindV2::bc6h,
            .transcode = mirakana::TextureCookTranscodeKindV1::offline_policy,
            .estimated_gpu_bytes = 1'048'576,
            .supported = true,
            .host_validated = true,
            .diagnostic = {},
        },
        mirakana::TextureCookBackendDecisionV1{
            .backend = mirakana::TextureCookBackendV1::vulkan,
            .device_format = "VK_FORMAT_BC6H_UFLOAT_BLOCK",
            .compression = mirakana::TextureCompressionKindV2::bc6h,
            .transcode = mirakana::TextureCookTranscodeKindV1::offline_policy,
            .estimated_gpu_bytes = 1'048'576,
            .supported = true,
            .host_validated = false,
            .diagnostic = "host-gated:vulkan-format-feature-query",
        },
        mirakana::TextureCookBackendDecisionV1{
            .backend = mirakana::TextureCookBackendV1::metal_macos,
            .device_format = "MTLPixelFormatRGBA16Float",
            .compression = mirakana::TextureCompressionKindV2::none,
            .transcode = mirakana::TextureCookTranscodeKindV1::not_required,
            .estimated_gpu_bytes = 16'777'216,
            .supported = true,
            .host_validated = false,
            .diagnostic = "host-gated:apple-metal-capability-check",
        },
        mirakana::TextureCookBackendDecisionV1{
            .backend = mirakana::TextureCookBackendV1::vulkan_android,
            .device_format = "VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK",
            .compression = mirakana::TextureCompressionKindV2::astc_4x4,
            .transcode = mirakana::TextureCookTranscodeKindV1::basis_transcode_policy,
            .estimated_gpu_bytes = 1'048'576,
            .supported = true,
            .host_validated = false,
            .diagnostic = "host-gated:android-vulkan-astc-format-query",
        },
        mirakana::TextureCookBackendDecisionV1{
            .backend = mirakana::TextureCookBackendV1::metal_ios,
            .device_format = "MTLPixelFormatASTC_4x4_HDR",
            .compression = mirakana::TextureCompressionKindV2::astc_4x4,
            .transcode = mirakana::TextureCookTranscodeKindV1::basis_transcode_policy,
            .estimated_gpu_bytes = 1'048'576,
            .supported = true,
            .host_validated = false,
            .diagnostic = "host-gated:ios-metal-astc-capability-check",
        },
    };
}

} // namespace

MK_TEST("texture source v2 serializes openexr review metadata deterministically") {
    const auto source = make_openexr_source();

    MK_REQUIRE(mirakana::is_valid_texture_source_document_v2(source));
    const auto text = mirakana::serialize_texture_source_document_v2(source);

    MK_REQUIRE(text == "format=GameEngine.TextureSource.v2\n"
                       "texture.source_path=source/textures/environment/studio.exr\n"
                       "texture.source_hash=sha256:00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff\n"
                       "texture.provenance_id=provenance.environment.studio\n"
                       "texture.license_id=LicenseRef-Proprietary\n"
                       "texture.source_kind=openexr\n"
                       "texture.width=2048\n"
                       "texture.height=1024\n"
                       "texture.color_space=scene_linear\n"
                       "texture.sampler_class=environment_radiance\n"
                       "openexr.data_window=0,0,2047,1023\n"
                       "openexr.display_window=0,0,2047,1023\n"
                       "openexr.channel_list=R:half,G:half,B:half\n"
                       "openexr.pixel_encoding=float16\n"
                       "openexr.channel_count=3\n"
                       "openexr.chromaticities_recorded=true\n"
                       "openexr.scene_linear_intent=true\n"
                       "openexr.multipart=false\n"
                       "openexr.deep=false\n"
                       "openexr.tiled=false\n");

    const auto round_trip = mirakana::deserialize_texture_source_document_v2(text);
    MK_REQUIRE(round_trip.source_kind == mirakana::TextureSourceKindV2::openexr);
    MK_REQUIRE(round_trip.openexr.channel_list == "R:half,G:half,B:half");
    MK_REQUIRE(round_trip.openexr.scene_linear_intent);
}

MK_TEST("texture source v2 rejects legacy texture source v1 explicitly") {
    bool rejected_legacy = false;
    try {
        (void)mirakana::deserialize_texture_source_document_v2("format=GameEngine.TextureSource.v1\n"
                                                               "texture.width=16\n"
                                                               "texture.height=16\n"
                                                               "texture.pixel_format=rgba8_unorm\n");
    } catch (const std::invalid_argument& error) {
        rejected_legacy = std::string{error.what()}.contains("legacy GameEngine.TextureSource.v1");
    }

    MK_REQUIRE(rejected_legacy);
}

MK_TEST("texture source v2 validates ktx2 basis container metadata") {
    const auto source = make_ktx2_basis_source();
    MK_REQUIRE(mirakana::is_valid_texture_source_document_v2(source));

    const auto text = mirakana::serialize_texture_source_document_v2(source);
    MK_REQUIRE(text.contains("ktx2.vk_format=VK_FORMAT_UNDEFINED\n"));
    MK_REQUIRE(text.contains("ktx2.level_count=11\n"));
    MK_REQUIRE(text.contains("ktx2.face_count=6\n"));
    MK_REQUIRE(text.contains("ktx2.supercompression=basis_lz\n"));
    MK_REQUIRE(text.contains("ktx2.basis_codec=basis_uastc\n"));
    MK_REQUIRE(text.contains("ktx2.requires_transcoding=true\n"));

    auto invalid = source;
    invalid.ktx2_basis.face_count = 5;
    MK_REQUIRE(!mirakana::is_valid_texture_source_document_v2(invalid));
}

MK_TEST("texture cook metadata serializes backend policy rows and host diagnostics") {
    const mirakana::TextureCookMetadataDocumentV1 metadata{
        .source = make_openexr_source(),
        .backend_decisions = make_backend_decisions(),
        .estimated_source_bytes = 8'388'608,
        .estimated_decoded_bytes = 12'582'912,
    };

    MK_REQUIRE(mirakana::is_valid_texture_cook_metadata_document_v1(metadata));
    const auto text = mirakana::serialize_texture_cook_metadata_document_v1(metadata);

    MK_REQUIRE(text.starts_with("format=GameEngine.CookedTextureMetadata.v1\n"));
    MK_REQUIRE(text.contains("source.format=GameEngine.TextureSource.v2\n"));
    MK_REQUIRE(text.contains("source.path=source/textures/environment/studio.exr\n"));
    MK_REQUIRE(text.contains("source.hash=sha256:00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff\n"));
    MK_REQUIRE(text.contains("source.provenance_id=provenance.environment.studio\n"));
    MK_REQUIRE(text.contains("source.license_id=LicenseRef-Proprietary\n"));
    MK_REQUIRE(text.contains("texture.color_space=scene_linear\n"));
    MK_REQUIRE(text.contains("texture.sampler_class=environment_radiance\n"));
    MK_REQUIRE(text.contains("texture.mip_count=1\n"));
    MK_REQUIRE(text.contains("texture.backend_policy_count=5\n"));
    MK_REQUIRE(text.contains("texture.backend.0.backend=d3d12\n"));
    MK_REQUIRE(text.contains("texture.backend.0.device_format=DXGI_FORMAT_BC6H_UF16\n"));
    MK_REQUIRE(text.contains("texture.backend.1.backend=vulkan\n"));
    MK_REQUIRE(text.contains("texture.backend.1.host_validated=false\n"));
    MK_REQUIRE(text.contains("texture.backend.1.diagnostic=host-gated:vulkan-format-feature-query\n"));
    MK_REQUIRE(text.contains("texture.backend.4.backend=metal_ios\n"));
    MK_REQUIRE(text.contains("texture.unsupported_host_diagnostic_count=4\n"));
}

MK_TEST("environment texture geasset metadata serializes deterministic cook provenance") {
    const mirakana::EnvironmentTextureGeassetMetadataDocumentV1 document{
        .geasset_path = "runtime/assets/environment/studio.texture.geasset",
        .cook_metadata =
            mirakana::TextureCookMetadataDocumentV1{
                .source = make_openexr_source(),
                .backend_decisions = make_backend_decisions(),
                .estimated_source_bytes = 8'388'608,
                .estimated_decoded_bytes = 12'582'912,
            },
        .mip_count = 1,
        .max_estimated_gpu_bytes = 16'777'216,
        .unsupported_host_diagnostic_count = 4,
        .metadata_only = true,
    };

    MK_REQUIRE(mirakana::is_valid_environment_texture_geasset_metadata_document_v1(document));
    const auto text = mirakana::serialize_environment_texture_geasset_metadata_document_v1(document);

    MK_REQUIRE(text.starts_with("format=GameEngine.EnvironmentTextureGeassetMetadata.v1\n"));
    MK_REQUIRE(text.contains("asset.path=runtime/assets/environment/studio.texture.geasset\n"));
    MK_REQUIRE(text.contains("asset.kind=environment_texture\n"));
    MK_REQUIRE(text.contains("asset.payload=metadata_only\n"));
    MK_REQUIRE(text.contains("source.hash=sha256:00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff\n"));
    MK_REQUIRE(text.contains("source.provenance_id=provenance.environment.studio\n"));
    MK_REQUIRE(text.contains("source.license_id=LicenseRef-Proprietary\n"));
    MK_REQUIRE(text.contains("texture.color_space=scene_linear\n"));
    MK_REQUIRE(text.contains("texture.mip_count=1\n"));
    MK_REQUIRE(text.contains("texture.backend.0.compression=bc6h\n"));
    MK_REQUIRE(text.contains("texture.backend.0.transcode=offline_policy\n"));
    MK_REQUIRE(text.contains("texture.max_estimated_gpu_bytes=16777216\n"));
    MK_REQUIRE(text.contains("texture.unsupported_host_diagnostic_count=4\n"));

    auto invalid = document;
    invalid.geasset_path = "../studio.texture.geasset";
    MK_REQUIRE(!mirakana::is_valid_environment_texture_geasset_metadata_document_v1(invalid));
}

MK_TEST("environment asset source requires texture source v2 radiance provenance") {
    const mirakana::EnvironmentAssetSourceDocumentV1 document{
        .environment_profile_source_path = "source/environment/default_outdoor.geenv",
        .radiance_texture_source_path = "source/textures/environment/studio.texture_source_v2",
        .provenance_id = "provenance.environment.default_outdoor",
        .license_id = "LicenseRef-Proprietary",
        .requires_scene_linear_radiance = true,
        .texture_source_v2_required = true,
    };

    MK_REQUIRE(mirakana::is_valid_environment_asset_source_document_v1(document));
    const auto text = mirakana::serialize_environment_asset_source_document_v1(document);
    MK_REQUIRE(text == "format=GameEngine.EnvironmentAssetSource.v1\n"
                       "environment.profile_source_path=source/environment/default_outdoor.geenv\n"
                       "environment.radiance_texture_source_path=source/textures/environment/studio.texture_source_v2\n"
                       "environment.provenance_id=provenance.environment.default_outdoor\n"
                       "environment.license_id=LicenseRef-Proprietary\n"
                       "environment.requires_scene_linear_radiance=true\n"
                       "environment.texture_source_v2_required=true\n");

    auto invalid = document;
    invalid.texture_source_v2_required = false;
    MK_REQUIRE(!mirakana::is_valid_environment_asset_source_document_v1(invalid));
}

int main() {
    return mirakana::test::run_all();
}
