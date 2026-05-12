// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

enum class LogLevel { trace, debug, info, warn, error, fatal };

struct LogRecord {
    LogLevel level;
    std::string category;
    std::string message;
};

class ILogger {
  public:
    virtual ~ILogger() = default;
    virtual void write(LogRecord record) = 0;
};

class RingBufferLogger final : public ILogger {
  public:
    explicit RingBufferLogger(std::size_t capacity);

    void write(LogRecord record) override;
    void log(LogLevel level, std::string_view category, std::string_view message);

    [[nodiscard]] std::vector<LogRecord> records() const;
    [[nodiscard]] std::size_t capacity() const noexcept;
    void clear();

  private:
    std::size_t capacity_;
    std::vector<LogRecord> records_;
    mutable std::mutex mutex_;
};

} // namespace mirakana
