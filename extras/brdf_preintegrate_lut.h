#pragma once

#include <vk.h>
#include <ogl.h>

namespace dw
{
class BRDFIntegrateLUT
{
public:
    BRDFIntegrateLUT(
#if defined(DWSF_VULKAN)
        vk::Backend::Ptr backend
#endif 
    );
    ~BRDFIntegrateLUT();

#if defined(DWSF_VULKAN)
    inline vk::Image::Ptr image() { return m_image; }
    inline vk::ImageView::Ptr image_view() { return m_image_view; }
#else
    inline gl::Texture2D::Ptr texture() { return m_texture; }
#endif

private:
#if defined(DWSF_VULKAN)
    vk::Image::Ptr               m_image;
    vk::ImageView::Ptr           m_image_view;
    vk::ComputePipeline::Ptr     m_pipeline;
    vk::PipelineLayout::Ptr      m_pipeline_layout;
    vk::DescriptorSetLayout::Ptr m_ds_layout;
    vk::DescriptorSet::Ptr       m_ds;
#else
    gl::Texture2D::Ptr m_texture;
    gl::Shader::Ptr    m_shader;
    gl::Program::Ptr   m_program;
#endif
};
} // namespace dw