// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include <mirakana/editor/game_module_driver.hpp>
#include <mirakana/editor/play_in_editor.hpp>
#include <mirakana/scene/scene.hpp>
#include <mirakana/ui/ui.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::editor {
namespace {

class EditorGameModuleDriverAdapter final : public IEditorPlaySessionDriver {
  public:
    explicit EditorGameModuleDriverAdapter(EditorGameModuleDriverApi api) noexcept : api_(api) {}

    EditorGameModuleDriverAdapter(const EditorGameModuleDriverAdapter&) = delete;
    EditorGameModuleDriverAdapter& operator=(const EditorGameModuleDriverAdapter&) = delete;
    EditorGameModuleDriverAdapter(EditorGameModuleDriverAdapter&&) noexcept = delete;
    EditorGameModuleDriverAdapter& operator=(EditorGameModuleDriverAdapter&&) noexcept = delete;

    ~EditorGameModuleDriverAdapter() override {
        if (api_.destroy != nullptr) {
            api_.destroy(api_.user_data);
        }
    }

    void on_play_begin(Scene& scene) override {
        if (api_.begin != nullptr) {
            api_.begin(api_.user_data, &scene);
        }
    }

    void on_play_tick(Scene& scene, const EditorPlaySessionTickContext& context) override {
        api_.tick(api_.user_data, &scene, &context);
    }

    void on_play_end(Scene& scene) override {
        if (api_.end != nullptr) {
            api_.end(api_.user_data, &scene);
        }
    }

  private:
    EditorGameModuleDriverApi api_;
};

[[nodiscard]] std::string sanitize_text(std::string_view value) {
    std::string text;
    text.reserve(value.size());
    for (const auto character : value) {
        if (character == '\n' || character == '\r' || character == '=') {
            text.push_back(' ');
        } else {
            text.push_back(character);
        }
    }
    return text.empty() ? "-" : text;
}

[[nodiscard]] std::string sanitize_element_id(std::string_view value) {
    std::string text;
    text.reserve(value.size());
    for (const auto character : value) {
        const auto byte = static_cast<unsigned char>(character);
        if (std::isalnum(byte) != 0 || character == '_' || character == '-' || character == '.') {
            text.push_back(character);
        } else {
            text.push_back('_');
        }
    }
    return text.empty() ? "game_module_driver" : text;
}

[[nodiscard]] bool has_control_character(std::string_view value) noexcept {
    return value.contains('\n') || value.contains('\r') || value.contains('\0');
}

[[nodiscard]] bool is_safe_factory_symbol(std::string_view value) noexcept {
    if (value.empty() || has_control_character(value)) {
        return false;
    }

    const auto first = static_cast<unsigned char>(value.front());
    if (std::isalpha(first) == 0 && value.front() != '_') {
        return false;
    }

    return std::ranges::all_of(value, [](const char character) {
        const auto byte = static_cast<unsigned char>(character);
        return std::isalnum(byte) != 0 || character == '_';
    });
}

[[nodiscard]] bool is_safe_absolute_module_path(std::string_view value) {
    if (value.empty() || has_control_character(value)) {
        return false;
    }
    return std::filesystem::path{value}.is_absolute();
}

void append_unique(std::vector<std::string>& values, std::string value) {
    if (value.empty()) {
        return;
    }
    if (std::ranges::find(values, value) == values.end()) {
        values.push_back(std::move(value));
    }
}

void append_blocker(EditorGameModuleDriverLoadModel& model, std::string blocker, std::string diagnostic) {
    model.has_blocking_diagnostics = true;
    append_unique(model.blocked_by, std::move(blocker));
    model.diagnostics.push_back(std::move(diagnostic));
}

void append_blocker(EditorGameModuleDriverReloadModel& model, std::string blocker, std::string diagnostic) {
    model.has_blocking_diagnostics = true;
    append_unique(model.blocked_by, std::move(blocker));
    model.diagnostics.push_back(std::move(diagnostic));
}

void append_blocker(EditorGameModuleDriverUnloadModel& model, std::string blocker, std::string diagnostic) {
    model.has_blocking_diagnostics = true;
    append_unique(model.blocked_by, std::move(blocker));
    model.diagnostics.push_back(std::move(diagnostic));
}

void append_blocker(EditorGameModuleDriverCreateResult& result, std::string blocker, std::string diagnostic) {
    append_unique(result.blocked_by, std::move(blocker));
    result.diagnostics.push_back(std::move(diagnostic));
}

void reject_unsupported_claim(EditorGameModuleDriverLoadModel& model, std::string claim, std::string diagnostic) {
    model.has_blocking_diagnostics = true;
    append_unique(model.unsupported_claims, std::move(claim));
    append_unique(model.blocked_by, "unsupported-game-module-driver-claim");
    model.diagnostics.push_back(std::move(diagnostic));
}

void reject_unsupported_claim(EditorGameModuleDriverReloadModel& model, std::string claim, std::string diagnostic) {
    model.has_blocking_diagnostics = true;
    append_unique(model.unsupported_claims, std::move(claim));
    append_unique(model.blocked_by, "unsupported-game-module-driver-claim");
    model.diagnostics.push_back(std::move(diagnostic));
}

[[nodiscard]] std::string join_values(const std::vector<std::string>& values) {
    if (values.empty()) {
        return "-";
    }
    std::string text;
    for (const auto& value : values) {
        if (!text.empty()) {
            text += ", ";
        }
        text += value;
    }
    return text;
}

[[nodiscard]] std::string_view
editor_game_module_driver_status_label(const EditorGameModuleDriverStatus status) noexcept {
    switch (status) {
    case EditorGameModuleDriverStatus::ready:
        return "ready";
    case EditorGameModuleDriverStatus::blocked:
        return "blocked";
    }
    return "unknown";
}

void add_or_throw(mirakana::ui::UiDocument& document, mirakana::ui::ElementDesc desc) {
    if (!document.try_add_element(std::move(desc))) {
        throw std::invalid_argument("editor game module driver ui element could not be added");
    }
}

[[nodiscard]] mirakana::ui::TextContent make_text(std::string label) {
    mirakana::ui::TextContent text;
    text.label = std::move(label);
    text.font_family = "ui/body";
    text.wrap = mirakana::ui::TextWrapMode::ellipsis;
    return text;
}

[[nodiscard]] mirakana::ui::ElementDesc make_root(std::string id, mirakana::ui::SemanticRole role) {
    mirakana::ui::ElementDesc desc;
    desc.id = mirakana::ui::ElementId{std::move(id)};
    desc.role = role;
    desc.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 1.0F, .height = 1.0F};
    return desc;
}

