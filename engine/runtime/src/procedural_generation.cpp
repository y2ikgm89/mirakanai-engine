// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/procedural_generation.hpp"

#include <algorithm>
#include <limits>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

namespace mirakana::runtime {
namespace {

constexpr std::uint64_t kFnvOffset = 14695981039346656037ULL;
constexpr std::uint64_t kFnvPrime = 1099511628211ULL;
constexpr std::uint64_t kSplitMixIncrement = 0x9e3779b97f4a7c15ULL;

[[nodiscard]] bool contains_control_character(const std::string_view value) noexcept {
    return std::ranges::any_of(value, [](const char character) {
        const auto byte = static_cast<unsigned char>(character);
        return byte < 0x20U || byte == 0x7FU;
    });
}

[[nodiscard]] bool is_safe_token_character(const char character) noexcept {
    return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
           (character >= '0' && character <= '9') || character == '_' || character == '-' || character == '.' ||
           character == ':' || character == '/';
}

[[nodiscard]] bool is_safe_token(const std::string_view value) noexcept {
    return !value.empty() && !contains_control_character(value) && std::ranges::all_of(value, is_safe_token_character);
}

[[nodiscard]] bool contains_string(const std::span<const std::string> values, const std::string_view value) noexcept {
    return std::ranges::find_if(values, [value](const std::string& candidate) {
               return std::string_view{candidate} == value;
           }) != values.end();
}

[[nodiscard]] bool is_supported_kind(const RuntimeProceduralGenerationContentKind kind) noexcept {
    switch (kind) {
    case RuntimeProceduralGenerationContentKind::map_tile:
    case RuntimeProceduralGenerationContentKind::encounter:
    case RuntimeProceduralGenerationContentKind::loot:
    case RuntimeProceduralGenerationContentKind::object:
        return true;
    }
    return false;
}

[[nodiscard]] std::uint64_t stable_hash(const std::string_view value) noexcept {
    std::uint64_t hash = kFnvOffset;
    for (const char character : value) {
        hash ^= static_cast<unsigned char>(character);
        hash *= kFnvPrime;
    }
    return hash;
}

[[nodiscard]] std::uint64_t mix64(std::uint64_t value) noexcept {
    value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31U);
}

[[nodiscard]] std::uint64_t combine_hash(std::uint64_t hash, const std::uint64_t value) noexcept {
    return mix64(hash ^ (value + kSplitMixIncrement + (hash << 6U) + (hash >> 2U)));
}

void add_diagnostic(std::vector<RuntimeProceduralGenerationDiagnostic>& diagnostics,
                    RuntimeProceduralGenerationDiagnostic diagnostic) {
    diagnostics.push_back(std::move(diagnostic));
}

[[nodiscard]] std::uint32_t
saturating_requested_rows(const std::span<const RuntimeProceduralGenerationContentRequest> content) noexcept {
    std::uint64_t requested = 0ULL;
    for (const auto& row : content) {
        requested += row.count;
    }
    return requested > std::numeric_limits<std::uint32_t>::max() ? std::numeric_limits<std::uint32_t>::max()
                                                                 : static_cast<std::uint32_t>(requested);
}

[[nodiscard]] bool output_budget_exceeded(const RuntimeProceduralGenerationRequest& request,
                                          const RuntimeProceduralGenerationContext context,
                                          const std::uint32_t requested_rows) noexcept {
    if (requested_rows == 0U) {
        return false;
    }
    if (request.output_budget == 0U || requested_rows > request.output_budget) {
        return true;
    }
    return context.max_output_rows != 0U && requested_rows > context.max_output_rows;
}

} // namespace

RuntimeProceduralSeedStream make_runtime_procedural_seed_stream(const std::uint64_t seed,
                                                                const std::string_view salt) noexcept {
    return RuntimeProceduralSeedStream{.state = mix64(seed ^ stable_hash(salt))};
}

std::uint64_t advance_runtime_procedural_seed(RuntimeProceduralSeedStream& stream) noexcept {
    stream.state += kSplitMixIncrement;
    return mix64(stream.state);
}

