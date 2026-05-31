// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "first_party_editor_adapter_boundaries.hpp"
#include "first_party_editor_document.hpp"
#include "native_editor_app.hpp"
#include "native_editor_launch.hpp"
#include "native_material_preview_cache.hpp"
#include "native_viewport_surface.hpp"
#include "win32_first_party_editor_host.hpp"

#include "mirakana/editor/editor_dock_layout.hpp"
#include "mirakana/platform/file_dialog.hpp"
#include "mirakana/platform/process.hpp"
#include "mirakana/ui/ui.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace {

[[nodiscard]] mirakana::editor::NativeEditorLaunchParseResult parse_args(std::vector<std::string> args) {
    std::vector<char*> argv;
    argv.reserve(args.size());
    for (auto& arg : args) {
        argv.push_back(arg.data());
    }
    return mirakana::editor::parse_native_editor_launch(static_cast<int>(argv.size()), argv.data());
}

[[nodiscard]] const mirakana::editor::EditorResourceRow*
find_resource_row(const mirakana::editor::EditorResourcePanelModel& model, std::string_view id) noexcept {
    for (const auto& row : model.status_rows) {
        if (row.id == id) {
            return &row;
        }
    }
    return nullptr;
}

class RecordingClipboardTextAdapter final : public mirakana::ui::IClipboardTextAdapter {
  public:
    void set_clipboard_text(std::string_view text) override {
        text_ = text;
    }

    [[nodiscard]] bool has_clipboard_text() const override {
        return !text_.empty();
    }

    [[nodiscard]] std::string clipboard_text() const override {
        return text_;
    }

  private:
    std::string text_;
};

[[nodiscard]] mirakana::editor::EditorAiReviewedValidationExecutionDesc make_reviewed_validation_execution_desc() {
    mirakana::editor::EditorAiPlaytestOperatorHandoffCommandRow row{
        .recipe_id = "desktop-editor",
        .status = mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::ready,
        .command_display =
            "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe "
            "desktop-editor",
        .argv = {"pwsh", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/run-validation-recipe.ps1",
                 "-Mode", "DryRun", "-Recipe", "desktop-editor"},
        .host_gates = {},
        .blocked_by = {},
        .readiness_dependency = "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
        .diagnostic = "desktop-editor handoff ready",
    };

    return mirakana::editor::EditorAiReviewedValidationExecutionDesc{
        .command_row = row,
        .working_directory = "G:/workspace/development/GameEngine",
        .acknowledge_host_gates = false,
        .acknowledged_host_gates = {},
    };
}

[[nodiscard]] bool contains_element(const mirakana::ui::UiDocument& document, std::string_view id) {
    return document.find(mirakana::ui::ElementId{.value = std::string{id}}) != nullptr;
}

[[nodiscard]] const mirakana::editor::FirstPartyEditorAdapterBoundaryRow*
find_adapter_boundary(const std::vector<mirakana::editor::FirstPartyEditorAdapterBoundaryRow>& rows,
                      mirakana::editor::FirstPartyEditorAdapterBoundary boundary) noexcept {
    for (const auto& row : rows) {
        if (row.boundary == boundary) {
            return &row;
        }
    }
    return nullptr;
}

} // namespace

MK_TEST("editor native shell launch options default to interactive window") {
    const auto launch = parse_args({"MK_editor"});
    const auto& options = launch.options;

    MK_REQUIRE(options.width == 1280U);
    MK_REQUIRE(options.height == 720U);
    MK_REQUIRE(options.smoke_frames == -1);
    MK_REQUIRE(!options.no_user_config);

    const auto validation = mirakana::editor::validate_native_editor_launch(launch);
    MK_REQUIRE(validation.valid);
    MK_REQUIRE(validation.diagnostic.empty());
}

MK_TEST("editor native shell launch options accept bounded smoke frames") {
    const auto launch =
        parse_args({"MK_editor", "--width", "1024", "--height", "768", "--smoke-frames", "3", "--no-user-config"});
    const auto& options = launch.options;

    MK_REQUIRE(options.width == 1024U);
    MK_REQUIRE(options.height == 768U);
    MK_REQUIRE(options.smoke_frames == 3);
    MK_REQUIRE(!options.smoke_resize);
    MK_REQUIRE(options.no_user_config);

    const auto validation = mirakana::editor::validate_native_editor_launch(launch);
    MK_REQUIRE(validation.valid);
}

