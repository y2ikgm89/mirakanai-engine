// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/platform/process.hpp"

namespace mirakana {

class Win32ProcessRunner final : public IProcessRunner {
  public:
    [[nodiscard]] ProcessResult run(const ProcessCommand& command) override;
};

} // namespace mirakana
