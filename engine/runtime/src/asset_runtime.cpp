// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/asset_runtime.hpp"

#include "mirakana/assets/asset_source_format.hpp"
#include "mirakana/assets/tilemap_metadata.hpp"
#include "mirakana/assets/ui_atlas_metadata.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace mirakana::runtime {
namespace {

[[nodiscard]] bool valid_package_path(std::string_view path) noexcept {
    return !path.empty() && path.find('\0') == std::string_view::npos && path.find('\n') == std::string_view::npos &&
           path.find('\r') == std::string_view::npos && path.find('\\') == std::string_view::npos &&
           path.find(':') == std::string_view::npos && path.front() != '/' && path.find("..") == std::string_view::npos;
}

[[nodiscard]] std::string join_root(std::string_view root, std::string_view path) {
    if (!valid_package_path(path)) {
        throw std::invalid_argument("runtime asset package path is invalid");
    }
    if (root.empty()) {
        return std::string(path);
    }
    if (!valid_package_path(root)) {
        throw std::invalid_argument("runtime asset package content root is invalid");
    }

    std::string result(root);
    if (result.back() != '/') {
        result.push_back('/');
    }
    result.append(path);
    return result;
}

using KeyValues = std::unordered_map<std::string, std::string>;

[[nodiscard]] KeyValues parse_payload_key_values(std::string_view text, std::string_view diagnostic_name) {
    KeyValues values;
    std::size_t line_start = 0;
    while (line_start < text.size()) {
        const auto line_end = text.find('\n', line_start);
        const auto raw_line = text.substr(line_start, line_end == std::string_view::npos ? text.size() - line_start
                                                                                         : line_end - line_start);
        line_start = line_end == std::string_view::npos ? text.size() : line_end + 1U;
        if (raw_line.empty()) {
            continue;
        }
        if (raw_line.find('\r') != std::string_view::npos) {
            throw std::invalid_argument(std::string(diagnostic_name) + " line contains carriage return");
        }
        const auto separator = raw_line.find('=');
        if (separator == std::string_view::npos) {
            throw std::invalid_argument(std::string(diagnostic_name) + " line is missing '='");
        }
        auto [_, inserted] =
            values.emplace(std::string(raw_line.substr(0, separator)), std::string(raw_line.substr(separator + 1U)));
        if (!inserted) {
            throw std::invalid_argument(std::string(diagnostic_name) + " contains duplicate keys");
        }
    }
    return values;
}

[[nodiscard]] const std::string& required_payload_value(const KeyValues& values, const std::string& key,
                                                        std::string_view diagnostic_name) {
    const auto it = values.find(key);
    if (it == values.end()) {
        throw std::invalid_argument(std::string(diagnostic_name) + " payload is missing key: " + key);
    }
    return it->second;
}

[[nodiscard]] const std::string* optional_payload_value(const KeyValues& values, const std::string& key) noexcept {
    const auto it = values.find(key);
    return it == values.end() ? nullptr : &it->second;
}

[[nodiscard]] std::uint32_t parse_payload_u32(std::string_view value, std::string_view diagnostic_name) {
    std::uint64_t parsed = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (error != std::errc{} || end != value.data() + value.size() ||
        parsed > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument(std::string(diagnostic_name) + " integer value is invalid");
    }
    return static_cast<std::uint32_t>(parsed);
}

[[nodiscard]] std::uint64_t parse_payload_u64(std::string_view value, std::string_view diagnostic_name) {
    std::uint64_t parsed = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (error != std::errc{} || end != value.data() + value.size()) {
        throw std::invalid_argument(std::string(diagnostic_name) + " integer value is invalid");
    }
    return parsed;
}

[[nodiscard]] bool parse_payload_bool(std::string_view value, std::string_view diagnostic_name) {
    if (value == "true") {
        return true;
    }
    if (value == "false") {
        return false;
    }
    throw std::invalid_argument(std::string(diagnostic_name) + " bool value is invalid");
}

[[nodiscard]] float parse_payload_float(std::string_view value, std::string_view diagnostic_name) {
    const auto valid_character = [](char character) noexcept {
        return (character >= '0' && character <= '9') || character == '+' || character == '-' || character == '.' ||
               character == 'e' || character == 'E';
    };
    if (value.empty() || !std::ranges::all_of(value, valid_character)) {
        throw std::invalid_argument(std::string(diagnostic_name) + " float value is invalid");
    }

    float parsed = 0.0F;
    std::istringstream stream{std::string{value}};
    stream.imbue(std::locale::classic());
    stream >> std::noskipws >> parsed;

    char trailing = '\0';
    if (!stream || (stream >> trailing) || !std::isfinite(parsed)) {
        throw std::invalid_argument(std::string(diagnostic_name) + " float value is invalid");
    }
    return parsed;
}

[[nodiscard]] std::array<float, 2> parse_payload_float2(std::string_view value, std::string_view diagnostic_name) {
    const auto separator = value.find(',');
    if (separator == std::string_view::npos || value.find(',', separator + 1U) != std::string_view::npos) {
        throw std::invalid_argument(std::string(diagnostic_name) + " float2 value is invalid");
    }
    return {
        parse_payload_float(value.substr(0, separator), diagnostic_name),
        parse_payload_float(value.substr(separator + 1U), diagnostic_name),
    };
}

[[nodiscard]] Vec3 parse_payload_float3(std::string_view value, std::string_view diagnostic_name) {
    Vec3 result{};
    std::size_t start = 0;
    for (std::size_t index = 0; index < 3U; ++index) {
        const auto separator = value.find(',', start);
        const auto last = index + 1U == 3U;
        if ((last && separator != std::string_view::npos) || (!last && separator == std::string_view::npos)) {
            throw std::invalid_argument(std::string(diagnostic_name) + " float3 value is invalid");
        }
        const auto length = last ? value.size() - start : separator - start;
        const auto component = parse_payload_float(value.substr(start, length), diagnostic_name);
        if (index == 0U) {
            result.x = component;
        } else if (index == 1U) {
            result.y = component;
        } else {
            result.z = component;
        }
        start = separator + 1U;
    }
    return result;
}

[[nodiscard]] std::array<float, 4> parse_payload_float4(std::string_view value, std::string_view diagnostic_name) {
    std::array<float, 4> result{};
    std::size_t start = 0;
    for (std::size_t index = 0; index < result.size(); ++index) {
        const auto separator = value.find(',', start);
        const auto last = index + 1U == result.size();
        if ((last && separator != std::string_view::npos) || (!last && separator == std::string_view::npos)) {
            throw std::invalid_argument(std::string(diagnostic_name) + " float4 value is invalid");
        }
        const auto length = last ? value.size() - start : separator - start;
        result[index] = parse_payload_float(value.substr(start, length), diagnostic_name);
        start = separator + 1U;
    }
    return result;
}

[[nodiscard]] PhysicsShape3DKind parse_physics_shape_3d_kind(std::string_view value, std::string_view diagnostic_name) {
    if (value == "aabb") {
        return PhysicsShape3DKind::aabb;
    }
    if (value == "sphere") {
        return PhysicsShape3DKind::sphere;
    }
    if (value == "capsule") {
        return PhysicsShape3DKind::capsule;
    }
    throw std::invalid_argument(std::string(diagnostic_name) + " shape kind is invalid");
}

[[nodiscard]] bool finite_vec3(Vec3 value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] bool finite_range(float value, float min_value, float max_value) noexcept {
    return std::isfinite(value) && value >= min_value && value <= max_value;
}

[[nodiscard]] bool valid_payload_token(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos &&
           value.find('\0') == std::string_view::npos;
}

[[nodiscard]] bool valid_physics_collision_token(std::string_view value, bool require_non_empty) noexcept {
    if (require_non_empty && value.empty()) {
        return false;
    }
    return value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos &&
           value.find('\0') == std::string_view::npos && value.find('=') == std::string_view::npos;
}

[[nodiscard]] bool valid_sprite_animation_frame(const RuntimeSpriteAnimationFrame& frame) noexcept {
    return finite_range(frame.duration_seconds, 0.0001F, 3600.0F) && frame.sprite.value != 0 &&
           frame.material.value != 0 && finite_range(frame.size[0], 0.0001F, 1000000.0F) &&
           finite_range(frame.size[1], 0.0001F, 1000000.0F) && finite_range(frame.tint[0], 0.0F, 64.0F) &&
           finite_range(frame.tint[1], 0.0F, 64.0F) && finite_range(frame.tint[2], 0.0F, 64.0F) &&
           finite_range(frame.tint[3], 0.0F, 64.0F);
}

[[nodiscard]] std::uint8_t hex_value(char value, std::string_view diagnostic_name) {
    if (value >= '0' && value <= '9') {
        return static_cast<std::uint8_t>(value - '0');
    }
    if (value >= 'a' && value <= 'f') {
        return static_cast<std::uint8_t>(10 + value - 'a');
    }
    if (value >= 'A' && value <= 'F') {
        return static_cast<std::uint8_t>(10 + value - 'A');
    }
    throw std::invalid_argument(std::string(diagnostic_name) + " hex byte payload is invalid");
}

[[nodiscard]] std::vector<std::uint8_t> parse_optional_hex_bytes(const KeyValues& values, const std::string& key,
                                                                 std::string_view diagnostic_name) {
    const auto* encoded = optional_payload_value(values, key);
    if (encoded == nullptr || encoded->empty()) {
        return {};
    }
    if ((encoded->size() % 2U) != 0U) {
        throw std::invalid_argument(std::string(diagnostic_name) + " hex byte payload length is invalid");
    }

    std::vector<std::uint8_t> bytes;
    bytes.reserve(encoded->size() / 2U);
    for (std::size_t index = 0; index < encoded->size(); index += 2U) {
        const auto high = hex_value((*encoded)[index], diagnostic_name);
        const auto low = hex_value((*encoded)[index + 1U], diagnostic_name);
        bytes.push_back(static_cast<std::uint8_t>((high << 4U) | low));
    }
    return bytes;
}

void validate_payload_header(const KeyValues& values, const RuntimeAssetRecord& record, std::string_view format,
                             std::string_view kind, std::string_view diagnostic_name) {
    if (required_payload_value(values, "format", diagnostic_name) != format) {
        throw std::invalid_argument(std::string(diagnostic_name) + " payload format is unsupported");
    }
    if (required_payload_value(values, "asset.kind", diagnostic_name) != kind) {
        throw std::invalid_argument(std::string(diagnostic_name) + " payload asset kind does not match");
    }
    const auto asset = parse_payload_u64(required_payload_value(values, "asset.id", diagnostic_name), diagnostic_name);
    if (asset != record.asset.value) {
        throw std::invalid_argument(std::string(diagnostic_name) + " payload asset id does not match record");
    }
}

template <typename Payload> [[nodiscard]] RuntimePayloadAccessResult<Payload> payload_failure(std::string diagnostic) {
    return RuntimePayloadAccessResult<Payload>{Payload{}, std::move(diagnostic)};
}

[[nodiscard]] bool dependency_edge_less(const AssetDependencyEdge& lhs, const AssetDependencyEdge& rhs) noexcept {
    if (lhs.asset.value != rhs.asset.value) {
        return lhs.asset.value < rhs.asset.value;
    }
    if (lhs.dependency.value != rhs.dependency.value) {
        return lhs.dependency.value < rhs.dependency.value;
    }
    if (static_cast<int>(lhs.kind) != static_cast<int>(rhs.kind)) {
        return static_cast<int>(lhs.kind) < static_cast<int>(rhs.kind);
    }
    return lhs.path < rhs.path;
}

} // namespace

