#pragma once

#include <renderer/renderer.h>

#include <renderer/gl/types.h>

namespace renderer::gl {
struct GLContext : public renderer::Context {
    GLTextureCacheState texture_cache;
    GLObjectArray<1> vertex_array;
    GLObjectArray<1> element_buffer;
    GLObjectArray<30> uniform_buffer;
    GLObjectArray<SCE_GXM_MAX_VERTEX_STREAMS> stream_vertex_buffers;
    GLuint last_draw_program{ 0 };

    float viewport_flip[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    std::vector<UniformSetRequest> vertex_set_requests;
    std::vector<UniformSetRequest> fragment_set_requests;

    void set_uniforms(const SceGxmProgram &program, const UniformBuffers &buffers, const MemState &mem) override;
    void set_uniform_buffers(const SceGxmProgram &program, const UniformBuffers &buffers, const UniformBufferSizes &sizes, const MemState &mem) override;
    void set_vertex_data(const StreamDatas &datas) override;
    void set_depth_bias(bool is_front, int factor, int units) override;
    void set_depth_func(bool is_front, SceGxmDepthFunc depth_func) override;
    void set_depth_write_enable_mode(bool is_front, SceGxmDepthWriteMode enable) override;
    void set_point_line_width(bool is_front, unsigned int width) override;
    void set_polygon_mode(bool is_front, SceGxmPolygonMode mode) override;
    void set_stencil_func(bool is_front, SceGxmStencilFunc func, SceGxmStencilOp stencilFail, SceGxmStencilOp depthFail, SceGxmStencilOp depthPass, unsigned char compareMask, unsigned char writeMask) override;
    void set_stencil_ref(bool is_front, unsigned char sref) override;
    void set_program(Ptr<const void> program, const bool is_fragment) override;
    void set_cull_mode(SceGxmCullMode cull) override;
    void set_fragment_texture(const std::uint32_t tex_index, const SceGxmTexture tex) override;
    void set_viewport(float xOffset, float yOffset, float zOffset, float xScale, float yScale, float zScale) override;
    void set_viewport_enable(SceGxmViewportMode enable) override;
    void set_region_clip(SceGxmRegionClipMode mode, unsigned int xMin, unsigned int xMax, unsigned int yMin, unsigned int yMax) override;
    void set_two_sided_enable(SceGxmTwoSidedMode mode) override;
    void set_uniform(const bool is_vertex_uniform, const SceGxmProgramParameter *parameter, const void *data) override;
    void set_uniform_buffer(const bool is_vertex_uniform, const int block_num, const std::uint16_t buffer_size, const void *data) override;
    void set_context(RenderTarget *target, SceGxmColorSurface *color_surface, SceGxmDepthStencilSurface *depth_stencil_surface) override;
    void set_vertex_stream(const std::size_t index, const std::size_t data_len, const void *data) override;
    void draw(SceGxmPrimitiveType prim_type, SceGxmIndexFormat index_type, const void *index_data, const std::uint32_t index_count) override;

    void sync_surface_data() = 0;

    GLContext(Renderer *renderer, GxmContextState *context_state);
};

struct GLRenderer : public renderer::Renderer {
    GLContextPtr gl_context;

    ShaderCache fragment_shader_cache;
    ShaderCache vertex_shader_cache;
    ProgramCache program_cache;

    std::unique_ptr<Context> create_context() override;
    std::unique_ptr<RenderTarget> create_render_target(const SceGxmRenderTargetParams *params) override;
    std::unique_ptr<VertexProgram> create_vertex_program(const SceGxmProgram *program, GXPPtrMap &map, const char *base_path, const char *title_id) override;
    std::unique_ptr<VertexProgram> create_fragment_program(const SceGxmProgram *program, GXPPtrMap &map, const char *base_path, const char *title_id, const SceGxmBlendInfo *blend_info) override;

    explicit GLRenderer(const WindowPtr &window);
};
}