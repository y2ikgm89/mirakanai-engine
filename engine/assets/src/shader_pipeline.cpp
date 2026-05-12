// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/shader_pipeline.hpp"

#include "mirakana/assets/asset_dependency_graph.hpp"
#include "mirakana/assets/shader_metadata.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace mirakana {
namespace {

constexpr std::string_view shader_artifact_manifest_format = "GameEngine.ShaderArtifacts.v1";

[[nodiscard]] bool valid_token(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos;
}

[[nodiscard]] bool contains_path_separator(std::string_view value) noexcept {
    return value.find('/') != std::string_view::npos || value.find('\\') != std::string_view::npos;
}

[[nodiscard]] bool is_absolute_or_parent_relative(std::string_view path) noexcept {
    if (path.empty()) {
        return true;
    }
    if (path.size() >= 2U && std::isalpha(static_cast<unsigned char>(path[0])) != 0 && path[1] == ':') {
        return true;
    }
    if (path[0] == '/' || path[0] == '\\') {
        return true;
    }
    std::size_t start = 0;
    while (start <= path.size()) {
        const auto separator = path.find_first_of("/\\", start);
        const auto part =
            path.substr(start, separator == std::string_view::npos ? path.size() - start : separator - start);
        if (part == "..") {
            return true;
        }
        if (separator == std::string_view::npos) {
            break;
        }
        start = separator + 1U;
    }
    return false;
}

[[nodiscard]] bool valid_argument(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos &&
           value.find('\0') == std::string_view::npos;
}

[[nodiscard]] bool allowed_tool_executable(std::string_view executable) noexcept {
    return executable == "dxc" || executable == "metal" || executable == "metallib";
}

[[nodiscard]] bool valid_target(ShaderCompileTarget target) noexcept {
    switch (target) {
    case ShaderCompileTarget::d3d12_dxil:
    case ShaderCompileTarget::vulkan_spirv:
    case ShaderCompileTarget::metal_ir:
    case ShaderCompileTarget::metal_library:
        return true;
    case ShaderCompileTarget::unknown:
        break;
    }
    return false;
}

[[nodiscard]] ShaderArtifactFormat artifact_format_for_target(ShaderCompileTarget target) noexcept {
    switch (target) {
    case ShaderCompileTarget::d3d12_dxil:
        return ShaderArtifactFormat::dxil;
    case ShaderCompileTarget::vulkan_spirv:
        return ShaderArtifactFormat::spirv;
    case ShaderCompileTarget::metal_ir:
        return ShaderArtifactFormat::metal_ir;
    case ShaderCompileTarget::metal_library:
        return ShaderArtifactFormat::metallib;
    case ShaderCompileTarget::unknown:
        break;
    }
    return ShaderArtifactFormat::unknown;
}

[[nodiscard]] bool target_accepts_language(ShaderCompileTarget target, ShaderSourceLanguage language) noexcept {
    switch (target) {
    case ShaderCompileTarget::d3d12_dxil:
    case ShaderCompileTarget::vulkan_spirv:
        return language == ShaderSourceLanguage::hlsl;
    case ShaderCompileTarget::metal_ir:
    case ShaderCompileTarget::metal_library:
        return language == ShaderSourceLanguage::msl;
    case ShaderCompileTarget::unknown:
        break;
    }
    return false;
}

[[nodiscard]] std::string_view language_name(ShaderSourceLanguage language) noexcept {
    switch (language) {
    case ShaderSourceLanguage::hlsl:
        return "hlsl";
    case ShaderSourceLanguage::glsl:
        return "glsl";
    case ShaderSourceLanguage::msl:
        return "msl";
    case ShaderSourceLanguage::wgsl:
        return "wgsl";
    case ShaderSourceLanguage::unknown:
        break;
    }
    return "unknown";
}

[[nodiscard]] ShaderSourceLanguage parse_language(std::string_view value) {
    if (value == "hlsl") {
        return ShaderSourceLanguage::hlsl;
    }
    if (value == "glsl") {
        return ShaderSourceLanguage::glsl;
    }
    if (value == "msl") {
        return ShaderSourceLanguage::msl;
    }
    if (value == "wgsl") {
        return ShaderSourceLanguage::wgsl;
    }
    throw std::invalid_argument("unsupported shader source language");
}

[[nodiscard]] std::string_view stage_name(ShaderSourceStage stage) noexcept {
    switch (stage) {
    case ShaderSourceStage::vertex:
        return "vertex";
    case ShaderSourceStage::fragment:
        return "fragment";
    case ShaderSourceStage::compute:
        return "compute";
    case ShaderSourceStage::unknown:
        break;
    }
    return "unknown";
}

[[nodiscard]] ShaderSourceStage parse_stage(std::string_view value) {
    if (value == "vertex") {
        return ShaderSourceStage::vertex;
    }
    if (value == "fragment") {
        return ShaderSourceStage::fragment;
    }
    if (value == "compute") {
        return ShaderSourceStage::compute;
    }
    throw std::invalid_argument("unsupported shader source stage");
}

[[nodiscard]] std::string_view artifact_format_name(ShaderArtifactFormat format) noexcept {
    switch (format) {
    case ShaderArtifactFormat::dxil:
        return "dxil";
    case ShaderArtifactFormat::spirv:
        return "spirv";
    case ShaderArtifactFormat::metal_ir:
        return "metal_ir";
    case ShaderArtifactFormat::metallib:
        return "metallib";
    case ShaderArtifactFormat::unknown:
        break;
    }
    return "unknown";
}

[[nodiscard]] ShaderArtifactFormat parse_artifact_format(std::string_view value) {
    if (value == "dxil") {
        return ShaderArtifactFormat::dxil;
    }
    if (value == "spirv") {
        return ShaderArtifactFormat::spirv;
    }
    if (value == "metal_ir") {
        return ShaderArtifactFormat::metal_ir;
    }
    if (value == "metallib") {
        return ShaderArtifactFormat::metallib;
    }
    throw std::invalid_argument("unsupported shader artifact format");
}

[[nodiscard]] std::string_view descriptor_resource_kind_name(ShaderDescriptorResourceKind kind) noexcept {
    switch (kind) {
    case ShaderDescriptorResourceKind::uniform_buffer:
        return "uniform_buffer";
    case ShaderDescriptorResourceKind::storage_buffer:
        return "storage_buffer";
    case ShaderDescriptorResourceKind::sampled_texture:
        return "sampled_texture";
    case ShaderDescriptorResourceKind::storage_texture:
        return "storage_texture";
    case ShaderDescriptorResourceKind::sampler:
        return "sampler";
    case ShaderDescriptorResourceKind::unknown:
        break;
    }
    return "unknown";
}

[[nodiscard]] ShaderDescriptorResourceKind parse_descriptor_resource_kind(std::string_view value) {
    if (value == "uniform_buffer") {
        return ShaderDescriptorResourceKind::uniform_buffer;
    }
    if (value == "storage_buffer") {
        return ShaderDescriptorResourceKind::storage_buffer;
    }
    if (value == "sampled_texture") {
        return ShaderDescriptorResourceKind::sampled_texture;
    }
    if (value == "storage_texture") {
        return ShaderDescriptorResourceKind::storage_texture;
    }
    if (value == "sampler") {
        return ShaderDescriptorResourceKind::sampler;
    }
    throw std::invalid_argument("unsupported shader descriptor reflection resource kind");
}

[[nodiscard]] std::vector<ShaderDescriptorReflection>
sorted_reflection(std::vector<ShaderDescriptorReflection> reflection) {
    std::ranges::sort(reflection, [](const auto& lhs, const auto& rhs) {
        if (lhs.set != rhs.set) {
            return lhs.set < rhs.set;
        }
        return lhs.binding < rhs.binding;
    });
    return reflection;
}

[[nodiscard]] std::vector<ShaderSourceMetadata> sorted_shaders(std::vector<ShaderSourceMetadata> shaders) {
    std::ranges::sort(shaders, [](const ShaderSourceMetadata& lhs, const ShaderSourceMetadata& rhs) {
        return lhs.source_path < rhs.source_path;
    });
    return shaders;
}

[[nodiscard]] std::string strip_comments_from_line(std::string_view line, bool& in_block_comment) {
    std::string result;
    for (std::size_t index = 0; index < line.size();) {
        if (in_block_comment) {
            const auto end = line.find("*/", index);
            if (end == std::string_view::npos) {
                return result;
            }
            in_block_comment = false;
            index = end + 2U;
            continue;
        }

        if (index + 1U < line.size() && line[index] == '/' && line[index + 1U] == '/') {
            return result;
        }
        if (index + 1U < line.size() && line[index] == '/' && line[index + 1U] == '*') {
            in_block_comment = true;
            index += 2U;
            continue;
        }

        result.push_back(line[index]);
        ++index;
    }
    return result;
}

[[nodiscard]] std::string_view trim_left(std::string_view value) noexcept {
    std::size_t index = 0;
    while (index < value.size() && std::isspace(static_cast<unsigned char>(value[index])) != 0) {
        ++index;
    }
    return value.substr(index);
}

[[nodiscard]] bool starts_with(std::string_view value, std::string_view prefix) noexcept {
    return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
}

[[nodiscard]] bool dependency_exists(const std::vector<ShaderSourceDependency>& dependencies,
                                     std::string_view path) noexcept {
    return std::ranges::find_if(dependencies, [path](const ShaderSourceDependency& dependency) {
               return dependency.path == path;
           }) != dependencies.end();
}

[[nodiscard]] std::string source_directory(std::string_view source_path) {
    const auto separator = source_path.find_last_of("/\\");
    if (separator == std::string_view::npos) {
        return {};
    }
    return std::string(source_path.substr(0, separator));
}

[[nodiscard]] std::string join_asset_path(std::string_view parent, std::string_view child) {
    if (parent.empty()) {
        return std::string(child);
    }
    std::string result(parent);
    result.push_back('/');
    result.append(child);
    return result;
}

[[nodiscard]] bool edge_path_exists(const std::vector<AssetDependencyEdge>& edges, std::string_view path) noexcept {
    return std::ranges::find_if(edges, [path](const AssetDependencyEdge& edge) { return edge.path == path; }) !=
           edges.end();
}

[[nodiscard]] std::unordered_map<std::string, std::string> parse_key_values(std::string_view text) {
    std::unordered_map<std::string, std::string> values;
    std::size_t line_start = 0;
    while (line_start < text.size()) {
        const auto line_end = text.find('\n', line_start);
        const auto raw_line = text.substr(line_start, line_end == std::string_view::npos ? text.size() - line_start
                                                                                         : line_end - line_start);
        line_start = line_end == std::string_view::npos ? text.size() : line_end + 1U;

        if (raw_line.empty()) {
            continue;
        }
        const auto separator = raw_line.find('=');
        if (separator == std::string_view::npos) {
            throw std::invalid_argument("shader artifact manifest line is missing '='");
        }
        const auto key = std::string(raw_line.substr(0, separator));
        const auto value = std::string(raw_line.substr(separator + 1U));
        auto [_, inserted] = values.emplace(key, value);
        if (!inserted) {
            throw std::invalid_argument("shader artifact manifest contains duplicate keys");
        }
    }
    return values;
}

[[nodiscard]] const std::string& required_value(const std::unordered_map<std::string, std::string>& values,
                                                const std::string& key) {
    const auto it = values.find(key);
    if (it == values.end()) {
        throw std::invalid_argument("shader artifact manifest is missing key: " + key);
    }
    return it->second;
}

[[nodiscard]] std::size_t parse_count(const std::unordered_map<std::string, std::string>& values,
                                      const std::string& key) {
    const auto& value = required_value(values, key);
    std::size_t parsed = 0;
    try {
        parsed = static_cast<std::size_t>(std::stoull(value));
    } catch (const std::exception&) {
        throw std::invalid_argument("shader artifact manifest count is invalid: " + key);
    }
    return parsed;
}

[[nodiscard]] std::uint32_t parse_u32(const std::unordered_map<std::string, std::string>& values,
                                      const std::string& key) {
    const auto parsed = parse_count(values, key);
    if (parsed > static_cast<std::size_t>((std::numeric_limits<std::uint32_t>::max)())) {
        throw std::invalid_argument("shader artifact manifest value is outside uint32 range: " + key);
    }
    return static_cast<std::uint32_t>(parsed);
}

[[nodiscard]] AssetId parse_asset_id(const std::unordered_map<std::string, std::string>& values,
                                     const std::string& key) {
    const auto& value = required_value(values, key);
    try {
        return AssetId{static_cast<std::uint64_t>(std::stoull(value))};
    } catch (const std::exception&) {
        throw std::invalid_argument("shader artifact manifest asset id is invalid: " + key);
    }
}

} // namespace

