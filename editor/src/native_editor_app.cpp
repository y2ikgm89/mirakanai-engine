// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "native_editor_app.hpp"

#include "mirakana/assets/material.hpp"
#include "mirakana/core/diagnostics.hpp"
#include "mirakana/editor/editor_dock_layout.hpp"
#include "mirakana/editor/shader_compile.hpp"
#include "mirakana/platform/clipboard.hpp"
#include "mirakana/scene/scene.hpp"

#include <algorithm>
#include <cstdint>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::editor {
namespace {

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

[[nodiscard]] EditorMaterialAssetPreviewPanelModel make_default_material_preview_panel_model() {
    constexpr std::string_view shader_output_root{"out/editor/shaders"};
    const auto material_id = AssetId::from_name("materials/default");

    MemoryFileSystem filesystem;
    AssetRegistry registry;
    registry.add(AssetRecord{
        .id = material_id,
        .kind = AssetKind::material,
        .path = "assets/materials/default.material",
    });
    const MaterialDefinition material{
        .id = material_id,
        .name = "Default Material",
        .shading_model = MaterialShadingModel::lit,
        .surface_mode = MaterialSurfaceMode::opaque,
        .factors =
            MaterialFactors{
                .base_color = {0.8F, 0.8F, 0.8F, 1.0F},
                .emissive = {0.0F, 0.0F, 0.0F},
                .metallic = 0.0F,
                .roughness = 1.0F,
            },
        .texture_bindings = {},
        .double_sided = false,
    };
    filesystem.write_text("assets/materials/default.material", serialize_material_definition(material));

    const auto shader_requests = make_material_preview_shader_compile_requests(shader_output_root);
    filesystem.write_text("out/editor/shaders/editor-material-preview-factor.vs.dxil", "native factor vertex");
    filesystem.write_text("out/editor/shaders/editor-material-preview-factor.ps.dxil", "native factor fragment");
    ViewportShaderArtifactState shader_artifacts;
    shader_artifacts.refresh_from(filesystem, shader_requests);

    return make_editor_material_asset_preview_panel_model(filesystem, registry, material_id, shader_artifacts);
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
                    .id = "first_party_ui_renderer", .label = "First-party UI renderer", .value = 1U},
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

class NativeEditorClipboardTextAdapter final : public mirakana::ui::IClipboardTextAdapter {
  public:
    explicit NativeEditorClipboardTextAdapter(IClipboard& clipboard) noexcept : clipboard_(&clipboard) {}

    void set_clipboard_text(std::string_view text) override {
        if (text.empty()) {
            clipboard_->clear();
            return;
        }
        clipboard_->set_text(text);
    }

    [[nodiscard]] bool has_clipboard_text() const override {
        return clipboard_->has_text();
    }

    [[nodiscard]] std::string clipboard_text() const override {
        return clipboard_->text();
    }

  private:
    IClipboard* clipboard_{nullptr};
};

} // namespace

struct NativeEditorApp::Impl {
    explicit Impl(const NativeEditorLaunchOptions& options)
        : project(make_default_project_document()), workspace(make_default_workspace(project)),
          scene(make_default_scene_document()), inspector_rows(make_default_inspector_rows(project)),
          asset_rows(make_default_asset_rows()), console_rows(make_default_console_rows()),
          resources(make_native_resource_panel_model(false, 0U)), ai_commands(make_default_ai_command_model()),
          profiler(make_default_profiler_model(console_rows)), timeline(make_default_timeline_model()),
          material_preview(make_default_material_preview_panel_model()),
          material_preview_display(plan_native_material_preview_display(NativeMaterialPreviewDisplayDesc{
              .shader_artifacts_available = material_preview.required_shader_ready,
              .gpu_payload_available = material_preview.gpu_payload_ready,
          })),
          viewport_display(plan_native_viewport_display(NativeViewportDisplayDesc{
              .extent = ViewportExtent{.width = options.width, .height = options.height},
          })),
          clipboard_text_adapter(memory_clipboard) {
        file_dialog_service = &memory_file_dialog_service;
        clipboard_adapter = &clipboard_text_adapter;
        process_runner = &recording_process_runner;
    }

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
    EditorMaterialAssetPreviewPanelModel material_preview;
    ViewportState viewport;
    NativeMaterialPreviewDisplayPlan material_preview_display;
    NativeViewportDisplayPlan viewport_display;
    MemoryFileDialogService memory_file_dialog_service;
    MemoryClipboard memory_clipboard;
    NativeEditorClipboardTextAdapter clipboard_text_adapter;
    RecordingProcessRunner recording_process_runner;
    IFileDialogService* file_dialog_service{nullptr};
    ui::IClipboardTextAdapter* clipboard_adapter{nullptr};
    IProcessRunner* process_runner{nullptr};
    NativeEditorServiceStatus service_status;
    std::string docking_status_last_frame{"not_rendered"};
    std::uint32_t dock_tab_headers_last_frame{0};
    std::uint32_t dock_split_gutters_last_frame{0};
    std::uint32_t dock_active_panels_last_frame{0};
    std::uint32_t dock_focusable_controls_last_frame{0};
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
    const auto catalog = editor_dock_panel_catalog();
    return static_cast<std::uint32_t>(std::ranges::count_if(
        catalog, [](const EditorDockPanelCatalogRow& panel) { return panel.native_shell_panel; }));
}

