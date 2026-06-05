// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/editor_ui_performance.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

namespace mirakana::editor {
namespace {

[[nodiscard]] double nearest_rank_p95(std::vector<double> values) {
    if (values.empty()) {
        return 0.0;
    }
    std::ranges::sort(values);
    auto one_based_rank = static_cast<std::size_t>(std::ceil(0.95 * static_cast<double>(values.size())));
    if (one_based_rank == 0U) {
        one_based_rank = 1U;
    }
    one_based_rank = std::min(one_based_rank, values.size());
    return values[one_based_rank - 1U];
}

[[nodiscard]] std::uint64_t nearest_rank_p95_u64(std::vector<std::uint64_t> values) {
    if (values.empty()) {
        return 0;
    }
    std::ranges::sort(values);
    auto one_based_rank = static_cast<std::size_t>(std::ceil(0.95 * static_cast<double>(values.size())));
    if (one_based_rank == 0U) {
        one_based_rank = 1U;
    }
    one_based_rank = std::min(one_based_rank, values.size());
    return values[one_based_rank - 1U];
}

[[nodiscard]] double metric_value(const EditorUiPerformanceSummary& summary,
                                  EditorUiPerformanceMetric metric) noexcept {
    switch (metric) {
    case EditorUiPerformanceMetric::layout_us:
        return summary.layout_us_p95;
    case EditorUiPerformanceMetric::document_build_us:
        return summary.document_build_us_p95;
    case EditorUiPerformanceMetric::renderer_submission_us:
        return summary.renderer_submission_us_p95;
    case EditorUiPerformanceMetric::text_runs:
        return static_cast<double>(summary.text_runs_p95);
    case EditorUiPerformanceMetric::renderer_boxes:
        return static_cast<double>(summary.renderer_boxes_p95);
    case EditorUiPerformanceMetric::visible_texture_composites:
        return static_cast<double>(summary.visible_texture_composites_p95);
    case EditorUiPerformanceMetric::memory_high_water_bytes:
        return static_cast<double>(summary.memory_high_water_bytes);
    }
    return 0.0;
}

} // namespace

std::string_view editor_ui_performance_metric_id(EditorUiPerformanceMetric metric) noexcept {
    switch (metric) {
    case EditorUiPerformanceMetric::layout_us:
        return "layout_us";
    case EditorUiPerformanceMetric::document_build_us:
        return "document_build_us";
    case EditorUiPerformanceMetric::renderer_submission_us:
        return "renderer_submission_us";
    case EditorUiPerformanceMetric::text_runs:
        return "text_runs";
    case EditorUiPerformanceMetric::renderer_boxes:
        return "renderer_boxes";
    case EditorUiPerformanceMetric::visible_texture_composites:
        return "visible_texture_composites";
    case EditorUiPerformanceMetric::memory_high_water_bytes:
        return "memory_high_water_bytes";
    }
    return "unknown";
}

std::string_view editor_ui_performance_budget_status_id(EditorUiPerformanceBudgetStatus status) noexcept {
    switch (status) {
    case EditorUiPerformanceBudgetStatus::missing_samples:
        return "missing_samples";
    case EditorUiPerformanceBudgetStatus::ready:
        return "ready";
    case EditorUiPerformanceBudgetStatus::violations:
        return "violations";
    case EditorUiPerformanceBudgetStatus::invalid_budget:
        return "invalid_budget";
    }
    return "invalid_budget";
}

std::vector<EditorUiPerformanceBudget> make_default_editor_ui_performance_budgets() {
    return {
        EditorUiPerformanceBudget{.metric = EditorUiPerformanceMetric::layout_us, .p95_limit = 50000.0},
        EditorUiPerformanceBudget{.metric = EditorUiPerformanceMetric::document_build_us, .p95_limit = 50000.0},
        EditorUiPerformanceBudget{.metric = EditorUiPerformanceMetric::renderer_submission_us, .p95_limit = 50000.0},
        EditorUiPerformanceBudget{.metric = EditorUiPerformanceMetric::text_runs, .p95_limit = 4096.0},
        EditorUiPerformanceBudget{.metric = EditorUiPerformanceMetric::renderer_boxes, .p95_limit = 16384.0},
        EditorUiPerformanceBudget{.metric = EditorUiPerformanceMetric::visible_texture_composites, .p95_limit = 64.0},
        EditorUiPerformanceBudget{.metric = EditorUiPerformanceMetric::memory_high_water_bytes,
                                  .p95_limit = 134217728.0},
    };
}

EditorUiPerformanceSummary summarize_editor_ui_performance(std::span<const EditorUiPerformanceSample> samples,
                                                           std::span<const EditorUiPerformanceBudget> budgets) {
    EditorUiPerformanceSummary summary;
    if (samples.empty()) {
        summary.diagnostics.push_back("editor UI performance requires at least one sample");
        return summary;
    }
    if (budgets.empty()) {
        summary.status = EditorUiPerformanceBudgetStatus::invalid_budget;
        summary.diagnostics.push_back("editor UI performance requires at least one budget row");
        return summary;
    }

    std::vector<double> layout_us;
    std::vector<double> document_build_us;
    std::vector<double> renderer_submission_us;
    std::vector<std::uint64_t> text_runs;
    std::vector<std::uint64_t> renderer_boxes;
    std::vector<std::uint64_t> visible_texture_composites;
    layout_us.reserve(samples.size());
    document_build_us.reserve(samples.size());
    renderer_submission_us.reserve(samples.size());
    text_runs.reserve(samples.size());
    renderer_boxes.reserve(samples.size());
    visible_texture_composites.reserve(samples.size());

    bool invalid_sample = false;
    for (const auto& sample : samples) {
        if (!std::isfinite(sample.layout_us) || !std::isfinite(sample.document_build_us) ||
            !std::isfinite(sample.renderer_submission_us) || sample.layout_us < 0.0 || sample.document_build_us < 0.0 ||
            sample.renderer_submission_us < 0.0) {
            invalid_sample = true;
        }
        layout_us.push_back(sample.layout_us);
        document_build_us.push_back(sample.document_build_us);
        renderer_submission_us.push_back(sample.renderer_submission_us);
        text_runs.push_back(sample.text_runs);
        renderer_boxes.push_back(sample.renderer_boxes);
        visible_texture_composites.push_back(sample.visible_texture_composites);
        summary.memory_high_water_bytes = std::max(summary.memory_high_water_bytes, sample.memory_high_water_bytes);
    }

    summary.layout_us_p95 = nearest_rank_p95(std::move(layout_us));
    summary.document_build_us_p95 = nearest_rank_p95(std::move(document_build_us));
    summary.renderer_submission_us_p95 = nearest_rank_p95(std::move(renderer_submission_us));
    summary.text_runs_p95 = nearest_rank_p95_u64(std::move(text_runs));
    summary.renderer_boxes_p95 = nearest_rank_p95_u64(std::move(renderer_boxes));
    summary.visible_texture_composites_p95 = nearest_rank_p95_u64(std::move(visible_texture_composites));

    if (invalid_sample) {
        summary.status = EditorUiPerformanceBudgetStatus::invalid_budget;
        summary.diagnostics.push_back("editor UI performance samples must be finite and non-negative");
        return summary;
    }

    for (const auto& budget : budgets) {
        if ((!std::isfinite(budget.p95_limit) && budget.p95_limit != std::numeric_limits<double>::infinity()) ||
            budget.p95_limit < 0.0) {
            summary.status = EditorUiPerformanceBudgetStatus::invalid_budget;
            summary.diagnostics.push_back(std::string(editor_ui_performance_metric_id(budget.metric)) +
                                          " budget limit must be finite, infinite, or non-negative");
            return summary;
        }
        if (budget.evidence_required && metric_value(summary, budget.metric) > budget.p95_limit) {
            ++summary.budget_violations;
            summary.diagnostics.push_back(std::string(editor_ui_performance_metric_id(budget.metric)) +
                                          " exceeds p95 budget");
        }
    }

    summary.status = summary.budget_violations == 0U ? EditorUiPerformanceBudgetStatus::ready
                                                     : EditorUiPerformanceBudgetStatus::violations;
    summary.broad_optimization_claimed = false;
    return summary;
}

} // namespace mirakana::editor
