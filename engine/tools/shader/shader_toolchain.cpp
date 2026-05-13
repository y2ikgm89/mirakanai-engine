// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/shader_toolchain.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace mirakana {
namespace {

constexpr std::string_view shader_provenance_format = "GameEngine.ShaderArtifactProvenance.v1";
constexpr std::string_view shader_cache_index_format = "GameEngine.ShaderArtifactCacheIndex.v1";

struct ToolProbe {
    ShaderToolKind kind{ShaderToolKind::unknown};
    std::string_view executable;
};

constexpr std::array<ToolProbe, 4> tool_probes{
    ToolProbe{.kind = ShaderToolKind::dxc, .executable = "dxc"},
    {.kind = ShaderToolKind::spirv_val, .executable = "spirv-val"},
    {.kind = ShaderToolKind::metal, .executable = "metal"},
    {.kind = ShaderToolKind::metallib, .executable = "metallib"},
};

[[nodiscard]] bool valid_token(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos &&
           value.find('\0') == std::string_view::npos && value.find('=') == std::string_view::npos;
}

[[nodiscard]] bool valid_optional_token(std::string_view value) noexcept {
    return value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos &&
           value.find('\0') == std::string_view::npos && value.find('=') == std::string_view::npos;
}

[[nodiscard]] bool valid_argument(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos &&
           value.find('\0') == std::string_view::npos;
}

[[nodiscard]] bool contains_path_separator(std::string_view value) noexcept {
    return value.find('/') != std::string_view::npos || value.find('\\') != std::string_view::npos;
}

[[nodiscard]] bool allowed_shader_validator_executable(std::string_view executable) noexcept {
    return executable == "spirv-val";
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

[[nodiscard]] std::string normalize_path(std::string_view path) {
    std::string result(path);
    for (auto& character : result) {
        if (character == '\\') {
            character = '/';
        }
    }
    while (!result.empty() && result.back() == '/') {
        result.pop_back();
    }
    return result;
}

[[nodiscard]] std::string join_path(std::string_view root, std::string_view child) {
    const auto normalized_root = normalize_path(root);
    if (normalized_root.empty()) {
        return std::string(child);
    }
    return normalized_root + "/" + std::string(child);
}

[[nodiscard]] std::string source_directory(std::string_view source_path) {
    const auto separator = source_path.find_last_of("/\\");
    if (separator == std::string_view::npos) {
        return {};
    }
    return std::string(source_path.substr(0, separator));
}

[[nodiscard]] std::string trim_version(std::string version) {
    while (!version.empty() && std::isspace(static_cast<unsigned char>(version.back())) != 0) {
        version.pop_back();
    }
    std::size_t start = 0;
    while (start < version.size() && std::isspace(static_cast<unsigned char>(version[start])) != 0) {
        ++start;
    }
    if (start > 0) {
        version.erase(0, start);
    }
    return version.empty() ? std::string("unknown") : version;
}

[[nodiscard]] int tool_order(ShaderToolKind kind) noexcept {
    switch (kind) {
    case ShaderToolKind::dxc:
        return 0;
    case ShaderToolKind::spirv_val:
        return 1;
    case ShaderToolKind::metal:
        return 2;
    case ShaderToolKind::metallib:
        return 3;
    case ShaderToolKind::unknown:
        break;
    }
    return 4;
}

[[nodiscard]] std::uint64_t fnv1a(std::string_view value) noexcept {
    std::uint64_t hash = 14695981039346656037ULL;
    for (const auto character : value) {
        hash ^= static_cast<unsigned char>(character);
        hash *= 1099511628211ULL;
    }
    return hash;
}

[[nodiscard]] std::string digest_text(std::string_view value) {
    std::ostringstream output;
    output << std::hex << std::setw(16) << std::setfill('0') << fnv1a(value);
    return output.str();
}

[[nodiscard]] std::string artifact_format_name(ShaderArtifactFormat format) noexcept {
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

[[nodiscard]] std::string compile_target_name(ShaderCompileTarget target) noexcept {
    switch (target) {
    case ShaderCompileTarget::d3d12_dxil:
        return "d3d12_dxil";
    case ShaderCompileTarget::vulkan_spirv:
        return "vulkan_spirv";
    case ShaderCompileTarget::metal_ir:
        return "metal_ir";
    case ShaderCompileTarget::metal_library:
        return "metal_library";
    case ShaderCompileTarget::unknown:
        break;
    }
    return "unknown";
}

[[nodiscard]] ShaderCompileTarget parse_compile_target(std::string_view value) {
    if (value == "d3d12_dxil") {
        return ShaderCompileTarget::d3d12_dxil;
    }
    if (value == "vulkan_spirv") {
        return ShaderCompileTarget::vulkan_spirv;
    }
    if (value == "metal_ir") {
        return ShaderCompileTarget::metal_ir;
    }
    if (value == "metal_library") {
        return ShaderCompileTarget::metal_library;
    }
    throw std::invalid_argument("unsupported shader compile target");
}

[[nodiscard]] ShaderToolKind parse_tool_kind(std::string_view value) {
    if (value == "dxc") {
        return ShaderToolKind::dxc;
    }
    if (value == "spirv-val") {
        return ShaderToolKind::spirv_val;
    }
    if (value == "metal") {
        return ShaderToolKind::metal;
    }
    if (value == "metallib") {
        return ShaderToolKind::metallib;
    }
    throw std::invalid_argument("unsupported shader tool kind");
}

[[nodiscard]] std::string command_fingerprint_text(const ShaderCompileRequest& request,
                                                   const ShaderCompileCommand& command,
                                                   const ShaderToolDescriptor& tool) {
    std::ostringstream text;
    text << "target=" << compile_target_name(request.target) << '\n';
    text << "executable=" << command.executable << '\n';
    text << "tool.path=" << tool.executable_path << '\n';
    text << "tool.version=" << tool.version << '\n';
    text << "artifact.path=" << command.artifact.path << '\n';
    text << "artifact.format=" << artifact_format_name(command.artifact.format) << '\n';
    for (const auto& argument : command.arguments) {
        text << "arg=" << argument << '\n';
    }
    return text.str();
}

[[nodiscard]] std::vector<std::string> sorted_unique_input_paths(const ShaderCompileRequest& request,
                                                                 const IFileSystem& filesystem) {
    if (!filesystem.exists(request.source.source_path)) {
        throw std::invalid_argument("shader source path does not exist");
    }

    std::vector<std::string> paths{request.source.source_path};
    const auto source_text = filesystem.read_text(request.source.source_path);
    const auto dependencies = discover_shader_source_dependencies(source_text);
    const auto parent = source_directory(request.source.source_path);
    for (const auto& dependency : dependencies) {
        auto resolved =
            dependency.kind == ShaderIncludeKind::quoted ? join_path(parent, dependency.path) : dependency.path;
        if (is_absolute_or_parent_relative(resolved)) {
            throw std::invalid_argument("shader dependency path is unsafe");
        }
        if (!filesystem.exists(resolved)) {
            throw std::invalid_argument("shader dependency path does not exist");
        }
        paths.push_back(std::move(resolved));
    }
    std::ranges::sort(paths);
    paths.erase(std::ranges::unique(paths).begin(), paths.end());
    return paths;
}

[[nodiscard]] std::vector<ShaderInputFingerprint> make_input_fingerprints(const IFileSystem& filesystem,
                                                                          const std::vector<std::string>& paths) {
    std::vector<ShaderInputFingerprint> inputs;
    inputs.reserve(paths.size());
    for (const auto& path : paths) {
        inputs.push_back(ShaderInputFingerprint{
            .path = path,
            .digest = digest_text(filesystem.read_text(path)),
        });
    }
    return inputs;
}

[[nodiscard]] std::vector<ShaderInputFingerprint> sorted_inputs(std::vector<ShaderInputFingerprint> inputs) {
    std::ranges::sort(inputs, [](const ShaderInputFingerprint& lhs, const ShaderInputFingerprint& rhs) {
        return lhs.path < rhs.path;
    });
    return inputs;
}

[[nodiscard]] std::vector<ShaderArtifactCacheIndexEntry>
sorted_cache_entries(std::vector<ShaderArtifactCacheIndexEntry> entries) {
    std::ranges::sort(entries, [](const ShaderArtifactCacheIndexEntry& lhs, const ShaderArtifactCacheIndexEntry& rhs) {
        return lhs.artifact_path < rhs.artifact_path;
    });
    return entries;
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
            throw std::invalid_argument("shader provenance line is missing '='");
        }
        const auto key = std::string(raw_line.substr(0, separator));
        const auto value = std::string(raw_line.substr(separator + 1U));
        auto [_, inserted] = values.emplace(key, value);
        if (!inserted) {
            throw std::invalid_argument("shader provenance contains duplicate keys");
        }
    }
    return values;
}

[[nodiscard]] const std::string& required_value(const std::unordered_map<std::string, std::string>& values,
                                                const std::string& key) {
    const auto it = values.find(key);
    if (it == values.end()) {
        throw std::invalid_argument("shader provenance is missing key: " + key);
    }
    return it->second;
}

[[nodiscard]] std::size_t parse_count(const std::unordered_map<std::string, std::string>& values,
                                      const std::string& key) {
    const auto& value = required_value(values, key);
    try {
        return static_cast<std::size_t>(std::stoull(value));
    } catch (const std::exception&) {
        throw std::invalid_argument("shader provenance count is invalid: " + key);
    }
}

void require_serializable(std::string_view value, std::string_view field) {
    if (!valid_token(value)) {
        throw std::invalid_argument(std::string("shader provenance field is invalid: ") + std::string(field));
    }
}

} // namespace

