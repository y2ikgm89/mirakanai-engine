// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/asset_import_provenance.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mirakana {
namespace {

constexpr std::string_view asset_import_provenance_format_v1 = "GameEngine.AssetImportProvenance.v1";

struct AssetImportProvenanceTextRow {
    bool has_asset_key{false};
    bool has_origin{false};
    bool has_source_url{false};
    bool has_retrieved_date{false};
    bool has_version_or_commit{false};
    bool has_copyright_holder{false};
    bool has_license_id{false};
    bool has_modification_status{false};
    bool has_distribution_target{false};
    bool has_notice_id{false};
    bool has_notice_complete{false};
    bool has_external_engine_material{false};
    AssetImportProvenanceRowV1 row;
};

[[nodiscard]] char ascii_lower(char value) noexcept {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(value)));
}

[[nodiscard]] std::string lower_ascii(std::string_view value) {
    std::string lowered;
    lowered.reserve(value.size());
    for (const char character : value) {
        lowered.push_back(ascii_lower(character));
    }
    return lowered;
}

[[nodiscard]] bool contains_case_insensitive(std::string_view text, std::string_view needle) {
    if (needle.empty()) {
        return true;
    }
    if (needle.size() > text.size()) {
        return false;
    }
    return !std::ranges::search(text, needle, [](char lhs, char rhs) {
                return ascii_lower(lhs) == ascii_lower(rhs);
            }).empty();
}

[[nodiscard]] bool is_ascii_control(char character) noexcept {
    const auto value = static_cast<unsigned char>(character);
    return value < 0x20U || value == 0x7FU;
}

[[nodiscard]] bool contains_ascii_control(std::string_view value) noexcept {
    return std::ranges::any_of(value, is_ascii_control);
}

[[nodiscard]] bool contains_ascii_whitespace(std::string_view value) noexcept {
    return std::ranges::any_of(value,
                               [](char character) { return std::isspace(static_cast<unsigned char>(character)) != 0; });
}