RuntimeAssetPackage merge_runtime_asset_packages_overlay(const std::vector<RuntimeAssetPackage>& mounts,
                                                         RuntimePackageMountOverlay overlay) {
    std::unordered_map<AssetId, RuntimeAssetRecord, AssetIdHash> winner;
    winner.reserve(mounts.size() * 4U);

    for (const auto& package : mounts) {
        for (const auto& record : package.records()) {
            if (overlay == RuntimePackageMountOverlay::last_mount_wins) {
                winner.insert_or_assign(record.asset, record);
            } else {
                winner.try_emplace(record.asset, record);
            }
        }
    }

    std::vector<AssetId> sorted_ids;
    sorted_ids.reserve(winner.size());
    for (const auto& entry : winner) {
        sorted_ids.push_back(entry.first);
    }
    std::ranges::sort(sorted_ids, [](AssetId lhs, AssetId rhs) noexcept { return lhs.value < rhs.value; });

    const std::unordered_set<AssetId, AssetIdHash> alive(sorted_ids.begin(), sorted_ids.end());

    std::vector<RuntimeAssetRecord> merged_records;
    merged_records.reserve(sorted_ids.size());
    std::uint32_t next_handle = 1U;
    for (const auto& id : sorted_ids) {
        const auto& source = winner.find(id)->second;
        RuntimeAssetRecord record = source;
        record.handle = RuntimeAssetHandle{.value = next_handle};
        ++next_handle;

        std::vector<AssetId> filtered_dependencies;
        filtered_dependencies.reserve(record.dependencies.size());
        for (const auto& dependency : record.dependencies) {
            if (alive.find(dependency) != alive.end()) {
                filtered_dependencies.push_back(dependency);
            }
        }
        record.dependencies = std::move(filtered_dependencies);
        merged_records.push_back(std::move(record));
    }

    std::vector<AssetDependencyEdge> merged_edges;
    for (const auto& package : mounts) {
        for (const auto& edge : package.dependency_edges()) {
            if (alive.find(edge.asset) == alive.end() || alive.find(edge.dependency) == alive.end()) {
                continue;
            }
            merged_edges.push_back(edge);
        }
    }
    std::ranges::sort(merged_edges, dependency_edge_less);
    const auto unique_end =
        std::ranges::unique(merged_edges, [](const AssetDependencyEdge& lhs, const AssetDependencyEdge& rhs) noexcept {
            return !dependency_edge_less(lhs, rhs) && !dependency_edge_less(rhs, lhs);
        });
    merged_edges.erase(unique_end.begin(), merged_edges.end());

    return RuntimeAssetPackage{std::move(merged_records), std::move(merged_edges)};
}

std::uint64_t estimate_runtime_asset_package_resident_bytes(const RuntimeAssetPackage& package) noexcept {
    std::uint64_t bytes = 0;
    for (const auto& record : package.records()) {
        bytes += static_cast<std::uint64_t>(record.content.size());
    }
    return bytes;
}

RuntimeAssetPackage::RuntimeAssetPackage(std::vector<RuntimeAssetRecord> records,
                                         std::vector<AssetDependencyEdge> dependency_edges)
    : records_(std::move(records)), dependency_edges_(std::move(dependency_edges)) {}

const std::vector<RuntimeAssetRecord>& RuntimeAssetPackage::records() const noexcept {
    return records_;
}

