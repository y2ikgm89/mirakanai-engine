// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#define VK_USE_PLATFORM_ANDROID_KHR

#include <android/native_window.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <vulkan/vulkan.h>

#include <cstdint>

extern "C" const char* mirakana_android_vulkan_surface_extension() noexcept {
    return "VK_KHR_android_surface";
}

extern "C" std::uintptr_t mirakana_android_vulkan_native_window_handle(android_app* app) noexcept {
    if (app == nullptr || app->window == nullptr) {
        return 0;
    }

    return reinterpret_cast<std::uintptr_t>(app->window);
}

extern "C" VkResult mirakana_android_create_vulkan_surface(VkInstance instance, android_app* app,
                                                     const VkAllocationCallbacks* allocator,
                                                     VkSurfaceKHR* surface) noexcept {
    if (instance == VK_NULL_HANDLE || app == nullptr || app->window == nullptr || surface == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    const VkAndroidSurfaceCreateInfoKHR create_info{
        VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
        nullptr,
        0,
        app->window,
    };
    return vkCreateAndroidSurfaceKHR(instance, &create_info, allocator, surface);
}
