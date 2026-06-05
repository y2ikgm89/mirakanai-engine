// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "native_editor_tsf_text_input.hpp"

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

#include <algorithm>
#include <atomic>
#include <cwchar>
#include <string>
#include <utility>

#endif

namespace mirakana::editor {

#if defined(_WIN32)
namespace {

using Microsoft::WRL::ComPtr;

[[nodiscard]] std::wstring utf8_to_wide(std::string_view text) {
    if (text.empty()) {
        return {};
    }
    const int required =
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (required <= 0) {
        return {};
    }
    std::wstring result(static_cast<std::size_t>(required), L'\0');
    const int written = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()),
                                            result.data(), required);
    if (written <= 0) {
        return {};
    }
    result.resize(static_cast<std::size_t>(written));
    return result;
}

[[nodiscard]] LONG utf8_byte_offset_to_wide_offset(std::string_view text, std::size_t byte_offset) {
    const auto clamped = std::min(byte_offset, text.size());
    return static_cast<LONG>(utf8_to_wide(text.substr(0U, clamped)).size());
}

[[nodiscard]] RECT rect_from_ui_rect(ui::Rect rect) noexcept {
    const LONG left = static_cast<LONG>(rect.x);
    const LONG top = static_cast<LONG>(rect.y);
    const LONG right = static_cast<LONG>(rect.x + rect.width);
    const LONG bottom = static_cast<LONG>(rect.y + rect.height);
    return RECT{.left = left, .top = top, .right = right, .bottom = bottom};
}

class TsfAcpTextStore final : public ITextStoreACP {
  public:
    explicit TsfAcpTextStore(NativeEditorTsfTextStoreEvidence* evidence) noexcept : evidence_(evidence) {}

    void reset_from_request(const ui::PlatformTextInputRequest& request) {
        text_ = utf8_to_wide(request.surrounding_text);
        selection_start_ = utf8_byte_offset_to_wide_offset(request.surrounding_text, request.cursor_byte_offset);
        selection_end_ = utf8_byte_offset_to_wide_offset(request.surrounding_text,
                                                         request.cursor_byte_offset + request.selection_byte_length);
        caret_rect_ = rect_from_ui_rect(request.text_bounds);
        if (evidence_ != nullptr) {
            evidence_->status = "not_ready";
            evidence_->text_store_ready = true;
            evidence_->sink_advisory_rows = 1U;
            evidence_->candidate_ui_host_owned = true;
            evidence_->reconversion_diagnostic_rows = request.surrounding_text.empty() ? 0U : 1U;
            evidence_->native_handles_exposed = false;
            evidence_->diagnostics.clear();
        }
    }

