// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/physics_collision_package_tool.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <limits>
#include <locale>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace mirakana {
namespace {

constexpr std::uint32_t max_collision_body_count = 4096U;

void add_diagnostic(std::vector<PhysicsCollisionPackageDiagnostic>& diagnostics, std::string code, std::string message,
                    std::string path = {}, std::size_t body_index = 0) {
    diagnostics.push_back(PhysicsCollisionPackageDiagnostic{
        .code = std::move(code),
        .message = std::move(message),
        .path = std::move(path),
        .body_index = body_index,
    });
}

void sort_diagnostics(std::vector<PhysicsCollisionPackageDiagnostic>& diagnostics) {
    std::ranges::sort(diagnostics,
                      [](const PhysicsCollisionPackageDiagnostic& lhs, const PhysicsCollisionPackageDiagnostic& rhs) {
                          if (lhs.path != rhs.path) {
                              return lhs.path < rhs.path;
                          }
                          if (lhs.body_index != rhs.body_index) {
                              return lhs.body_index < rhs.body_index;
                          }
                          if (lhs.code != rhs.code) {
                              return lhs.code < rhs.code;
                          }
                          return lhs.message < rhs.message;
                      });
}

[[nodiscard]] bool finite(float value) noexcept {
    return std::isfinite(value);
}

[[nodiscard]] bool finite_vec3(Vec3 value) noexcept {
    return finite(value.x) && finite(value.y) && finite(value.z);
}

[[nodiscard]] bool valid_output_path(std::string_view path) noexcept {
    if (path.empty() || path.find('\0') != std::string_view::npos || path.find('\n') != std::string_view::npos ||
        path.find('\r') != std::string_view::npos || path.find('\\') != std::string_view::npos ||
        path.find(':') != std::string_view::npos || path.front() == '/') {
        return false;
    }

    std::size_t segment_start = 0;
    while (segment_start <= path.size()) {
        const auto segment_end = path.find('/', segment_start);
        const auto segment =
            path.substr(segment_start, segment_end == std::string_view::npos ? path.size() - segment_start
                                                                             : segment_end - segment_start);
        if (segment.empty() || segment == "." || segment == ".." || segment.find("..") != std::string_view::npos) {
            return false;
        }
        if (segment_end == std::string_view::npos) {
            break;
        }
        segment_start = segment_end + 1U;
    }
    return true;
}

[[nodiscard]] bool valid_payload_token(std::string_view value, bool require_non_empty) noexcept {
    if (require_non_empty && value.empty()) {
        return false;
    }
    return value.find('\0') == std::string_view::npos && value.find('\n') == std::string_view::npos &&
           value.find('\r') == std::string_view::npos && value.find('=') == std::string_view::npos;
}

void sort_asset_ids(std::vector<AssetId>& ids) {
    std::ranges::sort(ids, [](AssetId lhs, AssetId rhs) { return lhs.value < rhs.value; });
}

void sort_dependency_edges(std::vector<AssetDependencyEdge>& edges) {
    std::ranges::sort(edges, [](const AssetDependencyEdge& lhs, const AssetDependencyEdge& rhs) {
        if (lhs.path != rhs.path) {
            return lhs.path < rhs.path;
        }
        if (lhs.asset.value != rhs.asset.value) {
            return lhs.asset.value < rhs.asset.value;
        }
        if (lhs.dependency.value != rhs.dependency.value) {
            return lhs.dependency.value < rhs.dependency.value;
        }
        return static_cast<int>(lhs.kind) < static_cast<int>(rhs.kind);
    });
}

void sort_package_entries(std::vector<AssetCookedPackageEntry>& entries) {
    std::ranges::sort(entries, [](const AssetCookedPackageEntry& lhs, const AssetCookedPackageEntry& rhs) {
        if (lhs.path != rhs.path) {
            return lhs.path < rhs.path;
        }
        return lhs.asset.value < rhs.asset.value;
    });
}

[[nodiscard]] bool edge_same_identity(const AssetDependencyEdge& lhs, const AssetDependencyEdge& rhs) noexcept {
    return lhs.asset == rhs.asset && lhs.dependency == rhs.dependency && lhs.kind == rhs.kind && lhs.path == rhs.path;
}

void canonicalize_package_index(AssetCookedPackageIndex& index) {
    for (auto& entry : index.entries) {
        sort_asset_ids(entry.dependencies);
        entry.dependencies.erase(
            std::ranges::unique(entry.dependencies, [](AssetId lhs, AssetId rhs) { return lhs == rhs; }).begin(),
            entry.dependencies.end());
    }
    sort_package_entries(index.entries);
    sort_dependency_edges(index.dependencies);
    index.dependencies.erase(std::ranges::unique(index.dependencies, edge_same_identity).begin(),
                             index.dependencies.end());
}

[[nodiscard]] AssetCookedPackageEntry* find_entry(AssetCookedPackageIndex& index, AssetId asset) noexcept {
    const auto it = std::ranges::find_if(
        index.entries, [asset](const AssetCookedPackageEntry& entry) { return entry.asset == asset; });
    return it == index.entries.end() ? nullptr : &*it;
}

[[nodiscard]] const AssetCookedPackageEntry* find_entry(const AssetCookedPackageIndex& index, AssetId asset) noexcept {
    const auto it = std::ranges::find_if(
        index.entries, [asset](const AssetCookedPackageEntry& entry) { return entry.asset == asset; });
    return it == index.entries.end() ? nullptr : &*it;
}

[[nodiscard]] bool has_path_collision(const AssetCookedPackageIndex& index, AssetId asset, std::string_view path) {
    return std::ranges::any_of(index.entries, [asset, path](const AssetCookedPackageEntry& entry) {
        return entry.asset != asset && entry.path == path;
    });
}

[[nodiscard]] std::string_view shape_name(PhysicsShape3DKind shape) noexcept {
    switch (shape) {
    case PhysicsShape3DKind::aabb:
        return "aabb";
    case PhysicsShape3DKind::sphere:
        return "sphere";
    case PhysicsShape3DKind::capsule:
        return "capsule";
    }
    return "unknown";
}

[[nodiscard]] PhysicsShape3DKind parse_shape(std::string_view value) {
    if (value == "aabb") {
        return PhysicsShape3DKind::aabb;
    }
    if (value == "sphere") {
        return PhysicsShape3DKind::sphere;
    }
    if (value == "capsule") {
        return PhysicsShape3DKind::capsule;
    }
    throw std::invalid_argument("physics collision shape is unsupported");
}

void write_float(std::ostringstream& output, float value) {
    std::array<char, 64> buffer{};
    const auto [end, error] = std::to_chars(std::to_address(buffer.begin()), std::to_address(buffer.end()), value);
    if (error != std::errc{}) {
        throw std::invalid_argument("failed to serialize physics collision float");
    }
    output.write(buffer.data(), static_cast<std::streamsize>(end - buffer.data()));
}

void write_vec3(std::ostringstream& output, Vec3 value) {
    write_float(output, value.x);
    output << ',';
    write_float(output, value.y);
    output << ',';
    write_float(output, value.z);
}

[[nodiscard]] bool unsigned_digits(std::string_view value) noexcept {
    return !value.empty() && std::ranges::all_of(value, [](char ch) { return ch >= '0' && ch <= '9'; });
}

[[nodiscard]] std::uint64_t parse_u64(std::string_view value, std::string_view field) {
    if (!unsigned_digits(value)) {
        throw std::invalid_argument(std::string{field} + " must be an unsigned integer");
    }
    std::size_t consumed = 0;
    const auto parsed = std::stoull(std::string{value}, &consumed, 10);
    if (consumed != value.size()) {
        throw std::invalid_argument(std::string{field} + " must be an unsigned integer");
    }
    return parsed;
}

[[nodiscard]] std::uint32_t parse_u32(std::string_view value, std::string_view field) {
    const auto parsed = parse_u64(value, field);
    if (parsed > (std::numeric_limits<std::uint32_t>::max)()) {
        throw std::invalid_argument(std::string{field} + " exceeds uint32 range");
    }
    return static_cast<std::uint32_t>(parsed);
}

[[nodiscard]] float parse_float(std::string_view value, std::string_view field) {
    const auto valid_character = [](char character) noexcept {
        return (character >= '0' && character <= '9') || character == '+' || character == '-' || character == '.' ||
               character == 'e' || character == 'E';
    };
    if (value.empty() || !std::ranges::all_of(value, valid_character)) {
        throw std::invalid_argument(std::string{field} + " must be a finite float");
    }

    float parsed = 0.0F;
    std::istringstream stream{std::string{value}};
    stream.imbue(std::locale::classic());
    stream >> std::noskipws >> parsed;

    char trailing = '\0';
    if (!stream || (stream >> trailing) || !finite(parsed)) {
        throw std::invalid_argument(std::string{field} + " must be a finite float");
    }
    return parsed;
}

[[nodiscard]] bool parse_bool(std::string_view value, std::string_view field) {
    if (value == "true") {
        return true;
    }
    if (value == "false") {
        return false;
    }
    throw std::invalid_argument(std::string{field} + " must be true or false");
}

[[nodiscard]] Vec3 parse_vec3(std::string_view value, std::string_view field) {
    const auto first_comma = value.find(',');
    const auto second_comma =
        first_comma == std::string_view::npos ? std::string_view::npos : value.find(',', first_comma + 1U);
    if (first_comma == std::string_view::npos || second_comma == std::string_view::npos ||
        value.find(',', second_comma + 1U) != std::string_view::npos) {
        throw std::invalid_argument(std::string{field} + " must have exactly three float components");
    }
    return Vec3{
        .x = parse_float(value.substr(0, first_comma), field),
        .y = parse_float(value.substr(first_comma + 1U, second_comma - first_comma - 1U), field),
        .z = parse_float(value.substr(second_comma + 1U), field),
    };
}

using KeyValues = std::unordered_map<std::string, std::string>;

[[nodiscard]] KeyValues parse_key_values(std::string_view text, std::string_view context) {
    KeyValues values;
    std::size_t line_start = 0;
    while (line_start < text.size()) {
        const auto line_end = text.find('\n', line_start);
        auto line = text.substr(line_start,
                                line_end == std::string_view::npos ? text.size() - line_start : line_end - line_start);
        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1U);
        }
        if (!line.empty()) {
            const auto separator = line.find('=');
            if (separator == std::string_view::npos || separator == 0U) {
                throw std::invalid_argument(std::string{context} + " has a malformed key/value line");
            }
            const auto key = std::string{line.substr(0, separator)};
            const auto value = std::string{line.substr(separator + 1U)};
            if (!values.emplace(key, value).second) {
                throw std::invalid_argument(std::string{context} + " contains a duplicated key");
            }
        }
        if (line_end == std::string_view::npos) {
            break;
        }
        line_start = line_end + 1U;
    }
    return values;
}

