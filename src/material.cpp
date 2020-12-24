#include <logger.h>
#include <macros.h>
#include <material.h>
#include <utility.h>
#include <assimp/scene.h>
#if defined(DWSF_VULKAN)
#    include <vk_mem_alloc.h>
#endif

namespace dw
{
std::unordered_map<std::string, std::weak_ptr<Material>> Material::m_cache;

#if defined(DWSF_VULKAN)
std::unordered_map<std::string, std::weak_ptr<vk::Image>>     Material::m_image_cache;
std::unordered_map<std::string, std::weak_ptr<vk::ImageView>> Material::m_image_view_cache;
vk::DescriptorSetLayout::Ptr                                  Material::m_common_ds_layout;
vk::Sampler::Ptr                                              Material::m_common_sampler;
vk::Image::Ptr                                                Material::m_default_image;
vk::ImageView::Ptr                                            Material::m_default_image_view;
#else
std::unordered_map<std::string, gl::Texture2D*> Material::m_texture_cache;
#endif

static uint32_t g_last_mat_idx = 0;

#if defined(DWSF_VULKAN)

// -----------------------------------------------------------------------------------------------------------------------------------

Material::Ptr Material::load(vk::Backend::Ptr backend, const std::vector<std::string>& textures, const int32_t& albedo_idx, const int32_t& normal_idx, const glm::ivec2& roughness_idx, const glm::ivec2& metallic_idx, const int32_t& emissive_idx)
{
    std::string mat_id;

    for (auto path : textures)
        mat_id += path;

    if (m_cache.find(mat_id) == m_cache.end() || m_cache[mat_id].expired())
    {
        Material::Ptr mat = std::shared_ptr<Material>(new Material(backend, textures, albedo_idx, normal_idx, roughness_idx, metallic_idx, emissive_idx));
        m_cache[mat_id]   = mat;
        return mat;
    }
    else
        return m_cache[mat_id].lock();
}

// -----------------------------------------------------------------------------------------------------------------------------------

Material::Ptr Material::create(vk::Backend::Ptr backend, glm::vec4 albedo, float roughness, float metallic, glm::vec3 emissive)
{
    Material::Ptr mat = std::shared_ptr<Material>(new Material());

    mat->m_albedo_color   = albedo;
    mat->m_roughness      = roughness;
    mat->m_metallic       = metallic;
    mat->m_emissive_color = emissive;

    return mat;
}

// -----------------------------------------------------------------------------------------------------------------------------------

Material::Material()
{
    m_id = g_last_mat_idx++;
}

// -----------------------------------------------------------------------------------------------------------------------------------

Material::Material(vk::Backend::Ptr backend, const std::vector<std::string>& textures, const int32_t& albedo_idx, const int32_t& normal_idx, const glm::ivec2& roughness_idx, const glm::ivec2& metallic_idx, const int32_t& emissive_idx) :
    m_albedo_idx(albedo_idx), m_normal_idx(normal_idx), m_roughness_idx(roughness_idx), m_metallic_idx(metallic_idx), m_emissive_idx(emissive_idx)
{
    m_id = g_last_mat_idx++;

    if (albedo_idx != -1 && textures[albedo_idx].size() > 0)
    {
        auto image = load_image(backend, textures[albedo_idx], true);

        m_images.push_back(image);

        if (image)
        {
            auto image_view = load_image_view(backend, textures[albedo_idx], image);
            m_image_views.push_back(image_view);
        }
    }

    if (normal_idx != -1 && textures[normal_idx].size() > 0)
    {
        auto image = load_image(backend, textures[normal_idx], true);

        m_images.push_back(image);

        if (image)
        {
            auto image_view = load_image_view(backend, textures[normal_idx], image);
            m_image_views.push_back(image_view);
        }
    }

    if (roughness_idx.x != -1 && textures[roughness_idx.x].size() > 0)
    {
        auto image = load_image(backend, textures[roughness_idx.x], true);

        m_images.push_back(image);

        if (image)
        {
            auto image_view = load_image_view(backend, textures[roughness_idx.x], image);
            m_image_views.push_back(image_view);
        }
    }

    if (metallic_idx.x != -1 && textures[metallic_idx.x].size() > 0)
    {
        auto image = load_image(backend, textures[metallic_idx.x], true);

        m_images.push_back(image);

        if (image)
        {
            auto image_view = load_image_view(backend, textures[metallic_idx.x], image);
            m_image_views.push_back(image_view);
        }
    }

    if (emissive_idx != -1 && textures[emissive_idx].size() > 0)
    {
        auto image = load_image(backend, textures[emissive_idx], true);

        m_images.push_back(image);

        if (image)
        {
            auto image_view = load_image_view(backend, textures[emissive_idx], image);
            m_image_views.push_back(image_view);
        }
    }

    // Create descriptor set
    m_descriptor_set = create_descriptor_set(backend);
}

// -----------------------------------------------------------------------------------------------------------------------------------

Material::~Material()
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Material::initialize_common_resources(vk::Backend::Ptr backend)
{
    vk::DescriptorSetLayout::Desc ds_layout_desc;

    ds_layout_desc.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    ds_layout_desc.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    ds_layout_desc.add_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    ds_layout_desc.add_binding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    ds_layout_desc.add_binding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);

