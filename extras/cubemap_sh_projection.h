#pragma once

#include <vk.h>
#include <ogl.h>

namespace dw
{
class CubemapSHProjection
{
public:
    CubemapSHProjection(
#if defined(DWSF_VULKAN)
        vk::Backend::Ptr backend,
        vk::Image::Ptr cubemap
#else
        gl::TextureCube::Ptr cubemap
#endif
    );
    ~CubemapSHProjection();

    void update(
#if defined(DWSF_VULKAN)
        vk::CommandBuffer::Ptr cmd_buf
#endif
    );

#if defined(DWSF_VULKAN)
    inline vk::Image::Ptr image()
    {
        return m_image;
    }
    inline vk::ImageView::Ptr image_view() { return m_image_view; }
#else
    inline gl::Texture2D::Ptr texture() { return m_texture; }
#endif

private:
#if defined(DWSF_VULKAN)
    vk::ImageView::Ptr           m_cubemap_image_view;
    vk::Image::Ptr               m_image;
    vk::ImageView::Ptr           m_image_view;
    vk::Image::Ptr               m_intermediate_image;
    vk::ImageView::Ptr           m_intermediate_image_view;
    vk::ComputePipeline::Ptr     m_projection_pipeline;
    vk::ComputePipeline::Ptr     m_add_pipeline;
    vk::PipelineLayout::Ptr      m_projection_pipeline_layout;
    vk::PipelineLayout::Ptr      m_add_pipeline_layout;
    vk::DescriptorSetLayout::Ptr m_ds_layout;
    vk::DescriptorSet::Ptr       m_projection_ds;
    vk::DescriptorSet::Ptr       m_add_ds;
    float                        m_size;
#else
    gl::TextureCube::Ptr m_cubemap;
    gl::Texture2D::Ptr   m_texture;
    gl::Texture2D::Ptr   m_texture_intermediate;
    gl::Shader::Ptr      m_projection_cs;
    gl::Program::Ptr     m_projection_program;
    gl::Shader::Ptr      m_add_cs;
    gl::Program::Ptr     m_add_program;
#endif
};
} // namespace dw