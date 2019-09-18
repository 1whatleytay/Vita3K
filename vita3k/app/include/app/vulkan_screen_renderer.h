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

#include <app/screen_rendererer.h>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

struct SDL_Window;
namespace renderer::vulkan {
struct VulkanState;
}

namespace app {
class vulkan_screen_renderer : screen_renderer {
public:
    explicit vulkan_screen_renderer(SDL_Window *window, renderer::vulkan::VulkanState &renderer);
    ~vulkan_screen_renderer() override;

    bool init(const std::string &base_path) override;
    void render(const HostState &state) override;

    void begin_render() override;
    void end_render() override;

private:
    renderer::vulkan::VulkanState &renderer;
    SDL_Window *window;

    vk::ShaderModule vertex_module;
    vk::ShaderModule fragment_module;

    VmaAllocation buffer_allocation;
    vk::Buffer buffer;

    VmaAllocation screen_allocation;
    vk::Image screen_image;
    vk::ImageView screen_view;
};
}
