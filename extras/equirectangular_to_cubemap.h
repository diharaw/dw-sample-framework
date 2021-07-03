#pragma once

#include <vk.h>
#include <ogl.h>
#include <glm.hpp>

namespace dw
{
class EquirectangularToCubemap
{
public:
    EquirectangularToCubemap(
#if defined(DWSF_VULKAN)
        vk::Backend::Ptr backend,
        VkFormat         image_format
#endif 
    );
    ~EquirectangularToCubemap();

    void convert(
#if defined(DWSF_VULKAN)
        vk::Image::Ptr input_image,
        vk::Image::Ptr output_image
#else
        gl::Texture2D::Ptr input_image,
        gl::Texture2D::Ptr output_image
#endif 
    );

private:
#if defined(DWSF_VULKAN)
    std::weak_ptr<vk::Backend>        m_backend;
    vk::RenderPass::Ptr               m_cubemap_renderpass;
    vk::GraphicsPipeline::Ptr         m_cubemap_pipeline;
    vk::PipelineLayout::Ptr           m_cubemap_pipeline_layout;
    vk::DescriptorSetLayout::Ptr      m_ds_layout;
    vk::Buffer::Ptr                   m_cube_vbo;
#else
    gl::VertexBuffer::Ptr             m_vbo;
    gl::VertexArray::Ptr              m_vao;
    gl::Shader::Ptr                   m_vs;
    gl::Shader::Ptr                   m_fs;
    gl::Program::Ptr                  m_program;
#endif
    std::vector<glm::mat4>            m_view_projection_mats;
};
} // namespace dw