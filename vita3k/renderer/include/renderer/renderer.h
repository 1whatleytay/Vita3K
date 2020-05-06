#pragma once

#include <renderer/types.h>
#include <renderer/commands.h>

#include <config/state.h>

#include <features/state.h>

#include <threads/queue.h>

namespace renderer {
struct Context;

struct Renderer {
    Backend current_backend;
    FeatureState features;

    GXPPtrMap gxp_ptr_map;
    Queue<CommandList> command_buffer_queue;
    std::condition_variable command_finish_one;
    std::mutex command_finish_one_mutex;

    uint32_t scene_processed_since_last_frame = 0;
    uint32_t average_scene_per_frame = 1;

    virtual std::unique_ptr<Context> create_context() = 0;
    virtual std::unique_ptr<RenderTarget> create_render_target(const SceGxmRenderTargetParams *params) = 0;

    virtual std::unique_ptr<VertexProgram> create_vertex_program(const SceGxmProgram *program, GXPPtrMap &map, const char *base_path, const char *title_id) = 0;
    virtual std::unique_ptr<VertexProgram> create_fragment_program(const SceGxmProgram *program, GXPPtrMap &map, const char *base_path, const char *title_id, const SceGxmBlendInfo *blend_info) = 0;

    virtual void complete(Command &command, int code);
    virtual int wait_for_status(int *result_code);

//    virtual void destroy(RenderTarget *render_target) = 0;

    explicit Renderer(Backend current_backend);
    virtual ~Renderer() = default;
};

struct Context {
    Renderer *parent;

    const RenderTarget *current_render_target{};
    GxmContextState *state{};
    CommandList command_list;
    int render_finish_status = 0;

    template <typename... Args>
    int send_single_command(const CommandOpcode opcode, Args... arguments) {
        // Make a temporary command list
        int status = CommandErrorCodePending; // Pending.

        CommandList list;
        list.commands.push(Command(opcode, &status, arguments...));

        // Submit it
        submit(list);
        return parent->wait_for_status(&status);
    }

    // Doesn't need to be virtual
    virtual void process_batch(MemState &mem, Config &config, CommandList &command_list, const char *base_path, const char *title_id);
    virtual void process_batches(MemState &mem, Config &config, const char *base_path, const char *title_id);

    virtual void submit(CommandList &command_list);
    virtual void finish();

    // Needs to be implemented
    virtual bool sync_state(const MemState &mem) = 0;
    virtual void set_uniforms(const SceGxmProgram &program, const UniformBuffers &buffers, const MemState &mem) = 0;
    virtual void set_uniform_buffers(const SceGxmProgram &program, const UniformBuffers &buffers, const UniformBufferSizes &sizes, const MemState &mem) = 0;
    virtual void set_vertex_data(const StreamDatas &datas) = 0;

    virtual void set_depth_bias(bool is_front, int factor, int units) = 0;
    virtual void set_depth_func(bool is_front, SceGxmDepthFunc depth_func) = 0;
    virtual void set_depth_write_enable_mode(bool is_front, SceGxmDepthWriteMode enable) = 0;
    virtual void set_point_line_width(bool is_front, unsigned int width) = 0;
    virtual void set_polygon_mode(bool is_front, SceGxmPolygonMode mode) = 0;
    virtual void set_stencil_func(bool is_front, SceGxmStencilFunc func, SceGxmStencilOp stencilFail, SceGxmStencilOp depthFail, SceGxmStencilOp depthPass, unsigned char compareMask, unsigned char writeMask) = 0;
    virtual void set_stencil_ref(bool is_front, unsigned char sref) = 0;
    virtual void set_program(Ptr<const void> program, bool is_fragment) = 0;
    virtual void set_cull_mode(SceGxmCullMode cull) = 0;
    virtual void set_fragment_texture(uint32_t tex_index, SceGxmTexture tex) = 0;
    virtual void set_viewport(float xOffset, float yOffset, float zOffset, float xScale, float yScale, float zScale) = 0;
    virtual void set_viewport_enable(SceGxmViewportMode enable) = 0;
    virtual void set_region_clip(SceGxmRegionClipMode mode, unsigned int xMin, unsigned int xMax, unsigned int yMin, unsigned int yMax) = 0;
    virtual void set_two_sided_enable(SceGxmTwoSidedMode mode) = 0;

    virtual void set_uniform(bool is_vertex_uniform, const SceGxmProgramParameter *parameter, const void *data) = 0;
    virtual void set_uniform_buffer(bool is_vertex_uniform, int block_num, uint16_t buffer_size, const void *data) = 0;
    virtual void set_context(RenderTarget *target, SceGxmColorSurface *color_surface, SceGxmDepthStencilSurface *depth_stencil_surface) = 0;
    virtual void set_vertex_stream(size_t index, size_t data_len, const void *data) = 0;
    virtual void draw(SceGxmPrimitiveType prim_type, SceGxmIndexFormat index_type, const void *index_data, uint32_t index_count) = 0;

    virtual void sync_surface_data() = 0;
//    virtual void sync_viewport(const bool hardware_flip) = 0;
//    virtual void sync_clipping(const bool hardware_flip) = 0;
//    virtual void sync_cull() = 0;
//    virtual void sync_front_depth_func() = 0;
//    virtual void sync_front_depth_write_enable() = 0;
//    virtual bool sync_depth_data() = 0;
//    virtual bool sync_stencil_data() = 0;
//    virtual void sync_stencil_func(bool is_back_stencil) = 0;
//    virtual void sync_front_polygon_mode() = 0;
//    virtual void sync_front_point_line_width() = 0;
//    virtual void sync_front_depth_bias() = 0;
//    virtual void sync_blending(const MemState &mem) = 0;
//    virtual void sync_texture(const MemState &mem, std::size_t index, bool enable_texture_cache) = 0;
//    virtual void sync_vertex_attributes(const MemState &mem) = 0;

    Context(Renderer *parent, GxmContextState *context_state);
    virtual ~Context() = default;
};
}