// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::rhi {

struct RhiResourceId {
    std::uint64_t value{0};

    friend bool operator==(RhiResourceId lhs, RhiResourceId rhs) noexcept {
        return lhs.value == rhs.value;
    }
};

enum class RhiResourceKind {
    unknown = 0,
    buffer,
    texture,
    sampler,
    swapchain,
    shader,
    descriptor_set_layout,
    descriptor_set,
    pipeline_layout,
    graphics_pipeline,
    compute_pipeline,
};

enum class RhiResourceLifetimeState { live = 0, deferred_release };

enum class RhiResourceLifetimeEventKind {
    register_resource = 0,
    rename,
    defer_release,
    retire,
    invalid_registration,
    invalid_resource,
    duplicate_release,
    stale_generation,
    invalid_debug_name,
};

enum class RhiResourceLifetimeDiagnosticCode {
    invalid_resource = 0,
    duplicate_release,
    stale_generation,
    invalid_debug_name,
    invalid_registration,
};

struct RhiResourceHandle {
    RhiResourceId id;
    std::uint32_t generation{0};

    friend bool operator==(RhiResourceHandle lhs, RhiResourceHandle rhs) noexcept {
        return lhs.id == rhs.id && lhs.generation == rhs.generation;
    }
};

struct RhiResourceRegistrationDesc {
    RhiResourceKind kind{RhiResourceKind::unknown};
    std::string owner;
    std::string debug_name;
};

struct RhiResourceLifetimeDiagnostic {
    RhiResourceLifetimeDiagnosticCode code{RhiResourceLifetimeDiagnosticCode::invalid_resource};
    RhiResourceHandle handle;
    std::string message;
};

struct RhiResourceLifetimeResult {
    std::vector<RhiResourceLifetimeDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

struct RhiResourceRegistrationResult {
    RhiResourceHandle handle;
    std::vector<RhiResourceLifetimeDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

struct RhiResourceLifetimeRecord {
    RhiResourceHandle handle;
    RhiResourceKind kind{RhiResourceKind::unknown};
    RhiResourceLifetimeState state{RhiResourceLifetimeState::live};
    std::string owner;
    std::string debug_name;
    std::uint64_t release_frame{0};
};

struct RhiResourceLifetimeEvent {
    RhiResourceLifetimeEventKind kind{RhiResourceLifetimeEventKind::register_resource};
    RhiResourceHandle handle;
    RhiResourceKind resource_kind{RhiResourceKind::unknown};
    std::string owner;
    std::string debug_name;
    std::uint64_t frame{0};
};

class RhiResourceLifetimeRegistry {
  public:
    [[nodiscard]] RhiResourceRegistrationResult register_resource(RhiResourceRegistrationDesc desc);
    [[nodiscard]] RhiResourceLifetimeResult set_debug_name(RhiResourceHandle handle, std::string debug_name);
    [[nodiscard]] RhiResourceLifetimeResult release_resource_deferred(RhiResourceHandle handle,
                                                                      std::uint64_t release_frame);
    [[nodiscard]] std::uint32_t retire_released_resources(std::uint64_t completed_frame);
    [[nodiscard]] bool is_live(RhiResourceHandle handle) const noexcept;
    [[nodiscard]] const std::vector<RhiResourceLifetimeRecord>& records() const noexcept;
    [[nodiscard]] const std::vector<RhiResourceLifetimeEvent>& events() const noexcept;

  private:
    std::vector<RhiResourceLifetimeRecord> records_;
    std::vector<RhiResourceLifetimeEvent> events_;
    std::uint64_t next_id_{1};
};

} // namespace mirakana::rhi