[[nodiscard]] std::string_view required_value(const KeyValues& values, std::string_view key, std::string_view context) {
    const auto it = values.find(std::string{key});
    if (it == values.end()) {
        throw std::invalid_argument(std::string{context} + " is missing " + std::string{key});
    }
    return it->second;
}

void validate_authoring_desc(std::vector<PhysicsCollisionPackageDiagnostic>& diagnostics,
                             const PhysicsCollisionPackageAuthoringDesc& desc) {
    if (desc.collision_asset.value == 0U) {
        add_diagnostic(diagnostics, "invalid_collision_asset", "collision asset id must be non-zero", desc.output_path);
    }
    if (!valid_output_path(desc.output_path)) {
        add_diagnostic(diagnostics, "unsafe_collision_path",
                       "collision output path must be a package-relative safe path", desc.output_path);
    }
    if (desc.source_revision == 0U) {
        add_diagnostic(diagnostics, "invalid_source_revision", "collision source revision must be non-zero",
                       desc.output_path);
    }
    if (desc.native_backend != "unsupported") {
        add_diagnostic(diagnostics, "unsupported_native_backend",
                       "native physics backend is not supported; backend.native must be unsupported", desc.output_path);
    }
    if (!finite_vec3(desc.world_config.gravity)) {
        add_diagnostic(diagnostics, "invalid_world_gravity", "collision world gravity must be finite",
                       desc.output_path);
    }
    if (desc.bodies.empty()) {
        add_diagnostic(diagnostics, "missing_collision_bodies", "collision scene requires at least one body",
                       desc.output_path);
    }
    if (desc.bodies.size() > max_collision_body_count) {
        add_diagnostic(diagnostics, "too_many_collision_bodies", "collision scene has too many bodies",
                       desc.output_path);
    }

    std::unordered_set<std::string> names;
    names.reserve(desc.bodies.size());
    for (std::size_t index = 0; index < desc.bodies.size(); ++index) {
        const auto& body = desc.bodies[index];
        if (!valid_payload_token(body.name, true)) {
            add_diagnostic(diagnostics, "invalid_collision_body_name",
                           "collision body name must be a non-empty line-oriented text value", desc.output_path, index);
        } else if (!names.insert(body.name).second) {
            add_diagnostic(diagnostics, "duplicate_collision_body_name", "collision body name is duplicated",
                           desc.output_path, index);
        }
        if (!valid_payload_token(body.material, false)) {
            add_diagnostic(diagnostics, "invalid_collision_material",
                           "collision body material must be a line-oriented text value", desc.output_path, index);
        }
        if (!valid_payload_token(body.compound, false)) {
            add_diagnostic(diagnostics, "invalid_collision_compound",
                           "collision body compound group must be a line-oriented text value", desc.output_path, index);
        }
        if (!is_valid_physics_body_desc(body.body)) {
            add_diagnostic(diagnostics, "invalid_collision_body", "collision body description is invalid",
                           desc.output_path, index);
        }
    }
}

