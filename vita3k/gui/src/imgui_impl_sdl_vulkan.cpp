#include <gui/imgui_impl_sdl_vulkan.h>

#include <renderer/types.h>
#include <renderer/vulkan/functions.h>
#include <renderer/vulkan/state.h>

#include <util/log.h>

#include <SDL_vulkan.h>

#include <fstream>

constexpr vk::IndexType imgui_index_type = sizeof(ImDrawIdx) == 2 ? vk::IndexType::eUint16 : vk::IndexType::eUint32;

typedef float mat4[4 * 4];

struct TextureState {
    VmaAllocation allocation = VK_NULL_HANDLE;
    vk::Image image;
    vk::ImageView image_view;
    vk::DescriptorSet descriptor_set;
};

inline static renderer::vulkan::VulkanState &get_renderer(ImGui_VulkanState &state) {
    return dynamic_cast<renderer::vulkan::VulkanState &>(*state.renderer);
}

IMGUI_API ImGui_VulkanState *ImGui_ImplSdlVulkan_Init(renderer::State *renderer, SDL_Window *window, const std::string &base_path) {
    auto *state = new ImGui_VulkanState;
    state->renderer = renderer;
    state->window = window;

    renderer::vulkan::VulkanState &vulkan = get_renderer(*state);

    state->vertex_module = renderer::vulkan::load_shader(vulkan, base_path + "shaders-builtin/vulkan_imgui_vert.spv");
    state->fragment_module = renderer::vulkan::load_shader(vulkan, base_path + "shaders-builtin/vulkan_imgui_frag.spv");

    return state;
}

IMGUI_API void ImGui_ImplSdlVulkan_Shutdown(ImGui_VulkanState &state) {
    ImGui_ImplSdlVulkan_InvalidateDeviceObjects(state);

    get_renderer(state).device.destroy(state.vertex_module);
    get_renderer(state).device.destroy(state.fragment_module);
}

static void ImGui_ImplSdlVulkan_DeletePipeline(ImGui_VulkanState &state) {
    get_renderer(state).device.destroy(state.pipeline);
}

