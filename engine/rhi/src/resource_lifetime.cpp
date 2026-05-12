// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/rhi/resource_lifetime.hpp"

#include <algorithm>
#include <string_view>
#include <utility>

namespace mirakana::rhi {
namespace {

[[nodiscard]] bool has_ascii_control_character(std::string_view value) noexcept {
    return std::ranges::any_of(value, [](char character) {
        const auto byte = static_cast<unsigned char>(character);
        return byte < 0x20U || byte == 0x7FU;
    });
}

[[nodiscard]] RhiResourceLifetimeDiagnostic make_diagnostic(RhiResourceLifetimeDiagnosticCode code,
                                                            RhiResourceHandle handle, std::string message) {
    return RhiResourceLifetimeDiagnostic{.code = code, .handle = handle, .message = std::move(message)};
}

} // namespace

RhiResourceRegistrationResult RhiResourceLifetimeRegistry::register_resource(RhiResourceRegistrationDesc desc) {
    RhiResourceRegistrationResult result;
    if (desc.kind == RhiResourceKind::unknown || desc.owner.empty()) {
        result.diagnostics.push_back(
            make_diagnostic(RhiResourceLifetimeDiagnosticCode::invalid_registration, {},
                            "RHI resource registration requires a known kind and non-empty owner."));
        events_.push_back(RhiResourceLifetimeEvent{
            .kind = RhiResourceLifetimeEventKind::invalid_registration,
            .handle = {},
            .resource_kind = desc.kind,
            .owner = std::move(desc.owner),
            .debug_name = std::move(desc.debug_name),
            .frame = 0,
        });
        return result;
    }
    if (has_ascii_control_character(desc.debug_name)) {
        result.diagnostics.push_back(
            make_diagnostic(RhiResourceLifetimeDiagnosticCode::invalid_debug_name, {},
                            "RHI resource debug names must not contain ASCII control characters."));
        events_.push_back(RhiResourceLifetimeEvent{
            .kind = RhiResourceLifetimeEventKind::invalid_debug_name,
            .handle = {},
            .resource_kind = desc.kind,
            .owner = std::move(desc.owner),
            .debug_name = std::move(desc.debug_name),
            .frame = 0,
        });
        return result;
    }

    const auto handle = RhiResourceHandle{.id = RhiResourceId{next_id_++}, .generation = 1};
    records_.push_back(RhiResourceLifetimeRecord{
        .handle = handle,
        .kind = desc.kind,
        .state = RhiResourceLifetimeState::live,
        .owner = std::move(desc.owner),
        .debug_name = std::move(desc.debug_name),
        .release_frame = 0,
    });
    const auto& record = records_.back();
    events_.push_back(RhiResourceLifetimeEvent{
        .kind = RhiResourceLifetimeEventKind::register_resource,
        .handle = record.handle,
        .resource_kind = record.kind,
        .owner = record.owner,
        .debug_name = record.debug_name,
        .frame = 0,
    });
    result.handle = handle;
    return result;
}

RhiResourceLifetimeResult RhiResourceLifetimeRegistry::set_debug_name(RhiResourceHandle handle,
                                                                      std::string debug_name) {
    RhiResourceLifetimeResult result;
    if (has_ascii_control_character(debug_name)) {
        result.diagnostics.push_back(
            make_diagnostic(RhiResourceLifetimeDiagnosticCode::invalid_debug_name, handle,
                            "RHI resource debug names must not contain ASCII control characters."));
        events_.push_back(RhiResourceLifetimeEvent{.kind = RhiResourceLifetimeEventKind::invalid_debug_name,
                                                   .handle = handle,
                                                   .resource_kind = RhiResourceKind::unknown,
                                                   .owner = {},
                                                   .debug_name = std::move(debug_name),
                                                   .frame = 0});
        return result;
    }

    const auto record = std::ranges::find_if(
        records_, [handle](const RhiResourceLifetimeRecord& candidate) { return candidate.handle.id == handle.id; });
    if (record == records_.end()) {
        result.diagnostics.push_back(make_diagnostic(RhiResourceLifetimeDiagnosticCode::invalid_resource, handle,
                                                     "RHI resource handle does not refer to a registered resource."));
        events_.push_back(RhiResourceLifetimeEvent{.kind = RhiResourceLifetimeEventKind::invalid_resource,
                                                   .handle = handle,
                                                   .resource_kind = RhiResourceKind::unknown,
                                                   .owner = {},
                                                   .debug_name = {},
                                                   .frame = 0});
        return result;
    }
    if (record->handle.generation != handle.generation) {
        result.diagnostics.push_back(
            make_diagnostic(RhiResourceLifetimeDiagnosticCode::stale_generation, handle,
                            "RHI resource handle generation does not match the registered resource."));
        events_.push_back(RhiResourceLifetimeEvent{.kind = RhiResourceLifetimeEventKind::stale_generation,
                                                   .handle = handle,
                                                   .resource_kind = record->kind,
                                                   .owner = record->owner,
                                                   .debug_name = record->debug_name,
                                                   .frame = 0});
        return result;
    }

    record->debug_name = std::move(debug_name);
    events_.push_back(RhiResourceLifetimeEvent{
        .kind = RhiResourceLifetimeEventKind::rename,
        .handle = record->handle,
        .resource_kind = record->kind,
        .owner = record->owner,
        .debug_name = record->debug_name,
        .frame = 0,
    });
    return result;
}

RhiResourceLifetimeResult RhiResourceLifetimeRegistry::release_resource_deferred(RhiResourceHandle handle,
                                                                                 std::uint64_t release_frame) {
    RhiResourceLifetimeResult result;
    const auto record = std::ranges::find_if(
        records_, [handle](const RhiResourceLifetimeRecord& candidate) { return candidate.handle.id == handle.id; });
    if (record == records_.end()) {
        result.diagnostics.push_back(make_diagnostic(RhiResourceLifetimeDiagnosticCode::invalid_resource, handle,
                                                     "RHI resource handle does not refer to a registered resource."));
        events_.push_back(RhiResourceLifetimeEvent{.kind = RhiResourceLifetimeEventKind::invalid_resource,
                                                   .handle = handle,
                                                   .resource_kind = RhiResourceKind::unknown,
                                                   .owner = {},
                                                   .debug_name = {},
                                                   .frame = release_frame});
        return result;
    }
    if (record->handle.generation != handle.generation) {
        result.diagnostics.push_back(
            make_diagnostic(RhiResourceLifetimeDiagnosticCode::stale_generation, handle,
                            "RHI resource handle generation does not match the registered resource."));
        events_.push_back(RhiResourceLifetimeEvent{.kind = RhiResourceLifetimeEventKind::stale_generation,
                                                   .handle = handle,
                                                   .resource_kind = record->kind,
                                                   .owner = record->owner,
                                                   .debug_name = record->debug_name,
                                                   .frame = release_frame});
        return result;
    }
    if (record->state == RhiResourceLifetimeState::deferred_release) {
        result.diagnostics.push_back(make_diagnostic(RhiResourceLifetimeDiagnosticCode::duplicate_release, handle,
                                                     "RHI resource is already deferred for release."));
        events_.push_back(RhiResourceLifetimeEvent{.kind = RhiResourceLifetimeEventKind::duplicate_release,
                                                   .handle = record->handle,
                                                   .resource_kind = record->kind,
                                                   .owner = record->owner,
                                                   .debug_name = record->debug_name,
                                                   .frame = release_frame});
        return result;
    }

    record->state = RhiResourceLifetimeState::deferred_release;
    record->release_frame = release_frame;
    events_.push_back(RhiResourceLifetimeEvent{
        .kind = RhiResourceLifetimeEventKind::defer_release,
        .handle = record->handle,
        .resource_kind = record->kind,
        .owner = record->owner,
        .debug_name = record->debug_name,
        .frame = release_frame,
    });
    return result;
}

std::uint32_t RhiResourceLifetimeRegistry::retire_released_resources(std::uint64_t completed_frame) {
    std::uint32_t retired_count = 0;
    auto record = records_.begin();
    while (record != records_.end()) {
        if (record->state == RhiResourceLifetimeState::deferred_release && record->release_frame <= completed_frame) {
            events_.push_back(RhiResourceLifetimeEvent{
                .kind = RhiResourceLifetimeEventKind::retire,
                .handle = record->handle,
                .resource_kind = record->kind,
                .owner = record->owner,
                .debug_name = record->debug_name,
                .frame = completed_frame,
            });
            record = records_.erase(record);
            ++retired_count;
            continue;
        }
        ++record;
    }
    return retired_count;
}

bool RhiResourceLifetimeRegistry::is_live(RhiResourceHandle handle) const noexcept {
    const auto record = std::ranges::find_if(
        records_, [handle](const RhiResourceLifetimeRecord& candidate) { return candidate.handle == handle; });
    return record != records_.end() && record->state == RhiResourceLifetimeState::live;
}

const std::vector<RhiResourceLifetimeRecord>& RhiResourceLifetimeRegistry::records() const noexcept {
    return records_;
}

const std::vector<RhiResourceLifetimeEvent>& RhiResourceLifetimeRegistry::events() const noexcept {
    return events_;
}

} // namespace mirakana::rhi