[[nodiscard]] std::string make_collision_payload(const PhysicsCollisionPackageAuthoringDesc& desc) {
    std::ostringstream output;
    output << "format=GameEngine.PhysicsCollisionScene3D.v1\n";
    output << "asset.id=" << desc.collision_asset.value << '\n';
    output << "asset.kind=physics_collision_scene\n";
    output << "backend.native=" << desc.native_backend << '\n';
    output << "world.gravity=";
    write_vec3(output, desc.world_config.gravity);
    output << '\n';
    output << "body.count=" << desc.bodies.size() << '\n';
    for (std::size_t index = 0; index < desc.bodies.size(); ++index) {
        const auto& row = desc.bodies[index];
        const auto& body = row.body;
        output << "body." << index << ".name=" << row.name << '\n';
        output << "body." << index << ".shape=" << shape_name(body.shape) << '\n';
        output << "body." << index << ".position=";
        write_vec3(output, body.position);
        output << '\n';
        output << "body." << index << ".velocity=";
        write_vec3(output, body.velocity);
        output << '\n';
        output << "body." << index << ".dynamic=" << (body.dynamic ? "true" : "false") << '\n';
        output << "body." << index << ".mass=";
        write_float(output, body.mass);
        output << '\n';
        output << "body." << index << ".linear_damping=";
        write_float(output, body.linear_damping);
        output << '\n';
        output << "body." << index << ".half_extents=";
        write_vec3(output, body.half_extents);
        output << '\n';
        output << "body." << index << ".radius=";
        write_float(output, body.radius);
        output << '\n';
        output << "body." << index << ".half_height=";
        write_float(output, body.half_height);
        output << '\n';
        output << "body." << index << ".layer=" << body.collision_layer << '\n';
        output << "body." << index << ".mask=" << body.collision_mask << '\n';
        output << "body." << index << ".trigger=" << (body.trigger ? "true" : "false") << '\n';
        output << "body." << index << ".material=" << row.material << '\n';
        if (!row.compound.empty()) {
            output << "body." << index << ".compound=" << row.compound << '\n';
        }
    }
    return output.str();
}