const std::vector<AssetDependencyEdge>& RuntimeAssetPackage::dependency_edges() const noexcept {
    return dependency_edges_;
}

const RuntimeAssetRecord* RuntimeAssetPackage::find(AssetId asset) const noexcept {
    const auto it =
        std::ranges::find_if(records_, [asset](const RuntimeAssetRecord& record) { return record.asset == asset; });
    return it == records_.end() ? nullptr : &*it;
}

const RuntimeAssetRecord* RuntimeAssetPackage::find(RuntimeAssetHandle handle) const noexcept {
    if (handle.value == 0 || handle.value > records_.size()) {
        return nullptr;
    }
    return &records_[handle.value - 1U];
}

bool RuntimeAssetPackage::empty() const noexcept {
    return records_.empty();
}

bool RuntimeAssetPackageLoadResult::succeeded() const noexcept {
    return failures.empty();
}

bool RuntimeSceneMaterialResolutionResult::succeeded() const noexcept {
    return failures.empty();
}

RuntimeAssetPackageLoadResult load_runtime_asset_package(IFileSystem& filesystem, const RuntimeAssetPackageDesc& desc) {
    if (!valid_package_path(desc.index_path)) {
        throw std::invalid_argument("runtime asset package index path is invalid");
    }

    const auto index = deserialize_asset_cooked_package_index(filesystem.read_text(desc.index_path));
    std::vector<RuntimeAssetRecord> records;
    std::vector<RuntimeAssetPackageLoadFailure> failures;
    std::unordered_set<AssetId, AssetIdHash> assets;
    std::unordered_set<AssetId, AssetIdHash> loaded_assets;

    records.reserve(index.entries.size());
    for (const auto& entry : index.entries) {
        if (!is_valid_asset_cooked_package_entry(entry)) {
            failures.push_back(RuntimeAssetPackageLoadFailure{
                .asset = entry.asset, .path = entry.path, .diagnostic = "invalid package entry"});
            continue;
        }
        if (!assets.insert(entry.asset).second) {
            failures.push_back(RuntimeAssetPackageLoadFailure{
                .asset = entry.asset, .path = entry.path, .diagnostic = "duplicate package asset"});
            continue;
        }

        const auto payload_path = join_root(desc.content_root, entry.path);
        if (!filesystem.exists(payload_path)) {
            failures.push_back(RuntimeAssetPackageLoadFailure{
                .asset = entry.asset, .path = payload_path, .diagnostic = "missing cooked payload"});
            continue;
        }

        auto content = filesystem.read_text(payload_path);
        if (hash_asset_cooked_content(content) != entry.content_hash) {
            failures.push_back(RuntimeAssetPackageLoadFailure{
                .asset = entry.asset, .path = payload_path, .diagnostic = "cooked payload hash mismatch"});
            continue;
        }

        records.push_back(RuntimeAssetRecord{
            .handle = RuntimeAssetHandle{static_cast<std::uint32_t>(records.size() + 1U)},
            .asset = entry.asset,
            .kind = entry.kind,
            .path = payload_path,
            .content_hash = entry.content_hash,
            .source_revision = entry.source_revision,
            .dependencies = entry.dependencies,
            .content = std::move(content),
        });
        loaded_assets.insert(entry.asset);
    }

    for (const auto& record : records) {
        for (const auto dependency : record.dependencies) {
            if (loaded_assets.find(dependency) == loaded_assets.end()) {
                failures.push_back(RuntimeAssetPackageLoadFailure{
                    .asset = record.asset,
                    .path = record.path,
                    .diagnostic = "runtime asset package dependency is missing",
                });
            }
        }
    }

    if (!failures.empty()) {
        return RuntimeAssetPackageLoadResult{.package = RuntimeAssetPackage{}, .failures = std::move(failures)};
    }
    return RuntimeAssetPackageLoadResult{.package = RuntimeAssetPackage{std::move(records), index.dependencies},
                                         .failures = {}};
}

RuntimePayloadAccessResult<RuntimeTexturePayload> runtime_texture_payload(const RuntimeAssetRecord& record) {
    if (record.kind != AssetKind::texture) {
        return payload_failure<RuntimeTexturePayload>("runtime asset record is not a texture payload");
    }

    try {
        const auto values = parse_payload_key_values(record.content, "runtime texture");
        validate_payload_header(values, record, "GameEngine.CookedTexture.v1", "texture", "runtime texture");
        const RuntimeTexturePayload payload{
            .asset = record.asset,
            .handle = record.handle,
            .width = parse_payload_u32(required_payload_value(values, "texture.width", "runtime texture"),
                                       "runtime texture"),
            .height = parse_payload_u32(required_payload_value(values, "texture.height", "runtime texture"),
                                        "runtime texture"),
            .pixel_format = parse_texture_source_pixel_format(
                required_payload_value(values, "texture.pixel_format", "runtime texture")),
            .source_bytes = parse_payload_u64(required_payload_value(values, "texture.source_bytes", "runtime texture"),
                                              "runtime texture"),
            .bytes = parse_optional_hex_bytes(values, "texture.data_hex", "runtime texture"),
        };
        const TextureSourceDocument document{
            .width = payload.width, .height = payload.height, .pixel_format = payload.pixel_format};
        if (!is_valid_texture_source_document(document)) {
            return payload_failure<RuntimeTexturePayload>("runtime texture payload is invalid");
        }
        if (payload.source_bytes != texture_source_uncompressed_bytes(document)) {
            return payload_failure<RuntimeTexturePayload>("runtime texture payload source byte count is invalid");
        }
        if (!payload.bytes.empty() && payload.bytes.size() != payload.source_bytes) {
            return payload_failure<RuntimeTexturePayload>("runtime texture payload byte data size is invalid");
        }
        return RuntimePayloadAccessResult<RuntimeTexturePayload>{.payload = payload, .diagnostic = {}};
    } catch (const std::exception& error) {
        return payload_failure<RuntimeTexturePayload>(error.what());
    }
}

RuntimePayloadAccessResult<RuntimeMeshPayload> runtime_mesh_payload(const RuntimeAssetRecord& record) {
    if (record.kind != AssetKind::mesh) {
        return payload_failure<RuntimeMeshPayload>("runtime asset record is not a mesh payload");
    }

    try {
        const auto values = parse_payload_key_values(record.content, "runtime mesh");
        validate_payload_header(values, record, "GameEngine.CookedMesh.v2", "mesh", "runtime mesh");
        const RuntimeMeshPayload payload{
            .asset = record.asset,
            .handle = record.handle,
            .vertex_count =
                parse_payload_u32(required_payload_value(values, "mesh.vertex_count", "runtime mesh"), "runtime mesh"),
            .index_count =
                parse_payload_u32(required_payload_value(values, "mesh.index_count", "runtime mesh"), "runtime mesh"),
            .has_normals =
                parse_payload_bool(required_payload_value(values, "mesh.has_normals", "runtime mesh"), "runtime mesh"),
            .has_uvs =
                parse_payload_bool(required_payload_value(values, "mesh.has_uvs", "runtime mesh"), "runtime mesh"),
            .has_tangent_frame = parse_payload_bool(
                required_payload_value(values, "mesh.has_tangent_frame", "runtime mesh"), "runtime mesh"),
            .vertex_bytes = parse_optional_hex_bytes(values, "mesh.vertex_data_hex", "runtime mesh"),
            .index_bytes = parse_optional_hex_bytes(values, "mesh.index_data_hex", "runtime mesh"),
        };
        const MeshSourceDocument document{
            .vertex_count = payload.vertex_count,
            .index_count = payload.index_count,
            .has_normals = payload.has_normals,
            .has_uvs = payload.has_uvs,
            .has_tangent_frame = payload.has_tangent_frame,
        };
        if (!is_valid_mesh_source_document(document)) {
            return payload_failure<RuntimeMeshPayload>("runtime mesh payload is invalid");
        }
        return RuntimePayloadAccessResult<RuntimeMeshPayload>{.payload = payload, .diagnostic = {}};
    } catch (const std::exception& error) {
        return payload_failure<RuntimeMeshPayload>(error.what());
    }
}

