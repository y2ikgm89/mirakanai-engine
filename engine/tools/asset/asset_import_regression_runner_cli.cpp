// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/asset_import_presets.hpp"
#include "mirakana/assets/asset_import_regression_corpus.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/tools/asset_import_regression_runner.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {
namespace {

struct CliOptions {
    std::string corpus_root;
    std::string output_root;
    std::string report_path;
    std::optional<std::string> presets_path;
    bool write_cooked_outputs{false};
    bool compare_expected_hashes{false};
    bool collect_preview_rows{false};
    std::uint64_t row_budget{10000U};
};

[[nodiscard]] bool contains_parent_segment(std::string_view value) noexcept {
    std::size_t segment_begin = 0U;
    while (segment_begin <= value.size()) {
        const auto segment_end = value.find('/', segment_begin);
        const auto segment =
            value.substr(segment_begin, segment_end == std::string_view::npos ? value.size() - segment_begin
                                                                              : segment_end - segment_begin);
        if (segment == "..") {
            return true;
        }
        if (segment_end == std::string_view::npos) {
            break;
        }
        segment_begin = segment_end + 1U;
    }
    return false;
}

[[nodiscard]] bool contains_empty_or_dot_segment(std::string_view value) noexcept {
    std::size_t segment_begin = 0U;
    while (segment_begin <= value.size()) {
        const auto segment_end = value.find('/', segment_begin);
        const auto segment =
            value.substr(segment_begin, segment_end == std::string_view::npos ? value.size() - segment_begin
                                                                              : segment_end - segment_begin);
        if (segment.empty() || segment == ".") {
            return true;
        }
        if (segment_end == std::string_view::npos) {
            break;
        }
        segment_begin = segment_end + 1U;
    }
    return false;
}

[[nodiscard]] bool contains_control_text(std::string_view value) noexcept {
    return std::ranges::any_of(value, [](char value) {
        const auto byte = static_cast<unsigned char>(value);
        return byte < 0x20U || byte == 0x7FU;
    });
}

[[nodiscard]] bool looks_like_network_url(std::string_view value) noexcept {
    return value.find("://") != std::string_view::npos || value.starts_with("http://") ||
           value.starts_with("https://") || value.starts_with("file://");
}

[[nodiscard]] bool is_safe_relative_path(std::string_view value) noexcept {
    return !value.empty() && value.front() != '/' && value.find('\\') == std::string_view::npos &&
           value.find(':') == std::string_view::npos && !contains_control_text(value) &&
           !contains_parent_segment(value) && !contains_empty_or_dot_segment(value);
}

void assert_reviewed_path(std::string_view option_name, std::string_view value) {
    if (looks_like_network_url(value)) {
        throw std::invalid_argument("network_url_rejected: " + std::string{option_name});
    }
    if (!is_safe_relative_path(value)) {
        throw std::invalid_argument("unsafe_relative_path: " + std::string{option_name});
    }
}

[[nodiscard]] std::vector<std::string> split_path(std::string_view value) {
    std::vector<std::string> segments;
    std::size_t segment_begin = 0U;
    while (segment_begin <= value.size()) {
        const auto segment_end = value.find('/', segment_begin);
        segments.push_back(std::string{value.substr(segment_begin, segment_end == std::string_view::npos
                                                                       ? value.size() - segment_begin
                                                                       : segment_end - segment_begin)});
        if (segment_end == std::string_view::npos) {
            break;
        }
        segment_begin = segment_end + 1U;
    }
    return segments;
}

void assert_no_reparse_point_prefix(const std::filesystem::path& repo_root, std::string_view value,
                                    std::string_view option_name) {
    std::filesystem::path probe = repo_root;
    for (const auto& segment : split_path(value)) {
        probe /= segment;
        std::error_code error;
        const auto status = std::filesystem::symlink_status(probe, error);
        if (error) {
            return;
        }
        if (std::filesystem::is_symlink(status)) {
            throw std::invalid_argument("reparse_point_rejected: " + std::string{option_name});
        }
    }
}

void assert_existing_directory(const std::filesystem::path& repo_root, std::string_view value,
                               std::string_view option_name) {
    const auto full_path = repo_root / std::filesystem::path{std::string{value}};
    std::error_code error;
    if (!std::filesystem::is_directory(full_path, error)) {
        throw std::invalid_argument("directory_missing: " + std::string{option_name});
    }
}

[[nodiscard]] std::string join_relative_path(std::string_view root, std::string_view leaf) {
    std::string path{root};
    if (!path.ends_with('/')) {
        path.push_back('/');
    }
    path += leaf;
    return path;
}

[[nodiscard]] std::uint64_t parse_u64(std::string_view value, std::string_view option_name) {
    if (value.empty()) {
        throw std::invalid_argument("invalid_integer: " + std::string{option_name});
    }
    std::uint64_t parsed = 0U;
    for (const char character : value) {
        if (character < '0' || character > '9') {
            throw std::invalid_argument("invalid_integer: " + std::string{option_name});
        }
        const auto digit = static_cast<std::uint64_t>(character - '0');
        if (parsed > (UINT64_MAX - digit) / 10U) {
            throw std::invalid_argument("invalid_integer: " + std::string{option_name});
        }
        parsed = (parsed * 10U) + digit;
    }
    if (parsed == 0U) {
        throw std::invalid_argument("invalid_integer: " + std::string{option_name});
    }
    return parsed;
}

[[nodiscard]] std::string require_value(int argc, char** argv, int& index, std::string_view option_name) {
    if (index + 1 >= argc) {
        throw std::invalid_argument("missing_argument_value: " + std::string{option_name});
    }
    ++index;
    return std::string{argv[index]};
}

[[nodiscard]] CliOptions parse_args(int argc, char** argv) {
    CliOptions options;
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument{argv[index]};
        if (argument == "--corpus-root") {
            options.corpus_root = require_value(argc, argv, index, argument);
        } else if (argument == "--output-root") {
            options.output_root = require_value(argc, argv, index, argument);
        } else if (argument == "--write-report") {
            options.report_path = require_value(argc, argv, index, argument);
        } else if (argument == "--presets") {
            options.presets_path = require_value(argc, argv, index, argument);
        } else if (argument == "--write-cooked-outputs") {
            options.write_cooked_outputs = true;
        } else if (argument == "--compare-expected-hashes") {
            options.compare_expected_hashes = true;
        } else if (argument == "--collect-preview-rows") {
            options.collect_preview_rows = true;
        } else if (argument == "--row-budget") {
            options.row_budget = parse_u64(require_value(argc, argv, index, argument), argument);
        } else {
            throw std::invalid_argument("unknown_argument: " + std::string{argument});
        }
    }

