#pragma once

#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <macros.h>
#include <vector>
#include <memory>
#include <ogl.h>
#include <vk.h>

// Hard-limit of vertices. Used to reserve space in vertex and draw command vectors.
#define MAX_VERTICES 100000

namespace dw
{
struct CameraUniforms
{
    DW_ALIGNED(16)
    glm::mat4 view_proj;
};

struct VertexWorld
{
    glm::vec3 position;
    glm::vec2 uv;
    glm::vec3 color;
};

struct DrawCommand
{
    int   type;
    int   vertices;
    bool  depth_test;
    bool  distance_fade;
    float fade_start;
    float fade_end;
};

const glm::vec4 kFrustumCorners[] = {
    glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),  // Far-Bottom-Left
    glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f),   // Far-Top-Left
    glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),    // Far-Top-Right
    glm::vec4(1.0f, -1.0f, 1.0f, 1.0f),   // Far-Bottom-Right
    glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f), // Near-Bottom-Left
    glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f),  // Near-Top-Left
    glm::vec4(1.0f, 1.0f, -1.0f, 1.0f),   // Near-Top-Right
    glm::vec4(1.0f, -1.0f, -1.0f, 1.0f)   // Near-Bottom-Right
};

class DebugDraw
{
public:
    DebugDraw();

    // Initialization and shutdown.
    bool init(
#if defined(DWSF_VULKAN)
        vk::Backend::Ptr    backend,
        vk::RenderPass::Ptr render_pass
#endif
    );
    void shutdown();

    inline void set_depth_test(const bool& depth_test) { m_depth_test = depth_test; }
    inline bool depth_test() { return m_depth_test; }

    inline void set_distance_fade(const bool& fade) { m_distance_fade = fade; }
    inline bool distance_fade() { return m_distance_fade; }

    inline void set_fade_start(const float& fade) { m_fade_start = fade; }
    inline bool fade_start() { return m_fade_start; }

    inline void set_fade_end(const float& fade) { m_fade_end = fade; }
    inline bool fade_end() { return m_fade_end; }

    // Debug shape drawing.
    void begin_batch();
    void end_batch();
    void capsule(const float& _height, const float& _radius, const glm::vec3& _pos, const glm::vec3& _c);
    void aabb(const glm::vec3& _min, const glm::vec3& _max, const glm::vec3& _c);
    void obb(const glm::vec3& _min, const glm::vec3& _max, const glm::mat4& _model, const glm::vec3& _c);
    void grid(const float& _x, const float& _z, const float& _y_level, const float& spacing, const glm::vec3& _c);
    void grid(const glm::mat4& view_proj, const float& unit_size, const float& highlight_unit_size);
    void line(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& c);
    void line_strip(glm::vec3* v, const int& count, const glm::vec3& c);
    void circle_xy(float radius, const glm::vec3& pos, const glm::vec3& c);
    void circle_xz(float radius, const glm::vec3& pos, const glm::vec3& c);
    void circle_yz(float radius, const glm::vec3& pos, const glm::vec3& c);
    void sphere(const float& radius, const glm::vec3& pos, const glm::vec3& c);
    void frustum(const glm::mat4& view_proj, const glm::vec3& c);
    void frustum(const glm::mat4& proj, const glm::mat4& view, const glm::vec3& c);
    void transform(const glm::mat4& trans, const float& axis_length = 5.0f);

    // Render method. Pass in target Framebuffer, viewport size and view-projection matrix.
#if defined(DWSF_VULKAN)
    void render(vk::Backend::Ptr backend, vk::CommandBuffer::Ptr cmd_buffer, int width, int height, const glm::mat4& view_proj, const glm::vec3& view_pos);
#else
    void                               render(gl::Framebuffer* fbo, int width, int height, const glm::mat4& view_proj, const glm::vec3& view_pos);
#endif

#if defined(DWSF_VULKAN)
private:
    void create_descriptor_set_layout(vk::Backend::Ptr backend);
    void create_descriptor_set(vk::Backend::Ptr backend);
    void create_uniform_buffer(vk::Backend::Ptr backend);
    void create_vertex_buffer(vk::Backend::Ptr backend);
    void create_pipeline_states(vk::Backend::Ptr backend, vk::RenderPass::Ptr render_pass);
#endif

private:
    // Vertex list to be uploaded to GPU.
    std::vector<VertexWorld> m_world_vertices;

    // Draw command list.
    std::vector<DrawCommand> m_draw_commands;

    // Camera matrix.
    CameraUniforms m_uniforms;

    // Depth state
    bool m_depth_test = false;

    // Batching
    uint32_t m_batch_start  = 0;
    uint32_t m_batch_end    = 0;
    bool     m_batched_mode = false;

    // Fading
    float m_fade_end      = 0.0f;
    float m_fade_start    = 0.0f;
    bool  m_distance_fade = false;

    // GPU resources.
#if defined(DWSF_VULKAN)
    size_t                       m_ubo_size;
    size_t                       m_vbo_size;
    vk::Buffer::Ptr              m_line_vbo;
    vk::Buffer::Ptr              m_ubo;
    vk::PipelineLayout::Ptr      m_pipeline_layout;
    vk::DescriptorSetLayout::Ptr m_ds_layout;
    vk::DescriptorSet::Ptr       m_ds;
    vk::GraphicsPipeline::Ptr    m_line_depth_pipeline;
    vk::GraphicsPipeline::Ptr    m_line_no_depth_pipeline;
    vk::GraphicsPipeline::Ptr    m_line_strip_depth_pipeline;
    vk::GraphicsPipeline::Ptr    m_line_strip_no_depth_pipeline;
#else
    std::unique_ptr<gl::VertexArray>   m_line_vao;
    std::unique_ptr<gl::VertexBuffer>  m_line_vbo;
    std::unique_ptr<gl::Shader>        m_line_vs;
    std::unique_ptr<gl::Shader>        m_line_fs;
    std::unique_ptr<gl::Program>       m_line_program;
    std::unique_ptr<gl::UniformBuffer> m_ubo;
#endif
};
} // namespace dw