// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::runtime {

struct RuntimeProceduralSeedStream {
    std::uint64_t state{0ULL};

    [[nodiscard]] bool operator==(const RuntimeProceduralSeedStream&) const = default;
};

enum class RuntimeProceduralGenerationContentKind : std::uint8_t {
    map_tile,
    encounter,
    loot,
    object,
};

struct RuntimeProceduralGenerationContentRequest {
    std::string content_id;
    RuntimeProceduralGenerationContentKind kind{RuntimeProceduralGenerationContentKind::object};
    std::uint32_t count{0U};

    [[nodiscard]] bool operator==(const RuntimeProceduralGenerationContentRequest&) const = default;
};

struct RuntimeProceduralGenerationRequest {
    std::string generator_id;
    std::uint64_t seed{0ULL};
    std::uint32_t map_width{0U};
    std::uint32_t map_height{0U};
    std::uint32_t output_budget{0U};
    std::vector<RuntimeProceduralGenerationContentRequest> content;

    [[nodiscard]] bool operator==(const RuntimeProceduralGenerationRequest&) const = default;
};

struct RuntimeProceduralGenerationContext {
    std::span<const std::string> supported_content_ids;
    std::uint32_t max_output_rows{0U};
};

enum class RuntimeProceduralGenerationDiagnosticCode : std::uint8_t {
    none,
    invalid_generator_id,
    invalid_seed,
    invalid_map_extent,
    output_budget_exceeded,
    invalid_content_count,
    duplicate_content_id,
    unsupported_content_id,
    unsupported_content_kind,
};

struct RuntimeProceduralGenerationDiagnostic {
    RuntimeProceduralGenerationDiagnosticCode code{RuntimeProceduralGenerationDiagnosticCode::none};
    std::string content_id;
    RuntimeProceduralGenerationContentKind kind{RuntimeProceduralGenerationContentKind::object};
    std::uint32_t count{0U};
    std::uint32_t output_budget{0U};
    std::uint32_t requested_output_rows{0U};

    [[nodiscard]] bool operator==(const RuntimeProceduralGenerationDiagnostic&) const = default;
};

struct RuntimeProceduralGenerationOutputRow {
    std::string id;
    std::string content_id;
    RuntimeProceduralGenerationContentKind kind{RuntimeProceduralGenerationContentKind::object};
    std::uint32_t index{0U};
    std::uint32_t x{0U};
    std::uint32_t y{0U};
    std::uint64_t stable_value{0ULL};

    [[nodiscard]] bool operator==(const RuntimeProceduralGenerationOutputRow&) const = default;
};

struct RuntimeProceduralGenerationPlan {
    bool succeeded{true};
    std::vector<RuntimeProceduralGenerationDiagnostic> diagnostics;
    std::vector<RuntimeProceduralGenerationOutputRow> rows;
    std::uint64_t replay_hash{0ULL};
};

[[nodiscard]] RuntimeProceduralSeedStream make_runtime_procedural_seed_stream(std::uint64_t seed,
                                                                              std::string_view salt) noexcept;

[[nodiscard]] std::uint64_t advance_runtime_procedural_seed(RuntimeProceduralSeedStream& stream) noexcept;

[[nodiscard]] RuntimeProceduralGenerationPlan
plan_runtime_procedural_generation(const RuntimeProceduralGenerationRequest& request,
                                   RuntimeProceduralGenerationContext context);

} // namespace mirakana::runtime