static bool ImGui_ImplSdlVulkan_CreatePipeline(ImGui_VulkanState &state) {
    std::vector<vk::PipelineShaderStageCreateInfo> shader_stage_infos = {
        vk::PipelineShaderStageCreateInfo(
            vk::PipelineShaderStageCreateFlags(), // No Flags
            vk::ShaderStageFlagBits::eVertex, // Vertex Shader
            state.vertex_module, // Module
            "main", // Name
            nullptr // Specialization
            ),
        vk::PipelineShaderStageCreateInfo(
            vk::PipelineShaderStageCreateFlags(), // No Flags
            vk::ShaderStageFlagBits::eFragment, // Fragment Shader
            state.fragment_module, // Module
            "main", // Name
            nullptr // Specialization
            ),
    };

    std::vector<vk::VertexInputBindingDescription> gui_pipeline_bindings = {
        vk::VertexInputBindingDescription(
            0, // Binding
            sizeof(ImDrawVert), // Stride
            vk::VertexInputRate::eVertex),
    };

    std::vector<vk::VertexInputAttributeDescription> gui_pipeline_attributes = {
        vk::VertexInputAttributeDescription(
            0, // Location
            0, // Binding
            vk::Format::eR32G32Sfloat,
            0 // Offset
            ),
        vk::VertexInputAttributeDescription(
            1, // Location
            0, // Binding
            vk::Format::eR32G32Sfloat,
            sizeof(ImVec2) // Offset
            ),
        vk::VertexInputAttributeDescription(
            2, // Location
            0, // Binding
            vk::Format::eR8G8B8A8Uint,
            sizeof(ImVec2) * 2 // Offset
            ),
    };

    vk::PipelineVertexInputStateCreateInfo gui_pipeline_vertex_info(
        vk::PipelineVertexInputStateCreateFlags(), // No Flags
        gui_pipeline_bindings.size(), gui_pipeline_bindings.data(), // Bindings
        gui_pipeline_attributes.size(), gui_pipeline_attributes.data() // Attributes
    );

    vk::PipelineInputAssemblyStateCreateInfo gui_pipeline_assembly_info(
        vk::PipelineInputAssemblyStateCreateFlags(), // No Flags
        vk::PrimitiveTopology::eTriangleList, // Topology
        false // No Primitive Restart?
    );

    vk::Viewport viewport(0, 0, get_renderer(state).swapchain_width, get_renderer(state).swapchain_height, 0.0f, 1.0f);
    vk::Rect2D scissor(vk::Offset2D(0, 0), vk::Extent2D(get_renderer(state).swapchain_width, get_renderer(state).swapchain_height));

    vk::PipelineViewportStateCreateInfo gui_pipeline_viewport_info(
        vk::PipelineViewportStateCreateFlags(),
        1, &viewport, // Viewport
        1, &scissor // Scissor
    );

    vk::PipelineRasterizationStateCreateInfo gui_pipeline_rasterization_info(
        vk::PipelineRasterizationStateCreateFlags(), // No Flags
        false, // No Depth Clamping
        false, // Rasterization is NOT Disabled
        vk::PolygonMode::eFill, // Fill Polygons
        vk::CullModeFlags(), // No Culling
        vk::FrontFace::eCounterClockwise, // Counter Clockwise Face Forwards
        false, 0, 0, 0, // No Depth Bias
        1.0f // Line Width
    );

    vk::PipelineMultisampleStateCreateInfo gui_pipeline_multisample_info(
        vk::PipelineMultisampleStateCreateFlags(), // No Flags
        vk::SampleCountFlagBits::e1, // No Multisampling
        false, 0, // No Sample Shading
        nullptr, // No Sample Mask
        false, false // Alpha Stays the Same
    );

    vk::PipelineDepthStencilStateCreateInfo gui_pipeline_depth_stencil_info(
        vk::PipelineDepthStencilStateCreateFlags(), // No Flags
        false, false, vk::CompareOp::eAlways, // No Depth Test
        false, // No Depth Bounds Test
        false, vk::StencilOpState(), vk::StencilOpState(), // No Stencil Test
        0.0f, 1.0f // Depth Bounds
    );

    vk::PipelineColorBlendAttachmentState attachment_blending(
        true, // Enable Blending
        vk::BlendFactor::eSrcAlpha, // Src Color
        vk::BlendFactor::eOneMinusSrcAlpha, // Dst Color
        vk::BlendOp::eAdd, // Color Blend Op
        vk::BlendFactor::eOne, // Src Alpha
        vk::BlendFactor::eZero, // Dst Alpha
        vk::BlendOp::eAdd, // Alpha Blend Op
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

    vk::PipelineColorBlendStateCreateInfo gui_pipeline_blend_info(
        vk::PipelineColorBlendStateCreateFlags(), // No Flags
        false, vk::LogicOp::eClear, // No Logic Op
        1, &attachment_blending, // Blending Attachments
        { 1, 1, 1, 1 } // Blend Constants
    );

    std::vector<vk::DynamicState> dynamic_states = {
        vk::DynamicState::eScissor,
        vk::DynamicState::eViewport,
    };

    vk::PipelineDynamicStateCreateInfo gui_pipeline_dynamic_info(
        vk::PipelineDynamicStateCreateFlags(), // No Flags
        dynamic_states.size(), dynamic_states.data() // Dynamic States
    );

    vk::GraphicsPipelineCreateInfo gui_pipeline_info(
        vk::PipelineCreateFlags(),
        shader_stage_infos.size(), shader_stage_infos.data(),
        &gui_pipeline_vertex_info,
        &gui_pipeline_assembly_info,
        nullptr, // No Tessellation
        &gui_pipeline_viewport_info,
        &gui_pipeline_rasterization_info,
        &gui_pipeline_multisample_info,
        &gui_pipeline_depth_stencil_info,
        &gui_pipeline_blend_info,
        &gui_pipeline_dynamic_info,
        state.pipeline_layout,
        get_renderer(state).renderpass,
        0,
        vk::Pipeline(),
        0);

    state.pipeline = get_renderer(state).device.createGraphicsPipeline(vk::PipelineCache(), gui_pipeline_info, nullptr);
    if (!state.pipeline) {
        LOG_ERROR("Failed to create Vulkan gui pipeline.");
        return false;
    }

    return true;
}

// Only one mapping can be created on a section of memory at a time. This method is split into the "vertex" and "index" parts to avoid overlapping maps.
static void ImGui_ImplSdlVulkan_UpdateBuffers(ImGui_VulkanState &state, ImDrawData *draw_data) {
    VkResult result;

    ImDrawVert *draw_buffer_data = nullptr;

    if (state.draw_buffer_vertices != draw_data->TotalVtxCount) {
        // Recreate Buffer
        if (state.draw_buffer)
            renderer::vulkan::destroy_buffer(get_renderer(state), state.draw_buffer, state.draw_allocation);

        vk::BufferCreateInfo draw_buffer_info(
            vk::BufferCreateFlags(), // No Flags
            draw_data->TotalVtxCount * sizeof(ImDrawVert), // Size
            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, // Usage
            vk::SharingMode::eExclusive, 0, nullptr // Exclusive Sharing Mode
        );

        state.draw_buffer = renderer::vulkan::create_buffer(get_renderer(state), draw_buffer_info,
            renderer::vulkan::MemoryType::Mappable, state.draw_allocation);

        state.draw_buffer_vertices = draw_data->TotalVtxCount;
    }

    result = vmaMapMemory(get_renderer(state).allocator, state.draw_allocation,
        reinterpret_cast<void **>(&draw_buffer_data));
    vmaInvalidateAllocation(get_renderer(state).allocator, state.draw_allocation,
        0, draw_data->TotalVtxCount * sizeof(ImDrawVert));
    if (result != VK_SUCCESS || !draw_buffer_data) {
        LOG_WARN("Failed to map memory for gui draw vertex buffer.");
        return;
    }

    size_t draw_buffer_pointer = 0;

    for (uint32_t a = 0; a < draw_data->CmdListsCount; a++) {
        ImDrawList *draw_list = draw_data->CmdLists[a];

        std::memcpy(&draw_buffer_data[draw_buffer_pointer], draw_list->VtxBuffer.Data, draw_list->VtxBuffer.Size * sizeof(ImDrawVert));

        draw_buffer_pointer += draw_list->VtxBuffer.Size;
    }

    vmaFlushAllocation(get_renderer(state).allocator, state.draw_allocation,
        0, draw_data->TotalVtxCount * sizeof(ImDrawVert));
    vmaUnmapMemory(get_renderer(state).allocator, state.draw_allocation);

    // Write to index buffer...
    ImDrawIdx *index_buffer_data = nullptr;

    if (state.index_buffer_indices != draw_data->TotalIdxCount) {
        // Recreate Buffer
        if (state.index_buffer)
            renderer::vulkan::destroy_buffer(get_renderer(state), state.index_buffer, state.index_allocation);

        vk::BufferCreateInfo index_buffer_info(
            vk::BufferCreateFlags(), // No Flags
            draw_data->TotalIdxCount * sizeof(ImDrawIdx), // Size
            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, // Usage
            vk::SharingMode::eExclusive, 0, nullptr // Exclusive Sharing Mode
        );

        state.index_buffer = renderer::vulkan::create_buffer(get_renderer(state), index_buffer_info,
            renderer::vulkan::MemoryType::Mappable, state.index_allocation);

        state.index_buffer_indices = draw_data->TotalIdxCount;
    }

    result = vmaMapMemory(get_renderer(state).allocator, state.index_allocation,
        reinterpret_cast<void **>(&index_buffer_data));
    vmaInvalidateAllocation(get_renderer(state).allocator, state.index_allocation,
        0, draw_data->TotalIdxCount * sizeof(ImDrawIdx));
    if (result != VK_SUCCESS || !index_buffer_data) {
        LOG_WARN("Failed to map memory for gui index buffer.");
        return;
    }

    size_t index_buffer_pointer = 0;

    for (uint32_t a = 0; a < draw_data->CmdListsCount; a++) {
        ImDrawList *draw_list = draw_data->CmdLists[a];

        std::memcpy(&index_buffer_data[index_buffer_pointer], draw_list->IdxBuffer.Data, draw_list->IdxBuffer.Size * sizeof(ImDrawIdx));

        index_buffer_pointer += draw_list->IdxBuffer.Size;
    }

    vmaFlushAllocation(get_renderer(state).allocator, state.index_allocation,
        0, draw_data->TotalIdxCount * sizeof(ImDrawIdx));
    vmaUnmapMemory(get_renderer(state).allocator, state.index_allocation);
}

IMGUI_API void ImGui_ImplSdlVulkan_RenderDrawData(ImGui_VulkanState &state) {
    ImDrawData *draw_data = ImGui::GetDrawData();

    if (!draw_data->CmdLists) return;

    if (get_renderer(state).needs_resize) {
        ImGui_ImplSdlVulkan_DeletePipeline(state);
        ImGui_ImplSdlVulkan_CreatePipeline(state);
    }

    ImGui_ImplSdlVulkan_UpdateBuffers(state, draw_data);

    const mat4 matrix = {
        2.0f / draw_data->DisplaySize.x, 0, 0, 0,
        0, 2.0f / draw_data->DisplaySize.y, 0, 0,
        0, 0, 1, 0,
        -1, -1, 0, 1,
    };

    get_renderer(state).render_command_buffer.pushConstants(
        state.pipeline_layout, vk::ShaderStageFlagBits::eVertex, // Pipeline Stage
        0, sizeof(mat4), // Offset and Size
        matrix // Data
    );

    get_renderer(state).render_command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, state.pipeline);

    vk::Viewport viewport(draw_data->DisplayPos.x, draw_data->DisplayPos.y,
        draw_data->DisplaySize.x, draw_data->DisplaySize.y, 0.0f, 1.0f);
    get_renderer(state).render_command_buffer.setViewport(0, 1, &viewport);

    uint64_t vertex_offset_null = 0;

    get_renderer(state).render_command_buffer.bindVertexBuffers(0, 1, &state.draw_buffer, &vertex_offset_null);
    get_renderer(state).render_command_buffer.bindIndexBuffer(state.index_buffer, 0, imgui_index_type);

    uint64_t vertex_offset = 0;
    uint64_t index_offset = 0;

    for (uint32_t a = 0; a < draw_data->CmdListsCount; a++) {
        ImDrawList *draw_list = draw_data->CmdLists[a];

        for (const auto &cmd : draw_list->CmdBuffer) {
            if (cmd.UserCallback) {
                cmd.UserCallback(draw_list, &cmd);
            } else {
                vk::Rect2D scissor_rect(
                    vk::Offset2D(cmd.ClipRect.x, cmd.ClipRect.y),
                    vk::Extent2D(cmd.ClipRect.z, cmd.ClipRect.w));
                get_renderer(state).render_command_buffer.setScissor(0, 1, &scissor_rect);
                auto *texture = reinterpret_cast<TextureState *>(cmd.TextureId);
                get_renderer(state).render_command_buffer.bindDescriptorSets(
                    vk::PipelineBindPoint::eGraphics, // Bind Point
                    state.pipeline_layout, // Layout
                    0, // Set Binding
                    1, &texture->descriptor_set,
                    0, nullptr);
                get_renderer(state).render_command_buffer.drawIndexed(cmd.ElemCount, 1, index_offset, vertex_offset, 0);
            }
            index_offset += cmd.ElemCount;
        }

        vertex_offset += draw_list->VtxBuffer.Size;
    }
}

