#pragma once

#include <unordered_map>
#include <string>
#include <glm.hpp>
#include <ogl.h>
#include <vk.h>
#include <memory>

namespace dw
{
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
    static Material::Ptr load(vk::Backend::Ptr backend, const std::vector<std::string>& textures, const int32_t& albedo_idx, const int32_t& normal_idx, const glm::ivec2& roughness_idx, const glm::ivec2& metallic_idx, const int32_t& emissive_idx);

    // Custom factory method for creating a material from provided data.
    static Material::Ptr create(vk::Backend::Ptr backend, glm::vec4 albedo = glm::vec4(1.0f), float roughness = 0.0f, float metalness = 0.0f, glm::vec3 emissive = glm::vec3(0.0f));
    static void          initialize_common_resources(vk::Backend::Ptr backend);
    static void          shutdown_common_resources();

    // Rendering related getters.
    inline vk::ImageView::Ptr                  albedo_image_view() { return m_albedo_idx != -1 ? m_image_views[m_albedo_idx] : m_default_image_view; }
    inline vk::ImageView::Ptr                  normal_image_view() { return m_normal_idx != -1 ? m_image_views[m_albedo_idx] : m_default_image_view; }
    inline vk::ImageView::Ptr                  roughness_image_view() { return m_roughness_idx.x != -1 ? m_image_views[m_roughness_idx.x] : m_default_image_view; }
    inline vk::ImageView::Ptr                  metallic_image_view() { return m_metallic_idx.x != -1 ? m_image_views[m_metallic_idx.x] : m_default_image_view; }
    inline vk::ImageView::Ptr                  emissive_image_view() { return m_emissive_idx != -1 ? m_image_views[m_emissive_idx] : m_default_image_view; }
    inline vk::Image::Ptr                      albedo_image() { return m_albedo_idx != -1 ? m_images[m_albedo_idx] : m_default_image; }
    inline vk::Image::Ptr                      normal_image() { return m_normal_idx != -1 ? m_images[m_normal_idx] : m_default_image; }
    inline vk::Image::Ptr                      roughness_image() { return m_roughness_idx.x != -1 ? m_images[m_roughness_idx.x] : m_default_image; }
    inline vk::Image::Ptr                      metallic_image() { return m_metallic_idx.x != -1 ? m_images[m_metallic_idx.x] : m_default_image; }
    inline vk::Image::Ptr                      emissive_image() { return m_emissive_idx != -1 ? m_images[m_emissive_idx] : m_default_image; }
    inline vk::DescriptorSet::Ptr              descriptor_set() { return m_descriptor_set; }
    static inline vk::Sampler::Ptr             common_sampler() { return m_common_sampler; }
    static inline vk::DescriptorSetLayout::Ptr descriptor_set_layout() { return m_common_ds_layout; }

private:
    static vk::Image::Ptr     load_image(vk::Backend::Ptr backend, const std::string& path, bool srgb = false);
    static vk::ImageView::Ptr load_image_view(vk::Backend::Ptr backend, const std::string& path, vk::Image::Ptr image);

    vk::DescriptorSet::Ptr create_descriptor_set(vk::Backend::Ptr backend);

    // Private constructor and destructor.
    Material(vk::Backend::Ptr backend, const std::vector<std::string>& textures, const int32_t& albedo_idx, const int32_t& normal_idx, const glm::ivec2& roughness_idx, const glm::ivec2& metallic_idx, const int32_t& emissive_idx);

#else

public:
    // Material factory methods.
    static Material::Ptr load(const std::string& name, const std::string& albedo, const std::string& normal, const std::string& roughness, const std::string& metallic, const std::string& emissive);

    // Custom factory method for creating a material from provided data.
    static Material::Ptr  load(const std::string& name, glm::vec4 albedo = glm::vec4(1.0f), float roughness = 0.0f, float metalness = 0.0f, glm::vec3 emissive = glm::vec3(0.0f));
    static gl::Texture2D* load_texture(const std::string& path, bool srgb = false);

    // Rendering related getters.
    inline gl::Texture2D* texture(const TextureType& type) { return m_textures[type]; }

    inline gl::Texture2D* albedo_texture() { return m_albedo_texture; }
    inline gl::Texture2D* normal_texture() { return m_normal_texture; }
    inline gl::Texture2D* roughness_texture() { return m_roughness_texture; }
    inline gl::Texture2D* metallic_texture() { return m_metallic_texture; }
    inline gl::Texture2D* emissive_texture() { return m_emissive_texture; }

private:
    // Private constructor and destructor.
    Material(const std::string& name, const std::string* textures);
#endif

public:
    inline glm::vec4 albedo_value() { return m_albedo_color; }
    inline float     roughness_value() { return m_roughness; }
    inline float     metallic_value() { return m_metallic; }
    inline glm::vec3 emissive_value() { return m_emissive_color; }

    inline void set_albedo_value(const glm::vec4& value) { m_albedo_color = value; }
    inline void set_roughness_value(const float& value) { m_roughness = value; }
    inline void set_metallic_value(const float& value) { m_metallic = value; }
    inline void set_emissive_value(const glm::vec3& value) { m_emissive_color = value; }

private:
    Material();

public:
    // Material cache.
    static std::unordered_map<std::string, std::weak_ptr<Material>> m_cache;

    int32_t  m_albedo_idx = -1;
    int32_t           m_normal_idx = -1;
    glm::ivec2        m_roughness_idx    = glm::ivec2(-1);
    glm::ivec2        m_metallic_idx  = glm::ivec2(-1);
    int32_t           m_emissive_idx   = -1;
    glm::vec4 m_albedo_color     = glm::vec4(1.0f);
    glm::vec3 m_emissive_color = glm::vec3(0.0f);
    float     m_roughness      = 1.0f;
    float     m_metallic       = 0.0f;

    uint32_t m_id = 0;

    // Texture list. In the same order as the Assimp texture enums.
#if defined(DWSF_VULKAN)
    std::vector<vk::Image::Ptr> m_images;
    std::vector<vk::ImageView::Ptr> m_image_views;

    vk::DescriptorSet::Ptr m_descriptor_set;

    // Texture cache.
    static std::unordered_map<std::string, std::weak_ptr<vk::Image>>     m_image_cache;
    static std::unordered_map<std::string, std::weak_ptr<vk::ImageView>> m_image_view_cache;
    static vk::DescriptorSetLayout::Ptr                                  m_common_ds_layout;
    static vk::Sampler::Ptr                                              m_common_sampler;
    static vk::Image::Ptr                                                m_default_image;
    static vk::ImageView::Ptr                                            m_default_image_view;
#else
    gl::Texture2D* m_albedo_texture;
    gl::Texture2D* m_normal_texture;
    gl::Texture2D* m_roughness_texture;
    gl::Texture2D* m_metallic_texture;
    gl::Texture2D* m_emissive_texture;

    // Texture cache.
    static std::unordered_map<std::string, gl::Texture2D*> m_texture_cache;
#endif
};
} // namespace dw