void validate_payload_rows(std::vector<PhysicsCollisionPackageDiagnostic>& diagnostics, AssetId expected_asset,
                           std::string_view content) {
    try {
        const auto values = parse_key_values(content, "physics collision payload");
        if (required_value(values, "format", "physics collision payload") != "GameEngine.PhysicsCollisionScene3D.v1") {
            add_diagnostic(diagnostics, "unsupported_collision_payload_format",
                           "physics collision payload format is unsupported");
        }
        const AssetId asset{parse_u64(required_value(values, "asset.id", "physics collision payload"), "asset.id")};
        if (asset != expected_asset) {
            add_diagnostic(diagnostics, "collision_payload_asset_mismatch",
                           "physics collision payload asset id does not match requested asset");
        }
        if (required_value(values, "asset.kind", "physics collision payload") != "physics_collision_scene") {
            add_diagnostic(diagnostics, "collision_payload_kind_mismatch",
                           "physics collision payload kind must be physics_collision_scene");
        }
        if (required_value(values, "backend.native", "physics collision payload") != "unsupported") {
            add_diagnostic(diagnostics, "unsupported_native_backend",
                           "native physics backend is not supported; backend.native must be unsupported");
        }
        if (!finite_vec3(
                parse_vec3(required_value(values, "world.gravity", "physics collision payload"), "world.gravity"))) {
            add_diagnostic(diagnostics, "invalid_world_gravity", "collision world gravity must be finite");
        }

        const auto body_count =
            parse_u32(required_value(values, "body.count", "physics collision payload"), "body.count");
        if (body_count == 0U || body_count > max_collision_body_count) {
            add_diagnostic(diagnostics, "invalid_collision_body_count", "collision body count is invalid");
            return;
        }

        std::unordered_set<std::string> names;
        names.reserve(body_count);
        for (std::size_t index = 0; index < body_count; ++index) {
            const auto prefix = std::string{"body."} + std::to_string(index) + ".";
            const auto name = std::string{required_value(values, prefix + "name", "physics collision payload")};
            const auto material = std::string{required_value(values, prefix + "material", "physics collision payload")};
            std::string compound;
            if (const auto compound_it = values.find(prefix + "compound"); compound_it != values.end()) {
                compound = compound_it->second;
            }
            PhysicsBody3DDesc body;
            body.shape = parse_shape(required_value(values, prefix + "shape", "physics collision payload"));
            body.position = parse_vec3(required_value(values, prefix + "position", "physics collision payload"),
                                       prefix + "position");
            body.velocity = parse_vec3(required_value(values, prefix + "velocity", "physics collision payload"),
                                       prefix + "velocity");
            body.dynamic =
                parse_bool(required_value(values, prefix + "dynamic", "physics collision payload"), prefix + "dynamic");
            body.mass =
                parse_float(required_value(values, prefix + "mass", "physics collision payload"), prefix + "mass");
            body.linear_damping =
                parse_float(required_value(values, prefix + "linear_damping", "physics collision payload"),
                            prefix + "linear_damping");
            body.half_extents = parse_vec3(required_value(values, prefix + "half_extents", "physics collision payload"),
                                           prefix + "half_extents");
            body.radius =
                parse_float(required_value(values, prefix + "radius", "physics collision payload"), prefix + "radius");
            body.half_height = parse_float(required_value(values, prefix + "half_height", "physics collision payload"),
                                           prefix + "half_height");
            body.collision_layer =
                parse_u32(required_value(values, prefix + "layer", "physics collision payload"), prefix + "layer");
            body.collision_mask =
                parse_u32(required_value(values, prefix + "mask", "physics collision payload"), prefix + "mask");
            body.trigger =
                parse_bool(required_value(values, prefix + "trigger", "physics collision payload"), prefix + "trigger");

            if (!valid_payload_token(name, true)) {
                add_diagnostic(diagnostics, "invalid_collision_body_name",
                               "collision body name must be a non-empty line-oriented text value", {}, index);
            } else if (!names.insert(name).second) {
                add_diagnostic(diagnostics, "duplicate_collision_body_name", "collision body name is duplicated", {},
                               index);
            }
            if (!valid_payload_token(material, false)) {
                add_diagnostic(diagnostics, "invalid_collision_material",
                               "collision body material must be a line-oriented text value", {}, index);
            }
            if (!valid_payload_token(compound, false)) {
                add_diagnostic(diagnostics, "invalid_collision_compound",
                               "collision body compound group must be a line-oriented text value", {}, index);
            }
            if (!is_valid_physics_body_desc(body)) {
                add_diagnostic(diagnostics, "invalid_collision_body", "collision body description is invalid", {},
                               index);
            }
        }
    } catch (const std::exception& error) {
        add_diagnostic(diagnostics, "malformed_collision_payload",
                       std::string{"physics collision payload is invalid: "} + error.what());
    }
}

