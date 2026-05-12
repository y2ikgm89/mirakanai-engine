// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

struct WindowExtent {
    std::uint32_t width{0};
    std::uint32_t height{0};
};

struct WindowPosition {
    std::int32_t x{0};
    std::int32_t y{0};
};

struct WindowDesc {
    std::string title;
    WindowExtent extent;
    WindowPosition position{};
};

using DisplayId = std::uint32_t;

struct DisplayRect {
    std::int32_t x{0};
    std::int32_t y{0};
    std::uint32_t width{0};
    std::uint32_t height{0};
};

struct DisplayInfo {
    DisplayId id{0};
    std::string name;
    DisplayRect bounds{};
    DisplayRect usable_bounds{};
    float content_scale{1.0F};
    bool primary{false};
};

struct WindowDisplayState {
    DisplayId display_id{0};
    float content_scale{1.0F};
    float pixel_density{1.0F};
    DisplayRect safe_area{};
};

enum class DisplaySelectionPolicy {
    primary,
    specific,
    highest_content_scale,
    largest_usable_area,
};

struct DisplaySelectionRequest {
    DisplaySelectionPolicy policy{DisplaySelectionPolicy::primary};
    DisplayId display_id{0};
};

enum class WindowPlacementPolicy {
    centered,
    top_left,
    absolute,
};

struct WindowPlacementRequest {
    WindowPlacementPolicy policy{WindowPlacementPolicy::centered};
    DisplaySelectionRequest display{};
    WindowExtent extent{};
    WindowPosition position{};
};

struct WindowPlacement {
    WindowPosition position{};
    WindowExtent extent{};
    DisplayId display_id{0};
};

[[nodiscard]] bool is_valid_display_rect(DisplayRect rect) noexcept;
[[nodiscard]] bool is_valid_display_info(const DisplayInfo& info) noexcept;
[[nodiscard]] bool is_valid_window_display_state(WindowDisplayState state) noexcept;
[[nodiscard]] std::optional<DisplayInfo> select_display(const std::vector<DisplayInfo>& displays,
                                                        DisplaySelectionRequest request);
[[nodiscard]] std::optional<WindowPlacement> plan_window_placement(const std::vector<DisplayInfo>& displays,
                                                                   WindowPlacementRequest request);

class IWindow {
  public:
    virtual ~IWindow() = default;

    [[nodiscard]] virtual std::string_view title() const noexcept = 0;
    [[nodiscard]] virtual WindowExtent extent() const noexcept = 0;
    [[nodiscard]] virtual WindowPosition position() const noexcept = 0;
    [[nodiscard]] virtual bool is_open() const noexcept = 0;

    virtual void resize(WindowExtent extent) = 0;
    virtual void move(WindowPosition position) = 0;
    virtual void apply_placement(WindowPlacement placement) = 0;
    virtual void request_close() noexcept = 0;
};

class HeadlessWindow final : public IWindow {
  public:
    explicit HeadlessWindow(WindowDesc desc);

    [[nodiscard]] std::string_view title() const noexcept override;
    [[nodiscard]] WindowExtent extent() const noexcept override;
    [[nodiscard]] WindowPosition position() const noexcept override;
    [[nodiscard]] bool is_open() const noexcept override;

    void resize(WindowExtent extent) override;
    void move(WindowPosition position) override;
    void apply_placement(WindowPlacement placement) override;
    void request_close() noexcept override;

  private:
    std::string title_;
    WindowExtent extent_;
    WindowPosition position_;
    bool open_{true};
};

} // namespace mirakana
