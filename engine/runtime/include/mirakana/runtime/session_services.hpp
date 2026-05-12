// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/platform/filesystem.hpp"
#include "mirakana/platform/input.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::runtime {

struct RuntimeKeyValue {
    std::string key;
    std::string value;
};

class RuntimeSaveData {
  public:
    std::uint32_t schema_version{1};

    void set_value(std::string key, std::string value);
    [[nodiscard]] const std::string* find_value(std::string_view key) const noexcept;
    [[nodiscard]] std::string value_or(std::string_view key, std::string_view fallback) const;
    [[nodiscard]] const std::vector<RuntimeKeyValue>& entries() const noexcept;

  private:
    std::vector<RuntimeKeyValue> entries_;
};

struct RuntimeSaveDataLoadResult {
    RuntimeSaveData data;
    std::string diagnostic;

    [[nodiscard]] bool succeeded() const noexcept;
};

class RuntimeSettings {
  public:
    std::uint32_t schema_version{1};

    void set_value(std::string key, std::string value);
    [[nodiscard]] const std::string* find_value(std::string_view key) const noexcept;
    [[nodiscard]] std::string value_or(std::string_view key, std::string_view fallback) const;
    [[nodiscard]] const std::vector<RuntimeKeyValue>& entries() const noexcept;

  private:
    std::vector<RuntimeKeyValue> entries_;
};

struct RuntimeSettingsLoadResult {
    RuntimeSettings settings;
    std::string diagnostic;

    [[nodiscard]] bool succeeded() const noexcept;
};

struct RuntimeLocalizationEntry {
    std::string key;
    std::string text;
};

class RuntimeLocalizationCatalog {
  public:
    std::string locale;

    void set_text(std::string key, std::string text);
    [[nodiscard]] const std::string* find_text(std::string_view key) const noexcept;
    [[nodiscard]] std::string text_or_key(std::string_view key) const;
    [[nodiscard]] const std::vector<RuntimeLocalizationEntry>& entries() const noexcept;

  private:
    std::vector<RuntimeLocalizationEntry> entries_;
};

struct RuntimeLocalizationCatalogLoadResult {
    RuntimeLocalizationCatalog catalog;
    std::string diagnostic;

    [[nodiscard]] bool succeeded() const noexcept;
};

enum class RuntimeInputActionTriggerKind {
    key,
    pointer,
    gamepad_button,
};

struct RuntimeInputActionTrigger {
    RuntimeInputActionTriggerKind kind{RuntimeInputActionTriggerKind::key};
    Key key{Key::unknown};
    PointerId pointer_id{0};
    GamepadId gamepad_id{0};
    GamepadButton gamepad_button{GamepadButton::unknown};
};

inline constexpr std::string_view runtime_input_default_context = "default";

struct RuntimeInputContextStack {
    std::vector<std::string> active_contexts;
};

struct RuntimeInputActionBinding {
    std::string context;
    std::string action;
    std::vector<RuntimeInputActionTrigger> triggers;
};

enum class RuntimeInputAxisSourceKind {
    key_pair,
    gamepad_axis,
};

struct RuntimeInputAxisSource {
    RuntimeInputAxisSourceKind kind{RuntimeInputAxisSourceKind::key_pair};
    Key negative_key{Key::unknown};
    Key positive_key{Key::unknown};
    GamepadId gamepad_id{0};
    GamepadAxis gamepad_axis{GamepadAxis::unknown};
    float scale{1.0F};
    float deadzone{0.0F};
};

struct RuntimeInputAxisBinding {
    std::string context;
    std::string action;
    std::vector<RuntimeInputAxisSource> sources;
};

struct RuntimeInputStateView {
    const VirtualInput* keyboard{nullptr};
    const VirtualPointerInput* pointer{nullptr};
    const VirtualGamepadInput* gamepad{nullptr};
};

class RuntimeInputActionMap {
  public:
    void bind_key(std::string action, Key key);
    void bind_pointer(std::string action, PointerId pointer_id);
    void bind_gamepad_button(std::string action, GamepadId gamepad_id, GamepadButton button);
    void bind_key_axis(std::string action, Key negative_key, Key positive_key);
    void bind_gamepad_axis(std::string action, GamepadId gamepad_id, GamepadAxis axis, float scale = 1.0F,
                           float deadzone = 0.0F);
    void bind_key_in_context(std::string context, std::string action, Key key);
    void bind_pointer_in_context(std::string context, std::string action, PointerId pointer_id);
    void bind_gamepad_button_in_context(std::string context, std::string action, GamepadId gamepad_id,
                                        GamepadButton button);
    void bind_key_axis_in_context(std::string context, std::string action, Key negative_key, Key positive_key);
    void bind_gamepad_axis_in_context(std::string context, std::string action, GamepadId gamepad_id, GamepadAxis axis,
                                      float scale = 1.0F, float deadzone = 0.0F);

