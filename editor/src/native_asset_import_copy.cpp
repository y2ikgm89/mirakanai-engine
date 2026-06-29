// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "native_asset_import_copy.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <unordered_set>
#include <vector>

namespace mirakana::editor {
namespace {

#if defined(MK_EDITOR_ASSET_IMPORT_COPY_TEST_HOOKS)
class CopyFailureInjectionForTests final {
  public:
    [[nodiscard]] std::uint64_t fail_after_finalized_count() const noexcept {
        return fail_after_finalized_count_.load();
    }

    void set_fail_after_finalized_count(std::size_t count) const noexcept {
        fail_after_finalized_count_.store(count);
    }

  private:
    mutable std::atomic_uint64_t fail_after_finalized_count_{0U};
};

[[nodiscard]] const CopyFailureInjectionForTests& copy_failure_injection_for_tests() noexcept {
    static const CopyFailureInjectionForTests injection;
    return injection;
}
#endif

[[nodiscard]] bool contains_line_separator(std::string_view value) noexcept {
    return value.find('\n') != std::string_view::npos || value.find('\r') != std::string_view::npos;
}

[[nodiscard]] bool is_device_path(std::string_view path) {
    auto normalized = std::string(path);
    std::ranges::replace(normalized, '\\', '/');
    return normalized.starts_with("//./") || normalized.starts_with("//?/");
}

[[nodiscard]] bool has_parent_segment(const std::filesystem::path& path) {
    return std::ranges::any_of(path, [](const std::filesystem::path& segment) { return segment == ".."; });
}

[[nodiscard]] bool is_path_prefix(const std::filesystem::path& prefix, const std::filesystem::path& path) {
    auto prefix_it = prefix.begin();
    auto path_it = path.begin();
    for (; prefix_it != prefix.end(); ++prefix_it, ++path_it) {
        if (path_it == path.end() || *prefix_it != *path_it) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool is_strict_child_path(const std::filesystem::path& root, const std::filesystem::path& path) {
    return is_path_prefix(root, path) && path != root;
}

[[nodiscard]] bool contains_invalid_target_character(std::string_view path) noexcept {
    return contains_line_separator(path) || path.find('=') != std::string_view::npos ||
           path.find(';') != std::string_view::npos || path.find('\\') != std::string_view::npos ||
           path.find(':') != std::string_view::npos;
}

void push_diagnostic(NativeAssetImportExternalCopyResult& result, NativeAssetImportExternalCopyResultRow& row,
                     std::string diagnostic) {
    row.diagnostic = std::move(diagnostic);
    result.diagnostics.push_back(row.diagnostic);
}

[[nodiscard]] bool reject_target_parent_symlink_or_file(const std::filesystem::path& root,
                                                        const std::filesystem::path& relative_parent,
                                                        std::string& diagnostic) {
    auto current = root;
    for (const auto& segment : relative_parent) {
        if (segment.empty() || segment == ".") {
            continue;
        }
        current /= segment;
        std::error_code status_error;
        const auto status = std::filesystem::symlink_status(current, status_error);
        if (status_error) {
            if (status.type() == std::filesystem::file_type::not_found) {
                return false;
            }
            diagnostic = "external import source copy target parent status failed: " + status_error.message();
            return true;
        }
        if (!std::filesystem::exists(status)) {
            return false;
        }
        if (std::filesystem::is_symlink(status)) {
            diagnostic = "external import source copy target parent must not be a symlink";
            return true;
        }
        if (!std::filesystem::is_directory(status)) {
            diagnostic = "external import source copy target parent must be a directory";
            return true;
        }
    }
    return false;
}

[[nodiscard]] std::filesystem::path next_temp_path(const std::filesystem::path& target, int attempt) {
    const auto timestamp = static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    const auto thread_id = static_cast<std::uint64_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    const auto suffix =
        ".copying-" + std::to_string(timestamp) + "-" + std::to_string(thread_id) + "-" + std::to_string(attempt);
    return std::filesystem::path{target.generic_string() + suffix};
}

void remove_temp_file(const std::filesystem::path& temp_path) noexcept {
    if (temp_path.empty()) {
        return;
    }
    std::error_code remove_error;
    std::filesystem::remove(temp_path, remove_error);
}

void remove_created_file(const std::filesystem::path& path) noexcept {
    if (path.empty()) {
        return;
    }
    std::error_code remove_error;
    std::filesystem::remove(path, remove_error);
}

struct PreparedCopy {
    std::filesystem::path source;
    std::filesystem::path target;
    std::filesystem::path temp;
};

} // namespace

NativeAssetImportExternalCopyResult
copy_reviewed_external_asset_sources_to_project(std::string_view project_root,
                                                std::span<const NativeAssetImportExternalCopyInput> inputs) {
    NativeAssetImportExternalCopyResult result;
    result.rows.reserve(inputs.size());

    if (inputs.empty()) {
        result.diagnostics.push_back("external import source copy requires at least one source");
        return result;
    }

    std::error_code root_error;
    const auto root = std::filesystem::canonical(std::filesystem::path{std::string(project_root)}, root_error);
    if (root_error || !std::filesystem::is_directory(root)) {
        result.diagnostics.push_back("external import source copy project root must be an existing directory");
        return result;
    }

    std::vector<PreparedCopy> prepared;
    prepared.reserve(inputs.size());
    std::unordered_set<std::string> target_set;
    target_set.reserve(inputs.size());

    for (const auto& input : inputs) {
        NativeAssetImportExternalCopyResultRow row{
            .source_path = input.absolute_source_path,
            .target_project_path = input.target_project_path,
        };

        if (input.absolute_source_path.empty()) {
            push_diagnostic(result, row, "external import source copy source path is required");
        } else if (is_device_path(input.absolute_source_path)) {
            push_diagnostic(result, row, "external import source copy source path must not be a device path");
        } else {
            const auto source = std::filesystem::path{input.absolute_source_path};
            if (!source.is_absolute()) {
                push_diagnostic(result, row, "external import source copy source path must be absolute");
            } else {
                std::error_code source_status_error;
                const auto source_link_status = std::filesystem::symlink_status(source, source_status_error);
                if (source_status_error || !std::filesystem::exists(source_link_status)) {
                    push_diagnostic(result, row, "external import source copy source path does not exist");
                } else if (std::filesystem::is_symlink(source_link_status)) {
                    push_diagnostic(result, row, "external import source copy source path must not be a symlink");
                } else {
                    std::error_code source_error;
                    const auto source_status = std::filesystem::status(source, source_error);
                    if (source_error || !std::filesystem::is_regular_file(source_status)) {
                        push_diagnostic(result, row, "external import source copy source path must be a regular file");
                    }
                }
            }
        }

        const auto target_relative = std::filesystem::path{input.target_project_path};
        if (row.diagnostic.empty()) {
            if (input.target_project_path.empty()) {
                push_diagnostic(result, row, "external import source copy target project path is required");
            } else if (is_device_path(input.target_project_path) || target_relative.is_absolute() ||
                       has_parent_segment(target_relative) ||
                       contains_invalid_target_character(input.target_project_path)) {
                push_diagnostic(result, row, "external import source copy target must be a safe project-relative path");
            } else if (const auto filename = target_relative.filename().generic_string();
                       filename.empty() || filename == "." || filename == "..") {
                push_diagnostic(result, row, "external import source copy target filename is invalid");
            } else {
                const auto target = (root / target_relative).lexically_normal();
                if (!is_strict_child_path(root, target)) {
                    push_diagnostic(result, row, "external import source copy target must be inside the project root");
                } else if (!target_set.insert(target.generic_string()).second) {
                    push_diagnostic(result, row, "external import source copy target path is duplicated");
                } else {
                    std::string target_parent_diagnostic;
                    if (reject_target_parent_symlink_or_file(root, target_relative.parent_path(),
                                                             target_parent_diagnostic)) {
                        push_diagnostic(result, row, std::move(target_parent_diagnostic));
                    } else {
                        std::error_code target_status_error;
                        const auto target_status = std::filesystem::symlink_status(target, target_status_error);
                        if (target_status_error) {
                            if (target_status.type() == std::filesystem::file_type::not_found) {
                                prepared.push_back(PreparedCopy{
                                    .source = std::filesystem::path{input.absolute_source_path},
                                    .target = target,
                                });
                            } else {
                                push_diagnostic(result, row,
                                                "external import source copy target status failed: " +
                                                    target_status_error.message());
                            }
                        } else if (!std::filesystem::exists(target_status)) {
                            prepared.push_back(PreparedCopy{
                                .source = std::filesystem::path{input.absolute_source_path},
                                .target = target,
                            });
                        } else if (std::filesystem::is_symlink(target_status)) {
                            push_diagnostic(result, row, "external import source copy target must not be a symlink");
                        } else if (std::filesystem::exists(target_status)) {
                            push_diagnostic(result, row, "external import source copy target already exists");
                        }
                    }
                }
            }
        }

        result.rows.push_back(std::move(row));
    }

    if (!result.diagnostics.empty()) {
        return result;
    }

    for (auto& copy : prepared) {
        std::error_code directory_error;
        std::filesystem::create_directories(copy.target.parent_path(), directory_error);
        if (directory_error) {
            result.diagnostics.push_back("external import source copy failed to create target directory: " +
                                         directory_error.message());
            return result;
        }

        std::error_code canonical_parent_error;
        const auto canonical_parent = std::filesystem::canonical(copy.target.parent_path(), canonical_parent_error);
        if (canonical_parent_error || !is_path_prefix(root, canonical_parent)) {
            result.diagnostics.push_back("external import source copy target directory escaped the project root");
            return result;
        }

        for (int attempt = 0; attempt < 32; ++attempt) {
            copy.temp = next_temp_path(copy.target, attempt);
            std::error_code temp_exists_error;
            const auto temp_exists = std::filesystem::exists(copy.temp, temp_exists_error);
            if (temp_exists_error) {
                result.diagnostics.push_back("external import source copy temp status failed: " +
                                             temp_exists_error.message());
                for (const auto& staged : prepared) {
                    remove_temp_file(staged.temp);
                }
                return result;
            }
            if (temp_exists) {
                continue;
            }

            std::error_code copy_error;
            if (std::filesystem::copy_file(copy.source, copy.temp, std::filesystem::copy_options::none, copy_error)) {
                break;
            }
            remove_temp_file(copy.temp);
            if (copy_error) {
                result.diagnostics.push_back("external import source copy failed: " + copy_error.message());
                for (const auto& staged : prepared) {
                    remove_temp_file(staged.temp);
                }
                return result;
            }
        }

        if (copy.temp.empty() || !std::filesystem::exists(copy.temp)) {
            result.diagnostics.push_back("external import source copy failed to allocate a temp target");
            for (const auto& staged : prepared) {
                remove_temp_file(staged.temp);
            }
            return result;
        }
    }

    for (const auto& copy : prepared) {
        std::error_code final_status_error;
        const auto final_status = std::filesystem::symlink_status(copy.target, final_status_error);
        if (final_status_error && final_status.type() != std::filesystem::file_type::not_found) {
            result.diagnostics.push_back("external import source copy target status failed: " +
                                         final_status_error.message());
            for (const auto& staged : prepared) {
                remove_temp_file(staged.temp);
            }
            return result;
        }
        if (std::filesystem::exists(final_status)) {
            result.diagnostics.push_back("external import source copy target already exists");
            for (const auto& staged : prepared) {
                remove_temp_file(staged.temp);
            }
            return result;
        }
    }

    std::vector<std::filesystem::path> finalized_targets;
    finalized_targets.reserve(prepared.size());
    for (std::size_t index = 0; index < prepared.size(); ++index) {
        auto& copy = prepared[index];
        // std::filesystem::rename can replace an existing POSIX target; hard-link promotion is no-overwrite.
        std::error_code link_error;
        std::filesystem::create_hard_link(copy.temp, copy.target, link_error);
        if (link_error) {
            result.diagnostics.push_back("external import source copy final link failed: " + link_error.message());
            for (const auto& staged : prepared) {
                remove_temp_file(staged.temp);
            }
            for (const auto& finalized : finalized_targets) {
                remove_created_file(finalized);
            }
            return result;
        }
        finalized_targets.push_back(copy.target);
#if defined(MK_EDITOR_ASSET_IMPORT_COPY_TEST_HOOKS)
        if (const auto fail_after = copy_failure_injection_for_tests().fail_after_finalized_count();
            fail_after > 0U && finalized_targets.size() >= fail_after) {
            result.diagnostics.push_back("external import source copy injected finalization failure");
            for (const auto& staged : prepared) {
                remove_temp_file(staged.temp);
            }
            for (const auto& finalized : finalized_targets) {
                remove_created_file(finalized);
            }
            return result;
        }
#endif
    }

    for (std::size_t index = 0; index < prepared.size(); ++index) {
        auto& copy = prepared[index];
        remove_temp_file(copy.temp);
        copy.temp.clear();
        result.rows[index].copied = true;
        result.target_project_paths.push_back(inputs[index].target_project_path);
        ++result.copied_count;
    }

    result.succeeded = result.copied_count == inputs.size();
    return result;
}

#if defined(MK_EDITOR_ASSET_IMPORT_COPY_TEST_HOOKS)
void set_native_asset_import_external_copy_fail_after_finalized_count_for_tests(std::size_t count) noexcept {
    copy_failure_injection_for_tests().set_fail_after_finalized_count(count);
}
#endif

} // namespace mirakana::editor