[[nodiscard]] mirakana::ui::ElementDesc make_child(std::string id, mirakana::ui::ElementId parent,
                                                   mirakana::ui::SemanticRole role) {
    mirakana::ui::ElementDesc desc;
    desc.id = mirakana::ui::ElementId{std::move(id)};
    desc.parent = std::move(parent);
    desc.role = role;
    desc.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 1.0F, .height = 1.0F};
    return desc;
}

void append_label(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& parent, std::string id,
                  std::string label) {
    mirakana::ui::ElementDesc desc = make_child(std::move(id), parent, mirakana::ui::SemanticRole::label);
    desc.text = make_text(std::move(label));
    add_or_throw(document, std::move(desc));
}

[[nodiscard]] EditorGameModuleDriverContractMetadataRow make_contract_row(std::string id, std::string label,
                                                                          std::string value, bool required,
                                                                          bool supported, std::string diagnostic = {}) {
    EditorGameModuleDriverContractMetadataRow row;
    row.id = std::move(id);
    row.label = std::move(label);
    row.value = std::move(value);
    row.required = required;
    row.supported = supported;
    row.diagnostic = std::move(diagnostic);
    return row;
}

[[nodiscard]] std::string_view host_session_phase_id(const EditorGameModuleDriverHostSessionPhase phase) noexcept {
    switch (phase) {
    case EditorGameModuleDriverHostSessionPhase::idle_no_driver_play_stopped:
        return "idle_no_driver_play_stopped";
    case EditorGameModuleDriverHostSessionPhase::driver_resident_play_stopped:
        return "driver_resident_play_stopped";
    case EditorGameModuleDriverHostSessionPhase::play_active_without_driver:
        return "play_active_without_driver";
    case EditorGameModuleDriverHostSessionPhase::play_active_with_driver:
        return "play_active_with_driver";
    }
    return "idle_no_driver_play_stopped";
}

