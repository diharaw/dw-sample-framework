#include "shadow_map.h"
#include <gtc/matrix_transform.hpp>

namespace dw
{
// -----------------------------------------------------------------------------------------------------------------------------------

ShadowMap::ShadowMap()
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

ShadowMap::~ShadowMap()
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool ShadowMap::initialize(uint32_t size)
{
    m_size       = size;
    m_shadow_map = std::make_unique<gl::Texture2D>(m_size, m_size, 1, 1, 1, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT);

    m_shadow_map_fbo = std::make_unique<gl::Framebuffer>();
    m_shadow_map_fbo->attach_depth_stencil_target(m_shadow_map.get(), 0, 0);

    update();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void ShadowMap::shutdown()
{
    m_shadow_map_fbo.reset();
    m_shadow_map.reset();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void ShadowMap::begin_render()
{
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    m_shadow_map_fbo->bind();

    glViewport(0, 0, m_size, m_size);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(1.0);
    glClear(GL_DEPTH_BUFFER_BIT);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void ShadowMap::end_render()
{
    glEnable(GL_CULL_FACE);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void ShadowMap::set_direction(const glm::vec3& d)
{
    m_light_direction = d;
    update();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void ShadowMap::set_target(const glm::vec3& t)
{
    m_light_target = t;
    update();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void ShadowMap::set_color(const glm::vec3& c)
{
    m_light_color = c;
    update();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void ShadowMap::set_near_plane(const float& n)
{
    m_near_plane = n;
    update();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void ShadowMap::set_far_plane(const float& f)
{
    m_far_plane = f;
    update();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void ShadowMap::set_extents(const float& e)
{
    m_extents = e;
    update();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void ShadowMap::set_backoff_distance(const float& d)
{
    m_backoff_distance = d;
    update();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void ShadowMap::update()
{
    glm::vec3 light_camera_pos = m_light_target - m_light_direction * m_backoff_distance;
    m_view                     = glm::lookAt(light_camera_pos, m_light_target, glm::vec3(0.0f, 1.0f, 0.0f));
    m_projection               = glm::ortho(-m_extents, m_extents, -m_extents, m_extents, m_near_plane, m_far_plane);
}

// -----------------------------------------------------------------------------------------------------------------------------------
} // namespace dw