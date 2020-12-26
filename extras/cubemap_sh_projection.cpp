#include "cubemap_sh_projection.h"
#include <macros.h>
#include <vk_mem_alloc.h>
#include <glm.hpp>

#define IRRADIANCE_CUBEMAP_SIZE 128
#define IRRADIANCE_WORK_GROUP_SIZE 8
#define SH_INTERMEDIATE_SIZE (IRRADIANCE_CUBEMAP_SIZE / IRRADIANCE_WORK_GROUP_SIZE)

namespace dw
{
// -----------------------------------------------------------------------------------------------------------------------------------

CubemapSHProjection::CubemapSHProjection(
#if defined(DWSF_VULKAN)
    vk::Backend::Ptr backend
#endif
)
{
#if defined(DWSF_VULKAN)
    vk::DescriptorSetLayout::Desc ds_layout_desc;

    ds_layout_desc.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT);

    m_ds_layout = vk::DescriptorSetLayout::create(backend, ds_layout_desc);

    m_ds = backend->allocate_descriptor_set(m_ds_layout);

    m_image = vk::Image::create(backend, VK_IMAGE_TYPE_2D, 9, 1, 1, 1, 1, VK_FORMAT_R32G32B32A32_SFLOAT, VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED);
    m_image->set_name("SH Projection");

    m_image_view = vk::ImageView::create(backend, m_image, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1);
    m_image_view->set_name("SH Projection");

    m_intermediate_image = vk::Image::create(backend, VK_IMAGE_TYPE_2D, SH_INTERMEDIATE_SIZE * 9, SH_INTERMEDIATE_SIZE, 1, 1, 6, VK_FORMAT_R32G32B32A32_SFLOAT, VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED);
    m_intermediate_image->set_name("SH Projection Intermediate");

    m_intermediate_image_view = vk::ImageView::create(backend, m_image, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 6);
    m_intermediate_image_view->set_name("SH Projection Intermediate");

    VkWriteDescriptorSet write_data;
    DW_ZERO_MEMORY(write_data);

    VkDescriptorImageInfo output_image;

    output_image.sampler     = VK_NULL_HANDLE;
    output_image.imageView   = m_image_view->handle();
    output_image.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    write_data.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_data.descriptorCount = 1;
    write_data.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write_data.pImageInfo      = &output_image;
    write_data.dstBinding      = 0;
    write_data.dstSet          = m_ds->handle();

    vkUpdateDescriptorSets(backend->device(), 1, &write_data, 0, nullptr);

    vk::PipelineLayout::Desc desc;

    desc.add_descriptor_set_layout(m_ds_layout);

    m_pipeline_layout = dw::vk::PipelineLayout::create(backend, desc);

    vk::ShaderModule::Ptr module = dw::vk::ShaderModule::create_from_file(backend, "shaders/brdf_preintegrate_lut.comp.spv");

    vk::ComputePipeline::Desc comp_desc;

    comp_desc.set_pipeline_layout(m_pipeline_layout);
    comp_desc.set_shader_stage(module, "main");

    m_pipeline = dw::vk::ComputePipeline::create(backend, comp_desc);

    dw::vk::CommandBuffer::Ptr cmd_buf = backend->allocate_graphics_command_buffer(true);

    vkCmdBindPipeline(cmd_buf->handle(), VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline->handle());

    vkCmdBindDescriptorSets(cmd_buf->handle(), VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline_layout->handle(), 0, 1, &m_ds->handle(), 0, nullptr);

    vkCmdDispatch(cmd_buf->handle(), BRDF_LUT_SIZE / BRDF_WORK_GROUP_SIZE, BRDF_LUT_SIZE / BRDF_WORK_GROUP_SIZE, 1);

    vkEndCommandBuffer(cmd_buf->handle());

    backend->flush_graphics({ cmd_buf });

#else
    m_texture = gl::Texture2D::create(BRDF_LUT_SIZE, BRDF_LUT_SIZE, 1, 1, 1, GL_RG16F, GL_RG, GL_HALF_FLOAT);
    m_texture->set_name("BRDF LUT");

    m_texture->set_min_filter(GL_NEAREST);
    m_texture->set_mag_filter(GL_NEAREST);

    // Create general shaders
    m_shader = gl::Shader::create_from_file(GL_COMPUTE_SHADER, "shader/brdf_cs.glsl");

    if (!m_shader->compiled())
    {
        DW_LOG_FATAL("Failed to create Shaders");
    }

    // Create general shader program
    m_program = gl::Program::create({ m_brdf_cs });

    if (!m_program)
    {
        DW_LOG_FATAL("Failed to create Shader Program");
    }

    m_program->use();

    m_texture->bind_image(0, 0, 0, GL_WRITE_ONLY, GL_RG16F);

    glDispatchCompute(BRDF_LUT_SIZE / BRDF_WORK_GROUP_SIZE, BRDF_LUT_SIZE / BRDF_WORK_GROUP_SIZE, 1);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glFinish();
#endif
}

// -----------------------------------------------------------------------------------------------------------------------------------

CubemapSHProjection::~CubemapSHProjection()
{
#if defined(DWSF_VULKAN)
    m_pipeline.reset();
    m_pipeline_layout.reset();
    m_ds.reset();
    m_ds_layout.reset();
    m_image_view.reset();
    m_image.reset();
#else
    m_program.reset();
    m_shader.reset();
    m_texture.reset();
#endif
}

// -----------------------------------------------------------------------------------------------------------------------------------
} // namespace dw