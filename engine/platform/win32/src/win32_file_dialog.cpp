// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/win32/win32_file_dialog.hpp"

#include "win32_utf.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <objbase.h>
#include <shobjidl.h>

#include <filesystem>
#include <stdexcept>
#include <utility>

namespace mirakana::win32 {
namespace {

template <typename T> class ComPtr final {
  public:
    ComPtr() = default;
    ~ComPtr() {
        reset();
    }

    ComPtr(const ComPtr&) = delete;
    ComPtr& operator=(const ComPtr&) = delete;

    [[nodiscard]] T* get() const noexcept {
        return value_;
    }

    [[nodiscard]] T** put() noexcept {
        reset();
        return &value_;
    }

    void reset() noexcept {
        if (value_ != nullptr) {
            value_->Release();
            value_ = nullptr;
        }
    }

  private:
    T* value_{nullptr};
};

class ComApartment final {
  public:
    ComApartment() {
        const HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        initialized_ = SUCCEEDED(hr);
        if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
            throw std::runtime_error("CoInitializeEx failed");
        }
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

class CoTaskString final {
  public:
    explicit CoTaskString(PWSTR value) noexcept : value_(value) {}
    ~CoTaskString() {
        CoTaskMemFree(value_);
    }

    CoTaskString(const CoTaskString&) = delete;
    CoTaskString& operator=(const CoTaskString&) = delete;

    [[nodiscard]] PWSTR get() const noexcept {
        return value_;
    }