[[nodiscard]] std::string_view
host_session_dll_mutation_order_guidance(const EditorGameModuleDriverHostSessionPhase phase) noexcept {
    switch (phase) {
    case EditorGameModuleDriverHostSessionPhase::idle_no_driver_play_stopped:
        return "phase_idle_no_driver_order_review_load_then_load_library";
    case EditorGameModuleDriverHostSessionPhase::driver_resident_play_stopped:
        return "phase_driver_stopped_order_unload_or_stopped_state_reload_only";
    case EditorGameModuleDriverHostSessionPhase::play_active_without_driver:
        return "phase_play_no_driver_order_stop_play_before_any_dll_change";
    case EditorGameModuleDriverHostSessionPhase::play_active_with_driver:
        return "phase_play_with_driver_order_stop_play_unload_driver_free_module_then_reload";
    }
    return "phase_idle_no_driver_order_review_load_then_load_library";
}

[[nodiscard]] std::string host_session_phase_summary(const EditorGameModuleDriverHostSessionPhase phase) {
    switch (phase) {
    case EditorGameModuleDriverHostSessionPhase::idle_no_driver_play_stopped:
        return "Play-In-Editor stopped; no game module driver resident; load review may proceed when path/symbol gates "
               "pass.";
    case EditorGameModuleDriverHostSessionPhase::driver_resident_play_stopped:
        return "Play-In-Editor stopped; game module driver resident; use explicit unload or stopped-state reload; load "
               "blocks duplicate residency.";
    case EditorGameModuleDriverHostSessionPhase::play_active_without_driver:
        return "Play-In-Editor active without game module driver; DLL load/unload/reload mutation paths blocked until "
               "play stops.";
    case EditorGameModuleDriverHostSessionPhase::play_active_with_driver:
        return "Play-In-Editor active with game module driver resident; DLL mutation paths blocked until play stops.";
    }
    return {};
}

} // namespace

EditorGameModuleDriverHostSessionSnapshot
make_editor_game_module_driver_host_session_snapshot(const bool play_session_active, const bool driver_loaded) {
    EditorGameModuleDriverHostSessionSnapshot out;
    out.play_session_active = play_session_active;
    out.driver_loaded = driver_loaded;
    if (!play_session_active && !driver_loaded) {
        out.phase = EditorGameModuleDriverHostSessionPhase::idle_no_driver_play_stopped;
    } else if (!play_session_active && driver_loaded) {
        out.phase = EditorGameModuleDriverHostSessionPhase::driver_resident_play_stopped;
    } else if (play_session_active && !driver_loaded) {
        out.phase = EditorGameModuleDriverHostSessionPhase::play_active_without_driver;
    } else {
        out.phase = EditorGameModuleDriverHostSessionPhase::play_active_with_driver;
    }
    out.phase_id = std::string(host_session_phase_id(out.phase));
    out.summary = host_session_phase_summary(out.phase);
    out.barrier_play_dll_surface_mutation_status =
        play_session_active ? "enforced_block_load_unload_reload" : "inactive_play_stopped";
    out.policy_active_session_hot_reload = "unsupported_no_silent_dll_replacement_mid_play";
    out.policy_stopped_state_reload_scope = "reload_and_duplicate_load_reviews_require_play_stopped_explicit_paths";
    out.policy_dll_mutation_order_guidance = std::string(host_session_dll_mutation_order_guidance(out.phase));
    return out;
}