bool is_valid_shader_compile_request(const ShaderCompileRequest& request) noexcept {
    return is_valid_shader_source_metadata(request.source) && valid_target(request.target) &&
           target_accepts_language(request.target, request.source.language) && valid_token(request.output_path) &&
           valid_token(request.profile);
}

ShaderToolRunResult RecordingShaderToolRunner::run(const ShaderCompileCommand& command) {
    commands_.push_back(command);
    return ShaderToolRunResult{
        .exit_code = 0,
        .diagnostic = "recorded",
        .stdout_text = {},
        .stderr_text = {},
        .artifact = command.artifact,
    };
}

const std::vector<ShaderCompileCommand>& RecordingShaderToolRunner::commands() const noexcept {
    return commands_;
}

ShaderCompileCommand make_shader_compile_command(const ShaderCompileRequest& request) {
    if (!is_valid_shader_compile_request(request)) {
        throw std::invalid_argument("shader compile request is invalid");
    }

    ShaderCompileCommand command;
    command.artifact = ShaderGeneratedArtifact{
        .path = request.output_path,
        .format = artifact_format_for_target(request.target),
        .profile = request.profile,
        .entry_point = request.source.entry_point,
    };

    if (request.target == ShaderCompileTarget::d3d12_dxil || request.target == ShaderCompileTarget::vulkan_spirv) {
        command.executable = "dxc";
        if (request.target == ShaderCompileTarget::vulkan_spirv) {
            command.arguments.emplace_back("-spirv");
            command.arguments.emplace_back("-fspv-target-env=vulkan1.3");
        }
        command.arguments.emplace_back("-T");
        command.arguments.push_back(request.profile);
        command.arguments.emplace_back("-E");
        command.arguments.push_back(request.source.entry_point);
        command.arguments.emplace_back("-Fo");
        command.arguments.push_back(request.output_path);
        for (const auto& include_path : request.include_paths) {
            command.arguments.emplace_back("-I");
            command.arguments.push_back(include_path);
        }
        for (const auto& define : request.source.defines) {
            command.arguments.emplace_back("-D");
            command.arguments.push_back(define);
        }
        if (request.debug_symbols) {
            command.arguments.emplace_back("-Zi");
        }
        if (!request.optimize) {
            command.arguments.emplace_back("-Od");
        } else {
            command.arguments.emplace_back("-O3");
        }
        command.arguments.push_back(request.source.source_path);
        return command;
    }

    if (request.target == ShaderCompileTarget::metal_ir) {
        command.executable = "metal";
        command.arguments.emplace_back("-c");
        command.arguments.push_back(request.source.source_path);
        command.arguments.emplace_back("-o");
        command.arguments.push_back(request.output_path);
        if (request.debug_symbols) {
            command.arguments.emplace_back("-gline-tables-only");
        }
        return command;
    }

    command.executable = "metallib";
    command.arguments.push_back(request.source.source_path);
    command.arguments.emplace_back("-o");
    command.arguments.push_back(request.output_path);
    return command;
}