static MorphMeshCpuSourceDocument morph_mesh_cpu_document_from_runtime_payload(const KeyValues& values,
                                                                               std::string_view diagnostic_name) {
    const auto vertex_count =
        parse_payload_u32(required_payload_value(values, "morph.vertex_count", diagnostic_name), diagnostic_name);
    if (vertex_count == 0U) {
        throw std::invalid_argument("runtime morph mesh CPU vertex_count must be non-zero");
    }
    const auto target_count =
        parse_payload_u32(required_payload_value(values, "morph.target_count", diagnostic_name), diagnostic_name);
    if (target_count == 0U) {
        throw std::invalid_argument("runtime morph mesh CPU target_count must be non-zero");
    }
    MorphMeshCpuSourceDocument document{
        .vertex_count = vertex_count,
        .bind_position_bytes = parse_optional_hex_bytes(values, "morph.bind_positions_hex", diagnostic_name),
        .bind_normal_bytes = parse_optional_hex_bytes(values, "morph.bind_normals_hex", diagnostic_name),
        .bind_tangent_bytes = parse_optional_hex_bytes(values, "morph.bind_tangents_hex", diagnostic_name),
        .targets = {},
        .target_weight_bytes = parse_optional_hex_bytes(values, "morph.target_weights_hex", diagnostic_name),
    };
    document.targets.resize(static_cast<std::size_t>(target_count));
    for (std::size_t index = 0; index < document.targets.size(); ++index) {
        const auto prefix = std::string{"morph.target."} + std::to_string(index) + ".";
        document.targets[index].position_delta_bytes =
            parse_optional_hex_bytes(values, prefix + "position_deltas_hex", diagnostic_name);
        document.targets[index].normal_delta_bytes =
            parse_optional_hex_bytes(values, prefix + "normal_deltas_hex", diagnostic_name);
        document.targets[index].tangent_delta_bytes =
            parse_optional_hex_bytes(values, prefix + "tangent_deltas_hex", diagnostic_name);
    }
    if (!is_valid_morph_mesh_cpu_source_document(document)) {
        throw std::invalid_argument("runtime morph mesh CPU payload is invalid");
    }
    return document;
}

static AnimationFloatClipSourceDocument
animation_float_clip_document_from_runtime_payload(const KeyValues& values, std::string_view diagnostic_name) {
    const auto track_count =
        parse_payload_u32(required_payload_value(values, "clip.track_count", diagnostic_name), diagnostic_name);
    if (track_count == 0U || track_count > 4096U) {
        throw std::invalid_argument("runtime animation float clip track_count is invalid");
    }
    AnimationFloatClipSourceDocument document;
    document.tracks.resize(static_cast<std::size_t>(track_count));
    for (std::size_t index = 0; index < document.tracks.size(); ++index) {
        const auto prefix = std::string{"clip.track."} + std::to_string(index) + ".";
        auto& track = document.tracks[index];
        track.target = required_payload_value(values, prefix + "target", diagnostic_name);
        const auto keyframe_count = parse_payload_u32(
            required_payload_value(values, prefix + "keyframe_count", diagnostic_name), diagnostic_name);
        if (keyframe_count == 0U || keyframe_count > 1'000'000U) {
            throw std::invalid_argument("runtime animation float clip keyframe_count is invalid");
        }
        track.time_seconds_bytes = parse_optional_hex_bytes(values, prefix + "times_hex", diagnostic_name);
        track.value_bytes = parse_optional_hex_bytes(values, prefix + "values_hex", diagnostic_name);
        if (track.time_seconds_bytes.size() != static_cast<std::size_t>(keyframe_count) * 4U ||
            track.value_bytes.size() != static_cast<std::size_t>(keyframe_count) * 4U) {
            throw std::invalid_argument("runtime animation float clip track byte length is invalid");
        }
    }
    if (!is_valid_animation_float_clip_source_document(document)) {
        throw std::invalid_argument("runtime animation float clip payload is invalid");
    }
    return document;
}

static AnimationQuaternionClipSourceDocument
animation_quaternion_clip_document_from_runtime_payload(const KeyValues& values, std::string_view diagnostic_name) {
    const auto track_count =
        parse_payload_u32(required_payload_value(values, "clip.track_count", diagnostic_name), diagnostic_name);
    if (track_count == 0U || track_count > 4096U) {
        throw std::invalid_argument("runtime animation quaternion clip track_count is invalid");
    }
    AnimationQuaternionClipSourceDocument document;
    document.tracks.resize(static_cast<std::size_t>(track_count));
    for (std::size_t index = 0; index < document.tracks.size(); ++index) {
        const auto prefix = std::string{"clip.track."} + std::to_string(index) + ".";
        auto& track = document.tracks[index];
        track.target = required_payload_value(values, prefix + "target", diagnostic_name);
        track.joint_index =
            parse_payload_u32(required_payload_value(values, prefix + "joint_index", diagnostic_name), diagnostic_name);
        const auto translation_count = parse_payload_u32(
            required_payload_value(values, prefix + "translation_keyframe_count", diagnostic_name), diagnostic_name);
        const auto rotation_count = parse_payload_u32(
            required_payload_value(values, prefix + "rotation_keyframe_count", diagnostic_name), diagnostic_name);
        const auto scale_count = parse_payload_u32(
            required_payload_value(values, prefix + "scale_keyframe_count", diagnostic_name), diagnostic_name);
        if (translation_count > 1'000'000U || rotation_count > 1'000'000U || scale_count > 1'000'000U) {
            throw std::invalid_argument("runtime animation quaternion clip keyframe_count is invalid");
        }

        track.translation_time_seconds_bytes =
            parse_optional_hex_bytes(values, prefix + "translation_times_hex", diagnostic_name);
        track.translation_xyz_bytes =
            parse_optional_hex_bytes(values, prefix + "translations_xyz_hex", diagnostic_name);
        track.rotation_time_seconds_bytes =
            parse_optional_hex_bytes(values, prefix + "rotation_times_hex", diagnostic_name);
        track.rotation_xyzw_bytes = parse_optional_hex_bytes(values, prefix + "rotations_xyzw_hex", diagnostic_name);
        track.scale_time_seconds_bytes = parse_optional_hex_bytes(values, prefix + "scale_times_hex", diagnostic_name);
        track.scale_xyz_bytes = parse_optional_hex_bytes(values, prefix + "scales_xyz_hex", diagnostic_name);

        if (track.translation_time_seconds_bytes.size() != static_cast<std::size_t>(translation_count) * 4U ||
            track.translation_xyz_bytes.size() != static_cast<std::size_t>(translation_count) * 12U ||
            track.rotation_time_seconds_bytes.size() != static_cast<std::size_t>(rotation_count) * 4U ||
            track.rotation_xyzw_bytes.size() != static_cast<std::size_t>(rotation_count) * 16U ||
            track.scale_time_seconds_bytes.size() != static_cast<std::size_t>(scale_count) * 4U ||
            track.scale_xyz_bytes.size() != static_cast<std::size_t>(scale_count) * 12U) {
            throw std::invalid_argument("runtime animation quaternion clip track byte length is invalid");
        }
    }
    if (!is_valid_animation_quaternion_clip_source_document(document)) {
        throw std::invalid_argument("runtime animation quaternion clip payload is invalid");
    }
    return document;
}

