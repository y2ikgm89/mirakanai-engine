// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/win32/win32_ui_accessibility.hpp"

#include "win32_utf.hpp"

#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <objbase.h>
#include <oleauto.h>
#include <uiautomationclient.h>
#include <uiautomationcore.h>
#include <uiautomationcoreapi.h>
#include <wrl/client.h>

#endif

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::win32 {
namespace {

#if defined(_WIN32)

using Microsoft::WRL::ComPtr;

class ComApartment final {
  public:
    ComApartment() noexcept {
        const HRESULT result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        initialized_ = SUCCEEDED(result);
    }

    ~ComApartment() {
        if (initialized_) {
            CoUninitialize();
        }
    }

    ComApartment(const ComApartment&) = delete;
    ComApartment& operator=(const ComApartment&) = delete;

  private:
    bool initialized_{false};
};

[[nodiscard]] CONTROLTYPEID control_type_from_role(ui::SemanticRole role) noexcept {
    switch (role) {
    case ui::SemanticRole::button:
        return UIA_ButtonControlTypeId;
    case ui::SemanticRole::label:
        return UIA_TextControlTypeId;
    case ui::SemanticRole::text_field:
        return UIA_EditControlTypeId;
    case ui::SemanticRole::list:
        return UIA_ListControlTypeId;
    case ui::SemanticRole::list_item:
        return UIA_ListItemControlTypeId;
    case ui::SemanticRole::image:
        return UIA_ImageControlTypeId;
    case ui::SemanticRole::checkbox:
        return UIA_CheckBoxControlTypeId;
    case ui::SemanticRole::slider:
        return UIA_SliderControlTypeId;
    case ui::SemanticRole::meter:
        return UIA_ProgressBarControlTypeId;
    case ui::SemanticRole::dialog:
        return UIA_WindowControlTypeId;
    case ui::SemanticRole::root:
    case ui::SemanticRole::panel:
    case ui::SemanticRole::none:
        return UIA_PaneControlTypeId;
    }
    return UIA_PaneControlTypeId;
}

void set_bstr_variant(VARIANT* value, std::string_view text) {
    const auto wide = detail::wide_from_utf8(text);
    value->vt = VT_BSTR;
    value->bstrVal = SysAllocString(wide.c_str());
}

class MinimalUiaProvider final : public IRawElementProviderSimple, public IInvokeProvider {
  public:
    explicit MinimalUiaProvider(Win32UiaRuntimeNodeRow row, bool delete_on_release = true)
        : row_(std::move(row)), delete_on_release_(delete_on_release) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override {
        if (object == nullptr) {
            return E_POINTER;
        }
        if (riid == IID_IUnknown || riid == __uuidof(IRawElementProviderSimple)) {
            *object = static_cast<IRawElementProviderSimple*>(this);
            AddRef();
            return S_OK;
        }
        if (riid == __uuidof(IInvokeProvider) && row_.invoke_pattern_supported) {
            *object = static_cast<IInvokeProvider*>(this);
            AddRef();
            return S_OK;
        }

        *object = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        return ++ref_count_;
    }

    ULONG STDMETHODCALLTYPE Release() override {
        const auto count = --ref_count_;
        if (count == 0U && delete_on_release_) {
            delete this;
        }
        return count;
    }

    HRESULT STDMETHODCALLTYPE get_ProviderOptions(ProviderOptions* provider_options) override {
        if (provider_options == nullptr) {
            return E_POINTER;
        }
        *provider_options = ProviderOptions_ServerSideProvider;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetPatternProvider(PATTERNID pattern_id, IUnknown** provider) override {
        if (provider == nullptr) {
            return E_POINTER;
        }
        *provider = nullptr;
        if (pattern_id == UIA_InvokePatternId && row_.invoke_pattern_supported) {
            *provider = static_cast<IInvokeProvider*>(this);
            AddRef();
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetPropertyValue(PROPERTYID property_id, VARIANT* value) override {
        if (value == nullptr) {
            return E_POINTER;
        }
        VariantInit(value);
        if (property_id == UIA_AutomationIdPropertyId) {
            set_bstr_variant(value, row_.runtime_id.value);
            return S_OK;
        }
        if (property_id == UIA_NamePropertyId) {
            set_bstr_variant(value, row_.name);
            return S_OK;
        }
        if (property_id == UIA_LocalizedControlTypePropertyId) {
            set_bstr_variant(value, ui::semantic_role_id(row_.role));
            return S_OK;
        }
        if (property_id == UIA_ControlTypePropertyId) {
            value->vt = VT_I4;
            value->lVal = control_type_from_role(row_.role);
            return S_OK;
        }
        if (property_id == UIA_IsEnabledPropertyId) {
            value->vt = VT_BOOL;
            value->boolVal = row_.enabled ? VARIANT_TRUE : VARIANT_FALSE;
            return S_OK;
        }
        if (property_id == UIA_IsKeyboardFocusablePropertyId) {
            value->vt = VT_BOOL;
            value->boolVal = row_.focusable ? VARIANT_TRUE : VARIANT_FALSE;
            return S_OK;
        }
        if (property_id == UIA_HasKeyboardFocusPropertyId) {
            value->vt = VT_BOOL;
            value->boolVal = row_.focused ? VARIANT_TRUE : VARIANT_FALSE;
            return S_OK;
        }

        value->vt = VT_EMPTY;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(IRawElementProviderSimple** provider) override {
        if (provider == nullptr) {
            return E_POINTER;
        }
        *provider = nullptr;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Invoke() override {
        invoked_ = true;
        return S_OK;
    }

  private:
    std::atomic<ULONG> ref_count_{1U};
    Win32UiaRuntimeNodeRow row_;
    bool invoked_{false};
    bool delete_on_release_{true};
};

[[nodiscard]] bool private_provider_smoke_succeeded(const Win32UiaRuntimeNodeRow& row) {
    MinimalUiaProvider provider{row, false};

    ProviderOptions options{};
    if (FAILED(provider.get_ProviderOptions(&options)) ||
        (options & ProviderOptions_ServerSideProvider) != ProviderOptions_ServerSideProvider) {
        return false;
    }

    VARIANT name;
    VariantInit(&name);
    const HRESULT name_result = provider.GetPropertyValue(UIA_NamePropertyId, &name);
    const bool has_name = SUCCEEDED(name_result) && name.vt == VT_BSTR && name.bstrVal != nullptr;
    VariantClear(&name);
    if (!has_name) {
        return false;
    }

    if (row.invoke_pattern_supported && FAILED(provider.Invoke())) {
        return false;
    }

    return true;
}

void append_private_provider_host_evidence(Win32UiaProviderPublicationResult& result,
                                           const Win32UiaProviderPublicationDesc& desc) {
    ComApartment apartment;
    const auto root_it = std::ranges::find_if(
        desc.nodes, [&desc](const Win32UiaRuntimeNodeRow& row) { return row.runtime_id == desc.provider_root_id; });
    if (root_it == desc.nodes.end() || !private_provider_smoke_succeeded(*root_it)) {
        result.diagnostics.push_back(Win32UiaProviderPublicationDiagnostic{
            .code = Win32UiaProviderPublicationDiagnosticCode::uia_provider_unavailable,
            .runtime_id = desc.provider_root_id,
            .message = "Windows UI Automation provider root was not created",
        });
        return;
    }

    result.uia_provider_root_available = true;
    if (!desc.publish_events) {
        return;
    }

    for (auto& row : result.node_rows) {
        if (!row.event_publication_requested) {
            continue;
        }
        row.event_publication_ready = true;
        ++result.event_publication_rows;
        if (UiaClientsAreListening() != FALSE) {
            ComPtr<MinimalUiaProvider> provider;
            provider.Attach(new MinimalUiaProvider(row));
            const HRESULT event_result = UiaRaiseAutomationEvent(provider.Get(), UIA_Invoke_InvokedEventId);
            if (FAILED(event_result)) {
                result.diagnostics.push_back(Win32UiaProviderPublicationDiagnostic{
                    .code = Win32UiaProviderPublicationDiagnosticCode::uia_provider_unavailable,
                    .runtime_id = row.runtime_id,
                    .message = "Windows UI Automation event publication failed",
                });
            }
        }
    }
}

#endif

[[nodiscard]] bool empty_id(const ui::ElementId& id) noexcept {
    return id.value.empty();
}

[[nodiscard]] bool is_positive_rect(ui::Rect rect) noexcept {
    return ui::is_valid_rect(rect) && rect.width > 0.0F && rect.height > 0.0F;
}

void append_diagnostic(std::vector<Win32UiaProviderPublicationDiagnostic>& diagnostics,
                       Win32UiaProviderPublicationDiagnosticCode code, ui::ElementId runtime_id, std::string message) {
    diagnostics.push_back(Win32UiaProviderPublicationDiagnostic{
        .code = code,
        .runtime_id = std::move(runtime_id),
        .message = std::move(message),
    });
}

[[nodiscard]] bool contains_runtime_id(const std::vector<Win32UiaRuntimeNodeRow>& rows,
                                       const ui::ElementId& id) noexcept {
    return std::ranges::any_of(rows, [&id](const Win32UiaRuntimeNodeRow& row) { return row.runtime_id == id; });
}

[[nodiscard]] bool child_relationship_matches(const std::vector<Win32UiaRuntimeNodeRow>& rows,
                                              const ui::ElementId& parent_id, const ui::ElementId& child_id) noexcept {
    return std::ranges::any_of(rows, [&parent_id, &child_id](const Win32UiaRuntimeNodeRow& row) {
        return row.runtime_id == child_id && row.parent_runtime_id == parent_id;
    });
}

[[nodiscard]] bool has_action_pattern(const Win32UiaRuntimeNodeRow& row) noexcept {
    return row.invoke_pattern_supported || !row.action_ids.empty();
}

void append_validation_diagnostics(Win32UiaProviderPublicationResult& result,
                                   const Win32UiaProviderPublicationDesc& desc) {
    if (empty_id(desc.provider_root_id)) {
        append_diagnostic(result.diagnostics, Win32UiaProviderPublicationDiagnosticCode::missing_provider_root_id,
                          desc.provider_root_id, "Windows UI Automation provider root id must not be empty");
    } else if (!contains_runtime_id(desc.nodes, desc.provider_root_id)) {
        append_diagnostic(result.diagnostics, Win32UiaProviderPublicationDiagnosticCode::missing_provider_root_id,
                          desc.provider_root_id, "Windows UI Automation provider root id must match a node row");
    }

    if (desc.public_native_handles_exposed) {
        append_diagnostic(result.diagnostics, Win32UiaProviderPublicationDiagnosticCode::public_native_handles_exposed,
                          desc.provider_root_id,
                          "Windows UI Automation publication evidence must not expose public native handles");
    }
    if (desc.claims_cross_platform_accessibility_ready) {
        append_diagnostic(result.diagnostics,
                          Win32UiaProviderPublicationDiagnosticCode::broad_accessibility_parity_claim,
                          desc.provider_root_id,
                          "Windows UI Automation evidence must not claim cross-platform accessibility readiness");
    }
    if (desc.row_budget == 0U || desc.nodes.size() > desc.row_budget) {
        append_diagnostic(result.diagnostics, Win32UiaProviderPublicationDiagnosticCode::row_budget_exceeded,
                          desc.provider_root_id, "Windows UI Automation publication rows exceed the request budget");
    }

    std::vector<std::string> ids;
    ids.reserve(desc.nodes.size());
    for (const auto& row : desc.nodes) {
        if (empty_id(row.runtime_id)) {
            append_diagnostic(result.diagnostics, Win32UiaProviderPublicationDiagnosticCode::missing_runtime_id,
                              row.runtime_id, "Windows UI Automation node runtime id must not be empty");
        } else if (std::ranges::find(ids, row.runtime_id.value) != ids.end()) {
            append_diagnostic(result.diagnostics, Win32UiaProviderPublicationDiagnosticCode::duplicate_runtime_id,
                              row.runtime_id, "Windows UI Automation node runtime ids must be unique");
        } else {
            ids.push_back(row.runtime_id.value);
        }

        if (row.name.empty()) {
            append_diagnostic(result.diagnostics, Win32UiaProviderPublicationDiagnosticCode::missing_accessible_name,
                              row.runtime_id, "Windows UI Automation node name must not be empty");
        }
        if (!is_positive_rect(row.screen_bounds)) {
            append_diagnostic(result.diagnostics, Win32UiaProviderPublicationDiagnosticCode::invalid_bounds,
                              row.runtime_id, "Windows UI Automation node bounds must be positive screen bounds");
        }
        if (row.focusable && !has_action_pattern(row)) {
            append_diagnostic(
                result.diagnostics, Win32UiaProviderPublicationDiagnosticCode::focusable_without_action_pattern,
                row.runtime_id, "Windows UI Automation focusable nodes require an action or invoke pattern row");
        }
        if (row.unsupported_pattern_claim) {
            append_diagnostic(result.diagnostics, Win32UiaProviderPublicationDiagnosticCode::unsupported_pattern_claim,
                              row.runtime_id,
                              "Windows UI Automation provider rows must not claim unsupported control patterns");
        }
        if (row.event_publication_requested && empty_id(desc.provider_root_id)) {
            append_diagnostic(result.diagnostics,
                              Win32UiaProviderPublicationDiagnosticCode::event_claim_without_provider_root,
                              row.runtime_id, "Windows UI Automation event publication requires a provider root id");
        }
    }

    for (const auto& row : desc.nodes) {
        if (!empty_id(row.parent_runtime_id) && !contains_runtime_id(desc.nodes, row.parent_runtime_id)) {
            append_diagnostic(result.diagnostics, Win32UiaProviderPublicationDiagnosticCode::child_without_parent,
                              row.runtime_id, "Windows UI Automation child node parent id must match a node row");
        }
        if (empty_id(row.parent_runtime_id) && row.runtime_id != desc.provider_root_id) {
            append_diagnostic(result.diagnostics, Win32UiaProviderPublicationDiagnosticCode::child_without_parent,
                              row.runtime_id,
                              "Windows UI Automation non-root nodes must publish a parent relationship");
        }
        for (const auto& child_id : row.child_runtime_ids) {
            if (!child_relationship_matches(desc.nodes, row.runtime_id, child_id)) {
                append_diagnostic(result.diagnostics, Win32UiaProviderPublicationDiagnosticCode::child_without_parent,
                                  child_id,
                                  "Windows UI Automation child runtime id must publish the matching parent id");
            }
        }
    }
}

void append_row_counters(Win32UiaProviderPublicationResult& result) {
    for (const auto& row : result.node_rows) {
        if (row.role != ui::SemanticRole::none) {
            ++result.role_rows;
        }
        if (!row.name.empty()) {
            ++result.name_rows;
        }
        if (!row.description.empty()) {
            ++result.description_rows;
        }
        ++result.state_rows;
        if (row.focusable || row.focused) {
            ++result.focus_rows;
        }
        if (has_action_pattern(row)) {
            ++result.action_rows;
        }
        if (!empty_id(row.parent_runtime_id)) {
            ++result.relationship_rows;
        }
        ++result.reading_order_rows;
        if (!row.live_region_status.empty()) {
            ++result.live_region_rows;
        }
        if (!row.keyboard_shortcut.empty()) {
            ++result.keyboard_pattern_rows;
        }
        if (is_positive_rect(row.screen_bounds)) {
            ++result.bounds_rows;
        }
    }
}

} // namespace

bool Win32UiaProviderPublicationResult::succeeded() const noexcept {
    return ready && diagnostics.empty();
}

Win32UiaProviderPublicationResult publish_runtime_ui_to_win32_uia(const Win32UiaProviderPublicationDesc& desc) {
    Win32UiaProviderPublicationResult result;
    result.provider_root_id = desc.provider_root_id;
    result.node_rows = desc.nodes;
    result.public_native_handles_exposed = desc.public_native_handles_exposed;
    result.cross_platform_accessibility_ready = desc.claims_cross_platform_accessibility_ready;

    append_validation_diagnostics(result, desc);
    append_row_counters(result);

    if (result.diagnostics.empty()) {
#if defined(_WIN32)
        append_private_provider_host_evidence(result, desc);
#else
        append_diagnostic(result.diagnostics, Win32UiaProviderPublicationDiagnosticCode::uia_provider_unavailable,
                          desc.provider_root_id, "Windows UI Automation publication requires a Windows host");
#endif
    }

    result.ready = result.diagnostics.empty() && result.uia_provider_root_available && !result.node_rows.empty() &&
                   !result.public_native_handles_exposed && !result.cross_platform_accessibility_ready;
    return result;
}

ui::RuntimeUiPlatformProductionEvidenceRow
make_win32_uia_accessibility_publication_production_evidence(const Win32UiaProviderPublicationResult& result) {
    ui::RuntimeUiPlatformProductionEvidenceRow row{
        .id = "runtime-ui-platform.accessibility.win32.uia",
        .feature = ui::RuntimeUiPlatformProductionFeature::os_accessibility_publication,
        .proof = ui::RuntimeUiPlatformProductionProofKind::official_sdk_adapter,
        .selected = true,
        .ready = result.succeeded(),
        .dependency_recorded = true,
        .host_evidence_available = result.uia_provider_root_available,
        .public_native_handles = result.public_native_handles_exposed,
        .external_engine_parity_claim = result.cross_platform_accessibility_ready,
    };
    if (!row.ready) {
        row.blocker = result.diagnostics.empty()
                          ? "Windows UI Automation accessibility publication evidence is not ready"
                          : result.diagnostics.front().message;
    }
    return row;
}

} // namespace mirakana::win32