bool is_safe_shader_tool_command(const ShaderCompileCommand& command) noexcept {
    if (!allowed_tool_executable(command.executable) || contains_path_separator(command.executable)) {
        return false;
    }
    if (!is_valid_shader_generated_artifact(command.artifact) ||
        is_absolute_or_parent_relative(command.artifact.path)) {
        return false;
    }
    return std::ranges::all_of(command.arguments, [](const auto& argument) { return valid_argument(argument); });
}

ShaderToolRunResult run_shader_tool_command(IShaderToolRunner& runner, const ShaderCompileCommand& command) {
    if (!is_safe_shader_tool_command(command)) {
        throw std::invalid_argument("shader tool command is unsafe");
    }
    return runner.run(command);
}

std::vector<ShaderSourceDependency> discover_shader_source_dependencies(std::string_view source_text) {
    std::vector<ShaderSourceDependency> dependencies;
    bool in_block_comment = false;
    std::size_t line_start = 0;
    while (line_start < source_text.size()) {
        const auto line_end = source_text.find('\n', line_start);
        const auto raw_line = source_text.substr(
            line_start, line_end == std::string_view::npos ? source_text.size() - line_start : line_end - line_start);
        line_start = line_end == std::string_view::npos ? source_text.size() : line_end + 1U;

        const auto uncommented = strip_comments_from_line(raw_line, in_block_comment);
        auto line = trim_left(uncommented);
        if (!starts_with(line, "#include")) {
            continue;
        }
        line = trim_left(line.substr(std::string_view{"#include"}.size()));
        if (line.size() < 3U) {
            continue;
        }

        const auto delimiter = line[0];
        const char close_delimiter = delimiter == '"' ? '"' : '>';
        const auto kind = delimiter == '"' ? ShaderIncludeKind::quoted : ShaderIncludeKind::system;
        if (delimiter != '"' && delimiter != '<') {
            continue;
        }

        const auto end = line.find(close_delimiter, 1U);
        if (end == std::string_view::npos || end <= 1U) {
            continue;
        }

        auto path = std::string(line.substr(1U, end - 1U));
        if (!valid_token(path) || dependency_exists(dependencies, path)) {
            continue;
        }
        dependencies.push_back(ShaderSourceDependency{
            .path = std::move(path),
            .kind = kind,
        });
    }
    return dependencies;
}