RuntimePayloadAccessResult<RuntimeMorphMeshCpuPayload>
runtime_morph_mesh_cpu_payload(const RuntimeAssetRecord& record) {
    if (record.kind != AssetKind::morph_mesh_cpu) {
        return payload_failure<RuntimeMorphMeshCpuPayload>("runtime asset record is not a morph mesh CPU payload");
    }

    try {
        const auto values = parse_payload_key_values(record.content, "runtime morph mesh CPU");
        validate_payload_header(values, record, "GameEngine.CookedMorphMeshCpu.v1", "morph_mesh_cpu",
                                "runtime morph mesh CPU");
        const auto morph = morph_mesh_cpu_document_from_runtime_payload(values, "runtime morph mesh CPU");
        return RuntimePayloadAccessResult<RuntimeMorphMeshCpuPayload>{
            .payload = RuntimeMorphMeshCpuPayload{.asset = record.asset, .handle = record.handle, .morph = morph},
            .diagnostic = {},
        };
    } catch (const std::exception& error) {
        return payload_failure<RuntimeMorphMeshCpuPayload>(error.what());
    }
}

RuntimePayloadAccessResult<RuntimeAnimationFloatClipPayload>
runtime_animation_float_clip_payload(const RuntimeAssetRecord& record) {
    if (record.kind != AssetKind::animation_float_clip) {
        return payload_failure<RuntimeAnimationFloatClipPayload>(
            "runtime asset record is not an animation float clip payload");
    }

    try {
        const auto values = parse_payload_key_values(record.content, "runtime animation float clip");
        validate_payload_header(values, record, "GameEngine.CookedAnimationFloatClip.v1", "animation_float_clip",
                                "runtime animation float clip");
        const auto clip = animation_float_clip_document_from_runtime_payload(values, "runtime animation float clip");
        return RuntimePayloadAccessResult<RuntimeAnimationFloatClipPayload>{
            .payload = RuntimeAnimationFloatClipPayload{.asset = record.asset, .handle = record.handle, .clip = clip},
            .diagnostic = {},
        };
    } catch (const std::exception& error) {
        return payload_failure<RuntimeAnimationFloatClipPayload>(error.what());
    }
}

RuntimePayloadAccessResult<RuntimeAnimationQuaternionClipPayload>
runtime_animation_quaternion_clip_payload(const RuntimeAssetRecord& record) {
    if (record.kind != AssetKind::animation_quaternion_clip) {
        return payload_failure<RuntimeAnimationQuaternionClipPayload>(
            "runtime asset record is not an animation quaternion clip payload");
    }

    try {
        const auto values = parse_payload_key_values(record.content, "runtime animation quaternion clip");
        validate_payload_header(values, record, "GameEngine.CookedAnimationQuaternionClip.v1",
                                "animation_quaternion_clip", "runtime animation quaternion clip");
        const auto clip =
            animation_quaternion_clip_document_from_runtime_payload(values, "runtime animation quaternion clip");
        return RuntimePayloadAccessResult<RuntimeAnimationQuaternionClipPayload>{
            .payload =
                RuntimeAnimationQuaternionClipPayload{.asset = record.asset, .handle = record.handle, .clip = clip},
            .diagnostic = {},
        };
    } catch (const std::exception& error) {
        return payload_failure<RuntimeAnimationQuaternionClipPayload>(error.what());
    }
}

RuntimePayloadAccessResult<RuntimeSpriteAnimationPayload>
runtime_sprite_animation_payload(const RuntimeAssetRecord& record) {
    if (record.kind != AssetKind::sprite_animation) {
        return payload_failure<RuntimeSpriteAnimationPayload>("runtime asset record is not a sprite animation payload");
    }

    try {
        const auto values = parse_payload_key_values(record.content, "runtime sprite animation");
        validate_payload_header(values, record, "GameEngine.CookedSpriteAnimation.v1", "sprite_animation",
                                "runtime sprite animation");

        const auto target_node = required_payload_value(values, "target.node", "runtime sprite animation");
        if (!valid_payload_token(target_node)) {
            return payload_failure<RuntimeSpriteAnimationPayload>("runtime sprite animation target node is invalid");
        }
        const auto frame_count = parse_payload_u32(
            required_payload_value(values, "frame.count", "runtime sprite animation"), "runtime sprite animation");
        if (frame_count == 0U || frame_count > 4096U) {
            return payload_failure<RuntimeSpriteAnimationPayload>("runtime sprite animation frame count is invalid");
        }

        RuntimeSpriteAnimationPayload payload;
        payload.asset = record.asset;
        payload.handle = record.handle;
        payload.target_node = target_node;
        payload.loop = parse_payload_bool(required_payload_value(values, "playback.loop", "runtime sprite animation"),
                                          "runtime sprite animation");
        payload.frames.resize(static_cast<std::size_t>(frame_count));
        for (std::size_t index = 0; index < payload.frames.size(); ++index) {
            const auto prefix = std::string{"frame."} + std::to_string(index) + ".";
            auto& frame = payload.frames[index];
            frame.duration_seconds = parse_payload_float(
                required_payload_value(values, prefix + "duration_seconds", "runtime sprite animation"),
                "runtime sprite animation");
            frame.sprite =
                AssetId{parse_payload_u64(required_payload_value(values, prefix + "sprite", "runtime sprite animation"),
                                          "runtime sprite animation")};
            frame.material = AssetId{
                parse_payload_u64(required_payload_value(values, prefix + "material", "runtime sprite animation"),
                                  "runtime sprite animation")};
            frame.size =
                parse_payload_float2(required_payload_value(values, prefix + "size", "runtime sprite animation"),
                                     "runtime sprite animation");
            frame.tint =
                parse_payload_float4(required_payload_value(values, prefix + "tint", "runtime sprite animation"),
                                     "runtime sprite animation");

            if (!valid_sprite_animation_frame(frame)) {
                return payload_failure<RuntimeSpriteAnimationPayload>("runtime sprite animation frame is invalid");
            }
            if (std::ranges::find(record.dependencies, frame.sprite) == record.dependencies.end()) {
                return payload_failure<RuntimeSpriteAnimationPayload>(
                    "runtime sprite animation frame sprite is not declared as a package dependency");
            }
            if (std::ranges::find(record.dependencies, frame.material) == record.dependencies.end()) {
                return payload_failure<RuntimeSpriteAnimationPayload>(
                    "runtime sprite animation frame material is not declared as a package dependency");
            }
        }

        return RuntimePayloadAccessResult<RuntimeSpriteAnimationPayload>{.payload = std::move(payload),
                                                                         .diagnostic = {}};
    } catch (const std::exception& error) {
        return payload_failure<RuntimeSpriteAnimationPayload>(error.what());
    }
}