    [[nodiscard]] const RuntimeInputActionBinding* find(std::string_view action) const noexcept;
    [[nodiscard]] const RuntimeInputActionBinding* find(std::string_view context,
                                                        std::string_view action) const noexcept;
    [[nodiscard]] const RuntimeInputAxisBinding* find_axis(std::string_view action) const noexcept;
    [[nodiscard]] const RuntimeInputAxisBinding* find_axis(std::string_view context,
                                                           std::string_view action) const noexcept;
    [[nodiscard]] bool action_down(std::string_view action, RuntimeInputStateView state) const noexcept;
    [[nodiscard]] bool action_down(std::string_view action, RuntimeInputStateView state,
                                   const RuntimeInputContextStack& context_stack) const noexcept;
    [[nodiscard]] bool action_pressed(std::string_view action, RuntimeInputStateView state) const noexcept;
    [[nodiscard]] bool action_pressed(std::string_view action, RuntimeInputStateView state,
                                      const RuntimeInputContextStack& context_stack) const noexcept;
    [[nodiscard]] bool action_released(std::string_view action, RuntimeInputStateView state) const noexcept;
    [[nodiscard]] bool action_released(std::string_view action, RuntimeInputStateView state,
                                       const RuntimeInputContextStack& context_stack) const noexcept;
    [[nodiscard]] float axis_value(std::string_view action, RuntimeInputStateView state) const noexcept;
    [[nodiscard]] float axis_value(std::string_view action, RuntimeInputStateView state,
                                   const RuntimeInputContextStack& context_stack) const noexcept;
    [[nodiscard]] const std::vector<RuntimeInputActionBinding>& bindings() const noexcept;
    [[nodiscard]] const std::vector<RuntimeInputAxisBinding>& axis_bindings() const noexcept;

  private:
    void bind_trigger(std::string context, std::string action, RuntimeInputActionTrigger trigger);
    void bind_axis_source(std::string context, std::string action, RuntimeInputAxisSource source);
    [[nodiscard]] const RuntimeInputActionBinding*
    find_active(std::string_view action, const RuntimeInputContextStack& context_stack) const noexcept;
    [[nodiscard]] const RuntimeInputAxisBinding*
    find_active_axis(std::string_view action, const RuntimeInputContextStack& context_stack) const noexcept;

    std::vector<RuntimeInputActionBinding> bindings_;
    std::vector<RuntimeInputAxisBinding> axis_bindings_;
};

struct RuntimeInputActionMapLoadResult {
    RuntimeInputActionMap actions;
    std::string diagnostic;

    [[nodiscard]] bool succeeded() const noexcept;
};

struct RuntimeInputRebindingActionOverride {
    std::string context;
    std::string action;
    std::vector<RuntimeInputActionTrigger> triggers;
};

struct RuntimeInputRebindingAxisOverride {
    std::string context;
    std::string action;
    std::vector<RuntimeInputAxisSource> sources;
};

struct RuntimeInputRebindingProfile {
    std::string profile_id{"default"};
    std::vector<RuntimeInputRebindingActionOverride> action_overrides;
    std::vector<RuntimeInputRebindingAxisOverride> axis_overrides;
};

enum class RuntimeInputRebindingDiagnosticCode {
    unsupported_format,
    invalid_profile_id,
    invalid_context,
    invalid_action,
    missing_base_binding,
    empty_override,
    duplicate_override,
    duplicate_trigger,
    duplicate_axis_source,
    trigger_conflict,
    invalid_trigger,
    invalid_axis_source,
    capture_not_armed,
    invalid_capture_id,
    invalid_capture_focus,
    modal_layer_mismatch,
};

struct RuntimeInputRebindingDiagnostic {
    RuntimeInputRebindingDiagnosticCode code{RuntimeInputRebindingDiagnosticCode::invalid_profile_id};
    std::string path;
    std::string message;
};

struct RuntimeInputRebindingProfileLoadResult {
    RuntimeInputRebindingProfile profile;
    std::string diagnostic;

    [[nodiscard]] bool succeeded() const noexcept;
};

