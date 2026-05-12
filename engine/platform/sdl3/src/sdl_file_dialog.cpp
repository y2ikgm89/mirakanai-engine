// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/sdl3/sdl_file_dialog.hpp"

#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_properties.h>

#include <stdexcept>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

[[nodiscard]] std::runtime_error sdl_file_dialog_error(const char* operation) {
    return std::runtime_error(std::string(operation) + " failed: " + SDL_GetError());
}

[[nodiscard]] std::string sdl_error_or(std::string fallback) {
    const char* error = SDL_GetError();
    if (error != nullptr && error[0] != '\0') {
        return std::string{error};
    }
    return fallback;
}

[[nodiscard]] SDL_FileDialogType sdl3_file_dialog_type(FileDialogKind kind) noexcept {
    switch (kind) {
    case FileDialogKind::open_file:
        return SDL_FILEDIALOG_OPENFILE;
    case FileDialogKind::save_file:
        return SDL_FILEDIALOG_SAVEFILE;
    case FileDialogKind::open_folder:
        return SDL_FILEDIALOG_OPENFOLDER;
    }
    return SDL_FILEDIALOG_OPENFILE;
}

class SdlFileDialogFilterStorage final {
  public:
    explicit SdlFileDialogFilterStorage(std::vector<FileDialogFilter> filters) {
        names_.reserve(filters.size());
        patterns_.reserve(filters.size());
        filters_.reserve(filters.size());

        for (auto& filter : filters) {
            if (!is_valid_file_dialog_filter(filter)) {
                throw std::invalid_argument("file dialog filter is invalid");
            }
            names_.push_back(std::move(filter.name));
            patterns_.push_back(std::move(filter.pattern));
        }

        for (std::size_t index = 0; index < names_.size(); ++index) {
            filters_.push_back(SDL_DialogFileFilter{
                names_[index].c_str(),
                patterns_[index].c_str(),
            });
        }
    }

    [[nodiscard]] const std::vector<SDL_DialogFileFilter>& filters() const noexcept {
        return filters_;
    }

  private:
    std::vector<std::string> names_;
    std::vector<std::string> patterns_;
    std::vector<SDL_DialogFileFilter> filters_;
};

struct SdlFileDialogCallbackContext {
    std::shared_ptr<FileDialogResultQueue> results;
    FileDialogId id{0};
    std::string fallback_error{"native file dialog failed"};
};

[[nodiscard]] SDL_Window* native_window(SdlWindow& window) noexcept {
    return reinterpret_cast<SDL_Window*>(window.native_window().value);
}

void sdl3_file_dialog_callback(void* userdata, const char* const* filelist, int filter) noexcept {
    auto* context = static_cast<SdlFileDialogCallbackContext*>(userdata);
    if (context == nullptr || !context->results) {
        return;
    }

    try {
        if (filelist == nullptr) {
            context->results->push(FileDialogResult{
                context->id,
                FileDialogStatus::failed,
                {},
                -1,
                sdl_error_or(context->fallback_error),
            });
            return;
        }

        std::vector<std::string> paths;
        for (const char* const* file = filelist; *file != nullptr; ++file) {
            paths.emplace_back(*file);
        }

        context->results->push(FileDialogResult{
            context->id,
            paths.empty() ? FileDialogStatus::canceled : FileDialogStatus::accepted,
            std::move(paths),
            filter,
            {},
        });
    } catch (...) {
    }
}

struct SdlFileDialogLaunchContext {
    std::shared_ptr<FileDialogResultQueue> results;
    FileDialogId id{0};
    SdlFileDialogFilterStorage filters;

    SdlFileDialogLaunchContext(std::shared_ptr<FileDialogResultQueue> result_queue, FileDialogId dialog_id,
                               std::vector<FileDialogFilter> request_filters)
        : results(std::move(result_queue)), id(dialog_id), filters(std::move(request_filters)) {}
};

void sdl3_file_dialog_launch_callback(void* userdata, const char* const* filelist, int filter) noexcept {
    const std::unique_ptr<SdlFileDialogLaunchContext> context{static_cast<SdlFileDialogLaunchContext*>(userdata)};
    SdlFileDialogCallbackContext callback_context{context->results, context->id};
    sdl3_file_dialog_callback(&callback_context, filelist, filter);
}

void set_string_property(SDL_PropertiesID props, const char* name, const std::string& value) {
    if (!value.empty() && !SDL_SetStringProperty(props, name, value.c_str())) {
        throw sdl_file_dialog_error("SDL_SetStringProperty");
    }
}

} // namespace

SdlFileDialogService::SdlFileDialogService(SdlWindow* owner_window)
    : results_(std::make_shared<FileDialogResultQueue>()), owner_window_(owner_window) {}

FileDialogId SdlFileDialogService::show(FileDialogRequest request) {
    if (const auto error = validate_file_dialog_request(request); !error.empty()) {
        throw std::invalid_argument(error);
    }

    const auto id = next_id_++;
    auto context = std::make_unique<SdlFileDialogLaunchContext>(results_, id, request.filters);

    const SDL_PropertiesID props = SDL_CreateProperties();
    if (props == 0) {
        throw sdl_file_dialog_error("SDL_CreateProperties");
    }

    try {
        if (owner_window_ != nullptr &&
            !SDL_SetPointerProperty(props, SDL_PROP_FILE_DIALOG_WINDOW_POINTER, native_window(*owner_window_))) {
            throw sdl_file_dialog_error("SDL_SetPointerProperty");
        }

        const auto& filters = context->filters.filters();
        if (request.kind != FileDialogKind::open_folder && !filters.empty()) {
            if (!SDL_SetPointerProperty(props, SDL_PROP_FILE_DIALOG_FILTERS_POINTER,
                                        const_cast<SDL_DialogFileFilter*>(filters.data()))) {
                throw sdl_file_dialog_error("SDL_SetPointerProperty");
            }
            if (!SDL_SetNumberProperty(props, SDL_PROP_FILE_DIALOG_NFILTERS_NUMBER,
                                       static_cast<Sint64>(filters.size()))) {
                throw sdl_file_dialog_error("SDL_SetNumberProperty");
            }
        }

        if (!SDL_SetBooleanProperty(props, SDL_PROP_FILE_DIALOG_MANY_BOOLEAN, request.allow_many)) {
            throw sdl_file_dialog_error("SDL_SetBooleanProperty");
        }

        set_string_property(props, SDL_PROP_FILE_DIALOG_TITLE_STRING, request.title);
        set_string_property(props, SDL_PROP_FILE_DIALOG_LOCATION_STRING, request.default_location);
        set_string_property(props, SDL_PROP_FILE_DIALOG_ACCEPT_STRING, request.accept_label);
        set_string_property(props, SDL_PROP_FILE_DIALOG_CANCEL_STRING, request.cancel_label);
    } catch (...) {
        SDL_DestroyProperties(props);
        throw;
    }

    auto* raw_context = context.release();
    SDL_ShowFileDialogWithProperties(sdl3_file_dialog_type(request.kind), sdl3_file_dialog_launch_callback, raw_context,
                                     props);
    SDL_DestroyProperties(props);
    return id;
}

std::optional<FileDialogResult> SdlFileDialogService::poll_result(FileDialogId id) {
    return results_->poll(id);
}

} // namespace mirakana