void validate_package_relative_path(std::vector<PhysicsCollisionPackageDiagnostic>& diagnostics, std::string code,
                                    std::string_view path, std::string_view field) {
    if (!valid_output_path(path)) {
        add_diagnostic(diagnostics, std::move(code), std::string{field} + " must be a package-relative safe path",
                       std::string{path});
    }
}

void validate_package_index_entry_paths(std::vector<PhysicsCollisionPackageDiagnostic>& diagnostics,
                                        const AssetCookedPackageIndex& index) {
    for (const auto& entry : index.entries) {
        validate_package_relative_path(diagnostics, "unsafe_package_index_entry_path", entry.path,
                                       "physics collision package index entry path");
    }
}

void append_changed_file(std::vector<PhysicsCollisionPackageChangedFile>& files, std::string path,
                         std::string content) {
    files.push_back(PhysicsCollisionPackageChangedFile{
        .path = std::move(path),
        .content = std::move(content),
        .content_hash = 0,
    });
    files.back().content_hash = hash_asset_cooked_content(files.back().content);
}

} // namespace

PhysicsCollisionPackageAuthoringResult
author_physics_collision_package_scene(const PhysicsCollisionPackageAuthoringDesc& desc) {
    PhysicsCollisionPackageAuthoringResult result;
    validate_authoring_desc(result.diagnostics, desc);
    if (!result.diagnostics.empty()) {
        sort_diagnostics(result.diagnostics);
        return result;
    }

    result.content = make_collision_payload(desc);
    result.artifact = AssetCookedArtifact{
        .asset = desc.collision_asset,
        .kind = AssetKind::physics_collision_scene,
        .path = desc.output_path,
        .content = result.content,
        .source_revision = desc.source_revision,
        .dependencies = {},
    };
    return result;
}