MK_TEST("editor native shell launch options accept deterministic smoke resize") {
    const auto launch = parse_args({"MK_editor", "--smoke-frames", "2", "--smoke-resize", "--no-user-config"});

    MK_REQUIRE(launch.options.smoke_frames == 2);
    MK_REQUIRE(launch.options.smoke_resize);

    const auto validation = mirakana::editor::validate_native_editor_launch(launch);
    MK_REQUIRE(validation.valid);
}

MK_TEST("editor native shell launch options reject smoke resize without enough frames") {
    const auto launch = parse_args({"MK_editor", "--smoke-frames", "1", "--smoke-resize"});

    const auto validation = mirakana::editor::validate_native_editor_launch(launch);
    MK_REQUIRE(!validation.valid);
    MK_REQUIRE(validation.diagnostic.find("smoke resize") != std::string::npos);
}

MK_TEST("editor native shell launch options reject zero window extent") {
    const auto launch = parse_args({"MK_editor", "--width", "0", "--height", "720"});

    const auto validation = mirakana::editor::validate_native_editor_launch(launch);
    MK_REQUIRE(!validation.valid);
    MK_REQUIRE(validation.diagnostic.find("window extent") != std::string::npos);
}

MK_TEST("editor native shell launch options reject negative smoke frames") {
    const auto launch = parse_args({"MK_editor", "--smoke-frames", "-3"});

    const auto validation = mirakana::editor::validate_native_editor_launch(launch);
    MK_REQUIRE(!validation.valid);
    MK_REQUIRE(validation.diagnostic.find("smoke frames") != std::string::npos);
}

MK_TEST("editor native shell launch options reject non numeric window extent") {
    const auto launch = parse_args({"MK_editor", "--width", "wide"});

    const auto validation = mirakana::editor::validate_native_editor_launch(launch);
    MK_REQUIRE(!validation.valid);
    MK_REQUIRE(validation.diagnostic.find("--width") != std::string::npos);
}

MK_TEST("editor native shell launch options reject missing option value") {
    const auto launch = parse_args({"MK_editor", "--height"});

    const auto validation = mirakana::editor::validate_native_editor_launch(launch);
    MK_REQUIRE(!validation.valid);
    MK_REQUIRE(validation.diagnostic.find("missing value") != std::string::npos);
    MK_REQUIRE(validation.diagnostic.find("--height") != std::string::npos);
}

MK_TEST("editor native shell launch options reject unknown option") {
    const auto launch = parse_args({"MK_editor", "--renderer", "legacy"});

    const auto validation = mirakana::editor::validate_native_editor_launch(launch);
    MK_REQUIRE(!validation.valid);
    MK_REQUIRE(validation.diagnostic.find("unknown option") != std::string::npos);
    MK_REQUIRE(validation.diagnostic.find("--renderer") != std::string::npos);
}

MK_TEST("editor native shell invalid launch exit code stays deterministic") {
    MK_REQUIRE(mirakana::editor::native_editor_launch_usage_error_exit_code() == 2);
}

MK_TEST("editor native shell no-user-config disables first-party shell persistence") {
    mirakana::editor::NativeEditorLaunchOptions options;

    const auto default_policy = mirakana::editor::make_native_editor_user_config_policy(options);
    MK_REQUIRE(default_policy.ini_file_enabled);
    MK_REQUIRE(default_policy.log_file_enabled);

    options.no_user_config = true;
    const auto smoke_policy = mirakana::editor::make_native_editor_user_config_policy(options);
    MK_REQUIRE(!smoke_policy.ini_file_enabled);
    MK_REQUIRE(!smoke_policy.log_file_enabled);
}

MK_TEST("editor native shell app exposes the core backed panel set") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};

    MK_REQUIRE(app.native_panel_count() == 11U);
    MK_REQUIRE(app.has_native_panel("main_menu"));
    MK_REQUIRE(app.has_native_panel("scene"));
    MK_REQUIRE(app.has_native_panel("inspector"));
    MK_REQUIRE(app.has_native_panel("assets"));
    MK_REQUIRE(app.has_native_panel("console"));
    MK_REQUIRE(app.has_native_panel("resources"));
    MK_REQUIRE(app.has_native_panel("ai_commands"));
    MK_REQUIRE(app.has_native_panel("profiler"));
    MK_REQUIRE(app.has_native_panel("timeline"));
    MK_REQUIRE(app.has_native_panel("project_settings"));
    MK_REQUIRE(app.has_native_panel("viewport"));
}

