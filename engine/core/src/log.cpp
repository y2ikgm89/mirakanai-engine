// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/core/log.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace mirakana {

RingBufferLogger::RingBufferLogger(std::size_t capacity) : capacity_(capacity) {
    if (capacity_ == 0) {
        throw std::invalid_argument("RingBufferLogger capacity must be greater than zero");
    }
    records_.reserve(capacity_);
}

void RingBufferLogger::write(LogRecord record) {
    std::scoped_lock lock(mutex_);
    if (records_.size() == capacity_) {
        std::ranges::rotate(records_, records_.begin() + 1);
        records_.back() = std::move(record);
        return;
    }
    records_.push_back(std::move(record));
}

void RingBufferLogger::log(LogLevel level, std::string_view category, std::string_view message) {
    write(LogRecord{.level = level, .category = std::string(category), .message = std::string(message)});
}

std::vector<LogRecord> RingBufferLogger::records() const {
    std::scoped_lock lock(mutex_);
    return records_;
}

std::size_t RingBufferLogger::capacity() const noexcept {
    return capacity_;
}

void RingBufferLogger::clear() {
    std::scoped_lock lock(mutex_);
    records_.clear();
}

} // namespace mirakana
