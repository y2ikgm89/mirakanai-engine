// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "first_party_editor_adapter_boundaries.hpp"
#include "first_party_editor_document.hpp"
#include "native_asset_import_copy.hpp"
#include "native_editor_app.hpp"
#include "native_editor_launch.hpp"
#include "native_editor_text_atlas_handoff.hpp"
#include "native_editor_text_input.hpp"
#include "native_editor_uia_provider.hpp"
#include "native_editor_visible_texture_compositor.hpp"
#if defined(_WIN32)
#include "native_editor_text_font_adapters.hpp"
#include "native_editor_tsf_text_input.hpp"
#endif
#include "native_material_preview_cache.hpp"
#include "native_texture_display_adapter.hpp"
#include "native_viewport_surface.hpp"
#include "win32_first_party_editor_host.hpp"

#include "mirakana/assets/asset_source_format.hpp"
#include "mirakana/assets/source_asset_registry.hpp"
#include "mirakana/editor/ai_operation_surface.hpp"
#include "mirakana/editor/editor_dock_layout.hpp"
#include "mirakana/editor/environment_authoring.hpp"
#include "mirakana/platform/file_dialog.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/platform/process.hpp"
#include "mirakana/rhi/rhi.hpp"
#include "mirakana/ui/ui.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
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

[[nodiscard]] const mirakana::editor::EditorAssetBrowserSourcePulseRow*
find_asset_browser_row_by_key(const mirakana::editor::EditorAssetBrowserProductionModel& model,
                              std::string_view asset_key_label) noexcept {
    const auto it = std::ranges::find_if(
        model.rows, [asset_key_label](const auto& row) { return row.asset_key_label == asset_key_label; });
    return it == model.rows.end() ? nullptr : &(*it);
}

[[nodiscard]] const mirakana::SourceAssetRegistryRowV1*
find_source_registry_row_by_key(const mirakana::SourceAssetRegistryDocumentV1& document,
                                std::string_view asset_key_label) noexcept {
    const auto it = std::ranges::find_if(
        document.assets, [asset_key_label](const auto& row) { return row.key.value == asset_key_label; });
    return it == document.assets.end() ? nullptr : &(*it);
}

[[nodiscard]] mirakana::editor::EditorAssetBrowserLegalProvenanceRow
make_native_asset_import_test_provenance(std::string asset_key_label) {
    return mirakana::editor::EditorAssetBrowserLegalProvenanceRow{
        .id = "native_asset_import.legal." + asset_key_label,
        .asset_key_label = asset_key_label,
        .source_url = "project://assets/imported_sources",
        .retrieved_date = "2026-06-29",
        .version_or_commit = "working-tree",
        .copyright_holder = "MIRAIKANAI contributors",
        .license_id = "LicenseRef-Proprietary",
        .modification_status = "unmodified",
        .distribution_target = "runtime_package",
        .notice_complete = true,
    };
}

void write_native_asset_import_texture_source(mirakana::IFileSystem& filesystem, std::string_view path) {
    filesystem.write_text(path, mirakana::serialize_texture_source_document(mirakana::TextureSourceDocument{
                                    .width = 1,
                                    .height = 1,
                                    .pixel_format = mirakana::TextureSourcePixelFormat::rgba8_unorm,
                                    .bytes = std::vector<std::uint8_t>{255, 64, 16, 255},
                                }));
}

[[nodiscard]] mirakana::AssetHotReloadRecookRequest
make_native_texture_recook_request(const mirakana::AssetKeyV2& key, std::uint64_t previous_revision = 10,
                                   std::uint64_t current_revision = 11) {
    const auto asset = mirakana::asset_id_from_key_v2(key);
    return mirakana::AssetHotReloadRecookRequest{
        .asset = asset,
        .source_asset = asset,
        .trigger_path = "source/textures/editor_preview.texture",
        .trigger_event_kind = mirakana::AssetHotReloadEventKind::modified,
        .reason = mirakana::AssetHotReloadRecookReason::source_modified,
        .previous_revision = previous_revision,
        .current_revision = current_revision,
        .ready_tick = 20,
    };
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

class RecordingPlatformTextInputAdapter final : public mirakana::ui::IPlatformIntegrationAdapter {
  public:
    void begin_text_input(const mirakana::ui::PlatformTextInputRequest& request) override {
        begin_requests.push_back(request);
    }

    void end_text_input(const mirakana::ui::ElementId& target) override {
        ended_targets.push_back(target);
    }

    std::vector<mirakana::ui::PlatformTextInputRequest> begin_requests;
    std::vector<mirakana::ui::ElementId> ended_targets;
};

class RecordingImeAdapter final : public mirakana::ui::IImeAdapter {
  public:
    void update_composition(const mirakana::ui::ImeComposition& composition) override {
        updates.push_back(composition);
    }

    std::vector<mirakana::ui::ImeComposition> updates;
};

class ThrowingUnsafePathFileSystem final : public mirakana::IFileSystem {
  public:
    [[nodiscard]] bool exists(std::string_view path) const override {
        throw_if_unsafe(path);
        return files.exists(path);
    }

    [[nodiscard]] bool is_directory(std::string_view path) const override {
        throw_if_unsafe(path);
        return files.is_directory(path);
    }

    [[nodiscard]] std::string read_text(std::string_view path) const override {
        throw_if_unsafe(path);
        return files.read_text(path);
    }

    [[nodiscard]] std::vector<std::string> list_files(std::string_view root) const override {
        throw_if_unsafe(root);
        return files.list_files(root);
    }

    void write_text(std::string_view path, std::string_view text) override {
        throw_if_unsafe(path);
        files.write_text(path, text);
    }

    void remove(std::string_view path) override {
        throw_if_unsafe(path);
        files.remove(path);
    }

    void remove_empty_directory(std::string_view path) override {
        throw_if_unsafe(path);
        files.remove_empty_directory(path);
    }

    mirakana::MemoryFileSystem files;
    mutable std::uint32_t unsafe_query_count{0};

  private:
    void throw_if_unsafe(std::string_view path) const {
        if (path.find("..") != std::string_view::npos || path.find(':') != std::string_view::npos) {
            ++unsafe_query_count;
            throw std::invalid_argument("unsafe rooted path");
        }
    }
};

class ScopedCurrentPath final {
  public:
    explicit ScopedCurrentPath(std::filesystem::path next_path) : previous_path_(std::filesystem::current_path()) {
        std::filesystem::current_path(next_path);
    }

    ~ScopedCurrentPath() {
        std::error_code error;
        std::filesystem::current_path(previous_path_, error);
    }

    ScopedCurrentPath(const ScopedCurrentPath&) = delete;
    ScopedCurrentPath& operator=(const ScopedCurrentPath&) = delete;
    ScopedCurrentPath(ScopedCurrentPath&&) = delete;
    ScopedCurrentPath& operator=(ScopedCurrentPath&&) = delete;

  private:
    std::filesystem::path previous_path_;
};

void write_test_file(const std::filesystem::path& path, std::string_view content) {
    if (const auto parent = path.parent_path(); !parent.empty()) {
        std::filesystem::create_directories(parent);
    }
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(content.data(), static_cast<std::streamsize>(content.size()));
}

[[nodiscard]] bool directory_has_copying_file(const std::filesystem::path& directory) {
    if (!std::filesystem::exists(directory)) {
        return false;
    }
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.path().filename().generic_string().contains(".copying-")) {
            return true;
        }
    }
    return false;
}

#if defined(MK_EDITOR_ASSET_IMPORT_COPY_TEST_HOOKS)
class ScopedExternalCopyFinalizationFailure final {
  public:
    explicit ScopedExternalCopyFinalizationFailure(std::size_t count) {
        mirakana::editor::set_native_asset_import_external_copy_fail_after_finalized_count_for_tests(count);
    }

    ~ScopedExternalCopyFinalizationFailure() {
        mirakana::editor::set_native_asset_import_external_copy_fail_after_finalized_count_for_tests(0U);
    }

    ScopedExternalCopyFinalizationFailure(const ScopedExternalCopyFinalizationFailure&) = delete;
    ScopedExternalCopyFinalizationFailure& operator=(const ScopedExternalCopyFinalizationFailure&) = delete;
    ScopedExternalCopyFinalizationFailure(ScopedExternalCopyFinalizationFailure&&) = delete;
    ScopedExternalCopyFinalizationFailure& operator=(ScopedExternalCopyFinalizationFailure&&) = delete;
};
#endif

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

[[nodiscard]] const mirakana::editor::NativeEditorUiaNode*
find_uia_node(const mirakana::editor::NativeEditorUiaProviderState& state, std::string_view id) noexcept {
    for (const auto& node : state.nodes) {
        if (node.id.value == id) {
            return &node;
        }
    }
    return nullptr;
}

[[nodiscard]] const mirakana::editor::EditorAiOperationStatusRow*
find_ai_operation_status_row(const mirakana::editor::EditorAiOperationSnapshot& snapshot,
                             std::string_view id) noexcept {
    for (const auto& row : snapshot.status_rows) {
        if (row.id == id) {
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

    MK_REQUIRE(app.native_panel_count() == 12U);
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
    MK_REQUIRE(app.has_native_panel("runtime_ui_editor"));
    MK_REQUIRE(app.has_native_panel("viewport"));

    const auto& runtime_ui = app.runtime_ui_authoring();
    MK_REQUIRE(runtime_ui.ready);
    MK_REQUIRE(runtime_ui.hierarchy_rows.size() >= 2U);
    MK_REQUIRE(runtime_ui.inspector_rows.size() >= 3U);
    MK_REQUIRE(!runtime_ui.native_handles_exposed);
    MK_REQUIRE(!runtime_ui.renderer_execution_requested);
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
    MK_REQUIRE(contains_element(shell_document.document, "editor.panel.runtime_ui_editor"));
}

MK_TEST("editor native shell exposes Source Pulse asset browser model") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};

    const auto& asset_browser = app.asset_browser();
    const auto command_plans = app.asset_browser_command_plans();

    MK_REQUIRE(asset_browser.status == mirakana::editor::EditorAssetBrowserProductionStatus::ready);
    MK_REQUIRE(asset_browser.rows.size() >= 3U);
    MK_REQUIRE(asset_browser.visible_row_count == asset_browser.rows.size());
    MK_REQUIRE(!asset_browser.mutates);
    MK_REQUIRE(!asset_browser.executes);
    MK_REQUIRE(!asset_browser.exposes_native_handles);
    MK_REQUIRE(command_plans.size() == 11U);
    for (const auto& command : command_plans) {
        MK_REQUIRE(command.current_generation == asset_browser.generation);
        MK_REQUIRE(!command.executes_package_scripts);
        MK_REQUIRE(!command.executes_validation_recipes);
        MK_REQUIRE(!command.exposes_native_handles);
    }
}

MK_TEST("native asset browser import plan includes texture mesh audio material and scene rows") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};

    const auto& asset_browser = app.asset_browser();

    struct ExpectedImportRow {
        std::string_view asset_key_label;
        mirakana::AssetKind kind;
    };
    const std::array expected_rows{
        ExpectedImportRow{.asset_key_label = "assets/textures/editor_preview", .kind = mirakana::AssetKind::texture},
        ExpectedImportRow{.asset_key_label = "assets/meshes/editor_preview", .kind = mirakana::AssetKind::mesh},
        ExpectedImportRow{.asset_key_label = "assets/audio/editor_preview", .kind = mirakana::AssetKind::audio},
        ExpectedImportRow{.asset_key_label = "assets/materials/default", .kind = mirakana::AssetKind::material},
        ExpectedImportRow{.asset_key_label = "assets/scenes/start", .kind = mirakana::AssetKind::scene},
    };

    for (const auto& expected : expected_rows) {
        const auto* row = find_asset_browser_row_by_key(asset_browser, expected.asset_key_label);
        MK_REQUIRE(row != nullptr);
        MK_REQUIRE(row->kind == expected.kind);
        MK_REQUIRE(row->source_visible);
        MK_REQUIRE(row->identity_backed);
        MK_REQUIRE(row->import_status_label == "planned");
    }
}

MK_TEST("editor first party document renders Source Pulse assets instead of legacy hard coded rows") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};

    const auto shell_document = mirakana::editor::make_first_party_editor_document(app);
    const auto counters = mirakana::editor::make_first_party_editor_shell_smoke_counters(app, shell_document);
    const auto& asset_browser = app.asset_browser();

    MK_REQUIRE(contains_element(shell_document.document, "editor.panel.assets"));
    MK_REQUIRE(contains_element(shell_document.document, "asset_browser"));
    MK_REQUIRE(contains_element(shell_document.document, "asset_browser.source_pulse"));
    MK_REQUIRE(!asset_browser.rows.empty());
    MK_REQUIRE(contains_element(shell_document.document, asset_browser.rows.front().row_id));
    MK_REQUIRE(!contains_element(shell_document.document, "assets.scene_start"));
    MK_REQUIRE(!contains_element(shell_document.document, "assets.material_default"));
    MK_REQUIRE(!contains_element(shell_document.document, "assets.shader_editor"));
    MK_REQUIRE(counters.editor_asset_browser_visible);
    MK_REQUIRE(counters.editor_asset_browser_source_pulse_rows == asset_browser.rows.size());
    MK_REQUIRE(counters.editor_asset_browser_hardcoded_rows == 0U);
    MK_REQUIRE(!counters.editor_asset_browser_native_handles_exposed);
}

MK_TEST("editor native UIA provider publishes asset browser retained rows") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    auto adapter = mirakana::editor::make_native_editor_uia_accessibility_adapter();

    app.bind_native_services(mirakana::editor::NativeEditorServiceBindings{
        .accessibility_adapter = adapter.get(),
        .accessibility_service_id = "win32_uia",
    });

    const auto shell_document = mirakana::editor::make_first_party_editor_document(app);
    const auto& asset_browser = app.asset_browser();
    const auto asset_document = mirakana::editor::make_editor_asset_browser_production_ui_model(asset_browser);
    const auto layout =
        mirakana::ui::solve_layout(asset_document, mirakana::ui::ElementId{"asset_browser"},
                                   mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 960.0F, .height = 640.0F});
    const auto submission = mirakana::ui::build_renderer_submission(asset_document, layout);

    MK_REQUIRE(contains_element(shell_document.document, "asset_browser.query"));
    MK_REQUIRE(contains_element(shell_document.document, "asset_browser.commands"));
    MK_REQUIRE(contains_element(shell_document.document, "asset_browser.preview"));
    MK_REQUIRE(!asset_browser.rows.empty());
    MK_REQUIRE(!asset_browser.preview_rows.empty());
    MK_REQUIRE(!asset_browser.command_rows.empty());

    const auto command_id = "asset_browser.commands." + asset_browser.command_rows.front().command_id;
    MK_REQUIRE(contains_element(shell_document.document, command_id));
    MK_REQUIRE(contains_element(shell_document.document, asset_browser.preview_rows.front().id + ".status"));

    const auto payload = mirakana::ui::build_accessibility_payload(submission);
    MK_REQUIRE(
        std::ranges::any_of(payload.nodes, [](const auto& node) { return node.id.value == "asset_browser.query"; }));
    const auto result = app.publish_native_accessibility_payload(payload, shell_document.focused_element);
    MK_REQUIRE(result.published);

    const auto* query = find_uia_node(app.accessibility_state(), "asset_browser.query");
    const auto* source_list = find_uia_node(app.accessibility_state(), "asset_browser.source_pulse");
    const auto* source_item = find_uia_node(app.accessibility_state(), asset_browser.rows.front().row_id);
    const auto* command = find_uia_node(app.accessibility_state(), command_id);
    const auto* status = find_uia_node(app.accessibility_state(), "asset_browser.status");
    const auto* preview_status =
        find_uia_node(app.accessibility_state(), asset_browser.preview_rows.front().id + ".status");

    MK_REQUIRE(query != nullptr);
    MK_REQUIRE(query->control_type_id == "UIA_EditControlTypeId");
    MK_REQUIRE(source_list != nullptr);
    MK_REQUIRE(source_list->control_type_id == "UIA_ListControlTypeId");
    MK_REQUIRE(source_item != nullptr);
    MK_REQUIRE(source_item->control_type_id == "UIA_ListItemControlTypeId");
    MK_REQUIRE(command != nullptr);
    MK_REQUIRE(command->control_type_id == "UIA_ButtonControlTypeId");
    MK_REQUIRE(status != nullptr);
    MK_REQUIRE(status->control_type_id == "UIA_TextControlTypeId");
    MK_REQUIRE(preview_status != nullptr);
    MK_REQUIRE(preview_status->control_type_id == "UIA_TextControlTypeId");
    MK_REQUIRE(app.accessibility_state().hidden_nodes == 0U);
    MK_REQUIRE(app.accessibility_state().unsupported_pattern_diagnostics == 0U);
    MK_REQUIRE(!app.accessibility_state().native_handles_exposed);
}