EditorGameModuleDriverContractMetadataModel make_editor_game_module_driver_contract_metadata_model() {
    EditorGameModuleDriverContractMetadataModel model;
    model.rows = {
        make_contract_row("abi.name", "ABI Contract", model.abi_contract, true, true),
        make_contract_row("abi.version", "ABI Version", std::to_string(model.abi_version), true, true),
        make_contract_row("factory.symbol", "Factory Symbol", model.factory_symbol, true, true),
        make_contract_row("callback.tick", "Tick Callback", "required", true, true),
        make_contract_row("callback.destroy", "Destroy Callback", "required", true, true),
        make_contract_row("callback.begin", "Begin Callback", "optional", false, true),
        make_contract_row("callback.end", "End Callback", "optional", false, true),
        make_contract_row("lifetime.dynamic_library", "Dynamic Library Lifetime", "host-owned while loaded", true, true,
                          "visible editor keeps the dynamic library alive while the driver is loaded"),
        make_contract_row("compatibility.same_engine_build", "Same-Engine-Build Compatibility", "required", true, true,
                          "game modules must be built against the same engine/editor contract"),
        make_contract_row("compatibility.stable_third_party_abi", "Stable Third-Party ABI", "unsupported", false, false,
                          "same-engine-build handoff only; no stable third-party ABI is promised"),
        make_contract_row("reload.hot", "Hot Reload", "unsupported", false, false,
                          "game module driver reload is explicit and stopped-state only"),
        make_contract_row("reload.active_session", "Active-Session Reload", "unsupported", false, false,
                          "active Play-In-Editor sessions must stop before reload"),
        make_contract_row("embedding.desktop_game_runner", "DesktopGameRunner Embedding", "unsupported", false, false,
                          "DesktopGameRunner is not embedded in the editor process"),
    };
    return model;
}

EditorGameModuleDriverCtestProbeEvidenceModel make_editor_game_module_driver_ctest_probe_evidence_model() {
    return EditorGameModuleDriverCtestProbeEvidenceModel{};
}

EditorGameModuleDriverReloadTransactionRecipeEvidenceModel
make_editor_game_module_driver_reload_transaction_recipe_evidence_model() {
    EditorGameModuleDriverReloadTransactionRecipeEvidenceModel model;
    model.validation_recipe_id = "dev-windows-editor-game-module-driver-load-tests";
    model.host_gate_acknowledgement_id = "windows-msvc-dev-editor-game-module-driver-ctest";
    model.reviewed_dry_run_command =
        "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe "
        "dev-windows-editor-game-module-driver-load-tests -HostGateAcknowledgements "
        "windows-msvc-dev-editor-game-module-driver-ctest";
    model.reviewed_execute_command =
        "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe "
        "dev-windows-editor-game-module-driver-load-tests -HostGateAcknowledgements "
        "windows-msvc-dev-editor-game-module-driver-ctest";
    model.editor_boundary_note =
        "mirakana_editor shows reviewed argv only; run-validation-recipe execution stays an operator/CI host action";
    return model;
}

mirakana::ui::UiDocument make_editor_game_module_driver_reload_transaction_recipe_evidence_ui_model(
    const EditorGameModuleDriverReloadTransactionRecipeEvidenceModel& model) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("play_in_editor.game_module_driver.reload_transaction_recipe_evidence",
                                     mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"play_in_editor.game_module_driver.reload_transaction_recipe_evidence"};

    append_label(document, root, "play_in_editor.game_module_driver.reload_transaction_recipe_evidence.contract_label",
                 std::string{editor_game_module_driver_reload_transaction_recipe_evidence_contract_v1()});
    append_label(document, root,
                 "play_in_editor.game_module_driver.reload_transaction_recipe_evidence.validation_recipe_id",
                 model.validation_recipe_id);
    append_label(document, root,
                 "play_in_editor.game_module_driver.reload_transaction_recipe_evidence.host_gate_acknowledgement_id",
                 model.host_gate_acknowledgement_id);
    append_label(document, root,
                 "play_in_editor.game_module_driver.reload_transaction_recipe_evidence.reviewed_dry_run_command",
                 model.reviewed_dry_run_command);
    append_label(document, root,
                 "play_in_editor.game_module_driver.reload_transaction_recipe_evidence.reviewed_execute_command",
                 model.reviewed_execute_command);
    append_label(document, root,
                 "play_in_editor.game_module_driver.reload_transaction_recipe_evidence.editor_boundary_note",
                 model.editor_boundary_note);
    return document;
}

