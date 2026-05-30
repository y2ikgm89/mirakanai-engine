// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/platform/win32/win32_event_pump.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace mirakana::editor {

struct Win32ImguiCopiedMessage {
    mirakana::win32::Win32CopiedMessage message;
    bool imgui_handled{false};
};

struct Win32ImguiMessageBridgeImpl;

class Win32ImguiMessageBridge final {
  public:
    explicit Win32ImguiMessageBridge(std::uintptr_t window_token);
    ~Win32ImguiMessageBridge();

    Win32ImguiMessageBridge(const Win32ImguiMessageBridge&) = delete;
    Win32ImguiMessageBridge& operator=(const Win32ImguiMessageBridge&) = delete;
    Win32ImguiMessageBridge(Win32ImguiMessageBridge&&) = delete;
    Win32ImguiMessageBridge& operator=(Win32ImguiMessageBridge&&) = delete;

    [[nodiscard]] std::vector<Win32ImguiCopiedMessage> drain_messages();

  private:
    std::unique_ptr<Win32ImguiMessageBridgeImpl> impl_;
};

} // namespace mirakana::editor
