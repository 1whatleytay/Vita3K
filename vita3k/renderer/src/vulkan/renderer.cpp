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

#include <renderer/functions.h>
#include <renderer/types.h>
#include <renderer/vulkan/functions.h>
#include <renderer/vulkan/state.h>

#include <config/version.h>

#include <SDL_vulkan.h>

//#define VULKAN_NO_VALIDATION_LAYERS

// Some random number as value. It will likely be very different. There are probably SCE feilds for this, I will look later.
constexpr static uint32_t max_sets = 192;
constexpr static uint32_t max_buffers = 64;
constexpr static uint32_t max_images = 64;
constexpr static uint32_t max_samplers = 64;

constexpr static vk::Format screen_format = vk::Format::eB8G8R8A8Unorm;

#ifdef __APPLE__
const char *surface_macos_extension = "VK_MVK_macos_surface";
const char *metal_surface_extension = "VK_EXT_metal_surface";

const char *get_surface_extension() {
    const std::vector<vk::ExtensionProperties> extensions = vk::enumerateInstanceExtensionProperties(nullptr);

    if (std::find_if(extensions.begin(), extensions.end(), [](const vk::ExtensionProperties &props) {
            return strcmp(props.extensionName, metal_surface_extension) == 0;
        })
        != extensions.end())
        return metal_surface_extension;

    if (std::find_if(extensions.begin(), extensions.end(), [](const vk::ExtensionProperties &props) {
            return strcmp(props.extensionName, surface_macos_extension) == 0;
        })
        != extensions.end())
        return surface_macos_extension;

    return nullptr;
}
#endif

const static std::vector<const char *> instance_layers = {
#if !defined(NDEBUG) && !defined(VULKAN_NO_VALIDATION_LAYERS)
    "VK_LAYER_LUNARG_standard_validation",
#endif
};

const static std::vector<const char *> instance_extensions = {
    "VK_KHR_surface",
#ifdef __APPLE__
    get_surface_extension(),
#endif
#ifdef WIN32
    "VK_KHR_win32_surface",
#endif
};

const static std::vector<const char *> device_layers = {
    // Nothing yet.
};

const static std::vector<const char *> device_extensions = {
    "VK_KHR_swapchain",
};

const static vk::PhysicalDeviceFeatures required_features({
    // .vertexPipelineStoresAndAtomics = true
    // etc.
});

const static std::vector<vk::DescriptorPoolSize> descriptor_pool_sizes = {
    vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, max_buffers),
    vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, max_images),
    vk::DescriptorPoolSize(vk::DescriptorType::eSampler, max_samplers),
};

const static std::vector<vk::Format> acceptable_surface_formats = {
    vk::Format::eB8G8R8A8Unorm,
};