MK_TEST("editor asset browser import source dialog routes through native shell service") {
    mirakana::MemoryFileDialogService file_dialogs;
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    app.bind_native_services(mirakana::editor::NativeEditorServiceBindings{
        .file_dialog_service = &file_dialogs,
        .file_dialog_service_id = "memory_import_dialog",
    });

    const auto root = std::filesystem::current_path().lexically_normal();
    const auto selected = (root / "assets/source/hero.texture").lexically_normal().generic_string();
    file_dialogs.enqueue_response(mirakana::MemoryFileDialogResponse{
        .status = mirakana::FileDialogStatus::accepted,
        .paths = {selected},
        .selected_filter = 0,
    });

    const auto id = app.show_asset_browser_import_sources_dialog();
    const auto* request = file_dialogs.last_request() ? std::addressof(*file_dialogs.last_request()) : nullptr;
    MK_REQUIRE(id != 0U);
    MK_REQUIRE(request != nullptr);
    MK_REQUIRE(request->title == "Import Assets");
    MK_REQUIRE(request->allow_many);
    MK_REQUIRE(app.services().file_dialog_requests_routed == 1U);

    const auto review = app.poll_asset_browser_import_sources_dialog(id);
    MK_REQUIRE(review.dialog.accepted);
    MK_REQUIRE(review.accepted_project_paths.size() == 1U);
    MK_REQUIRE(review.accepted_project_paths[0] == "assets/source/hero.texture");
    MK_REQUIRE(review.diagnostics.empty());
}

MK_TEST("editor asset browser import source dialog rejects unsafe project paths") {
    mirakana::MemoryFileDialogService file_dialogs;
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    app.bind_native_services(mirakana::editor::NativeEditorServiceBindings{.file_dialog_service = &file_dialogs});
    const auto root = std::filesystem::current_path().lexically_normal();

    file_dialogs.enqueue_response(mirakana::MemoryFileDialogResponse{
        .status = mirakana::FileDialogStatus::accepted,
        .paths = {(root.parent_path() / "outside/hero.texture").lexically_normal().generic_string()},
        .selected_filter = 0,
    });
    const auto outside = app.poll_asset_browser_import_sources_dialog(app.show_asset_browser_import_sources_dialog());
    MK_REQUIRE(!outside.dialog.accepted);
    MK_REQUIRE(!outside.diagnostics.empty());
    MK_REQUIRE(outside.diagnostics[0].contains("inside the project root"));

    file_dialogs.enqueue_response(mirakana::MemoryFileDialogResponse{
        .status = mirakana::FileDialogStatus::accepted,
        .paths = {"assets/source/hero.ogg"},
        .selected_filter = 0,
    });
    const auto unsupported =
        app.poll_asset_browser_import_sources_dialog(app.show_asset_browser_import_sources_dialog());
    MK_REQUIRE(!unsupported.dialog.accepted);
    MK_REQUIRE(!unsupported.diagnostics.empty());
    MK_REQUIRE(unsupported.diagnostics[0].contains("supported import source"));

    file_dialogs.enqueue_response(mirakana::MemoryFileDialogResponse{
        .status = mirakana::FileDialogStatus::accepted,
        .paths = {"assets/source/bad\nname.texture"},
        .selected_filter = 0,
    });
    const auto invalid = app.poll_asset_browser_import_sources_dialog(app.show_asset_browser_import_sources_dialog());
    MK_REQUIRE(!invalid.dialog.accepted);
    MK_REQUIRE(!invalid.diagnostics.empty());
    MK_REQUIRE(invalid.diagnostics[0].contains("invalid characters"));
}

MK_TEST("editor asset browser external source copy review targets imported sources only") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    const auto outside_source = "C:/drop/hero.png";

    const auto ready = app.review_asset_browser_external_source_copy(
        mirakana::editor::NativeEditorAssetBrowserExternalSourceCopyRequest{
            .source_paths = {outside_source},
            .existing_source_paths = {outside_source},
        });
    MK_REQUIRE(ready.copy.can_copy);
    MK_REQUIRE(ready.copy.target_project_paths.size() == 1U);
    MK_REQUIRE(ready.copy.target_project_paths[0] == "assets/imported_sources/hero.png");

    const auto existing = app.review_asset_browser_external_source_copy(
        mirakana::editor::NativeEditorAssetBrowserExternalSourceCopyRequest{
            .source_paths = {outside_source},
            .existing_source_paths = {outside_source},
            .existing_project_paths = {"assets/imported_sources/hero.png"},
        });
    MK_REQUIRE(existing.copy.blocked);
    MK_REQUIRE(!existing.copy.diagnostics.empty());
    MK_REQUIRE(existing.copy.diagnostics[0].contains("already exists"));

    const auto unsupported = app.review_asset_browser_external_source_copy(
        mirakana::editor::NativeEditorAssetBrowserExternalSourceCopyRequest{
            .source_paths = {"C:/drop/theme.ogg"},
            .existing_source_paths = {"C:/drop/theme.ogg"},
        });
    MK_REQUIRE(unsupported.copy.blocked);
    MK_REQUIRE(!unsupported.copy.diagnostics.empty());
    MK_REQUIRE(unsupported.copy.diagnostics[0].contains("supported import source"));

    const auto device = app.review_asset_browser_external_source_copy(
        mirakana::editor::NativeEditorAssetBrowserExternalSourceCopyRequest{
            .source_paths = {"//./C:/drop/hero.png"},
            .existing_source_paths = {"//./C:/drop/hero.png"},
        });
    MK_REQUIRE(device.copy.blocked);
    MK_REQUIRE(!device.copy.diagnostics.empty());
    MK_REQUIRE(device.copy.diagnostics[0].contains("device path"));
}

MK_TEST("native asset browser copies reviewed external source through temp target") {
    const auto root = std::filesystem::temp_directory_path() / "MK_native_asset_browser_copy_success";
    const auto project_root = root / "project";
    const auto external_root = root / "external";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(project_root);
    const auto source_path = external_root / "hero.png";
    write_test_file(source_path, "png source placeholder");

    {
        const ScopedCurrentPath current_path{project_root};
        mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
        mirakana::RootedFileSystem project_filesystem{project_root};
        project_filesystem.write_text(
            app.project().source_registry_path,
            mirakana::serialize_source_asset_registry_document(mirakana::SourceAssetRegistryDocumentV1{}));
        app.bind_native_services(mirakana::editor::NativeEditorServiceBindings{
            .asset_import_filesystem = &project_filesystem,
            .asset_import_filesystem_id = "rooted_project_test",
        });

        const auto initial_generation = app.asset_browser().generation;
        const auto result = app.copy_reviewed_asset_browser_external_sources(
            mirakana::editor::NativeEditorAssetBrowserExternalSourceCopyExecutionRequest{
                .expected_generation = initial_generation,
                .absolute_source_paths = {source_path.string()},
                .provenance_rows = {make_native_asset_import_test_provenance("assets/imported/hero")},
                .user_confirmed = true,
            });

        MK_REQUIRE(result.command.command_id == "asset_browser.import.copy_external_sources");
        MK_REQUIRE(result.command.mutates_project_files);
        MK_REQUIRE(!result.command.executes_import_tools);
        MK_REQUIRE(result.succeeded);
        MK_REQUIRE(result.copied_count == 1U);
        MK_REQUIRE(result.target_project_paths.size() == 1U);
        MK_REQUIRE(result.target_project_paths[0] == "assets/imported_sources/hero.png");
        MK_REQUIRE(std::filesystem::exists(project_root / "assets" / "imported_sources" / "hero.png"));
        MK_REQUIRE(!directory_has_copying_file(project_root / "assets" / "imported_sources"));
        MK_REQUIRE(result.source_registration.applied);
        MK_REQUIRE(result.source_registration.registered_count == 1U);
        MK_REQUIRE(app.asset_browser().generation == initial_generation + 1U);

        const auto updated_registry = mirakana::deserialize_source_asset_registry_document(
            project_filesystem.read_text(app.project().source_registry_path));
        const auto* registry_row = find_source_registry_row_by_key(updated_registry, "assets/imported/hero");
        MK_REQUIRE(registry_row != nullptr);
        MK_REQUIRE(registry_row->source_path == "assets/imported_sources/hero.png");
        MK_REQUIRE(registry_row->imported_path == "assets/imported/hero.texture");
    }

    std::filesystem::remove_all(root);
}

MK_TEST("native asset browser copy rejects symlink device traversal and collisions") {
    const auto root = std::filesystem::temp_directory_path() / "MK_native_asset_browser_copy_rejects";
    const auto project_root = root / "project";
    const auto external_root = root / "external";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(project_root / "assets" / "imported_sources");
    const auto source_path = external_root / "hero.png";
    write_test_file(source_path, "new source");
    write_test_file(project_root / "assets" / "imported_sources" / "hero.png", "existing source");

    const std::array collision_inputs{mirakana::editor::NativeAssetImportExternalCopyInput{
        .absolute_source_path = source_path.string(),
        .target_project_path = "assets/imported_sources/hero.png",
    }};
    const auto collision =
        mirakana::editor::copy_reviewed_external_asset_sources_to_project(project_root.string(), collision_inputs);
    MK_REQUIRE(!collision.succeeded);
    MK_REQUIRE(!collision.diagnostics.empty());
    MK_REQUIRE(collision.diagnostics[0].contains("already exists"));
    MK_REQUIRE(!directory_has_copying_file(project_root / "assets" / "imported_sources"));

    const std::array traversal_inputs{mirakana::editor::NativeAssetImportExternalCopyInput{
        .absolute_source_path = source_path.string(),
        .target_project_path = "../outside/hero.png",
    }};
    const auto traversal =
        mirakana::editor::copy_reviewed_external_asset_sources_to_project(project_root.string(), traversal_inputs);
    MK_REQUIRE(!traversal.succeeded);
    MK_REQUIRE(!traversal.diagnostics.empty());
    MK_REQUIRE(traversal.diagnostics[0].contains("project-relative"));
    MK_REQUIRE(!std::filesystem::exists(root / "outside" / "hero.png"));

    const std::array device_inputs{mirakana::editor::NativeAssetImportExternalCopyInput{
        .absolute_source_path = "//./C:/drop/hero.png",
        .target_project_path = "assets/imported_sources/device.png",
    }};
    const auto device =
        mirakana::editor::copy_reviewed_external_asset_sources_to_project(project_root.string(), device_inputs);
    MK_REQUIRE(!device.succeeded);
    MK_REQUIRE(!device.diagnostics.empty());
    MK_REQUIRE(device.diagnostics[0].contains("device path"));

    std::error_code symlink_error;
    const auto symlink_path = external_root / "hero-link.png";
    std::filesystem::create_symlink(source_path, symlink_path, symlink_error);
    if (!symlink_error) {
        const std::array symlink_inputs{mirakana::editor::NativeAssetImportExternalCopyInput{
            .absolute_source_path = symlink_path.string(),
            .target_project_path = "assets/imported_sources/hero-link.png",
        }};
        const auto symlink =
            mirakana::editor::copy_reviewed_external_asset_sources_to_project(project_root.string(), symlink_inputs);
        MK_REQUIRE(!symlink.succeeded);
        MK_REQUIRE(!symlink.diagnostics.empty());
        MK_REQUIRE(symlink.diagnostics[0].contains("symlink"));
    }

    std::filesystem::remove_all(root);
}

#if defined(MK_EDITOR_ASSET_IMPORT_COPY_TEST_HOOKS)
MK_TEST("native asset browser copy rolls back finalized targets after finalization failure") {
    const auto root = std::filesystem::temp_directory_path() / "MK_native_asset_browser_copy_rollback";
    const auto project_root = root / "project";
    const auto external_root = root / "external";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(project_root);
    const auto hero_source = external_root / "hero.png";
    const auto prop_source = external_root / "prop.png";
    write_test_file(hero_source, "hero source");
    write_test_file(prop_source, "prop source");

    const std::array inputs{
        mirakana::editor::NativeAssetImportExternalCopyInput{
            .absolute_source_path = hero_source.string(),
            .target_project_path = "assets/imported_sources/hero.png",
        },
        mirakana::editor::NativeAssetImportExternalCopyInput{
            .absolute_source_path = prop_source.string(),
            .target_project_path = "assets/imported_sources/prop.png",
        },
    };

    {
        const ScopedExternalCopyFinalizationFailure fail_after_first_finalized{1U};
        const auto result =
            mirakana::editor::copy_reviewed_external_asset_sources_to_project(project_root.string(), inputs);
        MK_REQUIRE(!result.succeeded);
        MK_REQUIRE(!result.diagnostics.empty());
        MK_REQUIRE(result.diagnostics[0].contains("injected finalization failure"));
    }

    MK_REQUIRE(!std::filesystem::exists(project_root / "assets" / "imported_sources" / "hero.png"));
    MK_REQUIRE(!std::filesystem::exists(project_root / "assets" / "imported_sources" / "prop.png"));
    MK_REQUIRE(!directory_has_copying_file(project_root / "assets" / "imported_sources"));

    std::filesystem::remove_all(root);
}
#endif

MK_TEST("editor asset browser import execution requires reviewed confirmed generation") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};

    const auto missing_confirmation =
        app.execute_reviewed_asset_browser_import_plan(mirakana::editor::NativeEditorAssetBrowserImportExecutionRequest{
            .expected_generation = app.asset_browser().generation,
            .user_confirmed = false,
        });
    MK_REQUIRE(!missing_confirmation.executed);
    MK_REQUIRE(!missing_confirmation.import_tools_invoked);
    MK_REQUIRE(missing_confirmation.command.requires_user_confirmation);

    const auto stale =
        app.execute_reviewed_asset_browser_import_plan(mirakana::editor::NativeEditorAssetBrowserImportExecutionRequest{
            .expected_generation = app.asset_browser().generation + 1U,
            .user_confirmed = true,
        });
    MK_REQUIRE(!stale.executed);
    MK_REQUIRE(stale.command.status == mirakana::editor::EditorAssetBrowserCommandStatus::rejected_stale_generation);

    mirakana::MemoryFileSystem filesystem;
    app.bind_native_services(mirakana::editor::NativeEditorServiceBindings{
        .asset_import_filesystem = &filesystem,
        .asset_import_filesystem_id = "memory_import_fs",
    });
    const auto confirmed =
        app.execute_reviewed_asset_browser_import_plan(mirakana::editor::NativeEditorAssetBrowserImportExecutionRequest{
            .expected_generation = app.asset_browser().generation,
            .user_confirmed = true,
        });
    MK_REQUIRE(confirmed.executed);
    MK_REQUIRE(confirmed.import_tools_invoked);
    MK_REQUIRE(confirmed.command.command_id == "asset_browser.import.execute_reviewed_plan");
    MK_REQUIRE(confirmed.import_failure_count > 0U);
    MK_REQUIRE(app.services().asset_import_filesystem_id == "memory_import_fs");
    MK_REQUIRE(app.services().asset_import_filesystem_available);
    MK_REQUIRE(app.services().asset_import_executions == 1U);
}