struct RuntimeInputRebindingProfileApplyResult {
    RuntimeInputActionMap actions;
    std::vector<RuntimeInputRebindingDiagnostic> diagnostics;
    std::size_t action_overrides_applied{0};
    std::size_t axis_overrides_applied{0};

    [[nodiscard]] bool succeeded() const noexcept;
};

enum class RuntimeInputRebindingCaptureStatus {
    waiting,
    captured,
    blocked,
};

struct RuntimeInputRebindingCaptureRequest {
    std::string context;
    std::string action;
    RuntimeInputStateView state;
    bool capture_keyboard{true};
    bool capture_pointer{true};
    bool capture_gamepad_buttons{true};
};

struct RuntimeInputRebindingCaptureResult {
    RuntimeInputRebindingCaptureStatus status{RuntimeInputRebindingCaptureStatus::waiting};
    std::optional<RuntimeInputActionTrigger> trigger;
    RuntimeInputRebindingActionOverride action_override;
    RuntimeInputRebindingProfile candidate_profile;
    std::vector<RuntimeInputRebindingDiagnostic> diagnostics;

    [[nodiscard]] bool captured() const noexcept;
    [[nodiscard]] bool waiting() const noexcept;
};

struct RuntimeInputRebindingAxisCaptureRequest {
    std::string context;
    std::string action;
    RuntimeInputStateView state;
    float capture_deadzone{0.25F};
    bool capture_gamepad_axes{true};
    /// When true, capture resolves a key-pair axis source from two simultaneously held keyboard keys (deterministic
    /// negative/positive assignment uses ascending Key enum order among held keys).
    bool capture_keyboard_key_pair_axes{true};
};

struct RuntimeInputRebindingAxisCaptureResult {
    RuntimeInputRebindingCaptureStatus status{RuntimeInputRebindingCaptureStatus::waiting};
    std::optional<RuntimeInputAxisSource> source;
    RuntimeInputRebindingAxisOverride axis_override;
    RuntimeInputRebindingProfile candidate_profile;
    std::vector<RuntimeInputRebindingDiagnostic> diagnostics;

    [[nodiscard]] bool captured() const noexcept;
    [[nodiscard]] bool waiting() const noexcept;
};

struct RuntimeInputRebindingFocusCaptureRequest {
    RuntimeInputRebindingCaptureRequest capture;
    std::string capture_id;
    std::string focused_id;
    std::string modal_layer_id;
    bool armed{false};
    bool consume_gameplay_input{true};
};

struct RuntimeInputRebindingFocusCaptureResult {
    RuntimeInputRebindingCaptureResult capture;
    std::string active_capture_id;
    bool gameplay_input_consumed{false};
    bool focus_retained{false};
    std::vector<RuntimeInputRebindingDiagnostic> diagnostics;

    [[nodiscard]] bool captured() const noexcept;
    [[nodiscard]] bool waiting() const noexcept;
    [[nodiscard]] bool blocked() const noexcept;
};

enum class RuntimeInputRebindingPresentationTokenKind {
    key,
    pointer,
    gamepad_button,
    key_pair,
    gamepad_axis,
};

struct RuntimeInputRebindingPresentationToken {
    RuntimeInputRebindingPresentationTokenKind kind{RuntimeInputRebindingPresentationTokenKind::key};
    std::string label;
    std::string glyph_lookup_key;
    Key key{Key::unknown};
    PointerId pointer_id{0};
    GamepadId gamepad_id{0};
    GamepadButton gamepad_button{GamepadButton::unknown};
    Key negative_key{Key::unknown};
    Key positive_key{Key::unknown};
    GamepadAxis gamepad_axis{GamepadAxis::unknown};
    float scale{1.0F};
    float deadzone{0.0F};
};

enum class RuntimeInputRebindingPresentationRowKind {
    action,
    axis,
};

struct RuntimeInputRebindingPresentationRow {
    std::string id;
    RuntimeInputRebindingPresentationRowKind kind{RuntimeInputRebindingPresentationRowKind::action};
    std::string context;
    std::string action;
    std::vector<RuntimeInputRebindingPresentationToken> base_tokens;
    std::vector<RuntimeInputRebindingPresentationToken> profile_tokens;
    bool overridden{false};
    bool ready{false};
    std::string diagnostic;
};

struct RuntimeInputRebindingPresentationModel {
    std::vector<RuntimeInputRebindingPresentationRow> rows;
    std::vector<RuntimeInputRebindingDiagnostic> diagnostics;
    bool ready_for_display{false};
    bool has_blocking_diagnostics{false};
    bool has_profile_overrides{false};

