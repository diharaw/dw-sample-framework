#pragma once

#include <unordered_map>
#include <string>
#include <glm.hpp>
#include <ogl.h>
#include <vk.h>
#include <memory>

namespace dw
{
#define TEXTURE_TYPE_ALBEDO 1
#define TEXTURE_TYPE_NORMAL 5
#define TEXTURE_TYPE_ROUGHNESS 7
#define TEXTURE_TYPE_METALLIC 3

class Material
{
public:
    using Ptr = std::shared_ptr<Material>;

    static bool is_loaded(const std::string& name);

    inline uint32_t id() { return m_id; }

    ~Material();

    // Texture factory methods.
#if defined(DWSF_VULKAN)
    // Material factory methods.
    static Material::Ptr load(vk::Backend::Ptr backend, const std::string& name, const std::string* textures);

    // Custom factory method for creating a material from provided data.
    static Material::Ptr load(vk::Backend::Ptr backend, const std::string& name, int num_textures, vk::Image::Ptr* images, glm::vec4 albedo = glm::vec4(1.0f), float roughness = 0.0f, float metalness = 0.0f);
    static void          initialize_common_resources(vk::Backend::Ptr backend);
    static void          shutdown_common_resources();

    // Rendering related getters.
    inline vk::ImageView::Ptr                  image_view(const uint32_t& index) { return m_image_views[index] ? m_image_views[index] : m_default_image_view; }
    inline vk::Image::Ptr                      image(const uint32_t& index) { return m_images[index] ? m_images[index] : m_default_image; }
    inline vk::DescriptorSet::Ptr              pbr_descriptor_set() { return m_descriptor_set; }
    static inline vk::Sampler::Ptr             common_sampler() { return m_common_sampler; }
    static inline vk::DescriptorSetLayout::Ptr pbr_descriptor_set_layout() { return m_common_ds_layout; }

private:
    static vk::Image::Ptr     load_image(vk::Backend::Ptr backend, const std::string& path, bool srgb = false);
    static void               unload_image(vk::Image::Ptr image);
    static vk::ImageView::Ptr load_image_view(vk::Backend::Ptr backend, const std::string& path, vk::Image::Ptr image);
    static void               unload_image_view(vk::ImageView::Ptr image);

    vk::DescriptorSet::Ptr create_pbr_descriptor_set(vk::Backend::Ptr backend);

    // Private constructor and destructor.
    Material(vk::Backend::Ptr backend, const std::string& name, const std::string* textures);

#else

public:
    // Material factory methods.
    static Material::Ptr load(const std::string& name, const std::string* textures);

    // Custom factory method for creating a material from provided data.
    static Material::Ptr  load(const std::string& name, int num_textures, gl::Texture2D** textures, glm::vec4 albedo = glm::vec4(1.0f), float roughness = 0.0f, float metalness = 0.0f);
    static gl::Texture2D* load_texture(const std::string& path, bool srgb = false);
    static void           unload_texture(gl::Texture2D*& tex);

    // Rendering related getters.
    inline gl::Texture2D* texture(const uint32_t& index) { return m_textures[index]; }

private:
    // Private constructor and destructor.
    Material(const std::string& name, const std::string* textures);
#endif

public:
    inline glm::vec4 albedo_value() { return m_albedo_val; }

private:
    Material();

public:
    // Material cache.
    static std::unordered_map<std::string, std::weak_ptr<Material>> m_cache;

    // Albedo color.
    glm::vec4 m_albedo_val = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

    uint32_t m_id = 0;

    // Texture list. In the same order as the Assimp texture enums.
#if defined(DWSF_VULKAN)
    vk::Image::Ptr         m_images[16];
    vk::ImageView::Ptr     m_image_views[16];
    vk::DescriptorSet::Ptr m_descriptor_set;

    // Texture cache.
    static std::unordered_map<std::string, std::weak_ptr<vk::Image>>     m_image_cache;
    static std::unordered_map<std::string, std::weak_ptr<vk::ImageView>> m_image_view_cache;
    static vk::DescriptorSetLayout::Ptr                                  m_common_ds_layout;
    static vk::Sampler::Ptr                                              m_common_sampler;
    static vk::Image::Ptr                                                m_default_image;
    static vk::ImageView::Ptr                                            m_default_image_view;
#else
    gl::Texture2D* m_textures[16];

    // Texture cache.
    static std::unordered_map<std::string, gl::Texture2D*> m_texture_cache;
#endif
};
} // namespace dw
