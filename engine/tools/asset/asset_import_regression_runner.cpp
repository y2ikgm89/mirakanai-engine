// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/asset_import_regression_runner.hpp"

#include "mirakana/assets/asset_package.hpp"
#include "mirakana/assets/asset_source_format.hpp"
#include "mirakana/tools/asset_import_adapters.hpp"
#include "mirakana/tools/gltf_node_animation_import.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

constexpr std::string_view runner_version = "asset-import-regression-runner.v1";

class OverlayFileSystem final : public IFileSystem {
  public:
    explicit OverlayFileSystem(IFileSystem& backing) : backing_(backing) {}

    [[nodiscard]] bool exists(std::string_view path) const override {
        return files_.find(std::string{path}) != files_.end() || backing_.exists(path);
    }

    [[nodiscard]] bool is_directory(std::string_view path) const override {
        const std::string prefix = std::string{path} + "/";
        if (std::ranges::any_of(files_, [&prefix](const auto& file) { return file.first.starts_with(prefix); })) {
            return true;
        }
        return backing_.is_directory(path);
    }

    [[nodiscard]] std::string read_text(std::string_view path) const override {
        const auto it = files_.find(std::string{path});
        if (it != files_.end()) {
            return it->second;
        }
        return backing_.read_text(path);
    }

    [[nodiscard]] std::vector<std::byte> read_binary_range(std::string_view path, std::uint64_t byte_offset,
                                                           std::uint64_t byte_size) const override {
        const auto it = files_.find(std::string{path});
        if (it == files_.end()) {
            return backing_.read_binary_range(path, byte_offset, byte_size);
        }
        const auto& content = it->second;
        if (byte_offset > content.size() || byte_size > content.size() - static_cast<std::size_t>(byte_offset)) {
            throw std::out_of_range("file byte range is out of bounds");
        }
        const auto* begin = content.data() + static_cast<std::size_t>(byte_offset);
        const auto bytes = std::as_bytes(std::span<const char>(begin, static_cast<std::size_t>(byte_size)));
        return std::vector<std::byte>(bytes.begin(), bytes.end());
    }

    [[nodiscard]] std::vector<std::string> list_files(std::string_view root) const override {
        auto files = backing_.list_files(root);
        const std::string prefix{root};
        for (const auto& [path, _] : files_) {
            if (path.starts_with(prefix) && std::ranges::find(files, path) == files.end()) {
                files.push_back(path);
            }
        }
        std::ranges::sort(files);
        return files;
    }

    void write_text(std::string_view path, std::string_view text) override {
        files_[std::string{path}] = std::string{text};
    }

    void remove(std::string_view path) override {
        files_.erase(std::string{path});
    }

    void remove_empty_directory(std::string_view /*path*/) override {}

  private:
    IFileSystem& backing_;
    std::unordered_map<std::string, std::string> files_;
};

[[nodiscard]] bool clean_text(std::string_view value) noexcept {
    return value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos &&
           value.find('\0') == std::string_view::npos;
}

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

[[nodiscard]] bool safe_relative_path(std::string_view value) noexcept {
    return !value.empty() && clean_text(value) && value.front() != '/' && value.find('\\') == std::string_view::npos &&
           value.find(':') == std::string_view::npos && !contains_parent_segment(value);
}

[[nodiscard]] std::string join_relative_path(std::string_view root, std::string_view leaf) {
    if (root.empty()) {
        return std::string{leaf};
    }
    std::string path{root};
    if (!path.ends_with('/')) {
        path.push_back('/');
    }
    path += leaf;
    return path;
}

