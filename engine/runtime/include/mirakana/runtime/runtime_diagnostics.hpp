// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime/session_services.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::runtime {

enum class RuntimeDiagnosticSeverity { unknown, info, warning, error };

enum class RuntimeDiagnosticDomain { unknown, asset_package, payload, scene, session };

struct RuntimeDiagnostic {
    RuntimeDiagnosticSeverity severity{RuntimeDiagnosticSeverity::unknown};
    RuntimeDiagnosticDomain domain{RuntimeDiagnosticDomain::unknown};
    AssetId asset;
    AssetKind kind{AssetKind::unknown};
    std::string path;
    std::string message;
};

struct RuntimeDiagnosticReport {
    std::vector<RuntimeDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
    [[nodiscard]] std::size_t warning_count() const noexcept;
    [[nodiscard]] std::size_t error_count() const noexcept;
};

[[nodiscard]] std::string_view runtime_diagnostic_severity_label(RuntimeDiagnosticSeverity severity) noexcept;
[[nodiscard]] std::string_view runtime_diagnostic_domain_label(RuntimeDiagnosticDomain domain) noexcept;

[[nodiscard]] RuntimeDiagnosticReport
make_runtime_asset_package_load_diagnostic_report(const RuntimeAssetPackageLoadResult& result);
[[nodiscard]] RuntimeDiagnosticReport inspect_runtime_asset_package(const RuntimeAssetPackage& package);
[[nodiscard]] RuntimeDiagnosticReport make_runtime_session_diagnostic_report(const RuntimeSaveDataLoadResult& result,
                                                                             std::string_view path = {});
[[nodiscard]] RuntimeDiagnosticReport make_runtime_session_diagnostic_report(const RuntimeSettingsLoadResult& result,
                                                                             std::string_view path = {});
[[nodiscard]] RuntimeDiagnosticReport
make_runtime_session_diagnostic_report(const RuntimeLocalizationCatalogLoadResult& result, std::string_view path = {});
[[nodiscard]] RuntimeDiagnosticReport
make_runtime_session_diagnostic_report(const RuntimeInputActionMapLoadResult& result, std::string_view path = {});

} // namespace mirakana::runtime
