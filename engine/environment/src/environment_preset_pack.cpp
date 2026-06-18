// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/environment/environment_preset_pack.hpp"

#include "mirakana/environment/environment_io.hpp"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace mirakana {
namespace {

constexpr std::string_view kEnvironmentPresetPackFormatV1{"GameEngine.EnvironmentPresetPack.v1"};

using KeyValues = std::unordered_map<std::string, std::string>;

[[nodiscard]] bool starts_with(std::string_view value, std::string_view prefix) noexcept {
    return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
}

[[nodiscard]] bool valid_text_token(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos &&
           value.find('\0') == std::string_view::npos && value.find(';') == std::string_view::npos;
}

[[nodiscard]] char lower_ascii(char ch) noexcept {
    if (ch >= 'A' && ch <= 'Z') {
        return static_cast<char>(ch - 'A' + 'a');
    }
    return ch;
}

[[nodiscard]] bool is_token_char(char ch) noexcept {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9');
}

[[nodiscard]] bool forbidden_identifier_token(std::string_view token) noexcept {
    return token == "native" || token == "backend" || token == "handle" || token == "hwnd" || token == "d3d12" ||
           token == "vulkan" || token == "vk" || token == "metal" || token == "mtl" || token == "imgui" ||
           token == "dearimgui" || token == "sdl" || token == "sdl3" || token == "editor";
}

[[nodiscard]] bool has_forbidden_identifier_token(std::string_view id) {
    std::string token;
    for (const auto ch : id) {
        if (is_token_char(ch)) {
            token.push_back(lower_ascii(ch));
            continue;
        }
        if (forbidden_identifier_token(token)) {
            return true;
        }
        token.clear();
    }
    return forbidden_identifier_token(token);
}

[[nodiscard]] bool quality_tier_valid(EnvironmentQualityPreset value) noexcept {
    switch (value) {
    case EnvironmentQualityPreset::low:
    case EnvironmentQualityPreset::medium:
    case EnvironmentQualityPreset::high:
    case EnvironmentQualityPreset::ultra:
    case EnvironmentQualityPreset::custom:
        return true;
    }
    return false;
}

void add_diagnostic(EnvironmentPresetPackValidationResult& result, EnvironmentPresetPackDiagnosticCode code,
                    std::uint32_t preset_index, std::string field, std::string id, std::string message) {
    result.diagnostics.push_back(EnvironmentPresetPackDiagnostic{
        .code = code,
        .preset_index = preset_index,
        .field = std::move(field),
        .id = std::move(id),
        .message = std::move(message),
    });
}

void validate_required_token(EnvironmentPresetPackValidationResult& result, std::string_view value,
                             EnvironmentPresetPackDiagnosticCode missing_code, std::uint32_t preset_index,
                             std::string field, std::string id, std::string message) {
    if (!valid_text_token(value)) {
        add_diagnostic(result, missing_code, preset_index, std::move(field), std::move(id), std::move(message));
    }
}

[[nodiscard]] std::uint64_t checked_add(std::uint64_t lhs, std::uint64_t rhs) noexcept {
    if (lhs > std::numeric_limits<std::uint64_t>::max() - rhs) {
        return std::numeric_limits<std::uint64_t>::max();
    }
    return lhs + rhs;
}

void validate_budget_positive(EnvironmentPresetPackValidationResult& result, std::uint64_t value,
                              std::uint32_t preset_index, std::string field, std::string id) {
    if (value == 0) {
        add_diagnostic(result, EnvironmentPresetPackDiagnosticCode::invalid_budget, preset_index, std::move(field),
                       std::move(id), "environment preset pack budget values must be positive");
    }
}

void validate_preset(EnvironmentPresetPackValidationResult& result, const EnvironmentPresetPackPresetV1& preset,
                     std::uint32_t index) {
    if (!valid_text_token(preset.id)) {
        add_diagnostic(result, EnvironmentPresetPackDiagnosticCode::empty_preset_id, index, "preset.id", preset.id,
                       "environment preset id must not be empty");
    } else if (has_forbidden_identifier_token(preset.id)) {
        add_diagnostic(result, EnvironmentPresetPackDiagnosticCode::forbidden_token, index, "preset.id", preset.id,
                       "environment preset id must not contain native, backend, editor, or middleware tokens");
    }

    validate_required_token(
        result, preset.profile_asset_path, EnvironmentPresetPackDiagnosticCode::missing_profile_reference, index,
        "preset.profile_asset_path", preset.id, "environment preset must reference an authored environment profile");
    validate_required_token(result, preset.art_direction, EnvironmentPresetPackDiagnosticCode::missing_art_direction,
                            index, "preset.art_direction", preset.id, "environment preset must include art direction");
    validate_required_token(
        result, preset.sky_atmosphere_values, EnvironmentPresetPackDiagnosticCode::missing_sky_atmosphere_values, index,
        "preset.sky_atmosphere_values", preset.id, "environment preset must include sky and atmosphere values");
    validate_required_token(result, preset.fog_cloud_weather_timeline,
                            EnvironmentPresetPackDiagnosticCode::missing_fog_cloud_weather_timeline, index,
                            "preset.fog_cloud_weather_timeline", preset.id,
                            "environment preset must include fog, cloud, and weather timeline intent");
    validate_required_token(result, preset.ibl_asset_ref, EnvironmentPresetPackDiagnosticCode::missing_ibl_reference,
                            index, "preset.ibl_asset_ref", preset.id,
                            "environment preset must include an IBL reference");
    validate_required_token(result, preset.material_weathering_ref,
                            EnvironmentPresetPackDiagnosticCode::missing_material_weathering_reference, index,
                            "preset.material_weathering_ref", preset.id,
                            "environment preset must include material weathering intent");
    validate_required_token(
        result, preset.audio_trigger_intent, EnvironmentPresetPackDiagnosticCode::missing_audio_trigger_intent, index,
        "preset.audio_trigger_intent", preset.id, "environment preset must include audio trigger intent");
    validate_required_token(
        result, preset.quality_budget_id, EnvironmentPresetPackDiagnosticCode::missing_quality_budget, index,
        "preset.quality_budget_id", preset.id, "environment preset must reference a quality budget");
    validate_required_token(
        result, preset.validation_recipe_id, EnvironmentPresetPackDiagnosticCode::missing_validation_recipe, index,
        "preset.validation_recipe_id", preset.id, "environment preset must reference a validation recipe");

    if (!quality_tier_valid(preset.quality_tier)) {
        add_diagnostic(result, EnvironmentPresetPackDiagnosticCode::invalid_quality_tier, index, "preset.quality_tier",
                       preset.id, "environment preset quality tier is unsupported");
    }

    validate_budget_positive(result, preset.package_size_bytes, index, "preset.package_size_bytes", preset.id);
    validate_budget_positive(result, preset.installed_size_bytes, index, "preset.installed_size_bytes", preset.id);
    validate_budget_positive(result, preset.estimated_decoded_memory_bytes, index,
                             "preset.estimated_decoded_memory_bytes", preset.id);
    validate_budget_positive(result, preset.estimated_gpu_memory_bytes, index, "preset.estimated_gpu_memory_bytes",
                             preset.id);
}

void validate_required_commercial_presets(EnvironmentPresetPackValidationResult& result,
                                          const std::unordered_set<std::string>& preset_ids) {
    constexpr std::string_view required_ids[] = {
        "clear_noon",
        "overcast_storm",
        "night_moonlit",
        "snowfield",
        "foggy_valley",
        "cinematic_sunset",
        "indoor_to_outdoor_transition",
    };

    for (const auto required_id : required_ids) {
        if (preset_ids.find(std::string{required_id}) == preset_ids.end()) {
            add_diagnostic(result, EnvironmentPresetPackDiagnosticCode::missing_required_preset, 0U, "presets",
                           std::string{required_id}, "environment preset pack is missing a required commercial preset");
        }
    }
}

[[nodiscard]] KeyValues parse_key_values(std::string_view text) {
    KeyValues values;
    std::size_t begin = 0;
    while (begin < text.size()) {
        const auto end = text.find('\n', begin);
        const auto line = text.substr(begin, end == std::string_view::npos ? text.size() - begin : end - begin);
        begin = end == std::string_view::npos ? text.size() : end + 1U;
        if (line.empty()) {
            continue;
        }
        if (line.find('\r') != std::string_view::npos) {
            throw std::invalid_argument("environment preset pack line contains carriage return");
        }
        const auto separator = line.find('=');
        if (separator == std::string_view::npos || separator == 0U) {
            throw std::invalid_argument("environment preset pack line is missing '='");
        }
        auto [_, inserted] =
            values.emplace(std::string{line.substr(0, separator)}, std::string{line.substr(separator + 1U)});
        if (!inserted) {
            throw std::invalid_argument("environment preset pack contains duplicate keys");
        }
    }
    return values;
}

[[nodiscard]] const std::string& required_value(const KeyValues& values, const std::string& key) {
    const auto it = values.find(key);
    if (it == values.end()) {
        throw std::invalid_argument("environment preset pack is missing key: " + key);
    }
    return it->second;
}

[[nodiscard]] std::uint64_t parse_u64(std::string_view value, std::string_view key) {
    std::uint64_t parsed{0};
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (error != std::errc{} || end != value.data() + value.size()) {
        throw std::invalid_argument("environment preset pack integer value is invalid: " + std::string{key});
    }
    return parsed;
}

[[nodiscard]] std::size_t parse_ordinal(std::string_view value, std::string_view key) {
    const auto parsed = parse_u64(value, key);
    if (parsed >= static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        throw std::invalid_argument("environment preset pack ordinal value is invalid: " + std::string{key});
    }
    return static_cast<std::size_t>(parsed);
}

void parse_backend_feature_row(EnvironmentPresetPackDocumentV1& document, std::string_view key, std::string_view suffix,
                               std::string value) {
    const auto ordinal = parse_ordinal(suffix, key);
    if (ordinal >= document.required_backend_feature_rows.size()) {
        throw std::invalid_argument("environment preset pack backend feature row ordinal is out of range");
    }
    document.required_backend_feature_rows[ordinal] = std::move(value);
}

void parse_preset_row(std::unordered_map<std::size_t, EnvironmentPresetPackPresetV1>& presets, std::string_view key,
                      std::string_view suffix, std::string_view value) {
    const auto separator = suffix.find('.');
    if (separator == std::string_view::npos) {
        throw std::invalid_argument("environment preset pack preset key is malformed");
    }
    const auto ordinal = parse_ordinal(suffix.substr(0, separator), key);
    const auto field = suffix.substr(separator + 1U);
    auto& preset = presets[ordinal];

    if (field == "id") {
        preset.id = std::string{value};
    } else if (field == "profile_asset_path") {
        preset.profile_asset_path = std::string{value};
    } else if (field == "art_direction") {
        preset.art_direction = std::string{value};
    } else if (field == "sky_atmosphere_values") {
        preset.sky_atmosphere_values = std::string{value};
    } else if (field == "fog_cloud_weather_timeline") {
        preset.fog_cloud_weather_timeline = std::string{value};
    } else if (field == "ibl_asset_ref") {
        preset.ibl_asset_ref = std::string{value};
    } else if (field == "material_weathering_ref") {
        preset.material_weathering_ref = std::string{value};
    } else if (field == "audio_trigger_intent") {
        preset.audio_trigger_intent = std::string{value};
    } else if (field == "quality_budget_id") {
        preset.quality_budget_id = std::string{value};
    } else if (field == "quality_tier") {
        preset.quality_tier = parse_environment_quality_preset(value);
    } else if (field == "package_size_bytes") {
        preset.package_size_bytes = parse_u64(value, key);
    } else if (field == "installed_size_bytes") {
        preset.installed_size_bytes = parse_u64(value, key);
    } else if (field == "estimated_decoded_memory_bytes") {
        preset.estimated_decoded_memory_bytes = parse_u64(value, key);
    } else if (field == "estimated_gpu_memory_bytes") {
        preset.estimated_gpu_memory_bytes = parse_u64(value, key);
    } else if (field == "validation_recipe_id") {
        preset.validation_recipe_id = std::string{value};
    } else {
        throw std::invalid_argument("environment preset pack preset field is unsupported");
    }
}

} // namespace