namespace renderer::vulkan {
static bool device_is_compatible(
    vk::PhysicalDeviceProperties &properties,
    vk::PhysicalDeviceFeatures &features,
    vk::SurfaceCapabilitiesKHR &capabilities) {
    // TODO: Do any required checks here. Should check against required_features.

    return true;
}

static bool select_queues(VulkanState &vulkan_state,
    std::vector<vk::DeviceQueueCreateInfo> &queue_infos, std::vector<std::vector<float>> &queue_priorities) {
    // TODO: Better queue allocation.

    /**
     * Here's the idea:
     *  - Dedicated queues to a task (e.g. with only graphics bit set) are faster.
     *  - Queues that appear first in the list are faster.
     *  - We really just need a queue for Graphics and Transfer right now afaik.
     *  - Multiple queues families can do the same thing.
     *  - Multiple different queues should be chosen if available.
     * The current algorithm only picks the first one it finds, a new algorithm should be made that takes everything into account.
     */

    bool found_graphics = false, found_transfer = false;

    for (size_t a = 0; a < vulkan_state.physical_device_queue_families.size(); a++) {
        const auto &queue_family = vulkan_state.physical_device_queue_families[a];

        // MoltenVK does not accept nullptr a pPriorities for some reason.
        std::vector<float> &priorities = queue_priorities.emplace_back(queue_family.queueCount, 1.0f);

        // Only one DeviceQueueCreateInfo should be created per family.
        if (!found_graphics && queue_family.queueFlags & vk::QueueFlagBits::eGraphics
            && queue_family.queueFlags & vk::QueueFlagBits::eTransfer) {
            queue_infos.emplace_back(
                vk::DeviceQueueCreateFlagBits(), // No Flags
                a, // Queue Family Index
                queue_family.queueCount, // Queue Count
                priorities.data() // Priorities
            );
            vulkan_state.general_family_index = a;
            found_graphics = true;
        } else if (!found_transfer && queue_family.queueFlags & vk::QueueFlagBits::eTransfer) {
            queue_infos.emplace_back(
                vk::DeviceQueueCreateFlagBits(), // No Flags
                a, // Queue Family Index
                queue_family.queueCount, // Queue Count
                priorities.data() // Priorities
            );
            vulkan_state.transfer_family_index = a;
            found_transfer = true;
        }

        if (found_graphics && found_transfer)
            break;
    }

    return found_graphics && found_transfer;
}

static vk::Format select_surface_format(std::vector<vk::SurfaceFormatKHR> &formats) {
    for (const auto &format : formats) {
        if (std::find(acceptable_surface_formats.begin(), acceptable_surface_formats.end(), format.format)
            != acceptable_surface_formats.end())
            return format.format;
    }

    assert(false);

    return vk::Format::eR8G8B8A8Unorm;
}

vk::Queue select_queue(VulkanState &state, CommandType type) {
    vk::Queue queue;

    switch (type) {
    case CommandType::General:
        queue = state.general_queues[state.general_queue_last % state.general_queues.size()];
        state.general_queue_last++;
        break;
    case CommandType::Transfer:
        queue = state.transfer_queues[state.transfer_queue_last % state.transfer_queues.size()];
        state.transfer_queue_last++;
        break;
    }

    return queue;
}

void present(VulkanState &state, uint32_t image_index) { // this needs semaphore, image index etc.
    // The general queue family is guaranteed (by an assert) to have present support.
    vk::Queue present_queue = select_queue(state, CommandType::General);

    vk::PresentInfoKHR present_info(
        0, nullptr, // No Semaphores
        1, &state.swapchain, &image_index, nullptr // Swapchain
    );

    present_queue.presentKHR(present_info);
}

bool resize_swapchain(VulkanState &state, vk::Extent2D size) {
    state.swapchain_width = size.width;
    state.swapchain_height = size.height;

    if (state.swapchain) {
        state.device.destroy(state.renderpass);
        for (vk::Framebuffer framebuffer : state.framebuffers)
            state.device.destroy(framebuffer);
        for (vk::ImageView view : state.swapchain_views)
            state.device.destroy(view);
        state.device.destroy(state.swapchain);
    }

    // Create Swapchain
    {
        // TODO: Extents should be based on surface capabilities.
        vk::SwapchainCreateInfoKHR swapchain_info(
            vk::SwapchainCreateFlagsKHR(), // No Flags
            state.surface, // Surface
            2, // Double Buffering
            screen_format, // Image Format, BGRA is supported by MoltenVK
            vk::ColorSpaceKHR::eSrgbNonlinear, // Color Space
            size, // Image Extent
            1, // Image Array Length
            vk::ImageUsageFlagBits::eColorAttachment, // Image Usage, consider VK_IMAGE_USAGE_STORAGE_BIT?
            vk::SharingMode::eExclusive,
            0, nullptr, // Unused when sharing mode is exclusive
            vk::SurfaceTransformFlagBitsKHR::eIdentity, // Transform
            vk::CompositeAlphaFlagBitsKHR::eOpaque, // Alpha
            vk::PresentModeKHR::eFifo, // Present Mode, FIFO and Immediate are supported on MoltenVK. Would've chosen Mailbox otherwise.
            true, // Clipping
            vk::SwapchainKHR() // No old swapchain.
        );

        state.swapchain = state.device.createSwapchainKHR(swapchain_info);
        if (!state.swapchain) {
            LOG_ERROR("Failed to create Vulkan swapchain.");
            return false;
        }
    }

    // Get Swapchain Images
    uint32_t swapchain_image_count = 2;
    state.device.getSwapchainImagesKHR(state.swapchain, &swapchain_image_count, state.swapchain_images);

    for (uint32_t a = 0; a < 2 /*vulkan_state.swapchain_images.size()*/; a++) {
        const auto &image = state.swapchain_images[a];
        vk::ImageViewCreateInfo view_info(
            vk::ImageViewCreateFlags(), // No Flags
            image, // Image
            vk::ImageViewType::e2D, // Image View Type
            select_surface_format(state.physical_device_surface_formats), // Format
            vk::ComponentMapping(), // Default Component Mapping
            vk::ImageSubresourceRange(
                vk::ImageAspectFlagBits::eColor,
                0, // Mipmap Level
                1, // Level Count
                0, // Base Array Index
                1 // Layer Count
                ));

        vk::ImageView view = state.device.createImageView(view_info);
        if (!view) {
            LOG_ERROR("Failed to Vulkan image view for swpachain image id {}.", a);
            return false;
        }

        state.swapchain_views[a] = view;
    }

    vk::AttachmentDescription attachment_description(
        vk::AttachmentDescriptionFlags(), // No Flags
        vk::Format::eB8G8R8A8Unorm, // Format, BGRA is standard
        vk::SampleCountFlagBits::e1, // No Multisampling
        vk::AttachmentLoadOp::eClear, // Clear Image
        vk::AttachmentStoreOp::eStore, // Keep Image Data
        vk::AttachmentLoadOp::eDontCare, // No Stencils
        vk::AttachmentStoreOp::eDontCare, // No Stencils
        vk::ImageLayout::eUndefined, // Initial Layout
        vk::ImageLayout::eColorAttachmentOptimal // Final Layout
    );

    vk::AttachmentReference attachment_reference(
        0, // attachments[0]
        vk::ImageLayout::eColorAttachmentOptimal // Image Layout
    );

    vk::SubpassDescription subpass(
        vk::SubpassDescriptionFlags(), // No Flags
        vk::PipelineBindPoint::eGraphics, // Type
        0, nullptr, // No Inputs
        1, &attachment_reference, // Color Attachment References
        nullptr, nullptr, // No Resolve or Depth/Stencil for now
        0, nullptr // Vulkan Book says you don't need this for a Color Attachment?
    );

    vk::RenderPassCreateInfo renderpass_info(
        vk::RenderPassCreateFlags(), // No Flags
        1, &attachment_description, // Attachments
        1, &subpass, // Subpasses
        0, nullptr // Dependencies
    );

    state.renderpass = state.device.createRenderPass(renderpass_info, nullptr);
    if (!state.renderpass) {
            LOG_ERROR("Failed to create Vulkan gui renderpass.");
        return false;
    }

    // Create Framebuffer
    for (uint32_t a = 0; a < 2; a++) {
        vk::FramebufferCreateInfo framebuffer_info(
            vk::FramebufferCreateFlags(), // No Flags
            state.renderpass, // Renderpass
            1, &state.swapchain_views[a], // Attachments
            state.swapchain_width, state.swapchain_height, // Size
            1 // Layers
        );

        state.framebuffers[a] = state.device.createFramebuffer(framebuffer_info, nullptr);
    }

    return true;
}

void bind_context(State &state, Context &context, RenderTarget &render_target) {

}

bool create(const WindowPtr &window, std::unique_ptr<renderer::State> &state) {
    auto &vulkan_state = dynamic_cast<VulkanState &>(*state);

    // Create Instance
    {
        vk::ApplicationInfo app_info(
            app_name, // App Name
            0, // App Version
            org_name, // Engine Name, using org instead.
            0, // Engine Version
            VK_API_VERSION_1_0);

        vk::InstanceCreateInfo instance_info(
            vk::InstanceCreateFlags(), // No Flags
            &app_info, // App Info
            instance_layers.size(), instance_layers.data(), // No Layers
            instance_extensions.size(), instance_extensions.data() // No Extensions
        );

        vulkan_state.instance = vk::createInstance(instance_info);
        if (!vulkan_state.instance) {
            LOG_ERROR("Failed to create Vulkan instance.");
            return false;
        }
    }

    // Create Surface
    {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        bool surface_error = SDL_Vulkan_CreateSurface(window.get(), vulkan_state.instance, &surface);
        if (!surface_error) {
            const char *error = SDL_GetError();
            LOG_ERROR("Failed to create vulkan surface. SDL Error: {}.", error);
            return false;
        }
        vulkan_state.surface = vk::SurfaceKHR(surface);
        if (!vulkan_state.surface) {
            LOG_ERROR("Failed to create Vulkan surface.");
            return false;
        }
    }

    // Select Physical Device
    {
        std::vector<vk::PhysicalDevice> physical_devices = vulkan_state.instance.enumeratePhysicalDevices();

        for (const auto &device : physical_devices) {
            vk::PhysicalDeviceProperties properties = device.getProperties();
            vk::PhysicalDeviceFeatures features = device.getFeatures();
            vk::SurfaceCapabilitiesKHR capabilities = device.getSurfaceCapabilitiesKHR(vulkan_state.surface);
            if (device_is_compatible(properties, features, capabilities)) {
                vulkan_state.physical_device = device;
                vulkan_state.physical_device_properties = properties;
                vulkan_state.physical_device_features = features;
                vulkan_state.physical_device_surface_capabilities = capabilities;
                vulkan_state.physical_device_surface_formats = device.getSurfaceFormatsKHR(vulkan_state.surface);
                vulkan_state.physical_device_memory = device.getMemoryProperties();
                vulkan_state.physical_device_queue_families = device.getQueueFamilyProperties();
                break;
            }
        }

        if (!vulkan_state.physical_device) {
            LOG_ERROR("Failed to select Vulkan physical device.");
            return false;
        }
    }

    // Create Device
    {
        std::vector<vk::DeviceQueueCreateInfo> queue_infos;
        std::vector<std::vector<float>> queue_priorities;
        if (!select_queues(vulkan_state, queue_infos, queue_priorities)) {
            LOG_ERROR("Failed to select proper Vulkan queues. This is likely a bug.");
            return false;
        }

        if (!vulkan_state.physical_device.getSurfaceSupportKHR(
                vulkan_state.general_family_index, vulkan_state.surface)) {
            LOG_ERROR("Failed to select a Vulkan queue that supports presentation. This is likely a bug.");
            return false;
        }

        vk::DeviceCreateInfo device_info(
            vk::DeviceCreateFlags(), // No Flags
            queue_infos.size(), queue_infos.data(), // No Queues
            device_layers.size(), device_layers.data(), // No Layers
            device_extensions.size(), device_extensions.data(), // No Extensions
            &required_features);

        vulkan_state.device = vulkan_state.physical_device.createDevice(device_info);
        if (!vulkan_state.device) {
            LOG_ERROR("Failed to create a Vulkan device.");
            return false;
        }
    }

    // Get Queues
    for (uint32_t a = 0; a < vulkan_state.physical_device_queue_families[vulkan_state.general_family_index].queueCount; a++) {
        vulkan_state.general_queues.push_back(
            vulkan_state.device.getQueue(vulkan_state.general_family_index, a));
    }

    for (uint32_t a = 0; a < vulkan_state.physical_device_queue_families[vulkan_state.transfer_family_index].queueCount; a++) {
        vulkan_state.transfer_queues.push_back(
            vulkan_state.device.getQueue(vulkan_state.transfer_family_index, a));
    }

    // Create Command Pools
    {
        vk::CommandPoolCreateInfo general_pool_info(
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer, // Flags
            vulkan_state.general_family_index // Queue Family Index
        );

        vk::CommandPoolCreateInfo transfer_pool_info(
            vk::CommandPoolCreateFlagBits::eTransient, // Flags
            vulkan_state.transfer_family_index // Queue Family Index
        );

        vulkan_state.general_command_pool = vulkan_state.device.createCommandPool(general_pool_info);
        if (!vulkan_state.general_command_pool) {
            LOG_ERROR("Failed to create general command pool.");
            return false;
        }
        vulkan_state.transfer_command_pool = vulkan_state.device.createCommandPool(transfer_pool_info);
        if (!vulkan_state.transfer_command_pool) {
            LOG_ERROR("Failed to create transfer command pool.");
            return false;
        }
    }

    // Allocate Memory for Images and Buffers
    {
        VmaAllocatorCreateInfo allocator_info = {};
        allocator_info.flags = 0;
        allocator_info.physicalDevice = vulkan_state.physical_device;
        allocator_info.device = vulkan_state.device;
        allocator_info.preferredLargeHeapBlockSize = 0;
        allocator_info.pAllocationCallbacks = nullptr;
        allocator_info.pDeviceMemoryCallbacks = nullptr;
        allocator_info.frameInUseCount = 0;
        allocator_info.pHeapSizeLimit = nullptr;
        allocator_info.pVulkanFunctions = nullptr; // VMA_STATIC_VULKAN_FUNCTIONS 1 is default I think
        allocator_info.pRecordSettings = nullptr;

        VkResult result = vmaCreateAllocator(&allocator_info, &vulkan_state.allocator);
        if (result != VK_SUCCESS) {
            LOG_ERROR("Failed to create VMA allocator. VMA result: {}.", static_cast<uint32_t>(result));
            return false;
        }
        if (vulkan_state.allocator == VK_NULL_HANDLE) {
            LOG_ERROR("Failed to create VMA allocator.");
            return false;
        }
    }

    int width, height;
    SDL_Vulkan_GetDrawableSize(window.get(), &width, &height);
    resize_swapchain(vulkan_state, vk::Extent2D(width, height));

    return true;
}

void close(std::unique_ptr<renderer::State> &state) {
    auto &vulkan_state = reinterpret_cast<VulkanState &>(*state);

    vulkan_state.device.waitIdle();

    vmaDestroyAllocator(vulkan_state.allocator);

    vulkan_state.device.destroy(vulkan_state.swapchain);
    vulkan_state.instance.destroy(vulkan_state.surface);

    vulkan_state.device.destroy(vulkan_state.general_command_pool);
    vulkan_state.device.destroy(vulkan_state.transfer_command_pool);

    vulkan_state.device.destroy();
    vulkan_state.instance.destroy();
}

bool create(renderer::State &state, std::unique_ptr<renderer::Context> &context) {
    context = std::make_unique<VulkanContext>();

    return true;
}

bool create(renderer::State &state, std::unique_ptr<renderer::RenderTarget> &render_target,
    const SceGxmRenderTargetParams &params) {
    render_target = std::make_unique<VulkanRenderTarget>();
    auto *target = reinterpret_cast<VulkanRenderTarget *>(render_target.get());
    auto &vulkan = dynamic_cast<VulkanState &>(state);

    vk::AttachmentDescription attachment_description(
        vk::AttachmentDescriptionFlags(),
        vk::Format::eR8G8B8A8Unorm,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal);

    vk::AttachmentReference attachment_refernece(1, vk::ImageLayout::eColorAttachmentOptimal);

    vk::SubpassDescription subpass(
        vk::SubpassDescriptionFlags(),
        vk::PipelineBindPoint::eGraphics,
        0, nullptr,
        1, &attachment_refernece,
        nullptr, nullptr,
        0, nullptr);

    vk::RenderPassCreateInfo renderpass_info(
        vk::RenderPassCreateFlags(),
        1, &attachment_description,
        1, &subpass,
        0, nullptr);

    target->renderpass = vulkan.device.createRenderPass(renderpass_info);

    for (uint32_t a = 0; a < 2; a++) {
        vk::ImageCreateInfo image_info(
            vk::ImageCreateFlags(),
            vk::ImageType::e2D,
            vk::Format::eR8G8B8A8Unorm,
            vk::Extent3D(params.width, params.height, 1),
            1, 1,
            vk::SampleCountFlagBits::e1, // No Multisampling...
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
            vk::SharingMode::eExclusive, 0, nullptr,
            vk::ImageLayout::eUndefined);

        target->images[a] = create_image(vulkan, image_info, MemoryType::Device, target->allocations[a]);

        vk::ImageViewCreateInfo view_info(
            vk::ImageViewCreateFlags(),
            target->images[a],
            vk::ImageViewType::e2D,
            vk::Format::eR8G8B8A8Unorm,
            vk::ComponentMapping(),
            base_subresource_range);

        target->views[a] = vulkan.device.createImageView(view_info);

        vk::FramebufferCreateInfo framebuffer_info(
            vk::FramebufferCreateFlags(),
            target->renderpass,
            1, &target->views[a],
            params.width, params.height, 1);

        target->framebuffers[a] = vulkan.device.createFramebuffer(framebuffer_info);
    }

    return true;
}

bool create(renderer::State &state, std::unique_ptr<VertexProgram> &vp, const SceGxmProgram &program,
    GXPPtrMap &gxp_ptr_map, const char *base_path, const char *title_id) {

    return true;
}

bool create(renderer::State &state, std::unique_ptr<FragmentProgram> &fp, const SceGxmProgram &program,
    const emu::SceGxmBlendInfo *blend, GXPPtrMap &gxp_ptr_map, const char *base_path, const char *title_id) {

    return true;
}
} // namespace renderer::vulkan