  private:
    PWSTR value_{nullptr};
};

[[nodiscard]] std::runtime_error win32_file_dialog_error(const char* operation) {
    return std::runtime_error(std::string(operation) + " failed");
}

[[nodiscard]] std::string filter_spec_from_pattern(std::string_view pattern) {
    if (pattern == "*") {
        return "*.*";
    }

    std::string result;
    std::size_t part_start = 0;
    while (part_start <= pattern.size()) {
        const auto part_end = pattern.find(';', part_start);
        const auto part = pattern.substr(part_start, part_end == std::string_view::npos ? std::string_view::npos
                                                                                        : part_end - part_start);
        if (!result.empty()) {
            result += ';';
        }
        if (part.starts_with("*.")) {
            result += part;
        } else if (part.starts_with('.')) {
            result += '*';
            result += part;
        } else {
            result += "*.";
            result += part;
        }

        if (part_end == std::string_view::npos) {
            break;
        }
        part_start = part_end + 1;
    }
    return result;
}

[[nodiscard]] std::vector<Win32FileDialogFilterSpec> filter_specs_from_request(const FileDialogRequest& request) {
    if (request.kind == FileDialogKind::open_folder) {
        return {};
    }

    std::vector<Win32FileDialogFilterSpec> result;
    result.reserve(request.filters.size());
    for (const auto& filter : request.filters) {
        result.push_back(Win32FileDialogFilterSpec{
            .display_name = filter.name,
            .spec = filter_spec_from_pattern(filter.pattern),
        });
    }
    return result;
}

[[nodiscard]] DWORD options_from_plan(const Win32FileDialogRequestPlan& plan) noexcept {
    DWORD options = FOS_NOCHANGEDIR;
    if (plan.force_filesystem) {
        options |= FOS_FORCEFILESYSTEM;
    }
    if (plan.path_must_exist) {
        options |= FOS_PATHMUSTEXIST;
    }
    if (plan.file_must_exist) {
        options |= FOS_FILEMUSTEXIST;
    }
    if (plan.overwrite_prompt) {
        options |= FOS_OVERWRITEPROMPT;
    }
    if (plan.allow_many) {
        options |= FOS_ALLOWMULTISELECT;
    }
    if (plan.pick_folders) {
        options |= FOS_PICKFOLDERS;
    }
    return options;
}

void set_dialog_title(IFileDialog& dialog, std::string_view title) {
    const auto title_wide = detail::wide_from_utf8(title);
    if (FAILED(dialog.SetTitle(title_wide.c_str()))) {
        throw win32_file_dialog_error("IFileDialog::SetTitle");
    }
}

void set_dialog_filters(IFileDialog& dialog, const std::vector<Win32FileDialogFilterSpec>& filters) {
    if (filters.empty()) {
        return;
    }

    std::vector<std::wstring> names;
    std::vector<std::wstring> specs;
    std::vector<COMDLG_FILTERSPEC> native_filters;
    names.reserve(filters.size());
    specs.reserve(filters.size());
    native_filters.reserve(filters.size());

    for (const auto& filter : filters) {
        names.push_back(detail::wide_from_utf8(filter.display_name));
        specs.push_back(detail::wide_from_utf8(filter.spec));
    }
    for (std::size_t index = 0; index < filters.size(); ++index) {
        native_filters.push_back(COMDLG_FILTERSPEC{names[index].c_str(), specs[index].c_str()});
    }

    if (FAILED(dialog.SetFileTypes(static_cast<UINT>(native_filters.size()), native_filters.data()))) {
        throw win32_file_dialog_error("IFileDialog::SetFileTypes");
    }
}

void set_default_folder_if_available(IFileDialog& dialog, std::string_view default_location) {
    if (default_location.empty()) {
        return;
    }

    std::error_code error;
    const auto absolute = std::filesystem::absolute(std::filesystem::path{std::string{default_location}}, error);
    if (error) {
        return;
    }

    const auto location_wide = absolute.wstring();
    ComPtr<IShellItem> folder;
    if (FAILED(SHCreateItemFromParsingName(location_wide.c_str(), nullptr, IID_PPV_ARGS(folder.put())))) {
        return;
    }
    static_cast<void>(dialog.SetDefaultFolder(folder.get()));
}

[[nodiscard]] std::string path_from_shell_item(IShellItem& item) {
    PWSTR raw_path = nullptr;
    if (FAILED(item.GetDisplayName(SIGDN_FILESYSPATH, &raw_path))) {
        throw win32_file_dialog_error("IShellItem::GetDisplayName");
    }
    CoTaskString path{raw_path};
    return detail::utf8_from_wide(std::wstring_view{path.get()});
}

[[nodiscard]] std::vector<std::string> collect_open_dialog_results(IFileOpenDialog& dialog, bool allow_many) {
    std::vector<std::string> paths;
    if (allow_many) {
        ComPtr<IShellItemArray> items;
        if (FAILED(dialog.GetResults(items.put()))) {
            throw win32_file_dialog_error("IFileOpenDialog::GetResults");
        }

        DWORD count = 0;
        if (FAILED(items.get()->GetCount(&count))) {
            throw win32_file_dialog_error("IShellItemArray::GetCount");
        }
        paths.reserve(count);
        for (DWORD index = 0; index < count; ++index) {
            ComPtr<IShellItem> item;
            if (FAILED(items.get()->GetItemAt(index, item.put()))) {
                throw win32_file_dialog_error("IShellItemArray::GetItemAt");
            }
            paths.push_back(path_from_shell_item(*item.get()));
        }
        return paths;
    }

    ComPtr<IShellItem> item;
    if (FAILED(dialog.GetResult(item.put()))) {
        throw win32_file_dialog_error("IFileDialog::GetResult");
    }
    paths.push_back(path_from_shell_item(*item.get()));
    return paths;
}

[[nodiscard]] std::vector<std::string> collect_save_dialog_result(IFileSaveDialog& dialog) {
    ComPtr<IShellItem> item;
    if (FAILED(dialog.GetResult(item.put()))) {
        throw win32_file_dialog_error("IFileDialog::GetResult");
    }
    return {path_from_shell_item(*item.get())};
}

[[nodiscard]] int selected_filter_index(IFileDialog& dialog) noexcept {
    UINT index = 0;
    if (SUCCEEDED(dialog.GetFileTypeIndex(&index)) && index > 0) {
        return static_cast<int>(index - 1);
    }
    return -1;
}

void configure_dialog(IFileDialog& dialog, const Win32FileDialogRequestPlan& plan) {
    DWORD options = 0;
    if (FAILED(dialog.GetOptions(&options))) {
        throw win32_file_dialog_error("IFileDialog::GetOptions");
    }
    options |= options_from_plan(plan);
    if (FAILED(dialog.SetOptions(options))) {
        throw win32_file_dialog_error("IFileDialog::SetOptions");
    }

    set_dialog_title(dialog, plan.request.title);
    set_dialog_filters(dialog, plan.filters);
    set_default_folder_if_available(dialog, plan.request.default_location);
}

[[nodiscard]] FileDialogResult show_open_dialog(const Win32FileDialogRequestPlan& plan, FileDialogId id,
                                                HWND owner_window) {
    ComPtr<IFileOpenDialog> dialog;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(dialog.put())))) {
        throw win32_file_dialog_error("CoCreateInstance(CLSID_FileOpenDialog)");
    }
    configure_dialog(*dialog.get(), plan);