PhysicsCollisionPackageAuthoringResult
write_physics_collision_package_scene(IFileSystem& filesystem, const PhysicsCollisionPackageAuthoringDesc& desc) {
    auto result = author_physics_collision_package_scene(desc);
    if (!result.succeeded()) {
        return result;
    }

    try {
        filesystem.write_text(desc.output_path, result.content);
    } catch (const std::exception& error) {
        add_diagnostic(result.diagnostics, "write_collision_payload_failed",
                       std::string{"failed to write physics collision payload: "} + error.what(), desc.output_path);
        sort_diagnostics(result.diagnostics);
    }
    return result;
}

PhysicsCollisionPackageVerificationResult verify_physics_collision_package_scene(const AssetCookedPackageIndex& index,
                                                                                 AssetId collision_asset,
                                                                                 std::string_view collision_content) {
    PhysicsCollisionPackageVerificationResult result;
    validate_payload_rows(result.diagnostics, collision_asset, collision_content);

    const auto* entry = find_entry(index, collision_asset);
    if (entry == nullptr) {
        add_diagnostic(result.diagnostics, "missing_collision_package_entry",
                       "physics collision package entry is missing");
        sort_diagnostics(result.diagnostics);
        return result;
    }
    if (entry->kind != AssetKind::physics_collision_scene) {
        add_diagnostic(result.diagnostics, "wrong_collision_package_entry_kind",
                       "physics collision package entry kind must be physics_collision_scene", entry->path);
    }
    if (entry->content_hash != hash_asset_cooked_content(collision_content)) {
        add_diagnostic(result.diagnostics, "collision_content_hash_mismatch",
                       "physics collision content hash does not match package entry", entry->path);
    }
    if (!entry->dependencies.empty()) {
        add_diagnostic(result.diagnostics, "unexpected_collision_dependencies",
                       "physics collision package entry must not declare dependencies", entry->path);
    }
    for (const auto& edge : index.dependencies) {
        if (edge.asset == collision_asset) {
            add_diagnostic(result.diagnostics, "unexpected_collision_dependency_edge",
                           "physics collision package must not declare dependency edges", edge.path);
        }
    }

    sort_diagnostics(result.diagnostics);
    return result;
}

