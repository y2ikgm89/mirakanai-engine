// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include <cstdlib>
#include <iostream>
#include <span>
#include <string>

int main(int argc, char** argv) {
    const std::span args(argv, static_cast<std::size_t>(argc));
    int exit_code = 0;
    for (std::size_t index = 1; index < args.size(); ++index) {
        const std::string arg = args[index];
        if (arg == "--stdout" && index + 1 < args.size()) {
            std::cout << args[++index] << '\n';
            continue;
        }
        if (arg == "--stderr" && index + 1 < args.size()) {
            std::cerr << args[++index] << '\n';
            continue;
        }
        if (arg == "--exit" && index + 1 < args.size()) {
            exit_code = std::atoi(args[++index]);
            continue;
        }
    }
    return exit_code;
}
