// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/environment/environment_io.hpp"
#include "mirakana/environment/environment_profile.hpp"

#include <cstddef>
#include <stdexcept>
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

MK_TEST("environment profile text io round trips with stable key ordering") {
    mirakana::EnvironmentProfileDesc desc{};
    desc.id = "default_outdoor";
    desc.sky_model = mirakana::EnvironmentSkyModel::physical_atmosphere;
    desc.weather = mirakana::EnvironmentWeatherKind::clear;
    desc.sun.direction = mirakana::Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F};
    desc.sun.illuminance_lux = 100000.0F;
    desc.fog.enabled = false;
    desc.precipitation.kind = mirakana::EnvironmentPrecipitationKind::none;

    const auto serialized = mirakana::serialize_environment_profile(desc);

    MK_REQUIRE(serialized.find("format=GameEngine.EnvironmentProfile.v1\n") == 0U);
    MK_REQUIRE(serialized.find("id=default_outdoor\nsky.model=physical_atmosphere\nweather.kind=clear\n") !=
               std::string::npos);
    MK_REQUIRE(serialized.find("sun.direction=0,-1,0\n") != std::string::npos);
    MK_REQUIRE(serialized.find("sun.illuminance_lux=100000\n") != std::string::npos);
    MK_REQUIRE(serialized.find("fog.enabled=false\n") != std::string::npos);
    MK_REQUIRE(serialized.find("precipitation.kind=none\n") != std::string::npos);

    const auto round_trip = mirakana::deserialize_environment_profile(serialized);
    MK_REQUIRE(round_trip.id == "default_outdoor");
    MK_REQUIRE(round_trip.sky_model == mirakana::EnvironmentSkyModel::physical_atmosphere);
    MK_REQUIRE(round_trip.weather == mirakana::EnvironmentWeatherKind::clear);
    MK_REQUIRE(round_trip.precipitation.kind == mirakana::EnvironmentPrecipitationKind::none);
    MK_REQUIRE(mirakana::serialize_environment_profile(round_trip) == serialized);
}

MK_TEST("environment profile text io rejects malformed documents") {
    for (const auto* document : {
             "format=GameEngine.LegacyEnvironment.v1\nid=default_outdoor\n",
             "format=GameEngine.EnvironmentProfile.v1\nid=default_outdoor\nid=duplicate\n",
             "format=GameEngine.EnvironmentProfile.v1\nid=default_outdoor\nsky.model=backend_handle\n",
             "format=GameEngine.EnvironmentProfile.v1\nid=default_outdoor\nsun.direction=nan,-1,0\n",
             "format=GameEngine.EnvironmentProfile.v1\nid=default_outdoor\nprecipitation.intensity=-0.1\n",
             "format=GameEngine.EnvironmentProfile.v1\nid=native_vk_editor\n",
         }) {
        bool threw = false;
        try {
            (void)mirakana::deserialize_environment_profile(document);
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        MK_REQUIRE(threw);
    }
}

int main() {
    return mirakana::test::run_all();
}