std::string_view shader_tool_kind_name(ShaderToolKind kind) noexcept {
    switch (kind) {
    case ShaderToolKind::dxc:
        return "dxc";
    case ShaderToolKind::spirv_val:
        return "spirv-val";
    case ShaderToolKind::metal:
        return "metal";
    case ShaderToolKind::metallib:
        return "metallib";
    case ShaderToolKind::unknown:
        break;
    }
    return "unknown";
}

bool ShaderToolchainReadiness::ready_for_d3d12_dxil() const noexcept {
    return dxc_available;
}

bool ShaderToolchainReadiness::ready_for_vulkan_spirv() const noexcept {
    return dxc_available && dxc_spirv_codegen_available && spirv_validator_available;
}

bool ShaderToolchainReadiness::ready_for_metal_library() const noexcept {
    return metal_available && metallib_available;
}

ShaderArtifactValidationResult
RecordingShaderArtifactValidatorRunner::run(const ShaderArtifactValidationCommand& command) {
    commands_.push_back(command);
    return ShaderArtifactValidationResult{
        .exit_code = 0,
        .diagnostic = "recorded",
        .stdout_text = {},
        .stderr_text = {},
        .artifact = command.artifact,
    };
}

const std::vector<ShaderArtifactValidationCommand>& RecordingShaderArtifactValidatorRunner::commands() const noexcept {
    return commands_;
}

