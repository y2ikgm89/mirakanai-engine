// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#if defined(_WIN32)
#define MK_DYNAMIC_LIBRARY_PROBE_EXPORT __declspec(dllexport)
#else
#define MK_DYNAMIC_LIBRARY_PROBE_EXPORT __attribute__((visibility("default")))
#endif

extern "C" MK_DYNAMIC_LIBRARY_PROBE_EXPORT int MK_dynamic_library_probe_add(const int lhs, const int rhs) {
    return lhs + rhs;
}
