#pragma once

#include <memory>
#include <ogl.h>
#include <glm.hpp>
#include <vk.h>

namespace dw
{
class ShadowMap
{
private:
#if defined(DWSF_VULKAN)
    vk::Image::Ptr       m_image;
    vk::ImageView::Ptr   m_image_view;
    vk::Framebuffer::Ptr m_framebuffer;
    vk::RenderPass::Ptr  m_render_pass;
#else
    gl::Framebuffer::Ptr        m_shadow_map_fbo;
    gl::Texture2D::Ptr          m_shadow_map;
#endif
    glm::vec3 m_light_target    = glm::vec3(0.0f);
    glm::vec3 m_light_direction = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 m_light_color     = glm::vec3(1.0f);
    glm::mat4 m_view;
    glm::mat4 m_projection;
    float     m_near_plane       = 1.0f;
    float     m_far_plane        = 1000.0f;
    float     m_extents          = 75.0f;
    float     m_backoff_distance = 200.0f;
    uint32_t  m_size             = 1024;

public:
    ShadowMap(
#if defined(DWSF_VULKAN)
        vk::Backend::Ptr backend,
#endif
        uint32_t size);
    ~ShadowMap();

    void begin_render(
#if defined(DWSF_VULKAN)
        vk::CommandBuffer::Ptr cmd_buf
#endif
    );
    void end_render(
#if defined(DWSF_VULKAN)
        vk::CommandBuffer::Ptr cmd_buf
#endif
    );

    void gui();
    void set_direction(const glm::vec3& d);
    void set_target(const glm::vec3& t);
    void set_color(const glm::vec3& c);
    void set_near_plane(const float& n);
    void set_far_plane(const float& f);
    void set_extents(const float& e);
    void set_backoff_distance(const float& d);

#if defined(DWSF_VULKAN)
    inline vk::Image::Ptr image()
    {
        return m_image;
    };
    inline vk::ImageView::Ptr   image_view() { return m_image_view; };
    inline vk::Framebuffer::Ptr framebuffer() { return m_framebuffer; };
    inline vk::RenderPass::Ptr  render_pass() { return m_render_pass; };
#else
    inline gl::Framebuffer::Ptr framebuffer()
    {
        return m_shadow_map_fbo;
    }
    inline gl::Texture2D::Ptr texture() { return m_shadow_map; }
#endif
    inline glm::vec3 direction()
    {
        return m_light_direction;
    }
    inline glm::vec3 target() { return m_light_target; }
    inline glm::vec3 color() { return m_light_color; }
    inline float     near_plane() { return m_near_plane; }
    inline float     far_plane() { return m_far_plane; }
    inline float     extents() { return m_extents; }
    inline float     backoff_distance() { return m_backoff_distance; }
    inline glm::mat4 view() { return m_view; }
    inline glm::mat4 projection() { return m_projection; }

private:
    void update();
};
} // namespace dw