IMGUI_API ImTextureID ImGui_ImplSdlVulkan_CreateTexture(ImGui_VulkanState &state, void *pixels, int width, int height) {
    auto *texture = new TextureState;

    const size_t buffer_size = width * height * 4;

    vk::BufferCreateInfo buffer_info(
        vk::BufferCreateFlags(), // No Flags
        buffer_size, // Size
        vk::BufferUsageFlagBits::eTransferSrc, // Usage
        vk::SharingMode::eExclusive, 0, nullptr // Sharing Mode
    );

    VmaAllocation temp_allocation;
    vk::Buffer temp_buffer = renderer::vulkan::create_buffer(get_renderer(state), buffer_info, renderer::vulkan::MemoryType::Mappable, temp_allocation);

    uint8_t *temp_memory;
    VkResult result = vmaMapMemory(get_renderer(state).allocator, temp_allocation, reinterpret_cast<void **>(&temp_memory));
    vmaInvalidateAllocation(get_renderer(state).allocator, temp_allocation, 0, buffer_size);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Could not map font buffer memory. VMA result: {}.", result);
        return nullptr;
    }
    if (!temp_memory) {
        LOG_ERROR("Could not map font buffer memory.");
        return nullptr;
    }
    std::memcpy(temp_memory, pixels, buffer_size);
    vmaFlushAllocation(get_renderer(state).allocator, temp_allocation, 0, buffer_size);
    vmaUnmapMemory(get_renderer(state).allocator, temp_allocation);

    vk::ImageCreateInfo image_info(
        vk::ImageCreateFlags(), // No Flags
        vk::ImageType::e2D, // 2D Image
        vk::Format::eR8G8B8A8Unorm, // RGBA32
        vk::Extent3D(width, height, 1), // Size
        1, // Mip Levels
        1, // Array Layers
        vk::SampleCountFlagBits::e1, // No Multisampling
        vk::ImageTiling::eOptimal, // Optimal Image Tiling
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, // Image can be copied to and sampled.
        vk::SharingMode::eExclusive, // Only one queue family can access at a time.
        0, nullptr, // Ignored on Exclusive sharing mode
        vk::ImageLayout::eUndefined // Sampling Layout (must be undefined)
    );

    texture->image = renderer::vulkan::create_image(get_renderer(state),
        image_info, renderer::vulkan::MemoryType::Device, texture->allocation);

    vk::CommandBuffer transfer_buffer = renderer::vulkan::create_command_buffer(get_renderer(state),
        renderer::vulkan::CommandType::Transfer);

    vk::CommandBufferBeginInfo begin_info(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit, // One Time Buffer
        nullptr // Inheritance Info
    );

    transfer_buffer.begin(begin_info);

    vk::ImageMemoryBarrier image_transfer_optimal_barrier(
        vk::AccessFlags(), // Was not written yet.
        vk::AccessFlagBits::eTransferWrite, // Will be written by a transfer operation.
        vk::ImageLayout::eUndefined, // Old Layout
        vk::ImageLayout::eTransferDstOptimal, // New Layout
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, // No Queue Family Transition
        texture->image,
        renderer::vulkan::base_subresource_range // Subresource Range
    );

    transfer_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, // Top Of Pipe -> Transfer Stage
        vk::DependencyFlags(), // No Dependency Flags
        0, nullptr, // No Memory Barriers
        0, nullptr, // No Buffer Memory Barriers
        1, &image_transfer_optimal_barrier // 1 Image Memory Barrier
    );

    vk::BufferImageCopy region(
        0, // Buffer Offset
        width, // Buffer Row Length
        height, // Buffer Height
        vk::ImageSubresourceLayers(
            vk::ImageAspectFlagBits::eColor, // Aspects
            0, 0, 1 // First Layer/Level
            ),
        vk::Offset3D(0, 0, 0), // Image Offset
        vk::Extent3D(width, height, 1) // Image Extent
    );

    transfer_buffer.copyBufferToImage(
        temp_buffer, // Buffer
        texture->image, // Image
        vk::ImageLayout::eTransferDstOptimal, // Image Layout
        1, &region // Regions
    );

    vk::ImageMemoryBarrier image_shader_read_only_barrier(
        vk::AccessFlagBits::eTransferWrite, // Was just written to.
        vk::AccessFlagBits(), // Will be read by the shader.
        vk::ImageLayout::eTransferDstOptimal, // Old Layout
        vk::ImageLayout::eShaderReadOnlyOptimal, // New Layout
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, // Transfer to General Queue (but I don't right now)
        texture->image, // Image
        renderer::vulkan::base_subresource_range // Subresource Range
    );

    transfer_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eBottomOfPipe, // Transfer -> Bottom of Pipe Stage
        vk::DependencyFlags(), // No Dependency Flags
        0, nullptr, // No Memory Barriers
        0, nullptr, // No Buffer Barriers
        1, &image_shader_read_only_barrier // Image Memory Barriers
    );

    transfer_buffer.end();

    vk::SubmitInfo submit_info(
        0, nullptr, nullptr, // No Wait Semaphores
        1, &transfer_buffer, // Command Buffer
        0, nullptr // No Signal Semaphores
    );
    vk::Queue submit_queue = renderer::vulkan::select_queue(get_renderer(state), renderer::vulkan::CommandType::Transfer);
    submit_queue.submit(1, &submit_info, vk::Fence());
    submit_queue.waitIdle();

    renderer::vulkan::free_command_buffer(get_renderer(state), renderer::vulkan::CommandType::Transfer, transfer_buffer);
    renderer::vulkan::destroy_buffer(get_renderer(state), temp_buffer, temp_allocation);

    vk::ImageViewCreateInfo font_view_info(
        vk::ImageViewCreateFlags(), // No Flags
        texture->image, // Image
        vk::ImageViewType::e2D, // Image View Type
        vk::Format::eR8G8B8A8Unorm, // Image View Format
        vk::ComponentMapping(), // Default Component Mapping (RGBA)
        renderer::vulkan::base_subresource_range // Subresource Range
    );

    texture->image_view = get_renderer(state).device.createImageView(font_view_info);

    vk::DescriptorSetAllocateInfo descriptor_info(
        state.descriptor_pool, // Descriptor Pool
        1, &state.sampler_layout // Layouts
    );

    texture->descriptor_set = get_renderer(state).device.allocateDescriptorSets(descriptor_info)[0];

    vk::DescriptorImageInfo descriptor_image_info(
        state.sampler, // Sampler
        texture->image_view, // Image View
        vk::ImageLayout::eShaderReadOnlyOptimal // Image Layout
    );

    vk::WriteDescriptorSet descriptor_write_info(
        texture->descriptor_set, // Set
        0, // Binding
        0, 1, // Array Range
        vk::DescriptorType::eCombinedImageSampler, // Type
        &descriptor_image_info,
        nullptr,
        nullptr);

    get_renderer(state).device.updateDescriptorSets(1, &descriptor_write_info, 0, nullptr);

    return texture;
}

