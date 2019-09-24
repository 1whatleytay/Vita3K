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

#pragma once

#include <renderer/types.h>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace renderer::vulkan {
struct VulkanContext : renderer::Context {
    // GXM Context Info
};

struct VulkanRenderTarget : renderer::RenderTarget {
    vk::RenderPass renderpass;
    VmaAllocation allocations[2];
    vk::Image images[2];
    vk::ImageView views[2];
    vk::Framebuffer framebuffers[2];
};

// This is seperated because I use similar objects a lot and it is getting irritating to type.
const vk::ImageSubresourceRange base_subresource_range = vk::ImageSubresourceRange(
    vk::ImageAspectFlagBits::eColor, // Aspect
    0, 1, // Level Range
    0, 1 // Layer Range
);
} // namespace renderer::vulkan
