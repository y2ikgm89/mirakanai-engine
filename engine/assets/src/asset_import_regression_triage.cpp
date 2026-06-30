// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/asset_import_regression_triage.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iomanip>
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

constexpr std::string_view triage_format_v1 = "GameEngine.AssetImportRegressionTriage.v1";

struct TriageTextRow {
    bool has_asset_id{false};
    bool has_kind{false};
    bool has_asset{false};
    bool has_source_path{false};
    bool has_source_sha256{false};
    bool has_preset_sha256{false};
    bool has_importer_id{false};
    bool has_importer_version{false};
    bool has_phase{false};
    bool has_code{false};
    bool has_severity{false};
    bool has_recommended_action{false};
    bool has_reimport_decision{false};
    bool has_repro_command_id{false};
    bool has_repro_command{false};
    bool has_source_excerpt_hash{false};
    bool has_preset_diff_required{false};
    bool has_axis_unit_preview_required{false};
    bool has_legal_blocked{false};
    bool has_nondeterministic{false};
    AssetImportRegressionTriageRowV1 row;
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

[[nodiscard]] std::string safe_relative_or_redacted(std::string_view value) {
    if (!value.empty() && clean_text(value) && value.front() != '/' && value.find('\\') == std::string_view::npos &&
        value.find(':') == std::string_view::npos && !contains_parent_segment(value) && !value.starts_with("http://") &&
        !value.starts_with("https://")) {
        return std::string{value};
    }
    return "unsafe_source_path_redacted";
}

[[nodiscard]] std::string sanitize_token(std::string_view value) {
    std::string token;
    token.reserve(value.size());
    for (const char character : value) {
        if ((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
            (character >= '0' && character <= '9') || character == '_' || character == '-' || character == '.') {
            token.push_back(character);
        } else {
            token.push_back('_');
        }
    }
    return token.empty() ? "unknown" : token;
}

[[nodiscard]] bool parse_bool(std::string_view value) {
    if (value == "true") {
        return true;
    }
    if (value == "false") {
        return false;
    }
    throw std::invalid_argument("asset import regression triage bool value is invalid");
}

[[nodiscard]] const char* bool_text(bool value) noexcept {
    return value ? "true" : "false";
}

[[nodiscard]] std::uint64_t parse_u64(std::string_view value, std::string_view message) {
    if (value.empty()) {
        throw std::invalid_argument(std::string{message});
    }
    std::uint64_t parsed = 0U;
    for (const char character : value) {
        if (character < '0' || character > '9') {
            throw std::invalid_argument(std::string{message});
        }
        const auto digit = static_cast<std::uint64_t>(character - '0');
        if (parsed > (std::numeric_limits<std::uint64_t>::max() - digit) / 10U) {
            throw std::invalid_argument(std::string{message});
        }
        parsed = (parsed * 10U) + digit;
    }
    return parsed;
}

[[nodiscard]] std::size_t parse_size(std::string_view value, std::string_view message) {
    const auto parsed = parse_u64(value, message);
    if (parsed > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        throw std::invalid_argument(std::string{message});
    }
    return static_cast<std::size_t>(parsed);
}

[[nodiscard]] AssetImportRegressionCorpusAssetKind parse_asset_kind(std::string_view value) {
    for (const auto kind : {
             AssetImportRegressionCorpusAssetKind::gltf_scene,
             AssetImportRegressionCorpusAssetKind::gltf_mesh,
             AssetImportRegressionCorpusAssetKind::gltf_animation,
             AssetImportRegressionCorpusAssetKind::png_texture,
             AssetImportRegressionCorpusAssetKind::openexr_texture,
             AssetImportRegressionCorpusAssetKind::ktx2_basis_texture,
             AssetImportRegressionCorpusAssetKind::material_document,
             AssetImportRegressionCorpusAssetKind::audio_source,
         }) {
        if (value == asset_import_regression_asset_kind_label(kind)) {
            return kind;
        }
    }
    throw std::invalid_argument("asset import regression triage asset kind is unsupported");
}

[[nodiscard]] AssetImportRegressionDiagnosticCode parse_diagnostic_code(std::string_view value) {
    for (const auto code : {
             AssetImportRegressionDiagnosticCode::none,
             AssetImportRegressionDiagnosticCode::invalid_manifest,
             AssetImportRegressionDiagnosticCode::duplicate_asset_id,
             AssetImportRegressionDiagnosticCode::unsafe_source_path,
             AssetImportRegressionDiagnosticCode::missing_source_file,
             AssetImportRegressionDiagnosticCode::source_hash_mismatch,
             AssetImportRegressionDiagnosticCode::missing_license_provenance,
             AssetImportRegressionDiagnosticCode::rejected_license,
             AssetImportRegressionDiagnosticCode::external_engine_material,
             AssetImportRegressionDiagnosticCode::unsupported_format,
             AssetImportRegressionDiagnosticCode::parser_error,
             AssetImportRegressionDiagnosticCode::validator_error,
             AssetImportRegressionDiagnosticCode::missing_external_resource,
             AssetImportRegressionDiagnosticCode::unsafe_external_resource_path,
             AssetImportRegressionDiagnosticCode::unsupported_extension,
             AssetImportRegressionDiagnosticCode::unsupported_animation_channel,
             AssetImportRegressionDiagnosticCode::unsupported_skin_or_morph_combination,
             AssetImportRegressionDiagnosticCode::coordinate_normalization_failed,
             AssetImportRegressionDiagnosticCode::material_extraction_failed,
             AssetImportRegressionDiagnosticCode::texture_decode_failed,
             AssetImportRegressionDiagnosticCode::texture_transcode_failed,
             AssetImportRegressionDiagnosticCode::cooked_output_mismatch,
             AssetImportRegressionDiagnosticCode::nondeterministic_output,
             AssetImportRegressionDiagnosticCode::row_budget_exceeded,
         }) {
        if (value == asset_import_regression_diagnostic_code_label(code)) {
            return code;
        }
    }
    throw std::invalid_argument("asset import regression triage diagnostic code is unsupported");
}

[[nodiscard]] AssetImportRegressionTriageSeverity parse_severity(std::string_view value) {
    if (value == "info") {
        return AssetImportRegressionTriageSeverity::info;
    }
    if (value == "action_required") {
        return AssetImportRegressionTriageSeverity::action_required;
    }
    if (value == "blocked") {
        return AssetImportRegressionTriageSeverity::blocked;
    }
    throw std::invalid_argument("asset import regression triage severity is unsupported");
}

[[nodiscard]] AssetImportRegressionRecommendedAction parse_recommended_action(std::string_view value) {
    for (const auto action : {
             AssetImportRegressionRecommendedAction::none,
             AssetImportRegressionRecommendedAction::fix_notice_or_remove_asset,
             AssetImportRegressionRecommendedAction::refresh_corpus_manifest,
             AssetImportRegressionRecommendedAction::inspect_source_asset,
             AssetImportRegressionRecommendedAction::record_unsupported_or_reduce_source,
             AssetImportRegressionRecommendedAction::open_axis_unit_preview,
             AssetImportRegressionRecommendedAction::inspect_codec_dependency,
             AssetImportRegressionRecommendedAction::rerun_isolated_and_compare_hashes,
             AssetImportRegressionRecommendedAction::split_corpus_or_raise_reviewed_budget,
         }) {
        if (value == asset_import_regression_recommended_action_label(action)) {
            return action;
        }
    }
    throw std::invalid_argument("asset import regression triage recommended action is unsupported");
}

[[nodiscard]] AssetImportRegressionReimportDecision parse_reimport_decision(std::string_view value) {
    if (value == "not_needed") {
        return AssetImportRegressionReimportDecision::not_needed;
    }
    if (value == "dry_run_allowed") {
        return AssetImportRegressionReimportDecision::dry_run_allowed;
    }
    if (value == "blocked") {
        return AssetImportRegressionReimportDecision::blocked;
    }
    throw std::invalid_argument("asset import regression triage reimport decision is unsupported");
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
            const auto offset = chunk + (index * 4U);
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

[[nodiscard]] AssetImportRegressionReimportDecision
reimport_decision_for_action(AssetImportRegressionRecommendedAction action) noexcept {
    switch (action) {
    case AssetImportRegressionRecommendedAction::none:
        return AssetImportRegressionReimportDecision::not_needed;
    case AssetImportRegressionRecommendedAction::open_axis_unit_preview:
    case AssetImportRegressionRecommendedAction::inspect_source_asset:
    case AssetImportRegressionRecommendedAction::inspect_codec_dependency:
        return AssetImportRegressionReimportDecision::dry_run_allowed;
    case AssetImportRegressionRecommendedAction::fix_notice_or_remove_asset:
    case AssetImportRegressionRecommendedAction::refresh_corpus_manifest:
    case AssetImportRegressionRecommendedAction::record_unsupported_or_reduce_source:
    case AssetImportRegressionRecommendedAction::rerun_isolated_and_compare_hashes:
    case AssetImportRegressionRecommendedAction::split_corpus_or_raise_reviewed_budget:
        return AssetImportRegressionReimportDecision::blocked;
    }
    return AssetImportRegressionReimportDecision::blocked;
}

[[nodiscard]] bool legal_blocked_code(AssetImportRegressionDiagnosticCode code) noexcept {
    return code == AssetImportRegressionDiagnosticCode::missing_license_provenance ||
           code == AssetImportRegressionDiagnosticCode::rejected_license ||
           code == AssetImportRegressionDiagnosticCode::external_engine_material;
}

[[nodiscard]] bool preset_diff_required_code(AssetImportRegressionDiagnosticCode code) noexcept {
    return code == AssetImportRegressionDiagnosticCode::material_extraction_failed ||
           code == AssetImportRegressionDiagnosticCode::texture_decode_failed ||
           code == AssetImportRegressionDiagnosticCode::texture_transcode_failed;
}

[[nodiscard]] AssetImportRegressionTriageSeverity
severity_for_decision(AssetImportRegressionReimportDecision decision) noexcept {
    switch (decision) {
    case AssetImportRegressionReimportDecision::not_needed:
        return AssetImportRegressionTriageSeverity::info;
    case AssetImportRegressionReimportDecision::dry_run_allowed:
        return AssetImportRegressionTriageSeverity::action_required;
    case AssetImportRegressionReimportDecision::blocked:
        return AssetImportRegressionTriageSeverity::blocked;
    }
    return AssetImportRegressionTriageSeverity::blocked;
}

[[nodiscard]] bool text_row_complete(const TriageTextRow& row) noexcept {
    return row.has_asset_id && row.has_kind && row.has_asset && row.has_source_path && row.has_source_sha256 &&
           row.has_preset_sha256 && row.has_importer_id && row.has_importer_version && row.has_phase && row.has_code &&
           row.has_severity && row.has_recommended_action && row.has_reimport_decision && row.has_repro_command_id &&
           row.has_repro_command && row.has_source_excerpt_hash && row.has_preset_diff_required &&
           row.has_axis_unit_preview_required && row.has_legal_blocked && row.has_nondeterministic;
}

void parse_triage_row_field(TriageTextRow& row, std::string_view field, std::string_view value) {
    if (field == "asset_id") {
        row.has_asset_id = true;
        row.row.asset_id = std::string{value};
    } else if (field == "kind") {
        row.has_kind = true;
        row.row.kind = parse_asset_kind(value);
    } else if (field == "asset") {
        row.has_asset = true;
        row.row.asset = AssetId{parse_u64(value, "asset import regression triage asset id is invalid")};
    } else if (field == "source_path") {
        row.has_source_path = true;
        row.row.source_path = std::string{value};
    } else if (field == "source_sha256") {
        row.has_source_sha256 = true;
        row.row.source_sha256 = std::string{value};
    } else if (field == "preset_sha256") {
        row.has_preset_sha256 = true;
        row.row.preset_sha256 = std::string{value};
    } else if (field == "importer_id") {
        row.has_importer_id = true;
        row.row.importer_id = std::string{value};
    } else if (field == "importer_version") {
        row.has_importer_version = true;
        row.row.importer_version = std::string{value};
    } else if (field == "phase") {
        row.has_phase = true;
        row.row.phase = std::string{value};
    } else if (field == "code") {
        row.has_code = true;
        row.row.code = parse_diagnostic_code(value);
    } else if (field == "severity") {
        row.has_severity = true;
        row.row.severity = parse_severity(value);
    } else if (field == "recommended_action") {
        row.has_recommended_action = true;
        row.row.recommended_action = parse_recommended_action(value);
    } else if (field == "reimport_decision") {
        row.has_reimport_decision = true;
        row.row.reimport_decision = parse_reimport_decision(value);
    } else if (field == "repro_command_id") {
        row.has_repro_command_id = true;
        row.row.repro_command_id = std::string{value};
    } else if (field == "repro_command") {
        row.has_repro_command = true;
        row.row.repro_command = std::string{value};
    } else if (field == "source_excerpt_hash") {
        row.has_source_excerpt_hash = true;
        row.row.source_excerpt_hash = std::string{value};
    } else if (field == "preset_diff_required") {
        row.has_preset_diff_required = true;
        row.row.preset_diff_required = parse_bool(value);
    } else if (field == "axis_unit_preview_required") {
        row.has_axis_unit_preview_required = true;
        row.row.axis_unit_preview_required = parse_bool(value);
    } else if (field == "legal_blocked") {
        row.has_legal_blocked = true;
        row.row.legal_blocked = parse_bool(value);
    } else if (field == "nondeterministic") {
        row.has_nondeterministic = true;
        row.row.nondeterministic = parse_bool(value);
    } else {
        throw std::invalid_argument("asset import regression triage row field is unsupported");
    }
}

} // namespace

std::string_view asset_import_regression_triage_severity_label(AssetImportRegressionTriageSeverity value) noexcept {
    switch (value) {
    case AssetImportRegressionTriageSeverity::info:
        return "info";
    case AssetImportRegressionTriageSeverity::action_required:
        return "action_required";
    case AssetImportRegressionTriageSeverity::blocked:
        return "blocked";
    }
    return "invalid";
}

std::string_view
asset_import_regression_recommended_action_label(AssetImportRegressionRecommendedAction value) noexcept {
    switch (value) {
    case AssetImportRegressionRecommendedAction::none:
        return "none";
    case AssetImportRegressionRecommendedAction::fix_notice_or_remove_asset:
        return "fix_notice_or_remove_asset";
    case AssetImportRegressionRecommendedAction::refresh_corpus_manifest:
        return "refresh_corpus_manifest";
    case AssetImportRegressionRecommendedAction::inspect_source_asset:
        return "inspect_source_asset";
    case AssetImportRegressionRecommendedAction::record_unsupported_or_reduce_source:
        return "record_unsupported_or_reduce_source";
    case AssetImportRegressionRecommendedAction::open_axis_unit_preview:
        return "open_axis_unit_preview";
    case AssetImportRegressionRecommendedAction::inspect_codec_dependency:
        return "inspect_codec_dependency";
    case AssetImportRegressionRecommendedAction::rerun_isolated_and_compare_hashes:
        return "rerun_isolated_and_compare_hashes";
    case AssetImportRegressionRecommendedAction::split_corpus_or_raise_reviewed_budget:
        return "split_corpus_or_raise_reviewed_budget";
    }
    return "invalid";
}

std::string_view asset_import_regression_reimport_decision_label(AssetImportRegressionReimportDecision value) noexcept {
    switch (value) {
    case AssetImportRegressionReimportDecision::not_needed:
        return "not_needed";
    case AssetImportRegressionReimportDecision::dry_run_allowed:
        return "dry_run_allowed";
    case AssetImportRegressionReimportDecision::blocked:
        return "blocked";
    }
    return "invalid";
}

AssetImportRegressionRecommendedAction
recommended_action_for_asset_import_regression_code(AssetImportRegressionDiagnosticCode code) noexcept {
    switch (code) {
    case AssetImportRegressionDiagnosticCode::none:
        return AssetImportRegressionRecommendedAction::none;
    case AssetImportRegressionDiagnosticCode::invalid_manifest:
    case AssetImportRegressionDiagnosticCode::duplicate_asset_id:
    case AssetImportRegressionDiagnosticCode::unsafe_source_path:
    case AssetImportRegressionDiagnosticCode::row_budget_exceeded:
        return AssetImportRegressionRecommendedAction::split_corpus_or_raise_reviewed_budget;
    case AssetImportRegressionDiagnosticCode::missing_source_file:
    case AssetImportRegressionDiagnosticCode::source_hash_mismatch:
        return AssetImportRegressionRecommendedAction::refresh_corpus_manifest;
    case AssetImportRegressionDiagnosticCode::missing_license_provenance:
    case AssetImportRegressionDiagnosticCode::rejected_license:
    case AssetImportRegressionDiagnosticCode::external_engine_material:
        return AssetImportRegressionRecommendedAction::fix_notice_or_remove_asset;
    case AssetImportRegressionDiagnosticCode::unsupported_format:
    case AssetImportRegressionDiagnosticCode::unsupported_extension:
    case AssetImportRegressionDiagnosticCode::unsupported_animation_channel:
    case AssetImportRegressionDiagnosticCode::unsupported_skin_or_morph_combination:
        return AssetImportRegressionRecommendedAction::record_unsupported_or_reduce_source;
    case AssetImportRegressionDiagnosticCode::parser_error:
    case AssetImportRegressionDiagnosticCode::validator_error:
    case AssetImportRegressionDiagnosticCode::missing_external_resource:
    case AssetImportRegressionDiagnosticCode::unsafe_external_resource_path:
    case AssetImportRegressionDiagnosticCode::material_extraction_failed:
        return AssetImportRegressionRecommendedAction::inspect_source_asset;
    case AssetImportRegressionDiagnosticCode::coordinate_normalization_failed:
        return AssetImportRegressionRecommendedAction::open_axis_unit_preview;
    case AssetImportRegressionDiagnosticCode::texture_decode_failed:
    case AssetImportRegressionDiagnosticCode::texture_transcode_failed:
        return AssetImportRegressionRecommendedAction::inspect_codec_dependency;
    case AssetImportRegressionDiagnosticCode::cooked_output_mismatch:
    case AssetImportRegressionDiagnosticCode::nondeterministic_output:
        return AssetImportRegressionRecommendedAction::rerun_isolated_and_compare_hashes;
    }
    return AssetImportRegressionRecommendedAction::split_corpus_or_raise_reviewed_budget;
}

AssetImportRegressionTriageDocumentV1
make_asset_import_regression_triage_v1(const AssetImportRegressionReportV1& report) {
    AssetImportRegressionTriageDocumentV1 document;
    document.corpus_id = report.corpus_id;
    document.run_id = report.run_id;
    document.rows.reserve(report.rows.size());

    const auto sanitized_run_id = sanitize_token(report.run_id);
    const auto repro_command = "pwsh -NoProfile -ExecutionPolicy Bypass -File "
                               "tools/run-asset-import-regression-corpus.ps1 -CorpusRoot "
                               "out/host-artifacts/asset-import-regression-corpus -OutputRoot "
                               "out/asset-import-regression/staging/" +
                               sanitized_run_id;

    for (const auto& report_row : report.rows) {
        const auto action = recommended_action_for_asset_import_regression_code(report_row.code);
        const auto decision = reimport_decision_for_action(action);
        const bool nondeterministic = report_row.code == AssetImportRegressionDiagnosticCode::nondeterministic_output;
        auto row = AssetImportRegressionTriageRowV1{
            .asset_id = sanitize_token(report_row.asset_id),
            .kind = report_row.kind,
            .asset = report_row.asset,
            .source_path = safe_relative_or_redacted(report_row.source_path),
            .source_sha256 = report_row.source_sha256,
            .preset_sha256 = report_row.preset_sha256,
            .importer_id = sanitize_token(report_row.importer_id),
            .importer_version = sanitize_token(report_row.importer_version),
            .phase = sanitize_token(report_row.phase),
            .code = report_row.code,
            .severity = severity_for_decision(decision),
            .recommended_action = action,
            .reimport_decision = decision,
            .repro_command_id = "asset_import_regression.repro." + sanitize_token(report_row.asset_id),
            .repro_command = repro_command,
            .source_excerpt_hash =
                sha256_label(report_row.asset_id + "|" +
                             std::string{asset_import_regression_diagnostic_code_label(report_row.code)} + "|" +
                             report_row.phase + "|" + report_row.message),
            .preset_diff_required = preset_diff_required_code(report_row.code),
            .axis_unit_preview_required =
                report_row.code == AssetImportRegressionDiagnosticCode::coordinate_normalization_failed,
            .legal_blocked = legal_blocked_code(report_row.code),
            .nondeterministic = nondeterministic,
        };
        if (row.reimport_decision == AssetImportRegressionReimportDecision::blocked) {
            ++document.blocked_count;
        }
        if (row.reimport_decision == AssetImportRegressionReimportDecision::dry_run_allowed) {
            ++document.reimport_candidate_count;
        }
        if (row.preset_diff_required) {
            ++document.preset_diff_required_count;
        }
        if (row.axis_unit_preview_required) {
            ++document.axis_unit_preview_required_count;
        }
        document.rows.push_back(std::move(row));
    }

    document.row_count = document.rows.size();
    document.ready_for_operator_review = document.row_count > 0U;
    return document;
}

std::string serialize_asset_import_regression_triage_v1(const AssetImportRegressionTriageDocumentV1& document) {
    std::ostringstream output;
    output << "format=" << triage_format_v1 << '\n';
    output << "corpus_id=" << document.corpus_id << '\n';
    output << "run_id=" << document.run_id << '\n';
    output << "row_count=" << document.row_count << '\n';
    output << "blocked_count=" << document.blocked_count << '\n';
    output << "reimport_candidate_count=" << document.reimport_candidate_count << '\n';
    output << "preset_diff_required_count=" << document.preset_diff_required_count << '\n';
    output << "axis_unit_preview_required_count=" << document.axis_unit_preview_required_count << '\n';
    output << "ready_for_operator_review=" << bool_text(document.ready_for_operator_review) << '\n';
    output << "row.count=" << document.rows.size() << '\n';
    for (std::size_t index = 0U; index < document.rows.size(); ++index) {
        const auto& row = document.rows[index];
        const auto prefix = "row." + std::to_string(index);
        output << prefix << ".asset_id=" << row.asset_id << '\n';
        output << prefix << ".kind=" << asset_import_regression_asset_kind_label(row.kind) << '\n';
        output << prefix << ".asset=" << row.asset.value << '\n';
        output << prefix << ".source_path=" << row.source_path << '\n';
        output << prefix << ".source_sha256=" << row.source_sha256 << '\n';
        output << prefix << ".preset_sha256=" << row.preset_sha256 << '\n';
        output << prefix << ".importer_id=" << row.importer_id << '\n';
        output << prefix << ".importer_version=" << row.importer_version << '\n';
        output << prefix << ".phase=" << row.phase << '\n';
        output << prefix << ".code=" << asset_import_regression_diagnostic_code_label(row.code) << '\n';
        output << prefix << ".severity=" << asset_import_regression_triage_severity_label(row.severity) << '\n';
        output << prefix
               << ".recommended_action=" << asset_import_regression_recommended_action_label(row.recommended_action)
               << '\n';
        output << prefix
               << ".reimport_decision=" << asset_import_regression_reimport_decision_label(row.reimport_decision)
               << '\n';
        output << prefix << ".repro_command_id=" << row.repro_command_id << '\n';
        output << prefix << ".repro_command=" << row.repro_command << '\n';
        output << prefix << ".source_excerpt_hash=" << row.source_excerpt_hash << '\n';
        output << prefix << ".preset_diff_required=" << bool_text(row.preset_diff_required) << '\n';
        output << prefix << ".axis_unit_preview_required=" << bool_text(row.axis_unit_preview_required) << '\n';
        output << prefix << ".legal_blocked=" << bool_text(row.legal_blocked) << '\n';
        output << prefix << ".nondeterministic=" << bool_text(row.nondeterministic) << '\n';
    }
    return output.str();
}

AssetImportRegressionTriageDocumentV1 deserialize_asset_import_regression_triage_v1(std::string_view text) {
    bool saw_format = false;
    bool saw_row_count = false;
    std::size_t serialized_row_count = 0U;
    std::unordered_set<std::string> seen_keys;
    std::unordered_map<std::size_t, TriageTextRow> rows;
    AssetImportRegressionTriageDocumentV1 document;

    std::size_t line_start = 0U;
    while (line_start < text.size()) {
        const auto line_end = text.find('\n', line_start);
        const auto line = text.substr(line_start, line_end == std::string_view::npos ? text.size() - line_start
                                                                                     : line_end - line_start);
        line_start = line_end == std::string_view::npos ? text.size() : line_end + 1U;
        if (line.empty()) {
            continue;
        }
        if (line.find('\r') != std::string_view::npos) {
            throw std::invalid_argument("asset import regression triage text contains carriage return");
        }
        const auto separator = line.find('=');
        if (separator == std::string_view::npos) {
            throw std::invalid_argument("asset import regression triage line is missing '='");
        }
        const auto key = line.substr(0U, separator);
        const auto value = line.substr(separator + 1U);
        if (!seen_keys.insert(std::string{key}).second) {
            throw std::invalid_argument("asset import regression triage text contains duplicate keys");
        }
        if (key == "format") {
            saw_format = true;
            if (value != triage_format_v1) {
                throw std::invalid_argument("asset import regression triage format is unsupported");
            }
        } else if (key == "corpus_id") {
            document.corpus_id = std::string{value};
        } else if (key == "run_id") {
            document.run_id = std::string{value};
        } else if (key == "row_count") {
            document.row_count = parse_size(value, "asset import regression triage row count is invalid");
        } else if (key == "blocked_count") {
            document.blocked_count = parse_size(value, "asset import regression triage blocked count is invalid");
        } else if (key == "reimport_candidate_count") {
            document.reimport_candidate_count =
                parse_size(value, "asset import regression triage reimport candidate count is invalid");
        } else if (key == "preset_diff_required_count") {
            document.preset_diff_required_count =
                parse_size(value, "asset import regression triage preset diff count is invalid");
        } else if (key == "axis_unit_preview_required_count") {
            document.axis_unit_preview_required_count =
                parse_size(value, "asset import regression triage axis unit preview count is invalid");
        } else if (key == "ready_for_operator_review") {
            document.ready_for_operator_review = parse_bool(value);
        } else if (key == "row.count") {
            saw_row_count = true;
            serialized_row_count = parse_size(value, "asset import regression triage serialized row count is invalid");
        } else if (key.starts_with("row.")) {
            const auto after_prefix = key.substr(std::string_view{"row."}.size());
            const auto dot = after_prefix.find('.');
            if (dot == std::string_view::npos) {
                throw std::invalid_argument("asset import regression triage row key is malformed");
            }
            const auto index =
                parse_size(after_prefix.substr(0U, dot), "asset import regression triage row index is invalid");
            parse_triage_row_field(rows[index], after_prefix.substr(dot + 1U), value);
        } else {
            throw std::invalid_argument("asset import regression triage key is unsupported");
        }
    }

    if (!saw_format || !saw_row_count || rows.size() != serialized_row_count ||
        document.row_count != serialized_row_count) {
        throw std::invalid_argument("asset import regression triage document is incomplete");
    }
    document.rows.reserve(serialized_row_count);
    for (std::size_t index = 0U; index < serialized_row_count; ++index) {
        const auto row = rows.find(index);
        if (row == rows.end() || !text_row_complete(row->second)) {
            throw std::invalid_argument("asset import regression triage row is incomplete");
        }
        document.rows.push_back(row->second.row);
    }
    return document;
}

} // namespace mirakana