PhysicsCollisionPackageUpdateResult
plan_physics_collision_package_update(const PhysicsCollisionPackageUpdateDesc& desc) {
    PhysicsCollisionPackageUpdateResult result;
    validate_package_relative_path(result.diagnostics, "unsafe_package_index_path", desc.package_index_path,
                                   "physics collision package index path");
    validate_package_relative_path(result.diagnostics, "unsafe_collision_path", desc.collision.output_path,
                                   "collision output path");
    if (!desc.package_index_path.empty() && desc.package_index_path == desc.collision.output_path) {
        add_diagnostic(result.diagnostics, "aliased_collision_package_path",
                       "physics collision package index path must not alias collision output path",
                       desc.package_index_path);
    }
    if (!result.diagnostics.empty()) {
        sort_diagnostics(result.diagnostics);
        return result;
    }

    const auto authored = author_physics_collision_package_scene(desc.collision);
    if (!authored.succeeded()) {
        result.diagnostics = authored.diagnostics;
        sort_diagnostics(result.diagnostics);
        return result;
    }

    AssetCookedPackageIndex index;
    try {
        index = deserialize_asset_cooked_package_index(desc.package_index_content);
    } catch (const std::exception& error) {
        add_diagnostic(result.diagnostics, "invalid_collision_package_index",
                       std::string{"physics collision package index is invalid: "} + error.what(),
                       desc.package_index_path);
        sort_diagnostics(result.diagnostics);
        return result;
    }
    validate_package_index_entry_paths(result.diagnostics, index);
    if (!result.diagnostics.empty()) {
        sort_diagnostics(result.diagnostics);
        return result;
    }

    if (has_path_collision(index, desc.collision.collision_asset, desc.collision.output_path)) {
        add_diagnostic(result.diagnostics, "collision_path_conflict",
                       "physics collision output path collides with another package entry", desc.collision.output_path);
    }

    auto* entry = find_entry(index, desc.collision.collision_asset);
    if (entry != nullptr && entry->kind != AssetKind::physics_collision_scene) {
        add_diagnostic(result.diagnostics, "wrong_existing_collision_entry_kind",
                       "existing collision package entry kind must be physics_collision_scene", entry->path);
    }
    if (!result.diagnostics.empty()) {
        sort_diagnostics(result.diagnostics);
        return result;
    }

    if (entry == nullptr) {
        index.entries.push_back(AssetCookedPackageEntry{
            .asset = authored.artifact.asset,
            .kind = authored.artifact.kind,
            .path = authored.artifact.path,
            .content_hash = hash_asset_cooked_content(authored.content),
            .source_revision = authored.artifact.source_revision,
            .dependencies = authored.artifact.dependencies,
        });
    } else {
        entry->kind = authored.artifact.kind;
        entry->path = authored.artifact.path;
        entry->content_hash = hash_asset_cooked_content(authored.content);
        entry->source_revision = authored.artifact.source_revision;
        entry->dependencies = authored.artifact.dependencies;
    }

    const auto removed_edges = std::ranges::remove_if(
        index.dependencies,
        [asset = desc.collision.collision_asset](const AssetDependencyEdge& edge) { return edge.asset == asset; });
    index.dependencies.erase(removed_edges.begin(), removed_edges.end());
    canonicalize_package_index(index);

    const auto verification =
        verify_physics_collision_package_scene(index, desc.collision.collision_asset, authored.content);
    if (!verification.succeeded()) {
        result.diagnostics = verification.diagnostics;
        sort_diagnostics(result.diagnostics);
        return result;
    }

    result.collision_content = authored.content;
    result.package_index_content = serialize_asset_cooked_package_index(index);
    append_changed_file(result.changed_files, desc.collision.output_path, result.collision_content);
    append_changed_file(result.changed_files, desc.package_index_path, result.package_index_content);
    return result;
}

