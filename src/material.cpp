#include <logger.h>
#include <macros.h>
#include <material.h>
#include <utility.h>

namespace dw
{
std::unordered_map<std::string, Material*> Material::m_cache;

#if defined(DWSF_VULKAN)
std::unordered_map<std::string, std::weak_ptr<vk::Image>>     Material::m_image_cache;
std::unordered_map<std::string, std::weak_ptr<vk::ImageView>> Material::m_image_view_cache;
vk::DescriptorSetLayout::Ptr                                  Material::m_common_ds_layout;
#else
std::unordered_map<std::string, gl::Texture2D*> Material::m_texture_cache;
#endif

#if defined(DWSF_VULKAN)

// -----------------------------------------------------------------------------------------------------------------------------------

Material* Material::load(vk::Backend::Ptr backend, const std::string& name, const std::string* textures)
{
    if (m_cache.find(name) == m_cache.end())
    {
        Material* mat = new Material(backend, name, textures);
        m_cache[name] = mat;
        return mat;
    }
    else
        return m_cache[name];
}

// -----------------------------------------------------------------------------------------------------------------------------------

Material* Material::load(vk::Backend::Ptr backend, const std::string& name, int num_textures, vk::Image::Ptr* image, glm::vec4 albedo, float roughness, float metalness)
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

Material::Material()
{
    for (uint32_t i = 0; i < 16; i++)
    {
        m_images[i]      = nullptr;
        m_image_views[i] = nullptr;
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

Material::Material(vk::Backend::Ptr backend, const std::string& name, const std::string* textures)
{
    for (uint32_t i = 0; i < 16; i++)
    {
        m_images[i]      = nullptr;
        m_image_views[i] = nullptr;

        if (!textures[i].empty())
        {
            // First index must always be diffuse/albedo, so SRGB is set to true.
            m_images[i]      = load_image(backend, textures[i], i == 0 ? true : false);
            m_image_views[i] = load_image_view(backend, textures[i], m_images[i]);
        }
    }

    // Create descriptor set
}

// -----------------------------------------------------------------------------------------------------------------------------------

Material::~Material()
{
    for (uint32_t i = 0; i < 16; i++)
    {
        if (m_images[i])
            unload_image(m_images[i]);
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Material::initialize_common_resources(vk::Backend::Ptr backend)
{
    vk::DescriptorSetLayout::Desc ds_layout_desc;

    ds_layout_desc.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    ds_layout_desc.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    ds_layout_desc.add_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);

    m_common_ds_layout = vk::DescriptorSetLayout::create(backend, ds_layout_desc);

    vk::Sampler::Desc sampler_desc;

    sampler_desc.mag_filter        = VK_FILTER_LINEAR;
    sampler_desc.min_filter        = VK_FILTER_LINEAR;
    sampler_desc.mipmap_mode       = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_desc.address_mode_u    = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_desc.address_mode_v    = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_desc.address_mode_w    = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_desc.mip_lod_bias      = 0.0f;
    sampler_desc.anisotropy_enable = VK_FALSE;
    sampler_desc.max_anisotropy    = 1.0f;
    sampler_desc.compare_enable    = false;
    sampler_desc.compare_op        = VK_COMPARE_OP_NEVER;
    sampler_desc.min_lod           = 0.0f;
    sampler_desc.max_lod           = 1.0f;
    sampler_desc.border_color      = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler_desc.unnormalized_coordinates;

    m_common_sampler = vk::Sampler::create(backend, sampler_desc);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Material::shutdown_common_resources()
{
    m_common_ds_layout.reset();
    m_common_sampler.reset();
}

// -----------------------------------------------------------------------------------------------------------------------------------

vk::Image::Ptr Material::load_image(vk::Backend::Ptr backend, const std::string& path, bool srgb)
{
    if (m_image_cache.find(path) == m_image_cache.end() || m_image_cache[path].expired())
    {
        vk::Image::Ptr tex  = vk::Image::create_from_file(backend, path, true, srgb);
        m_image_cache[path] = tex;
        return tex;
    }
    else        
        return m_image_cache[path].lock();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Material::unload_image(vk::Image::Ptr image)
{
    for (auto itr : m_image_cache)
    {
        if (itr.second.expired())
            continue;

        auto current = itr.second.lock();

        if (current->handle() == image->handle())
        {
            m_image_cache.erase(itr.first);
            return;
        }
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

vk::ImageView::Ptr Material::load_image_view(vk::Backend::Ptr backend, const std::string& path, vk::Image::Ptr image)
{
    if (m_image_view_cache.find(path) == m_image_view_cache.end() || m_image_view_cache[path].expired())
    {
        vk::ImageView::Ptr image_view  = vk::ImageView::create(backend, image, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT);
        m_image_view_cache[path]           = image_view;
        return image_view;
    }
    else
        return m_image_view_cache[path].lock();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Material::unload_image_view(vk::ImageView::Ptr image)
{
    for (auto itr : m_image_view_cache)
    {
        if (itr.second.expired())
            continue;

        auto current = itr.second.lock();

        if (current->handle() == image->handle())
        {
            m_image_view_cache.erase(itr.first);
            return;
        }
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

#else

Material* Material::load(const std::string& name, const std::string* textures)
{
    if (m_cache.find(name) == m_cache.end())
    {
        Material* mat = new Material(name, textures);
        m_cache[name] = mat;
        return mat;
    }
    else
        return m_cache[name];
}

// -----------------------------------------------------------------------------------------------------------------------------------

Material* Material::load(const std::string& name, int num_textures, gl::Texture2D** textures, glm::vec4 albedo, float roughness, float metalness)
{
    if (m_cache.find(name) == m_cache.end())
    {
        Material* mat = new Material();

        for (int i = 0; i < num_textures; i++)
            mat->m_textures[i] = textures[i];

        mat->m_albedo_val = albedo;

        m_cache[name] = mat;
        return mat;
    }
    else
        return m_cache[name];
}

// -----------------------------------------------------------------------------------------------------------------------------------

gl::Texture2D* Material::load_texture(const std::string& path, bool srgb)
{
    if (m_texture_cache.find(path) == m_texture_cache.end())
    {
        gl::Texture2D* tex    = gl::Texture2D::create_from_files(path, srgb);
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

void Material::unload(Material*& mat)
{
    for (auto itr : m_cache)
    {
        if (itr.second == mat)
        {
            m_cache.erase(itr.first);
            DW_SAFE_DELETE(mat);
            return;
        }
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------
} // namespace dw