[[nodiscard]] bool contains_parent_segment(std::string_view value) noexcept {
    std::size_t segment_begin = 0;
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

[[nodiscard]] bool valid_asset_key(std::string_view key) noexcept {
    return !key.empty() && !contains_ascii_control(key) && !contains_ascii_whitespace(key) && key.front() != '/' &&
           key.find('\\') == std::string_view::npos && key.find(':') == std::string_view::npos &&
           key.find(';') == std::string_view::npos && !contains_parent_segment(key);
}

[[nodiscard]] bool clean_text(std::string_view value) noexcept {
    return value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos &&
           value.find('\0') == std::string_view::npos;
}

[[nodiscard]] bool required_clean_text(std::string_view value) noexcept {
    return !value.empty() && clean_text(value);
}

[[nodiscard]] bool valid_iso_date(std::string_view value) noexcept {
    if (value.size() != 10U || value[4] != '-' || value[7] != '-') {
        return false;
    }
    for (std::size_t index = 0; index < value.size(); ++index) {
        if (index == 4U || index == 7U) {
            continue;
        }
        if (value[index] < '0' || value[index] > '9') {
            return false;
        }
    }
    const auto month = (static_cast<int>(value[5] - '0') * 10) + static_cast<int>(value[6] - '0');
    const auto day = (static_cast<int>(value[8] - '0') * 10) + static_cast<int>(value[9] - '0');
    return month >= 1 && month <= 12 && day >= 1 && day <= 31;
}

[[nodiscard]] bool is_license_ref(std::string_view license_id) noexcept {
    return license_id.starts_with("LicenseRef-");
}

[[nodiscard]] bool valid_origin(AssetImportProvenanceOrigin origin) noexcept {
    switch (origin) {
    case AssetImportProvenanceOrigin::first_party:
    case AssetImportProvenanceOrigin::third_party:
    case AssetImportProvenanceOrigin::generated_ai:
        return true;
    }
    return false;
}

[[nodiscard]] bool notice_reference_ready(std::string_view notice_id) noexcept {
    return notice_id.starts_with("LICENSES/") || notice_id.starts_with("THIRD_PARTY_NOTICES.md") ||
           notice_id.find("/LICENSES/") != std::string_view::npos ||
           notice_id.find("THIRD_PARTY_NOTICES.md#") != std::string_view::npos;
}

[[nodiscard]] bool known_allowed_spdx_id(std::string_view token) noexcept {
    constexpr std::string_view allowed[] = {
        "MIT", "MIT-0",   "Apache-2.0", "BSD-2-Clause", "BSD-3-Clause", "Zlib",
        "ISC", "BSL-1.0", "CC0-1.0",    "CC-BY-4.0",    "Unlicense",
    };
    return std::ranges::find(allowed, token) != std::end(allowed);
}

[[nodiscard]] std::vector<std::string_view> license_expression_tokens(std::string_view expression) {
    std::vector<std::string_view> tokens;
    std::size_t cursor = 0;
    while (cursor < expression.size()) {
        while (cursor < expression.size() && std::isspace(static_cast<unsigned char>(expression[cursor])) != 0) {
            ++cursor;
        }
        if (cursor >= expression.size()) {
            break;
        }
        if (expression[cursor] == '(' || expression[cursor] == ')') {
            tokens.push_back(expression.substr(cursor, 1U));
            ++cursor;
            continue;
        }
        const std::size_t begin = cursor;
        while (cursor < expression.size() && expression[cursor] != '(' && expression[cursor] != ')' &&
               std::isspace(static_cast<unsigned char>(expression[cursor])) == 0) {
            ++cursor;
        }
        tokens.push_back(expression.substr(begin, cursor - begin));
    }
    return tokens;
}

[[nodiscard]] bool supported_spdx_license_expression(std::string_view license_id) {
    if (license_id.empty() || is_license_ref(license_id)) {
        return false;
    }

    const auto tokens = license_expression_tokens(license_id);
    if (tokens.empty()) {
        return false;
    }

    int parenthesis_depth = 0;
    bool saw_license = false;
    bool previous_was_license_or_close = false;
    for (const auto token : tokens) {
        if (token == "(") {
            ++parenthesis_depth;
            previous_was_license_or_close = false;
            continue;
        }
        if (token == ")") {
            if (parenthesis_depth == 0 || !previous_was_license_or_close) {
                return false;
            }
            --parenthesis_depth;
            previous_was_license_or_close = true;
            continue;
        }
        if (token == "AND" || token == "OR") {
            if (!previous_was_license_or_close) {
                return false;
            }
            previous_was_license_or_close = false;
            continue;
        }
        if (!known_allowed_spdx_id(token) || previous_was_license_or_close) {
            return false;
        }
        saw_license = true;
        previous_was_license_or_close = true;
    }
    return saw_license && previous_was_license_or_close && parenthesis_depth == 0;
}

[[nodiscard]] bool missing_or_unknown_license(std::string_view license_id) {
    const std::string license = lower_ascii(license_id);
    return license_id.empty() || license == "none" || license == "noassertion" || license == "unknown" ||
           license == "unlicensed" || license == "license-less" || license == "licenseless" ||
           contains_case_insensitive(license_id, "no license");
}

[[nodiscard]] bool restricted_license(std::string_view license_id) {
    const std::string license = lower_ascii(license_id);
    return license.find("cc-by-nc") != std::string::npos || license.find("cc-nc") != std::string::npos ||
           license.find("-nc") != std::string::npos || license.find("cc-by-nd") != std::string::npos ||
           license.find("cc-nd") != std::string::npos || license.find("-nd") != std::string::npos;
}

[[nodiscard]] bool marketplace_only_license(std::string_view license_id) {
    const std::string license = lower_ascii(license_id);
    return license.find("marketplace") != std::string::npos || license.find("asset store") != std::string::npos ||
           license.find("fab standard") != std::string::npos || license.find("unity asset") != std::string::npos ||
           license.find("unreal marketplace") != std::string::npos || license.find("epic content") != std::string::npos;
}

[[nodiscard]] bool external_engine_material_reference(const AssetImportProvenanceRowV1& row) {
    if (row.external_engine_material) {
        return true;
    }
    const std::string review_text =
        lower_ascii(row.asset_key.value + " " + row.source_url + " " + row.modification_status);
    return review_text.find("assetstore.unity.com") != std::string::npos ||
           review_text.find("unity.com/packages") != std::string::npos ||
           review_text.find(".unitypackage") != std::string::npos ||
           review_text.find("unity asset store") != std::string::npos ||
           review_text.find("unrealengine.com/marketplace") != std::string::npos ||
           review_text.find("marketplace.unrealengine.com") != std::string::npos ||
           review_text.find("fab.com") != std::string::npos ||
           review_text.find("epicgames.com/fab") != std::string::npos ||
           review_text.find("unreal marketplace") != std::string::npos ||
           review_text.find(".uproject") != std::string::npos || review_text.find(".uasset") != std::string::npos ||
           review_text.find(".umap") != std::string::npos || review_text.find("project.godot") != std::string::npos ||
           review_text.find("godot scene") != std::string::npos ||
           review_text.find("godot editor ui") != std::string::npos ||
           review_text.find("unity scene") != std::string::npos ||
           review_text.find("unity meta file") != std::string::npos ||
           review_text.find("engine_sample_content") != std::string::npos ||
           review_text.find("engine_logo_or_trademark") != std::string::npos ||
           review_text.find("copied_editor_ui_expression") != std::string::npos ||
           review_text.find("copied_editor_screenshot") != std::string::npos ||
           review_text.find("copied_editor_icon") != std::string::npos ||
           review_text.find("copied_editor_layout") != std::string::npos ||
           review_text.find("external_engine_project_schema") != std::string::npos;
}

void push_diagnostic(std::vector<std::string>& diagnostics, std::size_t row_index, std::string_view code) {
    diagnostics.push_back("row." + std::to_string(row_index) + "." + std::string(code));
}

[[nodiscard]] std::uint64_t parse_decimal_u64(std::string_view value, std::string_view error_message) {
    if (value.empty()) {
        throw std::invalid_argument(std::string(error_message));
    }

    std::uint64_t parsed = 0;
    for (const char character : value) {
        if (character < '0' || character > '9') {
            throw std::invalid_argument(std::string(error_message));
        }
        const auto digit = static_cast<std::uint64_t>(character - '0');
        if (parsed > (std::numeric_limits<std::uint64_t>::max() - digit) / 10U) {
            throw std::invalid_argument(std::string(error_message));
        }
        parsed = (parsed * 10U) + digit;
    }
    return parsed;
}

[[nodiscard]] std::size_t parse_ordinal(std::string_view value) {
    const auto parsed = parse_decimal_u64(value, "asset import provenance row ordinal is invalid");
    if (parsed >= static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        throw std::invalid_argument("asset import provenance row ordinal is invalid");
    }
    return static_cast<std::size_t>(parsed);
}

[[nodiscard]] bool parse_bool(std::string_view value) {
    if (value == "true") {
        return true;
    }
    if (value == "false") {
        return false;
    }
    throw std::invalid_argument("asset import provenance bool value is invalid");
}

[[nodiscard]] const char* bool_text(bool value) noexcept {
    return value ? "true" : "false";
}

[[nodiscard]] AssetImportProvenanceOrigin parse_origin(std::string_view value) {
    if (value == "first_party") {
        return AssetImportProvenanceOrigin::first_party;
    }
    if (value == "third_party") {
        return AssetImportProvenanceOrigin::third_party;
    }
    if (value == "generated_ai") {
        return AssetImportProvenanceOrigin::generated_ai;
    }
    throw std::invalid_argument("asset import provenance origin is unsupported");
}

void parse_row_field(std::unordered_map<std::size_t, AssetImportProvenanceTextRow>& rows, std::string_view key,
                     std::string_view value) {
    constexpr std::string_view prefix = "row.";
    if (!key.starts_with(prefix)) {
        throw std::invalid_argument("asset import provenance text contains unsupported key");
    }

    const auto after_prefix = key.substr(prefix.size());
    const auto separator = after_prefix.find('.');
    if (separator == std::string_view::npos) {
        throw std::invalid_argument("asset import provenance row key is malformed");
    }
    const auto ordinal = parse_ordinal(after_prefix.substr(0, separator));
    const auto field = after_prefix.substr(separator + 1U);
    auto& text_row = rows[ordinal];

    if (field == "asset_key") {
        text_row.has_asset_key = true;
        text_row.row.asset_key.value = std::string(value);
    } else if (field == "origin") {
        text_row.has_origin = true;
        text_row.row.origin = parse_origin(value);
    } else if (field == "source_url") {
        text_row.has_source_url = true;
        text_row.row.source_url = std::string(value);
    } else if (field == "retrieved_date") {
        text_row.has_retrieved_date = true;
        text_row.row.retrieved_date = std::string(value);
    } else if (field == "version_or_commit") {
        text_row.has_version_or_commit = true;
        text_row.row.version_or_commit = std::string(value);
    } else if (field == "copyright_holder") {
        text_row.has_copyright_holder = true;
        text_row.row.copyright_holder = std::string(value);
    } else if (field == "license_id") {
        text_row.has_license_id = true;
        text_row.row.license_id = std::string(value);
    } else if (field == "modification_status") {
        text_row.has_modification_status = true;
        text_row.row.modification_status = std::string(value);
    } else if (field == "distribution_target") {
        text_row.has_distribution_target = true;
        text_row.row.distribution_target = std::string(value);
    } else if (field == "notice_id") {
        text_row.has_notice_id = true;
        text_row.row.notice_id = std::string(value);
    } else if (field == "notice_complete") {
        text_row.has_notice_complete = true;
        text_row.row.notice_complete = parse_bool(value);
    } else if (field == "external_engine_material") {
        text_row.has_external_engine_material = true;
        text_row.row.external_engine_material = parse_bool(value);
    } else {
        throw std::invalid_argument("asset import provenance row field is unsupported");
    }
}

[[nodiscard]] bool text_row_complete(const AssetImportProvenanceTextRow& row) noexcept {
    return row.has_asset_key && row.has_origin && row.has_source_url && row.has_retrieved_date &&
           row.has_version_or_commit && row.has_copyright_holder && row.has_license_id && row.has_modification_status &&
           row.has_distribution_target && row.has_notice_id && row.has_notice_complete &&
           row.has_external_engine_material;
}

} // namespace

