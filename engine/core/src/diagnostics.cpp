// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/core/diagnostics.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <iterator>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace mirakana {
namespace {

template <typename T> void push_bounded(std::vector<T>& records, std::size_t capacity, T record) {
    if (records.size() == capacity) {
        std::rotate(records.begin(), records.begin() + 1, records.end());
        records.back() = std::move(record);
        return;
    }
    records.push_back(std::move(record));
}

[[nodiscard]] bool try_record_profile_sample(DiagnosticsRecorder& recorder, ProfileSample sample) noexcept {
    try {
        recorder.record_profile_sample(std::move(sample));
        return true;
    } catch (...) {
        return false;
    }
}

void append_json_string(std::string& output, std::string_view value) {
    output.push_back('"');
    for (const auto character : value) {
        const auto byte = static_cast<unsigned char>(character);
        switch (character) {
        case '"':
            output += "\\\"";
            break;
        case '\\':
            output += "\\\\";
            break;
        case '\b':
            output += "\\b";
            break;
        case '\f':
            output += "\\f";
            break;
        case '\n':
            output += "\\n";
            break;
        case '\r':
            output += "\\r";
            break;
        case '\t':
            output += "\\t";
            break;
        default:
            if (byte < 0x20U) {
                constexpr char hex_digits[] = "0123456789abcdef";
                output += "\\u00";
                output.push_back(hex_digits[(byte >> 4U) & 0x0FU]);
                output.push_back(hex_digits[byte & 0x0FU]);
            } else {
                output.push_back(character);
            }
            break;
        }
    }
    output.push_back('"');
}

void append_json_key(std::string& output, std::string_view key) {
    append_json_string(output, key);
    output.push_back(':');
}

void append_uint(std::string& output, std::uint64_t value) {
    output += std::to_string(value);
}

void append_double(std::string& output, double value) {
    std::array<char, 64> buffer{};
    const auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    if (result.ec == std::errc{}) {
        output.append(buffer.data(), result.ptr);
        return;
    }
    output.push_back('0');
}

void append_common_trace_fields(std::string& output, std::string_view name, std::string_view category,
                                std::string_view phase, std::uint64_t timestamp_us,
                                const DiagnosticsTraceExportOptions& options) {
    append_json_key(output, "name");
    append_json_string(output, name);
    output.push_back(',');
    append_json_key(output, "cat");
    append_json_string(output, category);
    output.push_back(',');
    append_json_key(output, "ph");
    append_json_string(output, phase);
    output.push_back(',');
    append_json_key(output, "ts");
    append_uint(output, timestamp_us);
    output.push_back(',');
    append_json_key(output, "pid");
    append_uint(output, options.process_id);
    output.push_back(',');
    append_json_key(output, "tid");
    append_uint(output, options.thread_id);
}

void append_metadata_event(std::string& output, std::string_view name, std::string_view value,
                           const DiagnosticsTraceExportOptions& options) {
    output.push_back('{');
    append_json_key(output, "name");
    append_json_string(output, name);
    output.push_back(',');
    append_json_key(output, "ph");
    append_json_string(output, "M");
    output.push_back(',');
    append_json_key(output, "pid");
    append_uint(output, options.process_id);
    output.push_back(',');
    append_json_key(output, "tid");
    append_uint(output, options.thread_id);
    output.push_back(',');
    append_json_key(output, "args");
    output.push_back('{');
    append_json_key(output, "name");
    append_json_string(output, value);
    output += "}}";
}

[[nodiscard]] bool is_json_whitespace(char value) noexcept {
    return value == ' ' || value == '\n' || value == '\r' || value == '\t';
}

[[nodiscard]] bool is_json_digit(char value) noexcept {
    return value >= '0' && value <= '9';
}

[[nodiscard]] bool is_json_hex_digit(char value) noexcept {
    return (value >= '0' && value <= '9') || (value >= 'a' && value <= 'f') || (value >= 'A' && value <= 'F');
}

struct TraceEventFields {
    bool has_phase{false};
    std::string phase;
    bool has_name{false};
    std::string name;
    bool has_category{false};
    std::string category;
    bool has_timestamp{false};
    std::string timestamp;
    bool has_duration{false};
    std::string duration;
    bool has_severity{false};
    std::string severity;
    bool has_frame_index{false};
    std::string frame_index;
    bool has_depth{false};
    std::string depth;
    bool has_value{false};
    std::string value;
};

[[nodiscard]] bool parse_unsigned_token(std::string_view token, std::uint64_t& value) noexcept {
    if (token.empty() || token.front() == '-' || token.find_first_of(".eE") != std::string_view::npos) {
        return false;
    }
    const auto* begin = token.data();
    const auto* end = begin + token.size();
    const auto result = std::from_chars(begin, end, value);
    return result.ec == std::errc{} && result.ptr == end;
}

[[nodiscard]] bool parse_double_token(std::string_view token, double& value) {
    std::istringstream stream{std::string(token)};
    stream.imbue(std::locale::classic());
    stream >> value;
    return !stream.fail() && stream.eof() && std::isfinite(value);
}

[[nodiscard]] bool severity_from_label(std::string_view label, DiagnosticSeverity& severity) noexcept {
    if (label == "trace") {
        severity = DiagnosticSeverity::trace;
        return true;
    }
    if (label == "debug") {
        severity = DiagnosticSeverity::debug;
        return true;
    }
    if (label == "info") {
        severity = DiagnosticSeverity::info;
        return true;
    }
    if (label == "warning") {
        severity = DiagnosticSeverity::warning;
        return true;
    }
    if (label == "error") {
        severity = DiagnosticSeverity::error;
        return true;
    }
    if (label == "fatal") {
        severity = DiagnosticSeverity::fatal;
        return true;
    }
    return false;
}

[[nodiscard]] bool micros_to_nanos(std::uint64_t micros, std::uint64_t& nanos) noexcept {
    constexpr auto max = std::numeric_limits<std::uint64_t>::max();
    if (micros > max / 1000U) {
        return false;
    }
    nanos = micros * 1000U;
    return true;
}

class TraceJsonReviewParser {
  public:
    explicit TraceJsonReviewParser(std::string_view text, DiagnosticCapture* capture = nullptr,
                                   std::vector<std::string>* reconstruction_diagnostics = nullptr)
        : text_(text), capture_(capture), reconstruction_diagnostics_(reconstruction_diagnostics) {}