MK_TEST("editor asset import execution registers imported cooked records and refreshes browser") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    mirakana::MemoryFileSystem filesystem;
    filesystem.write_text(app.project().source_registry_path, mirakana::serialize_source_asset_registry_document(
                                                                  mirakana::SourceAssetRegistryDocumentV1{}));
    write_native_asset_import_texture_source(filesystem, "assets/imported_sources/hero.png");
    app.bind_native_services(mirakana::editor::NativeEditorServiceBindings{
        .asset_import_filesystem = &filesystem,
        .asset_import_filesystem_id = "memory_import_fs",
    });

    const auto registration_generation = app.asset_browser().generation;
    const auto registration = app.apply_reviewed_asset_browser_import_sources(
        mirakana::editor::NativeEditorAssetBrowserSourceRegistrationRequest{
            .expected_generation = registration_generation,
            .project_source_paths = {"assets/imported_sources/hero.png"},
            .provenance_rows = {make_native_asset_import_test_provenance("assets/imported/hero")},
            .user_confirmed = true,
        });
    MK_REQUIRE(registration.applied);
    MK_REQUIRE(!filesystem.exists("assets/imported/hero.texture"));

    const auto execution_generation = app.asset_browser().generation;
    const auto execution =
        app.execute_reviewed_asset_browser_import_plan(mirakana::editor::NativeEditorAssetBrowserImportExecutionRequest{
            .expected_generation = execution_generation,
            .user_confirmed = true,
        });

    MK_REQUIRE(execution.executed);
    MK_REQUIRE(execution.import_tools_invoked);
    MK_REQUIRE(execution.imported_count == 1U);
    MK_REQUIRE(execution.import_failure_count == 0U);
    MK_REQUIRE(execution.registered_imported_count == 1U);
    MK_REQUIRE(execution.browser_refreshed);
    MK_REQUIRE(execution.diagnostic == "asset browser import execution succeeded");
    MK_REQUIRE(filesystem.exists("assets/imported/hero.texture"));
    MK_REQUIRE(app.asset_browser().generation == execution_generation + 1U);

    const auto* browser_row = find_asset_browser_row_by_key(app.asset_browser(), "assets/imported/hero");
    MK_REQUIRE(browser_row != nullptr);
    MK_REQUIRE(browser_row->import_status_label == "imported");
}

MK_TEST("editor asset import execution keeps browser unchanged on failed import batch") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    mirakana::MemoryFileSystem filesystem;
    filesystem.write_text(app.project().source_registry_path, mirakana::serialize_source_asset_registry_document(
                                                                  mirakana::SourceAssetRegistryDocumentV1{}));
    write_native_asset_import_texture_source(filesystem, "assets/imported_sources/hero.png");
    write_native_asset_import_texture_source(filesystem, "assets/imported_sources/prop.png");
    app.bind_native_services(mirakana::editor::NativeEditorServiceBindings{
        .asset_import_filesystem = &filesystem,
        .asset_import_filesystem_id = "memory_import_fs",
    });

    const auto registration = app.apply_reviewed_asset_browser_import_sources(
        mirakana::editor::NativeEditorAssetBrowserSourceRegistrationRequest{
            .expected_generation = app.asset_browser().generation,
            .project_source_paths = {"assets/imported_sources/hero.png", "assets/imported_sources/prop.png"},
            .provenance_rows = {make_native_asset_import_test_provenance("assets/imported/hero"),
                                make_native_asset_import_test_provenance("assets/imported/prop")},
            .user_confirmed = true,
        });
    MK_REQUIRE(registration.applied);

    const auto execution_generation = app.asset_browser().generation;
    const auto* hero_before = find_asset_browser_row_by_key(app.asset_browser(), "assets/imported/hero");
    MK_REQUIRE(hero_before != nullptr);
    MK_REQUIRE(hero_before->import_status_label == "planned");
    filesystem.remove("assets/imported_sources/prop.png");

    const auto execution =
        app.execute_reviewed_asset_browser_import_plan(mirakana::editor::NativeEditorAssetBrowserImportExecutionRequest{
            .expected_generation = execution_generation,
            .user_confirmed = true,
        });

    MK_REQUIRE(execution.executed);
    MK_REQUIRE(execution.import_tools_invoked);
    MK_REQUIRE(execution.imported_count == 0U);
    MK_REQUIRE(execution.import_failure_count == 1U);
    MK_REQUIRE(execution.registered_imported_count == 0U);
    MK_REQUIRE(!execution.browser_refreshed);
    MK_REQUIRE(execution.diagnostic.contains("missing source file"));
    MK_REQUIRE(!filesystem.exists("assets/imported/hero.texture"));
    MK_REQUIRE(!filesystem.exists("assets/imported/prop.texture"));
    MK_REQUIRE(app.asset_browser().generation == execution_generation);

    const auto* hero_after = find_asset_browser_row_by_key(app.asset_browser(), "assets/imported/hero");
    MK_REQUIRE(hero_after != nullptr);
    MK_REQUIRE(hero_after->import_status_label == "planned");
}

MK_TEST("native asset import execution records job progress rows") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    mirakana::MemoryFileSystem filesystem;
    filesystem.write_text(app.project().source_registry_path, mirakana::serialize_source_asset_registry_document(
                                                                  mirakana::SourceAssetRegistryDocumentV1{}));
    write_native_asset_import_texture_source(filesystem, "assets/imported_sources/hero.png");
    app.bind_native_services(mirakana::editor::NativeEditorServiceBindings{
        .asset_import_filesystem = &filesystem,
        .asset_import_filesystem_id = "memory_import_fs",
    });

    const auto registration = app.apply_reviewed_asset_browser_import_sources(
        mirakana::editor::NativeEditorAssetBrowserSourceRegistrationRequest{
            .expected_generation = app.asset_browser().generation,
            .project_source_paths = {"assets/imported_sources/hero.png"},
            .provenance_rows = {make_native_asset_import_test_provenance("assets/imported/hero")},
            .user_confirmed = true,
        });
    MK_REQUIRE(registration.applied);

    const auto job_generation_before = app.asset_import_jobs().generation;
    const auto execution =
        app.execute_reviewed_asset_browser_import_plan(mirakana::editor::NativeEditorAssetBrowserImportExecutionRequest{
            .expected_generation = app.asset_browser().generation,
            .user_confirmed = true,
        });

    MK_REQUIRE(execution.executed);
    MK_REQUIRE(!execution.job_id.empty());
    MK_REQUIRE(app.asset_import_jobs().generation > job_generation_before);
    MK_REQUIRE(app.asset_import_jobs().rows.size() == 1U);
    const auto& job = app.asset_import_jobs().rows[0];
    MK_REQUIRE(job.id == execution.job_id);
    MK_REQUIRE(job.state == mirakana::editor::EditorAssetImportJobState::succeeded);
    MK_REQUIRE(job.state_label == "succeeded");
    MK_REQUIRE(job.source_count == 1U);
    MK_REQUIRE(job.imported_count == 1U);
    MK_REQUIRE(job.failed_count == 0U);
    MK_REQUIRE(job.progress_label == "3/3");
    MK_REQUIRE(!job.can_cancel);
    MK_REQUIRE(!job.can_retry);
    MK_REQUIRE(!job.exposes_native_handles);
}

MK_TEST("native asset import job retry and cancel keep completed snapshot immutable") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    mirakana::MemoryFileSystem filesystem;
    filesystem.write_text(app.project().source_registry_path, mirakana::serialize_source_asset_registry_document(
                                                                  mirakana::SourceAssetRegistryDocumentV1{}));
    write_native_asset_import_texture_source(filesystem, "assets/imported_sources/hero.png");
    filesystem.write_text("assets/imported_sources/prop.png", "not a texture source document");
    app.bind_native_services(mirakana::editor::NativeEditorServiceBindings{
        .asset_import_filesystem = &filesystem,
        .asset_import_filesystem_id = "memory_import_fs",
    });

    const auto registration = app.apply_reviewed_asset_browser_import_sources(
        mirakana::editor::NativeEditorAssetBrowserSourceRegistrationRequest{
            .expected_generation = app.asset_browser().generation,
            .project_source_paths = {"assets/imported_sources/hero.png", "assets/imported_sources/prop.png"},
            .provenance_rows = {make_native_asset_import_test_provenance("assets/imported/hero"),
                                make_native_asset_import_test_provenance("assets/imported/prop")},
            .user_confirmed = true,
        });
    MK_REQUIRE(registration.applied);

    const auto failed_execution =
        app.execute_reviewed_asset_browser_import_plan(mirakana::editor::NativeEditorAssetBrowserImportExecutionRequest{
            .expected_generation = app.asset_browser().generation,
            .user_confirmed = true,
        });
    MK_REQUIRE(failed_execution.executed);
    MK_REQUIRE(failed_execution.import_failure_count == 1U);
    MK_REQUIRE(app.asset_import_jobs().rows.size() == 1U);
    const auto failed_job_id = app.asset_import_jobs().rows[0].id;
    const auto failed_diagnostic = app.asset_import_jobs().rows[0].diagnostic;

    const auto retried = app.retry_asset_import_job(mirakana::editor::NativeEditorAssetImportJobCommandRequest{
        .expected_generation = app.asset_import_jobs().generation,
        .job_id = failed_job_id,
        .user_confirmed = true,
    });
    MK_REQUIRE(retried.applied);
    MK_REQUIRE(app.asset_import_jobs().rows.size() == 2U);
    MK_REQUIRE(app.asset_import_jobs().rows[0].id == failed_job_id);
    MK_REQUIRE(app.asset_import_jobs().rows[0].state == mirakana::editor::EditorAssetImportJobState::failed);
    MK_REQUIRE(app.asset_import_jobs().rows[0].diagnostic == failed_diagnostic);
    const auto retry_job_id = app.asset_import_jobs().rows[1].id;
    MK_REQUIRE(app.asset_import_jobs().rows[1].parent_job_id == failed_job_id);
    MK_REQUIRE(app.asset_import_jobs().rows[1].state == mirakana::editor::EditorAssetImportJobState::queued);

    const auto canceled = app.cancel_asset_import_job(mirakana::editor::NativeEditorAssetImportJobCommandRequest{
        .expected_generation = app.asset_import_jobs().generation,
        .job_id = retry_job_id,
        .user_confirmed = true,
    });
    MK_REQUIRE(canceled.applied);
    MK_REQUIRE(app.asset_import_jobs().rows[1].state == mirakana::editor::EditorAssetImportJobState::canceled);
    MK_REQUIRE(app.asset_import_jobs().rows[1].completed_steps == 0U);
    MK_REQUIRE(!filesystem.exists("assets/imported/hero.texture"));
    MK_REQUIRE(!filesystem.exists("assets/imported/prop.texture"));
}

MK_TEST("native asset browser reimport selection stages selected assets only") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    mirakana::MemoryFileSystem filesystem;
    const auto texture_key = mirakana::AssetKeyV2{"assets/textures/editor_preview"};
    write_native_asset_import_texture_source(filesystem, "source/textures/editor_preview.texture");
    app.bind_native_services(mirakana::editor::NativeEditorServiceBindings{
        .asset_import_filesystem = &filesystem,
        .asset_import_filesystem_id = "memory_import_fs",
    });

    const auto result = app.execute_reviewed_asset_browser_reimport_selection(
        mirakana::editor::NativeEditorAssetBrowserReimportExecutionRequest{
            .expected_generation = app.asset_browser().generation,
            .selected_asset_keys = {texture_key},
            .user_confirmed = true,
        });

    MK_REQUIRE(result.command.command_id == "asset_browser.import.reimport_selected");
    MK_REQUIRE(result.review.ready);
    MK_REQUIRE(result.review.recook_requests.size() == 1U);
    MK_REQUIRE(result.executed);
    MK_REQUIRE(result.import_tools_invoked);
    MK_REQUIRE(result.imported_count == 1U);
    MK_REQUIRE(result.staged_runtime_replacement_count == 1U);
    MK_REQUIRE(result.pending_runtime_replacement_count == 1U);
    MK_REQUIRE(!result.committed_at_safe_point);
    MK_REQUIRE(filesystem.exists("assets/textures/editor_preview.texture"));
}

MK_TEST("native asset browser recook failure rolls back without staged runtime replacement") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    mirakana::MemoryFileSystem filesystem;
    const auto texture_key = mirakana::AssetKeyV2{"assets/textures/editor_preview"};
    filesystem.write_text("source/textures/editor_preview.texture", "invalid texture source");
    app.bind_native_services(mirakana::editor::NativeEditorServiceBindings{
        .asset_import_filesystem = &filesystem,
        .asset_import_filesystem_id = "memory_import_fs",
    });

    const auto result =
        app.execute_reviewed_asset_browser_recook(mirakana::editor::NativeEditorAssetBrowserRecookExecutionRequest{
            .expected_generation = app.asset_browser().generation,
            .ready_recook_requests = {make_native_texture_recook_request(texture_key)},
            .content_hash_rows =
                {
                    mirakana::editor::EditorAssetRecookContentHashRow{
                        .asset_key = texture_key,
                        .source_content_hash = "sha256:source-new",
                        .output_content_hash = "sha256:output-old",
                    },
                },
            .user_confirmed = true,
        });

    MK_REQUIRE(result.command.command_id == "asset_browser.import.recook_stale");
    MK_REQUIRE(result.review.ready);
    MK_REQUIRE(result.executed);
    MK_REQUIRE(result.import_tools_invoked);
    MK_REQUIRE(result.import_failure_count == 1U);
    MK_REQUIRE(result.recook.apply_results.size() == 1U);
    MK_REQUIRE(result.recook.apply_results[0].kind == mirakana::AssetHotReloadApplyResultKind::failed_rolled_back);
    MK_REQUIRE(result.pending_runtime_replacement_count == 0U);
    MK_REQUIRE(!result.committed_at_safe_point);
}

MK_TEST("native asset browser hot reload commits staged replacements only at safe point") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    mirakana::MemoryFileSystem filesystem;
    const auto texture_key = mirakana::AssetKeyV2{"assets/textures/editor_preview"};
    write_native_asset_import_texture_source(filesystem, "source/textures/editor_preview.texture");
    app.bind_native_services(mirakana::editor::NativeEditorServiceBindings{
        .asset_import_filesystem = &filesystem,
        .asset_import_filesystem_id = "memory_import_fs",
    });

    const auto recook =
        app.execute_reviewed_asset_browser_recook(mirakana::editor::NativeEditorAssetBrowserRecookExecutionRequest{
            .expected_generation = app.asset_browser().generation,
            .ready_recook_requests = {make_native_texture_recook_request(texture_key)},
            .content_hash_rows =
                {
                    mirakana::editor::EditorAssetRecookContentHashRow{
                        .asset_key = texture_key,
                        .source_content_hash = "sha256:source-new",
                        .output_content_hash = "sha256:output-old",
                    },
                },
            .user_confirmed = true,
        });
    MK_REQUIRE(recook.executed);
    MK_REQUIRE(recook.staged_runtime_replacement_count == 1U);
    MK_REQUIRE(recook.pending_runtime_replacement_count == 1U);
    MK_REQUIRE(!recook.committed_at_safe_point);

    const auto committed =
        app.commit_staged_asset_browser_hot_reload(mirakana::editor::NativeEditorAssetBrowserHotReloadStageRequest{
            .expected_generation = app.asset_browser().generation,
            .selected_asset_keys = {texture_key},
            .user_confirmed = true,
        });

    MK_REQUIRE(committed.command.command_id == "asset_browser.import.stage_hot_reload");
    MK_REQUIRE(committed.review.ready);
    MK_REQUIRE(committed.committed_at_safe_point);
    MK_REQUIRE(committed.apply_results.size() == 1U);
    MK_REQUIRE(committed.apply_results[0].kind == mirakana::AssetHotReloadApplyResultKind::applied);
    MK_REQUIRE(committed.pending_runtime_replacement_count == 0U);
    MK_REQUIRE(app.asset_browser().generation == committed.generation_after_commit);
}