MK_TEST("editor first party document includes visible panel roots") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};

    const auto shell_document = mirakana::editor::make_first_party_editor_document(app);

    MK_REQUIRE(shell_document.panel_root_count == app.native_panel_count());
    MK_REQUIRE(contains_element(shell_document.document, "editor.root"));
    MK_REQUIRE(contains_element(shell_document.document, "editor.dock"));
    MK_REQUIRE(contains_element(shell_document.document, "editor.panel.main_menu"));
    MK_REQUIRE(contains_element(shell_document.document, "editor.panel.scene"));
    MK_REQUIRE(contains_element(shell_document.document, "editor.panel.inspector"));
    MK_REQUIRE(contains_element(shell_document.document, "editor.panel.assets"));
    MK_REQUIRE(contains_element(shell_document.document, "editor.panel.console"));
    MK_REQUIRE(contains_element(shell_document.document, "editor.panel.viewport"));
    MK_REQUIRE(contains_element(shell_document.document, "editor.panel.resources"));
    MK_REQUIRE(contains_element(shell_document.document, "editor.panel.ai_commands"));
    MK_REQUIRE(contains_element(shell_document.document, "editor.panel.profiler"));
    MK_REQUIRE(contains_element(shell_document.document, "editor.panel.timeline"));
    MK_REQUIRE(contains_element(shell_document.document, "editor.panel.project_settings"));
}

MK_TEST("editor first party document renders dock tab headers gutters and focused active panels") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};

    const auto shell_document = mirakana::editor::make_first_party_editor_document(app);
    const auto* left_tabs =
        shell_document.document.find(mirakana::ui::ElementId{.value = "editor.dock.dock.left_stack.tabs"});
    const auto* scene_tab =
        shell_document.document.find(mirakana::ui::ElementId{.value = "editor.dock.dock.left_stack.tab.scene"});
    const auto* viewport_tab =
        shell_document.document.find(mirakana::ui::ElementId{.value = "editor.dock.dock.viewport_stack.tab.viewport"});
    const auto* root_gutter =
        shell_document.document.find(mirakana::ui::ElementId{.value = "editor.dock.dock.root.gutter.1"});
    const auto* scene_panel = shell_document.document.find(mirakana::ui::ElementId{.value = "editor.panel.scene"});
    const auto* assets_panel = shell_document.document.find(mirakana::ui::ElementId{.value = "editor.panel.assets"});
    const auto* viewport_panel =
        shell_document.document.find(mirakana::ui::ElementId{.value = "editor.panel.viewport"});

    MK_REQUIRE(left_tabs != nullptr);
    MK_REQUIRE(left_tabs->role == mirakana::ui::SemanticRole::list);
    MK_REQUIRE(scene_tab != nullptr);
    MK_REQUIRE(scene_tab->role == mirakana::ui::SemanticRole::button);
    MK_REQUIRE(scene_tab->style.background_token == "editor.dock.tab.active");
    MK_REQUIRE(viewport_tab != nullptr);
    MK_REQUIRE(viewport_tab->style.background_token == "editor.dock.tab.focused");
    MK_REQUIRE(root_gutter != nullptr);
    MK_REQUIRE(root_gutter->style.background_token == "editor.dock.gutter.horizontal");
    MK_REQUIRE(scene_panel != nullptr);
    MK_REQUIRE(scene_panel->visible);
    MK_REQUIRE(scene_panel->style.background_token == "editor.panel.active");
    MK_REQUIRE(assets_panel != nullptr);
    MK_REQUIRE(!assets_panel->visible);
    MK_REQUIRE(viewport_panel != nullptr);
    MK_REQUIRE(viewport_panel->style.background_token == "editor.panel.focused");
    MK_REQUIRE(shell_document.focused_element.value == "editor.dock.dock.viewport_stack.tab.viewport");
    MK_REQUIRE(shell_document.tab_header_count == 11U);
    MK_REQUIRE(shell_document.split_gutter_count == 3U);
    MK_REQUIRE(shell_document.active_panel_count == 4U);
    MK_REQUIRE(shell_document.focusable_dock_control_count == 11U);
    MK_REQUIRE(shell_document.docking_status == "single_window_ready");
}