IMGUI_API void ImGui_ImplSdlVulkan_DeleteTexture(ImGui_VulkanState &state, ImTextureID texture) {
    auto texture_ptr = reinterpret_cast<TextureState *>(texture);

    get_renderer(state).device.free(state.descriptor_pool, 1, &texture_ptr->descriptor_set);
    get_renderer(state).device.destroy(texture_ptr->image_view);
    renderer::vulkan::destroy_image(get_renderer(state), texture_ptr->image, texture_ptr->allocation);

    delete texture_ptr;
}

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void ImGui_ImplSdlVulkan_InvalidateDeviceObjects(ImGui_VulkanState &state) {
    renderer::vulkan::free_command_buffer(get_renderer(state), renderer::vulkan::CommandType::General, get_renderer(state).render_command_buffer);

    ImGui_ImplSdlVulkan_DeleteTexture(state, state.font_texture);
    ImGui_ImplSdlVulkan_DeletePipeline(state);

    get_renderer(state).device.destroy(state.pipeline_layout);
    get_renderer(state).device.destroy(state.sampler_layout);

    get_renderer(state).device.destroy(state.sampler);
}

IMGUI_API bool ImGui_ImplSdlVulkan_CreateDeviceObjects(ImGui_VulkanState &state) {
    vk::SamplerCreateInfo sampler_info(
        vk::SamplerCreateFlags(), // No Flags
        vk::Filter::eLinear, // Mag Filter
        vk::Filter::eLinear, // Min Filter
        vk::SamplerMipmapMode::eLinear, // Mipmap Mode
        vk::SamplerAddressMode::eRepeat, // U Mode
        vk::SamplerAddressMode::eRepeat, // V Mode
        vk::SamplerAddressMode::eRepeat, // W Mode
        0, // LOD Bias
        false, 1.0f, // Disable Anisotropy
        false, vk::CompareOp::eAlways, // No Comparing
        0.0f, 1.0f, // Min/Max LOD
        vk::BorderColor::eFloatOpaqueWhite, // Border Color
        false // Sampler is Normalized [0.0 - 1.0]
    );

    state.sampler = get_renderer(state).device.createSampler(sampler_info, nullptr);
    if (!state.sampler) {
        LOG_ERROR("Failed to create Vulkan gui sampler.");
        return false;
    }

    // Create Layouts
    {
        vk::DescriptorSetLayoutBinding sampler_layout(
            0, // Binding
            vk::DescriptorType::eCombinedImageSampler, // Descriptor Type
            1, // Array Size
            vk::ShaderStageFlagBits::eFragment, // Usage Stage
            &state.sampler // Used Samplers
        );

        vk::DescriptorSetLayoutCreateInfo sampler_layout_info(
            vk::DescriptorSetLayoutCreateFlags(), // No Flags
            1, &sampler_layout // Bindings
        );

        state.sampler_layout = get_renderer(state).device.createDescriptorSetLayout(sampler_layout_info, nullptr);
        if (!state.sampler_layout) {
            LOG_ERROR("Failed to create Vulkan gui sampler layout.");
            return false;
        }

        vk::PushConstantRange matrix_range(
            vk::ShaderStageFlagBits::eVertex,
            0,
            sizeof(mat4)
        );

        vk::PipelineLayoutCreateInfo pipeline_layout_info(
            vk::PipelineLayoutCreateFlags(), // No Flags
            1, &state.sampler_layout, // Descriptor Layouts
            1, &matrix_range // Push Constants
        );

        state.pipeline_layout = get_renderer(state).device.createPipelineLayout(pipeline_layout_info, nullptr);
        if (!state.pipeline_layout) {
            LOG_ERROR("Failed to create Vulkan gui pipeline layout.");
            return false;
        }
    }

    if (!ImGui_ImplSdlVulkan_CreatePipeline(state))
        return false;

    // Create Descriptor Pool
    {
        std::vector<vk::DescriptorPoolSize> pool_sizes = {
            vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 512)
        };

        vk::DescriptorPoolCreateInfo descriptor_pool_info(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, // Flags
            512, // Max Sets
            pool_sizes.size(), pool_sizes.data() // Pool Sizes
        );

        state.descriptor_pool = get_renderer(state).device.createDescriptorPool(descriptor_pool_info);
        if (!state.descriptor_pool) {
            LOG_ERROR("Failed to create Vulkan gui descriptor pool.");
            return false;
        }
    }

    // Create ImGui Texture
    {
        ImGuiIO &io = ImGui::GetIO();

        uint8_t *pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        state.font_texture = ImGui_ImplSdlVulkan_CreateTexture(state, pixels, width, height);
        io.Fonts->TexID = state.font_texture;
    }

    get_renderer(state).render_command_buffer = renderer::vulkan::create_command_buffer(get_renderer(state), renderer::vulkan::CommandType::General);

    state.init = true;

    return true;
}
