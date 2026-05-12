// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/editor/play_in_editor.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

inline constexpr std::string_view editor_game_module_driver_abi_name_v1{"GameEngine.EditorGameModuleDriver.v1"};
inline constexpr std::uint32_t editor_game_module_driver_abi_version_v1{1U};
inline constexpr std::string_view editor_game_module_driver_factory_symbol_v1{
    "mirakana_create_editor_game_module_driver_v1"};

using EditorGameModuleDriverBeginCallback = void (*)(void* user_data, Scene* scene);
using EditorGameModuleDriverTickCallback = void (*)(void* user_data, Scene* scene,
                                                    const EditorPlaySessionTickContext* context);
using EditorGameModuleDriverEndCallback = void (*)(void* user_data, Scene* scene);
using EditorGameModuleDriverDestroyCallback = void (*)(void* user_data) noexcept;

struct EditorGameModuleDriverApi {
    std::uint32_t abi_version{editor_game_module_driver_abi_version_v1};
    void* user_data{nullptr};
    EditorGameModuleDriverBeginCallback begin{nullptr};
    EditorGameModuleDriverTickCallback tick{nullptr};
    EditorGameModuleDriverEndCallback end{nullptr};
    EditorGameModuleDriverDestroyCallback destroy{nullptr};
};

using EditorGameModuleDriverFactoryFn = EditorGameModuleDriverApi (*)();

enum class EditorGameModuleDriverStatus : std::uint8_t {
    ready,
    blocked,
};

struct EditorGameModuleDriverContractMetadataRow {
    std::string id;
    std::string label;
    std::string value;
    std::string diagnostic;
    bool required{false};
    bool supported{false};
};

struct EditorGameModuleDriverContractMetadataModel {
    std::string id{"editor_game_module_driver_contract_v1"};
    std::string abi_contract{std::string(editor_game_module_driver_abi_name_v1)};
    std::uint32_t abi_version{editor_game_module_driver_abi_version_v1};
    std::string factory_symbol{std::string(editor_game_module_driver_factory_symbol_v1)};
    std::vector<EditorGameModuleDriverContractMetadataRow> rows;
    bool same_engine_build_required{true};
    bool stable_third_party_abi_supported{false};
    bool hot_reload_supported{false};
    bool active_session_reload_supported{false};
    bool desktop_game_runner_embedding_supported{false};
    bool mutates{false};
    bool executes{false};
};

struct EditorGameModuleDriverCreateResult {
    EditorGameModuleDriverStatus status{EditorGameModuleDriverStatus::blocked};
    std::unique_ptr<IEditorPlaySessionDriver> driver;
    std::vector<std::string> blocked_by;
    std::vector<std::string> diagnostics;
};

struct EditorGameModuleDriverLoadDesc {
    std::string id;
    std::string label;
    std::string module_path;
    std::string factory_symbol{std::string(editor_game_module_driver_factory_symbol_v1)};
    /// When true, load review blocks until Play-In-Editor stops (matches shell gate).
    bool play_session_active{false};
    /// When true, load review blocks because a driver is already loaded (use reload/unload first).
    bool driver_already_loaded{false};
    bool request_hot_reload{false};
    bool request_desktop_game_runner_embedding{false};
    bool request_package_script_execution{false};
    bool request_validation_recipe_execution{false};
    bool request_arbitrary_shell_execution{false};
    bool request_renderer_rhi_uploads{false};
    bool request_renderer_rhi_handle_exposure{false};
    bool request_package_streaming{false};
    bool request_stable_third_party_abi{false};
    bool request_broad_editor_productization{false};
};

struct EditorGameModuleDriverLoadModel {
    std::string id;
    std::string label;
    std::string module_path;
    std::string factory_symbol;
    std::string abi_contract;
    std::uint32_t abi_version{editor_game_module_driver_abi_version_v1};
    EditorGameModuleDriverStatus status{EditorGameModuleDriverStatus::blocked};
    std::string status_label;
    bool can_load{false};
    bool has_blocking_diagnostics{false};
    std::vector<std::string> blocked_by;
    std::vector<std::string> unsupported_claims;
    std::vector<std::string> diagnostics;
};