MK_TEST("editor first party document exposes keyboard focus traversal over dock tabs") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};

    const auto shell_document = mirakana::editor::make_first_party_editor_document(app);
    mirakana::ui::InteractionState interaction;

    MK_REQUIRE(interaction.set_focus(shell_document.document, shell_document.focused_element));
    MK_REQUIRE(interaction.focused().value == "editor.dock.dock.viewport_stack.tab.viewport");
    MK_REQUIRE(interaction.route_navigation(shell_document.document, mirakana::ui::NavigationDirection::next));
    MK_REQUIRE(interaction.focused().value != "editor.dock.dock.viewport_stack.tab.viewport");
    MK_REQUIRE(interaction.route_navigation(shell_document.document, mirakana::ui::NavigationDirection::previous));
    MK_REQUIRE(interaction.focused().value == "editor.dock.dock.viewport_stack.tab.viewport");
}

MK_TEST("editor first party document disables hidden dock tab commands") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    app.set_panel_visible(mirakana::editor::PanelId::resources, false);

    const auto shell_document = mirakana::editor::make_first_party_editor_document(app);
    const auto* resources_tab =
        shell_document.document.find(mirakana::ui::ElementId{.value = "editor.dock.dock.left_stack.tab.resources"});
    const auto* resources_panel =
        shell_document.document.find(mirakana::ui::ElementId{.value = "editor.panel.resources"});

    MK_REQUIRE(resources_tab != nullptr);
    MK_REQUIRE(!resources_tab->enabled);
    MK_REQUIRE(resources_tab->style.background_token == "editor.dock.tab.disabled");
    MK_REQUIRE(resources_panel == nullptr);
    MK_REQUIRE(shell_document.tab_header_count == 11U);
    MK_REQUIRE(shell_document.focusable_dock_control_count == 10U);
}

MK_TEST("editor first party document keeps stable semantic element ids") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    app.record_native_viewport_d3d12_host_ready(1U);

    const auto shell_document = mirakana::editor::make_first_party_editor_document(app);
    const auto* inspector = shell_document.document.find(mirakana::ui::ElementId{.value = "editor.panel.inspector"});
    const auto* command_label =
        shell_document.document.find(mirakana::ui::ElementId{.value = "editor.panel.ai_commands.title"});
    const auto* viewport_status =
        shell_document.document.find(mirakana::ui::ElementId{.value = "editor.panel.viewport.status"});

    MK_REQUIRE(inspector != nullptr);
    MK_REQUIRE(inspector->role == mirakana::ui::SemanticRole::panel);
    MK_REQUIRE(inspector->accessibility_label == "Inspector");
    MK_REQUIRE(command_label != nullptr);
    MK_REQUIRE(command_label->text.label == "AI Commands");
    MK_REQUIRE(viewport_status != nullptr);
    MK_REQUIRE(viewport_status->text.label.contains("diagnostic"));
}

MK_TEST("editor first party document composes console diagnostics as read only rich text") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};

    const auto shell_document = mirakana::editor::make_first_party_editor_document(app);
    const auto* rich_text_root =
        shell_document.document.find(mirakana::ui::ElementId{.value = "editor.panel.console.rich_text"});
    const auto* severity = shell_document.document.find(
        mirakana::ui::ElementId{.value = "editor.panel.console.rich_text.paragraph.native_shell.span.severity"});
    const auto* message = shell_document.document.find(
        mirakana::ui::ElementId{.value = "editor.panel.console.rich_text.paragraph.native_shell.span.message"});

    MK_REQUIRE(rich_text_root != nullptr);
    MK_REQUIRE(rich_text_root->role == mirakana::ui::SemanticRole::root);
    MK_REQUIRE(rich_text_root->parent.value == "editor.panel.console");
    MK_REQUIRE(severity != nullptr);
    MK_REQUIRE(severity->text.label == "Info: ");
    MK_REQUIRE(severity->style.foreground_token == "editor.info");
    MK_REQUIRE(message != nullptr);
    MK_REQUIRE(message->text.label == "Native editor shell ready");
    MK_REQUIRE(message->style.foreground_token == "editor.info");
    MK_REQUIRE(std::ranges::any_of(shell_document.renderer_submission.text_runs, [](const auto& run) {
        return run.id.value == "editor.panel.console.rich_text.paragraph.native_shell.span.message" &&
               run.text.label == "Native editor shell ready";
    }));
}

