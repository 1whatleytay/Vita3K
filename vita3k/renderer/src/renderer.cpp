#include <renderer/renderer.h>

#include <renderer/types.h>

#include "driver_functions.h"

#include <util/log.h>

namespace renderer {
/**
     * NOTE: If your backend doesn't use command queue, you can directly call it by adding a switch case and call that
     * function.
     * 
     * The switch is reserved for backend like Vulkan, when building a command buffer directly is possible.
     */
/*
void set_depth_bias(State &state, Context *ctx, GxmContextState *gxm_context, bool is_front, int factor, int units) {
    switch (state.current_backend) {
    default:
        renderer::add_state_set_command(ctx, renderer::GXMState::DepthBias, is_front, factor, units);
        break;
    }
}

void set_depth_func(State &state, Context *ctx, GxmContextState *gxm_context, bool is_front, SceGxmDepthFunc depth_func) {
    switch (state.current_backend) {
    default:
        renderer::add_state_set_command(ctx, renderer::GXMState::DepthFunc, is_front, depth_func);
        break;
    }
}

void set_depth_write_enable_mode(State &state, Context *ctx, GxmContextState *gxm_context, bool is_front, SceGxmDepthWriteMode enable) {
    switch (state.current_backend) {
    default:
        renderer::add_state_set_command(ctx, renderer::GXMState::DepthWriteEnable, is_front, enable);
        break;
    }
}

void set_point_line_width(State &state, Context *ctx, GxmContextState *gxm_context, bool is_front, unsigned int width) {
    switch (state.current_backend) {
    default:
        renderer::add_state_set_command(ctx, renderer::GXMState::PointLineWidth, is_front, width);
        break;
    }
}

void set_polygon_mode(State &state, Context *ctx, GxmContextState *gxm_context, bool is_front, SceGxmPolygonMode mode) {
    switch (state.current_backend) {
    default:
        renderer::add_state_set_command(ctx, renderer::GXMState::PolygonMode, is_front, mode);
    }
}

void set_stencil_func(State &state, Context *ctx, GxmContextState *gxm_context, bool is_front, SceGxmStencilFunc func, SceGxmStencilOp stencilFail, SceGxmStencilOp depthFail, SceGxmStencilOp depthPass, unsigned char compareMask, unsigned char writeMask) {
    switch (state.current_backend) {
    default:
        renderer::add_state_set_command(ctx, renderer::GXMState::StencilFunc, is_front, func, stencilFail, depthFail, depthPass, compareMask, writeMask);
        break;
    }
}

void set_stencil_ref(State &state, Context *ctx, GxmContextState *gxm_context, bool is_front, unsigned char sref) {
    switch (state.current_backend) {
    default:
        renderer::add_state_set_command(ctx, renderer::GXMState::StencilRef, is_front, sref);
        break;
    }
}

void set_program(State &state, Context *ctx, GxmContextState *gxm_context, Ptr<const void> program, const bool is_fragment) {
    switch (state.current_backend) {
    default:
        renderer::add_state_set_command(ctx, renderer::GXMState::Program, program, is_fragment);
        break;
    }
}

void set_cull_mode(State &state, Context *ctx, GxmContextState *gxm_context, SceGxmCullMode cull) {
    switch (state.current_backend) {
    default:
        renderer::add_state_set_command(ctx, renderer::GXMState::CullMode, cull);
        break;
    }
}

void set_fragment_texture(State &state, Context *ctx, GxmContextState *gxm_context, const std::uint32_t tex_index, const SceGxmTexture tex) {
    switch (state.current_backend) {
    default:
        renderer::add_state_set_command(ctx, renderer::GXMState::FragmentTexture, tex_index, tex);
        break;
    }
}

void set_viewport(State &state, Context *ctx, GxmContextState *gxm_context, float xOffset, float yOffset, float zOffset, float xScale, float yScale, float zScale) {
    switch (state.current_backend) {
    default:
        renderer::add_state_set_command(ctx, renderer::GXMState::Viewport, SCE_GXM_VIEWPORT_ENABLED, true, xOffset, yOffset,
            zOffset, xScale, yScale, zScale);

        break;
    }
}

void set_viewport_enable(State &state, Context *ctx, GxmContextState *gxm_context, SceGxmViewportMode enable) {
    switch (state.current_backend) {
    default:
        renderer::add_state_set_command(ctx, renderer::GXMState::Viewport, enable, false);
        break;
    }
}

void set_region_clip(State &state, Context *ctx, GxmContextState *gxm_context, SceGxmRegionClipMode mode, unsigned int xMin, unsigned int xMax, unsigned int yMin, unsigned int yMax) {
    switch (state.current_backend) {
    default:
        renderer::add_state_set_command(ctx, renderer::GXMState::RegionClip, mode, xMin, xMax, yMin, yMax);
        break;
    }
}

void set_two_sided_enable(State &state, Context *ctx, GxmContextState *gxm_context, SceGxmTwoSidedMode mode) {
    switch (state.current_backend) {
    default:
        renderer::add_state_set_command(ctx, renderer::GXMState::TwoSided, mode);
        break;
    }
}

void set_context(State &state, Context *ctx, GxmContextState *gxm_context, RenderTarget *target, SceGxmColorSurface *color_surface, SceGxmDepthStencilSurface *depth_stencil_surface) {
    switch (state.current_backend) {
    default:
        renderer::add_command(ctx, renderer::CommandOpcode::SetContext, nullptr, target, color_surface, depth_stencil_surface);
        break;
    }
}

void set_vertex_stream(State &state, Context *ctx, GxmContextState *gxm_context, const std::size_t index, const std::size_t data_len, const void *data) {
    switch (state.current_backend) {
    default:
        renderer::add_state_set_command(ctx, renderer::GXMState::VertexStream, index, data_len, data);
        break;
    }
}

void draw(State &state, Context *ctx, GxmContextState *gxm_context, SceGxmPrimitiveType prim_type, SceGxmIndexFormat index_type, const void *index_data, const std::uint32_t index_count) {
    switch (state.current_backend) {
    default:
        renderer::add_command(ctx, renderer::CommandOpcode::Draw, nullptr, prim_type, index_type, index_data, index_count);
        break;
    }
}

void sync_surface_data(State &state, Context *ctx, GxmContextState *gxm_context) {
    switch (state.current_backend) {
    default:
        renderer::add_command(ctx, renderer::CommandOpcode::SyncSurfaceData, nullptr);
        break;
    }
}

bool create_context(State &state, std::unique_ptr<Context> &context) {
    switch (state.current_backend) {
    default:
        return renderer::send_single_command(state, nullptr, nullptr, renderer::CommandOpcode::CreateContext, &context);
    }
}
context
bool create_render_target(State &state, std::unique_ptr<RenderTarget> &rt, const SceGxmRenderTargetParams *params) {
    switch (state.current_backend) {
    default:
        return renderer::send_single_command(state, nullptr, nullptr, renderer::CommandOpcode::CreateRenderTarget, &rt, params);
    }
}

void destroy_render_target(State &state, std::unique_ptr<RenderTarget> &rt) {
    switch (state.current_backend) {
    default:
        renderer::send_single_command(state, nullptr, nullptr, renderer::CommandOpcode::DestroyRenderTarget, &rt);
        break;
    }
}

void set_uniform(State &state, Context *ctx, const bool is_vertex_uniform, const SceGxmProgramParameter *parameter, const void *data) {
    switch (state.current_backend) {
    default:
        renderer::add_state_set_command(ctx, renderer::GXMState::Uniform, is_vertex_uniform, parameter, data);
        break;
    }
}

void set_uniform_buffer(State &state, Context *ctx, const bool is_vertex_uniform, const int block_number, const std::uint16_t block_size, const void *data) {
    switch (state.current_backend) {
    default: {
        // Calculate the number of bytes
        std::uint32_t bytes_to_copy_and_pad = (((block_size + 15) / 16)) * 16;
        std::uint8_t *a_copy = new std::uint8_t[bytes_to_copy_and_pad];

        std::memcpy(a_copy, data, block_size);

        renderer::add_state_set_command(ctx, renderer::GXMState::UniformBuffer, is_vertex_uniform, block_number, bytes_to_copy_and_pad, a_copy);
        break;
    }
    }
}
*/

void Context::process_batch(MemState &mem, Config &config, CommandList &list, const char *base_path, const char *title_id) {
    static std::map<CommandOpcode, CommandHandlerFunc> handlers = {
        { CommandOpcode::SetContext, cmd_handle_set_context },
        { CommandOpcode::SyncSurfaceData, cmd_handle_sync_surface_data },
        { CommandOpcode::CreateContext, cmd_handle_create_context },
        { CommandOpcode::CreateRenderTarget, cmd_handle_create_render_target },
        { CommandOpcode::Draw, cmd_handle_draw },
        { CommandOpcode::Nop, cmd_handle_nop },
        { CommandOpcode::SetState, cmd_handle_set_state },
        { CommandOpcode::SignalSyncObject, cmd_handle_signal_sync_object },
        { CommandOpcode::DestroyRenderTarget, cmd_handle_destroy_render_target }
    };

    Command *cmd = list.first;

    // Take a batch, and execute it. Hope it's not too large
    while (true) {
        if (cmd == nullptr)
            break;

        auto handler = handlers.find(cmd->opcode);
        if (handler == handlers.end()) {
            LOG_ERROR("Unimplemented command opcode {}", static_cast<int>(cmd->opcode));
        } else {
            handler->second(*parent, mem, config, *cmd, parent->features, list.context,
                list.gxm_context, base_path, title_id);
        }

        Command *last_cmd = cmd;
        cmd = cmd->next;

        delete last_cmd;
    }
}

void Context::process_batches(MemState &mem, Config &config, const char *base_path, const char *title_id) {
    std::uint32_t processed_count = 0;

    while (processed_count < parent->average_scene_per_frame) {
        auto cmd_list = parent->command_buffer_queue.pop(2);

        if (!cmd_list) {
            // Try to wait for a batch (about 1 or 2ms, game should be fast for this)
            return;
        }

        process_batch(mem, config, *cmd_list, base_path, title_id);
        processed_count++;
    }
}

void Context::submit(CommandList &list) {
    list.context = this;
    list.gxm_context = state;
    parent->command_buffer_queue.push(list);
}

void Context::finish() {
    parent->wait_for_status(&render_finish_status);
}

Context::Context(Renderer *parent, GxmContextState *context_state) : parent(parent), state(context_state) { }

void Renderer::complete(Command &command, int code) {
    command.complete(code);
    command_finish_one.notify_all();
}

int Renderer::wait_for_status(int *result_code) {
    if (*result_code != CommandErrorCodePending) {
        // Signaled, return
        return *result_code;
    }

    // Wait for it to get signaled
    std::unique_lock<std::mutex> finish_mutex(command_finish_one_mutex);
    command_finish_one.wait(finish_mutex, [&]() { return *result_code != CommandErrorCodePending; });

    return *result_code;
}

Renderer::Renderer(Backend current_backend) : current_backend(current_backend) {
    command_buffer_queue.maxPendingCount_ = 30;
}
} // namespace renderer