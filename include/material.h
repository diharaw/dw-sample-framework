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

    // Material factory methods.
    static Material::Ptr load(
#if defined(DWSF_VULKAN)
        vk::Backend::Ptr backend,
#endif
        const std::vector<std::string>& textures,
        const int32_t&                  albedo_idx,
        const int32_t&                  normal_idx,
        const glm::ivec2&               roughness_idx,
        const glm::ivec2&               metallic_idx,
        const int32_t&                  emissive_idx);

    // Custom factory method for creating a material from provided data.
    static Material::Ptr create(glm::vec4 albedo    = glm::vec4(1.0f),
                                float     roughness = 0.0f,
                                float     metalness = 0.0f,
                                glm::vec3 emissive  = glm::vec3(0.0f));

    static bool is_loaded(const std::string& name);

    ~Material();

    inline uint32_t  id() { return m_id; }
    inline glm::vec4 albedo_value() { return m_albedo_color; }
    inline float     roughness_value() { return m_roughness; }
    inline float     metallic_value() { return m_metallic; }
    inline glm::vec3 emissive_value() { return m_emissive_color; }
    inline bool      alpha_test() { return m_alpha_test; }

    inline int32_t albedo_idx() { return m_albedo_idx; }
    inline int32_t normal_idx() { return m_normal_idx; }
    inline int32_t roughness_idx() { return m_roughness_idx; }
    inline int32_t metallic_idx() { return m_metallic_idx; }
    inline int32_t emissive_idx() { return m_emissive_idx; }
    inline int32_t roughness_channel() { return m_roughness_channel; }
    inline int32_t metallic_channel() { return m_metallic_channel; }

    inline void set_albedo_value(const glm::vec4& value) { m_albedo_color = value; }
    inline void set_roughness_value(const float& value) { m_roughness = value; }
    inline void set_metallic_value(const float& value) { m_metallic = value; }
    inline void set_emissive_value(const glm::vec3& value) { m_emissive_color = value; }
    inline void set_alpha_test(const bool& value) { m_alpha_test = value; }

    // Texture factory methods.
#if defined(DWSF_VULKAN)
    static void initialize_common_resources(vk::Backend::Ptr backend);
    static void shutdown_common_resources();

    // Rendering related getters.
    inline vk::ImageView::Ptr                  albedo_image_view() { return m_albedo_idx != -1 ? m_image_views[m_albedo_idx] : nullptr; }
    inline vk::ImageView::Ptr                  normal_image_view() { return m_normal_idx != -1 ? m_image_views[m_normal_idx] : nullptr; }
    inline vk::ImageView::Ptr                  roughness_image_view() { return m_roughness_idx != -1 ? m_image_views[m_roughness_idx] : nullptr; }
    inline vk::ImageView::Ptr                  metallic_image_view() { return m_metallic_idx != -1 ? m_image_views[m_metallic_idx] : nullptr; }
    inline vk::ImageView::Ptr                  emissive_image_view() { return m_emissive_idx != -1 ? m_image_views[m_emissive_idx] : nullptr; }
    inline vk::Image::Ptr                      albedo_image() { return m_albedo_idx != -1 ? m_images[m_albedo_idx] : nullptr; }
    inline vk::Image::Ptr                      normal_image() { return m_normal_idx != -1 ? m_images[m_normal_idx] : nullptr; }
    inline vk::Image::Ptr                      roughness_image() { return m_roughness_idx != -1 ? m_images[m_roughness_idx] : nullptr; }
    inline vk::Image::Ptr                      metallic_image() { return m_metallic_idx != -1 ? m_images[m_metallic_idx] : nullptr; }
    inline vk::Image::Ptr                      emissive_image() { return m_emissive_idx != -1 ? m_images[m_emissive_idx] : nullptr; }
    inline vk::DescriptorSet::Ptr              descriptor_set() { return m_descriptor_set; }
    static inline vk::Sampler::Ptr             common_sampler() { return m_common_sampler; }
    static inline vk::DescriptorSetLayout::Ptr descriptor_set_layout() { return m_common_ds_layout; }
#else
    // Rendering related getters.
    inline gl::Texture2D::Ptr       albedo_texture() { return m_albedo_idx != -1 ? m_textures[m_albedo_idx] : nullptr; }
    inline gl::Texture2D::Ptr       normal_texture() { return m_normal_idx != -1 ? m_textures[m_normal_idx] : nullptr; }
    inline gl::Texture2D::Ptr       roughness_texture() { return m_roughness_idx != -1 ? m_textures[m_roughness_idx] : nullptr; }
    inline gl::Texture2D::Ptr       metallic_texture() { return m_metallic_idx != -1 ? m_textures[m_metallic_idx] : nullptr; }
    inline gl::Texture2D::Ptr       emissive_texture() { return m_emissive_idx != -1 ? m_textures[m_emissive_idx] : nullptr; }

#endif

private:
#if defined(DWSF_VULKAN)
    static vk::Image::Ptr     load_image(vk::Backend::Ptr backend, const std::string& path, bool srgb = false);
    static vk::ImageView::Ptr load_image_view(vk::Backend::Ptr backend, const std::string& path, vk::Image::Ptr image);

    vk::DescriptorSet::Ptr create_descriptor_set(vk::Backend::Ptr backend);
#else
    static gl::Texture2D::Ptr       load_texture(const std::string& path, bool srgb = false);
#endif

private:
    // Private constructor and destructor.
    Material(
#if defined(DWSF_VULKAN)
        vk::Backend::Ptr backend,
#endif
        const std::vector<std::string>& textures,
        const int32_t&                  albedo_idx,
        const int32_t&                  normal_idx,
        const glm::ivec2&               roughness_idx,
        const glm::ivec2&               metallic_idx,
        const int32_t&                  emissive_idx);
    Material();

private:
    // Material cache.
    static std::unordered_map<std::string, std::weak_ptr<Material>> m_cache;

    int32_t   m_albedo_idx        = -1;
    int32_t   m_normal_idx        = -1;
    int32_t   m_roughness_idx     = -1;
    int32_t   m_metallic_idx      = -1;
    int32_t   m_emissive_idx      = -1;
    int32_t   m_roughness_channel = -1;
    int32_t   m_metallic_channel  = -1;
    glm::vec4 m_albedo_color      = glm::vec4(1.0f);
    glm::vec3 m_emissive_color    = glm::vec3(0.0f);
    float     m_roughness         = 1.0f;
    float     m_metallic          = 0.0f;
    bool      m_alpha_test        = false;

    uint32_t m_id = 0;

    // Texture list. In the same order as the Assimp texture enums.
#if defined(DWSF_VULKAN)
    std::vector<vk::Image::Ptr>     m_images;
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
    std::vector<gl::Texture2D::Ptr> m_textures;

    // Texture cache.
    static std::unordered_map<std::string, std::weak_ptr<gl::Texture2D>> m_texture_cache;
#endif
};
} // namespace dw
