// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "native_editor_app.hpp"

#include "mirakana/core/diagnostics.hpp"
#include "mirakana/scene/scene.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::editor {
namespace {

struct NativePanelToken {
    std::string_view id;
};

constexpr std::array<NativePanelToken, 10> native_panel_tokens{{
    NativePanelToken{.id = "main_menu"},
    NativePanelToken{.id = "scene"},
    NativePanelToken{.id = "inspector"},
    NativePanelToken{.id = "assets"},
    NativePanelToken{.id = "console"},
    NativePanelToken{.id = "resources"},
    NativePanelToken{.id = "ai_commands"},
    NativePanelToken{.id = "profiler"},
    NativePanelToken{.id = "timeline"},
    NativePanelToken{.id = "project_settings"},
}};

[[nodiscard]] ProjectDocument make_default_project_document() {
    return ProjectDocument{
        .name = "MIRAIKANAI Editor",
        .root_path = ".",
        .asset_root = "assets",
        .source_registry_path = "source/assets/package.geassets",
        .game_manifest_path = "game.agent.json",
        .startup_scene_path = "scenes/start.scene",
    };
}

[[nodiscard]] Workspace make_default_workspace(const ProjectDocument& project) {
    auto workspace = Workspace::create_default(ProjectInfo{.name = project.name, .root_path = project.root_path});
    workspace.set_panel_visible(PanelId::resources, true);
    workspace.set_panel_visible(PanelId::ai_commands, true);
    workspace.set_panel_visible(PanelId::profiler, true);
    workspace.set_panel_visible(PanelId::project_settings, true);
    workspace.set_panel_visible(PanelId::timeline, true);
    workspace.set_panel_visible(PanelId::viewport, false);
    workspace.set_panel_visible(PanelId::input_rebinding, false);
    return workspace;
}

[[nodiscard]] SceneAuthoringDocument make_default_scene_document() {
    mirakana::Scene scene{"EditorScene"};
    const auto root = scene.create_node("Root");
    const auto camera = scene.create_node("Camera");
    const auto light = scene.create_node("KeyLight");
    scene.set_parent(camera, root);
    scene.set_parent(light, root);

    auto document = SceneAuthoringDocument::from_scene(std::move(scene), "scenes/start.scene");
    (void)document.select_node(root);
    return document;
}

[[nodiscard]] std::vector<EditorPropertyRow> make_default_inspector_rows(const ProjectDocument& project) {
    return {
        EditorPropertyRow{.id = "project", .label = "Project", .value = project.name, .editable = false},
        EditorPropertyRow{.id = "scene", .label = "Startup Scene", .value = project.startup_scene_path},
        EditorPropertyRow{.id = "asset_root", .label = "Asset Root", .value = project.asset_root},
        EditorPropertyRow{.id = "renderer", .label = "Renderer", .value = "native_win32_d3d12", .editable = false},
    };
}

[[nodiscard]] std::vector<EditorAssetListRow> make_default_asset_rows() {
    return {
        EditorAssetListRow{.id = "scene_start", .path = "assets/scenes/start.scene", .kind = "scene"},
        EditorAssetListRow{.id = "material_default", .path = "assets/materials/default.material", .kind = "material"},
        EditorAssetListRow{.id = "shader_editor", .path = "assets/shaders/editor_preview.shader", .kind = "shader"},
    };
}

[[nodiscard]] std::vector<EditorDiagnosticRow> make_default_console_rows() {
    return {
        EditorDiagnosticRow{
            .id = "native_shell", .severity = EditorDiagnosticSeverity::info, .message = "Native editor shell ready"},
        EditorDiagnosticRow{
            .id = "sdl3_removed", .severity = EditorDiagnosticSeverity::info, .message = "SDL3 dependency absent"},
    };
}

[[nodiscard]] EditorResourcePanelInput make_native_resource_panel_input(bool device_available,
                                                                        std::uint64_t frame_index) {
    return EditorResourcePanelInput{
        .device_available = device_available,
        .backend_id = "d3d12",
        .backend_label = "Native Win32/D3D12 host",
        .frame_index = frame_index,
        .rhi_counters =
            {
                EditorResourceCounterInput{
                    .id = "swapchain_back_buffers", .label = "Swapchain back buffers", .value = 2U},
                EditorResourceCounterInput{
                    .id = "imgui_srv_descriptors", .label = "ImGui SRV descriptors", .value = 32U},
            },
        .capture_requests =
            {
                EditorResourceCaptureRequestInput{
                    .id = "pix_gpu_capture",
                    .label = "PIX GPU capture",
                    .tool_label = "PIX",
                    .action_label = "Request external capture",
                    .host_gates = {"windows", "pix"},
                    .available = false,
                    .diagnostic = "external host workflow only",
                },
            },
    };
}

[[nodiscard]] EditorResourcePanelModel make_native_resource_panel_model(bool device_available,
                                                                        std::uint64_t frame_index) {
    return make_editor_resource_panel_model(make_native_resource_panel_input(device_available, frame_index));
}

[[nodiscard]] EditorAiCommandPanelModel make_default_ai_command_model() {
    EditorAiCommandPanelDesc desc;
    return make_editor_ai_command_panel_model(desc);
}

[[nodiscard]] EditorProfilerPanelModel make_default_profiler_model(std::span<const EditorDiagnosticRow> console_rows) {
    mirakana::DiagnosticCapture capture;
    return make_editor_profiler_panel_model(
        capture, EditorProfilerStatus{.log_records = console_rows.size(), .dirty = false, .revision = 1});
}

[[nodiscard]] EditorTimelinePanelModel make_default_timeline_model() {
    mirakana::AnimationAuthoredTimelineDesc desc{
        .duration_seconds = 2.0F,
        .looping = true,
        .tracks =
            {
                mirakana::AnimationTimelineEventTrackDesc{
                    .name = "editor",
                    .events =
                        {
                            mirakana::AnimationTimelineEventDesc{
                                .time_seconds = 0.25F,
                                .name = "open",
                                .payload = "shell",
                                .track = "editor",
                            },
                            mirakana::AnimationTimelineEventDesc{
                                .time_seconds = 1.0F,
                                .name = "smoke",
                                .payload = "frames",
                                .track = "editor",
                            },
                        },
                },
            },
    };
    return make_editor_timeline_panel_model(desc, 0.0F, false);
}

} // namespace