EditorGameModuleDriverLoadModel make_editor_game_module_driver_load_model(const EditorGameModuleDriverLoadDesc& desc) {
    EditorGameModuleDriverLoadModel model;
    model.id = sanitize_element_id(desc.id);
    model.label = desc.label.empty() ? model.id : sanitize_text(desc.label);
    model.module_path = sanitize_text(desc.module_path);
    model.factory_symbol = desc.factory_symbol.empty() ? std::string(editor_game_module_driver_factory_symbol_v1)
                                                       : sanitize_text(desc.factory_symbol);
    model.abi_contract = std::string(editor_game_module_driver_abi_name_v1);
    model.abi_version = editor_game_module_driver_abi_version_v1;

    if (!is_safe_absolute_module_path(desc.module_path)) {
        append_blocker(model, "absolute-module-path-required",
                       "game module driver loading requires a reviewed absolute module path");
    }
    if (!is_safe_factory_symbol(model.factory_symbol)) {
        append_blocker(model, "unsafe-factory-symbol",
                       "game module driver loading requires a safe exported factory symbol name");
    }

    if (desc.play_session_active) {
        append_blocker(model, "play-session-active",
                       "game module driver loading is blocked while Play-In-Editor is active");
    }
    if (desc.driver_already_loaded) {
        append_blocker(model, "driver-already-loaded",
                       "game module driver is already loaded; use Reload Game Module Driver for stopped-state "
                       "reload or unload first");
    }

    if (desc.request_hot_reload) {
        reject_unsupported_claim(model, "hot reload", "game module driver loading does not hot reload game code");
    }
    if (desc.request_desktop_game_runner_embedding) {
        reject_unsupported_claim(model, "DesktopGameRunner embedding",
                                 "game module driver loading does not embed DesktopGameRunner");
    }
    if (desc.request_package_script_execution) {
        reject_unsupported_claim(model, "package scripts",
                                 "game module driver loading rejects package script execution");
    }
    if (desc.request_validation_recipe_execution) {
        reject_unsupported_claim(model, "validation recipes",
                                 "game module driver loading does not run validation recipes");
    }
    if (desc.request_arbitrary_shell_execution) {
        reject_unsupported_claim(model, "arbitrary shell",
                                 "game module driver loading rejects arbitrary shell execution");
    }
    if (desc.request_renderer_rhi_uploads) {
        reject_unsupported_claim(model, "renderer/RHI uploads",
                                 "game module driver loading does not create renderer/RHI uploads");
    }
    if (desc.request_renderer_rhi_handle_exposure) {
        reject_unsupported_claim(model, "renderer/RHI handles",
                                 "game module driver loading must not expose renderer/RHI handles");
    }
    if (desc.request_package_streaming) {
        reject_unsupported_claim(model, "package streaming",
                                 "game module driver loading does not make package streaming ready");
    }
    if (desc.request_stable_third_party_abi) {
        reject_unsupported_claim(model, "stable third-party ABI",
                                 "game module driver ABI is a same-engine-build handoff, not a stable third-party ABI");
    }
    if (desc.request_broad_editor_productization) {
        reject_unsupported_claim(model, "broad editor productization",
                                 "game module driver loading is a narrow reviewed Play-In-Editor handoff only");
    }

    if (model.has_blocking_diagnostics) {
        model.status = EditorGameModuleDriverStatus::blocked;
    } else {
        model.status = EditorGameModuleDriverStatus::ready;
        model.can_load = true;
        model.diagnostics.emplace_back("game module driver can be loaded by the visible editor host");
    }
    model.status_label = std::string(editor_game_module_driver_status_label(model.status));
    return model;
}

