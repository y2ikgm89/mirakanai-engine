// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/asset_import_tool.hpp"

#include "mirakana/assets/asset_source_format.hpp"
#include "mirakana/assets/material.hpp"
#include "mirakana/scene/scene.hpp"
#include "mirakana/tools/morph_mesh_cpu_source_bridge.hpp"

#include <algorithm>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mirakana {
namespace {

struct PreparedImport {
    AssetImportedArtifact artifact;
    std::string cooked_text;
};

[[nodiscard]] std::string action_kind_name(AssetImportActionKind kind) {
    switch (kind) {
    case AssetImportActionKind::texture:
        return "texture";
    case AssetImportActionKind::mesh:
        return "mesh";
    case AssetImportActionKind::morph_mesh_cpu:
        return "morph_mesh_cpu";
    case AssetImportActionKind::animation_float_clip:
        return "animation_float_clip";
    case AssetImportActionKind::animation_quaternion_clip:
        return "animation_quaternion_clip";
    case AssetImportActionKind::material:
        return "material";
    case AssetImportActionKind::scene:
        return "scene";
    case AssetImportActionKind::audio:
        return "audio";
    case AssetImportActionKind::unknown:
        break;
    }
    return "unknown";
}

[[nodiscard]] const char* cooked_format_name(AssetImportActionKind kind) {
    switch (kind) {
    case AssetImportActionKind::texture:
        return "GameEngine.CookedTexture.v1";
    case AssetImportActionKind::mesh:
        return "GameEngine.CookedMesh.v2";
    case AssetImportActionKind::morph_mesh_cpu:
        return "GameEngine.CookedMorphMeshCpu.v1";
    case AssetImportActionKind::animation_float_clip:
        return "GameEngine.CookedAnimationFloatClip.v1";
    case AssetImportActionKind::animation_quaternion_clip:
        return "GameEngine.CookedAnimationQuaternionClip.v1";
    case AssetImportActionKind::material:
        return "GameEngine.Material.v1";
    case AssetImportActionKind::scene:
        return "GameEngine.Scene.v1";
    case AssetImportActionKind::audio:
        return "GameEngine.CookedAudio.v1";
    case AssetImportActionKind::unknown:
        break;
    }
    return "GameEngine.Unknown.v1";
}

[[nodiscard]] char hex_digit(std::uint8_t value) noexcept {
    return static_cast<char>(value < 10U ? '0' + value : 'a' + (value - 10U));
}

[[nodiscard]] std::string encode_hex_bytes(const std::vector<std::uint8_t>& bytes) {
    std::string encoded;
    encoded.reserve(bytes.size() * 2U);
    for (const auto byte : bytes) {
        encoded.push_back(hex_digit(static_cast<std::uint8_t>(byte >> 4U)));
        encoded.push_back(hex_digit(static_cast<std::uint8_t>(byte & 0x0FU)));
    }
    return encoded;
}

[[nodiscard]] std::string cooked_texture_artifact(const AssetImportAction& action,
                                                  const TextureSourceDocument& texture) {
    std::ostringstream output;
    output << "format=" << cooked_format_name(AssetImportActionKind::texture) << '\n';
    output << "asset.id=" << action.id.value << '\n';
    output << "asset.kind=" << action_kind_name(AssetImportActionKind::texture) << '\n';
    output << "source.path=" << action.source_path << '\n';
    output << "texture.width=" << texture.width << '\n';
    output << "texture.height=" << texture.height << '\n';
    output << "texture.pixel_format=" << texture_source_pixel_format_name(texture.pixel_format) << '\n';
    output << "texture.source_bytes=" << texture_source_uncompressed_bytes(texture) << '\n';
    if (!texture.bytes.empty()) {
        output << "texture.data_hex=" << encode_hex_bytes(texture.bytes) << '\n';
    }
    return output.str();
}

[[nodiscard]] std::string cooked_mesh_artifact(const AssetImportAction& action, const MeshSourceDocument& mesh) {
    std::ostringstream output;
    output << "format=" << cooked_format_name(AssetImportActionKind::mesh) << '\n';
    output << "asset.id=" << action.id.value << '\n';
    output << "asset.kind=" << action_kind_name(AssetImportActionKind::mesh) << '\n';
    output << "source.path=" << action.source_path << '\n';
    output << "mesh.vertex_count=" << mesh.vertex_count << '\n';
    output << "mesh.index_count=" << mesh.index_count << '\n';
    output << "mesh.has_normals=" << (mesh.has_normals ? "true" : "false") << '\n';
    output << "mesh.has_uvs=" << (mesh.has_uvs ? "true" : "false") << '\n';
    output << "mesh.has_tangent_frame=" << (mesh.has_tangent_frame ? "true" : "false") << '\n';
    if (!mesh.vertex_bytes.empty()) {
        output << "mesh.vertex_data_hex=" << encode_hex_bytes(mesh.vertex_bytes) << '\n';
    }
    if (!mesh.index_bytes.empty()) {
        output << "mesh.index_data_hex=" << encode_hex_bytes(mesh.index_bytes) << '\n';
    }
    return output.str();
}

[[nodiscard]] std::string cooked_audio_artifact(const AssetImportAction& action, const AudioSourceDocument& audio) {
    std::ostringstream output;
    output << "format=" << cooked_format_name(AssetImportActionKind::audio) << '\n';
    output << "asset.id=" << action.id.value << '\n';
    output << "asset.kind=" << action_kind_name(AssetImportActionKind::audio) << '\n';
    output << "source.path=" << action.source_path << '\n';
    output << "audio.sample_rate=" << audio.sample_rate << '\n';
    output << "audio.channel_count=" << audio.channel_count << '\n';
    output << "audio.frame_count=" << audio.frame_count << '\n';
    output << "audio.sample_format=" << audio_source_sample_format_name(audio.sample_format) << '\n';
    output << "audio.source_bytes=" << audio_source_uncompressed_bytes(audio) << '\n';
    if (!audio.samples.empty()) {
        output << "audio.data_hex=" << encode_hex_bytes(audio.samples) << '\n';
    }
    return output.str();
}

[[nodiscard]] std::string cooked_morph_mesh_cpu_artifact(const AssetImportAction& action,
                                                         const MorphMeshCpuSourceDocument& morph) {
    std::ostringstream output;
    output << "format=" << cooked_format_name(AssetImportActionKind::morph_mesh_cpu) << '\n';
    output << "asset.id=" << action.id.value << '\n';
    output << "asset.kind=" << action_kind_name(AssetImportActionKind::morph_mesh_cpu) << '\n';
    output << "source.path=" << action.source_path << '\n';
    write_morph_mesh_cpu_document_payload(output, morph);
    return output.str();
}

[[nodiscard]] IExternalAssetImporter* find_external_importer(const AssetImportExecutionOptions& options,
                                                             const AssetImportAction& action) noexcept {
    for (auto* importer : options.external_importers) {
        if (importer != nullptr && importer->supports(action)) {
            return importer;
        }
    }
    return nullptr;
}

[[nodiscard]] std::string unsupported_external_source_diagnostic(const AssetImportAction& action,
                                                                 const std::exception& error) {
    return "unsupported external source; no importer matched " + action.source_path + ": " + error.what();
}

[[nodiscard]] std::string cooked_animation_float_clip_artifact(const AssetImportAction& action,
                                                               const AnimationFloatClipSourceDocument& clip) {
    std::ostringstream output;
    output << "format=" << cooked_format_name(AssetImportActionKind::animation_float_clip) << '\n';
    output << "asset.id=" << action.id.value << '\n';
    output << "asset.kind=" << action_kind_name(AssetImportActionKind::animation_float_clip) << '\n';
    output << "source.path=" << action.source_path << '\n';
    write_animation_float_clip_document_payload(output, clip);
    return output.str();
}

[[nodiscard]] std::string cooked_animation_quaternion_clip_artifact(const AssetImportAction& action,
                                                                    const AnimationQuaternionClipSourceDocument& clip) {
    std::ostringstream output;
    output << "format=" << cooked_format_name(AssetImportActionKind::animation_quaternion_clip) << '\n';
    output << "asset.id=" << action.id.value << '\n';
    output << "asset.kind=" << action_kind_name(AssetImportActionKind::animation_quaternion_clip) << '\n';
    output << "source.path=" << action.source_path << '\n';
    write_animation_quaternion_clip_document_payload(output, clip);
    return output.str();
}

[[nodiscard]] AnimationFloatClipSourceDocument
import_animation_float_clip_source_document(IFileSystem& filesystem, const AssetImportAction& action,
                                            const AssetImportExecutionOptions& options) {
    const auto source_text = filesystem.read_text(action.source_path);
    try {
        return deserialize_animation_float_clip_source_document(source_text);
    } catch (const std::exception& first_party_error) {
        auto* importer = find_external_importer(options, action);
        if (importer == nullptr) {
            throw std::invalid_argument(unsupported_external_source_diagnostic(action, first_party_error));
        }
        return deserialize_animation_float_clip_source_document(importer->import_source_document(filesystem, action));
    }
}

[[nodiscard]] AnimationQuaternionClipSourceDocument
import_animation_quaternion_clip_source_document(IFileSystem& filesystem, const AssetImportAction& action,
                                                 const AssetImportExecutionOptions& options) {
    const auto source_text = filesystem.read_text(action.source_path);
    try {
        return deserialize_animation_quaternion_clip_source_document(source_text);
    } catch (const std::exception& first_party_error) {
        auto* importer = find_external_importer(options, action);
        if (importer == nullptr) {
            throw std::invalid_argument(unsupported_external_source_diagnostic(action, first_party_error));
        }
        return deserialize_animation_quaternion_clip_source_document(
            importer->import_source_document(filesystem, action));
    }
}

[[nodiscard]] MorphMeshCpuSourceDocument
import_morph_mesh_cpu_source_document(IFileSystem& filesystem, const AssetImportAction& action,
                                      const AssetImportExecutionOptions& options) {
    const auto source_text = filesystem.read_text(action.source_path);
    try {
        return deserialize_morph_mesh_cpu_source_document(source_text);
    } catch (const std::exception& first_party_error) {
        auto* importer = find_external_importer(options, action);
        if (importer == nullptr) {
            throw std::invalid_argument(unsupported_external_source_diagnostic(action, first_party_error));
        }
        return deserialize_morph_mesh_cpu_source_document(importer->import_source_document(filesystem, action));
    }
}

[[nodiscard]] TextureSourceDocument import_texture_source_document(IFileSystem& filesystem,
                                                                   const AssetImportAction& action,
                                                                   const AssetImportExecutionOptions& options) {
    const auto source_text = filesystem.read_text(action.source_path);
    try {
        return deserialize_texture_source_document(source_text);
    } catch (const std::exception& first_party_error) {
        auto* importer = find_external_importer(options, action);
        if (importer == nullptr) {
            throw std::invalid_argument(unsupported_external_source_diagnostic(action, first_party_error));
        }
        return deserialize_texture_source_document(importer->import_source_document(filesystem, action));
    }
}

[[nodiscard]] MeshSourceDocument import_mesh_source_document(IFileSystem& filesystem, const AssetImportAction& action,
                                                             const AssetImportExecutionOptions& options) {
    const auto source_text = filesystem.read_text(action.source_path);
    try {
        return deserialize_mesh_source_document(source_text);
    } catch (const std::exception& first_party_error) {
        auto* importer = find_external_importer(options, action);
        if (importer == nullptr) {
            throw std::invalid_argument(unsupported_external_source_diagnostic(action, first_party_error));
        }
        return deserialize_mesh_source_document(importer->import_source_document(filesystem, action));
    }
}

[[nodiscard]] AudioSourceDocument import_audio_source_document(IFileSystem& filesystem, const AssetImportAction& action,
                                                               const AssetImportExecutionOptions& options) {
    const auto source_text = filesystem.read_text(action.source_path);
    try {
        return deserialize_audio_source_document(source_text);
    } catch (const std::exception& first_party_error) {
        auto* importer = find_external_importer(options, action);
        if (importer == nullptr) {
            throw std::invalid_argument(unsupported_external_source_diagnostic(action, first_party_error));
        }
        return deserialize_audio_source_document(importer->import_source_document(filesystem, action));
    }
}

[[nodiscard]] PreparedImport prepare_import_action(IFileSystem& filesystem, const AssetImportAction& action,
                                                   const AssetImportExecutionOptions& options) {
    if (!is_valid_asset_import_action(action)) {
        throw std::invalid_argument("asset import action is invalid");
    }
    if (!filesystem.exists(action.source_path)) {
        throw std::out_of_range("missing source file");
    }

    const auto source_text = filesystem.read_text(action.source_path);
    std::string cooked_text;
    if (action.kind == AssetImportActionKind::material) {
        cooked_text = serialize_material_definition(deserialize_material_definition(source_text));
    } else if (action.kind == AssetImportActionKind::scene) {
        cooked_text = serialize_scene(deserialize_scene(source_text));
    } else if (action.kind == AssetImportActionKind::texture) {
        cooked_text = cooked_texture_artifact(action, import_texture_source_document(filesystem, action, options));
    } else if (action.kind == AssetImportActionKind::mesh) {
        cooked_text = cooked_mesh_artifact(action, import_mesh_source_document(filesystem, action, options));
    } else if (action.kind == AssetImportActionKind::morph_mesh_cpu) {
        cooked_text =
            cooked_morph_mesh_cpu_artifact(action, import_morph_mesh_cpu_source_document(filesystem, action, options));
    } else if (action.kind == AssetImportActionKind::animation_float_clip) {
        cooked_text = cooked_animation_float_clip_artifact(
            action, import_animation_float_clip_source_document(filesystem, action, options));
    } else if (action.kind == AssetImportActionKind::animation_quaternion_clip) {
        cooked_text = cooked_animation_quaternion_clip_artifact(
            action, import_animation_quaternion_clip_source_document(filesystem, action, options));
    } else if (action.kind == AssetImportActionKind::audio) {
        cooked_text = cooked_audio_artifact(action, import_audio_source_document(filesystem, action, options));
    } else {
        throw std::invalid_argument("asset import action kind is unsupported");
    }

    return PreparedImport{
        .artifact = AssetImportedArtifact{.asset = action.id, .kind = action.kind, .output_path = action.output_path},
        .cooked_text = std::move(cooked_text),
    };
}

[[nodiscard]] bool apply_result_less(const AssetHotReloadApplyResult& lhs,
                                     const AssetHotReloadApplyResult& rhs) noexcept {
    if (lhs.path != rhs.path) {
        return lhs.path < rhs.path;
    }
    return lhs.asset.value < rhs.asset.value;
}

using RecookRequestByAsset = std::unordered_map<AssetId, const AssetHotReloadRecookRequest*, AssetIdHash>;

[[nodiscard]] RecookRequestByAsset map_recook_requests(const std::vector<AssetHotReloadRecookRequest>& requests) {
    RecookRequestByAsset by_asset;
    by_asset.reserve(requests.size());
    for (const auto& request : requests) {
        if (request.asset.value == 0) {
            throw std::invalid_argument("asset runtime recook request has an invalid asset id");
        }
        by_asset.emplace(request.asset, &request);
    }
    return by_asset;
}

[[nodiscard]] const AssetHotReloadRecookRequest& recook_request_for(const RecookRequestByAsset& requests,
                                                                    AssetId asset) {
    const auto it = requests.find(asset);
    if (it == requests.end() || it->second == nullptr) {
        throw std::invalid_argument("asset runtime recook result has no matching request");
    }
    return *it->second;
}

} // namespace

