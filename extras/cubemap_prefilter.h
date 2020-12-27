#pragma once

#include <vk.h>
#include <ogl.h>

namespace dw
{
class CubemapPrefiler
{
public:
    CubemapPrefiler(
#if defined(DWSF_VULKAN)
        vk::Backend::Ptr backend,
        vk::Image::Ptr   cubemap
#else
        gl::TextureCube::Ptr cubemap
#endif
    );
    ~CubemapPrefiler();

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
    inline gl::TextureCube::Ptr texture() { return m_texture; }
#endif

    void set_sample_count(const uint32_t& count);
    uint32_t sample_count();

private:
    void precompute_prefilter_constants();

private:
    int m_sample_count = 32;
    uint32_t m_size;
#if defined(DWSF_VULKAN)
    vk::ImageView::Ptr                  m_cubemap_image_view;
    vk::Image::Ptr                      m_image;
    vk::ImageView::Ptr                  m_image_view;
    std::vector<vk::ImageView::Ptr>     m_mip_image_views;
    vk::ComputePipeline::Ptr            m_pipeline;
    vk::PipelineLayout::Ptr             m_pipeline_layout;
    vk::DescriptorSetLayout::Ptr        m_ds_layout;
    std::vector<vk::Buffer::Ptr>        m_sample_directions_ubos;
    std::vector<vk::DescriptorSet::Ptr> m_ds;
#else
    gl::TextureCube::Ptr                m_cubemap;
    gl::TextureCube::Ptr                m_texture;
    gl::Shader::Ptr                     m_cs;
    gl::Program::Ptr                    m_program;
    std::vector<gl::UniformBuffer::Ptr> m_sample_directions_ubos;
#endif
};
} // namespace dw