    [[nodiscard]] bool ready() const noexcept;
};

[[nodiscard]] std::string serialize_runtime_save_data(const RuntimeSaveData& data);
[[nodiscard]] RuntimeSaveDataLoadResult deserialize_runtime_save_data(std::string_view text);
[[nodiscard]] RuntimeSaveDataLoadResult load_runtime_save_data(IFileSystem& filesystem, std::string_view path);
void write_runtime_save_data(IFileSystem& filesystem, std::string_view path, const RuntimeSaveData& data);

[[nodiscard]] std::string serialize_runtime_settings(const RuntimeSettings& settings);
[[nodiscard]] RuntimeSettingsLoadResult deserialize_runtime_settings(std::string_view text);
[[nodiscard]] RuntimeSettingsLoadResult load_runtime_settings(IFileSystem& filesystem, std::string_view path);
void write_runtime_settings(IFileSystem& filesystem, std::string_view path, const RuntimeSettings& settings);

[[nodiscard]] std::string serialize_runtime_localization_catalog(const RuntimeLocalizationCatalog& catalog);
[[nodiscard]] RuntimeLocalizationCatalogLoadResult deserialize_runtime_localization_catalog(std::string_view text);
[[nodiscard]] RuntimeLocalizationCatalogLoadResult load_runtime_localization_catalog(IFileSystem& filesystem,
                                                                                     std::string_view path);
void write_runtime_localization_catalog(IFileSystem& filesystem, std::string_view path,
                                        const RuntimeLocalizationCatalog& catalog);

[[nodiscard]] std::string serialize_runtime_input_actions(const RuntimeInputActionMap& actions);
[[nodiscard]] RuntimeInputActionMapLoadResult deserialize_runtime_input_actions(std::string_view text);
[[nodiscard]] RuntimeInputActionMapLoadResult load_runtime_input_actions(IFileSystem& filesystem,
                                                                         std::string_view path);
void write_runtime_input_actions(IFileSystem& filesystem, std::string_view path, const RuntimeInputActionMap& actions);

[[nodiscard]] std::vector<RuntimeInputRebindingDiagnostic>
validate_runtime_input_rebinding_profile(const RuntimeInputActionMap& base,
                                         const RuntimeInputRebindingProfile& profile);
[[nodiscard]] RuntimeInputRebindingProfileApplyResult
apply_runtime_input_rebinding_profile(const RuntimeInputActionMap& base, const RuntimeInputRebindingProfile& profile);
[[nodiscard]] RuntimeInputRebindingCaptureResult
capture_runtime_input_rebinding_action(const RuntimeInputActionMap& base, const RuntimeInputRebindingProfile& profile,
                                       const RuntimeInputRebindingCaptureRequest& request);
[[nodiscard]] RuntimeInputRebindingAxisCaptureResult
capture_runtime_input_rebinding_axis(const RuntimeInputActionMap& base, const RuntimeInputRebindingProfile& profile,
                                     const RuntimeInputRebindingAxisCaptureRequest& request);
[[nodiscard]] RuntimeInputRebindingFocusCaptureResult
capture_runtime_input_rebinding_action_with_focus(const RuntimeInputActionMap& base,
                                                  const RuntimeInputRebindingProfile& profile,
                                                  const RuntimeInputRebindingFocusCaptureRequest& request);
[[nodiscard]] RuntimeInputRebindingPresentationToken
present_runtime_input_action_trigger(const RuntimeInputActionTrigger& trigger);
[[nodiscard]] RuntimeInputRebindingPresentationToken
present_runtime_input_axis_source(const RuntimeInputAxisSource& source);
[[nodiscard]] RuntimeInputRebindingPresentationModel
make_runtime_input_rebinding_presentation(const RuntimeInputActionMap& base,
                                          const RuntimeInputRebindingProfile& profile);
[[nodiscard]] std::string serialize_runtime_input_rebinding_profile(const RuntimeInputRebindingProfile& profile);
[[nodiscard]] RuntimeInputRebindingProfileLoadResult deserialize_runtime_input_rebinding_profile(std::string_view text);
[[nodiscard]] RuntimeInputRebindingProfileLoadResult load_runtime_input_rebinding_profile(IFileSystem& filesystem,
                                                                                          std::string_view path);
void write_runtime_input_rebinding_profile(IFileSystem& filesystem, std::string_view path,
                                           const RuntimeInputRebindingProfile& profile);

} // namespace mirakana::runtime