AssetImportExecutionResult execute_asset_import_plan(IFileSystem& filesystem, const AssetImportPlan& plan) {
    return execute_asset_import_plan(filesystem, plan, AssetImportExecutionOptions{});
}

AssetImportExecutionResult execute_asset_import_plan(IFileSystem& filesystem, const AssetImportPlan& plan,
                                                     const AssetImportExecutionOptions& options) {
    AssetImportExecutionResult result;
    std::vector<PreparedImport> prepared;
    prepared.reserve(plan.actions.size());

    for (const auto& action : plan.actions) {
        try {
            prepared.push_back(prepare_import_action(filesystem, action, options));
        } catch (const std::out_of_range&) {
            result.failures.push_back(AssetImportFailure{
                .asset = action.id,
                .kind = action.kind,
                .source_path = action.source_path,
                .output_path = action.output_path,
                .diagnostic = "missing source file: " + action.source_path,
            });
        } catch (const std::exception& error) {
            result.failures.push_back(AssetImportFailure{
                .asset = action.id,
                .kind = action.kind,
                .source_path = action.source_path,
                .output_path = action.output_path,
                .diagnostic = error.what(),
            });
        }
    }

    if (!result.failures.empty()) {
        return result;
    }

    for (const auto& item : prepared) {
        filesystem.write_text(item.artifact.output_path, item.cooked_text);
        result.imported.push_back(item.artifact);
    }

    return result;
}