MK_TEST("native asset browser applies reviewed import sources to source registry") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    mirakana::MemoryFileSystem filesystem;
    filesystem.write_text(app.project().source_registry_path, mirakana::serialize_source_asset_registry_document(
                                                                  mirakana::SourceAssetRegistryDocumentV1{}));
    filesystem.write_text("assets/imported_sources/hero.png", "png source placeholder");
    app.bind_native_services(mirakana::editor::NativeEditorServiceBindings{
        .asset_import_filesystem = &filesystem,
        .asset_import_filesystem_id = "memory_import_fs",
    });

    const auto initial_generation = app.asset_browser().generation;
    const auto result = app.apply_reviewed_asset_browser_import_sources(
        mirakana::editor::NativeEditorAssetBrowserSourceRegistrationRequest{
            .expected_generation = initial_generation,
            .project_source_paths = {"assets/imported_sources/hero.png"},
            .provenance_rows = {make_native_asset_import_test_provenance("assets/imported/hero")},
            .user_confirmed = true,
        });

    MK_REQUIRE(result.command.command_id == "asset_browser.import.register_sources");
    MK_REQUIRE(result.command.mutates_project_files);
    MK_REQUIRE(!result.command.executes_import_tools);
    MK_REQUIRE(result.applied);
    MK_REQUIRE(result.registered_count == 1U);
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(result.review.ready);
    MK_REQUIRE(app.asset_browser().generation == initial_generation + 1U);

    const auto updated_registry =
        mirakana::deserialize_source_asset_registry_document(filesystem.read_text(app.project().source_registry_path));
    const auto* registry_row = find_source_registry_row_by_key(updated_registry, "assets/imported/hero");
    MK_REQUIRE(registry_row != nullptr);
    MK_REQUIRE(registry_row->kind == mirakana::AssetKind::texture);
    MK_REQUIRE(registry_row->source_path == "assets/imported_sources/hero.png");
    MK_REQUIRE(registry_row->imported_path == "assets/imported/hero.texture");

    const auto* browser_row = find_asset_browser_row_by_key(app.asset_browser(), "assets/imported/hero");
    MK_REQUIRE(browser_row != nullptr);
    MK_REQUIRE(browser_row->kind == mirakana::AssetKind::texture);
    MK_REQUIRE(browser_row->import_status_label == "planned");
}

MK_TEST("native asset browser source registration rejects stale generation and legal blockers") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    mirakana::MemoryFileSystem filesystem;
    const auto initial_registry =
        mirakana::serialize_source_asset_registry_document(mirakana::SourceAssetRegistryDocumentV1{});
    filesystem.write_text(app.project().source_registry_path, initial_registry);
    filesystem.write_text("assets/imported_sources/hero.png", "png source placeholder");
    app.bind_native_services(mirakana::editor::NativeEditorServiceBindings{
        .asset_import_filesystem = &filesystem,
        .asset_import_filesystem_id = "memory_import_fs",
    });

    const auto initial_generation = app.asset_browser().generation;
    const auto stale = app.apply_reviewed_asset_browser_import_sources(
        mirakana::editor::NativeEditorAssetBrowserSourceRegistrationRequest{
            .expected_generation = initial_generation + 1U,
            .project_source_paths = {"assets/imported_sources/hero.png"},
            .provenance_rows = {make_native_asset_import_test_provenance("assets/imported/hero")},
            .user_confirmed = true,
        });
    MK_REQUIRE(!stale.applied);
    MK_REQUIRE(stale.command.status == mirakana::editor::EditorAssetBrowserCommandStatus::rejected_stale_generation);
    MK_REQUIRE(filesystem.read_text(app.project().source_registry_path) == initial_registry);
    MK_REQUIRE(app.asset_browser().generation == initial_generation);

    auto blocked_provenance = make_native_asset_import_test_provenance("assets/imported/hero");
    blocked_provenance.license_id.clear();
    const auto blocked = app.apply_reviewed_asset_browser_import_sources(
        mirakana::editor::NativeEditorAssetBrowserSourceRegistrationRequest{
            .expected_generation = initial_generation,
            .project_source_paths = {"assets/imported_sources/hero.png"},
            .provenance_rows = {blocked_provenance},
            .user_confirmed = true,
        });
    MK_REQUIRE(!blocked.applied);
    MK_REQUIRE(blocked.command.status == mirakana::editor::EditorAssetBrowserCommandStatus::ready);
    MK_REQUIRE(!blocked.review.ready);
    MK_REQUIRE(blocked.review.rows.size() == 1U);
    MK_REQUIRE(blocked.review.rows[0].blocked_by_legal);
    MK_REQUIRE(blocked.review.rows[0].diagnostic == "asset_import_provenance_blocked");
    MK_REQUIRE(filesystem.read_text(app.project().source_registry_path) == initial_registry);
    MK_REQUIRE(app.asset_browser().generation == initial_generation);
}

MK_TEST("native asset browser source registration reviews unsafe paths before filesystem queries escape") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    ThrowingUnsafePathFileSystem filesystem;
    const auto initial_registry =
        mirakana::serialize_source_asset_registry_document(mirakana::SourceAssetRegistryDocumentV1{});
    filesystem.files.write_text(app.project().source_registry_path, initial_registry);
    app.bind_native_services(mirakana::editor::NativeEditorServiceBindings{
        .asset_import_filesystem = &filesystem,
        .asset_import_filesystem_id = "throwing_unsafe_path_fs",
    });

    const auto initial_generation = app.asset_browser().generation;
    const auto unsafe = app.apply_reviewed_asset_browser_import_sources(
        mirakana::editor::NativeEditorAssetBrowserSourceRegistrationRequest{
            .expected_generation = initial_generation,
            .project_source_paths = {"assets/../outside/hero.png"},
            .provenance_rows = {make_native_asset_import_test_provenance("assets/imported/hero")},
            .user_confirmed = true,
        });

    MK_REQUIRE(!unsafe.applied);
    MK_REQUIRE(unsafe.command.status == mirakana::editor::EditorAssetBrowserCommandStatus::ready);
    MK_REQUIRE(!unsafe.review.ready);
    MK_REQUIRE(unsafe.review.rows.size() == 1U);
    MK_REQUIRE(unsafe.review.rows[0].diagnostic == "unsafe_import_source_path");
    MK_REQUIRE(filesystem.unsafe_query_count == 0U);
    MK_REQUIRE(filesystem.files.read_text(app.project().source_registry_path) == initial_registry);
    MK_REQUIRE(app.asset_browser().generation == initial_generation);
}

MK_TEST("editor first party document exposes runtime UI editor authoring rows") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};

    const auto shell_document = mirakana::editor::make_first_party_editor_document(app);
    const auto counters = mirakana::editor::make_first_party_editor_shell_smoke_counters(app, shell_document);

    MK_REQUIRE(contains_element(shell_document.document, "editor.panel.runtime_ui_editor.runtime_ui.hierarchy"));
    MK_REQUIRE(contains_element(shell_document.document, "editor.panel.runtime_ui_editor.runtime_ui.hierarchy.root"));
    MK_REQUIRE(contains_element(shell_document.document, "editor.panel.runtime_ui_editor.runtime_ui.inspector"));
    MK_REQUIRE(contains_element(shell_document.document,
                                "editor.panel.runtime_ui_editor.runtime_ui.inspector.selected.label"));
    MK_REQUIRE(contains_element(shell_document.document, "editor.panel.runtime_ui_editor.runtime_ui.style_tokens"));
    MK_REQUIRE(contains_element(shell_document.document,
                                "editor.panel.runtime_ui_editor.runtime_ui.style_tokens.button.primary"));
    MK_REQUIRE(contains_element(shell_document.document, "editor.panel.runtime_ui_editor.runtime_ui.preview"));
    MK_REQUIRE(contains_element(shell_document.document, "editor.panel.runtime_ui_editor.runtime_ui.preview.document"));
    MK_REQUIRE(contains_element(shell_document.document,
                                "editor.panel.runtime_ui_editor.runtime_ui.preview.renderer_upload_status"));
    MK_REQUIRE(contains_element(shell_document.document,
                                "editor.panel.runtime_ui_editor.runtime_ui.platform_readiness.text_font_status"));
    MK_REQUIRE(contains_element(shell_document.document,
                                "editor.panel.runtime_ui_editor.runtime_ui.platform_readiness.ime_status"));
    MK_REQUIRE(contains_element(shell_document.document,
                                "editor.panel.runtime_ui_editor.runtime_ui.platform_readiness.accessibility_status"));
    MK_REQUIRE(contains_element(shell_document.document,
                                "editor.panel.runtime_ui_editor.runtime_ui.platform_readiness.renderer_upload_status"));
    MK_REQUIRE(contains_element(shell_document.document,
                                "editor.panel.runtime_ui_editor.runtime_ui.clean_room.external_engine_parity_claim"));
    MK_REQUIRE(contains_element(shell_document.document,
                                "editor.panel.runtime_ui_editor.runtime_ui.clean_room.native_handles_exposed"));

    MK_REQUIRE(counters.runtime_ui_editor_panel_visible);
    MK_REQUIRE(counters.runtime_ui_editor_hierarchy_rows >= 2U);
    MK_REQUIRE(counters.runtime_ui_editor_inspector_rows >= 3U);
    MK_REQUIRE(counters.runtime_ui_editor_style_rows >= 2U);
    MK_REQUIRE(counters.runtime_ui_editor_preview_rows >= 1U);
    MK_REQUIRE(!counters.runtime_ui_editor_external_engine_parity_claim);
    MK_REQUIRE(!counters.runtime_ui_editor_native_handles_exposed);
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
    MK_REQUIRE(shell_document.tab_header_count == 12U);
    MK_REQUIRE(shell_document.split_gutter_count == 3U);
    MK_REQUIRE(shell_document.active_panel_count == 4U);
    MK_REQUIRE(shell_document.focusable_dock_control_count == 12U);
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
    MK_REQUIRE(shell_document.tab_header_count == 12U);
    MK_REQUIRE(shell_document.focusable_dock_control_count == 11U);
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

MK_TEST("editor first party document composes ai commands as read only rich text") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};

    const auto shell_document = mirakana::editor::make_first_party_editor_document(app);
    const auto* ai_root =
        shell_document.document.find(mirakana::ui::ElementId{.value = "editor.panel.ai_commands.rich_text"});
    const auto* ai_status = shell_document.document.find(
        mirakana::ui::ElementId{.value = "editor.panel.ai_commands.rich_text.paragraph.status."
                                         "span.value"});

    MK_REQUIRE(ai_root != nullptr);
    MK_REQUIRE(ai_root->role == mirakana::ui::SemanticRole::root);
    MK_REQUIRE(ai_root->parent.value == "editor.panel.ai_commands");
    MK_REQUIRE(ai_status != nullptr);
    MK_REQUIRE(ai_status->text.label == "ready");
}

MK_TEST("editor first party document composes inspector as read only rich text") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};

    const auto shell_document = mirakana::editor::make_first_party_editor_document(app);
    const auto* inspector_root =
        shell_document.document.find(mirakana::ui::ElementId{.value = "editor.panel.inspector.rich_text"});
    const auto* project_value = shell_document.document.find(
        mirakana::ui::ElementId{.value = "editor.panel.inspector.rich_text.paragraph.property.project.span.value"});

    MK_REQUIRE(inspector_root != nullptr);
    MK_REQUIRE(inspector_root->role == mirakana::ui::SemanticRole::root);
    MK_REQUIRE(inspector_root->parent.value == "editor.panel.inspector");
    MK_REQUIRE(project_value != nullptr);
    MK_REQUIRE(project_value->text.label == "MIRAIKANAI Editor");
    MK_REQUIRE(std::ranges::any_of(shell_document.renderer_submission.text_runs, [](const auto& run) {
        return run.id.value == "editor.panel.inspector.rich_text.paragraph.property.project.span.value" &&
               run.text.label == "MIRAIKANAI Editor";
    }));
}

MK_TEST("editor first party document exposes a first party editable text target") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};

    const auto shell_document = mirakana::editor::make_first_party_editor_document(app);
    const auto& text_input = app.text_input_state();
    const auto* field = shell_document.document.find(text_input.edit_state.target);

    MK_REQUIRE(field != nullptr);
    MK_REQUIRE(field->role == mirakana::ui::SemanticRole::text_field);
    MK_REQUIRE(field->text.label == app.project().name);
    MK_REQUIRE(field->enabled);
    MK_REQUIRE(text_input.target_registered);
    MK_REQUIRE(text_input.caret_rect_ready);
    MK_REQUIRE(text_input.surrounding_text_ready);
    MK_REQUIRE(!text_input.native_handles_exposed);
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