std::vector<AssetDependencyEdge> make_shader_include_dependency_edges(const ShaderSourceMetadata& source,
                                                                      std::string_view source_text) {
    if (!is_valid_shader_source_metadata(source)) {
        throw std::invalid_argument("shader source metadata is invalid");
    }

    const auto dependencies = discover_shader_source_dependencies(source_text);
    const auto source_parent = source_directory(source.source_path);
    std::vector<AssetDependencyEdge> edges;
    edges.reserve(dependencies.size());
    for (const auto& dependency : dependencies) {
        if (is_absolute_or_parent_relative(dependency.path)) {
            throw std::invalid_argument("shader include dependency path is unsafe");
        }

        const auto resolved_path = dependency.kind == ShaderIncludeKind::quoted
                                       ? join_asset_path(source_parent, dependency.path)
                                       : dependency.path;
        if (edge_path_exists(edges, resolved_path)) {
            continue;
        }

        AssetDependencyEdge edge{
            .asset = source.id,
            .dependency = AssetId::from_name(resolved_path),
            .kind = AssetDependencyKind::shader_include,
            .path = resolved_path,
        };
        if (!is_valid_asset_dependency_edge(edge)) {
            throw std::invalid_argument("shader include dependency edge is invalid");
        }
        edges.push_back(std::move(edge));
    }
    return edges;
}

