// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/platform/lifecycle.hpp"
#include "mirakana/platform/win32/win32_event_pump.hpp"
#include "mirakana/platform/win32/win32_runtime.hpp"
#include "mirakana/platform/window.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace mirakana::win32 {

struct Win32WindowCreationPlan {
    std::string title;
    WindowExtent client_extent{};
    WindowPosition position{};
    bool resizable{true};
    std::string diagnostic;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostic.empty();
    }
};

[[nodiscard]] Win32WindowCreationPlan plan_win32_window_creation(const WindowDesc& desc);

class Win32WindowModel final : public IWindow {
  public:
    explicit Win32WindowModel(WindowDesc desc);

    [[nodiscard]] std::string_view title() const noexcept override;
    [[nodiscard]] WindowExtent extent() const noexcept override;
    [[nodiscard]] WindowPosition position() const noexcept override;
    [[nodiscard]] bool is_open() const noexcept override;
    [[nodiscard]] bool focused() const noexcept;
    [[nodiscard]] bool minimized() const noexcept;

    void resize(WindowExtent extent) override;
    void move(WindowPosition position) override;
    void apply_placement(WindowPlacement placement) override;
    void request_close() noexcept override;
    void handle_event(const Win32WindowEvent& event, VirtualLifecycle* lifecycle = nullptr) noexcept;

  private:
    std::string title_;
    WindowExtent extent_{};
    WindowPosition position_{};
    bool open_{true};
    bool focused_{true};
    bool minimized_{false};
};

class Win32Window final : public IWindow {
  public:
    Win32Window(Win32Runtime& runtime, WindowDesc desc);
    ~Win32Window() override;

    Win32Window(const Win32Window&) = delete;
    Win32Window& operator=(const Win32Window&) = delete;
    Win32Window(Win32Window&&) = delete;
    Win32Window& operator=(Win32Window&&) = delete;

    [[nodiscard]] std::string_view title() const noexcept override;
    [[nodiscard]] WindowExtent extent() const noexcept override;
    [[nodiscard]] WindowPosition position() const noexcept override;
    [[nodiscard]] bool is_open() const noexcept override;
    [[nodiscard]] WindowDisplayState display_state() const;
    [[nodiscard]] std::uintptr_t native_window_token() const noexcept;

    void resize(WindowExtent extent) override;
    void move(WindowPosition position) override;
    void apply_placement(WindowPlacement placement) override;
    void request_close() noexcept override;
    void post_close_request() noexcept;
    void handle_event(const Win32WindowEvent& event, VirtualLifecycle* lifecycle = nullptr) noexcept;

  private:
    Win32Runtime* runtime_{nullptr};
    std::string title_;
    WindowExtent extent_{};
    WindowPosition position_{};
    std::uintptr_t window_token_{0};
    bool open_{true};
};

} // namespace mirakana::win32
