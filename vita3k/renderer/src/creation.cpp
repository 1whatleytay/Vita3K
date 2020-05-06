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

#include <gxm/types.h>
#include <renderer/commands.h>
#include <renderer/renderer.h>
#include <renderer/texture_cache_state.h>
#include <renderer/types.h>

#include <renderer/gl/renderer.h>
#ifdef USE_VULKAN
#include <renderer/vulkan/renderer.h>
#endif
#include <renderer/texture_cache_state.h>

#include "driver_functions.h"

#include <gxm/types.h>
#include <renderer/functions.h>
#include <util/log.h>

namespace renderer {
COMMAND(handle_create_context) {
    std::unique_ptr<Context> *ctx = helper.pop<std::unique_ptr<Context> *>();
    bool result = false;

    switch (renderer.current_backend) {
    case Backend::OpenGL: {
        result = gl::create(*ctx);
        break;
    }

    default: {
        REPORT_MISSING(renderer.current_backend);
        break;
    }
    }

    renderer.complete(command, result);
}

COMMAND(handle_create_render_target) {
    std::unique_ptr<RenderTarget> *render_target = command.pop<std::unique_ptr<RenderTarget> *>();
    SceGxmRenderTargetParams *params = command.pop<SceGxmRenderTargetParams *>();

    *render_target = renderer.create_render_target(params);

    renderer.complete(command, true);
}

COMMAND(handle_destroy_render_target) {
    std::unique_ptr<RenderTarget> *render_target = command.pop<std::unique_ptr<RenderTarget> *>();
    render_target->reset();

    renderer.complete(command, 0);
}


std::unique_ptr<Renderer> init(WindowPtr &window, Backend backend) {
    switch (backend) {
    case Backend::OpenGL:
        return std::make_unique<gl::GLRenderer>(window);
#ifdef USE_VULKAN
    case Backend::Vulkan:
        return std::make_unique<vulkan::VulkanRenderer>();
#endif
    default:
        LOG_ERROR("Cannot create a renderer with unsupported backend {}.", static_cast<int>(backend));
        return nullptr;
    }
}
} // namespace renderer
