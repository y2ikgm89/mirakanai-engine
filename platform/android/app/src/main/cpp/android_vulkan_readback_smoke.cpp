// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "android_vulkan_readback_smoke.hpp"

#include <android/log.h>
#include <vulkan/vulkan.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <vector>

namespace {

constexpr std::uint32_t k_readback_word = 0x4d4b4149U; // "MKAI"
constexpr VkDeviceSize k_readback_size = sizeof(std::uint32_t);
constexpr const char* k_validation_layer_name = "VK_LAYER_KHRONOS_validation";

struct VulkanReadbackContext {
    VkInstance instance{VK_NULL_HANDLE};
    VkDevice device{VK_NULL_HANDLE};
    VkBuffer buffer{VK_NULL_HANDLE};
    VkDeviceMemory memory{VK_NULL_HANDLE};
    VkCommandPool command_pool{VK_NULL_HANDLE};
    VkFence fence{VK_NULL_HANDLE};
    bool submitted_to_queue{false};

    void wait_for_submitted_work() noexcept {
        if (device != VK_NULL_HANDLE && submitted_to_queue) {
            static_cast<void>(vkDeviceWaitIdle(device));
            submitted_to_queue = false;
        }
    }

    ~VulkanReadbackContext() {
        if (device != VK_NULL_HANDLE) {
            wait_for_submitted_work();
            if (fence != VK_NULL_HANDLE) {
                vkDestroyFence(device, fence, nullptr);
            }
            if (command_pool != VK_NULL_HANDLE) {
                vkDestroyCommandPool(device, command_pool, nullptr);
            }
            if (buffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(device, buffer, nullptr);
            }
            if (memory != VK_NULL_HANDLE) {
                vkFreeMemory(device, memory, nullptr);
            }
            vkDestroyDevice(device, nullptr);
        }
        if (instance != VK_NULL_HANDLE) {
            vkDestroyInstance(instance, nullptr);
        }
    }
};

[[nodiscard]] mirakana_android::AndroidVulkanReadbackSmokeResult fail(
    const char* stage, VkResult result, bool validation_layer_enumerated = false) noexcept {
    return mirakana_android::AndroidVulkanReadbackSmokeResult{
        .ready = false,
        .validation_layer_enumerated = validation_layer_enumerated,
        .expected_word = k_readback_word,
        .actual_word = 0U,
        .failure_stage = stage,
        .vulkan_result = static_cast<int>(result),
    };
}

[[nodiscard]] bool find_memory_type(const VkPhysicalDeviceMemoryProperties& properties, std::uint32_t type_bits,
                                    VkMemoryPropertyFlags required, std::uint32_t& index) noexcept {
    for (std::uint32_t i = 0; i < properties.memoryTypeCount; ++i) {
        const auto supported = (type_bits & (1U << i)) != 0U;
        const auto flags = properties.memoryTypes[i].propertyFlags;
        if (supported && (flags & required) == required) {
            index = i;
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool has_khronos_validation_layer() {
    std::uint32_t layer_count = 0;
    if (vkEnumerateInstanceLayerProperties(&layer_count, nullptr) != VK_SUCCESS || layer_count == 0) {
        return false;
    }
    std::vector<VkLayerProperties> layers(layer_count);
    if (vkEnumerateInstanceLayerProperties(&layer_count, layers.data()) != VK_SUCCESS) {
        return false;
    }
    return std::any_of(layers.begin(), layers.end(), [](const VkLayerProperties& layer) {
        return std::strcmp(layer.layerName, k_validation_layer_name) == 0;
    });
}

[[nodiscard]] bool select_queue_family(VkPhysicalDevice physical_device, std::uint32_t& queue_family) {
    std::uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, nullptr);
    if (count == 0) {
        return false;
    }
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, families.data());
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto flags = families[i].queueFlags;
        if ((flags & VK_QUEUE_TRANSFER_BIT) != 0 || (flags & VK_QUEUE_GRAPHICS_BIT) != 0 ||
            (flags & VK_QUEUE_COMPUTE_BIT) != 0) {
            queue_family = i;
            return true;
        }
    }
    return false;
}

} // namespace