std::string_view asset_import_provenance_origin_label(AssetImportProvenanceOrigin origin) noexcept {
    switch (origin) {
    case AssetImportProvenanceOrigin::first_party:
        return "first_party";
    case AssetImportProvenanceOrigin::third_party:
        return "third_party";
    case AssetImportProvenanceOrigin::generated_ai:
        return "generated_ai";
    }
    return "invalid";
}

std::vector<std::string> validate_asset_import_provenance_document(const AssetImportProvenanceDocumentV1& document) {
    std::vector<std::string> diagnostics;
    std::unordered_set<std::string> asset_keys;

    for (std::size_t row_index = 0; row_index < document.rows.size(); ++row_index) {
        const auto& row = document.rows[row_index];
        if (!valid_asset_key(row.asset_key.value)) {
            push_diagnostic(diagnostics, row_index, "invalid_asset_key");
        } else if (!asset_keys.insert(row.asset_key.value).second) {
            push_diagnostic(diagnostics, row_index, "duplicate_asset_key");
        }

        if (!valid_origin(row.origin)) {
            push_diagnostic(diagnostics, row_index, "invalid_origin");
        }

        if (external_engine_material_reference(row)) {
            push_diagnostic(diagnostics, row_index, "external_engine_material_rejected");
            continue;
        }

        if (missing_or_unknown_license(row.license_id)) {
            push_diagnostic(diagnostics, row_index, "missing_license");
            continue;
        }
        if (!clean_text(row.license_id)) {
            push_diagnostic(diagnostics, row_index, "invalid_license_id");
        } else if (restricted_license(row.license_id)) {
            push_diagnostic(diagnostics, row_index, "license_restricted");
        } else if (marketplace_only_license(row.license_id)) {
            push_diagnostic(diagnostics, row_index, "marketplace_license_rejected");
        } else if (is_license_ref(row.license_id)) {
            if (!row.notice_complete || !notice_reference_ready(row.notice_id)) {
                push_diagnostic(diagnostics, row_index, "license_ref_notice_missing");
            }
        } else if (!supported_spdx_license_expression(row.license_id)) {
            push_diagnostic(diagnostics, row_index, "unsupported_spdx_license_id");
        }

        if (valid_origin(row.origin) && row.origin == AssetImportProvenanceOrigin::first_party &&
            row.license_id != "LicenseRef-Proprietary") {
            push_diagnostic(diagnostics, row_index, "first_party_license_must_be_proprietary");
        }

        if (!required_clean_text(row.source_url)) {
            push_diagnostic(diagnostics, row_index, "missing_source_url");
        }
        if (!valid_iso_date(row.retrieved_date)) {
            push_diagnostic(diagnostics, row_index, "invalid_retrieved_date");
        }
        if (row.version_or_commit.empty()) {
            push_diagnostic(diagnostics, row_index, "missing_version_or_commit");
        } else if (!clean_text(row.version_or_commit)) {
            push_diagnostic(diagnostics, row_index, "invalid_version_or_commit");
        }
        if (!required_clean_text(row.copyright_holder)) {
            push_diagnostic(diagnostics, row_index, "missing_copyright_holder");
        }
        if (!required_clean_text(row.modification_status)) {
            push_diagnostic(diagnostics, row_index, "missing_modification_status");
        }
        if (!required_clean_text(row.distribution_target)) {
            push_diagnostic(diagnostics, row_index, "missing_distribution_target");
        }
        if (!required_clean_text(row.notice_id)) {
            push_diagnostic(diagnostics, row_index, "missing_notice_id");
        }
        if (!row.notice_complete) {
            push_diagnostic(diagnostics, row_index, "missing_notice");
        }
    }

    return diagnostics;
}

