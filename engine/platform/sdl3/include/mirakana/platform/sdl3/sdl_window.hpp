// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/platform/input.hpp"
#include "mirakana/platform/lifecycle.hpp"
#include "mirakana/platform/window.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

struct SdlRuntimeDesc {
    std::string video_driver_hint;
    bool enable_gamepad{true};
};

class SdlRuntime final {
  public:
    explicit SdlRuntime(SdlRuntimeDesc desc = {});
    ~SdlRuntime();

    SdlRuntime(const SdlRuntime&) = delete;
    SdlRuntime& operator=(const SdlRuntime&) = delete;
    SdlRuntime(SdlRuntime&&) = delete;
    SdlRuntime& operator=(SdlRuntime&&) = delete;

  private:
    std::uint32_t flags_{0};
};

[[nodiscard]] std::vector<DisplayInfo> sdl3_displays();
[[nodiscard]] std::optional<DisplayInfo> sdl3_select_display(DisplaySelectionRequest request);

struct SdlNativeWindowHandle {
    std::uintptr_t value{0};
};

struct SdlRawEventHandle {
    const void* value{nullptr};
};

enum class SdlWindowEventKind {
    unknown = 0,
    quit_requested,
    terminating,
    low_memory,
    will_enter_background,
    did_enter_background,
    will_enter_foreground,
    did_enter_foreground,
    close_requested,
    resized,
    moved,
    pointer_pressed,
    pointer_moved,
    pointer_released,
    pointer_canceled,
    gamepad_connected,
    gamepad_disconnected,
    gamepad_button_pressed,
    gamepad_button_released,
    gamepad_axis_moved,
    key_pressed,
    key_released,
    text_input,
    text_editing,
    text_editing_candidates,
};

struct SdlWindowEvent {
    SdlWindowEventKind kind{SdlWindowEventKind::unknown};
    std::uint32_t window_id{0};
    WindowExtent extent;
    WindowPosition position;
    PointerSample pointer;
    PointerId pointer_id{primary_pointer_id};
    GamepadId gamepad_id{0};
    GamepadButton gamepad_button{GamepadButton::unknown};
    GamepadAxis gamepad_axis{GamepadAxis::unknown};
    float gamepad_axis_value{0.0F};
    Key key{Key::unknown};
    std::int32_t sdl_keycode{0};
    std::uint16_t key_modifiers{0};
    bool repeated{false};
    std::string text;
    std::int32_t text_edit_start{-1};
    std::int32_t text_edit_length{-1};
    std::vector<std::string> text_editing_candidates;
    std::int32_t selected_text_editing_candidate{-1};
    bool text_editing_candidates_horizontal{false};
};

[[nodiscard]] SdlWindowEvent sdl3_translate_window_event(SdlRawEventHandle event, WindowExtent window_extent);

class SdlWindow final : public IWindow {
  public:
    explicit SdlWindow(WindowDesc desc);
    ~SdlWindow() override;

    SdlWindow(const SdlWindow&) = delete;
    SdlWindow& operator=(const SdlWindow&) = delete;
    SdlWindow(SdlWindow&&) = delete;
    SdlWindow& operator=(SdlWindow&&) = delete;

    [[nodiscard]] std::string_view title() const noexcept override;
    [[nodiscard]] WindowExtent extent() const noexcept override;
    [[nodiscard]] WindowPosition position() const noexcept override;
    [[nodiscard]] bool is_open() const noexcept override;
    [[nodiscard]] SdlNativeWindowHandle native_window() const noexcept;
    [[nodiscard]] WindowDisplayState display_state() const;

    void resize(WindowExtent extent) override;
    void move(WindowPosition position) override;
    void apply_placement(WindowPlacement placement) override;
    void request_close() noexcept override;
    void handle_event(const SdlWindowEvent& event, VirtualInput* input = nullptr,
                      VirtualPointerInput* pointer_input = nullptr, VirtualGamepadInput* gamepad_input = nullptr,
                      VirtualLifecycle* lifecycle = nullptr) noexcept;

  private:
    struct Impl;

    std::string title_;
    WindowExtent extent_{};
    WindowPosition position_{};
    std::unique_ptr<Impl> impl_;
    std::uint32_t window_id_{0};
    bool open_{true};
};

} // namespace mirakana
