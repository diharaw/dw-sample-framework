#pragma once

#include <vk.h>
#include <ogl.h>
#include <glm.hpp>

namespace dw
{
// An Analytic Model for Full Spectral Sky-Dome Radiance (Lukas Hosek, Alexander Wilkie)
class HosekWilkieSkyModel
{
public:
    HosekWilkieSkyModel(
#if defined(DWSF_VULKAN)
        vk::Backend::Ptr backend
#endif
    );
    ~HosekWilkieSkyModel();

    void update(
#if defined(DWSF_VULKAN)
        vk::CommandBuffer::Ptr cmd_buf,
#endif
        glm::vec3 direction);

    void render(
#if defined(DWSF_VULKAN)
        vk::CommandBuffer::Ptr cmd_buf,
#endif
        uint32_t  width,
        uint32_t  height,
        glm::mat4 view_mat,
        glm::mat4 projection_mat);

#if defined(DWSF_VULKAN)
    inline vk::Image::Ptr image()
    {
        return m_cubemap_image;
    }
    inline vk::ImageView::Ptr image_view() { return m_cubemap_image_view; }
    inline vk::Buffer::Ptr    cube_vbo() { return m_cube_vbo; }
#else
    inline gl::TextureCube::Ptr texture()
    {
        return m_cubemap;
    }
    inline gl::VertexArray::Ptr           cube_vao() { return m_vao; }
#endif

private:
#if defined(DWSF_VULKAN)
    vk::Image::Ptr                    m_cubemap_image;
    vk::ImageView::Ptr                m_cubemap_image_view;
    std::vector<vk::ImageView::Ptr>   m_face_image_views;
    std::vector<vk::Framebuffer::Ptr> m_face_framebuffers;
    vk::RenderPass::Ptr               m_cubemap_renderpass;
    vk::GraphicsPipeline::Ptr         m_cubemap_pipeline;
    vk::PipelineLayout::Ptr           m_cubemap_pipeline_layout;
    vk::Buffer::Ptr                   m_cube_vbo;
    vk::DescriptorSetLayout::Ptr      m_ds_layout;
    vk::DescriptorSet::Ptr            m_ds;
    vk::Buffer::Ptr                   m_ubo;

#else
    gl::TextureCube::Ptr                  m_cubemap;
    std::vector<gl::TextureCubeView::Ptr> m_texture_views;
    gl::Texture2D::Ptr                    m_depth;
    std::vector<gl::Framebuffer::Ptr>     m_fbos;
    gl::Buffer::Ptr                       m_vbo;
    gl::VertexArray::Ptr                  m_vao;
    gl::Shader::Ptr                       m_update_vs;
    gl::Shader::Ptr                       m_update_fs;
    gl::Program::Ptr                      m_update_program;
    gl::Shader::Ptr                       m_render_vs;
    gl::Shader::Ptr                       m_render_fs;
    gl::Program::Ptr                      m_render_program;
#endif
    std::vector<glm::mat4> m_view_projection_mats;
    float                  m_normalized_sun_y = 1.15f;
    float                  m_albedo           = 0.1f;
    float                  m_turbidity        = 4.0f;
    glm::vec3              A, B, C, D, E, F, G, H, I;
    glm::vec3              Z;
};
} // namespace dw