MK_TEST("editor first party document produces renderer submission without native handles") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};

    const auto shell_document = mirakana::editor::make_first_party_editor_document(app);

    MK_REQUIRE(!shell_document.native_handles_exposed);
    MK_REQUIRE(shell_document.renderer_submission.elements.size() < shell_document.document.size());
    MK_REQUIRE(!shell_document.renderer_submission.boxes.empty());
    MK_REQUIRE(!shell_document.renderer_submission.text_runs.empty());
    MK_REQUIRE(!shell_document.renderer_submission.accessibility_nodes.empty());
    MK_REQUIRE(shell_document.renderer_submission.image_placeholders.empty());
}

MK_TEST("editor first party shell smoke counters report imgui disabled") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    const auto shell_document = mirakana::editor::make_first_party_editor_document(app);

    const auto counters = mirakana::editor::make_first_party_editor_shell_smoke_counters(app, shell_document);

    MK_REQUIRE(counters.ui == "first_party");
    MK_REQUIRE(!counters.imgui_enabled);
    MK_REQUIRE(counters.backend == "d3d12");
    MK_REQUIRE(counters.panel_count == 11U);
    MK_REQUIRE(!counters.sdl3_enabled);
    MK_REQUIRE(!counters.viewport_native_handles_exposed);
    MK_REQUIRE(!counters.material_preview_native_handles_exposed);
    MK_REQUIRE(counters.docking_status == "single_window_ready");
    MK_REQUIRE(counters.dock_tab_header_count == 11U);
    MK_REQUIRE(counters.dock_split_gutter_count == 3U);
    MK_REQUIRE(counters.dock_active_panel_count == 4U);
    MK_REQUIRE(counters.dock_focusable_control_count == 11U);
}

MK_TEST("editor first party win32 host result defaults to explicit no backend") {
    mirakana::editor::Win32FirstPartyEditorRunResult result;

    MK_REQUIRE(!result.succeeded);
    MK_REQUIRE(result.exit_code == 1);
    MK_REQUIRE(result.frames_rendered == 0U);
    MK_REQUIRE(result.resize_count == 0U);
    MK_REQUIRE(result.adapter_kind == mirakana::editor::Win32FirstPartyEditorAdapterKind::none);
    MK_REQUIRE(result.renderer_boxes_submitted == 0U);
    MK_REQUIRE(result.renderer_text_runs_available == 0U);
    MK_REQUIRE(result.diagnostic.empty());
}

MK_TEST("first party editor document orders panels through dock graph") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};

    const auto shell_document = mirakana::editor::make_first_party_editor_document(app);
    const auto* root = shell_document.document.find(mirakana::ui::ElementId{.value = "editor.dock.dock.root"});
    const auto* left = shell_document.document.find(mirakana::ui::ElementId{.value = "editor.dock.dock.left_stack"});
    const auto* scene = shell_document.document.find(mirakana::ui::ElementId{.value = "editor.panel.scene"});
    const auto* viewport = shell_document.document.find(mirakana::ui::ElementId{.value = "editor.panel.viewport"});
    const auto* right = shell_document.document.find(mirakana::ui::ElementId{.value = "editor.dock.dock.right_stack"});
    const auto* inspector = shell_document.document.find(mirakana::ui::ElementId{.value = "editor.panel.inspector"});

    MK_REQUIRE(root != nullptr);
    MK_REQUIRE(left != nullptr);
    MK_REQUIRE(scene != nullptr);
    MK_REQUIRE(viewport != nullptr);
    MK_REQUIRE(right != nullptr);
    MK_REQUIRE(inspector != nullptr);
    MK_REQUIRE(left->parent.value == root->id.value);
    MK_REQUIRE(scene->parent.value == left->id.value);
    MK_REQUIRE(inspector->parent.value == right->id.value);
    MK_REQUIRE(shell_document.panel_root_count == app.native_panel_count());
}

MK_TEST("editor first party document consumes core dock layout and workspace visibility") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    app.set_panel_visible(mirakana::editor::PanelId::resources, false);

    const auto shell_document = mirakana::editor::make_first_party_editor_document(app);
    const auto layout = mirakana::editor::make_default_editor_dock_layout();
    const auto* root = mirakana::editor::find_editor_dock_node(layout, "dock.root");

    MK_REQUIRE(root != nullptr);
    MK_REQUIRE(contains_element(shell_document.document, "editor.panel.main_menu"));
    MK_REQUIRE(!contains_element(shell_document.document, "editor.panel.resources"));
    MK_REQUIRE(shell_document.panel_root_count == app.native_panel_count() - 1U);
}