RuntimePayloadAccessResult<RuntimeSkinnedMeshPayload> runtime_skinned_mesh_payload(const RuntimeAssetRecord& record) {
    if (record.kind != AssetKind::skinned_mesh) {
        return payload_failure<RuntimeSkinnedMeshPayload>("runtime asset record is not a skinned mesh payload");
    }

    try {
        constexpr std::uint32_t k_vertex_stride_bytes = 72U;
        constexpr std::uint32_t k_joint_matrix_bytes = 64U;

        const auto values = parse_payload_key_values(record.content, "runtime skinned mesh");
        validate_payload_header(values, record, "GameEngine.CookedSkinnedMesh.v1", "skinned_mesh",
                                "runtime skinned mesh");
        const auto vertex_count =
            parse_payload_u32(required_payload_value(values, "skinned_mesh.vertex_count", "runtime skinned mesh"),
                              "runtime skinned mesh");
        const auto index_count = parse_payload_u32(
            required_payload_value(values, "skinned_mesh.index_count", "runtime skinned mesh"), "runtime skinned mesh");
        const auto joint_count = parse_payload_u32(
            required_payload_value(values, "skinned_mesh.joint_count", "runtime skinned mesh"), "runtime skinned mesh");
        auto vertex_bytes = parse_optional_hex_bytes(values, "skinned_mesh.vertex_data_hex", "runtime skinned mesh");
        auto index_bytes = parse_optional_hex_bytes(values, "skinned_mesh.index_data_hex", "runtime skinned mesh");
        auto joint_palette_bytes =
            parse_optional_hex_bytes(values, "skinned_mesh.joint_palette_hex", "runtime skinned mesh");

        if (joint_count == 0U) {
            throw std::invalid_argument("runtime skinned mesh joint_count must be non-zero");
        }
        const auto expected_vertex =
            static_cast<std::uint64_t>(vertex_count) * static_cast<std::uint64_t>(k_vertex_stride_bytes);
        if (static_cast<std::uint64_t>(vertex_bytes.size()) != expected_vertex) {
            throw std::invalid_argument(
                "runtime skinned mesh vertex_data_hex byte length does not match vertex_count and stride");
        }
        const auto expected_index = static_cast<std::uint64_t>(index_count) * 4ULL;
        if (static_cast<std::uint64_t>(index_bytes.size()) != expected_index) {
            throw std::invalid_argument(
                "runtime skinned mesh index_data_hex byte length does not match uint32 index_count");
        }
        const auto expected_palette =
            static_cast<std::uint64_t>(joint_count) * static_cast<std::uint64_t>(k_joint_matrix_bytes);
        if (static_cast<std::uint64_t>(joint_palette_bytes.size()) != expected_palette) {
            throw std::invalid_argument(
                "runtime skinned mesh joint_palette_hex byte length does not match joint_count");
        }

        RuntimeSkinnedMeshPayload payload{
            .asset = record.asset,
            .handle = record.handle,
            .vertex_count = vertex_count,
            .index_count = index_count,
            .joint_count = joint_count,
            .vertex_bytes = std::move(vertex_bytes),
            .index_bytes = std::move(index_bytes),
            .joint_palette_bytes = std::move(joint_palette_bytes),
        };
        return RuntimePayloadAccessResult<RuntimeSkinnedMeshPayload>{.payload = payload, .diagnostic = {}};
    } catch (const std::exception& error) {
        return payload_failure<RuntimeSkinnedMeshPayload>(error.what());
    }
}

RuntimePayloadAccessResult<RuntimeAudioPayload> runtime_audio_payload(const RuntimeAssetRecord& record) {
    if (record.kind != AssetKind::audio) {
        return payload_failure<RuntimeAudioPayload>("runtime asset record is not an audio payload");
    }

    try {
        const auto values = parse_payload_key_values(record.content, "runtime audio");
        validate_payload_header(values, record, "GameEngine.CookedAudio.v1", "audio", "runtime audio");
        const RuntimeAudioPayload payload{
            .asset = record.asset,
            .handle = record.handle,
            .sample_rate = parse_payload_u32(required_payload_value(values, "audio.sample_rate", "runtime audio"),
                                             "runtime audio"),
            .channel_count = parse_payload_u32(required_payload_value(values, "audio.channel_count", "runtime audio"),
                                               "runtime audio"),
            .frame_count = parse_payload_u64(required_payload_value(values, "audio.frame_count", "runtime audio"),
                                             "runtime audio"),
            .sample_format = parse_audio_source_sample_format(
                required_payload_value(values, "audio.sample_format", "runtime audio")),
            .source_bytes = parse_payload_u64(required_payload_value(values, "audio.source_bytes", "runtime audio"),
                                              "runtime audio"),
            .samples = parse_optional_hex_bytes(values, "audio.data_hex", "runtime audio"),
        };
        const AudioSourceDocument document{
            .sample_rate = payload.sample_rate,
            .channel_count = payload.channel_count,
            .frame_count = payload.frame_count,
            .sample_format = payload.sample_format,
        };
        if (!is_valid_audio_source_document(document)) {
            return payload_failure<RuntimeAudioPayload>("runtime audio payload is invalid");
        }
        if (payload.source_bytes != audio_source_uncompressed_bytes(document)) {
            return payload_failure<RuntimeAudioPayload>("runtime audio payload source byte count is invalid");
        }
        if (!payload.samples.empty() && payload.samples.size() != payload.source_bytes) {
            return payload_failure<RuntimeAudioPayload>("runtime audio payload sample byte size is invalid");
        }
        return RuntimePayloadAccessResult<RuntimeAudioPayload>{.payload = payload, .diagnostic = {}};
    } catch (const std::exception& error) {
        return payload_failure<RuntimeAudioPayload>(error.what());
    }
}

RuntimePayloadAccessResult<RuntimeMaterialPayload> runtime_material_payload(const RuntimeAssetRecord& record) {
    if (record.kind != AssetKind::material) {
        return payload_failure<RuntimeMaterialPayload>("runtime asset record is not a material payload");
    }

    try {
        auto material = deserialize_material_definition(record.content);
        if (material.id != record.asset) {
            return payload_failure<RuntimeMaterialPayload>("runtime material payload asset id does not match record");
        }
        auto metadata = build_material_pipeline_binding_metadata(material);
        return RuntimePayloadAccessResult<RuntimeMaterialPayload>{
            .payload = RuntimeMaterialPayload{.asset = record.asset,
                                              .handle = record.handle,
                                              .material = std::move(material),
                                              .binding_metadata = std::move(metadata)},
            .diagnostic = {},
        };
    } catch (const std::exception& error) {
        return payload_failure<RuntimeMaterialPayload>(error.what());
    }
}

RuntimePayloadAccessResult<RuntimeUiAtlasPayload> runtime_ui_atlas_payload(const RuntimeAssetRecord& record) {
    if (record.kind != AssetKind::ui_atlas) {
        return payload_failure<RuntimeUiAtlasPayload>("runtime asset record is not a ui atlas payload");
    }

    try {
        const auto document = deserialize_ui_atlas_metadata_document(record.content);
        if (document.asset != record.asset) {
            return payload_failure<RuntimeUiAtlasPayload>("runtime ui atlas payload asset id does not match record");
        }

        RuntimeUiAtlasPayload payload;
        payload.asset = record.asset;
        payload.handle = record.handle;

        payload.pages.reserve(document.pages.size());
        for (const auto& page : document.pages) {
            if (std::ranges::find(record.dependencies, page.asset) == record.dependencies.end()) {
                return payload_failure<RuntimeUiAtlasPayload>(
                    "runtime ui atlas page asset is not declared as a package dependency");
            }
            payload.pages.push_back(RuntimeUiAtlasPage{.asset = page.asset, .asset_uri = page.asset_uri});
        }

        payload.images.reserve(document.images.size());
        for (const auto& image : document.images) {
            payload.images.push_back(RuntimeUiAtlasImage{
                .resource_id = image.resource_id,
                .asset_uri = image.asset_uri,
                .page = image.page,
                .u0 = image.uv.u0,
                .v0 = image.uv.v0,
                .u1 = image.uv.u1,
                .v1 = image.uv.v1,
                .color = image.color,
            });
        }

        payload.glyphs.reserve(document.glyphs.size());
        for (const auto& glyph : document.glyphs) {
            payload.glyphs.push_back(RuntimeUiAtlasGlyph{
                .font_family = glyph.font_family,
                .glyph = glyph.glyph,
                .page = glyph.page,
                .u0 = glyph.uv.u0,
                .v0 = glyph.uv.v0,
                .u1 = glyph.uv.u1,
                .v1 = glyph.uv.v1,
                .color = glyph.color,
            });
        }

        return RuntimePayloadAccessResult<RuntimeUiAtlasPayload>{.payload = std::move(payload), .diagnostic = {}};
    } catch (const std::exception& error) {
        return payload_failure<RuntimeUiAtlasPayload>(error.what());
    }
}