std::vector<ShaderToolDescriptor> discover_shader_tools(const IFileSystem& filesystem,
                                                        const ShaderToolDiscoveryRequest& request) {
    std::vector<ShaderToolDescriptor> discovered;
    for (const auto& root : request.search_roots) {
        if (!valid_token(root) || is_absolute_or_parent_relative(root)) {
            throw std::invalid_argument("shader tool search root is unsafe");
        }

        for (const auto& probe : tool_probes) {
            std::vector<std::string> candidates{join_path(root, probe.executable)};
            if (request.include_windows_exe_suffix) {
                candidates.push_back(join_path(root, std::string(probe.executable) + ".exe"));
            }

            for (const auto& candidate : candidates) {
                if (!filesystem.exists(candidate)) {
                    continue;
                }
                if (std::ranges::find_if(discovered, [&candidate](const ShaderToolDescriptor& descriptor) {
                        return descriptor.executable_path == candidate;
                    }) != discovered.end()) {
                    continue;
                }

                const auto version_path = join_path(root, std::string(probe.executable) + ".version");
                const auto spirv_codegen_marker = join_path(root, std::string(probe.executable) + ".spirv-codegen");
                discovered.push_back(ShaderToolDescriptor{
                    .kind = probe.kind,
                    .executable_path = candidate,
                    .version = filesystem.exists(version_path) ? trim_version(filesystem.read_text(version_path))
                                                               : std::string("unknown"),
                    .supports_spirv_codegen =
                        probe.kind == ShaderToolKind::dxc && filesystem.exists(spirv_codegen_marker),
                });
            }
        }
    }

    std::ranges::sort(discovered, [](const ShaderToolDescriptor& lhs, const ShaderToolDescriptor& rhs) {
        if (tool_order(lhs.kind) != tool_order(rhs.kind)) {
            return tool_order(lhs.kind) < tool_order(rhs.kind);
        }
        return lhs.executable_path < rhs.executable_path;
    });
    return discovered;
}