struct EditorGameModuleDriverReloadDesc {
    std::string id;
    std::string label;
    std::string module_path;
    std::string factory_symbol{std::string(editor_game_module_driver_factory_symbol_v1)};
    bool driver_loaded{false};
    bool play_session_active{false};
    bool request_hot_reload{false};
    bool request_active_session_reload{false};
    bool request_desktop_game_runner_embedding{false};
    bool request_package_script_execution{false};
    bool request_validation_recipe_execution{false};
    bool request_arbitrary_shell_execution{false};
    bool request_renderer_rhi_uploads{false};
    bool request_renderer_rhi_handle_exposure{false};
    bool request_package_streaming{false};
    bool request_stable_third_party_abi{false};
    bool request_broad_editor_productization{false};
};

struct EditorGameModuleDriverReloadModel {
    std::string id;
    std::string label;
    std::string module_path;
    std::string factory_symbol;
    std::string abi_contract;
    std::uint32_t abi_version{editor_game_module_driver_abi_version_v1};
    EditorGameModuleDriverStatus status{EditorGameModuleDriverStatus::blocked};
    std::string status_label;
    bool can_reload{false};
    bool has_blocking_diagnostics{false};
    std::vector<std::string> blocked_by;
    std::vector<std::string> unsupported_claims;
    std::vector<std::string> diagnostics;
};

struct EditorGameModuleDriverCtestProbeEvidenceModel {
    std::string id{"editor_game_module_driver_ctest_probe_evidence_v1"};
    std::string probe_shared_library_target{"MK_editor_game_module_driver_probe"};
    std::string ctest_executable_target{"MK_editor_game_module_driver_load_tests"};
    std::string factory_symbol{std::string(editor_game_module_driver_factory_symbol_v1)};
    std::string host_scope_note{
        "Windows-only CTest target resolves the probe DLL via MK_EDITOR_GAME_MODULE_DRIVER_PROBE_PATH"};
    std::string editor_boundary_note{
        "Visible editor module path is independent; UI does not run CTest or embed probe output paths"};
};

struct EditorGameModuleDriverReloadTransactionRecipeEvidenceModel {
    std::string id{"editor_game_module_driver_reload_transaction_recipe_evidence_v1"};
    std::string validation_recipe_id;
    std::string host_gate_acknowledgement_id;
    std::string reviewed_dry_run_command;
    std::string reviewed_execute_command;
    std::string editor_boundary_note;
};

[[nodiscard]] constexpr std::string_view
editor_game_module_driver_reload_transaction_recipe_evidence_contract_v1() noexcept {
    return "ge.editor.editor_game_module_driver_reload_transaction_recipe_evidence.v1";
}

struct EditorGameModuleDriverUnloadDesc {
    std::string id;
    std::string label;
    bool driver_loaded{false};
    bool play_session_active{false};
};

struct EditorGameModuleDriverUnloadModel {
    std::string id;
    std::string label;
    std::string abi_contract{std::string(editor_game_module_driver_abi_name_v1)};
    std::uint32_t abi_version{editor_game_module_driver_abi_version_v1};
    EditorGameModuleDriverStatus status{EditorGameModuleDriverStatus::blocked};
    std::string status_label;
    bool can_unload{false};
    bool has_blocking_diagnostics{false};
    std::vector<std::string> blocked_by;
    std::vector<std::string> diagnostics;
};

inline constexpr std::string_view editor_game_module_driver_host_session_contract_v1{
    "ge.editor.editor_game_module_driver_host_session.v1"};
inline constexpr std::string_view editor_game_module_driver_host_session_dll_barriers_contract_v1{
    "ge.editor.editor_game_module_driver_host_session_dll_barriers.v1"};