bool EnvironmentPresetPackValidationResult::succeeded() const noexcept {
    return status == EnvironmentPresetPackValidationStatus::valid && diagnostics.empty();
}

std::string_view environment_preset_pack_format_v1() noexcept {
    return kEnvironmentPresetPackFormatV1;
}

EnvironmentPresetPackValidationResult
validate_environment_preset_pack_v1(const EnvironmentPresetPackDocumentV1& document) {
    EnvironmentPresetPackValidationResult result{};
    if (!valid_text_token(document.id)) {
        add_diagnostic(result, EnvironmentPresetPackDiagnosticCode::empty_pack_id, 0U, "pack.id", document.id,
                       "environment preset pack id must not be empty");
    } else if (has_forbidden_identifier_token(document.id)) {
        add_diagnostic(result, EnvironmentPresetPackDiagnosticCode::forbidden_token, 0U, "pack.id", document.id,
                       "environment preset pack id must not contain native, backend, editor, or middleware tokens");
    }

    validate_required_token(result, document.provenance_id, EnvironmentPresetPackDiagnosticCode::missing_provenance_id,
                            0U, "pack.provenance_id", document.id,
                            "environment preset pack must include a provenance id");
    validate_required_token(result, document.license_id, EnvironmentPresetPackDiagnosticCode::missing_license_id, 0U,
                            "pack.license_id", document.id, "environment preset pack must include a license id");
    validate_required_token(result, document.art_direction, EnvironmentPresetPackDiagnosticCode::missing_art_direction,
                            0U, "pack.art_direction", document.id,
                            "environment preset pack must include art direction");

    if (!quality_tier_valid(document.quality_tier)) {
        add_diagnostic(result, EnvironmentPresetPackDiagnosticCode::invalid_quality_tier, 0U, "pack.quality_tier",
                       document.id, "environment preset pack quality tier is unsupported");
    }

    validate_budget_positive(result, document.package_size_budget_bytes, 0U, "pack.package_size_budget_bytes",
                             document.id);
    validate_budget_positive(result, document.installed_size_budget_bytes, 0U, "pack.installed_size_budget_bytes",
                             document.id);
    validate_budget_positive(result, document.decoded_memory_budget_bytes, 0U, "pack.decoded_memory_budget_bytes",
                             document.id);
    validate_budget_positive(result, document.gpu_memory_budget_bytes, 0U, "pack.gpu_memory_budget_bytes", document.id);

    if (document.required_backend_feature_rows.empty()) {
        add_diagnostic(result, EnvironmentPresetPackDiagnosticCode::missing_backend_feature_row, 0U,
                       "pack.required_backend_feature_rows", document.id,
                       "environment preset pack must reference at least one backend feature row");
    }
    std::unordered_set<std::string> backend_feature_rows;
    for (std::size_t index = 0U; index < document.required_backend_feature_rows.size(); ++index) {
        const auto& row = document.required_backend_feature_rows[index];
        if (!valid_text_token(row)) {
            add_diagnostic(result, EnvironmentPresetPackDiagnosticCode::missing_backend_feature_row,
                           static_cast<std::uint32_t>(index), "pack.required_backend_feature_row", row,
                           "environment preset pack backend feature rows must not be empty");
        } else if (!backend_feature_rows.insert(row).second) {
            add_diagnostic(result, EnvironmentPresetPackDiagnosticCode::duplicate_backend_feature_row,
                           static_cast<std::uint32_t>(index), "pack.required_backend_feature_row", row,
                           "environment preset pack backend feature rows must be unique");
        }
    }

    std::unordered_set<std::string> preset_ids;
    std::uint64_t package_sum{0};
    std::uint64_t installed_sum{0};
    std::uint64_t decoded_sum{0};
    std::uint64_t gpu_sum{0};
    for (std::size_t index = 0U; index < document.presets.size(); ++index) {
        const auto& preset = document.presets[index];
        validate_preset(result, preset, static_cast<std::uint32_t>(index));
        if (!preset.id.empty() && !preset_ids.insert(preset.id).second) {
            add_diagnostic(result, EnvironmentPresetPackDiagnosticCode::duplicate_preset_id,
                           static_cast<std::uint32_t>(index), "preset.id", preset.id,
                           "environment preset ids must be unique");
        }
        package_sum = checked_add(package_sum, preset.package_size_bytes);
        installed_sum = checked_add(installed_sum, preset.installed_size_bytes);
        decoded_sum = checked_add(decoded_sum, preset.estimated_decoded_memory_bytes);
        gpu_sum = checked_add(gpu_sum, preset.estimated_gpu_memory_bytes);
    }
    validate_required_commercial_presets(result, preset_ids);

    if (document.package_size_budget_bytes < package_sum || document.installed_size_budget_bytes < installed_sum ||
        document.decoded_memory_budget_bytes < decoded_sum || document.gpu_memory_budget_bytes < gpu_sum) {
        add_diagnostic(result, EnvironmentPresetPackDiagnosticCode::invalid_budget, 0U, "pack.budgets", document.id,
                       "environment preset pack aggregate budgets must cover all preset estimates");
    }

    result.status = result.diagnostics.empty() ? EnvironmentPresetPackValidationStatus::valid
                                               : EnvironmentPresetPackValidationStatus::invalid;
    return result;
}