MK_TEST("first party editor adapter boundaries stay value-only and unimplemented") {
    const auto rows = mirakana::editor::first_party_editor_required_adapter_boundaries();

    const auto* text_shaping =
        find_adapter_boundary(rows, mirakana::editor::FirstPartyEditorAdapterBoundary::text_shaping);
    const auto* font_rasterization =
        find_adapter_boundary(rows, mirakana::editor::FirstPartyEditorAdapterBoundary::font_rasterization);
    const auto* ime_text_services =
        find_adapter_boundary(rows, mirakana::editor::FirstPartyEditorAdapterBoundary::ime_text_services);
    const auto* accessibility =
        find_adapter_boundary(rows, mirakana::editor::FirstPartyEditorAdapterBoundary::accessibility_bridge);

    MK_REQUIRE(text_shaping != nullptr);
    MK_REQUIRE(font_rasterization != nullptr);
    MK_REQUIRE(ime_text_services != nullptr);
    MK_REQUIRE(accessibility != nullptr);
    MK_REQUIRE(text_shaping->official_source_family.find("DirectWrite") != std::string::npos);
    MK_REQUIRE(ime_text_services->official_source_family.find("Text Services Framework") != std::string::npos);
    MK_REQUIRE(accessibility->official_source_family.find("UI Automation") != std::string::npos);

    for (const auto& row : rows) {
        MK_REQUIRE(!row.id.empty());
        MK_REQUIRE(!row.implemented);
        MK_REQUIRE(!row.native_handles_public);
    }
}

MK_TEST("editor native shell app records deterministic panel smoke counters") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};

    MK_REQUIRE(app.panels_rendered_last_frame() == 0U);
    MK_REQUIRE(app.docking_status_last_frame() == "not_rendered");
    MK_REQUIRE(app.dock_tab_headers_last_frame() == 0U);
    MK_REQUIRE(app.dock_split_gutters_last_frame() == 0U);
    MK_REQUIRE(app.dock_active_panels_last_frame() == 0U);
    MK_REQUIRE(app.dock_focusable_controls_last_frame() == 0U);

    app.record_native_panels_rendered(11U);
    app.record_native_docking_frame("single_window_ready", 11U, 3U, 4U, 11U);

    MK_REQUIRE(app.panels_rendered_last_frame() == 11U);
    MK_REQUIRE(app.docking_status_last_frame() == "single_window_ready");
    MK_REQUIRE(app.dock_tab_headers_last_frame() == 11U);
    MK_REQUIRE(app.dock_split_gutters_last_frame() == 3U);
    MK_REQUIRE(app.dock_active_panels_last_frame() == 4U);
    MK_REQUIRE(app.dock_focusable_controls_last_frame() == 11U);
}

MK_TEST("editor native shell app updates resources panel from native host availability") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};

    MK_REQUIRE(!app.resources().device_available);
    MK_REQUIRE(app.resources().status == "No RHI device");

    app.record_native_resource_device_ready(3U);

    MK_REQUIRE(app.resources().device_available);
    MK_REQUIRE(app.resources().status == "Ready");
    const auto* const backend = find_resource_row(app.resources(), "backend");
    MK_REQUIRE(backend != nullptr);
    MK_REQUIRE(backend->available);
    MK_REQUIRE(backend->value == "Native Win32/D3D12 host");
    const auto* const frame = find_resource_row(app.resources(), "frame");
    MK_REQUIRE(frame != nullptr);
    MK_REQUIRE(frame->available);
    MK_REQUIRE(frame->value == "3");
}

MK_TEST("editor native viewport display plan rejects missing d3d12 host") {
    const auto plan = mirakana::editor::plan_native_viewport_display(mirakana::editor::NativeViewportDisplayDesc{
        .d3d12_host_available = false,
        .renderer_output_available = true,
        .extent = mirakana::editor::ViewportExtent{.width = 1280, .height = 720},
        .frame_index = 7,
    });

    MK_REQUIRE(!plan.accepted);
    MK_REQUIRE(plan.status_id == "host_unavailable");
    MK_REQUIRE(plan.diagnostic.contains("D3D12 host"));
    MK_REQUIRE(!plan.native_texture_handles_exposed);
}