    [[nodiscard]] DiagnosticsTraceImportReview parse() {
        DiagnosticsTraceImportReview review;
        review.payload_bytes = text_.size();

        if (text_.empty()) {
            review.diagnostics.emplace_back("trace import review requires non-empty JSON");
            return review;
        }

        if (!parse_root_object(review)) {
            return review;
        }

        skip_whitespace();
        if (cursor_ != text_.size()) {
            (void)fail_malformed(review, "unexpected trailing content");
            return review;
        }

        if (!found_trace_events_ && review.diagnostics.empty()) {
            review.diagnostics.emplace_back("trace import review requires a traceEvents array");
            return review;
        }

        review.valid = review.diagnostics.empty();
        return review;
    }

  private:
    static constexpr std::uint32_t k_max_json_depth = 64;

    void skip_whitespace() noexcept {
        while (cursor_ < text_.size() && is_json_whitespace(text_[cursor_])) {
            ++cursor_;
        }
    }

    [[nodiscard]] bool consume(char expected) noexcept {
        skip_whitespace();
        if (cursor_ < text_.size() && text_[cursor_] == expected) {
            ++cursor_;
            return true;
        }
        return false;
    }

    [[nodiscard]] static bool fail_malformed(DiagnosticsTraceImportReview& review, std::string_view reason) {
        if (review.diagnostics.empty()) {
            review.diagnostics.push_back("malformed trace JSON: " + std::string(reason));
        }
        return false;
    }

    [[nodiscard]] bool parse_root_object(DiagnosticsTraceImportReview& review) {
        if (!consume('{')) {
            return fail_malformed(review, "top-level value must be an object");
        }

        skip_whitespace();
        if (consume('}')) {
            review.diagnostics.emplace_back("trace import review requires a traceEvents array");
            return false;
        }

        while (cursor_ < text_.size()) {
            std::string key;
            if (!parse_string(key, review)) {
                return false;
            }
            if (!consume(':')) {
                return fail_malformed(review, "expected ':' after object key");
            }

            if (key == "traceEvents") {
                found_trace_events_ = true;
                if (!parse_trace_events_array(review)) {
                    return false;
                }
            } else if (!skip_value(review, 1)) {
                return false;
            }

            skip_whitespace();
            if (consume('}')) {
                return true;
            }
            if (!consume(',')) {
                return fail_malformed(review, "expected ',' or '}' after object member");
            }
        }

        return fail_malformed(review, "unterminated top-level object");
    }

    [[nodiscard]] bool parse_trace_events_array(DiagnosticsTraceImportReview& review) {
        if (!consume('[')) {
            review.diagnostics.emplace_back("trace import review requires a traceEvents array");
            return false;
        }

        skip_whitespace();
        if (consume(']')) {
            return true;
        }

        while (cursor_ < text_.size()) {
            skip_whitespace();
            if (cursor_ >= text_.size() || text_[cursor_] != '{') {
                review.diagnostics.emplace_back("traceEvents entries must be objects");
                return false;
            }
            if (!parse_trace_event_object(review)) {
                return false;
            }

            skip_whitespace();
            if (consume(']')) {
                return true;
            }
            if (!consume(',')) {
                return fail_malformed(review, "expected ',' or ']' after trace event");
            }
        }

        return fail_malformed(review, "unterminated traceEvents array");
    }