    const HRESULT show_result = dialog.get()->Show(owner_window);
    if (show_result == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
        return FileDialogResult{.id = id, .status = FileDialogStatus::canceled};
    }
    if (FAILED(show_result)) {
        throw win32_file_dialog_error("IFileDialog::Show");
    }

    return FileDialogResult{
        .id = id,
        .status = FileDialogStatus::accepted,
        .paths = collect_open_dialog_results(*dialog.get(), plan.allow_many),
        .selected_filter = selected_filter_index(*dialog.get()),
    };
}

[[nodiscard]] FileDialogResult show_save_dialog(const Win32FileDialogRequestPlan& plan, FileDialogId id,
                                                HWND owner_window) {
    ComPtr<IFileSaveDialog> dialog;
    if (FAILED(CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(dialog.put())))) {
        throw win32_file_dialog_error("CoCreateInstance(CLSID_FileSaveDialog)");
    }
    configure_dialog(*dialog.get(), plan);

    const HRESULT show_result = dialog.get()->Show(owner_window);
    if (show_result == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
        return FileDialogResult{.id = id, .status = FileDialogStatus::canceled};
    }
    if (FAILED(show_result)) {
        throw win32_file_dialog_error("IFileDialog::Show");
    }

    return FileDialogResult{
        .id = id,
        .status = FileDialogStatus::accepted,
        .paths = collect_save_dialog_result(*dialog.get()),
        .selected_filter = selected_filter_index(*dialog.get()),
    };
}

[[nodiscard]] FileDialogResult show_dialog(const Win32FileDialogRequestPlan& plan, FileDialogId id,
                                           std::uintptr_t owner_window_token) {
    ComApartment apartment;
    HWND owner_window = reinterpret_cast<HWND>(owner_window_token);
    if (plan.use_save_dialog) {
        return show_save_dialog(plan, id, owner_window);
    }
    return show_open_dialog(plan, id, owner_window);
}

} // namespace

Win32FileDialogRequestPlan plan_win32_file_dialog_request(FileDialogRequest request) {
    Win32FileDialogRequestPlan plan;
    plan.request = std::move(request);
    plan.diagnostic = validate_file_dialog_request(plan.request);
    if (!plan.diagnostic.empty()) {
        return plan;
    }

    plan.use_save_dialog = plan.request.kind == FileDialogKind::save_file;
    plan.use_open_dialog = !plan.use_save_dialog;
    plan.pick_folders = plan.request.kind == FileDialogKind::open_folder;
    plan.force_filesystem = true;
    plan.path_must_exist = true;
    plan.file_must_exist = plan.request.kind == FileDialogKind::open_file;
    plan.overwrite_prompt = plan.request.kind == FileDialogKind::save_file;
    plan.allow_many = plan.request.allow_many;
    plan.filters = filter_specs_from_request(plan.request);
    return plan;
}

Win32FileDialogService::Win32FileDialogService(std::uintptr_t owner_window_token)
    : results_(std::make_shared<FileDialogResultQueue>()), owner_window_token_(owner_window_token) {}

FileDialogId Win32FileDialogService::show(FileDialogRequest request) {
    auto plan = plan_win32_file_dialog_request(std::move(request));
    if (!plan.succeeded()) {
        throw std::invalid_argument(plan.diagnostic);
    }

    const auto id = next_id_++;
    try {
        results_->push(show_dialog(plan, id, owner_window_token_));
    } catch (const std::exception& exception) {
        results_->push(FileDialogResult{
            .id = id,
            .status = FileDialogStatus::failed,
            .paths = {},
            .selected_filter = -1,
            .error = exception.what(),
        });
    } catch (...) {
        results_->push(FileDialogResult{
            .id = id,
            .status = FileDialogStatus::failed,
            .paths = {},
            .selected_filter = -1,
            .error = "native Win32 file dialog failed",
        });
    }
    return id;
}

std::optional<FileDialogResult> Win32FileDialogService::poll_result(FileDialogId id) {
    return results_->poll(id);
}

} // namespace mirakana::win32