std::string serialize_shader_artifact_manifest(const std::vector<ShaderSourceMetadata>& shaders) {
    const auto sorted = sorted_shaders(shaders);
    std::ostringstream output;
    output << "format=" << shader_artifact_manifest_format << '\n';
    output << "shader.count=" << sorted.size() << '\n';
    for (std::size_t shader_index = 0; shader_index < sorted.size(); ++shader_index) {
        const auto ordinal = shader_index + 1U;
        const auto& shader = sorted[shader_index];
        if (!is_valid_shader_source_metadata(shader)) {
            throw std::invalid_argument("shader artifact manifest contains invalid shader metadata");
        }
        output << "shader." << ordinal << ".id=" << shader.id.value << '\n';
        output << "shader." << ordinal << ".source=" << shader.source_path << '\n';
        output << "shader." << ordinal << ".language=" << language_name(shader.language) << '\n';
        output << "shader." << ordinal << ".stage=" << stage_name(shader.stage) << '\n';
        output << "shader." << ordinal << ".entry=" << shader.entry_point << '\n';
        output << "shader." << ordinal << ".define.count=" << shader.defines.size() << '\n';
        for (std::size_t define_index = 0; define_index < shader.defines.size(); ++define_index) {
            output << "shader." << ordinal << ".define." << define_index + 1U << '=' << shader.defines[define_index]
                   << '\n';
        }
        output << "shader." << ordinal << ".artifact.count=" << shader.artifacts.size() << '\n';
        for (std::size_t artifact_index = 0; artifact_index < shader.artifacts.size(); ++artifact_index) {
            const auto artifact_ordinal = artifact_index + 1U;
            const auto& artifact = shader.artifacts[artifact_index];
            if (!is_valid_shader_generated_artifact(artifact)) {
                throw std::invalid_argument("shader artifact manifest contains invalid generated artifact");
            }
            output << "shader." << ordinal << ".artifact." << artifact_ordinal << ".path=" << artifact.path << '\n';
            output << "shader." << ordinal << ".artifact." << artifact_ordinal
                   << ".format=" << artifact_format_name(artifact.format) << '\n';
            output << "shader." << ordinal << ".artifact." << artifact_ordinal << ".profile=" << artifact.profile
                   << '\n';
            output << "shader." << ordinal << ".artifact." << artifact_ordinal << ".entry=" << artifact.entry_point
                   << '\n';
        }
        const auto reflection = sorted_reflection(shader.reflection);
        output << "shader." << ordinal << ".reflection.count=" << reflection.size() << '\n';
        for (std::size_t reflection_index = 0; reflection_index < reflection.size(); ++reflection_index) {
            const auto reflection_ordinal = reflection_index + 1U;
            const auto& item = reflection[reflection_index];
            if (!is_valid_shader_descriptor_reflection(item)) {
                throw std::invalid_argument("shader artifact manifest contains invalid descriptor reflection");
            }
            if (reflection_index > 0 && reflection[reflection_index - 1U].set == item.set &&
                reflection[reflection_index - 1U].binding == item.binding) {
                throw std::invalid_argument("shader artifact manifest contains duplicate descriptor reflection");
            }
            output << "shader." << ordinal << ".reflection." << reflection_ordinal << ".set=" << item.set << '\n';
            output << "shader." << ordinal << ".reflection." << reflection_ordinal << ".binding=" << item.binding
                   << '\n';
            output << "shader." << ordinal << ".reflection." << reflection_ordinal
                   << ".kind=" << descriptor_resource_kind_name(item.resource_kind) << '\n';
            output << "shader." << ordinal << ".reflection." << reflection_ordinal
                   << ".stage=" << stage_name(item.stage) << '\n';
            output << "shader." << ordinal << ".reflection." << reflection_ordinal << ".count=" << item.count << '\n';
            output << "shader." << ordinal << ".reflection." << reflection_ordinal << ".semantic=" << item.semantic
                   << '\n';
        }
    }
    return output.str();
}