    [[nodiscard]] bool parse_trace_event_object(DiagnosticsTraceImportReview& review) {
        if (!consume('{')) {
            return fail_malformed(review, "expected trace event object");
        }

        TraceEventFields event;

        skip_whitespace();
        if (!consume('}')) {
            while (cursor_ < text_.size()) {
                std::string key;
                if (!parse_string(key, review)) {
                    return false;
                }
                if (!consume(':')) {
                    return fail_malformed(review, "expected ':' after trace event key");
                }

                if (key == "ph") {
                    skip_whitespace();
                    if (cursor_ < text_.size() && text_[cursor_] == '"') {
                        if (!parse_string(event.phase, review)) {
                            return false;
                        }
                        event.has_phase = true;
                    } else if (!skip_value(review, 1)) {
                        return false;
                    }
                } else if (key == "name") {
                    skip_whitespace();
                    if (cursor_ < text_.size() && text_[cursor_] == '"') {
                        if (!parse_string(event.name, review)) {
                            return false;
                        }
                        event.has_name = true;
                    } else if (!skip_value(review, 1)) {
                        return false;
                    }
                } else if (key == "cat") {
                    skip_whitespace();
                    if (cursor_ < text_.size() && text_[cursor_] == '"') {
                        if (!parse_string(event.category, review)) {
                            return false;
                        }
                        event.has_category = true;
                    } else if (!skip_value(review, 1)) {
                        return false;
                    }
                } else if (key == "ts") {
                    skip_whitespace();
                    if (cursor_ < text_.size() && (text_[cursor_] == '-' || is_json_digit(text_[cursor_]))) {
                        if (!parse_number_token(event.timestamp, review)) {
                            return false;
                        }
                        event.has_timestamp = true;
                    } else if (!skip_value(review, 1)) {
                        return false;
                    }
                } else if (key == "dur") {
                    skip_whitespace();
                    if (cursor_ < text_.size() && (text_[cursor_] == '-' || is_json_digit(text_[cursor_]))) {
                        if (!parse_number_token(event.duration, review)) {
                            return false;
                        }
                        event.has_duration = true;
                    } else if (!skip_value(review, 1)) {
                        return false;
                    }
                } else if (key == "args") {
                    skip_whitespace();
                    if (cursor_ < text_.size() && text_[cursor_] == '{') {
                        if (!parse_trace_event_args(event, review)) {
                            return false;
                        }
                    } else if (!skip_value(review, 1)) {
                        return false;
                    }
                } else if (!skip_value(review, 1)) {
                    return false;
                }

                skip_whitespace();
                if (consume('}')) {
                    break;
                }
                if (!consume(',')) {
                    return fail_malformed(review, "expected ',' or '}' after trace event member");
                }
            }
        }

        ++review.trace_event_count;
        if (!event.has_phase) {
            ++review.unknown_event_count;
        } else if (event.phase == "M") {
            ++review.metadata_event_count;
        } else if (event.phase == "i") {
            ++review.instant_event_count;
        } else if (event.phase == "C") {
            ++review.counter_event_count;
        } else if (event.phase == "X") {
            ++review.profile_event_count;
        } else {
            ++review.unknown_event_count;
        }
        reconstruct_trace_event(event);
        return true;
    }

    [[nodiscard]] bool parse_trace_event_args(TraceEventFields& event, DiagnosticsTraceImportReview& review) {
        if (!consume('{')) {
            return fail_malformed(review, "expected trace event args object");
        }

        skip_whitespace();
        if (consume('}')) {
            return true;
        }

        while (cursor_ < text_.size()) {
            std::string key;
            if (!parse_string(key, review)) {
                return false;
            }
            if (!consume(':')) {
                return fail_malformed(review, "expected ':' after args key");
            }

            if (key == "severity") {
                skip_whitespace();
                if (cursor_ < text_.size() && text_[cursor_] == '"') {
                    if (!parse_string(event.severity, review)) {
                        return false;
                    }
                    event.has_severity = true;
                } else if (!skip_value(review, 1)) {
                    return false;
                }
            } else if (key == "frame_index") {
                skip_whitespace();
                if (cursor_ < text_.size() && (text_[cursor_] == '-' || is_json_digit(text_[cursor_]))) {
                    if (!parse_number_token(event.frame_index, review)) {
                        return false;
                    }
                    event.has_frame_index = true;
                } else if (!skip_value(review, 1)) {
                    return false;
                }
            } else if (key == "depth") {
                skip_whitespace();
                if (cursor_ < text_.size() && (text_[cursor_] == '-' || is_json_digit(text_[cursor_]))) {
                    if (!parse_number_token(event.depth, review)) {
                        return false;
                    }
                    event.has_depth = true;
                } else if (!skip_value(review, 1)) {
                    return false;
                }
            } else if (key == "value") {
                skip_whitespace();
                if (cursor_ < text_.size() && (text_[cursor_] == '-' || is_json_digit(text_[cursor_]))) {
                    if (!parse_number_token(event.value, review)) {
                        return false;
                    }
                    event.has_value = true;
                } else if (!skip_value(review, 1)) {
                    return false;
                }
            } else if (!skip_value(review, 1)) {
                return false;
            }

            skip_whitespace();
            if (consume('}')) {
                return true;
            }
            if (!consume(',')) {
                return fail_malformed(review, "expected ',' or '}' after args member");
            }
        }

        return fail_malformed(review, "unterminated args object");
    }