EditorGameModuleDriverReloadModel
make_editor_game_module_driver_reload_model(const EditorGameModuleDriverReloadDesc& desc) {
    EditorGameModuleDriverLoadDesc load_desc;
    load_desc.id = desc.id;
    load_desc.label = desc.label;
    load_desc.module_path = desc.module_path;
    load_desc.factory_symbol = desc.factory_symbol;
    load_desc.request_hot_reload = desc.request_hot_reload;
    load_desc.request_desktop_game_runner_embedding = desc.request_desktop_game_runner_embedding;
    load_desc.request_package_script_execution = desc.request_package_script_execution;
    load_desc.request_validation_recipe_execution = desc.request_validation_recipe_execution;
    load_desc.request_arbitrary_shell_execution = desc.request_arbitrary_shell_execution;
    load_desc.request_renderer_rhi_uploads = desc.request_renderer_rhi_uploads;
    load_desc.request_renderer_rhi_handle_exposure = desc.request_renderer_rhi_handle_exposure;
    load_desc.request_package_streaming = desc.request_package_streaming;
    load_desc.request_stable_third_party_abi = desc.request_stable_third_party_abi;
    load_desc.request_broad_editor_productization = desc.request_broad_editor_productization;

    const auto load_model = make_editor_game_module_driver_load_model(load_desc);

    EditorGameModuleDriverReloadModel model;
    model.id = load_model.id;
    model.label = desc.label.empty() ? load_model.label : sanitize_text(desc.label);
    model.module_path = load_model.module_path;
    model.factory_symbol = load_model.factory_symbol;
    model.abi_contract = load_model.abi_contract;
    model.abi_version = load_model.abi_version;
    model.has_blocking_diagnostics = load_model.has_blocking_diagnostics;
    model.blocked_by = load_model.blocked_by;
    model.unsupported_claims = load_model.unsupported_claims;
    model.diagnostics = load_model.diagnostics;

    if (!desc.driver_loaded) {
        append_blocker(model, "loaded-driver-required",
                       "game module driver reload requires an already loaded stopped driver");
    }
    if (desc.play_session_active) {
        append_blocker(model, "play-session-active",
                       "game module driver reload is blocked while Play-In-Editor is active");
    }
    if (desc.request_active_session_reload) {
        reject_unsupported_claim(
            model, "active-session reload",
            "game module driver reload is stopped-state only and does not hot reload active sessions");
    }

    if (model.has_blocking_diagnostics) {
        model.status = EditorGameModuleDriverStatus::blocked;
    } else {
        model.status = EditorGameModuleDriverStatus::ready;
        model.can_reload = true;
        model.diagnostics.emplace_back("game module driver can be safely reloaded after unloading the stopped driver");
    }
    model.status_label = std::string(editor_game_module_driver_status_label(model.status));
    return model;
}

EditorGameModuleDriverCreateResult make_editor_game_module_driver_from_api(EditorGameModuleDriverApi api) {
    EditorGameModuleDriverCreateResult result;
    if (api.abi_version != editor_game_module_driver_abi_version_v1) {
        append_blocker(result, "invalid-abi-version",
                       "game module driver function table uses an unsupported ABI version");
    }
    if (api.tick == nullptr) {
        append_blocker(result, "missing-tick-callback", "game module driver function table requires a tick callback");
    }
    if (api.destroy == nullptr) {
        append_blocker(result, "missing-destroy-callback",
                       "game module driver function table requires a destroy callback");
    }

    if (!result.blocked_by.empty()) {
        result.status = EditorGameModuleDriverStatus::blocked;
        return result;
    }

    result.status = EditorGameModuleDriverStatus::ready;
    result.driver = std::make_unique<EditorGameModuleDriverAdapter>(api);
    return result;
}

EditorGameModuleDriverCreateResult make_editor_game_module_driver_from_symbol(EditorGameModuleDriverFactoryFn factory) {
    if (factory == nullptr) {
        EditorGameModuleDriverCreateResult result;
        append_blocker(result, "missing-factory-symbol",
                       "game module driver factory symbol must resolve before creating a driver");
        return result;
    }
    return make_editor_game_module_driver_from_api(factory());
}

mirakana::ui::UiDocument make_editor_game_module_driver_load_ui_model(const EditorGameModuleDriverLoadModel& model) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("play_in_editor.game_module_driver", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"play_in_editor.game_module_driver"};

    const std::string row_id = "play_in_editor.game_module_driver." + sanitize_element_id(model.id);
    mirakana::ui::ElementDesc row = make_child(row_id, root, mirakana::ui::SemanticRole::list_item);
    row.text = make_text(model.label);
    row.enabled = model.can_load;
    add_or_throw(document, std::move(row));

    const mirakana::ui::ElementId row_element{row_id};
    append_label(document, row_element, row_id + ".status", model.status_label);
    append_label(document, row_element, row_id + ".module_path", model.module_path);
    append_label(document, row_element, row_id + ".factory_symbol", model.factory_symbol);
    append_label(document, row_element, row_id + ".abi_contract", model.abi_contract);
    append_label(document, row_element, row_id + ".blocked_by", join_values(model.blocked_by));
    append_label(document, row_element, row_id + ".unsupported_claims", join_values(model.unsupported_claims));
    append_label(document, row_element, row_id + ".diagnostic", join_values(model.diagnostics));
    return document;
}

