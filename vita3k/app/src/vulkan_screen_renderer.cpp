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

#include <app/vulkan_screen_renderer.h>

#include <util/log.h>
#include <renderer/vulkan/functions.h>

#include <SDL_vulkan.h>

// 200000000 Nanoseconds = 0.2 seconds
constexpr uint64_t next_image_timeout = 200000000;

const std::array<float, 4> clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };
const vk::ClearValue clear_value((vk::ClearColorValue(clear_color)));

namespace app {
vulkan_screen_renderer::vulkan_screen_renderer(SDL_Window *window, renderer::vulkan::VulkanState &renderer)
    : window(window), renderer(renderer) { }

vulkan_screen_renderer::~vulkan_screen_renderer() { // WILL THIS BE CALLED IN TIME
    renderer::vulkan::free_command_buffer(renderer,
        renderer::vulkan::CommandType::General, renderer.render_command_buffer);

    renderer.device.destroy(renderer.image_acquired_semaphore);
    renderer.device.destroy(renderer.render_complete_semaphore);
}

bool vulkan_screen_renderer::init(const std::string &base_path) {
    const auto builtin_shaders_path = base_path + "shaders-builtin/";

    vertex_module = renderer::vulkan::load_shader(renderer, builtin_shaders_path + "vulkan_render_vert.spv");
    fragment_module = renderer::vulkan::load_shader(renderer, builtin_shaders_path + "vulkan_render_frag.spv");
    if (!vertex_module || !fragment_module) {
        LOG_ERROR("Cannot find nessesary shaders for renderering. Exiting.");
        return true;
    }

    vk::BufferCreateInfo buffer_info(
        vk::BufferCreateFlags(),
        screen_vertex_size,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        vk::SharingMode::eExclusive, 0, nullptr);
    buffer = renderer::vulkan::create_buffer(renderer, buffer_info,
        renderer::vulkan::MemoryType::Device, buffer_allocation);

    vk::ImageCreateInfo image_info(
        vk::ImageCreateFlags(),
        vk::ImageType::e2D,
        vk::Format::eR8G8B8A8Uint,
        vk::Extent3D(DEFAULT_RES_WIDTH, DEFAULT_RES_HEIGHT, 1),
        1, 1,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eLinear,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::SharingMode::eExclusive, 0, nullptr,
        vk::ImageLayout::eUndefined
    );
    screen_image = renderer::vulkan::create_image(renderer, image_info,
        renderer::vulkan::MemoryType::Mappable, buffer_allocation);

    const screen_vertices_t vertex_buffer_data = {
        { { -1.f, -1.f, 0.0f }, { 0.f, 1.f } },
        { { 1.f, -1.f, 0.0f }, { 1.f, 1.f } },
        { { 1.f, 1.f, 0.0f }, { 1.f, 0.f } },
        { { -1.f, 1.f, 0.0f }, { 0.f, 0.f } }
    };

    vk::CommandBuffer transfer_buffer =
        renderer::vulkan::create_command_buffer(renderer, renderer::vulkan::CommandType::Transfer);

    vk::CommandBufferBeginInfo begin_info(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
        nullptr
    );
    transfer_buffer.begin(begin_info);
    transfer_buffer.updateBuffer(buffer, 0, screen_vertex_size, vertex_buffer_data);
    transfer_buffer.end();

    vk::SubmitInfo submit_info(
        0, nullptr, nullptr,
        1, &transfer_buffer,
        0, nullptr
    );
    vk::Queue queue = renderer::vulkan::select_queue(renderer, renderer::vulkan::CommandType::Transfer);
    queue.submit(1, &submit_info, vk::Fence());
    queue.waitIdle();

    renderer.render_command_buffer = create_command_buffer(renderer, renderer::vulkan::CommandType::General);

    vk::SemaphoreCreateInfo semaphore_info((vk::SemaphoreCreateFlags()));

    renderer.image_acquired_semaphore = renderer.device.createSemaphore(semaphore_info);
    renderer.render_complete_semaphore = renderer.device.createSemaphore(semaphore_info);

    return true;
}
void vulkan_screen_renderer::render(const HostState &state) {

}

void vulkan_screen_renderer::begin_render() {
    renderer.needs_resize = false;
    renderer.image_index = ~0u;

    vk::Result acquire_result = renderer.device.acquireNextImageKHR(renderer.swapchain,
        next_image_timeout, renderer.image_acquired_semaphore, vk::Fence(), &renderer.image_index);

    while (acquire_result == vk::Result::eErrorOutOfDateKHR) {
        int width, height;
        SDL_Vulkan_GetDrawableSize(window, &width, &height);
        renderer::vulkan::resize_swapchain(renderer, vk::Extent2D(width, height));
        renderer.needs_resize = true;

        acquire_result = renderer.device.acquireNextImageKHR(renderer.swapchain,
            next_image_timeout, renderer.image_acquired_semaphore, vk::Fence(), &renderer.image_index);
    }

    if (acquire_result != vk::Result::eSuccess) {
            LOG_WARN("Failed to get next image. Error: {}", static_cast<VkResult>(acquire_result));
        return;
    }
    
    renderer.render_command_buffer.reset(vk::CommandBufferResetFlags());

    vk::CommandBufferBeginInfo begin_info(
        vk::CommandBufferUsageFlags(), // No Flags
        nullptr // Inheritance
    );

    renderer.render_command_buffer.begin(begin_info);
    
    vk::RenderPassBeginInfo renderpass_begin_info(
        renderer.renderpass, // Renderpass
        renderer.framebuffers[renderer.image_index], // Framebuffer
        vk::Rect2D(
            vk::Offset2D(0, 0),
            vk::Extent2D(renderer.swapchain_width, renderer.swapchain_height)),
        1, &clear_value // Clear Colors
    );

    renderer.render_command_buffer.beginRenderPass(renderpass_begin_info, vk::SubpassContents::eInline);
}

void vulkan_screen_renderer::end_render() {
    renderer.render_command_buffer.endRenderPass();

    vk::ImageMemoryBarrier color_attachment_present_barrier(
        vk::AccessFlagBits::eColorAttachmentWrite, // From rendering
        vk::AccessFlagBits::eMemoryRead, // To readable format for presenting
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, // No Transfer
        renderer.swapchain_images[renderer.image_index], // Image
        renderer::vulkan::base_subresource_range);

    renderer.render_command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe, // Color Attachment Output -> Bottom of Pipe Stage
        vk::DependencyFlags(), // No Dependency Flags
        0, nullptr, // No Memory Barriers
        0, nullptr, // No Buffer Barriers
        1, &color_attachment_present_barrier // Image Barrier
    );

    renderer.render_command_buffer.end();

    vk::PipelineStageFlags image_wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    vk::SubmitInfo submit_info(
        1, &renderer.image_acquired_semaphore, &image_wait_stage, // Wait Semaphores (wait until image has been acquired to output)
        1, &renderer.render_command_buffer, // Command Buffers
        1, &renderer.render_complete_semaphore // Signal Render Complete Semaphore
    );

    vk::Queue render_queue = renderer::vulkan::select_queue(renderer, renderer::vulkan::CommandType::General);
    render_queue.submit(1, &submit_info, vk::Fence());
    
    vk::PresentInfoKHR present_info(
        1, &renderer.render_complete_semaphore, // Wait Render Complete Semaphore
        1, &renderer.swapchain, &renderer.image_index, nullptr // Swapchain
    );

    vk::Queue present_queue = renderer::vulkan::select_queue(renderer, renderer::vulkan::CommandType::General);
    present_queue.presentKHR(present_info);
    render_queue.waitIdle();
    present_queue.waitIdle(); // Wait idle is probably bad for performance.
}
}
