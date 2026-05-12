// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/win32_process.hpp"
#include "mirakana/platform/process.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace mirakana {
namespace {

class UniqueHandle {
  public:
    UniqueHandle() = default;
    explicit UniqueHandle(HANDLE handle) noexcept : handle_(handle) {}

    UniqueHandle(const UniqueHandle&) = delete;
    UniqueHandle& operator=(const UniqueHandle&) = delete;

    UniqueHandle(UniqueHandle&& other) noexcept : handle_(other.release()) {}

    UniqueHandle& operator=(UniqueHandle&& other) noexcept {
        if (this != &other) {
            reset(other.release());
        }
        return *this;
    }

    ~UniqueHandle() {
        reset();
    }

    [[nodiscard]] HANDLE get() const noexcept {
        return handle_;
    }

    [[nodiscard]] bool valid() const noexcept {
        return handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE;
    }

    [[nodiscard]] HANDLE release() noexcept {
        auto* const result = handle_;
        handle_ = nullptr;
        return result;
    }

    void reset(HANDLE handle = nullptr) noexcept {
        if (valid()) {
            CloseHandle(handle_);
        }
        handle_ = handle;
    }

  private:
    HANDLE handle_{nullptr};
};

[[nodiscard]] std::string last_error_message(std::string_view prefix) {
    const auto error = GetLastError();
    LPSTR buffer = nullptr;
    const auto chars = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&buffer), 0, nullptr);

    std::string message(prefix);
    message.append(" failed with error ");
    message.append(std::to_string(error));
    if (chars != 0 && buffer != nullptr) {
        message.append(": ");
        message.append(buffer, chars);
        LocalFree(buffer);
    }
    return message;
}

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

[[nodiscard]] bool needs_quotes(std::wstring_view argument) noexcept {
    return argument.empty() || argument.find_first_of(L" \t\"") != std::wstring_view::npos;
}

[[nodiscard]] std::wstring quote_windows_argument(std::wstring_view argument) {
    if (!needs_quotes(argument)) {
        return std::wstring(argument);
    }

    std::wstring result;
    result.push_back(L'"');
    std::size_t backslashes = 0;
    for (const auto character : argument) {
        if (character == L'\\') {
            ++backslashes;
            continue;
        }
        if (character == L'"') {
            result.append((backslashes * 2U) + 1U, L'\\');
            result.push_back(L'"');
            backslashes = 0;
            continue;
        }
        result.append(backslashes, L'\\');
        backslashes = 0;
        result.push_back(character);
    }
    result.append(backslashes * 2U, L'\\');
    result.push_back(L'"');
    return result;
}

[[nodiscard]] std::wstring build_command_line(const ProcessCommand& command) {
    auto command_line = quote_windows_argument(utf8_to_wide(command.executable));
    for (const auto& argument : command.arguments) {
        command_line.push_back(L' ');
        command_line.append(quote_windows_argument(utf8_to_wide(argument)));
    }
    return command_line;
}

[[nodiscard]] bool create_pipe_pair(UniqueHandle& read_handle, UniqueHandle& write_handle, std::string& diagnostic) {
    SECURITY_ATTRIBUTES attributes{};
    attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    attributes.bInheritHandle = TRUE;
    attributes.lpSecurityDescriptor = nullptr;

    HANDLE read = nullptr;
    HANDLE write = nullptr;
    if (CreatePipe(&read, &write, &attributes, 0) == 0) {
        diagnostic = last_error_message("CreatePipe");
        return false;
    }
    read_handle.reset(read);
    write_handle.reset(write);
    if (SetHandleInformation(read_handle.get(), HANDLE_FLAG_INHERIT, 0) == 0) {
        diagnostic = last_error_message("SetHandleInformation");
        return false;
    }
    return true;
}

void read_pipe_to_string(HANDLE handle, std::string& output) {
    std::array<char, 4096> buffer{};
    for (;;) {
        DWORD bytes_read = 0;
        const auto ok = ReadFile(handle, buffer.data(), static_cast<DWORD>(buffer.size()), &bytes_read, nullptr);
        if (ok == 0 || bytes_read == 0) {
            break;
        }
        output.append(buffer.data(), bytes_read);
    }
}

} // namespace

ProcessResult Win32ProcessRunner::run(const ProcessCommand& command) {
    ProcessResult result;
    if (!is_allowed_process_command(command)) {
        result.diagnostic = "process command is unsafe";
        return result;
    }

    UniqueHandle stdout_read;
    UniqueHandle stdout_write;
    UniqueHandle stderr_read;
    UniqueHandle stderr_write;
    if (!create_pipe_pair(stdout_read, stdout_write, result.diagnostic) ||
        !create_pipe_pair(stderr_read, stderr_write, result.diagnostic)) {
        return result;
    }

    STARTUPINFOW startup{};
    startup.cb = sizeof(STARTUPINFOW);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = stdout_write.get();
    startup.hStdError = stderr_write.get();

    PROCESS_INFORMATION process_info{};
    auto application = utf8_to_wide(command.executable);
    auto command_line = build_command_line(command);
    auto working_directory = utf8_to_wide(command.working_directory);

    const auto created =
        CreateProcessW(application.c_str(), command_line.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
                       working_directory.empty() ? nullptr : working_directory.c_str(), &startup, &process_info);

    if (created == 0) {
        result.diagnostic = last_error_message("CreateProcessW");
        result.exit_code = static_cast<int>(GetLastError());
        return result;
    }

    result.launched = true;
    UniqueHandle process(process_info.hProcess);
    UniqueHandle thread(process_info.hThread);
    stdout_write.reset();
    stderr_write.reset();

    std::thread stdout_reader(read_pipe_to_string, stdout_read.get(), std::ref(result.stdout_text));
    std::thread stderr_reader(read_pipe_to_string, stderr_read.get(), std::ref(result.stderr_text));

    WaitForSingleObject(process.get(), INFINITE);

    DWORD exit_code = 0;
    if (GetExitCodeProcess(process.get(), &exit_code) == 0) {
        result.diagnostic = last_error_message("GetExitCodeProcess");
        result.exit_code = -1;
    } else {
        result.exit_code = static_cast<int>(exit_code);
    }

    stdout_reader.join();
    stderr_reader.join();
    return result;
}

} // namespace mirakana