namespace mirakana_android {

AndroidVulkanReadbackSmokeResult run_android_vulkan_readback_smoke_impl() {
    VulkanReadbackContext context;
    const bool validation_layer_enumerated = has_khronos_validation_layer();
    const char* const enabled_layers[] = {k_validation_layer_name};

    const VkApplicationInfo app_info{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "Mirakanai Android Vulkan readback smoke",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "MIRAIKANAI Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
    };
    const VkInstanceCreateInfo instance_info{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = validation_layer_enumerated ? 1U : 0U,
        .ppEnabledLayerNames = validation_layer_enumerated ? enabled_layers : nullptr,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = nullptr,
    };
    if (const auto result = vkCreateInstance(&instance_info, nullptr, &context.instance); result != VK_SUCCESS) {
        return fail("vkCreateInstance", result, validation_layer_enumerated);
    }

    std::uint32_t physical_device_count = 0;
    if (const auto result = vkEnumeratePhysicalDevices(context.instance, &physical_device_count, nullptr);
        result != VK_SUCCESS || physical_device_count == 0) {
        return fail("vkEnumeratePhysicalDevices", result, validation_layer_enumerated);
    }
    std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
    if (const auto result =
            vkEnumeratePhysicalDevices(context.instance, &physical_device_count, physical_devices.data());
        result != VK_SUCCESS) {
        return fail("vkEnumeratePhysicalDevicesData", result, validation_layer_enumerated);
    }

    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    std::uint32_t queue_family = 0;
    for (const auto candidate : physical_devices) {
        if (select_queue_family(candidate, queue_family)) {
            physical_device = candidate;
            break;
        }
    }
    if (physical_device == VK_NULL_HANDLE) {
        return fail("selectQueueFamily", VK_ERROR_FEATURE_NOT_PRESENT, validation_layer_enumerated);
    }

    const float queue_priority = 1.0F;
    const VkDeviceQueueCreateInfo queue_info{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueFamilyIndex = queue_family,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
    };
    const VkDeviceCreateInfo device_info{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = nullptr,
        .pEnabledFeatures = nullptr,
    };
    if (const auto result = vkCreateDevice(physical_device, &device_info, nullptr, &context.device);
        result != VK_SUCCESS) {
        return fail("vkCreateDevice", result, validation_layer_enumerated);
    }

    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(context.device, queue_family, 0, &queue);
    if (queue == VK_NULL_HANDLE) {
        return fail("vkGetDeviceQueue", VK_ERROR_INITIALIZATION_FAILED, validation_layer_enumerated);
    }

    const VkBufferCreateInfo buffer_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = k_readback_size,
        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };
    if (const auto result = vkCreateBuffer(context.device, &buffer_info, nullptr, &context.buffer);
        result != VK_SUCCESS) {
        return fail("vkCreateBuffer", result, validation_layer_enumerated);
    }

    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(context.device, context.buffer, &requirements);

    VkPhysicalDeviceMemoryProperties memory_properties{};
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

    std::uint32_t memory_type = std::numeric_limits<std::uint32_t>::max();
    const auto coherent_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    bool host_coherent = find_memory_type(memory_properties, requirements.memoryTypeBits, coherent_flags, memory_type);
    if (!host_coherent &&
        !find_memory_type(memory_properties, requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                          memory_type)) {
        return fail("findHostVisibleMemory", VK_ERROR_MEMORY_MAP_FAILED, validation_layer_enumerated);
    }

    const VkMemoryAllocateInfo allocation_info{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = requirements.size,
        .memoryTypeIndex = memory_type,
    };
    if (const auto result = vkAllocateMemory(context.device, &allocation_info, nullptr, &context.memory);
        result != VK_SUCCESS) {
        return fail("vkAllocateMemory", result, validation_layer_enumerated);
    }
    if (const auto result = vkBindBufferMemory(context.device, context.buffer, context.memory, 0); result != VK_SUCCESS) {
        return fail("vkBindBufferMemory", result, validation_layer_enumerated);
    }

