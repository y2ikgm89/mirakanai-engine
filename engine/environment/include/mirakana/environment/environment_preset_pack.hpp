// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/environment/environment_profile.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

struct EnvironmentPresetPackPresetV1 {
    std::string id;
    std::string profile_asset_path;
    std::string art_direction;
    std::string sky_atmosphere_values;
    std::string fog_cloud_weather_timeline;
    std::string ibl_asset_ref;
    std::string material_weathering_ref;
    std::string audio_trigger_intent;
    std::string quality_budget_id;
    EnvironmentQualityPreset quality_tier{EnvironmentQualityPreset::medium};
    std::uint64_t package_size_bytes{0};
    std::uint64_t installed_size_bytes{0};
    std::uint64_t estimated_decoded_memory_bytes{0};
    std::uint64_t estimated_gpu_memory_bytes{0};
    std::string validation_recipe_id;
};

struct EnvironmentPresetPackDocumentV1 {
    std::string id;
    std::string provenance_id;
    std::string license_id;
    std::string art_direction;
    EnvironmentQualityPreset quality_tier{EnvironmentQualityPreset::medium};
    std::uint64_t package_size_budget_bytes{0};
    std::uint64_t installed_size_budget_bytes{0};
    std::uint64_t decoded_memory_budget_bytes{0};
    std::uint64_t gpu_memory_budget_bytes{0};
    std::vector<std::string> required_backend_feature_rows;
    std::vector<EnvironmentPresetPackPresetV1> presets;
};

enum class EnvironmentPresetAssetCategory : std::uint8_t {
    sky_atmosphere = 0,
    volumetric_cloud,
    fog_volume,
    rain,
    snow,
    wind,
    material_weathering,
    lighting_ibl,
    weather_timeline,
    biome_environment,
};

struct EnvironmentPresetAssetLibraryAssetRow {
    std::string id;
    EnvironmentPresetAssetCategory category{EnvironmentPresetAssetCategory::sky_atmosphere};
    std::string provenance_id;
    std::string license_id;
    std::string package_budget_id;
    bool license_provenance_ready{false};
    bool package_budget_ready{false};
};

struct EnvironmentPresetAssetLibraryPreviewScreenshotRow {
    std::string id;
    std::string asset_id;
    std::string artifact_path;
    std::string artifact_hash_sha256;
    bool reviewed{false};
};

struct EnvironmentPresetAssetLibrarySampleSceneRow {
    std::string id;
    std::string asset_id;
    std::string scene_path;
    bool package_visible{false};
    bool reviewed{false};
};

struct EnvironmentPresetAssetLibraryProductionDesc {
    std::vector<EnvironmentPresetAssetLibraryAssetRow> asset_rows;
    std::vector<EnvironmentPresetAssetLibraryPreviewScreenshotRow> preview_screenshot_rows;
    std::vector<EnvironmentPresetAssetLibrarySampleSceneRow> sample_scene_rows;
    bool request_backend_execution{false};
    bool request_package_script_execution{false};
    bool request_native_handle_access{false};
};

struct EnvironmentPresetAssetLibraryProductionResult {
    bool environment_aaa_preset_asset_library_ready{false};
    std::uint32_t asset_rows{0};
    std::uint32_t sky_atmosphere_presets{0};
    std::uint32_t volumetric_cloud_presets{0};
    std::uint32_t fog_volume_presets{0};
    std::uint32_t rain_presets{0};
    std::uint32_t snow_presets{0};
    std::uint32_t wind_presets{0};
    std::uint32_t material_weathering_presets{0};
    std::uint32_t lighting_ibl_presets{0};
    std::uint32_t weather_timeline_presets{0};
    std::uint32_t biome_environment_presets{0};
    std::uint32_t sample_scene_consumption_rows{0};
    std::uint32_t preview_screenshot_rows{0};
    std::uint32_t license_provenance_rows{0};
    std::uint32_t package_budget_rows{0};
    std::uint32_t license_missing_rows{0};
    std::uint32_t package_budget_overages{0};
    std::uint32_t missing_objective_rows{0};
    bool backend_execution{false};
    bool package_script_execution{false};
    bool native_handle_access{false};
    bool diagnostics{false};
};

enum class EnvironmentPresetPackValidationStatus : std::uint8_t {
    valid = 0,
    invalid,
};

enum class EnvironmentPresetPackDiagnosticCode : std::uint8_t {
    none = 0,
    empty_pack_id,
    forbidden_token,
    missing_provenance_id,
    missing_license_id,
    missing_art_direction,
    invalid_quality_tier,
    invalid_budget,
    missing_backend_feature_row,
    duplicate_backend_feature_row,
    empty_preset_id,
    duplicate_preset_id,
    missing_required_preset,
    missing_profile_reference,
    missing_sky_atmosphere_values,
    missing_fog_cloud_weather_timeline,
    missing_ibl_reference,
    missing_material_weathering_reference,
    missing_audio_trigger_intent,
    missing_quality_budget,
    missing_validation_recipe,
};

struct EnvironmentPresetPackDiagnostic {
    EnvironmentPresetPackDiagnosticCode code{EnvironmentPresetPackDiagnosticCode::none};
    std::uint32_t preset_index{0};
    std::string field;
    std::string id;
    std::string message;
};

struct EnvironmentPresetPackValidationResult {
    EnvironmentPresetPackValidationStatus status{EnvironmentPresetPackValidationStatus::invalid};
    std::vector<EnvironmentPresetPackDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] std::string_view environment_preset_pack_format_v1() noexcept;

[[nodiscard]] EnvironmentPresetPackValidationResult
validate_environment_preset_pack_v1(const EnvironmentPresetPackDocumentV1& document);
[[nodiscard]] bool is_valid_environment_preset_pack_v1(const EnvironmentPresetPackDocumentV1& document) noexcept;
[[nodiscard]] bool has_environment_preset_pack_diagnostic(const EnvironmentPresetPackValidationResult& result,
                                                          EnvironmentPresetPackDiagnosticCode code) noexcept;

[[nodiscard]] EnvironmentPresetAssetLibraryProductionResult
evaluate_environment_preset_asset_library_production(const EnvironmentPresetAssetLibraryProductionDesc& desc);

[[nodiscard]] std::string serialize_environment_preset_pack_v1(const EnvironmentPresetPackDocumentV1& document);
[[nodiscard]] EnvironmentPresetPackDocumentV1 deserialize_environment_preset_pack_v1(std::string_view text);

} // namespace mirakana