mirakana::ui::UiDocument
make_editor_game_module_driver_reload_ui_model(const EditorGameModuleDriverReloadModel& model) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("play_in_editor.game_module_driver.reload", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"play_in_editor.game_module_driver.reload"};

    const std::string row_id = "play_in_editor.game_module_driver.reload." + sanitize_element_id(model.id);
    mirakana::ui::ElementDesc row = make_child(row_id, root, mirakana::ui::SemanticRole::list_item);
    row.text = make_text(model.label);
    row.enabled = model.can_reload;
    add_or_throw(document, std::move(row));

    const mirakana::ui::ElementId row_element{row_id};
    append_label(document, row_element, row_id + ".status", model.status_label);
    append_label(document, row_element, row_id + ".module_path", model.module_path);
    append_label(document, row_element, row_id + ".factory_symbol", model.factory_symbol);
    append_label(document, row_element, row_id + ".abi_contract", model.abi_contract);
    append_label(document, row_element, row_id + ".blocked_by", join_values(model.blocked_by));
    append_label(document, row_element, row_id + ".unsupported_claims", join_values(model.unsupported_claims));
    append_label(document, row_element, row_id + ".diagnostic", join_values(model.diagnostics));
    return document;
}

mirakana::ui::UiDocument
make_editor_game_module_driver_contract_metadata_ui_model(const EditorGameModuleDriverContractMetadataModel& model) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("play_in_editor.game_module_driver.contract", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"play_in_editor.game_module_driver.contract"};

    append_label(document, root, "play_in_editor.game_module_driver.contract.abi_contract", model.abi_contract);
    append_label(document, root, "play_in_editor.game_module_driver.contract.abi_version",
                 std::to_string(model.abi_version));
    append_label(document, root, "play_in_editor.game_module_driver.contract.factory_symbol", model.factory_symbol);
    append_label(document, root, "play_in_editor.game_module_driver.contract.compatibility",
                 model.same_engine_build_required ? "same-engine-build required" : "unspecified");
    append_label(document, root, "play_in_editor.game_module_driver.contract.stable_third_party_abi",
                 model.stable_third_party_abi_supported ? "supported" : "unsupported");

    for (const auto& metadata_row : model.rows) {
        const std::string row_id = "play_in_editor.game_module_driver.contract." + sanitize_element_id(metadata_row.id);
        mirakana::ui::ElementDesc row = make_child(row_id, root, mirakana::ui::SemanticRole::list_item);
        row.text = make_text(metadata_row.label);
        row.enabled = metadata_row.supported;
        add_or_throw(document, std::move(row));

        const mirakana::ui::ElementId row_element{row_id};
        append_label(document, row_element, row_id + ".value", metadata_row.value);
        append_label(document, row_element, row_id + ".required", metadata_row.required ? "required" : "optional");
        append_label(document, row_element, row_id + ".supported",
                     metadata_row.supported ? "supported" : "unsupported");
        append_label(document, row_element, row_id + ".diagnostic", metadata_row.diagnostic);
    }
    return document;
}

mirakana::ui::UiDocument make_editor_game_module_driver_ctest_probe_evidence_ui_model(
    const EditorGameModuleDriverCtestProbeEvidenceModel& model) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("play_in_editor.game_module_driver.ctest_probe_evidence",
                                     mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"play_in_editor.game_module_driver.ctest_probe_evidence"};

    append_label(document, root, "play_in_editor.game_module_driver.ctest_probe_evidence.probe_shared_library_target",
                 model.probe_shared_library_target);
    append_label(document, root, "play_in_editor.game_module_driver.ctest_probe_evidence.ctest_executable_target",
                 model.ctest_executable_target);
    append_label(document, root, "play_in_editor.game_module_driver.ctest_probe_evidence.factory_symbol",
                 model.factory_symbol);
    append_label(document, root, "play_in_editor.game_module_driver.ctest_probe_evidence.host_scope_note",
                 model.host_scope_note);
    append_label(document, root, "play_in_editor.game_module_driver.ctest_probe_evidence.editor_boundary_note",
                 model.editor_boundary_note);
    return document;
}

