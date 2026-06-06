// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "native_editor_uia_provider.hpp"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

// UIAutomation.h includes MIDL headers that require COM interface macros from objbase.h first.
// clang-format off
#include <objbase.h>
#include <UIAutomation.h>
#include <oleauto.h>
// clang-format on
#endif

namespace mirakana::editor {
namespace {

[[nodiscard]] bool empty_id(const ui::ElementId& id) noexcept {
    return id.value.empty();
}

[[nodiscard]] bool positive_rect(ui::Rect rect) noexcept {
    return rect.width > 0.0F && rect.height > 0.0F;
}

[[nodiscard]] ui::Rect to_screen_bounds(ui::Rect bounds, NativeEditorUiaScreenOrigin screen_origin) noexcept {
    bounds.x += screen_origin.x;
    bounds.y += screen_origin.y;
    return bounds;
}

[[nodiscard]] std::uint32_t stable_runtime_hash(std::string_view text) noexcept {
    std::uint32_t hash = 2166136261U;
    for (const auto character : text) {
        hash ^= static_cast<unsigned char>(character);
        hash *= 16777619U;
    }
    hash &= 0x7fffffffU;
    return hash == 0U ? 1U : hash;
}

inline constexpr std::int32_t kUiaAppendRuntimeIdValue = 3;

[[nodiscard]] std::vector<std::int32_t> runtime_id_for(std::string_view id) {
    return {kUiaAppendRuntimeIdValue, static_cast<std::int32_t>(stable_runtime_hash(id))};
}

[[nodiscard]] std::string control_type_id_for(ui::SemanticRole role) {
    switch (role) {
    case ui::SemanticRole::root:
    case ui::SemanticRole::panel:
    case ui::SemanticRole::dialog:
        return "UIA_PaneControlTypeId";
    case ui::SemanticRole::button:
        return "UIA_ButtonControlTypeId";
    case ui::SemanticRole::label:
        return "UIA_TextControlTypeId";
    case ui::SemanticRole::text_field:
        return "UIA_EditControlTypeId";
    case ui::SemanticRole::list:
        return "UIA_ListControlTypeId";
    case ui::SemanticRole::list_item:
        return "UIA_ListItemControlTypeId";
    case ui::SemanticRole::image:
        return "UIA_ImageControlTypeId";
    case ui::SemanticRole::checkbox:
        return "UIA_CheckBoxControlTypeId";
    case ui::SemanticRole::slider:
        return "UIA_SliderControlTypeId";
    case ui::SemanticRole::none:
        break;
    }
    return "UIA_CustomControlTypeId";
}

[[nodiscard]] std::vector<std::string> actions_for(const ui::AccessibilityNode& node) {
    std::vector<std::string> actions;
    if (node.enabled && node.focusable) {
        actions.push_back("focus");
    }
    return actions;
}

void push_pattern(std::vector<std::string>& patterns, std::string pattern) {
    if (std::ranges::find(patterns, pattern) == patterns.end()) {
        patterns.push_back(std::move(pattern));
    }
}

[[nodiscard]] std::vector<std::string> control_patterns_for(const ui::AccessibilityNode& node, bool hosted_root) {
    std::vector<std::string> patterns;
    switch (node.role) {
    case ui::SemanticRole::root:
    case ui::SemanticRole::dialog:
        push_pattern(patterns, "Window");
        push_pattern(patterns, "Scroll");
        break;
    case ui::SemanticRole::panel:
        push_pattern(patterns, "Scroll");
        break;
    case ui::SemanticRole::button:
        push_pattern(patterns, "Invoke");
        break;
    case ui::SemanticRole::label:
        push_pattern(patterns, "Text");
        break;
    case ui::SemanticRole::text_field:
        push_pattern(patterns, "Value");
        push_pattern(patterns, "Text");
        push_pattern(patterns, "TextEdit");
        break;
    case ui::SemanticRole::list:
        push_pattern(patterns, "Selection");
        push_pattern(patterns, "Scroll");
        break;
    case ui::SemanticRole::list_item:
        push_pattern(patterns, "Invoke");
        break;
    case ui::SemanticRole::checkbox:
        push_pattern(patterns, "Invoke");
        push_pattern(patterns, "Toggle");
        break;
    case ui::SemanticRole::slider:
        push_pattern(patterns, "Value");
        break;
    case ui::SemanticRole::image:
        break;
    case ui::SemanticRole::none:
        break;
    }
    if (hosted_root && std::ranges::find(patterns, "Window") == patterns.end()) {
        push_pattern(patterns, "Window");
    }
    return patterns;
}

[[nodiscard]] bool contains_pattern(const NativeEditorUiaNode& node, std::string_view pattern) noexcept {
    return std::ranges::any_of(node.control_patterns, [pattern](const std::string& row) { return row == pattern; });
}

void count_pattern_rows(NativeEditorUiaProviderState& state, const NativeEditorUiaNode& node) noexcept {
    state.uia_pattern_rows += static_cast<std::uint32_t>(node.control_patterns.size());
    state.invoke_pattern_rows += contains_pattern(node, "Invoke") ? 1U : 0U;
    state.value_pattern_rows += contains_pattern(node, "Value") ? 1U : 0U;
    state.selection_pattern_rows += contains_pattern(node, "Selection") ? 1U : 0U;
    state.text_pattern_rows += contains_pattern(node, "Text") ? 1U : 0U;
    state.text_edit_pattern_rows += contains_pattern(node, "TextEdit") ? 1U : 0U;
    state.scroll_pattern_rows += contains_pattern(node, "Scroll") ? 1U : 0U;
    state.window_pattern_rows += contains_pattern(node, "Window") ? 1U : 0U;
    state.toggle_pattern_rows += contains_pattern(node, "Toggle") ? 1U : 0U;
}

void finalize_uia_event_rows(NativeEditorUiaProviderState& state) noexcept {
    state.focus_event_rows = state.focus_rows;
    state.property_change_event_rows =
        state.state_rows > 0U && state.name_property_rows > 0U && state.control_type_rows > 0U ? 1U : 0U;
    state.text_edit_event_rows = state.text_edit_pattern_rows > 0U ? 1U : 0U;
    state.selection_change_event_rows = state.selection_pattern_rows > 0U ? 1U : 0U;
    state.structure_change_event_rows = state.relationship_rows > 0U ? 1U : 0U;
    state.window_event_rows = state.window_pattern_rows > 0U ? 2U : 0U;
    state.live_region_event_rows = state.live_region_rows;
    state.uia_event_rows = state.focus_event_rows + state.property_change_event_rows + state.text_edit_event_rows +
                           state.selection_change_event_rows + state.structure_change_event_rows +
                           state.window_event_rows + state.live_region_event_rows;
    state.windows_uia_events_ready = state.focus_event_rows > 0U && state.property_change_event_rows > 0U &&
                                     state.text_edit_event_rows > 0U && state.selection_change_event_rows > 0U &&
                                     state.structure_change_event_rows > 0U && state.window_event_rows >= 2U;
}

void finalize_uia_pattern_rows(NativeEditorUiaProviderState& state) noexcept {
    state.windows_uia_patterns_ready =
        state.automation_id_rows == state.nodes.size() && state.name_property_rows == state.nodes.size() &&
        state.control_type_rows == state.nodes.size() &&
        (state.nodes.size() <= 1U || state.runtime_id_opaque_rows + 1U == state.nodes.size()) &&
        state.invoke_pattern_rows > 0U && state.value_pattern_rows > 0U && state.selection_pattern_rows > 0U &&
        state.text_pattern_rows > 0U && state.text_edit_pattern_rows > 0U && state.scroll_pattern_rows > 0U &&
        state.window_pattern_rows > 0U;
}

[[nodiscard]] ui::AdapterPayloadDiagnostic make_uia_diagnostic(ui::ElementId id, ui::AdapterPayloadDiagnosticCode code,
                                                               std::string message) {
    return ui::AdapterPayloadDiagnostic{.id = std::move(id), .code = code, .message = std::move(message)};
}

[[nodiscard]] std::uint32_t navigation_row_count(const NativeEditorUiaNode& node) noexcept {
    std::uint32_t rows = 0;
    rows += empty_id(node.parent) ? 0U : 1U;
    rows += empty_id(node.previous_sibling) ? 0U : 1U;
    rows += empty_id(node.next_sibling) ? 0U : 1U;
    rows += empty_id(node.first_child) ? 0U : 1U;
    rows += empty_id(node.last_child) ? 0U : 1U;
    return rows;
}

struct NativeEditorUiaTree {
    NativeEditorUiaProviderState state;
};

[[nodiscard]] const NativeEditorUiaNode* find_node(const NativeEditorUiaTree& tree, const ui::ElementId& id) noexcept {
    if (empty_id(id)) {
        return nullptr;
    }
    const auto found = std::ranges::find_if(tree.state.nodes, [&id](const auto& node) { return node.id == id; });
    return found == tree.state.nodes.end() ? nullptr : &*found;
}

[[nodiscard]] std::size_t find_node_index(const NativeEditorUiaTree& tree, const ui::ElementId& id) noexcept {
    for (std::size_t index = 0; index < tree.state.nodes.size(); ++index) {
        if (tree.state.nodes[index].id == id) {
            return index;
        }
    }
    return tree.state.nodes.size();
}

[[nodiscard]] bool point_inside(ui::Rect rect, double x, double y) noexcept {
    return x >= static_cast<double>(rect.x) && y >= static_cast<double>(rect.y) &&
           x <= static_cast<double>(rect.x + rect.width) && y <= static_cast<double>(rect.y + rect.height);
}

#if defined(_WIN32)
[[nodiscard]] std::wstring utf8_to_wide(std::string_view text) {
    if (text.empty()) {
        return {};
    }
    const auto required =
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (required <= 0) {
        std::wstring fallback;
        fallback.reserve(text.size());
        for (const auto character : text) {
            fallback.push_back(static_cast<unsigned char>(character));
        }
        return fallback;
    }
    std::wstring wide(static_cast<std::size_t>(required), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), wide.data(),
                        required);
    return wide;
}

void set_variant_bool(VARIANT* value, bool enabled) noexcept {
    value->vt = VT_BOOL;
    value->boolVal = enabled ? VARIANT_TRUE : VARIANT_FALSE;
}

void set_variant_i4(VARIANT* value, long number) noexcept {
    value->vt = VT_I4;
    value->lVal = number;
}

void set_variant_bstr(VARIANT* value, std::string_view text) {
    const auto wide = utf8_to_wide(text);
    value->vt = VT_BSTR;
    value->bstrVal = SysAllocStringLen(wide.data(), static_cast<UINT>(wide.size()));
}

[[nodiscard]] long control_type_value_for(std::string_view control_type_id) noexcept {
    if (control_type_id == "UIA_PaneControlTypeId") {
        return UIA_PaneControlTypeId;
    }
    if (control_type_id == "UIA_ButtonControlTypeId") {
        return UIA_ButtonControlTypeId;
    }
    if (control_type_id == "UIA_TextControlTypeId") {
        return UIA_TextControlTypeId;
    }
    if (control_type_id == "UIA_EditControlTypeId") {
        return UIA_EditControlTypeId;
    }
    if (control_type_id == "UIA_ListControlTypeId") {
        return UIA_ListControlTypeId;
    }
    if (control_type_id == "UIA_ListItemControlTypeId") {
        return UIA_ListItemControlTypeId;
    }
    if (control_type_id == "UIA_ImageControlTypeId") {
        return UIA_ImageControlTypeId;
    }
    if (control_type_id == "UIA_CheckBoxControlTypeId") {
        return UIA_CheckBoxControlTypeId;
    }
    if (control_type_id == "UIA_SliderControlTypeId") {
        return UIA_SliderControlTypeId;
    }
    return UIA_CustomControlTypeId;
}

class NativeEditorUiaElementProvider final : public IRawElementProviderSimple,
                                             public IRawElementProviderFragment,
                                             public IRawElementProviderFragmentRoot {
  public:
    NativeEditorUiaElementProvider(std::shared_ptr<NativeEditorUiaTree> tree, std::size_t node_index, HWND hwnd)
        : tree_(std::move(tree)), node_index_(node_index), hwnd_(hwnd) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override {
        if (object == nullptr) {
            return E_INVALIDARG;
        }
        *object = nullptr;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IRawElementProviderSimple)) {
            *object = static_cast<IRawElementProviderSimple*>(this);
        } else if (riid == __uuidof(IRawElementProviderFragment)) {
            *object = static_cast<IRawElementProviderFragment*>(this);
        } else if (riid == __uuidof(IRawElementProviderFragmentRoot) && node_index_ == 0U) {
            *object = static_cast<IRawElementProviderFragmentRoot*>(this);
        } else {
            return E_NOINTERFACE;
        }
        AddRef();
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        return ++ref_count_;
    }

    ULONG STDMETHODCALLTYPE Release() override {
        const auto remaining = --ref_count_;
        if (remaining == 0U) {
            delete this;
        }
        return remaining;
    }

    HRESULT STDMETHODCALLTYPE get_ProviderOptions(ProviderOptions* result) override {
        if (result == nullptr) {
            return E_INVALIDARG;
        }
        *result = ProviderOptions_ServerSideProvider;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetPatternProvider(PATTERNID, IUnknown** result) override {
        if (result == nullptr) {
            return E_INVALIDARG;
        }
        *result = nullptr;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetPropertyValue(PROPERTYID property_id, VARIANT* result) override {
        if (result == nullptr) {
            return E_INVALIDARG;
        }
        VariantInit(result);
        const auto* node = current_node();
        if (node == nullptr) {
            return UIA_E_ELEMENTNOTAVAILABLE;
        }

        if (property_id == UIA_NamePropertyId) {
            set_variant_bstr(result, node->name);
        } else if (property_id == UIA_AutomationIdPropertyId) {
            set_variant_bstr(result, node->id.value);
        } else if (property_id == UIA_ControlTypePropertyId) {
            set_variant_i4(result, control_type_value_for(node->control_type_id));
        } else if (property_id == UIA_IsControlElementPropertyId) {
            set_variant_bool(result, node->control_element);
        } else if (property_id == UIA_IsContentElementPropertyId) {
            set_variant_bool(result, node->content_element);
        } else if (property_id == UIA_IsEnabledPropertyId) {
            set_variant_bool(result, node->enabled);
        } else if (property_id == UIA_IsKeyboardFocusablePropertyId) {
            set_variant_bool(result, node->keyboard_focusable);
        } else if (property_id == UIA_HasKeyboardFocusPropertyId) {
            set_variant_bool(result, node->has_keyboard_focus);
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(IRawElementProviderSimple** result) override {
        if (result == nullptr) {
            return E_INVALIDARG;
        }
        *result = nullptr;
        if (node_index_ != 0U || hwnd_ == nullptr) {
            return S_OK;
        }
        return UiaHostProviderFromHwnd(hwnd_, result);
    }

    HRESULT STDMETHODCALLTYPE Navigate(NavigateDirection direction, IRawElementProviderFragment** result) override {
        if (result == nullptr) {
            return E_INVALIDARG;
        }
        *result = nullptr;
        const auto* node = current_node();
        if (node == nullptr) {
            return UIA_E_ELEMENTNOTAVAILABLE;
        }

        ui::ElementId target;
        switch (direction) {
        case NavigateDirection_Parent:
            target = node_index_ == 0U ? ui::ElementId{} : node->parent;
            break;
        case NavigateDirection_NextSibling:
            target = node_index_ == 0U ? ui::ElementId{} : node->next_sibling;
            break;
        case NavigateDirection_PreviousSibling:
            target = node_index_ == 0U ? ui::ElementId{} : node->previous_sibling;
            break;
        case NavigateDirection_FirstChild:
            target = node->first_child;
            break;
        case NavigateDirection_LastChild:
            target = node->last_child;
            break;
        }
        return provider_for(target, result);
    }

    HRESULT STDMETHODCALLTYPE GetRuntimeId(SAFEARRAY** result) override {
        if (result == nullptr) {
            return E_INVALIDARG;
        }
        *result = nullptr;
        const auto* node = current_node();
        if (node == nullptr) {
            return UIA_E_ELEMENTNOTAVAILABLE;
        }
        if (node->runtime_id.empty()) {
            return S_OK;
        }
        auto* array = SafeArrayCreateVector(VT_I4, 0, static_cast<ULONG>(node->runtime_id.size()));
        if (array == nullptr) {
            return E_OUTOFMEMORY;
        }
        for (LONG index = 0; index < static_cast<LONG>(node->runtime_id.size()); ++index) {
            auto value = node->runtime_id[static_cast<std::size_t>(index)];
            if (FAILED(SafeArrayPutElement(array, &index, &value))) {
                SafeArrayDestroy(array);
                return E_FAIL;
            }
        }
        *result = array;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE get_BoundingRectangle(UiaRect* result) override {
        if (result == nullptr) {
            return E_INVALIDARG;
        }
        const auto* node = current_node();
        if (node == nullptr) {
            return UIA_E_ELEMENTNOTAVAILABLE;
        }
        *result = UiaRect{
            .left = node->bounds.x, .top = node->bounds.y, .width = node->bounds.width, .height = node->bounds.height};
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetEmbeddedFragmentRoots(SAFEARRAY** result) override {
        if (result == nullptr) {
            return E_INVALIDARG;
        }
        *result = nullptr;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE SetFocus() override {
        if (tree_ == nullptr || node_index_ >= tree_->state.nodes.size()) {
            return UIA_E_ELEMENTNOTAVAILABLE;
        }
        for (auto& node : tree_->state.nodes) {
            node.has_keyboard_focus = false;
        }
        tree_->state.nodes[node_index_].has_keyboard_focus = true;
        if (hwnd_ != nullptr) {
            ::SetFocus(hwnd_);
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE get_FragmentRoot(IRawElementProviderFragmentRoot** result) override {
        if (result == nullptr) {
            return E_INVALIDARG;
        }
        *result = nullptr;
        if (tree_ == nullptr || tree_->state.nodes.empty()) {
            return UIA_E_ELEMENTNOTAVAILABLE;
        }
        auto* provider = new (std::nothrow) NativeEditorUiaElementProvider(tree_, 0U, hwnd_);
        if (provider == nullptr) {
            return E_OUTOFMEMORY;
        }
        *result = static_cast<IRawElementProviderFragmentRoot*>(provider);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ElementProviderFromPoint(double x, double y,
                                                       IRawElementProviderFragment** result) override {
        if (result == nullptr) {
            return E_INVALIDARG;
        }
        *result = nullptr;
        if (tree_ == nullptr || tree_->state.nodes.empty()) {
            return UIA_E_ELEMENTNOTAVAILABLE;
        }
        std::size_t selected = 0U;
        for (std::size_t index = 0; index < tree_->state.nodes.size(); ++index) {
            if (point_inside(tree_->state.nodes[index].bounds, x, y)) {
                selected = index;
            }
        }
        auto* provider = new (std::nothrow) NativeEditorUiaElementProvider(tree_, selected, hwnd_);
        if (provider == nullptr) {
            return E_OUTOFMEMORY;
        }
        *result = static_cast<IRawElementProviderFragment*>(provider);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetFocus(IRawElementProviderFragment** result) override {
        if (result == nullptr) {
            return E_INVALIDARG;
        }
        *result = nullptr;
        if (tree_ == nullptr || tree_->state.nodes.empty()) {
            return UIA_E_ELEMENTNOTAVAILABLE;
        }
        std::size_t selected = 0U;
        for (std::size_t index = 0; index < tree_->state.nodes.size(); ++index) {
            if (tree_->state.nodes[index].has_keyboard_focus) {
                selected = index;
                break;
            }
        }
        auto* provider = new (std::nothrow) NativeEditorUiaElementProvider(tree_, selected, hwnd_);
        if (provider == nullptr) {
            return E_OUTOFMEMORY;
        }
        *result = static_cast<IRawElementProviderFragment*>(provider);
        return S_OK;
    }

  private:
    [[nodiscard]] const NativeEditorUiaNode* current_node() const noexcept {
        if (tree_ == nullptr || node_index_ >= tree_->state.nodes.size()) {
            return nullptr;
        }
        return &tree_->state.nodes[node_index_];
    }

    HRESULT provider_for(const ui::ElementId& id, IRawElementProviderFragment** result) const {
        if (empty_id(id) || tree_ == nullptr) {
            return S_OK;
        }
        const auto index = find_node_index(*tree_, id);
        if (index == tree_->state.nodes.size()) {
            return S_OK;
        }
        auto* provider = new (std::nothrow) NativeEditorUiaElementProvider(tree_, index, hwnd_);
        if (provider == nullptr) {
            return E_OUTOFMEMORY;
        }
        *result = static_cast<IRawElementProviderFragment*>(provider);
        return S_OK;
    }

    std::atomic<ULONG> ref_count_{1U};
    std::shared_ptr<NativeEditorUiaTree> tree_;
    std::size_t node_index_{0U};
    HWND hwnd_{nullptr};
};

inline constexpr wchar_t kNativeEditorUiaProviderProperty[] = L"MIRAIKANAI.NativeEditorUiaProvider";

#endif

} // namespace

NativeEditorUiaProviderState plan_native_editor_uia_provider_tree(const ui::AccessibilityPayload& payload,
                                                                  const ui::ElementId& focused,
                                                                  NativeEditorUiaScreenOrigin screen_origin) {
    NativeEditorUiaProviderState state;
    state.nodes.reserve(payload.nodes.size());
    state.diagnostics = payload.diagnostics;
    state.invalid_bounds_diagnostics =
        static_cast<std::uint32_t>(std::ranges::count_if(payload.diagnostics, [](const auto& diagnostic) {
            return diagnostic.code == ui::AdapterPayloadDiagnosticCode::invalid_accessibility_bounds;
        }));

    std::unordered_map<std::string, std::size_t> index_by_id;
    index_by_id.reserve(payload.nodes.size());
    for (const auto& node : payload.nodes) {
        const bool hosted_root = state.nodes.empty();
        NativeEditorUiaNode row{
            .id = node.id,
            .role = node.role,
            .name = node.label,
            .control_type_id = control_type_id_for(node.role),
            .bounds = to_screen_bounds(node.bounds, screen_origin),
            .enabled = node.enabled,
            .keyboard_focusable = node.focusable,
            .has_keyboard_focus = !empty_id(focused) && node.id == focused,
            .control_element = node.role != ui::SemanticRole::none,
            .content_element = node.role != ui::SemanticRole::none,
            .live_region = node.live_region,
            .runtime_id = hosted_root ? std::vector<std::int32_t>{} : runtime_id_for(node.id.value),
            .actions = actions_for(node),
            .control_patterns = control_patterns_for(node, hosted_root),
        };

        if (row.role != ui::SemanticRole::none) {
            ++state.role_rows;
            ++state.control_type_rows;
        } else {
            ++state.missing_role_diagnostics;
            state.diagnostics.push_back(
                make_uia_diagnostic(row.id, ui::AdapterPayloadDiagnosticCode::invalid_accessibility_bounds,
                                    "UI Automation provider row must have a semantic role"));
        }
        if (!row.name.empty()) {
            ++state.name_rows;
            ++state.name_property_rows;
        } else {
            ++state.missing_name_diagnostics;
            state.diagnostics.push_back(
                make_uia_diagnostic(row.id, ui::AdapterPayloadDiagnosticCode::invalid_accessibility_bounds,
                                    "UI Automation provider row must have an accessible name"));
        }
        if (!positive_rect(row.bounds)) {
            state.diagnostics.push_back(
                make_uia_diagnostic(row.id, ui::AdapterPayloadDiagnosticCode::invalid_accessibility_bounds,
                                    "UI Automation provider row must have positive bounds"));
            ++state.invalid_bounds_diagnostics;
        }
        if (row.has_keyboard_focus) {
            ++state.focus_rows;
        }
        if (!row.id.value.empty()) {
            ++state.automation_id_rows;
        }
        if (!row.runtime_id.empty() && row.runtime_id.front() == kUiaAppendRuntimeIdValue) {
            ++state.runtime_id_opaque_rows;
        }
        if (row.live_region) {
            ++state.live_region_rows;
        }
        count_pattern_rows(state, row);
        state.action_rows += static_cast<std::uint32_t>(row.actions.size());
        ++state.state_rows;

        index_by_id[row.id.value] = state.nodes.size();
        state.nodes.push_back(std::move(row));
    }

    std::vector<std::vector<std::size_t>> children(state.nodes.size());
    for (std::size_t index = 0; index < payload.nodes.size(); ++index) {
        const auto& source = payload.nodes[index];
        if (empty_id(source.parent)) {
            continue;
        }
        const auto parent = index_by_id.find(source.parent.value);
        if (parent == index_by_id.end()) {
            continue;
        }
        state.nodes[index].parent = source.parent;
        children[parent->second].push_back(index);
        ++state.relationship_rows;
    }

    for (std::size_t parent_index = 0; parent_index < children.size(); ++parent_index) {
        const auto& child_indices = children[parent_index];
        if (child_indices.empty()) {
            continue;
        }
        state.nodes[parent_index].first_child = state.nodes[child_indices.front()].id;
        state.nodes[parent_index].last_child = state.nodes[child_indices.back()].id;
        for (std::size_t sibling = 0; sibling < child_indices.size(); ++sibling) {
            auto& child = state.nodes[child_indices[sibling]];
            if (sibling > 0U) {
                child.previous_sibling = state.nodes[child_indices[sibling - 1U]].id;
            }
            if (sibling + 1U < child_indices.size()) {
                child.next_sibling = state.nodes[child_indices[sibling + 1U]].id;
            }
        }
    }

    for (const auto& node : state.nodes) {
        state.tree_navigation_rows += navigation_row_count(node);
    }
    finalize_uia_pattern_rows(state);
    finalize_uia_event_rows(state);

    if (state.nodes.empty()) {
        state.status_id = "uia_provider_no_nodes";
    } else if (!state.diagnostics.empty()) {
        state.status_id = "uia_provider_diagnostics";
    } else {
        state.status_id = "uia_provider_ready";
        state.provider_available = true;
    }
    if (state.provider_available && state.windows_uia_patterns_ready && state.windows_uia_events_ready &&
        !state.native_handles_exposed) {
        state.parity_status_id = "ready";
    }
    return state;
}

struct NativeEditorUiaAccessibilityAdapter::Impl {
    explicit Impl(std::uintptr_t owner_window_token) {
#if defined(_WIN32)
        hwnd = reinterpret_cast<HWND>(owner_window_token);
        install_subclass();
#else
        (void)owner_window_token;
#endif
        publish_state();
    }

    ~Impl() {
#if defined(_WIN32)
        uninstall_subclass();
#endif
    }

    void set_focused_element(ui::ElementId next_focused) {
        std::scoped_lock lock{mutex};
        focused = std::move(next_focused);
        publish_state();
    }

    void publish_nodes(std::vector<ui::AccessibilityNode> next_nodes) {
        std::scoped_lock lock{mutex};
        nodes = std::move(next_nodes);
        publish_state();
    }

    void publish_state() {
        ui::AccessibilityPayload payload{.nodes = nodes};
        state = plan_native_editor_uia_provider_tree(payload, focused, screen_origin());
        auto next_tree = std::make_shared<NativeEditorUiaTree>();
        next_tree->state = state;
        tree = std::move(next_tree);
    }

    [[nodiscard]] NativeEditorUiaScreenOrigin screen_origin() const noexcept {
#if defined(_WIN32)
        if (hwnd == nullptr || IsWindow(hwnd) == 0) {
            return {};
        }
        POINT origin{.x = 0, .y = 0};
        if (ClientToScreen(hwnd, &origin) == 0) {
            return {};
        }
        return NativeEditorUiaScreenOrigin{.x = static_cast<float>(origin.x), .y = static_cast<float>(origin.y)};
#else
        return {};
#endif
    }

    std::mutex mutex;
    std::vector<ui::AccessibilityNode> nodes;
    ui::ElementId focused;
    NativeEditorUiaProviderState state;
    std::shared_ptr<NativeEditorUiaTree> tree;

#if defined(_WIN32)
    HWND hwnd{nullptr};
    WNDPROC previous_proc{nullptr};
    bool subclass_installed{false};

    static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
        auto* impl = reinterpret_cast<Impl*>(GetPropW(window, kNativeEditorUiaProviderProperty));
        if (impl != nullptr && message == WM_GETOBJECT) {
            return impl->return_raw_element_provider(window, wparam, lparam);
        }
        if (impl != nullptr && message == WM_DESTROY) {
            UiaReturnRawElementProvider(window, 0, 0, nullptr);
        }
        if (impl != nullptr && impl->previous_proc != nullptr) {
            return CallWindowProcW(impl->previous_proc, window, message, wparam, lparam);
        }
        return DefWindowProcW(window, message, wparam, lparam);
    }

    void install_subclass() {
        if (hwnd == nullptr || IsWindow(hwnd) == 0) {
            return;
        }
        SetPropW(hwnd, kNativeEditorUiaProviderProperty, this);
        previous_proc =
            reinterpret_cast<WNDPROC>(SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&window_proc)));
        subclass_installed = previous_proc != nullptr;
    }

    void uninstall_subclass() noexcept {
        if (hwnd == nullptr || IsWindow(hwnd) == 0) {
            hwnd = nullptr;
            return;
        }
        if (subclass_installed && reinterpret_cast<WNDPROC>(GetWindowLongPtrW(hwnd, GWLP_WNDPROC)) == &window_proc &&
            previous_proc != nullptr) {
            SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(previous_proc));
        }
        RemovePropW(hwnd, kNativeEditorUiaProviderProperty);
        UiaReturnRawElementProvider(hwnd, 0, 0, nullptr);
        hwnd = nullptr;
        previous_proc = nullptr;
        subclass_installed = false;
    }

    LRESULT return_raw_element_provider(HWND window, WPARAM wparam, LPARAM lparam) {
        std::shared_ptr<NativeEditorUiaTree> snapshot;
        {
            std::scoped_lock lock{mutex};
            snapshot = tree;
        }
        if (snapshot == nullptr || !snapshot->state.provider_available || snapshot->state.nodes.empty()) {
            return previous_proc == nullptr ? DefWindowProcW(window, WM_GETOBJECT, wparam, lparam)
                                            : CallWindowProcW(previous_proc, window, WM_GETOBJECT, wparam, lparam);
        }
        auto* provider = new (std::nothrow) NativeEditorUiaElementProvider(std::move(snapshot), 0U, window);
        if (provider == nullptr) {
            return 0;
        }
        const auto result = UiaReturnRawElementProvider(window, wparam, lparam, provider);
        provider->Release();
        return result;
    }
#endif
};

NativeEditorUiaAccessibilityAdapter::NativeEditorUiaAccessibilityAdapter() : impl_(std::make_unique<Impl>(0U)) {}

NativeEditorUiaAccessibilityAdapter::NativeEditorUiaAccessibilityAdapter(std::uintptr_t owner_window_token)
    : impl_(std::make_unique<Impl>(owner_window_token)) {}

NativeEditorUiaAccessibilityAdapter::~NativeEditorUiaAccessibilityAdapter() = default;

NativeEditorUiaAccessibilityAdapter::NativeEditorUiaAccessibilityAdapter(
    NativeEditorUiaAccessibilityAdapter&&) noexcept = default;

NativeEditorUiaAccessibilityAdapter&
NativeEditorUiaAccessibilityAdapter::operator=(NativeEditorUiaAccessibilityAdapter&&) noexcept = default;

void NativeEditorUiaAccessibilityAdapter::set_focused_element(ui::ElementId focused) {
    impl_->set_focused_element(std::move(focused));
}

void NativeEditorUiaAccessibilityAdapter::publish_nodes(const std::vector<ui::AccessibilityNode>& nodes) {
    impl_->publish_nodes(nodes);
}

const NativeEditorUiaProviderState& NativeEditorUiaAccessibilityAdapter::state() const noexcept {
    return impl_->state;
}

bool NativeEditorUiaAccessibilityAdapter::native_handles_exposed() const noexcept {
    return false;
}

std::unique_ptr<NativeEditorUiaAccessibilityAdapter> make_native_editor_uia_accessibility_adapter() {
    return std::make_unique<NativeEditorUiaAccessibilityAdapter>();
}

std::unique_ptr<NativeEditorUiaAccessibilityAdapter>
make_native_editor_win32_uia_accessibility_adapter(std::uintptr_t owner_window_token) {
    return std::make_unique<NativeEditorUiaAccessibilityAdapter>(owner_window_token);
}

bool native_editor_uia_provider_exposes_native_handles() noexcept {
    return false;
}

} // namespace mirakana::editor