[[nodiscard]] std::string hex_u64(std::uint64_t value) {
    std::ostringstream output;
    output << std::hex << std::setw(16) << std::setfill('0') << value;
    return output.str();
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

[[nodiscard]] std::string mesh_preset_hash_text(const AssetImportMeshPresetV1& preset,
                                                const std::vector<std::string>& metadata) {
    std::ostringstream output;
    output << "mesh.unit_scale=" << preset.unit_scale << '\n';
    output << "mesh.up_axis=" << asset_import_mesh_up_axis_label(preset.up_axis) << '\n';
    output << "mesh.triangulate=" << (preset.triangulate ? "true" : "false") << '\n';
    output << "mesh.generate_normals=" << (preset.generate_normals ? "true" : "false") << '\n';
    output << "mesh.generate_tangents=" << (preset.generate_tangents ? "true" : "false") << '\n';
    output << "mesh.material_extraction=" << asset_import_mesh_material_extraction_label(preset.material_extraction)
           << '\n';
    output << "metadata.count=" << metadata.size() << '\n';
    for (std::size_t index = 0; index < metadata.size(); ++index) {
        output << "metadata." << index << '=' << metadata[index] << '\n';
    }
    return output.str();
}

[[nodiscard]] AssetKind asset_kind_for(AssetImportRegressionCorpusAssetKind kind) noexcept {
    switch (kind) {
    case AssetImportRegressionCorpusAssetKind::png_texture:
    case AssetImportRegressionCorpusAssetKind::openexr_texture:
    case AssetImportRegressionCorpusAssetKind::ktx2_basis_texture:
        return AssetKind::texture;
    case AssetImportRegressionCorpusAssetKind::gltf_mesh:
        return AssetKind::mesh;
    case AssetImportRegressionCorpusAssetKind::gltf_animation:
        return AssetKind::animation_quaternion_clip;
    case AssetImportRegressionCorpusAssetKind::material_document:
        return AssetKind::material;
    case AssetImportRegressionCorpusAssetKind::gltf_scene:
        return AssetKind::scene;
    case AssetImportRegressionCorpusAssetKind::audio_source:
        return AssetKind::audio;
    }
    return AssetKind::unknown;
}

[[nodiscard]] AssetImportActionKind action_kind_for(AssetImportRegressionCorpusAssetKind kind) noexcept {
    switch (kind) {
    case AssetImportRegressionCorpusAssetKind::png_texture:
    case AssetImportRegressionCorpusAssetKind::openexr_texture:
    case AssetImportRegressionCorpusAssetKind::ktx2_basis_texture:
        return AssetImportActionKind::texture;
    case AssetImportRegressionCorpusAssetKind::gltf_mesh:
        return AssetImportActionKind::mesh;
    case AssetImportRegressionCorpusAssetKind::gltf_animation:
        return AssetImportActionKind::animation_quaternion_clip;
    case AssetImportRegressionCorpusAssetKind::material_document:
        return AssetImportActionKind::material;
    case AssetImportRegressionCorpusAssetKind::gltf_scene:
        return AssetImportActionKind::scene;
    case AssetImportRegressionCorpusAssetKind::audio_source:
        return AssetImportActionKind::audio;
    }
    return AssetImportActionKind::unknown;
}

[[nodiscard]] std::string importer_id_for(AssetImportRegressionCorpusAssetKind kind) {
    switch (kind) {
    case AssetImportRegressionCorpusAssetKind::gltf_scene:
        return "mirakana.importer.gltf_scene";
    case AssetImportRegressionCorpusAssetKind::gltf_mesh:
        return "mirakana.importer.gltf_mesh";
    case AssetImportRegressionCorpusAssetKind::gltf_animation:
        return "mirakana.importer.gltf_animation";
    case AssetImportRegressionCorpusAssetKind::png_texture:
        return "mirakana.importer.png_texture";
    case AssetImportRegressionCorpusAssetKind::openexr_texture:
        return "mirakana.importer.openexr_texture";
    case AssetImportRegressionCorpusAssetKind::ktx2_basis_texture:
        return "mirakana.importer.ktx2_basis_texture";
    case AssetImportRegressionCorpusAssetKind::material_document:
        return "mirakana.importer.material_document";
    case AssetImportRegressionCorpusAssetKind::audio_source:
        return "mirakana.importer.audio_source";
    }
    return "mirakana.importer.unknown";
}

[[nodiscard]] AssetImportRegressionDiagnosticCode diagnostic_code_from_text(std::string_view diagnostic) noexcept {
    if (diagnostic.find("missing source file") != std::string_view::npos ||
        diagnostic.find("file does not exist") != std::string_view::npos) {
        return AssetImportRegressionDiagnosticCode::missing_source_file;
    }
    if (diagnostic.find("feature is disabled") != std::string_view::npos ||
        diagnostic.find("asset importers are disabled") != std::string_view::npos ||
        diagnostic.find("no importer matched") != std::string_view::npos) {
        return AssetImportRegressionDiagnosticCode::unsupported_format;
    }
    if (diagnostic.find("extension") != std::string_view::npos) {
        return AssetImportRegressionDiagnosticCode::unsupported_extension;
    }
    if (diagnostic.find("interpolation") != std::string_view::npos ||
        diagnostic.find("duplicate") != std::string_view::npos ||
        diagnostic.find("channel") != std::string_view::npos) {
        return AssetImportRegressionDiagnosticCode::unsupported_animation_channel;
    }
    if (diagnostic.find("JOINTS_0") != std::string_view::npos ||
        diagnostic.find("WEIGHTS_0") != std::string_view::npos || diagnostic.find("skin") != std::string_view::npos ||
        diagnostic.find("morph") != std::string_view::npos) {
        return AssetImportRegressionDiagnosticCode::unsupported_skin_or_morph_combination;
    }
    if (diagnostic.find("external") != std::string_view::npos || diagnostic.find("buffer") != std::string_view::npos) {
        return AssetImportRegressionDiagnosticCode::missing_external_resource;
    }
    if (diagnostic.find("normalization") != std::string_view::npos ||
        diagnostic.find("mesh preset") != std::string_view::npos) {
        return AssetImportRegressionDiagnosticCode::coordinate_normalization_failed;
    }
    if (diagnostic.find("material") != std::string_view::npos || diagnostic.find("texture") != std::string_view::npos) {
        return AssetImportRegressionDiagnosticCode::material_extraction_failed;
    }
    if (diagnostic.find("PNG") != std::string_view::npos || diagnostic.find("decode") != std::string_view::npos) {
        return AssetImportRegressionDiagnosticCode::texture_decode_failed;
    }
    if (diagnostic.find("glTF") != std::string_view::npos || diagnostic.find("parse") != std::string_view::npos) {
        return AssetImportRegressionDiagnosticCode::parser_error;
    }
    return AssetImportRegressionDiagnosticCode::validator_error;
}

[[nodiscard]] AssetImportRegressionReportRowV1 make_row(const AssetImportRegressionCorpusAssetV1& asset,
                                                        std::string source_path, std::string source_sha256,
                                                        std::string preset_sha256, std::string phase,
                                                        AssetImportRegressionDiagnosticCode code, std::string message,
                                                        std::string output_hash, bool succeeded) {
    return AssetImportRegressionReportRowV1{
        .asset_id = asset.asset_id,
        .kind = asset.kind,
        .asset = AssetId::from_name(asset.asset_key.value),
        .source_path = std::move(source_path),
        .source_sha256 = std::move(source_sha256),
        .preset_sha256 = std::move(preset_sha256),
        .importer_id = importer_id_for(asset.kind),
        .importer_version = std::string{runner_version},
        .phase = std::move(phase),
        .code = code,
        .message = std::move(message),
        .deterministic_output_hash = std::move(output_hash),
        .succeeded = succeeded,
        .ready_for_commercial_evidence = succeeded,
    };
}

[[nodiscard]] AssetImportRegressionReportRowV1 make_manifest_row(std::string message) {
    return AssetImportRegressionReportRowV1{
        .asset_id = "manifest",
        .kind = AssetImportRegressionCorpusAssetKind::gltf_mesh,
        .asset = AssetId::from_name("asset_import_regression/manifest"),
        .source_path = "",
        .source_sha256 = "",
        .preset_sha256 = "",
        .importer_id = "mirakana.importer.manifest",
        .importer_version = std::string{runner_version},
        .phase = "manifest",
        .code = AssetImportRegressionDiagnosticCode::invalid_manifest,
        .message = std::move(message),
        .deterministic_output_hash = "",
        .succeeded = false,
        .ready_for_commercial_evidence = false,
    };
}

[[nodiscard]] std::optional<std::string> make_gltf_animation_source_document(std::string_view source_text,
                                                                             std::string_view source_path,
                                                                             const AssetImportMeshPresetV1& preset,
                                                                             std::string& diagnostic) {
    auto imported = import_gltf_node_transform_animation_quaternion_clip(source_text, source_path, 0U, preset);
    if (!imported.succeeded) {
        diagnostic = imported.diagnostic;
        return std::nullopt;
    }
    return serialize_animation_quaternion_clip_source_document(imported.clip);
}

[[nodiscard]] AssetImportExecutionResult execute_import_twice(IFileSystem& filesystem, const AssetImportPlan& plan,
                                                              const AssetImportExecutionOptions& options,
                                                              std::string& first_output, std::string& second_output) {
    OverlayFileSystem first_overlay{filesystem};
    auto first_result = execute_asset_import_plan(first_overlay, plan, options);
    if (!first_result.succeeded()) {
        return first_result;
    }
    first_output = first_overlay.read_text(plan.actions.front().output_path);

    OverlayFileSystem second_overlay{filesystem};
    auto second_result = execute_asset_import_plan(second_overlay, plan, options);
    if (!second_result.succeeded()) {
        return second_result;
    }
    second_output = second_overlay.read_text(plan.actions.front().output_path);
    return first_result;
}

void finalize_counts(AssetImportRegressionReportV1& report) {
    report.succeeded_count = 0U;
    report.failed_count = 0U;
    report.legal_blocked_count = 0U;
    report.nondeterministic_count = 0U;
    for (const auto& row : report.rows) {
        if (row.succeeded) {
            ++report.succeeded_count;
        } else {
            ++report.failed_count;
        }
        if (row.code == AssetImportRegressionDiagnosticCode::rejected_license ||
            row.code == AssetImportRegressionDiagnosticCode::missing_license_provenance ||
            row.code == AssetImportRegressionDiagnosticCode::external_engine_material) {
            ++report.legal_blocked_count;
        }
        if (row.code == AssetImportRegressionDiagnosticCode::nondeterministic_output) {
            ++report.nondeterministic_count;
        }
    }
    report.ready = report.asset_count > 0U && report.failed_count == 0U && report.legal_blocked_count == 0U &&
                   report.nondeterministic_count == 0U;
}

[[nodiscard]] std::string manifest_diagnostic_message(const std::vector<std::string>& diagnostics) {
    std::ostringstream message;
    message << "asset import regression corpus manifest is invalid";
    for (const auto& diagnostic : diagnostics) {
        message << ';' << diagnostic;
    }
    return message.str();
}

} // namespace