std::string serialize_asset_import_provenance_document(const AssetImportProvenanceDocumentV1& document) {
    const auto diagnostics = validate_asset_import_provenance_document(document);
    if (!diagnostics.empty()) {
        throw std::invalid_argument("asset import provenance document is invalid");
    }

    std::ostringstream output;
    output << "format=" << asset_import_provenance_format_v1 << '\n';
    output << "row.count=" << document.rows.size() << '\n';
    for (std::size_t row_index = 0; row_index < document.rows.size(); ++row_index) {
        const auto& row = document.rows[row_index];
        output << "row." << row_index << ".asset_key=" << row.asset_key.value << '\n';
        output << "row." << row_index << ".origin=" << asset_import_provenance_origin_label(row.origin) << '\n';
        output << "row." << row_index << ".source_url=" << row.source_url << '\n';
        output << "row." << row_index << ".retrieved_date=" << row.retrieved_date << '\n';
        output << "row." << row_index << ".version_or_commit=" << row.version_or_commit << '\n';
        output << "row." << row_index << ".copyright_holder=" << row.copyright_holder << '\n';
        output << "row." << row_index << ".license_id=" << row.license_id << '\n';
        output << "row." << row_index << ".modification_status=" << row.modification_status << '\n';
        output << "row." << row_index << ".distribution_target=" << row.distribution_target << '\n';
        output << "row." << row_index << ".notice_id=" << row.notice_id << '\n';
        output << "row." << row_index << ".notice_complete=" << bool_text(row.notice_complete) << '\n';
        output << "row." << row_index << ".external_engine_material=" << bool_text(row.external_engine_material)
               << '\n';
    }
    return output.str();
}