struct NativeEditorApp::Impl {
    explicit Impl(const NativeEditorLaunchOptions& /*options*/)
        : project(make_default_project_document()), workspace(make_default_workspace(project)),
          scene(make_default_scene_document()), inspector_rows(make_default_inspector_rows(project)),
          asset_rows(make_default_asset_rows()), console_rows(make_default_console_rows()),
          resources(make_native_resource_panel_model(false, 0U)), ai_commands(make_default_ai_command_model()),
          profiler(make_default_profiler_model(console_rows)), timeline(make_default_timeline_model()) {}

    ProjectDocument project;
    Workspace workspace;
    SceneAuthoringDocument scene;
    std::vector<EditorPropertyRow> inspector_rows;
    std::vector<EditorAssetListRow> asset_rows;
    std::vector<EditorDiagnosticRow> console_rows;
    EditorResourcePanelModel resources;
    EditorAiCommandPanelModel ai_commands;
    EditorProfilerPanelModel profiler;
    EditorTimelinePanelModel timeline;
};

NativeEditorApp::NativeEditorApp(NativeEditorLaunchOptions options)
    : options_{options}, impl_{std::make_unique<Impl>(options_)} {}

NativeEditorApp::~NativeEditorApp() = default;

NativeEditorApp::NativeEditorApp(NativeEditorApp&&) noexcept = default;

NativeEditorApp& NativeEditorApp::operator=(NativeEditorApp&&) noexcept = default;

const NativeEditorLaunchOptions& NativeEditorApp::options() const noexcept {
    return options_;
}

std::uint32_t NativeEditorApp::frames_recorded() const noexcept {
    return frames_recorded_;
}

std::uint32_t NativeEditorApp::native_panel_count() const noexcept {
    return static_cast<std::uint32_t>(native_panel_tokens.size());
}

bool NativeEditorApp::has_native_panel(std::string_view id) const noexcept {
    return std::ranges::any_of(native_panel_tokens, [id](const NativePanelToken& panel) { return panel.id == id; });
}

std::uint32_t NativeEditorApp::panels_rendered_last_frame() const noexcept {
    return panels_rendered_last_frame_;
}

const Workspace& NativeEditorApp::workspace() const noexcept {
    return impl_->workspace;
}

bool NativeEditorApp::is_panel_visible(PanelId id) const noexcept {
    return impl_->workspace.is_panel_visible(id);
}

void NativeEditorApp::set_panel_visible(PanelId id, bool visible) {
    impl_->workspace.set_panel_visible(id, visible);
}

const ProjectDocument& NativeEditorApp::project() const noexcept {
    return impl_->project;
}

const SceneAuthoringDocument& NativeEditorApp::scene_document() const noexcept {
    return impl_->scene;
}

std::span<const EditorPropertyRow> NativeEditorApp::inspector_rows() const noexcept {
    return impl_->inspector_rows;
}

std::span<const EditorAssetListRow> NativeEditorApp::asset_rows() const noexcept {
    return impl_->asset_rows;
}

std::span<const EditorDiagnosticRow> NativeEditorApp::console_rows() const noexcept {
    return impl_->console_rows;
}

const EditorResourcePanelModel& NativeEditorApp::resources() const noexcept {
    return impl_->resources;
}

const EditorAiCommandPanelModel& NativeEditorApp::ai_commands() const noexcept {
    return impl_->ai_commands;
}

const EditorProfilerPanelModel& NativeEditorApp::profiler() const noexcept {
    return impl_->profiler;
}

const EditorTimelinePanelModel& NativeEditorApp::timeline() const noexcept {
    return impl_->timeline;
}

std::vector<ProjectSettingsError> NativeEditorApp::project_settings_errors() const {
    return ProjectSettingsDraft::from_project(impl_->project).validation_errors();
}

int NativeEditorApp::run() {
    return 0;
}

void NativeEditorApp::record_native_frame() noexcept {
    ++frames_recorded_;
}

void NativeEditorApp::record_native_panels_rendered(std::uint32_t count) noexcept {
    panels_rendered_last_frame_ = count;
}

void NativeEditorApp::record_native_resource_device_ready(std::uint64_t frame_index) {
    impl_->resources = make_native_resource_panel_model(true, frame_index);
}

} // namespace mirakana::editor
