// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/procedural_generation.hpp"

#include <span>
#include <string>
#include <vector>

namespace {

[[nodiscard]] mirakana::runtime::RuntimeProceduralGenerationContext procedural_context() {
    static const std::vector<std::string> content_ids{"terrain", "encounter", "loot", "prop"};

    return mirakana::runtime::RuntimeProceduralGenerationContext{
        .supported_content_ids = std::span<const std::string>{content_ids},
        .max_output_rows = 32U,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeProceduralGenerationRequest procedural_request() {
    using namespace mirakana::runtime;

    return RuntimeProceduralGenerationRequest{
        .generator_id = "arena.seeded.v1",
        .seed = 0x1234abcdULL,
        .map_width = 16U,
        .map_height = 9U,
        .output_budget = 8U,
        .content =
            std::vector<RuntimeProceduralGenerationContentRequest>{
                RuntimeProceduralGenerationContentRequest{
                    .content_id = "terrain",
                    .kind = RuntimeProceduralGenerationContentKind::map_tile,
                    .count = 3U,
                },
                RuntimeProceduralGenerationContentRequest{
                    .content_id = "encounter",
                    .kind = RuntimeProceduralGenerationContentKind::encounter,
                    .count = 2U,
                },
                RuntimeProceduralGenerationContentRequest{
                    .content_id = "loot",
                    .kind = RuntimeProceduralGenerationContentKind::loot,
                    .count = 1U,
                },
            },
    };
}

} // namespace

MK_TEST("runtime procedural generation plan is deterministic for identical seed and inputs") {
    const auto request = procedural_request();

    const auto first = mirakana::runtime::plan_runtime_procedural_generation(request, procedural_context());
    const auto second = mirakana::runtime::plan_runtime_procedural_generation(request, procedural_context());

    MK_REQUIRE(first.succeeded);
    MK_REQUIRE(first.diagnostics.empty());
    MK_REQUIRE(first.rows == second.rows);
    MK_REQUIRE(first.replay_hash == second.replay_hash);
    MK_REQUIRE(first.rows.size() == 6U);
    MK_REQUIRE(first.rows[0].id == "terrain:0");
    MK_REQUIRE(first.rows[0].kind == mirakana::runtime::RuntimeProceduralGenerationContentKind::map_tile);
    MK_REQUIRE(first.rows[0].x < request.map_width);
    MK_REQUIRE(first.rows[0].y < request.map_height);
    MK_REQUIRE(first.rows[3].id == "encounter:0");
    MK_REQUIRE(first.rows[5].id == "loot:0");
    MK_REQUIRE(first.replay_hash != 0ULL);
}

MK_TEST("runtime procedural generation reports invalid requests without output rows") {
    using Code = mirakana::runtime::RuntimeProceduralGenerationDiagnosticCode;
    using Kind = mirakana::runtime::RuntimeProceduralGenerationContentKind;
    using namespace mirakana::runtime;

    auto request = procedural_request();
    request.generator_id = "unsafe generator id";
    request.seed = 0ULL;
    request.map_width = 0U;
    request.output_budget = 1U;
    request.content.push_back(RuntimeProceduralGenerationContentRequest{
        .content_id = "terrain",
        .kind = RuntimeProceduralGenerationContentKind::map_tile,
        .count = 1U,
    });
    request.content.push_back(RuntimeProceduralGenerationContentRequest{
        .content_id = "missing",
        .kind = RuntimeProceduralGenerationContentKind::object,
        .count = 1U,
    });
    request.content.push_back(RuntimeProceduralGenerationContentRequest{
        .content_id = "prop",
        .kind = static_cast<Kind>(255),
        .count = 1U,
    });
    request.content.push_back(RuntimeProceduralGenerationContentRequest{
        .content_id = "loot",
        .kind = RuntimeProceduralGenerationContentKind::loot,
        .count = 0U,
    });

    const auto first = plan_runtime_procedural_generation(request, procedural_context());
    const auto second = plan_runtime_procedural_generation(request, procedural_context());

    MK_REQUIRE(!first.succeeded);
    MK_REQUIRE(first.rows.empty());
    MK_REQUIRE(first.replay_hash == 0ULL);
    MK_REQUIRE(first.diagnostics == second.diagnostics);
    MK_REQUIRE(first.diagnostics.size() == 8U);
    MK_REQUIRE(first.diagnostics[0].code == Code::invalid_generator_id);
    MK_REQUIRE(first.diagnostics[1].code == Code::invalid_seed);
    MK_REQUIRE(first.diagnostics[2].code == Code::invalid_map_extent);
    MK_REQUIRE(first.diagnostics[3].code == Code::output_budget_exceeded);
    MK_REQUIRE(first.diagnostics[4].code == Code::duplicate_content_id);
    MK_REQUIRE(first.diagnostics[5].code == Code::unsupported_content_id);
    MK_REQUIRE(first.diagnostics[6].code == Code::unsupported_content_kind);
    MK_REQUIRE(first.diagnostics[7].code == Code::invalid_content_count);
}

MK_TEST("runtime procedural seed stream is stable and salt-sensitive") {
    auto first = mirakana::runtime::make_runtime_procedural_seed_stream(0x42ULL, "loot");
    auto repeat = mirakana::runtime::make_runtime_procedural_seed_stream(0x42ULL, "loot");
    auto other = mirakana::runtime::make_runtime_procedural_seed_stream(0x42ULL, "encounter");

    const auto first_a = mirakana::runtime::advance_runtime_procedural_seed(first);
    const auto repeat_a = mirakana::runtime::advance_runtime_procedural_seed(repeat);
    const auto other_a = mirakana::runtime::advance_runtime_procedural_seed(other);
    const auto first_b = mirakana::runtime::advance_runtime_procedural_seed(first);

    MK_REQUIRE(first_a == repeat_a);
    MK_REQUIRE(first_a != other_a);
    MK_REQUIRE(first_a != first_b);
}

int main() {
    return mirakana::test::run_all();
}