RuntimePayloadAccessResult<RuntimeTilemapPayload> runtime_tilemap_payload(const RuntimeAssetRecord& record) {
    if (record.kind != AssetKind::tilemap) {
        return payload_failure<RuntimeTilemapPayload>("runtime asset record is not a tilemap payload");
    }

    try {
        const auto document = deserialize_tilemap_metadata_document(record.content);
        if (document.asset != record.asset) {
            return payload_failure<RuntimeTilemapPayload>("runtime tilemap payload asset id does not match record");
        }
        if (std::ranges::find(record.dependencies, document.atlas_page) == record.dependencies.end()) {
            return payload_failure<RuntimeTilemapPayload>(
                "runtime tilemap atlas page asset is not declared as a package dependency");
        }

        RuntimeTilemapPayload payload;
        payload.asset = record.asset;
        payload.handle = record.handle;
        payload.atlas_page = document.atlas_page;
        payload.atlas_page_uri = document.atlas_page_uri;
        payload.tile_width = document.tile_width;
        payload.tile_height = document.tile_height;

        payload.tiles.reserve(document.tiles.size());
        for (const auto& tile : document.tiles) {
            payload.tiles.push_back(RuntimeTilemapTile{
                .id = tile.id,
                .page = tile.page,
                .u0 = tile.uv.u0,
                .v0 = tile.uv.v0,
                .u1 = tile.uv.u1,
                .v1 = tile.uv.v1,
                .color = tile.color,
            });
        }
        payload.layers.reserve(document.layers.size());
        for (const auto& layer : document.layers) {
            payload.layers.push_back(RuntimeTilemapLayer{
                .name = layer.name,
                .width = layer.width,
                .height = layer.height,
                .visible = layer.visible,
                .cells = layer.cells,
            });
        }

        return RuntimePayloadAccessResult<RuntimeTilemapPayload>{.payload = std::move(payload), .diagnostic = {}};
    } catch (const std::exception& error) {
        return payload_failure<RuntimeTilemapPayload>(error.what());
    }
}

RuntimeTilemapVisibleCellSampleResult sample_runtime_tilemap_visible_cells(const RuntimeTilemapPayload& tilemap) {
    RuntimeTilemapVisibleCellSampleResult result;
    result.layer_count = tilemap.layers.size();
    result.tile_count = tilemap.tiles.size();

    if (tilemap.asset.value == 0) {
        ++result.diagnostic_count;
    }
    if (tilemap.atlas_page.value == 0) {
        ++result.diagnostic_count;
    }
    if (tilemap.tile_width == 0 || tilemap.tile_height == 0) {
        ++result.diagnostic_count;
    }
    if (tilemap.tiles.empty()) {
        ++result.diagnostic_count;
    }
    if (tilemap.layers.empty()) {
        ++result.diagnostic_count;
    }

    std::unordered_set<std::string_view> tile_ids;
    tile_ids.reserve(tilemap.tiles.size());
    for (const auto& tile : tilemap.tiles) {
        if (tile.id.empty()) {
            ++result.diagnostic_count;
            continue;
        }
        if (!tile_ids.insert(tile.id).second) {
            ++result.diagnostic_count;
        }
        if (tile.page != tilemap.atlas_page) {
            ++result.diagnostic_count;
        }
    }

    for (const auto& layer : tilemap.layers) {
        if (!layer.visible) {
            continue;
        }
        ++result.visible_layer_count;

        const auto expected_cell_count =
            static_cast<std::uint64_t>(layer.width) * static_cast<std::uint64_t>(layer.height);
        if (expected_cell_count != layer.cells.size()) {
            ++result.diagnostic_count;
            continue;
        }

        for (const auto& cell : layer.cells) {
            if (cell.empty()) {
                continue;
            }
            ++result.non_empty_cell_count;
            if (tile_ids.contains(cell)) {
                ++result.sampled_cell_count;
            } else {
                ++result.diagnostic_count;
            }
        }
    }

    result.succeeded = result.diagnostic_count == 0;
    result.diagnostic = result.succeeded ? "runtime tilemap visible cells sampled"
                                         : "runtime tilemap visible cell sampling produced " +
                                               std::to_string(result.diagnostic_count) + " diagnostics";
    return result;
}