    [[nodiscard]] bool parse_number_token(std::string& output, DiagnosticsTraceImportReview& review) {
        const auto begin = cursor_;
        if (!skip_number(review)) {
            return false;
        }
        output.assign(text_.substr(begin, cursor_ - begin));
        return true;
    }

    void add_reconstruction_diagnostic(std::string message) {
        if (reconstruction_diagnostics_ != nullptr) {
            reconstruction_diagnostics_->push_back(std::move(message));
        }
    }

    [[nodiscard]] bool parse_required_uint(std::string_view token, std::uint64_t& value, std::string_view label) {
        if (parse_unsigned_token(token, value)) {
            return true;
        }
        add_reconstruction_diagnostic(std::string("trace import reconstruction requires unsigned integer ") +
                                      std::string(label));
        return false;
    }

    void reconstruct_trace_event(const TraceEventFields& event) {
        if (capture_ == nullptr) {
            return;
        }
        if (!event.has_phase) {
            add_reconstruction_diagnostic("trace event without phase cannot be reconstructed");
            return;
        }
        if (event.phase == "M") {
            return;
        }
        if (event.phase == "i") {
            reconstruct_instant_event(event);
            return;
        }
        if (event.phase == "C") {
            reconstruct_counter_event(event);
            return;
        }
        if (event.phase == "X") {
            reconstruct_profile_event(event);
            return;
        }

        add_reconstruction_diagnostic("trace event phase '" + event.phase + "' cannot be reconstructed");
    }

    void reconstruct_instant_event(const TraceEventFields& event) {
        if (!event.has_name || event.name.empty() || !event.has_category || event.category.empty() ||
            !event.has_severity || !event.has_frame_index) {
            add_reconstruction_diagnostic("trace instant event is missing exported diagnostic fields");
            return;
        }

        DiagnosticSeverity severity{};
        if (!severity_from_label(event.severity, severity)) {
            add_reconstruction_diagnostic("trace instant event severity cannot be reconstructed");
            return;
        }

        std::uint64_t frame_index = 0;
        if (!parse_required_uint(event.frame_index, frame_index, "instant frame_index")) {
            return;
        }

        capture_->events.push_back(DiagnosticEvent{
            .severity = severity,
            .category = event.category,
            .message = event.name,
            .frame_index = frame_index,
        });
    }

    void reconstruct_counter_event(const TraceEventFields& event) {
        if (!event.has_name || event.name.empty() || !event.has_value || !event.has_frame_index) {
            add_reconstruction_diagnostic("trace counter event is missing exported counter fields");
            return;
        }

        double value = 0.0;
        if (!parse_double_token(event.value, value)) {
            add_reconstruction_diagnostic("trace counter event value cannot be reconstructed");
            return;
        }

        std::uint64_t frame_index = 0;
        if (!parse_required_uint(event.frame_index, frame_index, "counter frame_index")) {
            return;
        }

        capture_->counters.push_back(CounterSample{
            .name = event.name,
            .value = value,
            .frame_index = frame_index,
        });
    }

    void reconstruct_profile_event(const TraceEventFields& event) {
        if (!event.has_name || event.name.empty() || !event.has_timestamp || !event.has_duration ||
            !event.has_frame_index || !event.has_depth) {
            add_reconstruction_diagnostic("trace profile event is missing exported profile fields");
            return;
        }

        std::uint64_t timestamp_us = 0;
        std::uint64_t duration_us = 0;
        std::uint64_t frame_index = 0;
        std::uint64_t depth_wide = 0;
        if (!parse_required_uint(event.timestamp, timestamp_us, "profile timestamp") ||
            !parse_required_uint(event.duration, duration_us, "profile duration") ||
            !parse_required_uint(event.frame_index, frame_index, "profile frame_index") ||
            !parse_required_uint(event.depth, depth_wide, "profile depth")) {
            return;
        }
        if (depth_wide > std::numeric_limits<std::uint32_t>::max()) {
            add_reconstruction_diagnostic("trace profile event depth cannot be reconstructed");
            return;
        }

        std::uint64_t start_time_ns = 0;
        std::uint64_t duration_ns = 0;
        if (!micros_to_nanos(timestamp_us, start_time_ns) || !micros_to_nanos(duration_us, duration_ns)) {
            add_reconstruction_diagnostic("trace profile event timestamp cannot be reconstructed");
            return;
        }

        capture_->profiles.push_back(ProfileSample{
            .name = event.name,
            .frame_index = frame_index,
            .start_time_ns = start_time_ns,
            .duration_ns = duration_ns,
            .depth = static_cast<std::uint32_t>(depth_wide),
        });
    }