ShaderToolchainReadiness evaluate_shader_toolchain_readiness(const std::vector<ShaderToolDescriptor>& tools) {
    ShaderToolchainReadiness readiness;
    for (const auto& tool : tools) {
        if (tool.executable_path.empty()) {
            continue;
        }

        switch (tool.kind) {
        case ShaderToolKind::dxc:
            readiness.dxc_available = true;
            readiness.dxc_spirv_codegen_available =
                readiness.dxc_spirv_codegen_available || tool.supports_spirv_codegen;
            break;
        case ShaderToolKind::spirv_val:
            readiness.spirv_validator_available = true;
            break;
        case ShaderToolKind::metal:
            readiness.metal_available = true;
            break;
        case ShaderToolKind::metallib:
            readiness.metallib_available = true;
            break;
        case ShaderToolKind::unknown:
            break;
        }
    }

    if (!readiness.dxc_available) {
        readiness.diagnostics.emplace_back("Missing dxc for D3D12 DXIL and Vulkan SPIR-V shader compilation");
    } else if (!readiness.dxc_spirv_codegen_available) {
        readiness.diagnostics.emplace_back(
            "DXC SPIR-V CodeGen is unavailable; install a DXC build compiled with ENABLE_SPIRV_CODEGEN=ON for Vulkan "
            "SPIR-V shader compilation");
    }
    if (!readiness.spirv_validator_available) {
        readiness.diagnostics.emplace_back("Missing spirv-val for Vulkan SPIR-V validation");
    }
    if (!readiness.metal_available) {
        readiness.diagnostics.emplace_back("Missing metal for Metal IR shader compilation");
    }
    if (!readiness.metallib_available) {
        readiness.diagnostics.emplace_back("Missing metallib for Metal library packaging");
    }
    return readiness;
}

ShaderArtifactValidationCommand make_spirv_shader_validation_command(const ShaderGeneratedArtifact& artifact) {
    if (!is_valid_shader_generated_artifact(artifact) || artifact.format != ShaderArtifactFormat::spirv ||
        is_absolute_or_parent_relative(artifact.path)) {
        throw std::invalid_argument("SPIR-V shader validation artifact is invalid");
    }

    return ShaderArtifactValidationCommand{
        .executable = "spirv-val",
        .arguments = {artifact.path},
        .artifact = artifact,
    };
}

bool is_safe_shader_artifact_validation_command(const ShaderArtifactValidationCommand& command) noexcept {
    if (!allowed_shader_validator_executable(command.executable) || contains_path_separator(command.executable)) {
        return false;
    }
    if (!is_valid_shader_generated_artifact(command.artifact) ||
        command.artifact.format != ShaderArtifactFormat::spirv ||
        is_absolute_or_parent_relative(command.artifact.path)) {
        return false;
    }
    if (command.arguments.size() != 1 || command.arguments[0] != command.artifact.path) {
        return false;
    }
    if (!std::ranges::all_of(command.arguments, [](std::string_view argument) { return valid_argument(argument); })) {
        return false;
    }
    return true;
}

ShaderArtifactValidationResult run_shader_artifact_validation_command(IShaderArtifactValidatorRunner& runner,
                                                                      const ShaderArtifactValidationCommand& command) {
    if (!is_safe_shader_artifact_validation_command(command)) {
        throw std::invalid_argument("shader artifact validation command is unsafe");
    }
    return runner.run(command);
}

ShaderArtifactProvenance build_shader_artifact_provenance(const IFileSystem& filesystem,
                                                          const ShaderCompileRequest& request,
                                                          const ShaderCompileCommand& command,
                                                          const ShaderToolDescriptor& tool) {
    if (!is_valid_shader_compile_request(request) || !is_safe_shader_tool_command(command)) {
        throw std::invalid_argument("shader provenance command is invalid");
    }
    if (!valid_token(tool.executable_path) || !valid_optional_token(tool.version) ||
        tool.kind == ShaderToolKind::unknown) {
        throw std::invalid_argument("shader provenance tool is invalid");
    }
    if (command.artifact.path != request.output_path || command.artifact.profile != request.profile ||
        command.artifact.entry_point != request.source.entry_point) {
        throw std::invalid_argument("shader provenance command does not match request");
    }

    const auto input_paths = sorted_unique_input_paths(request, filesystem);
    return ShaderArtifactProvenance{
        .artifact = command.artifact,
        .target = request.target,
        .source_path = request.source.source_path,
        .profile = request.profile,
        .entry_point = request.source.entry_point,
        .tool = tool,
        .inputs = make_input_fingerprints(filesystem, input_paths),
        .command_fingerprint = digest_text(command_fingerprint_text(request, command, tool)),
    };
}

