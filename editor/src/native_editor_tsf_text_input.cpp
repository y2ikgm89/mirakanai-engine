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
#include <wrl/client.h>

#include <utility>

#endif

namespace mirakana::editor {

#if defined(_WIN32)
namespace {

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

        ComPtr<ITfContext> created_context;
        TfEditCookie created_edit_cookie = TF_INVALID_EDIT_COOKIE;
        if (FAILED(created_document_manager->CreateContext(client_id, 0, nullptr, created_context.GetAddressOf(),
                                                           &created_edit_cookie))) {
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
        edit_cookie = created_edit_cookie;
        return true;
    }

    void end_tsf_document() noexcept {
        if (document_manager != nullptr) {
            const HRESULT pop_result = document_manager->Pop(TF_POPF_ALL);
            (void)pop_result;
        }
        context.Reset();
        document_manager.Reset();
        edit_cookie = TF_INVALID_EDIT_COOKIE;
    }

    ComApartment apartment;
    ComPtr<ITfThreadMgr> thread_manager;
    ComPtr<ITfDocumentMgr> document_manager;
    ComPtr<ITfContext> context;
    TfClientId client_id{TF_CLIENTID_NULL};
    TfEditCookie edit_cookie{TF_INVALID_EDIT_COOKIE};
    std::optional<ui::PlatformTextInputRequest> active_request;
    ui::ImeComposition last_composition;
};

#else

struct NativeEditorTsfTextServicesAdapter::Impl {
    std::optional<ui::PlatformTextInputRequest> active_request;
    ui::ImeComposition last_composition;
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

std::unique_ptr<NativeEditorTsfTextServicesAdapter> make_native_editor_tsf_text_services_adapter() {
    return std::make_unique<NativeEditorTsfTextServicesAdapter>();
}

} // namespace mirakana::editor