MK_TEST("editor native UIA provider maps accessibility payload to stable fragment tree rows") {
    const mirakana::ui::ElementId root_id{.value = "editor.root"};
    const mirakana::ui::ElementId panel_id{.value = "editor.panel.inspector"};
    const mirakana::ui::ElementId button_id{.value = "editor.panel.inspector.apply"};
    const mirakana::ui::ElementId text_field_id{.value = "editor.panel.project_settings.name.text_field"};
    const mirakana::ui::AccessibilityPayload payload{
        .nodes = {
            mirakana::ui::AccessibilityNode{
                .id = root_id,
                .role = mirakana::ui::SemanticRole::root,
                .label = "MIRAIKANAI Editor",
                .bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 1280.0F, .height = 720.0F},
            },
            mirakana::ui::AccessibilityNode{
                .id = panel_id,
                .role = mirakana::ui::SemanticRole::panel,
                .label = "Inspector",
                .bounds = mirakana::ui::Rect{.x = 960.0F, .y = 0.0F, .width = 320.0F, .height = 720.0F},
                .parent = root_id,
                .depth = 1U,
            },
            mirakana::ui::AccessibilityNode{
                .id = button_id,
                .role = mirakana::ui::SemanticRole::button,
                .label = "Apply",
                .bounds = mirakana::ui::Rect{.x = 976.0F, .y = 48.0F, .width = 96.0F, .height = 28.0F},
                .enabled = true,
                .focusable = true,
                .parent = panel_id,
                .depth = 2U,
            },
            mirakana::ui::AccessibilityNode{
                .id = text_field_id,
                .role = mirakana::ui::SemanticRole::text_field,
                .label = "Project Name",
                .bounds = mirakana::ui::Rect{.x = 976.0F, .y = 92.0F, .width = 240.0F, .height = 28.0F},
                .enabled = true,
                .focusable = true,
                .parent = panel_id,
                .depth = 2U,
            },
        }};

    const mirakana::editor::NativeEditorUiaScreenOrigin screen_origin{.x = 320.0F, .y = 48.0F};
    const auto state = mirakana::editor::plan_native_editor_uia_provider_tree(payload, button_id, screen_origin);

    MK_REQUIRE(state.service_id == "win32_uia");
    MK_REQUIRE(state.status_id == "uia_provider_ready");
    MK_REQUIRE(state.provider_available);
    MK_REQUIRE(!state.native_handles_exposed);
    MK_REQUIRE(state.nodes.size() == 4U);
    MK_REQUIRE(state.role_rows == 4U);
    MK_REQUIRE(state.name_rows == 4U);
    MK_REQUIRE(state.state_rows == 4U);
    MK_REQUIRE(state.focus_rows == 1U);
    MK_REQUIRE(state.action_rows == 2U);
    MK_REQUIRE(state.relationship_rows == 3U);
    MK_REQUIRE(state.tree_navigation_rows >= 6U);
    MK_REQUIRE(state.diagnostics.empty());
    MK_REQUIRE(state.hidden_nodes == 0U);
    MK_REQUIRE(state.unsupported_pattern_diagnostics == 0U);

    const auto* root = find_uia_node(state, root_id.value);
    const auto* panel = find_uia_node(state, panel_id.value);
    const auto* button = find_uia_node(state, button_id.value);
    const auto* text_field = find_uia_node(state, text_field_id.value);

    MK_REQUIRE(root != nullptr);
    MK_REQUIRE(panel != nullptr);
    MK_REQUIRE(button != nullptr);
    MK_REQUIRE(text_field != nullptr);
    MK_REQUIRE(root->control_type_id == "UIA_PaneControlTypeId");
    MK_REQUIRE(panel->control_type_id == "UIA_PaneControlTypeId");
    MK_REQUIRE(button->control_type_id == "UIA_ButtonControlTypeId");
    MK_REQUIRE(text_field->control_type_id == "UIA_EditControlTypeId");
    MK_REQUIRE(root->bounds == (mirakana::ui::Rect{.x = 320.0F, .y = 48.0F, .width = 1280.0F, .height = 720.0F}));
    MK_REQUIRE(panel->bounds == (mirakana::ui::Rect{.x = 1280.0F, .y = 48.0F, .width = 320.0F, .height = 720.0F}));
    MK_REQUIRE(button->bounds == (mirakana::ui::Rect{.x = 1296.0F, .y = 96.0F, .width = 96.0F, .height = 28.0F}));
    MK_REQUIRE(root->first_child == panel_id);
    MK_REQUIRE(panel->parent == root_id);
    MK_REQUIRE(panel->first_child == button_id);
    MK_REQUIRE(panel->last_child == text_field_id);
    MK_REQUIRE(button->next_sibling == text_field_id);
    MK_REQUIRE(text_field->previous_sibling == button_id);
    MK_REQUIRE(button->has_keyboard_focus);
    MK_REQUIRE(!text_field->has_keyboard_focus);
    MK_REQUIRE(root->runtime_id.empty());
    MK_REQUIRE(!panel->runtime_id.empty());
    MK_REQUIRE(!button->runtime_id.empty());
    MK_REQUIRE(button->runtime_id.front() == 3);
    MK_REQUIRE(std::ranges::find(button->actions, "focus") != button->actions.end());
    MK_REQUIRE(std::ranges::find(text_field->actions, "focus") != text_field->actions.end());
}

MK_TEST("editor native UIA provider diagnoses incomplete accessibility rows") {
    const mirakana::ui::ElementId invalid_id{.value = "editor.invalid"};
    const mirakana::ui::AccessibilityPayload payload{
        .nodes = {
            mirakana::ui::AccessibilityNode{
                .id = invalid_id,
                .role = mirakana::ui::SemanticRole::none,
                .label = "",
                .bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 0.0F, .height = 16.0F},
            },
        }};

    const auto state = mirakana::editor::plan_native_editor_uia_provider_tree(payload, {});

    MK_REQUIRE(!state.provider_available);
    MK_REQUIRE(state.status_id == "uia_provider_diagnostics");
    MK_REQUIRE(state.missing_name_diagnostics == 1U);
    MK_REQUIRE(state.missing_role_diagnostics == 1U);
    MK_REQUIRE(state.invalid_bounds_diagnostics == 1U);
    MK_REQUIRE(state.hidden_nodes == 0U);
    MK_REQUIRE(state.unsupported_pattern_diagnostics == 0U);
    MK_REQUIRE(state.diagnostics.size() == 3U);
}

MK_TEST("editor first party shell smoke counters report imgui disabled") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    const auto shell_document = mirakana::editor::make_first_party_editor_document(app);

    const auto counters = mirakana::editor::make_first_party_editor_shell_smoke_counters(app, shell_document);

    MK_REQUIRE(counters.ui == "first_party");
    MK_REQUIRE(!counters.imgui_enabled);
    MK_REQUIRE(counters.backend == "d3d12");
    MK_REQUIRE(counters.panel_count == 12U);
    MK_REQUIRE(!counters.sdl3_enabled);
    MK_REQUIRE(!counters.viewport_native_handles_exposed);
    MK_REQUIRE(!counters.material_preview_native_handles_exposed);
    MK_REQUIRE(counters.docking_status == "single_window_ready");
    MK_REQUIRE(counters.dock_tab_header_count == 12U);
    MK_REQUIRE(counters.dock_split_gutter_count == 3U);
    MK_REQUIRE(counters.dock_active_panel_count == 4U);
    MK_REQUIRE(counters.dock_focusable_control_count == 12U);
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

MK_TEST("first party editor adapter boundaries report private DirectWrite text and font readiness") {
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
    MK_REQUIRE(font_rasterization->official_source_family.find("DirectWrite") != std::string::npos);
    MK_REQUIRE(ime_text_services->official_source_family.find("Text Services Framework") != std::string::npos);
    MK_REQUIRE(accessibility->official_source_family.find("UI Automation") != std::string::npos);

#if defined(_WIN32)
    constexpr bool text_font_adapter_implemented = true;
    constexpr bool accessibility_adapter_implemented = true;
#else
    constexpr bool text_font_adapter_implemented = false;
    constexpr bool accessibility_adapter_implemented = false;
#endif
    MK_REQUIRE(text_shaping->implemented == text_font_adapter_implemented);
    MK_REQUIRE(font_rasterization->implemented == text_font_adapter_implemented);
    MK_REQUIRE(ime_text_services->implemented == text_font_adapter_implemented);
    MK_REQUIRE(accessibility->implemented == accessibility_adapter_implemented);
    for (const auto& row : rows) {
        MK_REQUIRE(!row.id.empty());
        MK_REQUIRE(!row.native_handles_public);
    }
}

#if defined(_WIN32)
MK_TEST("editor DirectWrite text and font adapters feed runtime UI validation without native handles") {
    auto text_shaping = mirakana::editor::make_native_editor_directwrite_text_shaping_adapter();
    auto font_rasterizer = mirakana::editor::make_native_editor_directwrite_font_rasterizer_adapter();

    MK_REQUIRE(text_shaping != nullptr);
    MK_REQUIRE(font_rasterizer != nullptr);
    MK_REQUIRE(!mirakana::editor::native_editor_directwrite_text_font_adapters_expose_native_handles());

    const mirakana::ui::TextLayoutRequest text_request{
        .text = "Mirakanai",
        .font_family = "Segoe UI",
        .direction = mirakana::ui::TextDirection::left_to_right,
        .wrap = mirakana::ui::TextWrapMode::clip,
        .max_width = 256.0F,
    };
    const auto shaped = mirakana::ui::shape_text_run(*text_shaping, text_request);

    MK_REQUIRE(shaped.succeeded());
    MK_REQUIRE(!shaped.runs.empty());
    MK_REQUIRE(!shaped.runs.front().glyphs.empty());

    const auto& shaped_glyph = shaped.runs.front().glyphs.front();
    const mirakana::ui::FontRasterizationRequest font_request{
        .font_family = shaped_glyph.font_family,
        .glyph = shaped_glyph.glyph,
        .pixel_size = 18.0F,
    };
    const auto rasterized = mirakana::ui::rasterize_font_glyph(*font_rasterizer, font_request);

    MK_REQUIRE(rasterized.succeeded());
    MK_REQUIRE(rasterized.allocation.has_value());
    MK_REQUIRE(rasterized.allocation->glyph == shaped_glyph.glyph);
    MK_REQUIRE(rasterized.allocation->bitmap.pixel_format == mirakana::ui::FontRasterizationPixelFormat::alpha8);
    MK_REQUIRE(rasterized.allocation->metrics.advance_x > 0.0F);
}

MK_TEST("editor DirectWrite text atlas handoff evidence separates adapter glyph fallback and gate rows") {
    const auto evidence = mirakana::editor::make_native_editor_directwrite_text_atlas_handoff_evidence(
        mirakana::editor::NativeEditorTextAtlasHandoffDesc{
            .text = "Mirakanai",
            .font_family = "Segoe UI",
            .pixel_size = 18.0F,
            .max_width = 512.0F,
        });

    MK_REQUIRE(evidence.status == "glyphs_ready_atlas_handoff_host_gated");
    MK_REQUIRE(evidence.text_shaping_adapter_invoked);
    MK_REQUIRE(evidence.font_rasterizer_adapter_invoked);
    MK_REQUIRE(evidence.adapter_invoked_rows == 2U);
    MK_REQUIRE(evidence.glyphs_ready);
    MK_REQUIRE(evidence.glyphs_ready_rows == 1U);
    MK_REQUIRE(evidence.shaped_glyph_count >= 4U);
    MK_REQUIRE(evidence.rasterized_glyph_count >= 1U);
    MK_REQUIRE(evidence.fallback_rows >= 1U);
    MK_REQUIRE(evidence.fallback_used_rows == 0U);
    MK_REQUIRE(evidence.fallback_not_used_rows >= 1U);
    MK_REQUIRE(!evidence.fallback_used);
    MK_REQUIRE(evidence.host_gated_rows == 1U);
    MK_REQUIRE(evidence.unsupported_rows == 1U);
    MK_REQUIRE(!evidence.atlas_handoff_ready);
    MK_REQUIRE(!evidence.native_handles_exposed);

    MK_REQUIRE(std::ranges::any_of(evidence.rows, [](const auto& row) {
        return row.id == "editor.text_atlas.adapter.text_shaping" && row.status == "adapter_invoked" &&
               row.adapter_invoked && !row.native_handles_public;
    }));
    MK_REQUIRE(std::ranges::any_of(evidence.rows, [](const auto& row) {
        return row.id == "editor.text_atlas.glyphs.ready" && row.status == "ready" && row.glyphs_ready;
    }));
    MK_REQUIRE(std::ranges::any_of(evidence.rows, [](const auto& row) {
        return row.id == "editor.text_atlas.font_fallback" && row.status == "not_used" && !row.fallback_used;
    }));
    MK_REQUIRE(std::ranges::any_of(evidence.rows, [](const auto& row) {
        return row.id == "editor.text_atlas.direct2d_atlas.host_gate" && row.status == "host_evidence_required" &&
               row.host_evidence_required && !row.host_evidence_available;
    }));
    MK_REQUIRE(std::ranges::any_of(evidence.rows, [](const auto& row) {
        return row.id == "editor.text_atlas.gpu_upload.unsupported" && row.status == "unsupported" && row.unsupported;
    }));
}

MK_TEST("editor private TSF text services adapter tracks platform request rows without native handles") {
    auto adapter = mirakana::editor::make_native_editor_tsf_text_services_adapter();

    MK_REQUIRE(adapter != nullptr);
    MK_REQUIRE(!adapter->native_handles_exposed());
    MK_REQUIRE(!adapter->active_request().has_value());

    const mirakana::ui::PlatformTextInputRequest request{
        .target = mirakana::ui::ElementId{.value = "editor.panel.project_settings.name.text_field"},
        .text_bounds = mirakana::ui::Rect{.x = 16.0F, .y = 48.0F, .width = 320.0F, .height = 24.0F},
        .surrounding_text = "MIRAIKANAI",
        .cursor_byte_offset = 10U,
        .selection_byte_length = 0U,
    };

    adapter->begin_text_input(request);

    MK_REQUIRE(adapter->active_request().has_value());
    MK_REQUIRE(adapter->active_request()->target == request.target);
    MK_REQUIRE(adapter->active_request()->surrounding_text == "MIRAIKANAI");
    MK_REQUIRE(adapter->active_request()->cursor_byte_offset == 10U);
    MK_REQUIRE(adapter->tsf_thread_manager_ready());
    MK_REQUIRE(adapter->tsf_document_manager_ready());

    adapter->end_text_input(request.target);

    MK_REQUIRE(!adapter->active_request().has_value());
    MK_REQUIRE(!adapter->tsf_document_manager_ready());
}

MK_TEST("editor native app recognizes private TSF text input and IME adapters") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    auto adapter = mirakana::editor::make_native_editor_tsf_text_services_adapter();

    app.bind_native_services(mirakana::editor::NativeEditorServiceBindings{
        .platform_text_input_adapter = adapter.get(),
        .ime_adapter = adapter.get(),
        .platform_text_input_service_id = "win32_tsf",
        .ime_service_id = "win32_tsf",
    });

    MK_REQUIRE(app.services().platform_text_input_service_id == "win32_tsf");
    MK_REQUIRE(app.services().ime_service_id == "win32_tsf");
    MK_REQUIRE(app.text_input_state().tsf_adapter_selected);
    MK_REQUIRE(!app.text_input_state().native_handles_exposed);
    MK_REQUIRE(mirakana::editor::native_editor_text_input_status(app.text_input_state()) == "win32_tsf_selected");
}

MK_TEST("editor native app publishes first party shell accessibility through private UIA adapter") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    auto adapter = mirakana::editor::make_native_editor_uia_accessibility_adapter();

    app.bind_native_services(mirakana::editor::NativeEditorServiceBindings{
        .accessibility_adapter = adapter.get(),
        .accessibility_service_id = "win32_uia",
    });

    const auto shell_document = mirakana::editor::make_first_party_editor_document(app);
    const auto payload = mirakana::ui::build_accessibility_payload(shell_document.renderer_submission);
    const auto result = app.publish_native_accessibility_payload(payload, shell_document.focused_element);

    MK_REQUIRE(result.published);
    MK_REQUIRE(result.nodes_published == payload.nodes.size());
    MK_REQUIRE(app.services().accessibility_service_id == "win32_uia");
    MK_REQUIRE(app.services().accessibility_available);
    MK_REQUIRE(app.services().accessibility_publish_requests == 1U);
    MK_REQUIRE(app.accessibility_state().status_id == "uia_provider_ready");
    MK_REQUIRE(app.accessibility_state().provider_available);
    MK_REQUIRE(app.accessibility_state().nodes.size() == payload.nodes.size());
    MK_REQUIRE(app.accessibility_state().focus_rows == 1U);
    MK_REQUIRE(!app.accessibility_state().nodes.empty());
    MK_REQUIRE(app.accessibility_state().nodes.front().runtime_id.empty());
    MK_REQUIRE(app.accessibility_state().hidden_nodes == 0U);
    MK_REQUIRE(app.accessibility_state().unsupported_pattern_diagnostics == 0U);
    MK_REQUIRE(!app.accessibility_state().native_handles_exposed);
    MK_REQUIRE(adapter->state().status_id == "uia_provider_ready");
}
#endif