    [[nodiscard]] bool parse_string(std::string& output, DiagnosticsTraceImportReview& review) {
        if (!consume('"')) {
            return fail_malformed(review, "expected string");
        }

        while (cursor_ < text_.size()) {
            const char character = text_[cursor_++];
            const auto byte = static_cast<unsigned char>(character);
            if (character == '"') {
                return true;
            }
            if (byte < 0x20U) {
                return fail_malformed(review, "unescaped control character in string");
            }
            if (character != '\\') {
                output.push_back(character);
                continue;
            }

            if (cursor_ >= text_.size()) {
                return fail_malformed(review, "unterminated string escape");
            }

            const char escaped = text_[cursor_++];
            switch (escaped) {
            case '"':
            case '\\':
            case '/':
                output.push_back(escaped);
                break;
            case 'b':
                output.push_back('\b');
                break;
            case 'f':
                output.push_back('\f');
                break;
            case 'n':
                output.push_back('\n');
                break;
            case 'r':
                output.push_back('\r');
                break;
            case 't':
                output.push_back('\t');
                break;
            case 'u':
                for (int index = 0; index < 4; ++index) {
                    if (cursor_ >= text_.size() || !is_json_hex_digit(text_[cursor_])) {
                        return fail_malformed(review, "invalid unicode escape");
                    }
                    ++cursor_;
                }
                output.push_back('?');
                break;
            default:
                return fail_malformed(review, "invalid string escape");
            }
        }

        return fail_malformed(review, "unterminated string");
    }

    [[nodiscard]] bool skip_value(DiagnosticsTraceImportReview& review, std::uint32_t depth) {
        if (depth > k_max_json_depth) {
            return fail_malformed(review, "maximum nesting depth exceeded");
        }

        skip_whitespace();
        if (cursor_ >= text_.size()) {
            return fail_malformed(review, "unexpected end of input");
        }

        const char value = text_[cursor_];
        if (value == '"') {
            std::string ignored;
            return parse_string(ignored, review);
        }
        if (value == '{') {
            return skip_object(review, depth + 1U);
        }
        if (value == '[') {
            return skip_array(review, depth + 1U);
        }
        if (value == '-' || is_json_digit(value)) {
            return skip_number(review);
        }
        if (consume_literal("true") || consume_literal("false") || consume_literal("null")) {
            return true;
        }
        return fail_malformed(review, "unexpected JSON value");
    }

    [[nodiscard]] bool skip_object(DiagnosticsTraceImportReview& review, std::uint32_t depth) {
        if (!consume('{')) {
            return fail_malformed(review, "expected object");
        }
        skip_whitespace();
        if (consume('}')) {
            return true;
        }
        while (cursor_ < text_.size()) {
            std::string ignored_key;
            if (!parse_string(ignored_key, review)) {
                return false;
            }
            if (!consume(':')) {
                return fail_malformed(review, "expected ':' after object key");
            }
            if (!skip_value(review, depth)) {
                return false;
            }
            skip_whitespace();
            if (consume('}')) {
                return true;
            }
            if (!consume(',')) {
                return fail_malformed(review, "expected ',' or '}' after object member");
            }
        }
        return fail_malformed(review, "unterminated object");
    }

    [[nodiscard]] bool skip_array(DiagnosticsTraceImportReview& review, std::uint32_t depth) {
        if (!consume('[')) {
            return fail_malformed(review, "expected array");
        }
        skip_whitespace();
        if (consume(']')) {
            return true;
        }
        while (cursor_ < text_.size()) {
            if (!skip_value(review, depth)) {
                return false;
            }
            skip_whitespace();
            if (consume(']')) {
                return true;
            }
            if (!consume(',')) {
                return fail_malformed(review, "expected ',' or ']' after array value");
            }
        }
        return fail_malformed(review, "unterminated array");
    }

    [[nodiscard]] bool skip_number(DiagnosticsTraceImportReview& review) {
        if (cursor_ < text_.size() && text_[cursor_] == '-') {
            ++cursor_;
        }
        if (cursor_ >= text_.size()) {
            return fail_malformed(review, "invalid number");
        }
        if (text_[cursor_] == '0') {
            ++cursor_;
        } else if (text_[cursor_] >= '1' && text_[cursor_] <= '9') {
            while (cursor_ < text_.size() && is_json_digit(text_[cursor_])) {
                ++cursor_;
            }
        } else {
            return fail_malformed(review, "invalid number");
        }

        if (cursor_ < text_.size() && text_[cursor_] == '.') {
            ++cursor_;
            if (cursor_ >= text_.size() || !is_json_digit(text_[cursor_])) {
                return fail_malformed(review, "invalid number fraction");
            }
            while (cursor_ < text_.size() && is_json_digit(text_[cursor_])) {
                ++cursor_;
            }
        }

        if (cursor_ < text_.size() && (text_[cursor_] == 'e' || text_[cursor_] == 'E')) {
            ++cursor_;
            if (cursor_ < text_.size() && (text_[cursor_] == '+' || text_[cursor_] == '-')) {
                ++cursor_;
            }
            if (cursor_ >= text_.size() || !is_json_digit(text_[cursor_])) {
                return fail_malformed(review, "invalid number exponent");
            }
            while (cursor_ < text_.size() && is_json_digit(text_[cursor_])) {
                ++cursor_;
            }
        }

        return true;
    }

