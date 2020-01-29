#pragma once

#include <glm.hpp>
#include <gtc/quaternion.hpp>
#include <geometry.h>

namespace dw
{
struct Camera
{
    float     m_fov;
    float     m_near;
    float     m_far;
    float     m_aspect_ratio;
    glm::vec3 m_position;
    glm::vec3 m_forward;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_world_up;
    glm::quat m_orientation;
    float     m_yaw;
    float     m_pitch;
    float     m_roll;

    glm::mat4 m_view;
    glm::mat3 m_model;
    glm::mat4 m_projection;
    glm::mat4 m_view_projection;
    glm::mat4 m_prev_view_projection;
    glm::mat4 m_rotate;
    glm::mat4 m_translate;

    Plane m_near_plane;
    Plane m_far_plane;
    Plane m_left_plane;
    Plane m_right_plane;
    Plane m_top_plane;
    Plane m_bottom_plane;

    dw::Frustum m_frustum;

    Camera(float fov, float near, float far, float aspect_ratio, glm::vec3 position, glm::vec3 forward);
    void set_translation_delta(glm::vec3 direction, float amount);
    void set_rotatation_delta(glm::vec3 angles);
    void set_position(glm::vec3 position);
    void update();
    void update_from_frame(glm::vec3 position, glm::vec3 forward, glm::vec3 right);
    void update_projection(float fov, float near, float far, float aspect_ratio);
    bool aabb_inside_frustum(glm::vec3 max_v, glm::vec3 min_v);
    bool aabb_inside_plane(Plane plane, glm::vec3 max_v, glm::vec3 min_v);
};
} // namespace dw