// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/environment/environment_profile.hpp"

#include <cstddef>
#include <string>

namespace {

[[nodiscard]] std::size_t diagnostic_count(const mirakana::EnvironmentProfileValidationResult& result,
                                           mirakana::EnvironmentProfileDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

} // namespace

MK_TEST("environment profile validation accepts clean first party defaults") {
    mirakana::EnvironmentProfileDesc desc{};
    desc.id = "default_outdoor";

    const auto result = mirakana::validate_environment_profile(desc);

    MK_REQUIRE(result.status == mirakana::EnvironmentProfileValidationStatus::valid);
    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(mirakana::is_valid_environment_profile(desc));
}

MK_TEST("environment profile validation rejects native backend and editor tokens") {
    mirakana::EnvironmentProfileDesc desc{};
    desc.id = "native_vk_editor_profile";

    const auto result = mirakana::validate_environment_profile(desc);

    MK_REQUIRE(result.status == mirakana::EnvironmentProfileValidationStatus::invalid);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(diagnostic_count(result, mirakana::EnvironmentProfileDiagnosticCode::forbidden_profile_id_token) == 1U);
    MK_REQUIRE(mirakana::has_environment_profile_diagnostic(
        result, mirakana::EnvironmentProfileDiagnosticCode::forbidden_profile_id_token));
    MK_REQUIRE(!mirakana::is_valid_environment_profile(desc));
}

MK_TEST("environment profile collection validation rejects duplicate ids deterministically") {
    mirakana::EnvironmentProfileDesc first{};
    first.id = "overcast_evening";
    mirakana::EnvironmentProfileDesc second{};
    second.id = "overcast_evening";

    const mirakana::EnvironmentProfileDesc profiles[] = {first, second};
    const auto result = mirakana::validate_environment_profiles(profiles);

    MK_REQUIRE(result.status == mirakana::EnvironmentProfileValidationStatus::invalid);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.diagnostics.size() == 1U);
    MK_REQUIRE(result.diagnostics[0].code == mirakana::EnvironmentProfileDiagnosticCode::duplicate_profile_id);
    MK_REQUIRE(result.diagnostics[0].profile_index == 1U);
    MK_REQUIRE(result.diagnostics[0].field == "id");
    MK_REQUIRE(result.diagnostics[0].id == "overcast_evening");
}

MK_TEST("environment profile validation rejects invalid weather atmosphere and quality values") {
    mirakana::EnvironmentProfileDesc desc{};
    desc.sun.direction = mirakana::Vec3{};
    desc.atmosphere.atmosphere_height_km = -1.0F;
    desc.fog.density = -0.25F;
    desc.precipitation.kind = mirakana::EnvironmentPrecipitationKind::rain;
    desc.precipitation.intensity = 1.5F;

    const auto result = mirakana::validate_environment_profile(desc);

    MK_REQUIRE(result.status == mirakana::EnvironmentProfileValidationStatus::invalid);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(diagnostic_count(result, mirakana::EnvironmentProfileDiagnosticCode::invalid_direction) == 1U);
    MK_REQUIRE(diagnostic_count(result, mirakana::EnvironmentProfileDiagnosticCode::invalid_scalar) == 3U);
}

MK_TEST("environment profile validation rejects invalid enum casts") {
    mirakana::EnvironmentProfileDesc desc{};
    desc.id = "invalid_enum_case";
    desc.sky_model = static_cast<mirakana::EnvironmentSkyModel>(255U);
    desc.weather = static_cast<mirakana::EnvironmentWeatherKind>(255U);

    const auto result = mirakana::validate_environment_profile(desc);

    MK_REQUIRE(result.status == mirakana::EnvironmentProfileValidationStatus::invalid);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(diagnostic_count(result, mirakana::EnvironmentProfileDiagnosticCode::invalid_enum) == 2U);
}

int main() {
    return mirakana::test::run_all();
}
