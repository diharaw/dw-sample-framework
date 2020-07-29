#pragma once

#include <memory>
#include <ogl.h>
#include <glm.hpp>

namespace dw
{
class ShadowMap
{
private:
    std::unique_ptr<gl::Framebuffer> m_shadow_map_fbo;
    std::unique_ptr<gl::Texture2D>   m_shadow_map;
    glm::vec3                        m_light_target = glm::vec3(0.0f);
    glm::vec3                        m_light_direction = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3                        m_light_color = glm::vec3(1.0f);
    glm::mat4                        m_view;
    glm::mat4                        m_projection;
    float                            m_near_plane = 1.0f;
    float                            m_far_plane = 1000.0f;
    float                            m_extents    = 75.0f;
    float                            m_backoff_distance = 200.0f;
    uint32_t                         m_size       = 1024;

public:
    ShadowMap();
    ~ShadowMap();

    bool initialize(uint32_t size);
    void shutdown();

    void begin_render();
    void end_render();

    void set_direction(const glm::vec3& d);
    void set_target(const glm::vec3& t);
    void set_color(const glm::vec3& c);
    void set_near_plane(const float& n);
    void set_far_plane(const float& f);
    void set_extents(const float& e);
    void set_backoff_distance(const float& d);

    inline gl::Framebuffer* framebuffer() { return m_shadow_map_fbo.get(); }
    inline gl::Texture2D*   texture() { return m_shadow_map.get(); }
    inline glm::vec3 direction(const glm::vec3& d) { return m_light_direction; }
    inline glm::vec3 target(const glm::vec3& t) { return m_light_target; }
    inline glm::vec3 color(const glm::vec3& c) { return m_light_color; }
    inline float            near_plane() { return m_near_plane; }
    inline float            far_plane() { return m_far_plane; }
    inline float            extents() { return m_extents; }
    inline float            backoff_distance() { return m_backoff_distance; }

private:
    void update();
};
} // namespace dw