MK_TEST("editor native viewport display plan records diagnostic-only viewport when renderer output is unavailable") {
    const auto plan = mirakana::editor::plan_native_viewport_display(mirakana::editor::NativeViewportDisplayDesc{
        .d3d12_host_available = true,
        .renderer_output_available = false,
        .extent = mirakana::editor::ViewportExtent{.width = 1280, .height = 720},
        .frame_index = 8,
    });

    MK_REQUIRE(plan.accepted);
    MK_REQUIRE(plan.status_id == "diagnostic_only");
    MK_REQUIRE(plan.extent.width == 1280U);
    MK_REQUIRE(plan.frame_index == 8U);
    MK_REQUIRE(plan.diagnostic.contains("renderer output"));
    MK_REQUIRE(!plan.texture_display_ready);
}

MK_TEST("editor native viewport display plan does not expose native texture handles") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};

    app.record_native_viewport_d3d12_host_ready(9U);

    MK_REQUIRE(app.viewport_display().status_id == "diagnostic_only");
    MK_REQUIRE(!app.viewport_display().native_texture_handles_exposed);
    MK_REQUIRE(app.viewport_display().native_texture_handle_policy == "private");
    MK_REQUIRE(app.viewport().renderer_name() == "d3d12");
}

MK_TEST("editor native material preview plan rejects missing shader artifacts") {
    const auto plan =
        mirakana::editor::plan_native_material_preview_display(mirakana::editor::NativeMaterialPreviewDisplayDesc{
            .d3d12_host_available = true,
            .shader_artifacts_available = false,
            .gpu_payload_available = true,
            .frame_index = 12,
        });

    MK_REQUIRE(!plan.accepted);
    MK_REQUIRE(plan.status_id == "shader_artifacts_missing");
    MK_REQUIRE(plan.diagnostic.contains("shader artifacts"));
    MK_REQUIRE(!plan.texture_display_ready);
    MK_REQUIRE(!plan.native_texture_handles_exposed);
    MK_REQUIRE(plan.native_texture_handle_policy == "private");
    MK_REQUIRE(plan.execution_snapshot.status == mirakana::editor::EditorMaterialGpuPreviewStatus::rhi_unavailable);
    MK_REQUIRE(!plan.execution_snapshot.executes);
    MK_REQUIRE(!plan.execution_snapshot.exposes_native_handles);
}

MK_TEST("editor native material preview plan reports diagnostic-only preview without gpu display") {
    const auto plan =
        mirakana::editor::plan_native_material_preview_display(mirakana::editor::NativeMaterialPreviewDisplayDesc{
            .d3d12_host_available = true,
            .shader_artifacts_available = true,
            .gpu_payload_available = true,
            .frame_index = 13,
        });

    MK_REQUIRE(plan.accepted);
    MK_REQUIRE(plan.status_id == "diagnostic_only");
    MK_REQUIRE(!plan.texture_display_ready);
    MK_REQUIRE(plan.execution_snapshot.backend_label == "D3D12");
    MK_REQUIRE(plan.execution_snapshot.display_path_label == "host-private-native");
    MK_REQUIRE(plan.execution_snapshot.frames_rendered == 0U);
    MK_REQUIRE(plan.execution_snapshot.status == mirakana::editor::EditorMaterialGpuPreviewStatus::rhi_unavailable);
    MK_REQUIRE(plan.diagnostic.contains("diagnostic-only"));
}

MK_TEST("editor native material preview plan keeps d3d12 handles private") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};

    app.record_native_material_preview_d3d12_host_ready(14U);

    MK_REQUIRE(app.material_preview_display().status_id == "diagnostic_only");
    MK_REQUIRE(!app.material_preview_display().texture_display_ready);
    MK_REQUIRE(!app.material_preview_display().native_texture_handles_exposed);
    MK_REQUIRE(app.material_preview_display().native_texture_handle_policy == "private");
    MK_REQUIRE(!app.material_preview_display().execution_snapshot.executes);
    MK_REQUIRE(!app.material_preview_display().execution_snapshot.exposes_native_handles);
    MK_REQUIRE(app.material_preview().gpu_execution_display_path_label == "host-private-native");
    MK_REQUIRE(!app.material_preview().gpu_execution_ready);
    MK_REQUIRE(!app.material_preview().gpu_execution_rendered);
    MK_REQUIRE(!app.material_preview().executes);
    MK_REQUIRE(!app.material_preview().exposes_native_handles);
}

