// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace mirakana::test {

struct Case {
    std::string name;
    std::function<void()> run;
};

inline std::vector<Case>& cases() {
    static std::vector<Case> value;
    return value;
}

struct Register {
    Register(std::string name, std::function<void()> run) {
        cases().push_back(Case{.name = std::move(name), .run = std::move(run)});
    }
};

// Single evaluation, no do-while (clang-tidy cppcoreguidelines-avoid-do-while).
[[noreturn]] inline void throw_require_failed(const char* const expression_text) {
    throw std::runtime_error(std::string("requirement failed: ") + expression_text);
}

inline void require_true(const bool ok, const char* const expression_text) {
    if (!ok) {
        throw_require_failed(expression_text);
    }
}

inline int run_all() noexcept {
    try {
        int failed = 0;
        for (const auto& test : cases()) {
            try {
                test.run();
                std::cout << "[PASS] " << test.name << '\n';
            } catch (const std::exception& error) {
                ++failed;
                std::cerr << "[FAIL] " << test.name << ": " << error.what() << '\n';
            } catch (...) {
                ++failed;
                std::cerr << "[FAIL] " << test.name << ": unknown exception\n";
            }
        }
        return failed == 0 ? 0 : 1;
    } catch (...) {
        return 1;
    }
}

} // namespace mirakana::test

#define MK_TEST_CONCAT_IMPL(a, b) a##b
#define MK_TEST_CONCAT(a, b) MK_TEST_CONCAT_IMPL(a, b)
#define MK_TEST(name)                                                                                                  \
    static void MK_TEST_CONCAT(MK_test_, __LINE__)(); /* NOLINT(misc-use-anonymous-namespace) */                       \
    static const ::mirakana::test::Register MK_TEST_CONCAT(MK_test_register_, __LINE__){                               \
        std::string(name),                                                                                             \
        +[]() { MK_TEST_CONCAT(MK_test_, __LINE__)(); }}; /* NOLINT(misc-use-anonymous-namespace) */                   \
    static void MK_TEST_CONCAT(MK_test_, __LINE__)()      /* NOLINT(misc-use-anonymous-namespace) */

#define MK_REQUIRE(condition) (::mirakana::test::require_true(static_cast<bool>(condition), #condition))