    [[nodiscard]] bool consume_literal(std::string_view literal) noexcept {
        skip_whitespace();
        if (text_.substr(cursor_, literal.size()) == literal) {
            cursor_ += literal.size();
            return true;
        }
        return false;
    }

    std::string_view text_;
    std::size_t cursor_{0};
    bool found_trace_events_{false};
    DiagnosticCapture* capture_{nullptr};
    std::vector<std::string>* reconstruction_diagnostics_{nullptr};
};

} // namespace

ManualProfileClock::ManualProfileClock(std::uint64_t initial_time_ns) noexcept : time_ns_(initial_time_ns) {}

std::uint64_t ManualProfileClock::now_ns() const noexcept {
    return time_ns_;
}

void ManualProfileClock::set(std::uint64_t time_ns) noexcept {
    time_ns_ = time_ns;
}

void ManualProfileClock::advance(std::uint64_t delta_ns) noexcept {
    time_ns_ += delta_ns;
}

std::uint64_t SteadyProfileClock::now_ns() const noexcept {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
}

DiagnosticsRecorder::DiagnosticsRecorder(std::size_t capacity) : capacity_(capacity) {
    if (capacity_ == 0) {
        throw std::invalid_argument("DiagnosticsRecorder capacity must be greater than zero");
    }
    events_.reserve(capacity_);
    counters_.reserve(capacity_);
    profiles_.reserve(capacity_);
}

void DiagnosticsRecorder::record_event(DiagnosticEvent event) {
    if (!valid_label(event.category) || event.message.empty()) {
        record_internal_warning("rejected invalid diagnostic event");
        return;
    }
    push_bounded(events_, capacity_, std::move(event));
}

void DiagnosticsRecorder::record_counter(CounterSample sample) {
    if (!valid_label(sample.name) || !std::isfinite(sample.value)) {
        record_internal_warning("rejected invalid diagnostic counter");
        return;
    }
    push_bounded(counters_, capacity_, std::move(sample));
}

void DiagnosticsRecorder::record_profile_sample(ProfileSample sample) {
    if (!valid_label(sample.name)) {
        record_internal_warning("rejected invalid profile sample");
        return;
    }
    push_bounded(profiles_, capacity_, std::move(sample));
}

DiagnosticCapture DiagnosticsRecorder::snapshot() const {
    return DiagnosticCapture{
        .events = events_,
        .counters = counters_,
        .profiles = profiles_,
    };
}

std::size_t DiagnosticsRecorder::capacity() const noexcept {
    return capacity_;
}

std::uint32_t DiagnosticsRecorder::current_profile_depth() const noexcept {
    return profile_depth_;
}

std::uint32_t DiagnosticsRecorder::begin_profile_zone() noexcept {
    const auto depth = profile_depth_;
    if (profile_depth_ < std::numeric_limits<std::uint32_t>::max()) {
        ++profile_depth_;
    }
    return depth;
}

void DiagnosticsRecorder::end_profile_zone() noexcept {
    if (profile_depth_ > 0) {
        --profile_depth_;
    }
}

void DiagnosticsRecorder::clear() noexcept {
    events_.clear();
    counters_.clear();
    profiles_.clear();
    profile_depth_ = 0;
}

bool DiagnosticsRecorder::valid_label(std::string_view label) noexcept {
    return !label.empty();
}

void DiagnosticsRecorder::record_internal_warning(std::string message) {
    push_bounded(events_, capacity_,
                 DiagnosticEvent{
                     .severity = DiagnosticSeverity::warning,
                     .category = "diagnostics",
                     .message = std::move(message),
                     .frame_index = 0,
                 });
}

ScopedProfileZone::ScopedProfileZone(DiagnosticsRecorder& recorder, const IProfileClock& clock, std::string name,
                                     std::uint64_t frame_index)
    : recorder_(&recorder), clock_(&clock), name_(std::move(name)), frame_index_(frame_index),
      start_time_ns_(clock.now_ns()), depth_(recorder.begin_profile_zone()), active_(true) {}

ScopedProfileZone::ScopedProfileZone(ScopedProfileZone&& other) noexcept
    : recorder_(other.recorder_), clock_(other.clock_), name_(std::move(other.name_)), frame_index_(other.frame_index_),
      start_time_ns_(other.start_time_ns_), depth_(other.depth_), active_(other.active_) {
    other.recorder_ = nullptr;
    other.clock_ = nullptr;
    other.active_ = false;
}

ScopedProfileZone& ScopedProfileZone::operator=(ScopedProfileZone&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    close();

    recorder_ = other.recorder_;
    clock_ = other.clock_;
    name_ = std::move(other.name_);
    frame_index_ = other.frame_index_;
    start_time_ns_ = other.start_time_ns_;
    depth_ = other.depth_;
    active_ = other.active_;

    other.recorder_ = nullptr;
    other.clock_ = nullptr;
    other.active_ = false;

    return *this;
}

ScopedProfileZone::~ScopedProfileZone() noexcept {
    close();
}