EditorGameModuleDriverUnloadModel
make_editor_game_module_driver_unload_model(const EditorGameModuleDriverUnloadDesc& desc) {
    EditorGameModuleDriverUnloadModel model;
    model.id = sanitize_element_id(desc.id);
    model.label = desc.label.empty() ? model.id : sanitize_text(desc.label);

    if (!desc.driver_loaded) {
        append_blocker(model, "no-loaded-driver", "game module driver unload requires a loaded driver");
    }
    if (desc.play_session_active) {
        append_blocker(model, "play-session-active",
                       "game module driver unload is blocked while Play-In-Editor is active");
    }

    if (model.has_blocking_diagnostics) {
        model.status = EditorGameModuleDriverStatus::blocked;
    } else {
        model.status = EditorGameModuleDriverStatus::ready;
        model.can_unload = true;
        model.diagnostics.emplace_back("game module driver can be unloaded when Play-In-Editor is stopped");
    }
    model.status_label = std::string(editor_game_module_driver_status_label(model.status));
    return model;
}

mirakana::ui::UiDocument
make_editor_game_module_driver_unload_ui_model(const EditorGameModuleDriverUnloadModel& model) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("play_in_editor.game_module_driver.unload", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"play_in_editor.game_module_driver.unload"};

    const std::string row_id = "play_in_editor.game_module_driver.unload." + sanitize_element_id(model.id);
    mirakana::ui::ElementDesc row = make_child(row_id, root, mirakana::ui::SemanticRole::list_item);
    row.text = make_text(model.label);
    row.enabled = model.can_unload;
    add_or_throw(document, std::move(row));

    const mirakana::ui::ElementId row_element{row_id};
    append_label(document, row_element, row_id + ".status", model.status_label);
    append_label(document, row_element, row_id + ".abi_contract", model.abi_contract);
    append_label(document, row_element, row_id + ".blocked_by", join_values(model.blocked_by));
    append_label(document, row_element, row_id + ".diagnostic", join_values(model.diagnostics));
    return document;
}

mirakana::ui::UiDocument
make_editor_game_module_driver_host_session_ui_model(const EditorGameModuleDriverHostSessionSnapshot& snapshot) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("play_in_editor.game_module_driver.session", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"play_in_editor.game_module_driver.session"};
    append_label(document, root, "play_in_editor.game_module_driver.session.contract_label",
                 std::string(editor_game_module_driver_host_session_contract_v1));
    append_label(document, root, "play_in_editor.game_module_driver.session.phase_id", snapshot.phase_id);
    append_label(document, root, "play_in_editor.game_module_driver.session.summary", snapshot.summary);
    append_label(document, root, "play_in_editor.game_module_driver.session.play_session_active",
                 snapshot.play_session_active ? std::string{"true"} : std::string{"false"});
    append_label(document, root, "play_in_editor.game_module_driver.session.driver_loaded",
                 snapshot.driver_loaded ? std::string{"true"} : std::string{"false"});
    append_label(document, root, "play_in_editor.game_module_driver.session.barriers_contract_label",
                 std::string(editor_game_module_driver_host_session_dll_barriers_contract_v1));
    append_label(document, root, "play_in_editor.game_module_driver.session.barrier.play_dll_surface_mutation.status",
                 snapshot.barrier_play_dll_surface_mutation_status);
    append_label(document, root, "play_in_editor.game_module_driver.session.policy.active_session_hot_reload",
                 snapshot.policy_active_session_hot_reload);
    append_label(document, root, "play_in_editor.game_module_driver.session.policy.stopped_state_reload_scope",
                 snapshot.policy_stopped_state_reload_scope);
    append_label(document, root, "play_in_editor.game_module_driver.session.policy.dll_mutation_order_guidance",
                 snapshot.policy_dll_mutation_order_guidance);
    return document;
}

} // namespace mirakana::editor