MK_TEST("editor first party shell smoke counters expose text atlas handoff evidence") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    app.record_native_text_atlas_handoff_evidence(mirakana::editor::NativeEditorTextAtlasHandoffEvidence{
        .status = "glyphs_ready_atlas_handoff_host_gated",
        .text_shaping_adapter_invoked = true,
        .font_rasterizer_adapter_invoked = true,
        .glyphs_ready = true,
        .fallback_used = false,
        .atlas_handoff_ready = false,
        .native_handles_exposed = false,
        .adapter_invoked_rows = 2U,
        .glyphs_ready_rows = 1U,
        .fallback_rows = 1U,
        .fallback_used_rows = 0U,
        .fallback_not_used_rows = 1U,
        .host_gated_rows = 1U,
        .unsupported_rows = 1U,
        .shaped_glyph_count = 9U,
        .rasterized_glyph_count = 9U,
        .zero_ink_glyph_count = 0U,
        .rows = {},
    });

    const auto shell_document = mirakana::editor::make_first_party_editor_document(app);
    const auto counters = mirakana::editor::make_first_party_editor_shell_smoke_counters(app, shell_document);

    MK_REQUIRE(counters.text_atlas_handoff_status == "glyphs_ready_atlas_handoff_host_gated");
    MK_REQUIRE(counters.text_font_adapter_invoked);
    MK_REQUIRE(counters.text_font_glyphs_ready);
    MK_REQUIRE(!counters.text_font_fallback_used);
    MK_REQUIRE(counters.text_atlas_handoff_host_gated_rows == 1U);
    MK_REQUIRE(counters.text_atlas_handoff_unsupported_rows == 1U);
    MK_REQUIRE(!counters.text_atlas_handoff_ready);
    MK_REQUIRE(!counters.text_font_native_handles_exposed);
}

MK_TEST("editor native shell first party IME controller routes focused text target sessions") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    RecordingPlatformTextInputAdapter platform_text_input;
    RecordingImeAdapter ime;
    app.bind_native_services(mirakana::editor::NativeEditorServiceBindings{
        .platform_text_input_adapter = &platform_text_input,
        .ime_adapter = &ime,
        .platform_text_input_service_id = "test_platform_text_input",
        .ime_service_id = "test_ime",
    });

    auto project_name = mirakana::editor::make_native_editor_project_name_text_input_target("MIRAIKANAI");
    project_name.edit_state.cursor_byte_offset = project_name.edit_state.text.size();
    const auto first_focus = app.focus_text_input_target(project_name);

    MK_REQUIRE(first_focus.succeeded());
    MK_REQUIRE(first_focus.session_begun);
    MK_REQUIRE(!first_focus.previous_session_ended);
    MK_REQUIRE(platform_text_input.begin_requests.size() == 1U);
    MK_REQUIRE(platform_text_input.begin_requests[0].target == project_name.edit_state.target);
    MK_REQUIRE(platform_text_input.ended_targets.empty());
    MK_REQUIRE(app.text_input_state().session_active);
    MK_REQUIRE(app.text_input_state().surrounding_text == "MIRAIKANAI");
    MK_REQUIRE(app.services().platform_text_input_service_id == "test_platform_text_input");
    MK_REQUIRE(app.services().ime_service_id == "test_ime");
    MK_REQUIRE(app.services().platform_text_input_sessions_started == 1U);

    auto asset_root = mirakana::editor::make_native_editor_project_name_text_input_target("assets");
    asset_root.edit_state.target.value = "editor.panel.project_settings.asset_root.text_field";
    asset_root.edit_state.cursor_byte_offset = asset_root.edit_state.text.size();
    asset_root.caret_bounds.x = 32.0F;

    const auto second_focus = app.focus_text_input_target(asset_root);

    MK_REQUIRE(second_focus.succeeded());
    MK_REQUIRE(second_focus.session_begun);
    MK_REQUIRE(second_focus.previous_session_ended);
    MK_REQUIRE(platform_text_input.begin_requests.size() == 2U);
    MK_REQUIRE(platform_text_input.ended_targets.size() == 1U);
    MK_REQUIRE(platform_text_input.ended_targets[0] == project_name.edit_state.target);
    MK_REQUIRE(app.text_input_state().edit_state.target == asset_root.edit_state.target);
    MK_REQUIRE(app.services().platform_text_input_sessions_started == 2U);
    MK_REQUIRE(app.services().platform_text_input_sessions_ended == 1U);
}

MK_TEST("editor native shell first party IME composition and commit use shared UI text contracts") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    RecordingPlatformTextInputAdapter platform_text_input;
    RecordingImeAdapter ime;
    app.bind_native_services(mirakana::editor::NativeEditorServiceBindings{
        .platform_text_input_adapter = &platform_text_input,
        .ime_adapter = &ime,
    });

    auto target = mirakana::editor::make_native_editor_project_name_text_input_target("MIRAI");
    target.edit_state.cursor_byte_offset = target.edit_state.text.size();
    MK_REQUIRE(app.focus_text_input_target(target).succeeded());

    const auto composition = app.update_ime_composition(mirakana::ui::ImeComposition{
        .target = target.edit_state.target,
        .composition_text = "kana",
        .cursor_index = 4U,
    });

    MK_REQUIRE(composition.succeeded());
    MK_REQUIRE(ime.updates.size() == 1U);
    MK_REQUIRE(app.text_input_state().composition_active);
    MK_REQUIRE(app.text_input_state().composition.composition_text == "kana");
    MK_REQUIRE(app.text_input_state().edit_state.text == "MIRAI");
    MK_REQUIRE(app.services().ime_composition_updates == 1U);

    const auto commit = app.commit_text_input(mirakana::ui::CommittedTextInput{
        .target = target.edit_state.target,
        .text = "KANAI",
    });

    MK_REQUIRE(commit.succeeded());
    MK_REQUIRE(commit.state.text == "MIRAIKANAI");
    MK_REQUIRE(commit.state.cursor_byte_offset == commit.state.text.size());
    MK_REQUIRE(!app.text_input_state().composition_active);
    MK_REQUIRE(app.text_input_state().commit_applied);
    MK_REQUIRE(app.text_input_state().surrounding_text == "MIRAIKANAI");
    MK_REQUIRE(app.services().committed_text_inputs == 1U);
}

MK_TEST("editor native shell first party IME cancel clears composition without changing committed text") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    RecordingPlatformTextInputAdapter platform_text_input;
    RecordingImeAdapter ime;
    app.bind_native_services(mirakana::editor::NativeEditorServiceBindings{
        .platform_text_input_adapter = &platform_text_input,
        .ime_adapter = &ime,
    });

    auto target = mirakana::editor::make_native_editor_project_name_text_input_target("MIRAI");
    target.edit_state.cursor_byte_offset = target.edit_state.text.size();
    MK_REQUIRE(app.focus_text_input_target(target).succeeded());
    MK_REQUIRE(app.update_ime_composition(mirakana::ui::ImeComposition{
                                              .target = target.edit_state.target,
                                              .composition_text = "kana",
                                              .cursor_index = 4U,
                                          })
                   .succeeded());

    const auto canceled = app.cancel_ime_composition();

    MK_REQUIRE(canceled.succeeded());
    MK_REQUIRE(ime.updates.size() == 2U);
    MK_REQUIRE(ime.updates[1].composition_text.empty());
    MK_REQUIRE(!app.text_input_state().composition_active);
    MK_REQUIRE(app.text_input_state().edit_state.text == "MIRAI");
}

MK_TEST("editor native shell first party IME controller rejects invalid targets before adapter dispatch") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    RecordingPlatformTextInputAdapter platform_text_input;
    RecordingImeAdapter ime;
    app.bind_native_services(mirakana::editor::NativeEditorServiceBindings{
        .platform_text_input_adapter = &platform_text_input,
        .ime_adapter = &ime,
    });

    auto read_only = mirakana::editor::make_native_editor_project_name_text_input_target("MIRAIKANAI");
    read_only.editable = false;
    const auto read_only_focus = app.focus_text_input_target(read_only);

    MK_REQUIRE(!read_only_focus.succeeded());
    MK_REQUIRE(platform_text_input.begin_requests.empty());

    auto invalid_bounds = mirakana::editor::make_native_editor_project_name_text_input_target("MIRAIKANAI");
    invalid_bounds.caret_bounds.width = 0.0F;
    const auto invalid_focus = app.focus_text_input_target(invalid_bounds);

    MK_REQUIRE(!invalid_focus.succeeded());
    MK_REQUIRE(platform_text_input.begin_requests.empty());

    auto target = mirakana::editor::make_native_editor_project_name_text_input_target("MIRAIKANAI");
    MK_REQUIRE(app.focus_text_input_target(target).succeeded());

    const auto mismatched_composition = app.update_ime_composition(mirakana::ui::ImeComposition{
        .target = mirakana::ui::ElementId{.value = "editor.panel.assets.search"},
        .composition_text = "kana",
        .cursor_index = 4U,
    });

    MK_REQUIRE(!mismatched_composition.succeeded());
    MK_REQUIRE(ime.updates.empty());
}

MK_TEST("editor first party shell smoke counters expose value IME controller readiness") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};

    const auto shell_document = mirakana::editor::make_first_party_editor_document(app);
    const auto counters = mirakana::editor::make_first_party_editor_shell_smoke_counters(app, shell_document);

    MK_REQUIRE(counters.ime_status == "value_text_input_controller_ready");
    MK_REQUIRE(counters.ime_text_input_session_rows == 0U);
    MK_REQUIRE(counters.ime_composition_rows == 0U);
    MK_REQUIRE(counters.ime_committed_text_rows == 0U);
    MK_REQUIRE(counters.ime_caret_rect_rows == 1U);
    MK_REQUIRE(counters.ime_surrounding_text_rows == 1U);
    MK_REQUIRE(counters.ime_candidate_ui_host_owned);
    MK_REQUIRE(!counters.ime_native_handles_exposed);

    RecordingPlatformTextInputAdapter platform_text_input;
    app.bind_native_services(mirakana::editor::NativeEditorServiceBindings{
        .platform_text_input_adapter = &platform_text_input,
    });
    auto target = mirakana::editor::make_native_editor_project_name_text_input_target("MIRAI");
    target.edit_state.cursor_byte_offset = target.edit_state.text.size();
    MK_REQUIRE(app.focus_text_input_target(target).succeeded());
    MK_REQUIRE(app.commit_text_input(mirakana::ui::CommittedTextInput{
                                         .target = target.edit_state.target,
                                         .text = "KANAI",
                                     })
                   .succeeded());

    const auto committed_counters = mirakana::editor::make_first_party_editor_shell_smoke_counters(app, shell_document);
    MK_REQUIRE(committed_counters.ime_status == "value_text_input_commit_applied");
    MK_REQUIRE(committed_counters.ime_text_input_session_rows == 1U);
    MK_REQUIRE(committed_counters.ime_committed_text_rows == 1U);
}

MK_TEST("editor native shell app records deterministic panel smoke counters") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};

    MK_REQUIRE(app.panels_rendered_last_frame() == 0U);
    MK_REQUIRE(app.docking_status_last_frame() == "not_rendered");
    MK_REQUIRE(app.dock_tab_headers_last_frame() == 0U);
    MK_REQUIRE(app.dock_split_gutters_last_frame() == 0U);
    MK_REQUIRE(app.dock_active_panels_last_frame() == 0U);
    MK_REQUIRE(app.dock_focusable_controls_last_frame() == 0U);

    app.record_native_panels_rendered(12U);
    app.record_native_docking_frame("single_window_ready", 12U, 3U, 4U, 12U);

    MK_REQUIRE(app.panels_rendered_last_frame() == 12U);
    MK_REQUIRE(app.docking_status_last_frame() == "single_window_ready");
    MK_REQUIRE(app.dock_tab_headers_last_frame() == 12U);
    MK_REQUIRE(app.dock_split_gutters_last_frame() == 3U);
    MK_REQUIRE(app.dock_active_panels_last_frame() == 4U);
    MK_REQUIRE(app.dock_focusable_controls_last_frame() == 12U);
}

MK_TEST("editor first party shell smoke counters expose visible texture readiness") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    const auto viewport_plan =
        mirakana::editor::plan_native_viewport_display(mirakana::editor::NativeViewportDisplayDesc{
            .d3d12_host_available = true,
            .renderer_output_available = true,
            .texture_display_requested = true,
            .texture_adapter_available = true,
            .offscreen_target_available = true,
            .descriptor_lease_available = true,
            .resource_barriers_recorded = true,
            .fence_lifecycle_ready = true,
            .visible_panel_available = true,
            .visible_texture_composite_recorded = true,
            .visible_texture_composites = 1U,
            .extent = mirakana::editor::ViewportExtent{.width = 1280, .height = 720},
            .frame_index = 26U,
            .backend_id = "d3d12",
        });
    const auto material_plan =
        mirakana::editor::plan_native_material_preview_display(mirakana::editor::NativeMaterialPreviewDisplayDesc{
            .d3d12_host_available = true,
            .shader_artifacts_available = true,
            .gpu_payload_available = true,
            .texture_display_requested = true,
            .texture_adapter_available = true,
            .offscreen_target_available = true,
            .descriptor_lease_available = true,
            .resource_barriers_recorded = true,
            .fence_lifecycle_ready = true,
            .visible_panel_available = true,
            .visible_texture_composite_recorded = true,
            .visible_texture_composites = 1U,
            .frame_index = 26U,
            .backend_id = "d3d12",
            .frames_rendered = 1U,
            .executes = true,
        });

    app.record_native_viewport_texture_display(viewport_plan);
    app.record_native_material_preview_texture_display(material_plan);

    const auto shell_document = mirakana::editor::make_first_party_editor_document(app);
    const auto counters = mirakana::editor::make_first_party_editor_shell_smoke_counters(app, shell_document);
    const auto* viewport_status =
        shell_document.document.find(mirakana::ui::ElementId{.value = "editor.panel.viewport.status"});

    MK_REQUIRE(viewport_status != nullptr);
    MK_REQUIRE(viewport_status->text.label.contains("d3d12_texture_ready"));
    MK_REQUIRE(counters.viewport_status == "d3d12_texture_ready");
    MK_REQUIRE(counters.viewport_visible_texture_composites == 1U);
    MK_REQUIRE(!counters.viewport_native_handles_exposed);
    MK_REQUIRE(counters.material_preview_status == "d3d12_texture_ready");
    MK_REQUIRE(counters.material_preview_visible_texture_composites == 1U);
    MK_REQUIRE(!counters.material_preview_native_handles_exposed);
}