enum class EditorGameModuleDriverHostSessionPhase : std::uint8_t {
    idle_no_driver_play_stopped,
    driver_resident_play_stopped,
    play_active_without_driver,
    play_active_with_driver,
};

struct EditorGameModuleDriverHostSessionSnapshot {
    EditorGameModuleDriverHostSessionPhase phase{EditorGameModuleDriverHostSessionPhase::idle_no_driver_play_stopped};
    std::string phase_id{"idle_no_driver_play_stopped"};
    std::string summary;
    bool play_session_active{false};
    bool driver_loaded{false};
    std::string barrier_play_dll_surface_mutation_status{"inactive_play_stopped"};
    std::string policy_active_session_hot_reload{"unsupported_no_silent_dll_replacement_mid_play"};
    std::string policy_stopped_state_reload_scope{
        "reload_and_duplicate_load_reviews_require_play_stopped_explicit_paths"};
    /// Canonical operator order for DLL residency changes (fail-closed; does not authorize mid-play mutation).
    std::string policy_dll_mutation_order_guidance;
};

[[nodiscard]] EditorGameModuleDriverHostSessionSnapshot
make_editor_game_module_driver_host_session_snapshot(bool play_session_active, bool driver_loaded);

[[nodiscard]] EditorGameModuleDriverContractMetadataModel make_editor_game_module_driver_contract_metadata_model();
[[nodiscard]] EditorGameModuleDriverCtestProbeEvidenceModel make_editor_game_module_driver_ctest_probe_evidence_model();
[[nodiscard]] EditorGameModuleDriverLoadModel
make_editor_game_module_driver_load_model(const EditorGameModuleDriverLoadDesc& desc);
[[nodiscard]] EditorGameModuleDriverReloadModel
make_editor_game_module_driver_reload_model(const EditorGameModuleDriverReloadDesc& desc);
[[nodiscard]] EditorGameModuleDriverCreateResult make_editor_game_module_driver_from_api(EditorGameModuleDriverApi api);
[[nodiscard]] EditorGameModuleDriverCreateResult
make_editor_game_module_driver_from_symbol(EditorGameModuleDriverFactoryFn factory);
[[nodiscard]] mirakana::ui::UiDocument
make_editor_game_module_driver_load_ui_model(const EditorGameModuleDriverLoadModel& model);
[[nodiscard]] mirakana::ui::UiDocument
make_editor_game_module_driver_reload_ui_model(const EditorGameModuleDriverReloadModel& model);
[[nodiscard]] mirakana::ui::UiDocument
make_editor_game_module_driver_contract_metadata_ui_model(const EditorGameModuleDriverContractMetadataModel& model);
[[nodiscard]] mirakana::ui::UiDocument make_editor_game_module_driver_ctest_probe_evidence_ui_model(
    const EditorGameModuleDriverCtestProbeEvidenceModel& model);
[[nodiscard]] EditorGameModuleDriverReloadTransactionRecipeEvidenceModel
make_editor_game_module_driver_reload_transaction_recipe_evidence_model();
[[nodiscard]] mirakana::ui::UiDocument make_editor_game_module_driver_reload_transaction_recipe_evidence_ui_model(
    const EditorGameModuleDriverReloadTransactionRecipeEvidenceModel& model);
[[nodiscard]] EditorGameModuleDriverUnloadModel
make_editor_game_module_driver_unload_model(const EditorGameModuleDriverUnloadDesc& desc);
[[nodiscard]] mirakana::ui::UiDocument
make_editor_game_module_driver_unload_ui_model(const EditorGameModuleDriverUnloadModel& model);
[[nodiscard]] mirakana::ui::UiDocument
make_editor_game_module_driver_host_session_ui_model(const EditorGameModuleDriverHostSessionSnapshot& snapshot);

} // namespace mirakana::editor