    void finalize_evidence() noexcept {
        if (evidence_ == nullptr) {
            return;
        }
        const bool ready = evidence_->text_store_ready && evidence_->sink_advisory_rows > 0U &&
                           evidence_->document_lock_rows > 0U && evidence_->request_lock_rows > 0U &&
                           evidence_->selection_read_rows > 0U && evidence_->selection_write_rows > 0U &&
                           evidence_->text_read_rows > 0U && evidence_->text_replace_rows > 0U &&
                           evidence_->insert_text_at_selection_rows > 0U && evidence_->text_ext_rows > 0U &&
                           evidence_->screen_ext_rows > 0U && evidence_->candidate_ui_host_owned &&
                           evidence_->reconversion_diagnostic_rows > 0U && !evidence_->native_handles_exposed &&
                           evidence_->diagnostics.empty();
        evidence_->status = ready ? "ready" : "not_ready";
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override {
        if (object == nullptr) {
            return E_POINTER;
        }
        if (riid == IID_IUnknown || riid == IID_ITextStoreACP) {
            *object = static_cast<ITextStoreACP*>(this);
            AddRef();
            return S_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        return static_cast<ULONG>(++ref_count_);
    }

    ULONG STDMETHODCALLTYPE Release() override {
        const auto count = static_cast<ULONG>(--ref_count_);
        if (count == 0U) {
            delete this;
        }
        return count;
    }

    HRESULT STDMETHODCALLTYPE AdviseSink(REFIID riid, IUnknown* punk, DWORD /*dwMask*/) override {
        if (punk == nullptr || riid != IID_ITextStoreACPSink) {
            return E_INVALIDARG;
        }
        if (evidence_ != nullptr) {
            evidence_->sink_advisory_rows = 1U;
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE UnadviseSink(IUnknown* /*punk*/) override {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE RequestLock(DWORD /*dwLockFlags*/, HRESULT* session_result) override {
        if (session_result == nullptr) {
            return E_POINTER;
        }
        *session_result = S_OK;
        if (evidence_ != nullptr) {
            evidence_->document_lock_rows = 1U;
            evidence_->request_lock_rows = 1U;
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetStatus(TS_STATUS* status) override {
        if (status == nullptr) {
            return E_POINTER;
        }
        status->dwDynamicFlags = 0U;
        status->dwStaticFlags = 0U;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE QueryInsert(LONG start, LONG end, ULONG count, LONG* result_start,
                                          LONG* result_end) override {
        if (result_start == nullptr || result_end == nullptr) {
            return E_POINTER;
        }
        const LONG clamped_start = clamp_acp(start);
        const LONG clamped_end = std::max(clamped_start, clamp_acp(end));
        *result_start = clamped_start;
        *result_end = clamped_end + static_cast<LONG>(count);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetSelection(ULONG /*index*/, ULONG count, TS_SELECTION_ACP* selection,
                                           ULONG* fetched) override {
        if (selection == nullptr || fetched == nullptr) {
            return E_POINTER;
        }
        if (count == 0U) {
            *fetched = 0U;
            return S_OK;
        }
        selection[0] = TS_SELECTION_ACP{
            .acpStart = selection_start_,
            .acpEnd = selection_end_,
            .style = TS_SELECTIONSTYLE{.ase = TS_AE_END, .fInterimChar = FALSE},
        };
        *fetched = 1U;
        if (evidence_ != nullptr) {
            evidence_->selection_read_rows = 1U;
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE SetSelection(ULONG count, const TS_SELECTION_ACP* selection) override {
        if (count == 0U || selection == nullptr) {
            return E_INVALIDARG;
        }
        selection_start_ = clamp_acp(selection[0].acpStart);
        selection_end_ = clamp_acp(selection[0].acpEnd);
        if (selection_end_ < selection_start_) {
            std::swap(selection_start_, selection_end_);
        }
        if (evidence_ != nullptr) {
            evidence_->selection_write_rows = 1U;
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetText(LONG start, LONG end, WCHAR* plain_text, ULONG plain_requested,
                                      ULONG* plain_returned, TS_RUNINFO* run_info, ULONG run_info_requested,
                                      ULONG* run_info_returned, LONG* next) override {
        if (plain_returned == nullptr || run_info_returned == nullptr || next == nullptr) {
            return E_POINTER;
        }
        const LONG clamped_start = clamp_acp(start);
        const LONG clamped_end = end < 0 ? end_acp() : std::max(clamped_start, clamp_acp(end));
        const auto begin = static_cast<std::size_t>(clamped_start);
        const auto count = static_cast<std::size_t>(clamped_end - clamped_start);
        const auto available = count <= text_.size() - begin ? count : text_.size() - begin;
        const auto to_copy = std::min<std::size_t>(plain_requested, available);
        if (plain_text != nullptr && to_copy > 0U) {
            std::wmemcpy(plain_text, text_.data() + begin, to_copy);
        }
        *plain_returned = static_cast<ULONG>(to_copy);
        if (run_info != nullptr && run_info_requested > 0U) {
            run_info[0] = TS_RUNINFO{.uCount = static_cast<ULONG>(to_copy), .type = TS_RT_PLAIN};
            *run_info_returned = 1U;
        } else {
            *run_info_returned = 0U;
        }
        *next = static_cast<LONG>(begin + to_copy);
        if (evidence_ != nullptr) {
            evidence_->text_read_rows = 1U;
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE SetText(DWORD /*flags*/, LONG start, LONG end, const WCHAR* text, ULONG count,
                                      TS_TEXTCHANGE* change) override {
        const auto clamped_start = clamp_acp(start);
        const auto clamped_end = std::max(clamped_start, clamp_acp(end));
        const std::wstring replacement = text == nullptr || count == 0U ? std::wstring{} : std::wstring{text, count};
        text_.replace(static_cast<std::size_t>(clamped_start), static_cast<std::size_t>(clamped_end - clamped_start),
                      replacement);
        selection_start_ = clamped_start + static_cast<LONG>(replacement.size());
        selection_end_ = selection_start_;
        fill_change(change, clamped_start, clamped_end, selection_end_);
        if (evidence_ != nullptr) {
            evidence_->text_replace_rows = 1U;
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetFormattedText(LONG /*start*/, LONG /*end*/, IDataObject** data_object) override {
        if (data_object != nullptr) {
            *data_object = nullptr;
        }
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetEmbedded(LONG /*position*/, REFGUID /*service*/, REFIID /*riid*/,
                                          IUnknown** object) override {
        if (object != nullptr) {
            *object = nullptr;
        }
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

    HRESULT STDMETHODCALLTYPE InsertEmbedded(DWORD /*flags*/, LONG /*start*/, LONG /*end*/, IDataObject* /*data*/,
                                             TS_TEXTCHANGE* change) override {
        fill_change(change, selection_start_, selection_end_, selection_end_);
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE InsertTextAtSelection(DWORD /*flags*/, const WCHAR* text, ULONG count, LONG* start,
                                                    LONG* end, TS_TEXTCHANGE* change) override {
        if (start == nullptr || end == nullptr) {
            return E_POINTER;
        }
        const LONG insert_start = selection_start_;
        const LONG insert_end = selection_end_;
        const std::wstring replacement = text == nullptr || count == 0U ? std::wstring{} : std::wstring{text, count};
        text_.replace(static_cast<std::size_t>(insert_start), static_cast<std::size_t>(insert_end - insert_start),
                      replacement);
        selection_start_ = insert_start + static_cast<LONG>(replacement.size());
        selection_end_ = selection_start_;
        *start = insert_start;
        *end = selection_start_;
        fill_change(change, insert_start, insert_end, selection_start_);
        if (evidence_ != nullptr) {
            evidence_->insert_text_at_selection_rows = 1U;
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE InsertEmbeddedAtSelection(DWORD /*flags*/, IDataObject* /*data*/, LONG* start, LONG* end,
                                                        TS_TEXTCHANGE* change) override {
        if (start != nullptr) {
            *start = selection_start_;
        }
        if (end != nullptr) {
            *end = selection_end_;
        }
        fill_change(change, selection_start_, selection_end_, selection_end_);
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE RequestSupportedAttrs(DWORD /*flags*/, ULONG /*count*/,
                                                    const TS_ATTRID* /*attrs*/) override {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE RequestAttrsAtPosition(LONG /*position*/, ULONG /*count*/, const TS_ATTRID* /*attrs*/,
                                                     DWORD /*flags*/) override {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE RequestAttrsTransitioningAtPosition(LONG /*position*/, ULONG /*count*/,
                                                                  const TS_ATTRID* /*attrs*/,
                                                                  DWORD /*flags*/) override {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE FindNextAttrTransition(LONG start, LONG /*halt*/, ULONG /*count*/,
                                                     const TS_ATTRID* /*attrs*/, DWORD /*flags*/, LONG* next,
                                                     BOOL* found, LONG* found_offset) override {
        if (next == nullptr || found == nullptr || found_offset == nullptr) {
            return E_POINTER;
        }
        *next = clamp_acp(start);
        *found = FALSE;
        *found_offset = 0;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE RetrieveRequestedAttrs(ULONG /*count*/, TS_ATTRVAL* /*attrs*/, ULONG* fetched) override {
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
        *acp = end_acp();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetActiveView(TsViewCookie* view) override {
        if (view == nullptr) {
            return E_POINTER;
        }
        *view = TS_VCOOKIE_NUL;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetACPFromPoint(TsViewCookie /*view*/, const POINT* /*point*/, DWORD /*flags*/,
                                              LONG* acp) override {
        if (acp == nullptr) {
            return E_POINTER;
        }
        *acp = selection_start_;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetTextExt(TsViewCookie /*view*/, LONG /*start*/, LONG /*end*/, RECT* rect,
                                         BOOL* clipped) override {
        if (rect == nullptr || clipped == nullptr) {
            return E_POINTER;
        }
        *rect = caret_rect_;
        *clipped = FALSE;
        if (evidence_ != nullptr) {
            evidence_->text_ext_rows = 1U;
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetScreenExt(TsViewCookie /*view*/, RECT* rect) override {
        if (rect == nullptr) {
            return E_POINTER;
        }
        *rect = caret_rect_;
        if (evidence_ != nullptr) {
            evidence_->screen_ext_rows = 1U;
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetWnd(TsViewCookie /*view*/, HWND* hwnd) override {
        if (hwnd == nullptr) {
            return E_POINTER;
        }
        *hwnd = nullptr;
        return S_OK;
    }

  private:
    [[nodiscard]] LONG end_acp() const noexcept {
        return static_cast<LONG>(text_.size());
    }

    [[nodiscard]] LONG clamp_acp(LONG value) const noexcept {
        return std::clamp<LONG>(value, 0, end_acp());
    }

    static void fill_change(TS_TEXTCHANGE* change, LONG start, LONG old_end, LONG new_end) noexcept {
        if (change == nullptr) {
            return;
        }
        change->acpStart = start;
        change->acpOldEnd = old_end;
        change->acpNewEnd = new_end;
    }

    std::atomic_ulong ref_count_{1U};
    NativeEditorTsfTextStoreEvidence* evidence_{nullptr};
    std::wstring text_;
    LONG selection_start_{0};
    LONG selection_end_{0};
    RECT caret_rect_{};
};

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

} // namespace

struct NativeEditorTsfTextServicesAdapter::Impl {
    Impl() = default;
    ~Impl() {
        end_tsf_document();
        if (thread_manager != nullptr && client_id != TF_CLIENTID_NULL) {
            const HRESULT deactivate_result = thread_manager->Deactivate();
            (void)deactivate_result;
        }
    }

    void begin(const ui::PlatformTextInputRequest& request) noexcept {
        const auto plan = ui::plan_platform_text_input_session(request);
        if (!plan.ready()) {
            return;
        }

        active_request = plan.request;
        reset_text_store_evidence();
        ensure_tsf_session();
    }

    void end(const ui::ElementId& target) noexcept {
        if (active_request.has_value() && active_request->target == target) {
            end_tsf_document();
            active_request.reset();
        }
    }

    void update(const ui::ImeComposition& composition) {
        last_composition = composition;
    }

    void ensure_tsf_session() noexcept {
        if (active_request.has_value() && document_manager != nullptr) {
            return;
        }
        if (thread_manager == nullptr) {
            if (!activate_thread_manager()) {
                return;
            }
        }
        if (!create_document_context()) {
            end_tsf_document();
        }
    }

    [[nodiscard]] bool activate_thread_manager() noexcept {
        ComPtr<ITfThreadMgr> created_thread_manager;
        if (FAILED(CoCreateInstance(CLSID_TF_ThreadMgr, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_PPV_ARGS(created_thread_manager.GetAddressOf())))) {
            return false;
        }

        TfClientId created_client_id = TF_CLIENTID_NULL;
        if (FAILED(created_thread_manager->Activate(&created_client_id)) || created_client_id == TF_CLIENTID_NULL) {
            return false;
        }

        thread_manager = std::move(created_thread_manager);
        client_id = created_client_id;
        return true;
    }

    [[nodiscard]] bool create_document_context() noexcept {
        if (thread_manager == nullptr || client_id == TF_CLIENTID_NULL) {
            return false;
        }

        ComPtr<ITfDocumentMgr> created_document_manager;
        if (FAILED(thread_manager->CreateDocumentMgr(created_document_manager.GetAddressOf()))) {
            return false;
        }

        ComPtr<TsfAcpTextStore> created_text_store;
        created_text_store.Attach(new TsfAcpTextStore(&text_store_evidence));
        created_text_store->reset_from_request(*active_request);

        ComPtr<ITfContext> created_context;
        TfEditCookie created_edit_cookie = TF_INVALID_EDIT_COOKIE;
        if (FAILED(created_document_manager->CreateContext(client_id, 0,
                                                           static_cast<IUnknown*>(created_text_store.Get()),
                                                           created_context.GetAddressOf(), &created_edit_cookie))) {
            return false;
        }
        if (FAILED(created_document_manager->Push(created_context.Get()))) {
            return false;
        }
        if (FAILED(thread_manager->SetFocus(created_document_manager.Get()))) {
            return false;
        }

        document_manager = std::move(created_document_manager);
        context = std::move(created_context);
        text_store = std::move(created_text_store);
        edit_cookie = created_edit_cookie;
        exercise_text_store_contract();
        return true;
    }

    void end_tsf_document() noexcept {
        if (document_manager != nullptr) {
            const HRESULT pop_result = document_manager->Pop(TF_POPF_ALL);
            (void)pop_result;
        }
        context.Reset();
        document_manager.Reset();
        text_store.Reset();
        edit_cookie = TF_INVALID_EDIT_COOKIE;
    }

    void reset_text_store_evidence() noexcept {
        text_store_evidence = NativeEditorTsfTextStoreEvidence{};
    }

    void exercise_text_store_contract() noexcept {
        if (text_store == nullptr || !active_request.has_value()) {
            return;
        }

        HRESULT lock_result = E_FAIL;
        const HRESULT request_lock_result = text_store->RequestLock(TS_LF_READWRITE, &lock_result);
        if (FAILED(request_lock_result) || FAILED(lock_result)) {
            text_store_evidence.diagnostics.push_back("RequestLock failed");
        }

        TS_SELECTION_ACP selection{};
        ULONG selection_count = 0U;
        if (FAILED(text_store->GetSelection(TS_DEFAULT_SELECTION, 1U, &selection, &selection_count)) ||
            selection_count == 0U) {
            text_store_evidence.diagnostics.push_back("GetSelection failed");
        } else if (FAILED(text_store->SetSelection(1U, &selection))) {
            text_store_evidence.diagnostics.push_back("SetSelection failed");
        }

        WCHAR text_buffer[64]{};
        ULONG text_count = 0U;
        TS_RUNINFO run_info{};
        ULONG run_count = 0U;
        LONG next = 0;
        if (FAILED(text_store->GetText(0, -1, text_buffer, 64U, &text_count, &run_info, 1U, &run_count, &next))) {
            text_store_evidence.diagnostics.push_back("GetText failed");
        }

        TS_TEXTCHANGE change{};
        if (FAILED(text_store->SetText(0U, selection.acpStart, selection.acpStart, nullptr, 0U, &change))) {
            text_store_evidence.diagnostics.push_back("SetText failed");
        }

        LONG inserted_start = 0;
        LONG inserted_end = 0;
        if (FAILED(text_store->InsertTextAtSelection(0U, nullptr, 0U, &inserted_start, &inserted_end, &change))) {
            text_store_evidence.diagnostics.push_back("InsertTextAtSelection failed");
        }

        RECT rect{};
        BOOL clipped = FALSE;
        if (FAILED(text_store->GetTextExt(TS_VCOOKIE_NUL, selection.acpStart, selection.acpEnd, &rect, &clipped))) {
            text_store_evidence.diagnostics.push_back("GetTextExt failed");
        }
        if (FAILED(text_store->GetScreenExt(TS_VCOOKIE_NUL, &rect))) {
            text_store_evidence.diagnostics.push_back("GetScreenExt failed");
        }

        text_store->finalize_evidence();
    }

    ComApartment apartment;
    ComPtr<ITfThreadMgr> thread_manager;
    ComPtr<ITfDocumentMgr> document_manager;
    ComPtr<ITfContext> context;
    ComPtr<TsfAcpTextStore> text_store;
    TfClientId client_id{TF_CLIENTID_NULL};
    TfEditCookie edit_cookie{TF_INVALID_EDIT_COOKIE};
    std::optional<ui::PlatformTextInputRequest> active_request;
    ui::ImeComposition last_composition;
    NativeEditorTsfTextStoreEvidence text_store_evidence;
};

#else

struct NativeEditorTsfTextServicesAdapter::Impl {
    std::optional<ui::PlatformTextInputRequest> active_request;
    ui::ImeComposition last_composition;
    NativeEditorTsfTextStoreEvidence text_store_evidence;
};

#endif

NativeEditorTsfTextServicesAdapter::NativeEditorTsfTextServicesAdapter() : impl_(std::make_unique<Impl>()) {}

NativeEditorTsfTextServicesAdapter::~NativeEditorTsfTextServicesAdapter() = default;

NativeEditorTsfTextServicesAdapter::NativeEditorTsfTextServicesAdapter(NativeEditorTsfTextServicesAdapter&&) noexcept =
    default;

NativeEditorTsfTextServicesAdapter&
NativeEditorTsfTextServicesAdapter::operator=(NativeEditorTsfTextServicesAdapter&&) noexcept = default;

void NativeEditorTsfTextServicesAdapter::begin_text_input(const ui::PlatformTextInputRequest& request) {
#if defined(_WIN32)
    impl_->begin(request);
#else
    impl_->active_request = request;
#endif
}

void NativeEditorTsfTextServicesAdapter::end_text_input(const ui::ElementId& target) {
#if defined(_WIN32)
    impl_->end(target);
#else
    if (impl_->active_request.has_value() && impl_->active_request->target == target) {
        impl_->active_request.reset();
    }
#endif
}

void NativeEditorTsfTextServicesAdapter::update_composition(const ui::ImeComposition& composition) {
#if defined(_WIN32)
    impl_->update(composition);
#else
    impl_->last_composition = composition;
#endif
}

const std::optional<ui::PlatformTextInputRequest>& NativeEditorTsfTextServicesAdapter::active_request() const noexcept {
    return impl_->active_request;
}

bool NativeEditorTsfTextServicesAdapter::native_handles_exposed() const noexcept {
    return false;
}

bool NativeEditorTsfTextServicesAdapter::tsf_thread_manager_ready() const noexcept {
#if defined(_WIN32)
    return impl_->thread_manager != nullptr;
#else
    return false;
#endif
}

bool NativeEditorTsfTextServicesAdapter::tsf_document_manager_ready() const noexcept {
#if defined(_WIN32)
    return impl_->document_manager != nullptr;
#else
    return false;
#endif
}

NativeEditorTsfTextStoreEvidence NativeEditorTsfTextServicesAdapter::tsf_text_store_evidence() const {
    return impl_->text_store_evidence;
}

std::unique_ptr<NativeEditorTsfTextServicesAdapter> make_native_editor_tsf_text_services_adapter() {
    return std::make_unique<NativeEditorTsfTextServicesAdapter>();
}

} // namespace mirakana::editor