AssetRuntimeRecookExecutionResult
execute_asset_runtime_recook(IFileSystem& filesystem, const AssetImportPlan& import_plan,
                             AssetRuntimeReplacementState& replacements,
                             const std::vector<AssetHotReloadRecookRequest>& requests) {
    AssetRuntimeRecookExecutionResult result;
    result.recook_plan = build_asset_recook_plan(import_plan, requests);
    const auto request_by_asset = map_recook_requests(requests);
    result.import_result = execute_asset_import_plan(filesystem, result.recook_plan);

    if (result.import_result.succeeded()) {
        for (const auto& imported : result.import_result.imported) {
            const auto& request = recook_request_for(request_by_asset, imported.asset);
            result.apply_results.push_back(replacements.stage(request, imported.output_path, request.current_revision));
        }
    } else {
        std::unordered_set<AssetId, AssetIdHash> failed_assets;
        failed_assets.reserve(result.import_result.failures.size());
        for (const auto& failure : result.import_result.failures) {
            failed_assets.insert(failure.asset);
            const auto& request = recook_request_for(request_by_asset, failure.asset);
            result.apply_results.push_back(replacements.mark_failed(request, failure.diagnostic));
        }

        for (const auto& action : result.recook_plan.actions) {
            if (failed_assets.find(action.id) != failed_assets.end()) {
                continue;
            }
            const auto& request = recook_request_for(request_by_asset, action.id);
            result.apply_results.push_back(
                replacements.mark_failed(request, "asset recook transaction failed before apply"));
        }
    }

    std::ranges::sort(result.apply_results, apply_result_less);
    return result;
}

} // namespace mirakana