bool is_shader_artifact_cache_valid(const IFileSystem& filesystem, const ShaderArtifactProvenance& provenance) {
    if (!is_valid_shader_generated_artifact(provenance.artifact) || !filesystem.exists(provenance.artifact.path)) {
        return false;
    }
    if (!std::ranges::all_of(provenance.inputs, [&filesystem](const auto& input) {
            return valid_token(input.path) && valid_token(input.digest) && filesystem.exists(input.path) &&
                   digest_text(filesystem.read_text(input.path)) == input.digest;
        })) {
        return false;
    }
    return true;
}

std::string serialize_shader_artifact_provenance(const ShaderArtifactProvenance& provenance) {
    if (!is_valid_shader_generated_artifact(provenance.artifact) || provenance.target == ShaderCompileTarget::unknown ||
        provenance.tool.kind == ShaderToolKind::unknown) {
        throw std::invalid_argument("shader provenance is invalid");
    }

    require_serializable(provenance.source_path, "source_path");
    require_serializable(provenance.profile, "profile");
    require_serializable(provenance.entry_point, "entry_point");
    require_serializable(provenance.tool.executable_path, "tool.executable");
    require_serializable(provenance.tool.version, "tool.version");
    require_serializable(provenance.command_fingerprint, "command.fingerprint");

    const auto inputs = sorted_inputs(provenance.inputs);
    std::ostringstream output;
    output << "format=" << shader_provenance_format << '\n';
    output << "target=" << compile_target_name(provenance.target) << '\n';
    output << "source=" << provenance.source_path << '\n';
    output << "profile=" << provenance.profile << '\n';
    output << "entry=" << provenance.entry_point << '\n';
    output << "artifact.path=" << provenance.artifact.path << '\n';
    output << "artifact.format=" << artifact_format_name(provenance.artifact.format) << '\n';
    output << "artifact.profile=" << provenance.artifact.profile << '\n';
    output << "artifact.entry=" << provenance.artifact.entry_point << '\n';
    output << "tool.kind=" << shader_tool_kind_name(provenance.tool.kind) << '\n';
    output << "tool.executable=" << provenance.tool.executable_path << '\n';
    output << "tool.version=" << provenance.tool.version << '\n';
    output << "command.fingerprint=" << provenance.command_fingerprint << '\n';
    output << "input.count=" << inputs.size() << '\n';
    for (std::size_t index = 0; index < inputs.size(); ++index) {
        const auto ordinal = index + 1U;
        require_serializable(inputs[index].path, "input.path");
        require_serializable(inputs[index].digest, "input.digest");
        output << "input." << ordinal << ".path=" << inputs[index].path << '\n';
        output << "input." << ordinal << ".digest=" << inputs[index].digest << '\n';
    }
    return output.str();
}

ShaderArtifactProvenance deserialize_shader_artifact_provenance(std::string_view text) {
    const auto values = parse_key_values(text);
    if (required_value(values, "format") != shader_provenance_format) {
        throw std::invalid_argument("shader provenance format is unsupported");
    }

    ShaderArtifactProvenance provenance;
    provenance.target = parse_compile_target(required_value(values, "target"));
    provenance.source_path = required_value(values, "source");
    provenance.profile = required_value(values, "profile");
    provenance.entry_point = required_value(values, "entry");
    provenance.artifact = ShaderGeneratedArtifact{
        .path = required_value(values, "artifact.path"),
        .format = parse_artifact_format(required_value(values, "artifact.format")),
        .profile = required_value(values, "artifact.profile"),
        .entry_point = required_value(values, "artifact.entry"),
    };
    provenance.tool = ShaderToolDescriptor{
        .kind = parse_tool_kind(required_value(values, "tool.kind")),
        .executable_path = required_value(values, "tool.executable"),
        .version = required_value(values, "tool.version"),
    };
    provenance.command_fingerprint = required_value(values, "command.fingerprint");

    const auto input_count = parse_count(values, "input.count");
    provenance.inputs.reserve(input_count);
    for (std::size_t index = 0; index < input_count; ++index) {
        const auto ordinal = index + 1U;
        provenance.inputs.push_back(ShaderInputFingerprint{
            .path = required_value(values, "input." + std::to_string(ordinal) + ".path"),
            .digest = required_value(values, "input." + std::to_string(ordinal) + ".digest"),
        });
    }
    provenance.inputs = sorted_inputs(std::move(provenance.inputs));
    if (!is_valid_shader_generated_artifact(provenance.artifact)) {
        throw std::invalid_argument("shader provenance artifact is invalid");
    }
    return provenance;
}

