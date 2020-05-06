#pragma once

#include <renderer/state.h>
#include <renderer/types.h>

#include <renderer/vulkan/types.h>

namespace renderer::vulkan {
struct VulkanRenderer : public renderer::Renderer {
    vk::Instance instance;
    vk::Device device;

    // Used for memory allocation and general query later.
    vk::PhysicalDevice physical_device;
    vk::PhysicalDeviceProperties physical_device_properties;
    vk::PhysicalDeviceFeatures physical_device_features;
    vk::SurfaceCapabilitiesKHR physical_device_surface_capabilities;
    std::vector<vk::SurfaceFormatKHR> physical_device_surface_formats;
    vk::PhysicalDeviceMemoryProperties physical_device_memory;
    std::vector<vk::QueueFamilyProperties> physical_device_queue_families;

    VmaAllocator allocator;

    uint32_t general_family_index = 0;
    uint32_t transfer_family_index = 0;
    uint32_t general_queue_last = 0;
    uint32_t transfer_queue_last = 0;
    std::vector<vk::Queue> general_queues;
    std::vector<vk::Queue> transfer_queues;

    // These might be merged into one queue, but for now they are different.
    vk::CommandPool general_command_pool;
    // Transfer pool has transient bit set.
    vk::CommandPool transfer_command_pool;

    vk::CommandBuffer general_command_buffer;

    vk::SurfaceKHR surface;
    vk::SwapchainKHR swapchain;

    // These would be vectors...
    uint32_t swapchain_width = 0, swapchain_height = 0;
    vk::Image swapchain_images[2];
    vk::ImageView swapchain_views[2];

    VulkanRenderer(); // implement me!
};
}