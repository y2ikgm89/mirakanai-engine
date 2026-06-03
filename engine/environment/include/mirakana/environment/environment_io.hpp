// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/environment/environment_profile.hpp"

#include <iosfwd>
#include <string>
#include <string_view>

namespace mirakana {

[[nodiscard]] std::string_view environment_profile_format_v1() noexcept;

[[nodiscard]] std::string_view environment_sky_model_name(EnvironmentSkyModel value) noexcept;
[[nodiscard]] EnvironmentSkyModel parse_environment_sky_model(std::string_view value);

[[nodiscard]] std::string_view environment_weather_kind_name(EnvironmentWeatherKind value) noexcept;
[[nodiscard]] EnvironmentWeatherKind parse_environment_weather_kind(std::string_view value);

[[nodiscard]] std::string_view environment_precipitation_kind_name(EnvironmentPrecipitationKind value) noexcept;
[[nodiscard]] EnvironmentPrecipitationKind parse_environment_precipitation_kind(std::string_view value);

void write_environment_profile_payload(std::ostream& output, const EnvironmentProfileDesc& desc,
                                       std::string_view key_prefix = {});

[[nodiscard]] std::string serialize_environment_profile(const EnvironmentProfileDesc& desc);
[[nodiscard]] EnvironmentProfileDesc deserialize_environment_profile(std::string_view text);

} // namespace mirakana