bool is_valid_environment_preset_pack_v1(const EnvironmentPresetPackDocumentV1& document) noexcept {
    try {
        return validate_environment_preset_pack_v1(document).succeeded();
    } catch (...) {
        return false;
    }
}

bool has_environment_preset_pack_diagnostic(const EnvironmentPresetPackValidationResult& result,
                                            EnvironmentPresetPackDiagnosticCode code) noexcept {
    return std::ranges::any_of(result.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

EnvironmentPresetAssetLibraryProductionResult
evaluate_environment_preset_asset_library_production(const EnvironmentPresetAssetLibraryProductionDesc& desc) {
    auto result = EnvironmentPresetAssetLibraryProductionResult{};
    const auto count_row = [](std::uint32_t value, std::uint32_t required) noexcept {
        return value >= required ? 0U : required - value;
    };
    const auto valid_hash = [](std::string_view value) noexcept {
        if (value.size() != 64U) {
            return false;
        }
        for (const auto ch : value) {
            const bool decimal = ch >= '0' && ch <= '9';
            const bool lower_hex = ch >= 'a' && ch <= 'f';
            const bool upper_hex = ch >= 'A' && ch <= 'F';
            if (!decimal && !lower_hex && !upper_hex) {
                return false;
            }
        }
        return true;
    };

    result.asset_rows = static_cast<std::uint32_t>(desc.asset_rows.size());
    result.backend_execution = desc.request_backend_execution;
    result.package_script_execution = desc.request_package_script_execution;
    result.native_handle_access = desc.request_native_handle_access;

    for (const auto& row : desc.asset_rows) {
        if (!valid_text_token(row.id)) {
            result.diagnostics = true;
        }
        switch (row.category) {
        case EnvironmentPresetAssetCategory::sky_atmosphere:
            ++result.sky_atmosphere_presets;
            break;
        case EnvironmentPresetAssetCategory::volumetric_cloud:
            ++result.volumetric_cloud_presets;
            break;
        case EnvironmentPresetAssetCategory::fog_volume:
            ++result.fog_volume_presets;
            break;
        case EnvironmentPresetAssetCategory::rain:
            ++result.rain_presets;
            break;
        case EnvironmentPresetAssetCategory::snow:
            ++result.snow_presets;
            break;
        case EnvironmentPresetAssetCategory::wind:
            ++result.wind_presets;
            break;
        case EnvironmentPresetAssetCategory::material_weathering:
            ++result.material_weathering_presets;
            break;
        case EnvironmentPresetAssetCategory::lighting_ibl:
            ++result.lighting_ibl_presets;
            break;
        case EnvironmentPresetAssetCategory::weather_timeline:
            ++result.weather_timeline_presets;
            break;
        case EnvironmentPresetAssetCategory::biome_environment:
            ++result.biome_environment_presets;
            break;
        }

        if (row.license_provenance_ready && valid_text_token(row.provenance_id) && valid_text_token(row.license_id)) {
            ++result.license_provenance_rows;
        } else {
            ++result.license_missing_rows;
        }

        if (row.package_budget_ready && valid_text_token(row.package_budget_id)) {
            ++result.package_budget_rows;
        } else {
            ++result.package_budget_overages;
        }
    }

    for (const auto& row : desc.preview_screenshot_rows) {
        if (row.reviewed && valid_text_token(row.id) && valid_text_token(row.asset_id) &&
            valid_text_token(row.artifact_path) && valid_hash(row.artifact_hash_sha256)) {
            ++result.preview_screenshot_rows;
        } else {
            result.diagnostics = true;
        }
    }

    for (const auto& row : desc.sample_scene_rows) {
        if (row.reviewed && row.package_visible && valid_text_token(row.id) && valid_text_token(row.asset_id) &&
            valid_text_token(row.scene_path)) {
            ++result.sample_scene_consumption_rows;
        } else {
            result.diagnostics = true;
        }
    }

    result.missing_objective_rows += count_row(result.sky_atmosphere_presets, 24U);
    result.missing_objective_rows += count_row(result.volumetric_cloud_presets, 24U);
    result.missing_objective_rows += count_row(result.fog_volume_presets, 16U);
    result.missing_objective_rows += count_row(result.rain_presets, 12U);
    result.missing_objective_rows += count_row(result.snow_presets, 12U);
    result.missing_objective_rows += count_row(result.wind_presets, 12U);
    result.missing_objective_rows += count_row(result.material_weathering_presets, 24U);
    result.missing_objective_rows += count_row(result.lighting_ibl_presets, 12U);
    result.missing_objective_rows += count_row(result.weather_timeline_presets, 12U);
    result.missing_objective_rows += count_row(result.biome_environment_presets, 8U);
    result.missing_objective_rows += count_row(result.sample_scene_consumption_rows, 8U);
    result.missing_objective_rows += count_row(result.preview_screenshot_rows, 144U);

    result.diagnostics = result.diagnostics || result.missing_objective_rows != 0U ||
                         result.license_missing_rows != 0U || result.package_budget_overages != 0U ||
                         result.backend_execution || result.package_script_execution || result.native_handle_access;
    result.environment_aaa_preset_asset_library_ready =
        result.missing_objective_rows == 0U && result.license_provenance_rows == result.asset_rows &&
        result.package_budget_rows == result.asset_rows && result.license_missing_rows == 0U &&
        result.package_budget_overages == 0U && !result.backend_execution && !result.package_script_execution &&
        !result.native_handle_access && !result.diagnostics;

    return result;
}

std::string serialize_environment_preset_pack_v1(const EnvironmentPresetPackDocumentV1& document) {
    const auto validation = validate_environment_preset_pack_v1(document);
    if (!validation.succeeded()) {
        throw std::invalid_argument("environment preset pack document is invalid");
    }

    std::ostringstream output;
    output << "format=" << kEnvironmentPresetPackFormatV1 << '\n';
    output << "pack.id=" << document.id << '\n';
    output << "pack.provenance_id=" << document.provenance_id << '\n';
    output << "pack.license_id=" << document.license_id << '\n';
    output << "pack.art_direction=" << document.art_direction << '\n';
    output << "pack.quality_tier=" << environment_quality_preset_name(document.quality_tier) << '\n';
    output << "pack.package_size_budget_bytes=" << document.package_size_budget_bytes << '\n';
    output << "pack.installed_size_budget_bytes=" << document.installed_size_budget_bytes << '\n';
    output << "pack.decoded_memory_budget_bytes=" << document.decoded_memory_budget_bytes << '\n';
    output << "pack.gpu_memory_budget_bytes=" << document.gpu_memory_budget_bytes << '\n';
    output << "pack.required_backend_feature_row.count=" << document.required_backend_feature_rows.size() << '\n';
    for (std::size_t index = 0U; index < document.required_backend_feature_rows.size(); ++index) {
        output << "pack.required_backend_feature_row." << index << '=' << document.required_backend_feature_rows[index]
               << '\n';
    }
    output << "preset.count=" << document.presets.size() << '\n';
    for (std::size_t index = 0U; index < document.presets.size(); ++index) {
        const auto& preset = document.presets[index];
        const auto prefix = std::string{"preset."} + std::to_string(index) + ".";
        output << prefix << "id=" << preset.id << '\n';
        output << prefix << "profile_asset_path=" << preset.profile_asset_path << '\n';
        output << prefix << "art_direction=" << preset.art_direction << '\n';
        output << prefix << "sky_atmosphere_values=" << preset.sky_atmosphere_values << '\n';
        output << prefix << "fog_cloud_weather_timeline=" << preset.fog_cloud_weather_timeline << '\n';
        output << prefix << "ibl_asset_ref=" << preset.ibl_asset_ref << '\n';
        output << prefix << "material_weathering_ref=" << preset.material_weathering_ref << '\n';
        output << prefix << "audio_trigger_intent=" << preset.audio_trigger_intent << '\n';
        output << prefix << "quality_budget_id=" << preset.quality_budget_id << '\n';
        output << prefix << "quality_tier=" << environment_quality_preset_name(preset.quality_tier) << '\n';
        output << prefix << "package_size_bytes=" << preset.package_size_bytes << '\n';
        output << prefix << "installed_size_bytes=" << preset.installed_size_bytes << '\n';
        output << prefix << "estimated_decoded_memory_bytes=" << preset.estimated_decoded_memory_bytes << '\n';
        output << prefix << "estimated_gpu_memory_bytes=" << preset.estimated_gpu_memory_bytes << '\n';
        output << prefix << "validation_recipe_id=" << preset.validation_recipe_id << '\n';
    }
    return output.str();
}

EnvironmentPresetPackDocumentV1 deserialize_environment_preset_pack_v1(std::string_view text) {
    const auto values = parse_key_values(text);
    if (required_value(values, "format") != kEnvironmentPresetPackFormatV1) {
        throw std::invalid_argument("environment preset pack format is unsupported");
    }

    EnvironmentPresetPackDocumentV1 document{};
    document.id = required_value(values, "pack.id");
    document.provenance_id = required_value(values, "pack.provenance_id");
    document.license_id = required_value(values, "pack.license_id");
    document.art_direction = required_value(values, "pack.art_direction");
    document.quality_tier = parse_environment_quality_preset(required_value(values, "pack.quality_tier"));
    document.package_size_budget_bytes =
        parse_u64(required_value(values, "pack.package_size_budget_bytes"), "pack.package_size_budget_bytes");
    document.installed_size_budget_bytes =
        parse_u64(required_value(values, "pack.installed_size_budget_bytes"), "pack.installed_size_budget_bytes");
    document.decoded_memory_budget_bytes =
        parse_u64(required_value(values, "pack.decoded_memory_budget_bytes"), "pack.decoded_memory_budget_bytes");
    document.gpu_memory_budget_bytes =
        parse_u64(required_value(values, "pack.gpu_memory_budget_bytes"), "pack.gpu_memory_budget_bytes");

    const auto backend_feature_count = parse_u64(required_value(values, "pack.required_backend_feature_row.count"),
                                                 "pack.required_backend_feature_row.count");
    document.required_backend_feature_rows.resize(static_cast<std::size_t>(backend_feature_count));

    std::unordered_map<std::size_t, EnvironmentPresetPackPresetV1> preset_rows;
    const auto preset_count = parse_u64(required_value(values, "preset.count"), "preset.count");
    preset_rows.reserve(static_cast<std::size_t>(preset_count));

    for (const auto& [key, value] : values) {
        const std::string_view key_view{key};
        if (key_view == "format" || starts_with(key_view, "pack.") || key_view == "preset.count") {
            if (starts_with(key_view, "pack.required_backend_feature_row.") &&
                key_view != "pack.required_backend_feature_row.count") {
                parse_backend_feature_row(
                    document, key_view, key_view.substr(std::string_view{"pack.required_backend_feature_row."}.size()),
                    value);
            }
            continue;
        }
        if (starts_with(key_view, "preset.")) {
            parse_preset_row(preset_rows, key_view, key_view.substr(std::string_view{"preset."}.size()), value);
            continue;
        }
        throw std::invalid_argument("environment preset pack key is unsupported");
    }

    document.presets.reserve(static_cast<std::size_t>(preset_count));
    for (std::size_t index = 0U; index < static_cast<std::size_t>(preset_count); ++index) {
        const auto it = preset_rows.find(index);
        if (it == preset_rows.end()) {
            throw std::invalid_argument("environment preset pack preset ordinals are not contiguous");
        }
        document.presets.push_back(std::move(it->second));
    }

    const auto validation = validate_environment_preset_pack_v1(document);
    if (!validation.succeeded()) {
        throw std::invalid_argument("environment preset pack document is invalid");
    }
    return document;
}

} // namespace mirakana
