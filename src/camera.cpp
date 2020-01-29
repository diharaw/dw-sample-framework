#include <camera.h>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include <iostream>

#define PI 3.14159265359

namespace dw
{
Camera::Camera(float fov, float near, float far, float aspect_ratio, glm::vec3 position, glm::vec3 forward) :
    m_fov(fov), m_near(near), m_far(far), m_aspect_ratio(aspect_ratio),
    m_position(position)
{
    m_forward  = glm::normalize(forward);
    m_world_up = glm::vec3(0, 1, 0);
    m_right    = glm::normalize(glm::cross(m_forward, m_world_up));
    m_up       = glm::normalize(glm::cross(m_right, m_forward));

    m_roll  = 0.0f;
    m_pitch = 0.0f;
    m_yaw   = 0.0f;

    m_orientation          = glm::quat();
    m_projection           = glm::perspective(glm::radians(fov), aspect_ratio, near, far);
    m_prev_view_projection = glm::mat4(1.0f);

    // m_near_plane.n = glm::vec3(0.0f);

    // m_far_plane.point = position + m_forward * far;
    // m_far_plane.n = -m_forward;

    // m_left_plane.point = position;
    // float angle = PI / 2.0f + glm::radians(m_fov) / 2.0f + 0.1f;
    // m_left_plane.n = glm::vec3(glm::rotate(glm::mat4(1.0f), angle,
    // m_up)*glm::vec4(m_forward, 1.0f));

    // m_right_plane.point = position;
    // m_right_plane.n = glm::vec3(glm::rotate(glm::mat4(1.0f), -angle,
    // m_up)*glm::vec4(m_forward, 1.0f));

    // m_top_plane.point = position;
    // m_top_plane.n = glm::vec3(glm::rotate(glm::mat4(1.0f), angle,
    // m_right)*glm::vec4(m_forward, 1.0f));

    // m_bottom_plane.point = position;
    // m_bottom_plane.n = glm::vec3(glm::rotate(glm::mat4(1.0f), -angle,
    // m_right)*glm::vec4(m_forward, 1.0f));

    update();
}

void Camera::update_projection(float fov, float near, float far, float aspect_ratio)
{
    m_projection = glm::perspective(glm::radians(fov), aspect_ratio, near, far);
}

void Camera::set_translation_delta(glm::vec3 direction, float amount)
{
    m_position += direction * amount;
}

void Camera::set_rotatation_delta(glm::vec3 angles)
{
    m_yaw   = glm::radians(angles.y);
    m_pitch = glm::radians(angles.x);
    m_roll  = glm::radians(angles.z);
}

void Camera::set_position(glm::vec3 position) { m_position = position; }

void Camera::update()
{
    glm::quat qPitch = glm::angleAxis(m_pitch, glm::vec3(1.0f, 0.0f, 0.0f));
    glm::quat qYaw   = glm::angleAxis(m_yaw, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::quat qRoll  = glm::angleAxis(m_roll, glm::vec3(0.0f, 0.0f, 1.0f));

    m_orientation = qPitch * m_orientation;
    m_orientation = m_orientation * qYaw;
    m_orientation = qRoll * m_orientation;
    m_orientation = glm::normalize(m_orientation);

    m_rotate  = glm::mat4_cast(m_orientation);
    m_forward = glm::conjugate(m_orientation) * glm::vec3(0.0f, 0.0f, -1.0f);

    m_right = glm::normalize(glm::cross(m_forward, m_world_up));

    m_up = glm::normalize(glm::cross(m_right, m_forward));

    m_translate            = glm::translate(glm::mat4(1.0f), -m_position);
    m_view                 = m_rotate * m_translate;
    m_prev_view_projection = m_view_projection;
    m_view_projection      = m_projection * m_view;

    dw::frustum_from_matrix(m_frustum, m_view_projection);
}

void Camera::update_from_frame(glm::vec3 position, glm::vec3 forward, glm::vec3 right)
{
    m_position = position;
    m_forward  = forward;
    m_right    = right;

    m_up = glm::normalize(glm::cross(m_right, m_forward));

    m_view                 = glm::lookAt(m_position, m_position + m_forward * 10.0f, m_up);
    m_prev_view_projection = m_view_projection;
    m_view_projection      = m_projection * m_view;

    dw::frustum_from_matrix(m_frustum, m_view_projection);
}

bool Camera::aabb_inside_frustum(glm::vec3 max_v, glm::vec3 min_v)
{
    // m_model = glm::mat3(m_right, m_up, m_forward);

    // m_near_plane.point = m_position - m_forward * m_near;
    // if (!aabb_inside_plane(m_near_plane, max_v, min_v))
    //	return false;

    // m_far_plane.point = m_position - m_forward * m_far;
    // if (!aabb_inside_plane(m_far_plane, max_v, min_v))
    //	return false;

    // m_left_plane.point = m_position;
    // if (!aabb_inside_plane(m_left_plane, max_v, min_v))
    //	return false;

    // m_right_plane.point = m_position;
    // if (!aabb_inside_plane(m_right_plane, max_v, min_v))
    //	return false;

    // m_top_plane.point = m_position;
    // if (!aabb_inside_plane(m_top_plane, max_v, min_v))
    //	return false;

    // m_bottom_plane.point = m_position;
    // if (!aabb_inside_plane(m_bottom_plane, max_v, min_v))
    //	return false;

    return true;
}

bool Camera::aabb_inside_plane(Plane plane, glm::vec3 max_v, glm::vec3 min_v)
{
    // plane.normal = m_model * plane.normal;

    // glm::vec3 p = min_v;

    // if (plane.normal.x >= 0)
    //	p.x = max_v.x;
    // if (plane.normal.y >= 0)
    //	p.y = max_v.y;
    // if (plane.normal.z >= 0)
    //	p.z = max_v.z;

    // float dotProd = glm::dot(p - plane.point, plane.normal);
    // if (dotProd < 0)
    //	return false;
    // else
    return true;
}
} // namespace dw