MK_TEST("editor first party shell exposes AI operation UX rows from native readiness") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};
    RecordingPlatformTextInputAdapter platform_text_input;
    RecordingImeAdapter ime;
    auto accessibility = mirakana::editor::make_native_editor_uia_accessibility_adapter();
    app.bind_native_services(mirakana::editor::NativeEditorServiceBindings{
        .platform_text_input_adapter = &platform_text_input,
        .ime_adapter = &ime,
        .accessibility_adapter = accessibility.get(),
        .platform_text_input_service_id = "win32_tsf",
        .ime_service_id = "win32_tsf",
        .accessibility_service_id = "win32_uia",
    });
    app.record_native_text_atlas_handoff_evidence(mirakana::editor::NativeEditorTextAtlasHandoffEvidence{
        .status = "glyphs_ready_atlas_handoff_host_gated",
        .text_shaping_adapter_invoked = true,
        .font_rasterizer_adapter_invoked = true,
        .glyphs_ready = true,
        .fallback_used = false,
        .atlas_handoff_ready = false,
        .native_handles_exposed = false,
        .host_gated_rows = 1U,
        .unsupported_rows = 1U,
    });
    const auto viewport_plan =
        mirakana::editor::plan_native_viewport_display(mirakana::editor::NativeViewportDisplayDesc{
            .d3d12_host_available = true,
            .renderer_output_available = true,
            .texture_display_requested = true,
            .texture_adapter_available = true,
            .offscreen_target_available = true,
            .descriptor_lease_available = true,
            .resource_barriers_recorded = true,
            .fence_lifecycle_ready = true,
            .visible_panel_available = true,
            .visible_texture_composite_recorded = true,
            .visible_texture_composites = 3U,
            .extent = mirakana::editor::ViewportExtent{.width = 1280, .height = 720},
            .frame_index = 28U,
            .backend_id = "d3d12",
        });
    const auto material_plan =
        mirakana::editor::plan_native_material_preview_display(mirakana::editor::NativeMaterialPreviewDisplayDesc{
            .d3d12_host_available = true,
            .shader_artifacts_available = true,
            .gpu_payload_available = true,
            .texture_display_requested = true,
            .texture_adapter_available = true,
            .offscreen_target_available = true,
            .descriptor_lease_available = true,
            .resource_barriers_recorded = true,
            .fence_lifecycle_ready = true,
            .visible_panel_available = true,
            .visible_texture_composite_recorded = true,
            .visible_texture_composites = 3U,
            .frame_index = 28U,
            .backend_id = "d3d12",
            .frames_rendered = 1U,
            .executes = true,
        });
    app.record_native_viewport_texture_display(viewport_plan);
    app.record_native_material_preview_texture_display(material_plan);

    const auto published_document = mirakana::editor::make_first_party_editor_document(app);
    const auto payload = mirakana::ui::build_accessibility_payload(published_document.renderer_submission);
    MK_REQUIRE(app.publish_native_accessibility_payload(payload, published_document.focused_element).published);
    const auto shell_document = mirakana::editor::make_first_party_editor_document(app);
    const auto snapshot = mirakana::editor::make_first_party_editor_ai_operation_snapshot(app, shell_document);
    const auto* text_input = find_ai_operation_status_row(snapshot, "editor.ai.text_input.focused_target");
    const auto* accessibility_row = find_ai_operation_status_row(snapshot, "editor.ai.accessibility.uia_provider");
    const auto* viewport = find_ai_operation_status_row(snapshot, "editor.ai.viewport.display");
    const auto* material = find_ai_operation_status_row(snapshot, "editor.ai.material_preview.display");

    MK_REQUIRE(text_input != nullptr);
    MK_REQUIRE(text_input->status == "win32_tsf_selected");
    MK_REQUIRE(text_input->target_element_id == "editor.panel.project_settings.name.text_field");
    MK_REQUIRE(!text_input->native_handles_public);
    MK_REQUIRE(accessibility_row != nullptr);
    MK_REQUIRE(accessibility_row->status == "uia_provider_ready");
    MK_REQUIRE(accessibility_row->ready);
    MK_REQUIRE(accessibility_row->count == app.accessibility_state().nodes.size());
    MK_REQUIRE(!accessibility_row->native_handles_public);
    MK_REQUIRE(viewport != nullptr);
    MK_REQUIRE(viewport->status == "d3d12_texture_ready");
    MK_REQUIRE(viewport->count == 3U);
    MK_REQUIRE(!viewport->native_handles_public);
    MK_REQUIRE(material != nullptr);
    MK_REQUIRE(material->status == "d3d12_texture_ready");
    MK_REQUIRE(material->count == 3U);
    MK_REQUIRE(!material->native_handles_public);
    MK_REQUIRE(find_ai_operation_status_row(snapshot, "editor.ai.validation_recipe.execution") == nullptr);
}

MK_TEST("editor first party shell exposes environment artist workflow execution bridge rows") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};

    const auto shell_document = mirakana::editor::make_first_party_editor_document(app);
    const auto counters = mirakana::editor::make_first_party_editor_shell_smoke_counters(app, shell_document);

    MK_REQUIRE(contains_element(shell_document.document, "environment_artist_workflow_shell_execution_bridge"));
    MK_REQUIRE(contains_element(shell_document.document,
                                "environment_artist_workflow_shell_execution_bridge.command_plans.environment.command."
                                "source_asset.review.dry_run_status"));
    MK_REQUIRE(contains_element(shell_document.document,
                                "environment_artist_workflow_shell_execution_bridge.command_plans.environment.command."
                                "cook.preview.apply_revision_checked"));
    MK_REQUIRE(contains_element(shell_document.document,
                                "environment_artist_workflow_shell_execution_bridge.command_plans.environment.command."
                                "package.preview.rollback_metadata_available"));
    MK_REQUIRE(contains_element(shell_document.document,
                                "environment_artist_workflow_shell_execution_bridge.command_plans.environment.command."
                                "validation.remediation.apply_status"));
    MK_REQUIRE(contains_element(shell_document.document,
                                "environment_artist_workflow_shell_execution_bridge.command_plans.environment.command."
                                "publish.package.requires_confirmation"));
    MK_REQUIRE(contains_element(shell_document.document,
                                "environment_artist_workflow_execution_review.rows.environment.workflow.execution."
                                "external_execution.status"));
    MK_REQUIRE(contains_element(shell_document.document,
                                "environment_artist_workflow_execution_review.rows.environment.workflow.execution."
                                "operator_review.status"));
    MK_REQUIRE(contains_element(shell_document.document,
                                "environment_artist_workflow_execution_review.rows.environment.workflow.execution."
                                "ready_promotion_guard.value"));

    MK_REQUIRE(counters.environment_artist_workflow_command_plan_rows == 5U);
    MK_REQUIRE(counters.environment_artist_workflow_execution_review_rows == 8U);
    MK_REQUIRE(counters.environment_artist_workflow_external_execution_rows == 1U);
    MK_REQUIRE(counters.environment_artist_workflow_operator_review_rows == 1U);
    MK_REQUIRE(!counters.environment_artist_workflow_executes_backend);
    MK_REQUIRE(!counters.environment_artist_workflow_executes_package_scripts);
    MK_REQUIRE(!counters.environment_artist_workflow_executes_validation_recipes);
    MK_REQUIRE(!counters.environment_artist_workflow_native_handles_exposed);
    MK_REQUIRE(!counters.environment_artist_workflow_ready_claimed);
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

MK_TEST("editor native viewport display plan rejects zero extent before texture readiness") {
    const auto plan = mirakana::editor::plan_native_viewport_display(mirakana::editor::NativeViewportDisplayDesc{
        .d3d12_host_available = true,
        .renderer_output_available = true,
        .texture_adapter_available = true,
        .offscreen_target_available = true,
        .descriptor_lease_available = true,
        .resource_barriers_recorded = true,
        .fence_lifecycle_ready = true,
        .resize_safe_teardown_completed = true,
        .extent = mirakana::editor::ViewportExtent{.width = 0, .height = 720},
        .frame_index = 8,
    });

    MK_REQUIRE(!plan.accepted);
    MK_REQUIRE(plan.status_id == "invalid_extent");
    MK_REQUIRE(!plan.texture_display_ready);
    MK_REQUIRE(!plan.native_texture_handles_exposed);
    MK_REQUIRE(plan.lifecycle_status == "invalid_extent");
}

MK_TEST("editor native viewport display plan waits for requested texture adapter") {
    const auto plan = mirakana::editor::plan_native_viewport_display(mirakana::editor::NativeViewportDisplayDesc{
        .d3d12_host_available = true,
        .renderer_output_available = true,
        .texture_display_requested = true,
        .extent = mirakana::editor::ViewportExtent{.width = 1280, .height = 720},
        .frame_index = 9,
    });

    MK_REQUIRE(plan.accepted);
    MK_REQUIRE(plan.status_id == "texture_adapter_unavailable");
    MK_REQUIRE(plan.texture_display_requested);
    MK_REQUIRE(!plan.texture_adapter_available);
    MK_REQUIRE(!plan.texture_display_ready);
    MK_REQUIRE(plan.lifecycle_status == "adapter_pending");
    MK_REQUIRE(plan.diagnostic.contains("texture adapter"));
}

MK_TEST("editor native viewport display plan waits for resize-safe teardown") {
    const auto plan = mirakana::editor::plan_native_viewport_display(mirakana::editor::NativeViewportDisplayDesc{
        .d3d12_host_available = true,
        .renderer_output_available = true,
        .texture_display_requested = true,
        .texture_adapter_available = true,
        .offscreen_target_available = true,
        .descriptor_lease_available = true,
        .resource_barriers_recorded = true,
        .fence_lifecycle_ready = true,
        .resize_safe_teardown_completed = false,
        .resize_recreate_required = true,
        .extent = mirakana::editor::ViewportExtent{.width = 1366, .height = 768},
        .frame_index = 9,
    });

    MK_REQUIRE(plan.accepted);
    MK_REQUIRE(plan.status_id == "resize_recreate_required");
    MK_REQUIRE(!plan.texture_display_ready);
    MK_REQUIRE(plan.resize_recreate_required);
    MK_REQUIRE(!plan.resize_safe_teardown_completed);
    MK_REQUIRE(plan.lifecycle_status == "resize_pending");
    MK_REQUIRE(plan.diagnostic.contains("resize-safe teardown"));
}

MK_TEST("editor native viewport display plan reports private d3d12 texture readiness") {
    const auto plan = mirakana::editor::plan_native_viewport_display(mirakana::editor::NativeViewportDisplayDesc{
        .d3d12_host_available = true,
        .renderer_output_available = true,
        .texture_display_requested = true,
        .texture_adapter_available = true,
        .offscreen_target_available = true,
        .descriptor_lease_available = true,
        .resource_barriers_recorded = true,
        .fence_lifecycle_ready = true,
        .resize_safe_teardown_completed = true,
        .visible_texture_composite_recorded = true,
        .visible_texture_composites = 1U,
        .extent = mirakana::editor::ViewportExtent{.width = 1280, .height = 720},
        .frame_index = 10,
    });

    MK_REQUIRE(plan.accepted);
    MK_REQUIRE(plan.status_id == "d3d12_texture_ready");
    MK_REQUIRE(plan.texture_display_ready);
    MK_REQUIRE(plan.texture_display_requested);
    MK_REQUIRE(plan.texture_adapter_available);
    MK_REQUIRE(plan.offscreen_target_available);
    MK_REQUIRE(plan.descriptor_lease_available);
    MK_REQUIRE(plan.resource_barriers_recorded);
    MK_REQUIRE(plan.fence_lifecycle_ready);
    MK_REQUIRE(plan.resize_safe_teardown_completed);
    MK_REQUIRE(plan.lifecycle_status == "ready");
    MK_REQUIRE(!plan.native_texture_handles_exposed);
    MK_REQUIRE(plan.native_texture_handle_policy == "private");
}

MK_TEST("editor native viewport display plan waits for visible compositor consumption") {
    const auto plan = mirakana::editor::plan_native_viewport_display(mirakana::editor::NativeViewportDisplayDesc{
        .d3d12_host_available = true,
        .renderer_output_available = true,
        .texture_display_requested = true,
        .texture_adapter_available = true,
        .offscreen_target_available = true,
        .descriptor_lease_available = true,
        .resource_barriers_recorded = true,
        .fence_lifecycle_ready = true,
        .resize_safe_teardown_completed = true,
        .extent = mirakana::editor::ViewportExtent{.width = 1280, .height = 720},
        .frame_index = 10,
    });

    MK_REQUIRE(plan.accepted);
    MK_REQUIRE(plan.status_id == "visible_composite_pending");
    MK_REQUIRE(!plan.texture_display_ready);
    MK_REQUIRE(plan.texture_display_requested);
    MK_REQUIRE(plan.texture_adapter_available);
    MK_REQUIRE(plan.offscreen_target_available);
    MK_REQUIRE(plan.descriptor_lease_available);
    MK_REQUIRE(plan.resource_barriers_recorded);
    MK_REQUIRE(plan.fence_lifecycle_ready);
    MK_REQUIRE(plan.lifecycle_status == "presentation_pending");
    MK_REQUIRE(!plan.native_texture_handles_exposed);
    MK_REQUIRE(plan.native_texture_handle_policy == "private");
}

MK_TEST("editor native viewport display plan does not expose native texture handles") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};

    app.record_native_viewport_d3d12_host_ready(9U);

    MK_REQUIRE(app.viewport_display().status_id == "diagnostic_only");
    MK_REQUIRE(!app.viewport_display().native_texture_handles_exposed);
    MK_REQUIRE(app.viewport_display().native_texture_handle_policy == "private");
    MK_REQUIRE(app.viewport().renderer_name() == "d3d12");
}

MK_TEST("editor native texture display adapter prepares viewport display frame through rhi descriptors") {
    mirakana::rhi::NullRhiDevice device;
    mirakana::editor::NativeTextureDisplayAdapter adapter(mirakana::editor::NativeTextureDisplayAdapterDesc{
        .device = &device,
        .extent = mirakana::editor::ViewportExtent{.width = 64, .height = 36},
        .d3d12_host_available = true,
        .renderer_output_available = true,
        .backend_id = "d3d12",
    });

    const auto plan = adapter.render_viewport_frame(17U);
    const auto& evidence = adapter.evidence();

    MK_REQUIRE(plan.accepted);
    MK_REQUIRE(plan.status_id == "visible_composite_pending");
    MK_REQUIRE(!plan.texture_display_ready);
    MK_REQUIRE(plan.texture_display_requested);
    MK_REQUIRE(plan.texture_adapter_available);
    MK_REQUIRE(plan.offscreen_target_available);
    MK_REQUIRE(plan.descriptor_lease_available);
    MK_REQUIRE(plan.resource_barriers_recorded);
    MK_REQUIRE(plan.fence_lifecycle_ready);
    MK_REQUIRE(plan.lifecycle_status == "presentation_pending");
    MK_REQUIRE(!plan.native_texture_handles_exposed);
    MK_REQUIRE(plan.frame_index == 17U);
    MK_REQUIRE(evidence.frames_rendered == 1U);
    MK_REQUIRE(evidence.descriptor_writes >= 1U);
    MK_REQUIRE(evidence.resource_transitions >= 1U);
    MK_REQUIRE(evidence.fence_waits >= 1U);
    MK_REQUIRE(device.stats().texture_buffer_copies == 0U);
    MK_REQUIRE(device.stats().buffer_reads == 0U);
}

MK_TEST("editor native texture display adapter keeps prepared viewport frame pending until visible composite") {
    mirakana::rhi::NullRhiDevice device;
    mirakana::editor::NativeTextureDisplayAdapter adapter(mirakana::editor::NativeTextureDisplayAdapterDesc{
        .device = &device,
        .extent = mirakana::editor::ViewportExtent{.width = 64, .height = 36},
        .d3d12_host_available = true,
        .renderer_output_available = true,
        .backend_id = "d3d12",
    });

    const auto plan = adapter.render_viewport_frame(17U);
    const auto& evidence = adapter.evidence();

    MK_REQUIRE(plan.accepted);
    MK_REQUIRE(plan.status_id == "visible_composite_pending");
    MK_REQUIRE(!plan.texture_display_ready);
    MK_REQUIRE(plan.texture_display_requested);
    MK_REQUIRE(plan.texture_adapter_available);
    MK_REQUIRE(plan.offscreen_target_available);
    MK_REQUIRE(plan.descriptor_lease_available);
    MK_REQUIRE(plan.resource_barriers_recorded);
    MK_REQUIRE(plan.fence_lifecycle_ready);
    MK_REQUIRE(!plan.native_texture_handles_exposed);
    MK_REQUIRE(plan.frame_index == 17U);
    MK_REQUIRE(evidence.frames_rendered == 1U);
    MK_REQUIRE(evidence.descriptor_writes >= 1U);
    MK_REQUIRE(evidence.resource_transitions >= 1U);
    MK_REQUIRE(evidence.fence_waits >= 1U);
}