PhysicsCollisionPackageUpdateResult
apply_physics_collision_package_update(IFileSystem& filesystem, const PhysicsCollisionPackageApplyDesc& desc) {
    PhysicsCollisionPackageUpdateResult result;
    std::string package_index_content;
    try {
        package_index_content = filesystem.read_text(desc.package_index_path);
    } catch (const std::exception& error) {
        add_diagnostic(result.diagnostics, "read_collision_package_index_failed",
                       std::string{"failed to read physics collision package index: "} + error.what(),
                       desc.package_index_path);
        sort_diagnostics(result.diagnostics);
        return result;
    }

    const std::string previous_package_index_content = package_index_content;
    result = plan_physics_collision_package_update(PhysicsCollisionPackageUpdateDesc{
        .package_index_path = desc.package_index_path,
        .package_index_content = package_index_content,
        .collision = desc.collision,
    });
    if (!result.succeeded()) {
        return result;
    }

    bool had_collision_content = false;
    std::string previous_collision_content;
    try {
        had_collision_content = filesystem.exists(desc.collision.output_path);
        if (had_collision_content) {
            previous_collision_content = filesystem.read_text(desc.collision.output_path);
        }
    } catch (const std::exception& error) {
        add_diagnostic(result.diagnostics, "read_existing_collision_payload_failed",
                       std::string{"failed to read existing physics collision payload before update: "} + error.what(),
                       desc.collision.output_path);
        sort_diagnostics(result.diagnostics);
        return result;
    }

    bool collision_written = false;
    try {
        filesystem.write_text(desc.collision.output_path, result.collision_content);
        collision_written = true;
        filesystem.write_text(desc.package_index_path, result.package_index_content);
    } catch (const std::exception& error) {
        if (collision_written) {
            try {
                if (had_collision_content) {
                    filesystem.write_text(desc.collision.output_path, previous_collision_content);
                } else {
                    filesystem.remove(desc.collision.output_path);
                }
                filesystem.write_text(desc.package_index_path, previous_package_index_content);
            } catch (const std::exception& rollback_error) {
                add_diagnostic(result.diagnostics, "rollback_collision_package_update_failed",
                               std::string{"failed to roll back physics collision package update: "} +
                                   rollback_error.what(),
                               desc.package_index_path);
            }
        }
        add_diagnostic(result.diagnostics, "write_collision_package_update_failed",
                       std::string{"failed to write physics collision package update: "} + error.what(),
                       desc.package_index_path);
        sort_diagnostics(result.diagnostics);
    }
    return result;
}

} // namespace mirakana
