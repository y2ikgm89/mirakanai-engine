// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

enum class EditorUiPerformanceMetric : std::uint8_t {
    layout_us,
    document_build_us,
    renderer_submission_us,
    text_runs,
    renderer_boxes,
    visible_texture_composites,
    memory_high_water_bytes
};

enum class EditorUiPerformanceBudgetStatus : std::uint8_t { missing_samples, ready, violations, invalid_budget };

struct EditorUiPerformanceBudget {
    EditorUiPerformanceMetric metric{EditorUiPerformanceMetric::layout_us};
    double p95_limit{0.0};
    bool evidence_required{true};
};

struct EditorUiPerformanceSample {
    double layout_us{0.0};
    double document_build_us{0.0};
    double renderer_submission_us{0.0};
    std::uint64_t text_runs{0};
    std::uint64_t renderer_boxes{0};
    std::uint64_t visible_texture_composites{0};
    std::uint64_t memory_high_water_bytes{0};
};

struct EditorUiPerformanceSummary {
    EditorUiPerformanceBudgetStatus status{EditorUiPerformanceBudgetStatus::missing_samples};
    double layout_us_p95{0.0};
    double document_build_us_p95{0.0};
    double renderer_submission_us_p95{0.0};
    std::uint64_t text_runs_p95{0};
    std::uint64_t renderer_boxes_p95{0};
    std::uint64_t visible_texture_composites_p95{0};
    std::uint64_t memory_high_water_bytes{0};
    std::uint32_t budget_violations{0};
    bool broad_optimization_claimed{false};
    std::vector<std::string> diagnostics;
};

[[nodiscard]] std::string_view editor_ui_performance_metric_id(EditorUiPerformanceMetric metric) noexcept;
[[nodiscard]] std::string_view editor_ui_performance_budget_status_id(EditorUiPerformanceBudgetStatus status) noexcept;
[[nodiscard]] std::vector<EditorUiPerformanceBudget> make_default_editor_ui_performance_budgets();
[[nodiscard]] EditorUiPerformanceSummary
summarize_editor_ui_performance(std::span<const EditorUiPerformanceSample> samples,
                                std::span<const EditorUiPerformanceBudget> budgets);

} // namespace mirakana::editor