MK_TEST("editor native texture display adapter waits before resize replacement") {
    mirakana::rhi::NullRhiDevice device;
    mirakana::editor::NativeTextureDisplayAdapter adapter(mirakana::editor::NativeTextureDisplayAdapterDesc{
        .device = &device,
        .extent = mirakana::editor::ViewportExtent{.width = 32, .height = 18},
        .d3d12_host_available = true,
        .renderer_output_available = true,
        .backend_id = "d3d12",
    });

    const auto first = adapter.render_viewport_frame(1U);
    adapter.resize(mirakana::editor::ViewportExtent{.width = 80, .height = 45});
    const auto resized = adapter.render_viewport_frame(2U);
    const auto& evidence = adapter.evidence();

    MK_REQUIRE(first.status_id == "visible_composite_pending");
    MK_REQUIRE(!first.texture_display_ready);
    MK_REQUIRE(resized.status_id == "visible_composite_pending");
    MK_REQUIRE(!resized.texture_display_ready);
    MK_REQUIRE(resized.resize_recreate_required);
    MK_REQUIRE(resized.resize_safe_teardown_completed);
    MK_REQUIRE(resized.extent.width == 80U);
    MK_REQUIRE(resized.extent.height == 45U);
    MK_REQUIRE(evidence.resize_recreate_required);
    MK_REQUIRE(evidence.resize_safe_teardown_completed);
    MK_REQUIRE(evidence.frames_rendered == 2U);
    MK_REQUIRE(device.stats().fence_waits >= device.stats().fences_signaled);
}

MK_TEST("editor native texture display adapter prepares material preview execution evidence") {
    mirakana::rhi::NullRhiDevice device;
    mirakana::editor::NativeTextureDisplayAdapter adapter(mirakana::editor::NativeTextureDisplayAdapterDesc{
        .device = &device,
        .extent = mirakana::editor::ViewportExtent{.width = 96, .height = 96},
        .d3d12_host_available = true,
        .shader_artifacts_available = true,
        .gpu_payload_available = true,
        .backend_id = "d3d12",
    });

    const auto plan = adapter.render_material_preview_frame(23U);
    const auto& evidence = adapter.evidence();

    MK_REQUIRE(plan.accepted);
    MK_REQUIRE(plan.status_id == "visible_panel_unavailable");
    MK_REQUIRE(!plan.texture_display_ready);
    MK_REQUIRE(plan.execution_snapshot.status == mirakana::editor::EditorMaterialGpuPreviewStatus::rhi_unavailable);
    MK_REQUIRE(plan.execution_snapshot.frames_rendered == 0U);
    MK_REQUIRE(!plan.execution_snapshot.executes);
    MK_REQUIRE(plan.execution_snapshot.display_path_label == "host-private-native");
    MK_REQUIRE(!plan.execution_snapshot.exposes_native_handles);
    MK_REQUIRE(evidence.frames_rendered == 1U);
}

MK_TEST("editor visible texture compositor samples viewport texture into swapchain before readiness") {
    mirakana::rhi::NullRhiDevice device;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 36},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = false,
        .surface = mirakana::rhi::SurfaceHandle{.value = 1U},
    });
    constexpr std::array<std::uint8_t, 4> shader_bytecode{0x4d, 0x4b, 0x43, 0x00};
    mirakana::editor::NativeTextureDisplayAdapter adapter(mirakana::editor::NativeTextureDisplayAdapterDesc{
        .device = &device,
        .extent = mirakana::editor::ViewportExtent{.width = 64, .height = 36},
        .d3d12_host_available = true,
        .renderer_output_available = true,
        .backend_id = "d3d12",
    });
    mirakana::editor::NativeEditorVisibleTextureCompositor compositor(
        mirakana::editor::NativeEditorVisibleTextureCompositorDesc{
            .device = &device,
            .swapchain = swapchain,
            .extent = mirakana::editor::ViewportExtent{.width = 64, .height = 36},
            .vertex_shader_entry_point = "vs_native_editor_visible_texture",
            .vertex_shader_bytecode = shader_bytecode,
            .fragment_shader_entry_point = "ps_native_editor_visible_texture",
            .fragment_shader_bytecode = shader_bytecode,
            .backend_id = "d3d12",
        });

    const auto plan = compositor.render_viewport_frame(adapter, 24U);
    const auto& evidence = compositor.evidence();

    MK_REQUIRE(plan.accepted);
    MK_REQUIRE(plan.status_id == "d3d12_texture_ready");
    MK_REQUIRE(plan.texture_display_ready);
    MK_REQUIRE(plan.visible_panel_available);
    MK_REQUIRE(plan.visible_texture_composite_recorded);
    MK_REQUIRE(plan.visible_texture_composites == 1U);
    MK_REQUIRE(plan.frame_index == 24U);
    MK_REQUIRE(!plan.native_texture_handles_exposed);
    MK_REQUIRE(evidence.swapchain_frame_acquired);
    MK_REQUIRE(evidence.render_pass_recorded);
    MK_REQUIRE(evidence.sampled_texture_descriptor_bound);
    MK_REQUIRE(evidence.draw_recorded);
    MK_REQUIRE(evidence.present_recorded);
    MK_REQUIRE(evidence.fence_waited);
    MK_REQUIRE(device.stats().swapchain_frames_acquired >= 1U);
    MK_REQUIRE(device.stats().render_passes_begun >= 1U);
    MK_REQUIRE(device.stats().descriptor_sets_bound >= 1U);
    MK_REQUIRE(device.stats().draw_calls >= 1U);
    MK_REQUIRE(device.stats().present_calls >= 1U);
}

MK_TEST("editor visible texture compositor promotes material preview only after visible panel composite") {
    mirakana::rhi::NullRhiDevice device;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 96, .height = 96},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = false,
        .surface = mirakana::rhi::SurfaceHandle{.value = 2U},
    });
    constexpr std::array<std::uint8_t, 4> shader_bytecode{0x4d, 0x4b, 0x43, 0x00};
    mirakana::editor::NativeTextureDisplayAdapter adapter(mirakana::editor::NativeTextureDisplayAdapterDesc{
        .device = &device,
        .extent = mirakana::editor::ViewportExtent{.width = 96, .height = 96},
        .d3d12_host_available = true,
        .shader_artifacts_available = true,
        .gpu_payload_available = true,
        .backend_id = "d3d12",
    });
    mirakana::editor::NativeEditorVisibleTextureCompositor compositor(
        mirakana::editor::NativeEditorVisibleTextureCompositorDesc{
            .device = &device,
            .swapchain = swapchain,
            .extent = mirakana::editor::ViewportExtent{.width = 96, .height = 96},
            .vertex_shader_entry_point = "vs_native_editor_visible_texture",
            .vertex_shader_bytecode = shader_bytecode,
            .fragment_shader_entry_point = "ps_native_editor_visible_texture",
            .fragment_shader_bytecode = shader_bytecode,
            .backend_id = "d3d12",
        });

    const auto plan = compositor.render_material_preview_frame(adapter, 25U);

    MK_REQUIRE(plan.accepted);
    MK_REQUIRE(plan.status_id == "d3d12_texture_ready");
    MK_REQUIRE(plan.texture_display_ready);
    MK_REQUIRE(plan.visible_panel_available);
    MK_REQUIRE(plan.visible_texture_composite_recorded);
    MK_REQUIRE(plan.visible_texture_composites == 1U);
    MK_REQUIRE(plan.execution_snapshot.status == mirakana::editor::EditorMaterialGpuPreviewStatus::ready);
    MK_REQUIRE(plan.execution_snapshot.frames_rendered >= 1U);
    MK_REQUIRE(plan.execution_snapshot.executes);
    MK_REQUIRE(!plan.execution_snapshot.exposes_native_handles);
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

MK_TEST("editor native material preview plan waits for descriptor and fence lifecycle") {
    const auto plan =
        mirakana::editor::plan_native_material_preview_display(mirakana::editor::NativeMaterialPreviewDisplayDesc{
            .d3d12_host_available = true,
            .shader_artifacts_available = true,
            .gpu_payload_available = true,
            .texture_display_requested = true,
            .texture_adapter_available = true,
            .offscreen_target_available = true,
            .descriptor_lease_available = false,
            .resource_barriers_recorded = true,
            .fence_lifecycle_ready = true,
            .frame_index = 15,
        });

    MK_REQUIRE(plan.accepted);
    MK_REQUIRE(plan.status_id == "descriptor_lease_unavailable");
    MK_REQUIRE(!plan.texture_display_ready);
    MK_REQUIRE(plan.texture_display_requested);
    MK_REQUIRE(plan.texture_adapter_available);
    MK_REQUIRE(plan.offscreen_target_available);
    MK_REQUIRE(!plan.descriptor_lease_available);
    MK_REQUIRE(plan.resource_barriers_recorded);
    MK_REQUIRE(plan.fence_lifecycle_ready);
    MK_REQUIRE(plan.lifecycle_status == "descriptor_pending");
    MK_REQUIRE(plan.execution_snapshot.status == mirakana::editor::EditorMaterialGpuPreviewStatus::rhi_unavailable);
    MK_REQUIRE(!plan.execution_snapshot.executes);
    MK_REQUIRE(!plan.execution_snapshot.exposes_native_handles);
}

MK_TEST("editor native material preview plan waits for requested texture adapter") {
    const auto plan =
        mirakana::editor::plan_native_material_preview_display(mirakana::editor::NativeMaterialPreviewDisplayDesc{
            .d3d12_host_available = true,
            .shader_artifacts_available = true,
            .gpu_payload_available = true,
            .texture_display_requested = true,
            .frame_index = 15,
        });

    MK_REQUIRE(plan.accepted);
    MK_REQUIRE(plan.status_id == "texture_adapter_unavailable");
    MK_REQUIRE(plan.texture_display_requested);
    MK_REQUIRE(!plan.texture_adapter_available);
    MK_REQUIRE(!plan.texture_display_ready);
    MK_REQUIRE(plan.lifecycle_status == "adapter_pending");
    MK_REQUIRE(plan.execution_snapshot.status == mirakana::editor::EditorMaterialGpuPreviewStatus::rhi_unavailable);
    MK_REQUIRE(!plan.execution_snapshot.executes);
    MK_REQUIRE(!plan.execution_snapshot.exposes_native_handles);
}

MK_TEST("editor native material preview plan reports private d3d12 texture readiness") {
    const auto plan =
        mirakana::editor::plan_native_material_preview_display(mirakana::editor::NativeMaterialPreviewDisplayDesc{
            .d3d12_host_available = true,
            .shader_artifacts_available = true,
            .gpu_payload_available = true,
            .texture_display_requested = true,
            .texture_adapter_available = true,
            .offscreen_target_available = true,
            .descriptor_lease_available = true,
            .resource_barriers_recorded = true,
            .fence_lifecycle_ready = true,
            .visible_panel_available = true,
            .visible_texture_composite_recorded = true,
            .visible_texture_composites = 1U,
            .frame_index = 16,
            .frames_rendered = 1U,
            .executes = false,
        });

    MK_REQUIRE(plan.accepted);
    MK_REQUIRE(plan.status_id == "d3d12_texture_ready");
    MK_REQUIRE(plan.texture_display_ready);
    MK_REQUIRE(plan.texture_display_requested);
    MK_REQUIRE(plan.texture_adapter_available);
    MK_REQUIRE(plan.offscreen_target_available);
    MK_REQUIRE(plan.descriptor_lease_available);
    MK_REQUIRE(plan.resource_barriers_recorded);
    MK_REQUIRE(plan.fence_lifecycle_ready);
    MK_REQUIRE(plan.lifecycle_status == "ready");
    MK_REQUIRE(!plan.native_texture_handles_exposed);
    MK_REQUIRE(plan.native_texture_handle_policy == "private");
    MK_REQUIRE(plan.execution_snapshot.status == mirakana::editor::EditorMaterialGpuPreviewStatus::ready);
    MK_REQUIRE(plan.execution_snapshot.frames_rendered == 1U);
    MK_REQUIRE(plan.execution_snapshot.display_path_label == "host-private-native");
    MK_REQUIRE(!plan.execution_snapshot.executes);
    MK_REQUIRE(!plan.execution_snapshot.exposes_native_handles);

    mirakana::editor::EditorMaterialAssetPreviewPanelModel model;
    mirakana::editor::apply_editor_material_gpu_preview_execution_snapshot(model, plan.execution_snapshot);
    MK_REQUIRE(model.gpu_execution_ready);
    MK_REQUIRE(model.gpu_execution_rendered);
    MK_REQUIRE(model.gpu_execution_frames_rendered == 1U);
    MK_REQUIRE(!model.executes);
    MK_REQUIRE(!model.exposes_native_handles);
}

MK_TEST("editor native material preview plan waits for visible preview panel before texture readiness") {
    const auto plan =
        mirakana::editor::plan_native_material_preview_display(mirakana::editor::NativeMaterialPreviewDisplayDesc{
            .d3d12_host_available = true,
            .shader_artifacts_available = true,
            .gpu_payload_available = true,
            .texture_display_requested = true,
            .texture_adapter_available = true,
            .offscreen_target_available = true,
            .descriptor_lease_available = true,
            .resource_barriers_recorded = true,
            .fence_lifecycle_ready = true,
            .frame_index = 16,
            .frames_rendered = 1U,
            .executes = true,
        });

    MK_REQUIRE(plan.accepted);
    MK_REQUIRE(plan.status_id == "visible_panel_unavailable");
    MK_REQUIRE(!plan.texture_display_ready);
    MK_REQUIRE(plan.lifecycle_status == "panel_pending");
    MK_REQUIRE(!plan.native_texture_handles_exposed);
    MK_REQUIRE(plan.native_texture_handle_policy == "private");
    MK_REQUIRE(plan.execution_snapshot.status == mirakana::editor::EditorMaterialGpuPreviewStatus::rhi_unavailable);
    MK_REQUIRE(!plan.execution_snapshot.executes);
    MK_REQUIRE(!plan.execution_snapshot.exposes_native_handles);
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

MK_TEST("editor native shell routes material preview evidence into asset browser preview rows") {
    mirakana::editor::NativeEditorApp app{mirakana::editor::NativeEditorLaunchOptions{}};

    const auto plan =
        mirakana::editor::plan_native_material_preview_display(mirakana::editor::NativeMaterialPreviewDisplayDesc{
            .d3d12_host_available = true,
            .shader_artifacts_available = true,
            .gpu_payload_available = true,
            .texture_display_requested = true,
            .texture_adapter_available = true,
            .offscreen_target_available = true,
            .descriptor_lease_available = true,
            .resource_barriers_recorded = true,
            .fence_lifecycle_ready = true,
            .visible_panel_available = true,
            .visible_texture_composite_recorded = true,
            .visible_texture_composites = 1U,
            .frame_index = 33U,
            .backend_id = "d3d12",
            .frames_rendered = 3U,
            .executes = false,
        });

    app.record_native_material_preview_texture_display(plan);

    const auto& asset_browser = app.asset_browser();
    const auto material_row = std::ranges::find_if(asset_browser.preview_rows,
                                                   [](const auto& row) { return row.preview_kind == "material"; });
    MK_REQUIRE(material_row != asset_browser.preview_rows.end());
    MK_REQUIRE(material_row->backend_label == "D3D12");
    MK_REQUIRE(material_row->display_path_label == "host-private-native");
    MK_REQUIRE(material_row->frame_or_sample_count == 3U);
    MK_REQUIRE(material_row->ready);
    MK_REQUIRE(material_row->host_owned);
    MK_REQUIRE(!material_row->exposes_native_handles);
    MK_REQUIRE(!asset_browser.executes);
    MK_REQUIRE(!asset_browser.exposes_native_handles);
    MK_REQUIRE(!asset_browser.uploads_gpu_resources);
    MK_REQUIRE(std::ranges::any_of(asset_browser.preview_rows, [](const auto& row) {
        return row.preview_kind == "thumbnail_material" && row.status_label == "host_request_queued";
    }));
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
    MK_REQUIRE(app.services().asset_import_filesystem_id == "unbound");
    MK_REQUIRE(!app.services().asset_import_filesystem_available);
    MK_REQUIRE(app.services().user_confirmation_required_for_process_execution);
}

int main() {
    return mirakana::test::run_all();
}
