// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace mirakana::win32 {

struct Win32RuntimeDesc {
    std::string window_class_name{"MIRAIKANAI Win32 Window"};
    bool dpi_aware{true};
};

struct Win32RuntimeStartupPlan {
    bool register_window_class{false};
    bool initialize_dpi_awareness{false};
    std::string window_class_name;
    std::string diagnostic;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostic.empty();
    }
};

struct Win32RuntimeShutdownPlan {
    bool unregister_window_class{false};
    std::string diagnostic;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostic.empty();
    }
};

[[nodiscard]] Win32RuntimeStartupPlan plan_win32_runtime_startup(const Win32RuntimeDesc& desc);
[[nodiscard]] Win32RuntimeShutdownPlan plan_win32_runtime_shutdown(const Win32RuntimeStartupPlan& startup);

class Win32Runtime final {
  public:
    explicit Win32Runtime(Win32RuntimeDesc desc = {});
    ~Win32Runtime();

    Win32Runtime(const Win32Runtime&) = delete;
    Win32Runtime& operator=(const Win32Runtime&) = delete;
    Win32Runtime(Win32Runtime&&) = delete;
    Win32Runtime& operator=(Win32Runtime&&) = delete;

    [[nodiscard]] std::string_view window_class_name() const noexcept;
    [[nodiscard]] std::uintptr_t native_instance_token() const noexcept;

  private:
    std::string window_class_name_;
    std::uintptr_t instance_token_{0};
    bool registered_window_class_{false};
};

} // namespace mirakana::win32