void ScopedProfileZone::close() noexcept {
    if (!active_ || recorder_ == nullptr || clock_ == nullptr) {
        return;
    }

    active_ = false;
    const auto end_time_ns = clock_->now_ns();
    const auto duration_ns = end_time_ns >= start_time_ns_ ? end_time_ns - start_time_ns_ : 0;

    (void)try_record_profile_sample(*recorder_, ProfileSample{
                                                    .name = std::move(name_),
                                                    .frame_index = frame_index_,
                                                    .start_time_ns = start_time_ns_,
                                                    .duration_ns = duration_ns,
                                                    .depth = depth_,
                                                });

    recorder_->end_profile_zone();
    recorder_ = nullptr;
    clock_ = nullptr;
}

DiagnosticSummary summarize_diagnostics(const DiagnosticCapture& capture) noexcept {
    DiagnosticSummary summary;
    summary.event_count = static_cast<std::uint64_t>(capture.events.size());
    summary.counter_count = static_cast<std::uint64_t>(capture.counters.size());
    summary.profile_count = static_cast<std::uint64_t>(capture.profiles.size());

    for (const auto& event : capture.events) {
        if (event.severity == DiagnosticSeverity::warning) {
            ++summary.warning_count;
        }
        if (event.severity == DiagnosticSeverity::error || event.severity == DiagnosticSeverity::fatal) {
            ++summary.error_count;
        }
    }

    bool has_profile_sample = false;
    for (const auto& profile : capture.profiles) {
        summary.total_profile_time_ns += profile.duration_ns;
        if (!has_profile_sample || profile.duration_ns < summary.min_profile_time_ns) {
            summary.min_profile_time_ns = profile.duration_ns;
        }
        summary.max_profile_time_ns = std::max(profile.duration_ns, summary.max_profile_time_ns);
        has_profile_sample = true;
    }

    return summary;
}

std::string_view diagnostic_severity_label(DiagnosticSeverity severity) noexcept {
    switch (severity) {
    case DiagnosticSeverity::trace:
        return "trace";
    case DiagnosticSeverity::debug:
        return "debug";
    case DiagnosticSeverity::info:
        return "info";
    case DiagnosticSeverity::warning:
        return "warning";
    case DiagnosticSeverity::error:
        return "error";
    case DiagnosticSeverity::fatal:
        return "fatal";
    }
    return "unknown";
}

std::string_view diagnostics_ops_artifact_kind_label(DiagnosticsOpsArtifactKind kind) noexcept {
    switch (kind) {
    case DiagnosticsOpsArtifactKind::summary:
        return "summary";
    case DiagnosticsOpsArtifactKind::trace_event_json:
        return "trace_event_json";
    case DiagnosticsOpsArtifactKind::crash_dump_review:
        return "crash_dump_review";
    case DiagnosticsOpsArtifactKind::telemetry_upload:
        return "telemetry_upload";
    }
    return "unknown";
}

std::string_view diagnostics_ops_artifact_status_label(DiagnosticsOpsArtifactStatus status) noexcept {
    switch (status) {
    case DiagnosticsOpsArtifactStatus::ready:
        return "ready";
    case DiagnosticsOpsArtifactStatus::host_gated:
        return "host_gated";
    case DiagnosticsOpsArtifactStatus::unsupported:
        return "unsupported";
    }
    return "unknown";
}

DiagnosticsOpsPlan build_diagnostics_ops_plan(const DiagnosticCapture& capture,
                                              const DiagnosticsOpsPlanOptions& options) {
    DiagnosticsOpsPlan plan;
    plan.summary = summarize_diagnostics(capture);
    plan.artifacts.reserve(4);

    const auto add_capture_counts = [&capture](DiagnosticsOpsArtifact& artifact) {
        artifact.event_count = static_cast<std::uint64_t>(capture.events.size());
        artifact.counter_count = static_cast<std::uint64_t>(capture.counters.size());
        artifact.profile_count = static_cast<std::uint64_t>(capture.profiles.size());
    };

    DiagnosticsOpsArtifact summary;
    summary.kind = DiagnosticsOpsArtifactKind::summary;
    summary.status = DiagnosticsOpsArtifactStatus::ready;
    summary.id = "diagnostic-summary";
    summary.label = "Diagnostic summary";
    summary.producer = "summarize_diagnostics";
    summary.format = "mirakana::DiagnosticSummary";
    summary.event_count = plan.summary.event_count;
    summary.counter_count = plan.summary.counter_count;
    summary.profile_count = plan.summary.profile_count;
    plan.artifacts.push_back(std::move(summary));

    DiagnosticsOpsArtifact trace;
    trace.kind = DiagnosticsOpsArtifactKind::trace_event_json;
    trace.status = DiagnosticsOpsArtifactStatus::ready;
    trace.id = "trace-event-json";
    trace.label = "Trace Event JSON";
    trace.producer = "export_diagnostics_trace_json";
    trace.format = "Chrome Trace Event JSON";
    add_capture_counts(trace);
    plan.artifacts.push_back(std::move(trace));

    DiagnosticsOpsArtifact crash;
    crash.kind = DiagnosticsOpsArtifactKind::crash_dump_review;
    crash.id = "crash-dump-review";
    crash.label = "Crash dump review";
    crash.format = "Windows native dump review";
    if (options.host_status.debugging_tools_for_windows_available) {
        crash.status = DiagnosticsOpsArtifactStatus::ready;
        crash.producer = "Debugging Tools for Windows";
    } else {
        crash.status = DiagnosticsOpsArtifactStatus::host_gated;
        crash.blocker = "Debugging Tools for Windows is required for native crash and dump review.";
    }
    plan.artifacts.push_back(std::move(crash));

    DiagnosticsOpsArtifact telemetry;
    telemetry.kind = DiagnosticsOpsArtifactKind::telemetry_upload;
    telemetry.id = "telemetry-upload";
    telemetry.label = "Telemetry upload";
    telemetry.format = "caller-defined telemetry payload";
    add_capture_counts(telemetry);
    if (options.host_status.telemetry_backend_configured) {
        telemetry.status = DiagnosticsOpsArtifactStatus::ready;
        telemetry.producer = "caller-provided telemetry backend";
    } else {
        telemetry.status = DiagnosticsOpsArtifactStatus::unsupported;
        telemetry.blocker = "No telemetry backend is configured; mirakana_core does not upload diagnostics.";
    }
    plan.artifacts.push_back(std::move(telemetry));

    return plan;
}

