// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

struct NativeAssetImportExternalCopyInput {
    std::string absolute_source_path;
    std::string target_project_path;
};

struct NativeAssetImportExternalCopyResultRow {
    std::string source_path;
    std::string target_project_path;
    std::string diagnostic;
    bool copied{false};
};

struct NativeAssetImportExternalCopyResult {
    std::vector<NativeAssetImportExternalCopyResultRow> rows;
    std::vector<std::string> target_project_paths;
    std::vector<std::string> diagnostics;
    std::size_t copied_count{0};
    bool succeeded{false};
};

[[nodiscard]] NativeAssetImportExternalCopyResult
copy_reviewed_external_asset_sources_to_project(std::string_view project_root,
                                                std::span<const NativeAssetImportExternalCopyInput> inputs);

#if defined(MK_EDITOR_ASSET_IMPORT_COPY_TEST_HOOKS)
void set_native_asset_import_external_copy_fail_after_finalized_count_for_tests(std::size_t count) noexcept;
#endif

} // namespace mirakana::editor