std::vector<ShaderSourceMetadata> deserialize_shader_artifact_manifest(std::string_view text) {
    const auto values = parse_key_values(text);
    if (required_value(values, "format") != shader_artifact_manifest_format) {
        throw std::invalid_argument("shader artifact manifest format is unsupported");
    }

    std::vector<ShaderSourceMetadata> shaders;
    const auto shader_count = parse_count(values, "shader.count");
    shaders.reserve(shader_count);
    for (std::size_t shader_index = 0; shader_index < shader_count; ++shader_index) {
        const auto ordinal = shader_index + 1U;
        const auto prefix = std::string("shader.") + std::to_string(ordinal);
        ShaderSourceMetadata shader{
            .id = parse_asset_id(values, prefix + ".id"),
            .source_path = required_value(values, prefix + ".source"),
            .language = parse_language(required_value(values, prefix + ".language")),
            .stage = parse_stage(required_value(values, prefix + ".stage")),
            .entry_point = required_value(values, prefix + ".entry"),
        };

        const auto define_count = parse_count(values, prefix + ".define.count");
        shader.defines.reserve(define_count);
        for (std::size_t define_index = 0; define_index < define_count; ++define_index) {
            shader.defines.push_back(required_value(values, prefix + ".define." + std::to_string(define_index + 1U)));
        }

        const auto artifact_count = parse_count(values, prefix + ".artifact.count");
        shader.artifacts.reserve(artifact_count);
        for (std::size_t artifact_index = 0; artifact_index < artifact_count; ++artifact_index) {
            const auto artifact_prefix = prefix + ".artifact." + std::to_string(artifact_index + 1U);
            shader.artifacts.push_back(ShaderGeneratedArtifact{
                .path = required_value(values, artifact_prefix + ".path"),
                .format = parse_artifact_format(required_value(values, artifact_prefix + ".format")),
                .profile = required_value(values, artifact_prefix + ".profile"),
                .entry_point = required_value(values, artifact_prefix + ".entry"),
            });
        }

        const auto reflection_count = parse_count(values, prefix + ".reflection.count");
        shader.reflection.reserve(reflection_count);
        for (std::size_t reflection_index = 0; reflection_index < reflection_count; ++reflection_index) {
            const auto reflection_prefix = prefix + ".reflection." + std::to_string(reflection_index + 1U);
            shader.reflection.push_back(ShaderDescriptorReflection{
                .set = parse_u32(values, reflection_prefix + ".set"),
                .binding = parse_u32(values, reflection_prefix + ".binding"),
                .resource_kind = parse_descriptor_resource_kind(required_value(values, reflection_prefix + ".kind")),
                .stage = parse_stage(required_value(values, reflection_prefix + ".stage")),
                .count = parse_u32(values, reflection_prefix + ".count"),
                .semantic = required_value(values, reflection_prefix + ".semantic"),
            });
        }
        shader.reflection = sorted_reflection(std::move(shader.reflection));

        if (!is_valid_shader_source_metadata(shader)) {
            throw std::invalid_argument("shader artifact manifest contains invalid shader metadata");
        }
        for (const auto& artifact : shader.artifacts) {
            if (!is_valid_shader_generated_artifact(artifact)) {
                throw std::invalid_argument("shader artifact manifest contains invalid generated artifact");
            }
        }
        for (std::size_t reflection_index = 0; reflection_index < shader.reflection.size(); ++reflection_index) {
            if (!is_valid_shader_descriptor_reflection(shader.reflection[reflection_index])) {
                throw std::invalid_argument("shader artifact manifest contains invalid descriptor reflection");
            }
            if (reflection_index > 0 &&
                shader.reflection[reflection_index - 1U].set == shader.reflection[reflection_index].set &&
                shader.reflection[reflection_index - 1U].binding == shader.reflection[reflection_index].binding) {
                throw std::invalid_argument("shader artifact manifest contains duplicate descriptor reflection");
            }
        }
        shaders.push_back(std::move(shader));
    }
    return sorted_shaders(std::move(shaders));
}

} // namespace mirakana