    m_common_ds_layout = vk::DescriptorSetLayout::create(backend, ds_layout_desc);

    vk::Sampler::Desc sampler_desc;

    sampler_desc.mag_filter        = VK_FILTER_LINEAR;
    sampler_desc.min_filter        = VK_FILTER_LINEAR;
    sampler_desc.mipmap_mode       = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_desc.address_mode_u    = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_desc.address_mode_v    = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_desc.address_mode_w    = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_desc.mip_lod_bias      = 0.0f;
    sampler_desc.anisotropy_enable = VK_FALSE;
    sampler_desc.max_anisotropy    = 1.0f;
    sampler_desc.compare_enable    = false;
    sampler_desc.compare_op        = VK_COMPARE_OP_NEVER;
    sampler_desc.min_lod           = 0.0f;
    sampler_desc.max_lod           = 12.0f;
    sampler_desc.border_color      = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler_desc.unnormalized_coordinates;

    m_common_sampler = vk::Sampler::create(backend, sampler_desc);

    uint8_t data[] = { 255, 255, 255, 255 };

    m_default_image      = vk::Image::create(backend, VK_IMAGE_TYPE_2D, 1, 1, 1, 1, 1, VK_FORMAT_R8G8B8A8_SNORM, VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED, sizeof(uint8_t) * 4, data);
    m_default_image_view = vk::ImageView::create(backend, m_default_image, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Material::shutdown_common_resources()
{
    m_default_image_view.reset();
    m_default_image.reset();
    m_common_ds_layout.reset();
    m_common_sampler.reset();
}

// -----------------------------------------------------------------------------------------------------------------------------------

vk::Image::Ptr Material::load_image(vk::Backend::Ptr backend, const std::string& path, bool srgb)
{
    if (m_image_cache.find(path) == m_image_cache.end() || m_image_cache[path].expired())
    {
        vk::Image::Ptr tex  = vk::Image::create_from_file(backend, path, false, srgb);
        m_image_cache[path] = tex;
        return tex;
    }
    else
        return m_image_cache[path].lock();
}

// -----------------------------------------------------------------------------------------------------------------------------------

vk::ImageView::Ptr Material::load_image_view(vk::Backend::Ptr backend, const std::string& path, vk::Image::Ptr image)
{
    if (m_image_view_cache.find(path) == m_image_view_cache.end() || m_image_view_cache[path].expired())
    {
        vk::ImageView::Ptr image_view = vk::ImageView::create(backend, image, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 0, image->mip_levels());
        m_image_view_cache[path]      = image_view;
        return image_view;
    }
    else
        return m_image_view_cache[path].lock();
}

// -----------------------------------------------------------------------------------------------------------------------------------

vk::DescriptorSet::Ptr Material::create_descriptor_set(vk::Backend::Ptr backend)
{
    vk::DescriptorSet::Ptr ds = backend->allocate_descriptor_set(m_common_ds_layout);

    VkDescriptorImageInfo image_info[5];

    image_info[0].sampler     = m_common_sampler->handle();
    image_info[0].imageView   = m_albedo_idx != -1 ? m_image_views[m_albedo_idx]->handle() : m_default_image_view->handle();
    image_info[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    image_info[1].sampler     = m_common_sampler->handle();
    image_info[1].imageView   = m_normal_idx != -1 ? m_image_views[m_normal_idx]->handle() : m_default_image_view->handle();
    image_info[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    image_info[2].sampler     = m_common_sampler->handle();
    image_info[2].imageView   = m_roughness_idx.x != -1 ? m_image_views[m_roughness_idx.x]->handle() : m_default_image_view->handle();
    image_info[2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    image_info[3].sampler     = m_common_sampler->handle();
    image_info[3].imageView   = m_metallic_idx.x != -1 ? m_image_views[m_metallic_idx.x]->handle() : m_default_image_view->handle();
    image_info[3].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    image_info[4].sampler     = m_common_sampler->handle();
    image_info[4].imageView   = m_emissive_idx != -1 ? m_image_views[m_emissive_idx]->handle() : m_default_image_view->handle();
    image_info[4].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write_data[5];

    DW_ZERO_MEMORY(write_data[0]);

    write_data[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_data[0].descriptorCount = 1;
    write_data[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_data[0].pImageInfo      = &image_info[0];
    write_data[0].dstBinding      = 0;
    write_data[0].dstSet          = ds->handle();

    DW_ZERO_MEMORY(write_data[1]);

    write_data[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_data[1].descriptorCount = 1;
    write_data[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_data[1].pImageInfo      = &image_info[1];
    write_data[1].dstBinding      = 1;
    write_data[1].dstSet          = ds->handle();

    DW_ZERO_MEMORY(write_data[2]);

    write_data[2].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_data[2].descriptorCount = 1;
    write_data[2].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_data[2].pImageInfo      = &image_info[2];
    write_data[2].dstBinding      = 2;
    write_data[2].dstSet          = ds->handle();

    DW_ZERO_MEMORY(write_data[3]);

    write_data[3].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_data[3].descriptorCount = 1;
    write_data[3].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_data[3].pImageInfo      = &image_info[3];
    write_data[3].dstBinding      = 3;
    write_data[3].dstSet          = ds->handle();

    DW_ZERO_MEMORY(write_data[4]);

    write_data[4].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_data[4].descriptorCount = 1;
    write_data[4].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_data[4].pImageInfo      = &image_info[4];
    write_data[4].dstBinding      = 4;
    write_data[4].dstSet          = ds->handle();

    vkUpdateDescriptorSets(backend->device(), 5, write_data, 0, nullptr);

    return ds;
}

// -----------------------------------------------------------------------------------------------------------------------------------

#else

Material::Ptr Material::load(const std::string& name, const std::string* textures)
{
    if (m_cache.find(name) == m_cache.end() || m_cache[name].expired())
    {
        Material::Ptr mat = std::shared_ptr<Material>(new Material(name, textures));
        m_cache[name]     = mat;
        return mat;
    }
    else
        return m_cache[name].lock();
}

// -----------------------------------------------------------------------------------------------------------------------------------

Material::Ptr Material::load(const std::string& name, int num_textures, gl::Texture2D** textures, glm::vec4 albedo, float roughness, float metalness)
{
    if (m_cache.find(name) == m_cache.end() || m_cache[name].expired())
    {
        Material::Ptr mat = std::shared_ptr<Material>(new Material());

        for (int i = 0; i < num_textures; i++)
            mat->m_textures[i] = textures[i];

        mat->m_albedo_val = albedo;

        m_cache[name] = mat;
        return mat;
    }
    else
        return m_cache[name].lock();
}

// -----------------------------------------------------------------------------------------------------------------------------------

gl::Texture2D* Material::load_texture(const std::string& path, bool srgb)
{
    if (m_texture_cache.find(path) == m_texture_cache.end())
    {
        gl::Texture2D* tex    = gl::Texture2D::create_from_files(path, false, srgb);
        m_texture_cache[path] = tex;
        return tex;
    }
    else
        return m_texture_cache[path];
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Material::unload_texture(gl::Texture2D*& tex)
{
    for (auto itr : m_texture_cache)
    {
        if (itr.second == tex)
        {
            m_texture_cache.erase(itr.first);
            DW_SAFE_DELETE(tex);
            return;
        }
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

Material::Material()
{
    for (uint32_t i = 0; i < 16; i++)
        m_textures[i] = nullptr;
}

// -----------------------------------------------------------------------------------------------------------------------------------

Material::Material(const std::string& name, const std::string* textures)
{
    for (uint32_t i = 0; i < 16; i++)
    {
        m_textures[i] = nullptr;

        if (!textures[i].empty())
        {
            // First index must always be diffuse/albedo, so SRGB is set to true.
            m_textures[i] = load_texture(textures[i], i == 0 ? true : false);
        }
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

Material::~Material()
{
    for (uint32_t i = 0; i < 16; i++)
    {
        if (m_textures[i])
            unload_texture(m_textures[i]);
    }
}

#endif

// -----------------------------------------------------------------------------------------------------------------------------------

bool Material::is_loaded(const std::string& name)
{
    return m_cache.find(name) != m_cache.end();
}

// -----------------------------------------------------------------------------------------------------------------------------------
} // namespace dw