AssetImportRegressionReportV1 run_asset_import_regression_corpus(IFileSystem& filesystem,
                                                                 const AssetImportRegressionCorpusDocumentV1& corpus,
                                                                 const AssetImportRegressionRunnerOptions& options) {
    AssetImportRegressionReportV1 report;
    report.corpus_id = corpus.corpus_id;
    report.asset_count = corpus.assets.size();

    std::ostringstream run_key;
    run_key << corpus.corpus_id << '\n' << corpus.corpus_version << '\n' << corpus.root_path << '\n';
    for (const auto& asset : corpus.assets) {
        run_key << asset.asset_id << '\n' << asset.source_path << '\n';
    }
    report.run_id = "asset-import-regression-" + hex_u64(hash_asset_cooked_content(run_key.str()));

    auto diagnostics = validate_asset_import_regression_corpus_v1(corpus);
    const auto corpus_root = options.corpus_root.empty() ? corpus.root_path : options.corpus_root;
    if (!safe_relative_path(corpus_root)) {
        diagnostics.push_back("options.unsafe_corpus_root");
    }
    const auto output_root =
        options.output_root.empty() ? std::string{"out/asset-import-regression"} : options.output_root;
    if (!safe_relative_path(output_root)) {
        diagnostics.push_back("options.unsafe_output_root");
    }
    if (options.row_budget == 0U || corpus.assets.size() > options.row_budget) {
        diagnostics.push_back("options.row_budget_exceeded");
    }
    if (options.require_large_corpus && corpus.assets.size() < 128U) {
        diagnostics.push_back("options.large_corpus_required");
    }
    if (!diagnostics.empty()) {
        report.rows.push_back(make_manifest_row(manifest_diagnostic_message(diagnostics)));
        finalize_counts(report);
        return report;
    }

    ExternalAssetImportAdapters default_adapters;
    auto execution_options = options.import_execution_options;
    if (execution_options.external_importers.empty()) {
        execution_options = default_adapters.options();
    }

    report.rows.reserve(corpus.assets.size());
    for (const auto& asset : corpus.assets) {
        const auto source_path = join_relative_path(corpus_root, asset.source_path);
        auto preset_review =
            review_asset_import_preset_for_asset(options.project_presets, asset.asset_key, asset_kind_for(asset.kind));
        if (!preset_review.ready) {
            report.rows.push_back(make_row(asset, source_path, "", "", "preset",
                                           AssetImportRegressionDiagnosticCode::coordinate_normalization_failed,
                                           "asset import preset review failed", "", false));
            continue;
        }
        const auto preset_sha256 = sha256_label(mesh_preset_hash_text(preset_review.mesh, preset_review.metadata));

        std::string source_text;
        try {
            if (!filesystem.exists(source_path)) {
                report.rows.push_back(make_row(asset, source_path, "", preset_sha256, "source",
                                               AssetImportRegressionDiagnosticCode::missing_source_file,
                                               "missing source file: " + source_path, "", false));
                continue;
            }
            source_text = filesystem.read_text(source_path);
        } catch (const std::exception& error) {
            report.rows.push_back(make_row(asset, source_path, "", preset_sha256, "source",
                                           diagnostic_code_from_text(error.what()), error.what(), "", false));
            continue;
        }

        const auto source_sha256 = sha256_label(source_text);
        if (options.compare_expected_hashes && !asset.expected_sha256.empty() &&
            asset.expected_sha256 != source_sha256) {
            report.rows.push_back(make_row(asset, source_path, source_sha256, preset_sha256, "source_hash",
                                           AssetImportRegressionDiagnosticCode::source_hash_mismatch,
                                           "source SHA-256 does not match corpus manifest", "", false));
            continue;
        }

        OverlayFileSystem source_overlay{filesystem};
        std::string action_source_path = source_path;
        if (asset.kind == AssetImportRegressionCorpusAssetKind::gltf_animation) {
            std::string diagnostic;
            if (auto source_document =
                    make_gltf_animation_source_document(source_text, source_path, preset_review.mesh, diagnostic)) {
                action_source_path = join_relative_path(output_root, asset.asset_id + ".source.animationclip");
                source_overlay.write_text(action_source_path, *source_document);
            } else if (!diagnostic.empty() && diagnostic.find("asset importers are disabled") == std::string::npos) {
                report.rows.push_back(make_row(asset, source_path, source_sha256, preset_sha256, "parser",
                                               diagnostic_code_from_text(diagnostic), diagnostic, "", false));
                continue;
            }
        }

        const auto output_path = join_relative_path(output_root, asset.asset_id + ".cooked");
        AssetImportPlan plan;
        plan.actions.push_back(AssetImportAction{
            .id = AssetId::from_name(asset.asset_key.value),
            .kind = action_kind_for(asset.kind),
            .source_path = action_source_path,
            .output_path = output_path,
            .dependencies = {},
            .preset_metadata = preset_review.metadata.empty() ? asset.preset_metadata : preset_review.metadata,
            .mesh_preset = preset_review.mesh,
        });

        std::string first_output;
        std::string second_output;
        const auto import_result =
            execute_import_twice(source_overlay, plan, execution_options, first_output, second_output);
        if (!import_result.succeeded()) {
            const auto& failure = import_result.failures.front();
            report.rows.push_back(make_row(asset, source_path, source_sha256, preset_sha256, "cook",
                                           diagnostic_code_from_text(failure.diagnostic), failure.diagnostic, "",
                                           false));
            continue;
        }
        if (first_output != second_output) {
            report.rows.push_back(make_row(asset, source_path, source_sha256, preset_sha256, "determinism",
                                           AssetImportRegressionDiagnosticCode::nondeterministic_output,
                                           "cooked output changed between two executions", "", false));
            continue;
        }
        const auto output_hash = "fnv64:" + hex_u64(hash_asset_cooked_content(first_output));
        if (options.write_cooked_outputs) {
            filesystem.write_text(output_path, first_output);
        }
        report.rows.push_back(make_row(asset, source_path, source_sha256, preset_sha256, "cook",
                                       AssetImportRegressionDiagnosticCode::none, "asset import succeeded", output_hash,
                                       true));
    }

    finalize_counts(report);
    return report;
}

} // namespace mirakana
