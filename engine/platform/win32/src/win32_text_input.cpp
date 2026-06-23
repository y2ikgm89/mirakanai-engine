// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/win32/win32_text_input.hpp"

#include "win32_utf.hpp"

#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <msctf.h>
#include <objbase.h>
#include <textstor.h>
#include <wrl/client.h>

#endif

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>

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

[[nodiscard]] LONG long_from_size(std::size_t value) noexcept {
    constexpr auto max_long = static_cast<std::size_t>((std::numeric_limits<LONG>::max)());
    return value > max_long ? (std::numeric_limits<LONG>::max)() : static_cast<LONG>(value);
}

[[nodiscard]] LONG text_offset_from_utf8_byte_offset(std::string_view text, std::size_t byte_offset) {
    const auto clamped_offset = std::min(byte_offset, text.size());
    return long_from_size(detail::wide_from_utf8(text.substr(0U, clamped_offset)).size());
}

[[nodiscard]] RECT rect_from_ui_rect(ui::Rect rect) noexcept {
    return RECT{
        .left = static_cast<LONG>(rect.x),
        .top = static_cast<LONG>(rect.y),
        .right = static_cast<LONG>(rect.x + rect.width),
        .bottom = static_cast<LONG>(rect.y + rect.height),
    };
}

class MinimalTsfTextStore final : public ITextStoreACP, public ITfContextOwnerCompositionSink {
  public:
    explicit MinimalTsfTextStore(const ui::PlatformTextInputRequest& request)
        : text_(detail::wide_from_utf8(request.surrounding_text)),
          selection_start_(text_offset_from_utf8_byte_offset(request.surrounding_text, request.cursor_byte_offset)),
          selection_end_(text_offset_from_utf8_byte_offset(request.surrounding_text,
                                                           request.cursor_byte_offset + request.selection_byte_length)),
          text_rect_(rect_from_ui_rect(request.text_bounds)) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override {
        if (object == nullptr) {
            return E_POINTER;
        }
        if (riid == IID_IUnknown || riid == IID_ITextStoreACP) {
            *object = static_cast<ITextStoreACP*>(this);
            AddRef();
            return S_OK;
        }
        if (riid == IID_ITfContextOwnerCompositionSink) {
            *object = static_cast<ITfContextOwnerCompositionSink*>(this);
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
        if (count == 0U) {
            delete this;
        }
        return count;
    }

    HRESULT STDMETHODCALLTYPE AdviseSink(REFIID riid, IUnknown* sink, DWORD mask) override {
        if (riid != IID_ITextStoreACPSink || sink == nullptr) {
            return E_INVALIDARG;
        }
        const HRESULT query_result = sink->QueryInterface(IID_PPV_ARGS(sink_.ReleaseAndGetAddressOf()));
        if (FAILED(query_result)) {
            return query_result;
        }
        sink_mask_ = mask;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE UnadviseSink(IUnknown* /*sink*/) override {
        sink_.Reset();
        sink_mask_ = 0U;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE RequestLock(DWORD lock_flags, HRESULT* session_result) override {
        if (session_result == nullptr) {
            return E_POINTER;
        }
        *session_result = sink_ != nullptr ? sink_->OnLockGranted(lock_flags) : S_OK;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetStatus(TS_STATUS* status) override {
        if (status == nullptr) {
            return E_POINTER;
        }
        status->dwDynamicFlags = 0U;
        status->dwStaticFlags = TS_SS_NOHIDDENTEXT;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE QueryInsert(LONG acp_test_start, LONG /*acp_test_end*/, ULONG count,
                                          LONG* acp_result_start, LONG* acp_result_end) override {
        if (acp_result_start == nullptr || acp_result_end == nullptr) {
            return E_POINTER;
        }
        *acp_result_start = acp_test_start;
        *acp_result_end = acp_test_start + long_from_size(count);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetSelection(ULONG /*index*/, ULONG count, TS_SELECTION_ACP* selection,
                                           ULONG* fetched) override {
        if (fetched == nullptr || (count > 0U && selection == nullptr)) {
            return E_POINTER;
        }
        if (count == 0U) {
            *fetched = 0U;
            return S_OK;
        }
        selection[0].acpStart = selection_start_;
        selection[0].acpEnd = selection_end_;
        selection[0].style = TS_SELECTIONSTYLE{.ase = TS_AE_END, .fInterimChar = FALSE};
        *fetched = 1U;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE SetSelection(ULONG count, const TS_SELECTION_ACP* selection) override {
        if (count > 0U && selection == nullptr) {
            return E_POINTER;
        }
        if (count > 0U) {
            selection_start_ = selection[0].acpStart;
            selection_end_ = selection[0].acpEnd;
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetText(LONG acp_start, LONG acp_end, WCHAR* plain_text, ULONG plain_text_capacity,
                                      ULONG* plain_text_count, TS_RUNINFO* run_info, ULONG run_info_capacity,
                                      ULONG* run_info_count, LONG* acp_next) override {
        if (plain_text_count == nullptr || run_info_count == nullptr || acp_next == nullptr) {
            return E_POINTER;
        }
        const auto text_size = long_from_size(text_.size());
        const auto start = std::clamp(acp_start, 0L, text_size);
        const auto requested_end = acp_end < 0 ? text_size : std::clamp(acp_end, start, text_size);
        const auto available = static_cast<ULONG>(requested_end - start);
        const auto copied = std::min(plain_text_capacity, available);
        if (copied > 0U) {
            if (plain_text == nullptr) {
                return E_POINTER;
            }
            std::copy_n(text_.data() + start, copied, plain_text);
        }
        *plain_text_count = copied;
        if (run_info_capacity > 0U && copied > 0U) {
            if (run_info == nullptr) {
                return E_POINTER;
            }
            run_info[0] = TS_RUNINFO{.uCount = copied, .type = TS_RT_PLAIN};
            *run_info_count = 1U;
        } else {
            *run_info_count = 0U;
        }
        *acp_next = start + long_from_size(copied);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE SetText(DWORD /*flags*/, LONG acp_start, LONG acp_end, const WCHAR* /*text*/,
                                      ULONG /*count*/, TS_TEXTCHANGE* change) override {
        if (change != nullptr) {
            *change = TS_TEXTCHANGE{.acpStart = acp_start, .acpOldEnd = acp_end, .acpNewEnd = acp_end};
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetFormattedText(LONG /*acp_start*/, LONG /*acp_end*/,
                                               IDataObject** data_object) override {
        if (data_object == nullptr) {
            return E_POINTER;
        }
        *data_object = nullptr;
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetEmbedded(LONG /*acp_pos*/, REFGUID /*service*/, REFIID /*riid*/,
                                          IUnknown** unknown) override {
        if (unknown == nullptr) {
            return E_POINTER;
        }
        *unknown = nullptr;
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE QueryInsertEmbedded(const GUID* /*service*/, const FORMATETC* /*format*/,
                                                  BOOL* insertable) override {
        if (insertable == nullptr) {
            return E_POINTER;
        }
        *insertable = FALSE;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE InsertEmbedded(DWORD /*flags*/, LONG acp_start, LONG acp_end,
                                             IDataObject* /*data_object*/, TS_TEXTCHANGE* change) override {
        if (change != nullptr) {
            *change = TS_TEXTCHANGE{.acpStart = acp_start, .acpOldEnd = acp_end, .acpNewEnd = acp_end};
        }
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE InsertTextAtSelection(DWORD /*flags*/, const WCHAR* /*text*/, ULONG /*count*/,
                                                    LONG* acp_start, LONG* acp_end, TS_TEXTCHANGE* change) override {
        if (acp_start != nullptr) {
            *acp_start = selection_start_;
        }
        if (acp_end != nullptr) {
            *acp_end = selection_end_;
        }
        if (change != nullptr) {
            *change =
                TS_TEXTCHANGE{.acpStart = selection_start_, .acpOldEnd = selection_end_, .acpNewEnd = selection_end_};
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE InsertEmbeddedAtSelection(DWORD /*flags*/, IDataObject* /*data_object*/, LONG* acp_start,
                                                        LONG* acp_end, TS_TEXTCHANGE* change) override {
        if (acp_start != nullptr) {
            *acp_start = selection_start_;
        }
        if (acp_end != nullptr) {
            *acp_end = selection_end_;
        }
        if (change != nullptr) {
            *change =
                TS_TEXTCHANGE{.acpStart = selection_start_, .acpOldEnd = selection_end_, .acpNewEnd = selection_end_};
        }
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE RequestSupportedAttrs(DWORD /*flags*/, ULONG /*filter_count*/,
                                                    const TS_ATTRID* /*filter_attrs*/) override {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE RequestAttrsAtPosition(LONG /*acp_pos*/, ULONG /*filter_count*/,
                                                     const TS_ATTRID* /*filter_attrs*/, DWORD /*flags*/) override {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE RequestAttrsTransitioningAtPosition(LONG /*acp_pos*/, ULONG /*filter_count*/,
                                                                  const TS_ATTRID* /*filter_attrs*/,
                                                                  DWORD /*flags*/) override {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE FindNextAttrTransition(LONG /*acp_start*/, LONG acp_halt, ULONG /*filter_count*/,
                                                     const TS_ATTRID* /*filter_attrs*/, DWORD /*flags*/, LONG* acp_next,
                                                     BOOL* found, LONG* found_offset) override {
        if (acp_next == nullptr || found == nullptr || found_offset == nullptr) {
            return E_POINTER;
        }
        *acp_next = acp_halt;
        *found = FALSE;
        *found_offset = 0;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE RetrieveRequestedAttrs(ULONG /*count*/, TS_ATTRVAL* /*attr_values*/,
                                                     ULONG* fetched) override {
        if (fetched == nullptr) {
            return E_POINTER;
        }
        *fetched = 0U;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetEndACP(LONG* acp) override {
        if (acp == nullptr) {
            return E_POINTER;
        }
        *acp = long_from_size(text_.size());
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetActiveView(TsViewCookie* view_cookie) override {
        if (view_cookie == nullptr) {
            return E_POINTER;
        }
        *view_cookie = 1U;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetACPFromPoint(TsViewCookie /*view_cookie*/, const POINT* /*screen_point*/,
                                              DWORD /*flags*/, LONG* acp) override {
        if (acp == nullptr) {
            return E_POINTER;
        }
        *acp = selection_start_;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetTextExt(TsViewCookie /*view_cookie*/, LONG /*acp_start*/, LONG /*acp_end*/, RECT* rect,
                                         BOOL* clipped) override {
        if (rect == nullptr || clipped == nullptr) {
            return E_POINTER;
        }
        *rect = text_rect_;
        *clipped = FALSE;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetScreenExt(TsViewCookie /*view_cookie*/, RECT* rect) override {
        if (rect == nullptr) {
            return E_POINTER;
        }
        *rect = text_rect_;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetWnd(TsViewCookie /*view_cookie*/, HWND* window) override {
        if (window == nullptr) {
            return E_POINTER;
        }
        *window = nullptr;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnStartComposition(ITfCompositionView* /*composition*/, BOOL* ok) override {
        if (ok != nullptr) {
            *ok = TRUE;
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnUpdateComposition(ITfCompositionView* /*composition*/,
                                                  ITfRange* /*range_new*/) override {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnEndComposition(ITfCompositionView* /*composition*/) override {
        return S_OK;
    }

  private:
    std::atomic<ULONG> ref_count_{1U};
    ComPtr<ITextStoreACPSink> sink_;
    DWORD sink_mask_{0U};
    std::wstring text_;
    LONG selection_start_{0};
    LONG selection_end_{0};
    RECT text_rect_{};
};

#endif

void append_tsf_diagnostic(std::vector<Win32TsfTextSessionDiagnostic>& diagnostics,
                           Win32TsfTextSessionDiagnosticCode code, std::string message) {
    diagnostics.push_back(Win32TsfTextSessionDiagnostic{.code = code, .message = std::move(message)});
}

[[nodiscard]] std::string first_diagnostic_message(const std::vector<ui::AdapterPayloadDiagnostic>& diagnostics,
                                                   std::string fallback) {
    if (!diagnostics.empty()) {
        return diagnostics.front().message;
    }
    return fallback;
}

[[nodiscard]] std::optional<ui::TextEditCommandKind> text_edit_command_kind_from_key(Key key) noexcept {
    switch (key) {
    case Key::left:
        return ui::TextEditCommandKind::move_cursor_backward;
    case Key::right:
        return ui::TextEditCommandKind::move_cursor_forward;
    case Key::home:
        return ui::TextEditCommandKind::move_cursor_to_start;
    case Key::end:
        return ui::TextEditCommandKind::move_cursor_to_end;
    case Key::backspace:
        return ui::TextEditCommandKind::delete_backward;
    case Key::delete_key:
        return ui::TextEditCommandKind::delete_forward;
    default:
        return std::nullopt;
    }
}

[[nodiscard]] bool has_text_edit_shortcut_modifier(const Win32ModifierState& modifiers) noexcept {
    return (modifiers.control || modifiers.super) && !modifiers.alt;
}

[[nodiscard]] std::optional<ui::TextEditClipboardCommandKind>
text_edit_clipboard_command_kind_from_virtual_key(std::uint32_t virtual_key) noexcept {
    switch (virtual_key) {
    case win32_vk_c:
        return ui::TextEditClipboardCommandKind::copy_selection;
    case win32_vk_x:
        return ui::TextEditClipboardCommandKind::cut_selection;
    case win32_vk_v:
        return ui::TextEditClipboardCommandKind::paste_text;
    default:
        return std::nullopt;
    }
}

void append_tsf_request_diagnostics(Win32TsfTextSessionResult& result, const ui::PlatformTextInputSessionPlan& plan) {
    for (const auto& diagnostic : plan.diagnostics) {
        switch (diagnostic.code) {
        case ui::AdapterPayloadDiagnosticCode::invalid_platform_text_input_target:
            append_tsf_diagnostic(result.diagnostics, Win32TsfTextSessionDiagnosticCode::missing_active_target,
                                  diagnostic.message);
            break;
        case ui::AdapterPayloadDiagnosticCode::invalid_platform_text_input_bounds:
            append_tsf_diagnostic(result.diagnostics, Win32TsfTextSessionDiagnosticCode::invalid_caret_rect,
                                  diagnostic.message);
            break;
        case ui::AdapterPayloadDiagnosticCode::invalid_text_edit_cursor:
        case ui::AdapterPayloadDiagnosticCode::invalid_text_edit_selection:
            append_tsf_diagnostic(result.diagnostics, Win32TsfTextSessionDiagnosticCode::invalid_surrounding_text,
                                  diagnostic.message);
            break;
        default:
            append_tsf_diagnostic(result.diagnostics, Win32TsfTextSessionDiagnosticCode::invalid_surrounding_text,
                                  diagnostic.message);
            break;
        }
    }
}

[[nodiscard]] std::size_t count_tsf_result_rows(const Win32TsfTextSessionResult& result) noexcept {
    return result.composition_rows.size() + result.committed_text_rows.size() + result.candidate_intent_rows.size() +
           result.text_area_rows.size() + result.focus_sink_rows + result.text_store_lock_rows;
}

void append_tsf_input_rows(Win32TsfTextSessionResult& result, const Win32TsfTextSessionDesc& desc) {
    if (desc.public_native_handles_exposed) {
        append_tsf_diagnostic(result.diagnostics, Win32TsfTextSessionDiagnosticCode::public_native_handles_exposed,
                              "Windows TSF text session evidence must not expose native handles");
    }
    if (desc.claims_cross_platform_ime_ready) {
        result.cross_platform_ime_ready = true;
        append_tsf_diagnostic(result.diagnostics, Win32TsfTextSessionDiagnosticCode::broad_ime_parity_claim,
                              "Windows TSF text session evidence must not claim cross-platform IME readiness");
    }
    if (!desc.request_tsf_session && !desc.candidate_intent_rows.empty()) {
        append_tsf_diagnostic(result.diagnostics, Win32TsfTextSessionDiagnosticCode::candidate_rows_without_session,
                              "Windows TSF candidate intent rows require an active TSF text session request");
    }
    if (!desc.composition_text.empty() && !desc.composition_begin) {
        append_tsf_diagnostic(result.diagnostics, Win32TsfTextSessionDiagnosticCode::composition_update_without_begin,
                              "Windows TSF composition updates require a composition begin row");
    }

    result.public_native_handles_exposed = desc.public_native_handles_exposed;
    result.text_area_rows.push_back(Win32TsfTextAreaRow{
        .target = desc.active_request.target,
        .caret_rect = desc.active_request.text_bounds,
        .text_bounds = desc.active_request.text_bounds,
        .surrounding_text = desc.active_request.surrounding_text,
        .cursor_byte_offset = desc.active_request.cursor_byte_offset,
    });

    if (desc.composition_begin) {
        result.composition_rows.push_back(Win32TsfCompositionRow{
            .target = desc.active_request.target,
            .begin = true,
        });
    }
    if (!desc.composition_text.empty()) {
        result.composition_rows.push_back(Win32TsfCompositionRow{
            .target = desc.active_request.target,
            .composition_text = desc.composition_text,
            .cursor_byte_offset = desc.composition_cursor_byte_offset,
            .update = true,
        });
    }
    if (desc.composition_end) {
        result.composition_rows.push_back(Win32TsfCompositionRow{
            .target = desc.active_request.target,
            .cursor_byte_offset = desc.composition_cursor_byte_offset,
            .end = true,
        });
    }

    for (const auto& committed : desc.committed_text_rows) {
        if (committed.target != desc.active_request.target) {
            append_tsf_diagnostic(result.diagnostics, Win32TsfTextSessionDiagnosticCode::committed_text_target_mismatch,
                                  "Windows TSF committed text target must match the active text session target");
        }
        result.committed_text_rows.push_back(committed);
    }
    for (auto candidate : desc.candidate_intent_rows) {
        candidate.native_candidate_ui_ready = false;
        result.candidate_intent_rows.push_back(std::move(candidate));
    }

    if (desc.row_budget == 0U || count_tsf_result_rows(result) > desc.row_budget) {
        append_tsf_diagnostic(result.diagnostics, Win32TsfTextSessionDiagnosticCode::row_budget_exceeded,
                              "Windows TSF text session evidence rows exceed the request budget");
    }
}

void append_tsf_host_rows(Win32TsfTextSessionResult& result, const ui::PlatformTextInputRequest& request) {
#if defined(_WIN32)
    ComApartment apartment;
    ComPtr<ITfThreadMgr> thread_manager;
    if (FAILED(CoCreateInstance(CLSID_TF_ThreadMgr, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(thread_manager.GetAddressOf()))) ||
        thread_manager == nullptr) {
        append_tsf_diagnostic(result.diagnostics, Win32TsfTextSessionDiagnosticCode::tsf_thread_manager_unavailable,
                              "Windows TSF thread manager was not created");
        return;
    }
    result.tsf_thread_manager_available = true;

    TfClientId client_id = TF_CLIENTID_NULL;
    if (FAILED(thread_manager->Activate(&client_id)) || client_id == TF_CLIENTID_NULL) {
        append_tsf_diagnostic(result.diagnostics, Win32TsfTextSessionDiagnosticCode::tsf_thread_manager_unavailable,
                              "Windows TSF thread manager did not activate a client id");
        return;
    }

    ComPtr<ITfDocumentMgr> document_manager;
    if (FAILED(thread_manager->CreateDocumentMgr(document_manager.GetAddressOf())) || document_manager == nullptr) {
        const HRESULT deactivate_result = thread_manager->Deactivate();
        (void)deactivate_result;
        append_tsf_diagnostic(result.diagnostics, Win32TsfTextSessionDiagnosticCode::tsf_document_manager_unavailable,
                              "Windows TSF document manager was not created");
        return;
    }
    result.tsf_document_manager_available = true;

    ComPtr<MinimalTsfTextStore> text_store;
    text_store.Attach(new MinimalTsfTextStore(request));
    ComPtr<ITfContext> context;
    TfEditCookie edit_cookie = TF_INVALID_EDIT_COOKIE;
    const HRESULT create_context_result = document_manager->CreateContext(
        client_id, 0, static_cast<ITextStoreACP*>(text_store.Get()), context.GetAddressOf(), &edit_cookie);
    if (FAILED(create_context_result) || context == nullptr || edit_cookie == TF_INVALID_EDIT_COOKIE) {
        const HRESULT deactivate_result = thread_manager->Deactivate();
        (void)deactivate_result;
        append_tsf_diagnostic(result.diagnostics, Win32TsfTextSessionDiagnosticCode::tsf_context_unavailable,
                              "Windows TSF edit context and read-only edit cookie were not created: hr=" +
                                  std::to_string(static_cast<unsigned long>(create_context_result)));
        return;
    }
    result.tsf_context_available = true;
    result.text_store_lock_rows = 1U;

    bool pushed = false;
    if (SUCCEEDED(document_manager->Push(context.Get()))) {
        pushed = true;
        result.focus_sink_rows = 1U;
        const HRESULT set_focus_result = thread_manager->SetFocus(document_manager.Get());
        (void)set_focus_result;
    }
    if (pushed) {
        const HRESULT pop_result = document_manager->Pop(TF_POPF_ALL);
        (void)pop_result;
    }
    const HRESULT deactivate_result = thread_manager->Deactivate();
    (void)deactivate_result;

    if (result.focus_sink_rows == 0U) {
        append_tsf_diagnostic(result.diagnostics, Win32TsfTextSessionDiagnosticCode::tsf_context_unavailable,
                              "Windows TSF document context focus row was not established");
    }
#else
    append_tsf_diagnostic(result.diagnostics, Win32TsfTextSessionDiagnosticCode::tsf_thread_manager_unavailable,
                          "Windows TSF text sessions require a Windows host");
#endif
}

} // namespace

Win32TextInputSessionPlan plan_win32_text_input_begin(const ui::PlatformTextInputRequest& request) {
    Win32TextInputSessionPlan plan{.request = request};
    const auto ui_plan = ui::plan_platform_text_input_session(request);
    if (!ui_plan.ready()) {
        plan.diagnostic = first_diagnostic_message(ui_plan.diagnostics, "platform text input request is invalid");
        return plan;
    }

    plan.start_session = true;
    plan.update_composition_window = true;
    return plan;
}

Win32TextInputEndPlan plan_win32_text_input_end(const ui::ElementId& target) {
    Win32TextInputEndPlan plan{.target = target};
    const auto ui_plan = ui::plan_platform_text_input_end(target);
    if (!ui_plan.ready()) {
        plan.diagnostic = first_diagnostic_message(ui_plan.diagnostics, "platform text input target is invalid");
        return plan;
    }

    plan.end_session = true;
    return plan;
}

bool Win32TsfTextSessionResult::succeeded() const noexcept {
    return ready && diagnostics.empty();
}

Win32TsfTextSessionResult plan_win32_tsf_text_session(const Win32TsfTextSessionDesc& desc) {
    Win32TsfTextSessionResult result;
    const auto text_input_plan = ui::plan_platform_text_input_session(desc.active_request);
    append_tsf_request_diagnostics(result, text_input_plan);
    append_tsf_input_rows(result, desc);

    if (result.diagnostics.empty() && desc.request_tsf_session) {
        append_tsf_host_rows(result, desc.active_request);
    }

    result.ready = result.diagnostics.empty() && desc.request_tsf_session && result.tsf_thread_manager_available &&
                   result.tsf_document_manager_available && result.tsf_context_available &&
                   result.focus_sink_rows > 0U && result.text_store_lock_rows > 0U &&
                   !result.public_native_handles_exposed && !result.cross_platform_ime_ready;
    return result;
}

ui::RuntimeUiPlatformProductionEvidenceRow
make_win32_tsf_native_ime_production_evidence(const Win32TsfTextSessionResult& result) {
    ui::RuntimeUiPlatformProductionEvidenceRow row{
        .id = "runtime-ui-platform.native-ime.win32.tsf",
        .feature = ui::RuntimeUiPlatformProductionFeature::native_ime_session,
        .proof = ui::RuntimeUiPlatformProductionProofKind::official_sdk_adapter,
        .selected = true,
        .ready = result.succeeded(),
        .dependency_recorded = true,
        .host_evidence_available = result.tsf_thread_manager_available && result.tsf_document_manager_available &&
                                   result.tsf_context_available && result.focus_sink_rows > 0U &&
                                   result.text_store_lock_rows > 0U,
        .public_native_handles = result.public_native_handles_exposed,
        .external_engine_parity_claim = result.cross_platform_ime_ready,
    };
    if (!row.ready) {
        row.blocker = result.diagnostics.empty() ? "Windows TSF native IME session evidence is not ready"
                                                 : result.diagnostics.front().message;
    }
    return row;
}

std::optional<ui::CommittedTextInput> win32_committed_text_from_message(const ui::ElementId& target,
                                                                        const Win32CopiedTextInputMessage& message) {
    if (message.message != Win32TextInputMessageId::char_input || message.utf16_text.empty()) {
        return std::nullopt;
    }

    try {
        return ui::CommittedTextInput{
            .target = target,
            .text = detail::utf8_from_utf16(message.utf16_text),
        };
    } catch (...) {
        return std::nullopt;
    }
}

ui::TextEditCommitResult apply_win32_committed_text_message(const ui::TextEditState& state, const ui::ElementId& target,
                                                            const Win32CopiedTextInputMessage& message) {
    const auto committed = win32_committed_text_from_message(target, message);
    if (!committed.has_value()) {
        return ui::TextEditCommitResult{.committed = false, .state = state, .diagnostics = {}};
    }

    return ui::apply_committed_text_input(state, *committed);
}

std::optional<ui::TextEditCommand> win32_text_edit_command_from_input_event(const ui::ElementId& target,
                                                                            const Win32InputEvent& event) {
    if (event.kind != Win32InputEventKind::key_pressed || event.repeated) {
        return std::nullopt;
    }

    const auto command_kind = text_edit_command_kind_from_key(event.key);
    if (!command_kind.has_value()) {
        return std::nullopt;
    }

    return ui::TextEditCommand{.target = target, .kind = *command_kind};
}

ui::TextEditCommandResult apply_win32_text_edit_command_event(const ui::TextEditState& state,
                                                              const ui::ElementId& target,
                                                              const Win32InputEvent& event) {
    const auto command = win32_text_edit_command_from_input_event(target, event);
    if (!command.has_value()) {
        return ui::TextEditCommandResult{.applied = false, .state = state, .diagnostics = {}};
    }

    return ui::apply_text_edit_command(state, *command);
}

std::optional<ui::TextEditClipboardCommand>
win32_text_edit_clipboard_command_from_input_event(const ui::ElementId& target, const Win32InputEvent& event) {
    if (event.kind != Win32InputEventKind::key_pressed || event.repeated) {
        return std::nullopt;
    }
    if (!has_text_edit_shortcut_modifier(event.modifiers)) {
        return std::nullopt;
    }

    const auto command_kind = text_edit_clipboard_command_kind_from_virtual_key(event.virtual_key);
    if (!command_kind.has_value()) {
        return std::nullopt;
    }

    return ui::TextEditClipboardCommand{.target = target, .kind = *command_kind};
}

ui::TextEditClipboardCommandResult apply_win32_text_edit_clipboard_command_event(ui::IClipboardTextAdapter& adapter,
                                                                                 const ui::TextEditState& state,
                                                                                 const ui::ElementId& target,
                                                                                 const Win32InputEvent& event) {
    const auto command = win32_text_edit_clipboard_command_from_input_event(target, event);
    if (!command.has_value()) {
        return ui::TextEditClipboardCommandResult{.applied = false, .state = state, .diagnostics = {}};
    }

    return ui::apply_text_edit_clipboard_command(adapter, state, *command);
}

void Win32PlatformIntegrationAdapter::begin_text_input(const ui::PlatformTextInputRequest& request) {
    const auto plan = plan_win32_text_input_begin(request);
    if (!plan.succeeded()) {
        throw std::invalid_argument(plan.diagnostic);
    }

    active_request_ = plan.request;
}

void Win32PlatformIntegrationAdapter::end_text_input(const ui::ElementId& target) {
    const auto plan = plan_win32_text_input_end(target);
    if (!plan.succeeded()) {
        throw std::invalid_argument(plan.diagnostic);
    }

    if (active_request_.has_value() && active_request_->target.value == target.value) {
        active_request_.reset();
    }
}

const std::optional<ui::PlatformTextInputRequest>& Win32PlatformIntegrationAdapter::active_request() const noexcept {
    return active_request_;
}

} // namespace mirakana::win32