    if (options.corpus_root.empty()) {
        throw std::invalid_argument("missing_required_argument: --corpus-root");
    }
    if (options.output_root.empty()) {
        throw std::invalid_argument("missing_required_argument: --output-root");
    }
    if (options.report_path.empty()) {
        throw std::invalid_argument("missing_required_argument: --write-report");
    }
    return options;
}

[[nodiscard]] std::uint32_t rotr(std::uint32_t value, int shift) noexcept {
    return (value >> shift) | (value << (32 - shift));
}

[[nodiscard]] std::string sha256_label(std::string_view bytes) {
    static constexpr std::array<std::uint32_t, 64> round_constants{
        0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
        0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
        0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
        0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U, 0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
        0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
        0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
        0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
        0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U, 0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U};

    std::array<std::uint32_t, 8> hash{0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
                                      0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U};
    std::vector<std::uint8_t> data;
    data.reserve(bytes.size() + 72U);
    for (const char value : bytes) {
        data.push_back(static_cast<std::uint8_t>(value));
    }
    const auto bit_size = static_cast<std::uint64_t>(data.size()) * 8ULL;
    data.push_back(0x80U);
    while ((data.size() % 64U) != 56U) {
        data.push_back(0U);
    }
    for (int shift = 56; shift >= 0; shift -= 8) {
        data.push_back(static_cast<std::uint8_t>((bit_size >> shift) & 0xFFU));
    }

    for (std::size_t chunk = 0; chunk < data.size(); chunk += 64U) {
        std::array<std::uint32_t, 64> words{};
        for (std::size_t index = 0; index < 16U; ++index) {
            const auto offset = chunk + index * 4U;
            words[index] = (static_cast<std::uint32_t>(data[offset]) << 24U) |
                           (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
                           (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) |
                           static_cast<std::uint32_t>(data[offset + 3U]);
        }
        for (std::size_t index = 16U; index < 64U; ++index) {
            const auto s0 = rotr(words[index - 15U], 7) ^ rotr(words[index - 15U], 18) ^ (words[index - 15U] >> 3U);
            const auto s1 = rotr(words[index - 2U], 17) ^ rotr(words[index - 2U], 19) ^ (words[index - 2U] >> 10U);
            words[index] = words[index - 16U] + s0 + words[index - 7U] + s1;
        }

        auto a = hash[0];
        auto b = hash[1];
        auto c = hash[2];
        auto d = hash[3];
        auto e = hash[4];
        auto f = hash[5];
        auto g = hash[6];
        auto h = hash[7];
        for (std::size_t index = 0; index < 64U; ++index) {
            const auto s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            const auto ch = (e & f) ^ ((~e) & g);
            const auto temp1 = h + s1 + ch + round_constants[index] + words[index];
            const auto s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            const auto maj = (a & b) ^ (a & c) ^ (b & c);
            const auto temp2 = s0 + maj;
            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }
        hash[0] += a;
        hash[1] += b;
        hash[2] += c;
        hash[3] += d;
        hash[4] += e;
        hash[5] += f;
        hash[6] += g;
        hash[7] += h;
    }

    std::ostringstream output;
    output << "sha256:";
    for (const auto value : hash) {
        output << std::hex << std::setw(8) << std::setfill('0') << value;
    }
    return output.str();
}

[[nodiscard]] AssetImportRegressionReportV1 make_invalid_manifest_report(std::string message) {
    AssetImportRegressionReportV1 report;
    report.corpus_id = "GameEngine.AssetImportRegressionCorpus.v1";
    report.run_id = "asset-import-regression-invalid-manifest";
    report.asset_count = 0U;
    report.failed_count = 1U;
    report.rows.push_back(AssetImportRegressionReportRowV1{
        .asset_id = "manifest",
        .kind = AssetImportRegressionCorpusAssetKind::gltf_mesh,
        .asset = AssetId::from_name("asset_import_regression/manifest"),
        .source_path = "",
        .source_sha256 = "",
        .preset_sha256 = "",
        .importer_id = "mirakana.importer.manifest",
        .importer_version = "asset-import-regression-runner-cli.v1",
        .phase = "manifest",
        .code = AssetImportRegressionDiagnosticCode::invalid_manifest,
        .message = std::move(message),
        .deterministic_output_hash = "",
        .succeeded = false,
        .ready_for_commercial_evidence = false,
    });
    return report;
}

void write_summary(std::string_view report_path, const AssetImportRegressionReportV1& report,
                   std::string_view serialized_report) {
    std::cout << "asset_import_regression_report=" << report_path << '\n';
    std::cout << "asset_import_regression_asset_count=" << report.asset_count << '\n';
    std::cout << "asset_import_regression_succeeded_count=" << report.succeeded_count << '\n';
    std::cout << "asset_import_regression_failed_count=" << report.failed_count << '\n';
    std::cout << "asset_import_regression_legal_blocked_count=" << report.legal_blocked_count << '\n';
    std::cout << "asset_import_regression_nondeterministic_count=" << report.nondeterministic_count << '\n';
    std::cout << "asset_import_regression_ready=" << (report.ready ? 1 : 0) << '\n';
    std::cout << "asset_import_regression_replay_hash=" << sha256_label(serialized_report) << '\n';
}

int run(int argc, char** argv) {
    const auto options = parse_args(argc, argv);
    assert_reviewed_path("--corpus-root", options.corpus_root);
    assert_reviewed_path("--output-root", options.output_root);
    assert_reviewed_path("--write-report", options.report_path);
    if (options.presets_path.has_value()) {
        assert_reviewed_path("--presets", *options.presets_path);
    }

    const auto repo_root = std::filesystem::current_path();
    assert_existing_directory(repo_root, options.corpus_root, "--corpus-root");
    assert_no_reparse_point_prefix(repo_root, options.corpus_root, "--corpus-root");
    assert_no_reparse_point_prefix(repo_root, options.output_root, "--output-root");
    assert_no_reparse_point_prefix(repo_root, options.report_path, "--write-report");
    if (options.presets_path.has_value()) {
        assert_no_reparse_point_prefix(repo_root, *options.presets_path, "--presets");
    }

    RootedFileSystem filesystem{repo_root};
    AssetImportPresetsDocumentV1 presets;
    if (options.presets_path.has_value()) {
        presets = deserialize_asset_import_presets_document(filesystem.read_text(*options.presets_path));
    }

    AssetImportRegressionReportV1 report;
    try {
        auto corpus = deserialize_asset_import_regression_corpus_v1(
            filesystem.read_text(join_relative_path(options.corpus_root, "corpus.gecorpus")));
        AssetImportRegressionRunnerOptions runner_options;
        runner_options.corpus_root = options.corpus_root;
        runner_options.output_root = options.output_root;
        runner_options.project_presets = std::move(presets);
        runner_options.write_cooked_outputs = options.write_cooked_outputs;
        runner_options.compare_expected_hashes = options.compare_expected_hashes;
        runner_options.collect_preview_rows = options.collect_preview_rows;
        runner_options.row_budget = options.row_budget;
        report = run_asset_import_regression_corpus(filesystem, corpus, runner_options);
    } catch (const std::invalid_argument& error) {
        report = make_invalid_manifest_report(error.what());
    }

    const auto serialized_report = serialize_asset_import_regression_report_v1(report);
    filesystem.write_text(options.report_path, serialized_report);
    write_summary(options.report_path, report, serialized_report);
    return 0;
}

} // namespace
} // namespace mirakana

int main(int argc, char** argv) {
    try {
        return mirakana::run(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
