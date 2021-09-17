#include "brdf_preintegrate_lut.h"
#include <macros.h>
#include <vk_mem_alloc.h>
#include <glm.hpp>
#include <logger.h>
#include <fstream>

#define BRDF_LUT_SIZE 512
#define BRDF_WORK_GROUP_SIZE 8

namespace dw
{
// -----------------------------------------------------------------------------------------------------------------------------------

BRDFIntegrateLUT::BRDFIntegrateLUT(
#if defined(DWSF_VULKAN)
    vk::Backend::Ptr backend
#endif
)
{
    size_t                size = BRDF_LUT_SIZE * BRDF_LUT_SIZE * sizeof(uint16_t) * 2;
    std::vector<uint16_t> buffer(size);

    std::fstream f("textures/brdf_lut.bin", std::ios::in | std::ios::binary);

    f.seekp(0);
    f.read((char*)buffer.data(), size);
    f.close();

#if defined(DWSF_VULKAN)
    m_image = vk::Image::create(backend, VK_IMAGE_TYPE_2D, BRDF_LUT_SIZE, BRDF_LUT_SIZE, 1, 1, 1, VK_FORMAT_R16G16_SFLOAT, VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED, size, buffer.data());
    m_image->set_name("BRDF LUT");

    m_image_view = vk::ImageView::create(backend, m_image, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1);
    m_image_view->set_name("BRDF LUT");
#else
    m_texture = gl::Texture2D::create(BRDF_LUT_SIZE, BRDF_LUT_SIZE, 1, 1, 1, GL_RG16F, GL_RG, GL_HALF_FLOAT);
    m_texture->set_name("BRDF LUT");

    m_texture->set_min_filter(GL_NEAREST);
    m_texture->set_mag_filter(GL_NEAREST);

    m_texture->set_data(0, 0, (char*)buffer.data());
#endif
}

// -----------------------------------------------------------------------------------------------------------------------------------

BRDFIntegrateLUT::~BRDFIntegrateLUT()
{
#if defined(DWSF_VULKAN)
    m_image_view.reset();
    m_image.reset();
#else
    m_texture.reset();
#endif
}

// -----------------------------------------------------------------------------------------------------------------------------------
} // namespace dw