    const VkCommandPoolCreateInfo pool_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queue_family,
    };
    if (const auto result = vkCreateCommandPool(context.device, &pool_info, nullptr, &context.command_pool);
        result != VK_SUCCESS) {
        return fail("vkCreateCommandPool", result, validation_layer_enumerated);
    }

    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    const VkCommandBufferAllocateInfo command_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = context.command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    if (const auto result = vkAllocateCommandBuffers(context.device, &command_info, &command_buffer);
        result != VK_SUCCESS) {
        return fail("vkAllocateCommandBuffers", result, validation_layer_enumerated);
    }

    const VkCommandBufferBeginInfo begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
    };
    if (const auto result = vkBeginCommandBuffer(command_buffer, &begin_info); result != VK_SUCCESS) {
        return fail("vkBeginCommandBuffer", result, validation_layer_enumerated);
    }
    vkCmdFillBuffer(command_buffer, context.buffer, 0, k_readback_size, k_readback_word);
    const VkBufferMemoryBarrier readback_barrier{
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = context.buffer,
        .offset = 0,
        .size = k_readback_size,
    };
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1,
                         &readback_barrier, 0, nullptr);
    if (const auto result = vkEndCommandBuffer(command_buffer); result != VK_SUCCESS) {
        return fail("vkEndCommandBuffer", result, validation_layer_enumerated);
    }

    const VkFenceCreateInfo fence_info{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };
    if (const auto result = vkCreateFence(context.device, &fence_info, nullptr, &context.fence); result != VK_SUCCESS) {
        return fail("vkCreateFence", result, validation_layer_enumerated);
    }
    const VkSubmitInfo submit_info{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr,
    };
    if (const auto result = vkQueueSubmit(queue, 1, &submit_info, context.fence); result != VK_SUCCESS) {
        return fail("vkQueueSubmit", result, validation_layer_enumerated);
    }
    context.submitted_to_queue = true;
    if (const auto result = vkWaitForFences(context.device, 1, &context.fence, VK_TRUE, 10'000'000'000ULL);
        result != VK_SUCCESS) {
        return fail("vkWaitForFences", result, validation_layer_enumerated);
    }
    context.submitted_to_queue = false;

    void* mapped = nullptr;
    if (const auto result = vkMapMemory(context.device, context.memory, 0, requirements.size, 0, &mapped);
        result != VK_SUCCESS || mapped == nullptr) {
        return fail("vkMapMemory", result, validation_layer_enumerated);
    }

    if (!host_coherent) {
        const VkMappedMemoryRange range{
            .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .pNext = nullptr,
            .memory = context.memory,
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        };
        if (const auto result = vkInvalidateMappedMemoryRanges(context.device, 1, &range); result != VK_SUCCESS) {
            vkUnmapMemory(context.device, context.memory);
            return fail("vkInvalidateMappedMemoryRanges", result, validation_layer_enumerated);
        }
    }

    std::uint32_t actual = 0;
    std::memcpy(&actual, mapped, sizeof(actual));
    vkUnmapMemory(context.device, context.memory);

    return AndroidVulkanReadbackSmokeResult{
        .ready = actual == k_readback_word,
        .validation_layer_enumerated = validation_layer_enumerated,
        .expected_word = k_readback_word,
        .actual_word = actual,
        .failure_stage = actual == k_readback_word ? "none" : "readback_mismatch",
        .vulkan_result = static_cast<int>(actual == k_readback_word ? VK_SUCCESS : VK_ERROR_INITIALIZATION_FAILED),
    };
}

AndroidVulkanReadbackSmokeResult run_android_vulkan_readback_smoke() noexcept {
    try {
        return run_android_vulkan_readback_smoke_impl();
    } catch (...) {
        return fail("exception", VK_ERROR_OUT_OF_HOST_MEMORY);
    }
}

void log_android_vulkan_readback_smoke_result(const AndroidVulkanReadbackSmokeResult& result) noexcept {
    __android_log_print(ANDROID_LOG_INFO, "Mirakanai",
                        "android_vulkan_readback_ready=%d android_vulkan_readback_backend=offscreen_transfer "
                        "android_vulkan_readback_bytes=%u android_vulkan_readback_expected=0x%08x "
                        "android_vulkan_readback_actual=0x%08x android_vulkan_readback_surface_required=0 "
                        "android_vulkan_validation_layer_enumerated=%d "
                        "android_vulkan_validation_layer_name=VK_LAYER_KHRONOS_validation "
                        "native_handle_access=0 failure_stage=%s vulkan_result=%d",
                        result.ready ? 1 : 0, static_cast<unsigned>(k_readback_size), result.expected_word,
                        result.actual_word, result.validation_layer_enumerated ? 1 : 0, result.failure_stage,
                        result.vulkan_result);
}

} // namespace mirakana_android