RuntimeProceduralGenerationPlan plan_runtime_procedural_generation(const RuntimeProceduralGenerationRequest& request,
                                                                   const RuntimeProceduralGenerationContext context) {
    RuntimeProceduralGenerationPlan plan;
    const auto requested_rows = saturating_requested_rows(request.content);

    if (!is_safe_token(request.generator_id)) {
        add_diagnostic(plan.diagnostics, RuntimeProceduralGenerationDiagnostic{
                                             .code = RuntimeProceduralGenerationDiagnosticCode::invalid_generator_id,
                                             .requested_output_rows = requested_rows,
                                         });
    }
    if (request.seed == 0ULL) {
        add_diagnostic(plan.diagnostics, RuntimeProceduralGenerationDiagnostic{
                                             .code = RuntimeProceduralGenerationDiagnosticCode::invalid_seed,
                                             .requested_output_rows = requested_rows,
                                         });
    }
    if (request.map_width == 0U || request.map_height == 0U) {
        add_diagnostic(plan.diagnostics, RuntimeProceduralGenerationDiagnostic{
                                             .code = RuntimeProceduralGenerationDiagnosticCode::invalid_map_extent,
                                             .requested_output_rows = requested_rows,
                                         });
    }
    if (output_budget_exceeded(request, context, requested_rows)) {
        add_diagnostic(plan.diagnostics, RuntimeProceduralGenerationDiagnostic{
                                             .code = RuntimeProceduralGenerationDiagnosticCode::output_budget_exceeded,
                                             .output_budget = request.output_budget,
                                             .requested_output_rows = requested_rows,
                                         });
    }

    std::vector<std::string> seen_content_ids;
    for (const auto& content : request.content) {
        if (content.count == 0U) {
            add_diagnostic(plan.diagnostics,
                           RuntimeProceduralGenerationDiagnostic{
                               .code = RuntimeProceduralGenerationDiagnosticCode::invalid_content_count,
                               .content_id = content.content_id,
                               .kind = content.kind,
                               .count = content.count,
                               .output_budget = request.output_budget,
                               .requested_output_rows = requested_rows,
                           });
            continue;
        }

        if (std::ranges::find(seen_content_ids, content.content_id) != seen_content_ids.end()) {
            add_diagnostic(plan.diagnostics,
                           RuntimeProceduralGenerationDiagnostic{
                               .code = RuntimeProceduralGenerationDiagnosticCode::duplicate_content_id,
                               .content_id = content.content_id,
                               .kind = content.kind,
                               .count = content.count,
                               .output_budget = request.output_budget,
                               .requested_output_rows = requested_rows,
                           });
            continue;
        }
        seen_content_ids.push_back(content.content_id);

        if (!is_safe_token(content.content_id) || !contains_string(context.supported_content_ids, content.content_id)) {
            add_diagnostic(plan.diagnostics,
                           RuntimeProceduralGenerationDiagnostic{
                               .code = RuntimeProceduralGenerationDiagnosticCode::unsupported_content_id,
                               .content_id = content.content_id,
                               .kind = content.kind,
                               .count = content.count,
                               .output_budget = request.output_budget,
                               .requested_output_rows = requested_rows,
                           });
        }
        if (!is_supported_kind(content.kind)) {
            add_diagnostic(plan.diagnostics,
                           RuntimeProceduralGenerationDiagnostic{
                               .code = RuntimeProceduralGenerationDiagnosticCode::unsupported_content_kind,
                               .content_id = content.content_id,
                               .kind = content.kind,
                               .count = content.count,
                               .output_budget = request.output_budget,
                               .requested_output_rows = requested_rows,
                           });
        }
    }

    plan.succeeded = plan.diagnostics.empty();
    if (!plan.succeeded) {
        plan.replay_hash = 0ULL;
        return plan;
    }

    plan.replay_hash = combine_hash(stable_hash(request.generator_id), request.seed);
    plan.replay_hash = combine_hash(plan.replay_hash, request.map_width);
    plan.replay_hash = combine_hash(plan.replay_hash, request.map_height);
    for (const auto& content : request.content) {
        std::string salt = request.generator_id;
        salt.push_back(':');
        salt.append(content.content_id);
        auto stream = make_runtime_procedural_seed_stream(request.seed, salt);

        for (std::uint32_t index = 0U; index < content.count; ++index) {
            const auto stable_value = advance_runtime_procedural_seed(stream);
            RuntimeProceduralGenerationOutputRow row{
                .id = content.content_id + ":" + std::to_string(index),
                .content_id = content.content_id,
                .kind = content.kind,
                .index = index,
                .x = static_cast<std::uint32_t>(stable_value % request.map_width),
                .y = static_cast<std::uint32_t>((stable_value >> 32U) % request.map_height),
                .stable_value = stable_value,
            };
            plan.replay_hash = combine_hash(plan.replay_hash, stable_hash(row.id));
            plan.replay_hash = combine_hash(plan.replay_hash, static_cast<std::uint64_t>(row.kind));
            plan.replay_hash = combine_hash(plan.replay_hash, row.index);
            plan.replay_hash = combine_hash(plan.replay_hash, row.x);
            plan.replay_hash = combine_hash(plan.replay_hash, row.y);
            plan.replay_hash = combine_hash(plan.replay_hash, row.stable_value);
            plan.rows.push_back(std::move(row));
        }
    }

    return plan;
}

} // namespace mirakana::runtime