void upsert_shader_artifact_cache_entry(ShaderArtifactCacheIndex& index, ShaderArtifactCacheIndexEntry entry) {
    if (!valid_token(entry.artifact_path) || !valid_token(entry.provenance_path) ||
        !valid_token(entry.command_fingerprint)) {
        throw std::invalid_argument("shader cache entry is invalid");
    }

    const auto existing = std::ranges::find_if(index.entries, [&entry](const ShaderArtifactCacheIndexEntry& candidate) {
        return candidate.artifact_path == entry.artifact_path;
    });
    if (existing != index.entries.end()) {
        *existing = std::move(entry);
    } else {
        index.entries.push_back(std::move(entry));
    }
    index.entries = sorted_cache_entries(std::move(index.entries));
}

std::string serialize_shader_artifact_cache_index(const ShaderArtifactCacheIndex& index) {
    const auto entries = sorted_cache_entries(index.entries);
    std::ostringstream output;
    output << "format=" << shader_cache_index_format << '\n';
    output << "entry.count=" << entries.size() << '\n';
    for (std::size_t entry_index = 0; entry_index < entries.size(); ++entry_index) {
        const auto ordinal = entry_index + 1U;
        const auto& entry = entries[entry_index];
        if (!valid_token(entry.artifact_path) || !valid_token(entry.provenance_path) ||
            !valid_token(entry.command_fingerprint)) {
            throw std::invalid_argument("shader cache index contains invalid entry");
        }
        output << "entry." << ordinal << ".artifact=" << entry.artifact_path << '\n';
        output << "entry." << ordinal << ".provenance=" << entry.provenance_path << '\n';
        output << "entry." << ordinal << ".fingerprint=" << entry.command_fingerprint << '\n';
    }
    return output.str();
}

ShaderArtifactCacheIndex deserialize_shader_artifact_cache_index(std::string_view text) {
    const auto values = parse_key_values(text);
    if (required_value(values, "format") != shader_cache_index_format) {
        throw std::invalid_argument("shader cache index format is unsupported");
    }

    ShaderArtifactCacheIndex index;
    const auto entry_count = parse_count(values, "entry.count");
    index.entries.reserve(entry_count);
    for (std::size_t entry_index = 0; entry_index < entry_count; ++entry_index) {
        const auto ordinal = entry_index + 1U;
        ShaderArtifactCacheIndexEntry entry{
            .artifact_path = required_value(values, "entry." + std::to_string(ordinal) + ".artifact"),
            .provenance_path = required_value(values, "entry." + std::to_string(ordinal) + ".provenance"),
            .command_fingerprint = required_value(values, "entry." + std::to_string(ordinal) + ".fingerprint"),
        };
        if (!valid_token(entry.artifact_path) || !valid_token(entry.provenance_path) ||
            !valid_token(entry.command_fingerprint)) {
            throw std::invalid_argument("shader cache index contains invalid entry");
        }
        index.entries.push_back(std::move(entry));
    }
    index.entries = sorted_cache_entries(std::move(index.entries));
    return index;
}

void save_shader_artifact_cache_index(IFileSystem& filesystem, std::string_view path,
                                      const ShaderArtifactCacheIndex& index) {
    if (!valid_token(path) || is_absolute_or_parent_relative(path)) {
        throw std::invalid_argument("shader cache index path is unsafe");
    }
    filesystem.write_text(path, serialize_shader_artifact_cache_index(index));
}

ShaderArtifactCacheIndex load_shader_artifact_cache_index(const IFileSystem& filesystem, std::string_view path) {
    if (!valid_token(path) || is_absolute_or_parent_relative(path)) {
        throw std::invalid_argument("shader cache index path is unsafe");
    }
    return deserialize_shader_artifact_cache_index(filesystem.read_text(path));
}

} // namespace mirakana