DiagnosticsTraceImportReview review_diagnostics_trace_json(std::string_view json) {
    TraceJsonReviewParser parser(json);
    return parser.parse();
}

DiagnosticsTraceImportResult import_diagnostics_trace_json(std::string_view json) {
    DiagnosticsTraceImportResult result;
    TraceJsonReviewParser parser(json, &result.capture, &result.diagnostics);
    result.review = parser.parse();
    if (!result.review.valid) {
        result.diagnostics = result.review.diagnostics;
        return result;
    }

    result.valid = result.diagnostics.empty();
    if (!result.valid) {
        result.capture = DiagnosticCapture{};
    }
    return result;
}

std::string export_diagnostics_trace_json(const DiagnosticCapture& capture,
                                          const DiagnosticsTraceExportOptions& options) {
    std::string output;
    output.reserve(256 + ((capture.events.size() + capture.counters.size() + capture.profiles.size()) * 160));
    output += "{\"traceEvents\":[";

    bool needs_separator = false;
    const auto append_separator = [&output, &needs_separator]() {
        if (needs_separator) {
            output.push_back(',');
        }
        needs_separator = true;
    };

    if (options.include_metadata) {
        append_separator();
        append_metadata_event(output, "process_name", options.trace_name, options);
        append_separator();
        append_metadata_event(output, "thread_name", options.thread_name, options);
    }

    for (const auto& event : capture.events) {
        append_separator();
        output.push_back('{');
        append_json_key(output, "name");
        append_json_string(output, event.message);
        output.push_back(',');
        append_json_key(output, "cat");
        append_json_string(output, event.category);
        output.push_back(',');
        append_json_key(output, "ph");
        append_json_string(output, "i");
        output.push_back(',');
        append_json_key(output, "s");
        append_json_string(output, "t");
        output.push_back(',');
        append_json_key(output, "ts");
        append_uint(output, event.frame_index);
        output.push_back(',');
        append_json_key(output, "pid");
        append_uint(output, options.process_id);
        output.push_back(',');
        append_json_key(output, "tid");
        append_uint(output, options.thread_id);
        output.push_back(',');
        append_json_key(output, "args");
        output.push_back('{');
        append_json_key(output, "severity");
        append_json_string(output, diagnostic_severity_label(event.severity));
        output.push_back(',');
        append_json_key(output, "frame_index");
        append_uint(output, event.frame_index);
        output += "}}";
    }

    for (const auto& counter : capture.counters) {
        if (!std::isfinite(counter.value)) {
            continue;
        }
        append_separator();
        output.push_back('{');
        append_common_trace_fields(output, counter.name, "counter", "C", counter.frame_index, options);
        output.push_back(',');
        append_json_key(output, "args");
        output.push_back('{');
        append_json_key(output, "value");
        append_double(output, counter.value);
        output.push_back(',');
        append_json_key(output, "frame_index");
        append_uint(output, counter.frame_index);
        output += "}}";
    }

    for (const auto& profile : capture.profiles) {
        append_separator();
        output.push_back('{');
        append_common_trace_fields(output, profile.name, "profile", "X", profile.start_time_ns / 1000U, options);
        output.push_back(',');
        append_json_key(output, "dur");
        append_uint(output, profile.duration_ns / 1000U);
        output.push_back(',');
        append_json_key(output, "args");
        output.push_back('{');
        append_json_key(output, "frame_index");
        append_uint(output, profile.frame_index);
        output.push_back(',');
        append_json_key(output, "depth");
        append_uint(output, profile.depth);
        output += "}}";
    }

    output += "]}";
    return output;
}

} // namespace mirakana