AssetImportProvenanceDocumentV1 deserialize_asset_import_provenance_document(std::string_view text) {
    bool saw_format = false;
    bool saw_row_count = false;
    std::size_t row_count = 0;
    std::unordered_set<std::string> seen_keys;
    std::unordered_map<std::size_t, AssetImportProvenanceTextRow> rows;

    std::size_t line_start = 0;
    while (line_start < text.size()) {
        const auto line_end = text.find('\n', line_start);
        const auto line = text.substr(line_start, line_end == std::string_view::npos ? text.size() - line_start
                                                                                     : line_end - line_start);
        line_start = line_end == std::string_view::npos ? text.size() : line_end + 1U;
        if (line.empty()) {
            continue;
        }
        if (line.find('\r') != std::string_view::npos) {
            throw std::invalid_argument("asset import provenance text contains carriage return");
        }
        const auto separator = line.find('=');
        if (separator == std::string_view::npos) {
            throw std::invalid_argument("asset import provenance line is missing '='");
        }
        const auto key = line.substr(0, separator);
        const auto value = line.substr(separator + 1U);
        if (!seen_keys.insert(std::string(key)).second) {
            throw std::invalid_argument("asset import provenance text contains duplicate keys");
        }
        if (key == "format") {
            saw_format = true;
            if (value != asset_import_provenance_format_v1) {
                throw std::invalid_argument("asset import provenance format is unsupported");
            }
            continue;
        }
        if (key == "row.count") {
            saw_row_count = true;
            row_count = parse_ordinal(value);
            continue;
        }
        parse_row_field(rows, key, value);
    }

    if (!saw_format) {
        throw std::invalid_argument("asset import provenance format is missing");
    }
    if (!saw_row_count) {
        throw std::invalid_argument("asset import provenance row count is missing");
    }
    if (rows.size() != row_count) {
        throw std::invalid_argument("asset import provenance row count does not match rows");
    }

    AssetImportProvenanceDocumentV1 document;
    document.rows.reserve(row_count);
    for (std::size_t row_index = 0; row_index < row_count; ++row_index) {
        const auto row = rows.find(row_index);
        if (row == rows.end() || !text_row_complete(row->second)) {
            throw std::invalid_argument("asset import provenance row is incomplete");
        }
        document.rows.push_back(row->second.row);
    }

    const auto diagnostics = validate_asset_import_provenance_document(document);
    if (!diagnostics.empty()) {
        throw std::invalid_argument("asset import provenance document is invalid");
    }
    return document;
}

} // namespace mirakana