RuntimePayloadAccessResult<RuntimePhysicsCollisionScene3DPayload>
runtime_physics_collision_scene_3d_payload(const RuntimeAssetRecord& record) {
    if (record.kind != AssetKind::physics_collision_scene) {
        return payload_failure<RuntimePhysicsCollisionScene3DPayload>(
            "runtime asset record is not a physics collision scene payload");
    }

    try {
        constexpr std::uint32_t max_body_count = 4096U;
        const auto values = parse_payload_key_values(record.content, "runtime physics collision scene");
        validate_payload_header(values, record, "GameEngine.PhysicsCollisionScene3D.v1", "physics_collision_scene",
                                "runtime physics collision scene");
        if (required_payload_value(values, "backend.native", "runtime physics collision scene") != "unsupported") {
            return payload_failure<RuntimePhysicsCollisionScene3DPayload>(
                "runtime physics collision scene native backend request is unsupported");
        }

        RuntimePhysicsCollisionScene3DPayload payload;
        payload.asset = record.asset;
        payload.handle = record.handle;
        payload.world_config.gravity =
            parse_payload_float3(required_payload_value(values, "world.gravity", "runtime physics collision scene"),
                                 "runtime physics collision scene");
        if (!finite_vec3(payload.world_config.gravity)) {
            return payload_failure<RuntimePhysicsCollisionScene3DPayload>(
                "runtime physics collision scene world gravity is invalid");
        }

        const auto body_count =
            parse_payload_u32(required_payload_value(values, "body.count", "runtime physics collision scene"),
                              "runtime physics collision scene");
        if (body_count == 0U || body_count > max_body_count) {
            return payload_failure<RuntimePhysicsCollisionScene3DPayload>(
                "runtime physics collision scene body count is invalid");
        }

        std::unordered_set<std::string> body_names;
        body_names.reserve(body_count);
        payload.bodies.reserve(body_count);
        for (std::size_t index = 0; index < body_count; ++index) {
            const auto prefix = std::string{"body."} + std::to_string(index) + ".";
            RuntimePhysicsCollisionBody3DPayload body;
            body.name = required_payload_value(values, prefix + "name", "runtime physics collision scene");
            body.material = required_payload_value(values, prefix + "material", "runtime physics collision scene");
            if (const auto compound = values.find(prefix + "compound"); compound != values.end()) {
                body.compound = compound->second;
            }
            body.body.shape = parse_physics_shape_3d_kind(
                required_payload_value(values, prefix + "shape", "runtime physics collision scene"),
                "runtime physics collision scene");
            body.body.position = parse_payload_float3(
                required_payload_value(values, prefix + "position", "runtime physics collision scene"),
                "runtime physics collision scene");
            body.body.velocity = parse_payload_float3(
                required_payload_value(values, prefix + "velocity", "runtime physics collision scene"),
                "runtime physics collision scene");
            body.body.dynamic = parse_payload_bool(
                required_payload_value(values, prefix + "dynamic", "runtime physics collision scene"),
                "runtime physics collision scene");
            body.body.mass =
                parse_payload_float(required_payload_value(values, prefix + "mass", "runtime physics collision scene"),
                                    "runtime physics collision scene");
            body.body.linear_damping = parse_payload_float(
                required_payload_value(values, prefix + "linear_damping", "runtime physics collision scene"),
                "runtime physics collision scene");
            body.body.half_extents = parse_payload_float3(
                required_payload_value(values, prefix + "half_extents", "runtime physics collision scene"),
                "runtime physics collision scene");
            body.body.radius = parse_payload_float(
                required_payload_value(values, prefix + "radius", "runtime physics collision scene"),
                "runtime physics collision scene");
            body.body.half_height = parse_payload_float(
                required_payload_value(values, prefix + "half_height", "runtime physics collision scene"),
                "runtime physics collision scene");
            body.body.collision_layer =
                parse_payload_u32(required_payload_value(values, prefix + "layer", "runtime physics collision scene"),
                                  "runtime physics collision scene");
            body.body.collision_mask =
                parse_payload_u32(required_payload_value(values, prefix + "mask", "runtime physics collision scene"),
                                  "runtime physics collision scene");
            body.body.trigger = parse_payload_bool(
                required_payload_value(values, prefix + "trigger", "runtime physics collision scene"),
                "runtime physics collision scene");

            if (!valid_physics_collision_token(body.name, true) || !body_names.insert(body.name).second) {
                return payload_failure<RuntimePhysicsCollisionScene3DPayload>(
                    "runtime physics collision scene body name is invalid");
            }
            if (!valid_physics_collision_token(body.material, false)) {
                return payload_failure<RuntimePhysicsCollisionScene3DPayload>(
                    "runtime physics collision scene body material is invalid");
            }
            if (!valid_physics_collision_token(body.compound, false)) {
                return payload_failure<RuntimePhysicsCollisionScene3DPayload>(
                    "runtime physics collision scene body compound group is invalid");
            }
            if (!is_valid_physics_body_desc(body.body)) {
                return payload_failure<RuntimePhysicsCollisionScene3DPayload>(
                    "runtime physics collision scene body is invalid");
            }

            payload.bodies.push_back(std::move(body));
        }

        return RuntimePayloadAccessResult<RuntimePhysicsCollisionScene3DPayload>{.payload = std::move(payload),
                                                                                 .diagnostic = {}};
    } catch (const std::exception& error) {
        return payload_failure<RuntimePhysicsCollisionScene3DPayload>(error.what());
    }
}

RuntimePayloadAccessResult<RuntimeScenePayload> runtime_scene_payload(const RuntimeAssetRecord& record) {
    if (record.kind != AssetKind::scene) {
        return payload_failure<RuntimeScenePayload>("runtime asset record is not a scene payload");
    }

    try {
        const auto values = parse_payload_key_values(record.content, "runtime scene");
        if (required_payload_value(values, "format", "runtime scene") != "GameEngine.Scene.v1") {
            return payload_failure<RuntimeScenePayload>("runtime scene payload format is unsupported");
        }
        const auto& name = required_payload_value(values, "scene.name", "runtime scene");
        if (name.empty()) {
            return payload_failure<RuntimeScenePayload>("runtime scene payload name is empty");
        }
        const auto node_count =
            parse_payload_u32(required_payload_value(values, "node.count", "runtime scene"), "runtime scene");
        return RuntimePayloadAccessResult<RuntimeScenePayload>{
            .payload = RuntimeScenePayload{.asset = record.asset,
                                           .handle = record.handle,
                                           .name = name,
                                           .node_count = node_count,
                                           .serialized_scene = record.content},
            .diagnostic = {},
        };
    } catch (const std::exception& error) {
        return payload_failure<RuntimeScenePayload>(error.what());
    }
}

RuntimeSceneMaterialResolutionResult resolve_runtime_scene_materials(const RuntimeAssetPackage& package,
                                                                     AssetId scene) {
    RuntimeSceneMaterialResolutionResult result;
    const auto* scene_record = package.find(scene);
    if (scene_record == nullptr) {
        result.failures.push_back(
            RuntimeSceneMaterialResolutionFailure{.asset = scene, .diagnostic = "runtime scene asset is missing"});
        return result;
    }
    const auto scene_payload = runtime_scene_payload(*scene_record);
    if (!scene_payload.succeeded()) {
        result.failures.push_back(
            RuntimeSceneMaterialResolutionFailure{.asset = scene, .diagnostic = scene_payload.diagnostic});
        return result;
    }
    result.scene = scene_payload.payload;

    for (const auto dependency : scene_record->dependencies) {
        const auto* dependency_record = package.find(dependency);
        if (dependency_record == nullptr) {
            result.failures.push_back(RuntimeSceneMaterialResolutionFailure{
                .asset = dependency, .diagnostic = "runtime scene dependency is missing"});
            continue;
        }
        if (dependency_record->kind != AssetKind::material) {
            continue;
        }

        const auto material_payload = runtime_material_payload(*dependency_record);
        if (!material_payload.succeeded()) {
            result.failures.push_back(
                RuntimeSceneMaterialResolutionFailure{.asset = dependency, .diagnostic = material_payload.diagnostic});
            continue;
        }
        result.materials.push_back(material_payload.payload);
    }

    return result;
}

void RuntimeAssetPackageStore::seed(RuntimeAssetPackage package) {
    active_ = std::move(package);
    has_active_ = true;
    pending_ = RuntimeAssetPackage{};
    has_pending_ = false;
}

void RuntimeAssetPackageStore::stage(RuntimeAssetPackage package) {
    pending_ = std::move(package);
    has_pending_ = true;
}

bool RuntimeAssetPackageStore::stage_if_loaded(RuntimeAssetPackageLoadResult result) {
    if (!result.succeeded()) {
        return false;
    }
    stage(std::move(result.package));
    return true;
}

bool RuntimeAssetPackageStore::commit_safe_point() {
    if (!has_pending_) {
        return false;
    }
    active_ = std::move(pending_);
    has_active_ = true;
    pending_ = RuntimeAssetPackage{};
    has_pending_ = false;
    return true;
}

bool RuntimeAssetPackageStore::unload_safe_point() noexcept {
    if (!has_active_) {
        return false;
    }
    active_ = RuntimeAssetPackage{};
    pending_ = RuntimeAssetPackage{};
    has_active_ = false;
    has_pending_ = false;
    return true;
}

void RuntimeAssetPackageStore::rollback_pending() noexcept {
    pending_ = RuntimeAssetPackage{};
    has_pending_ = false;
}

const RuntimeAssetPackage* RuntimeAssetPackageStore::active() const noexcept {
    return has_active_ ? &active_ : nullptr;
}

const RuntimeAssetPackage* RuntimeAssetPackageStore::pending() const noexcept {
    return has_pending_ ? &pending_ : nullptr;
}

} // namespace mirakana::runtime
