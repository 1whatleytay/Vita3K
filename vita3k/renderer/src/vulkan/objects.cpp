// Vita3K emulator project
// Copyright (C) 2018 Vita3K team
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#include <util/log.h>
#include <renderer/vulkan/functions.h>
#include <renderer/vulkan/state.h>

#include <fstream>

namespace renderer::vulkan {
vk::ShaderModule load_shader(VulkanState &state, const std::string &path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.good()) {
        LOG_ERROR("Could not find shader SPIR-V at `{}`.", path);
        return vk::ShaderModule();
    }
    std::vector<char> shader_data(file.tellg());
    file.seekg(0, std::ios::beg);

    file.read(shader_data.data(), shader_data.size());
    file.close();

    vk::ShaderModuleCreateInfo shader_info(
        vk::ShaderModuleCreateFlags(), // No Flags
        shader_data.size(), reinterpret_cast<uint32_t *>(shader_data.data()) // Code
    );

    vk::ShaderModule module = state.device.createShaderModule(shader_info, nullptr);
    if (!module)
        LOG_ERROR("Could not build Vulkan shader module from SPIR-V at `{}`.", path);

    return module;
}

vk::CommandBuffer create_command_buffer(VulkanState &state, CommandType type) {
    vk::CommandBuffer buffer;

    switch (type) {
    case CommandType::General: {
        vk::CommandBufferAllocateInfo command_buffer_info(
            state.general_command_pool, // Command Pool
            vk::CommandBufferLevel::ePrimary, // Level
            1 // Count
        );

        state.device.allocateCommandBuffers(&command_buffer_info, &buffer);
        break;
    }
    case CommandType::Transfer: {
        vk::CommandBufferAllocateInfo command_buffer_info(
            state.transfer_command_pool, // Command Pool
            vk::CommandBufferLevel::ePrimary, // Level
            1 // Count
        );

        state.device.allocateCommandBuffers(&command_buffer_info, &buffer);
        break;
    }
    }

    assert(buffer);

    return buffer;
}

void free_command_buffer(VulkanState &state, CommandType type, vk::CommandBuffer buffer) {
    vk::CommandPool pool;

    switch (type) {
    case CommandType::General:
        pool = state.general_command_pool;
        break;
    case CommandType::Transfer:
        pool = state.transfer_command_pool;
        break;
    }

    state.device.freeCommandBuffers(pool, 1, &buffer);
}

vk::Buffer create_buffer(VulkanState &state, const vk::BufferCreateInfo &buffer_info, MemoryType type, VmaAllocation &allocation) {
    VmaMemoryUsage memory_usage;
    switch (type) {
    case MemoryType::Mappable:
        memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        break;
    case MemoryType::Device:
        memory_usage = VMA_MEMORY_USAGE_GPU_ONLY;
        break;
    }

    VmaAllocationCreateInfo allocation_info = {};
    allocation_info.flags = 0;
    allocation_info.usage = memory_usage;
    // Usage is specified via usage field. Others are ignored.
    allocation_info.requiredFlags = 0;
    allocation_info.preferredFlags = 0;
    allocation_info.memoryTypeBits = 0;
    allocation_info.pool = VK_NULL_HANDLE;
    allocation_info.pUserData = nullptr;

    VkBuffer buffer;
    VkResult result = vmaCreateBuffer(state.allocator,
                                      reinterpret_cast<const VkBufferCreateInfo *>(&buffer_info),
                                      &allocation_info, &buffer, &allocation, nullptr);
    assert(result == VK_SUCCESS);
    assert(allocation != VK_NULL_HANDLE);
    assert(buffer != VK_NULL_HANDLE);

    return buffer;
}

void destroy_buffer(VulkanState &state, vk::Buffer buffer, VmaAllocation allocation) {
    vmaDestroyBuffer(state.allocator, buffer, allocation);
}

vk::Image create_image(VulkanState &state, const vk::ImageCreateInfo &image_info, MemoryType type, VmaAllocation &allocation) {
    VmaMemoryUsage memory_usage;
    switch (type) {
    case MemoryType::Mappable:
        memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        break;
    case MemoryType::Device:
        memory_usage = VMA_MEMORY_USAGE_GPU_ONLY;
        break;
    }

    VmaAllocationCreateInfo allocation_info = {};
    allocation_info.flags = 0;
    allocation_info.usage = memory_usage;
    // Usage is specified via usage field. Others are ignored.
    allocation_info.requiredFlags = 0;
    allocation_info.preferredFlags = 0;
    allocation_info.memoryTypeBits = 0;
    allocation_info.pool = VK_NULL_HANDLE;
    allocation_info.pUserData = nullptr;

    VkImage image;
    VkResult result = vmaCreateImage(state.allocator,
                                     reinterpret_cast<const VkImageCreateInfo *>(&image_info),
                                     &allocation_info, &image, &allocation, nullptr);
    assert(result == VK_SUCCESS);
    assert(allocation != VK_NULL_HANDLE);
    assert(image != VK_NULL_HANDLE);

    return image;
}

void destroy_image(VulkanState &state, vk::Image image, VmaAllocation allocation) {
    vmaDestroyImage(state.allocator, image, allocation);
}
}