MK_TEST("editor native shell routes file dialog requests through bound service") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    mirakana::MemoryFileDialogService file_dialogs;
    file_dialogs.enqueue_response(mirakana::MemoryFileDialogResponse{
        .status = mirakana::FileDialogStatus::accepted,
        .paths = {"games/sample/GameEngine.geproject"},
        .selected_filter = 0,
        .error = {},
    });

    app.bind_native_services(mirakana::editor::NativeEditorServiceBindings{
        .file_dialog_service = &file_dialogs,
        .file_dialog_service_id = "win32",
    });

    const auto request = mirakana::editor::make_project_open_dialog_request(".");
    const auto id = app.show_file_dialog(request);
    const auto result = app.poll_file_dialog_result(id);

    MK_REQUIRE(id != 0U);
    MK_REQUIRE(result.has_value());
    MK_REQUIRE(result->status == mirakana::FileDialogStatus::accepted);
    MK_REQUIRE(result->paths.size() == 1U);
    MK_REQUIRE(result->paths[0] == "games/sample/GameEngine.geproject");
    MK_REQUIRE(file_dialogs.last_request().has_value());
    MK_REQUIRE(file_dialogs.last_request()->kind == mirakana::FileDialogKind::open_file);
    MK_REQUIRE(app.services().file_dialog_service_id == "win32");
    MK_REQUIRE(app.services().file_dialog_requests_routed == 1U);
}

MK_TEST("editor native shell routes clipboard text through bound adapter") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    RecordingClipboardTextAdapter clipboard;

    app.bind_native_services(mirakana::editor::NativeEditorServiceBindings{
        .clipboard_text_adapter = &clipboard,
        .clipboard_service_id = "win32",
    });

    const auto write = app.write_clipboard_text(mirakana::ui::ClipboardTextWriteRequest{
        .target = mirakana::ui::ElementId{.value = "project_settings.name"},
        .text = "MIRAIKANAI",
    });
    const auto read = app.read_clipboard_text(mirakana::ui::ClipboardTextReadRequest{
        .target = mirakana::ui::ElementId{.value = "project_settings.name"},
    });

    MK_REQUIRE(write.succeeded());
    MK_REQUIRE(read.succeeded());
    MK_REQUIRE(read.has_text);
    MK_REQUIRE(read.text == "MIRAIKANAI");
    MK_REQUIRE(app.services().clipboard_service_id == "win32");
    MK_REQUIRE(app.services().clipboard_operations_routed == 2U);
}

MK_TEST("editor native shell reviewed process execution requires confirmation") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    mirakana::RecordingProcessRunner runner;

    app.bind_native_services(mirakana::editor::NativeEditorServiceBindings{
        .reviewed_process_runner = &runner,
        .reviewed_process_runner_id = "win32",
    });

    const auto plan = app.reviewed_validation_execution_plan(make_reviewed_validation_execution_desc());
    MK_REQUIRE(plan.can_execute);

    const auto blocked = app.run_reviewed_process(mirakana::editor::NativeEditorReviewedProcessRequest{
        .plan = plan,
        .user_confirmed = false,
    });
    MK_REQUIRE(!blocked.executed);
    MK_REQUIRE(!blocked.process.launched);
    MK_REQUIRE(blocked.diagnostic.contains("confirmation"));
    MK_REQUIRE(runner.commands().empty());

    const auto executed = app.run_reviewed_process(mirakana::editor::NativeEditorReviewedProcessRequest{
        .plan = plan,
        .user_confirmed = true,
    });
    MK_REQUIRE(executed.executed);
    MK_REQUIRE(executed.process.succeeded());
    MK_REQUIRE(runner.commands().size() == 1U);
    MK_REQUIRE(runner.commands()[0].executable == "pwsh");
    MK_REQUIRE(app.services().reviewed_process_runner_id == "win32");
    MK_REQUIRE(app.services().reviewed_process_plans == 1U);
    MK_REQUIRE(app.services().reviewed_process_executions == 1U);
}

MK_TEST("editor native shell service status defaults stay deterministic") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};

    MK_REQUIRE(app.services().file_dialog_service_id == "memory");
    MK_REQUIRE(app.services().clipboard_service_id == "memory");
    MK_REQUIRE(app.services().reviewed_process_runner_id == "recording");
    MK_REQUIRE(app.services().user_confirmation_required_for_process_execution);
}

int main() {
    return mirakana::test::run_all();
}