bool NativeEditorApp::has_native_panel(std::string_view id) const noexcept {
    const auto catalog = editor_dock_panel_catalog();
    const auto* panel = find_editor_dock_panel(catalog, id);
    return panel != nullptr && panel->native_shell_panel;
}

std::uint32_t NativeEditorApp::panels_rendered_last_frame() const noexcept {
    return panels_rendered_last_frame_;
}

std::string_view NativeEditorApp::docking_status_last_frame() const noexcept {
    return impl_->docking_status_last_frame;
}

std::uint32_t NativeEditorApp::dock_tab_headers_last_frame() const noexcept {
    return impl_->dock_tab_headers_last_frame;
}

std::uint32_t NativeEditorApp::dock_split_gutters_last_frame() const noexcept {
    return impl_->dock_split_gutters_last_frame;
}

std::uint32_t NativeEditorApp::dock_active_panels_last_frame() const noexcept {
    return impl_->dock_active_panels_last_frame;
}

std::uint32_t NativeEditorApp::dock_focusable_controls_last_frame() const noexcept {
    return impl_->dock_focusable_controls_last_frame;
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

const NativeEditorServiceStatus& NativeEditorApp::services() const noexcept {
    return impl_->service_status;
}

const ViewportState& NativeEditorApp::viewport() const noexcept {
    return impl_->viewport;
}

const NativeViewportDisplayPlan& NativeEditorApp::viewport_display() const noexcept {
    return impl_->viewport_display;
}

const EditorMaterialAssetPreviewPanelModel& NativeEditorApp::material_preview() const noexcept {
    return impl_->material_preview;
}

const NativeMaterialPreviewDisplayPlan& NativeEditorApp::material_preview_display() const noexcept {
    return impl_->material_preview_display;
}

void NativeEditorApp::bind_native_services(NativeEditorServiceBindings services) {
    if (services.file_dialog_service != nullptr) {
        impl_->file_dialog_service = services.file_dialog_service;
        impl_->service_status.file_dialog_service_id =
            services.file_dialog_service_id.empty() ? "external" : std::move(services.file_dialog_service_id);
        impl_->service_status.file_dialog_available = true;
    }
    if (services.clipboard_text_adapter != nullptr) {
        impl_->clipboard_adapter = services.clipboard_text_adapter;
        impl_->service_status.clipboard_service_id =
            services.clipboard_service_id.empty() ? "external" : std::move(services.clipboard_service_id);
        impl_->service_status.clipboard_available = true;
    }
    if (services.reviewed_process_runner != nullptr) {
        impl_->process_runner = services.reviewed_process_runner;
        impl_->service_status.reviewed_process_runner_id =
            services.reviewed_process_runner_id.empty() ? "external" : std::move(services.reviewed_process_runner_id);
        impl_->service_status.reviewed_process_runner_available = true;
    }
}

FileDialogId NativeEditorApp::show_file_dialog(FileDialogRequest request) {
    if (impl_->file_dialog_service == nullptr) {
        impl_->service_status.file_dialog_available = false;
        return 0;
    }
    ++impl_->service_status.file_dialog_requests_routed;
    return impl_->file_dialog_service->show(std::move(request));
}

std::optional<FileDialogResult> NativeEditorApp::poll_file_dialog_result(FileDialogId id) {
    if (impl_->file_dialog_service == nullptr) {
        impl_->service_status.file_dialog_available = false;
        return std::nullopt;
    }
    return impl_->file_dialog_service->poll_result(id);
}

ui::ClipboardTextWriteResult NativeEditorApp::write_clipboard_text(ui::ClipboardTextWriteRequest request) {
    if (impl_->clipboard_adapter == nullptr) {
        impl_->service_status.clipboard_available = false;
        ui::ClipboardTextWriteResult result;
        result.diagnostics.push_back(ui::AdapterPayloadDiagnostic{
            .id = request.target,
            .code = ui::AdapterPayloadDiagnosticCode::invalid_clipboard_text,
            .message = "native editor clipboard service is unavailable",
        });
        return result;
    }

    auto result = ui::write_clipboard_text(*impl_->clipboard_adapter, request);
    if (result.succeeded()) {
        ++impl_->service_status.clipboard_operations_routed;
    }
    return result;
}

ui::ClipboardTextReadResult NativeEditorApp::read_clipboard_text(ui::ClipboardTextReadRequest request) {
    if (impl_->clipboard_adapter == nullptr) {
        impl_->service_status.clipboard_available = false;
        ui::ClipboardTextReadResult result;
        result.diagnostics.push_back(ui::AdapterPayloadDiagnostic{
            .id = request.target,
            .code = ui::AdapterPayloadDiagnosticCode::invalid_clipboard_text_result,
            .message = "native editor clipboard service is unavailable",
        });
        return result;
    }

    auto result = ui::read_clipboard_text(*impl_->clipboard_adapter, request);
    if (result.succeeded()) {
        ++impl_->service_status.clipboard_operations_routed;
    }
    return result;
}

EditorAiReviewedValidationExecutionModel
NativeEditorApp::reviewed_validation_execution_plan(const EditorAiReviewedValidationExecutionDesc& desc) {
    auto model = make_editor_ai_reviewed_validation_execution_plan(desc);
    ++impl_->service_status.reviewed_process_plans;
    impl_->service_status.reviewed_process_status_label = model.status_label;
    return model;
}

NativeEditorReviewedProcessResult NativeEditorApp::run_reviewed_process(NativeEditorReviewedProcessRequest request) {
    NativeEditorReviewedProcessResult result{
        .reviewed = request.plan.can_execute && is_allowed_process_command(request.plan.command),
        .user_confirmed = request.user_confirmed,
        .executed = false,
        .process = {},
        .diagnostic = {},
    };

    if (!result.reviewed) {
        result.diagnostic = "reviewed process execution requires a ready allowlisted command";
        impl_->service_status.reviewed_process_status_label = "blocked";
        return result;
    }
    if (!request.user_confirmed) {
        result.diagnostic = "reviewed process execution requires user confirmation before launch";
        impl_->service_status.reviewed_process_status_label = "confirmation required";
        return result;
    }
    if (impl_->process_runner == nullptr) {
        impl_->service_status.reviewed_process_runner_available = false;
        result.diagnostic = "reviewed process runner is unavailable";
        return result;
    }

    result.process = run_process_command(*impl_->process_runner, request.plan.command);
    result.executed = result.process.launched;
    ++impl_->service_status.reviewed_process_executions;
    impl_->service_status.reviewed_process_status_label = result.process.succeeded() ? "executed" : "failed";
    return result;
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

void NativeEditorApp::record_native_docking_frame(std::string status, std::uint32_t tab_header_count,
                                                  std::uint32_t split_gutter_count, std::uint32_t active_panel_count,
                                                  std::uint32_t focusable_control_count) {
    impl_->docking_status_last_frame = std::move(status);
    impl_->dock_tab_headers_last_frame = tab_header_count;
    impl_->dock_split_gutters_last_frame = split_gutter_count;
    impl_->dock_active_panels_last_frame = active_panel_count;
    impl_->dock_focusable_controls_last_frame = focusable_control_count;
}

void NativeEditorApp::record_native_resource_device_ready(std::uint64_t frame_index) {
    impl_->resources = make_native_resource_panel_model(true, frame_index);
}

void NativeEditorApp::record_native_viewport_d3d12_host_ready(std::uint64_t frame_index) {
    const ViewportExtent extent{.width = options_.width, .height = options_.height};
    impl_->viewport.set_renderer("d3d12");
    impl_->viewport.resize(extent);
    impl_->viewport.mark_frame_rendered();
    impl_->viewport_display = plan_native_viewport_display(NativeViewportDisplayDesc{
        .d3d12_host_available = true,
        .renderer_output_available = false,
        .extent = extent,
        .frame_index = frame_index,
        .backend_id = "d3d12",
    });
}

void NativeEditorApp::record_native_material_preview_d3d12_host_ready(std::uint64_t frame_index) {
    impl_->material_preview_display = plan_native_material_preview_display(NativeMaterialPreviewDisplayDesc{
        .d3d12_host_available = true,
        .shader_artifacts_available = impl_->material_preview.required_shader_ready,
        .gpu_payload_available = impl_->material_preview.gpu_payload_ready,
        .frame_index = frame_index,
        .backend_id = "d3d12",
    });
    apply_editor_material_gpu_preview_execution_snapshot(impl_->material_preview,
                                                         impl_->material_preview_display.execution_snapshot);
}

} // namespace mirakana::editor
