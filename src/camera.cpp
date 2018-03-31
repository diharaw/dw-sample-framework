#include <camera.h>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include <iostream>

Camera::Camera(float fov, float near, float far, float aspect_ratio, glm::vec3 position, glm::vec3 forward) : m_fov(fov),
                                                                                                              m_near(near),
                                                                                                              m_far(far),
                                                                                                              m_aspect_ratio(aspect_ratio),
                                                                                                              m_position(position)
{
    m_forward = glm::normalize(forward);
    m_world_up = glm::vec3(0, 1, 0);
    m_right = glm::normalize(glm::cross(m_forward, m_world_up));
    m_up = glm::normalize(glm::cross(m_right, m_forward));
    
    m_roll = 0.0f;
    m_pitch = 0.0f;
    m_yaw = 0.0f;
    
	m_orientation = glm::quat();
    m_projection = glm::perspective(glm::radians(fov), aspect_ratio, near, far);
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
    m_yaw = glm::radians(angles.y);
    m_pitch = glm::radians(angles.x);
    m_roll = glm::radians(angles.z);
}

void Camera::set_position(glm::vec3 position)
{
    m_position = position;
}

void Camera::update()
{
    glm::quat qPitch = glm::angleAxis(m_pitch, glm::vec3(1.0f, 0.0f, 0.0f));
    glm::quat qYaw = glm::angleAxis(m_yaw, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::quat qRoll = glm::angleAxis(m_roll, glm::vec3(0.0f, 0.0f, 1.0f));
    
    m_orientation = qPitch * m_orientation;
    m_orientation = m_orientation * qYaw;
    m_orientation = qRoll * m_orientation;
    m_orientation = glm::normalize(m_orientation);
    
    m_rotate = glm::mat4_cast(m_orientation);
    m_forward = glm::conjugate(m_orientation) * glm::vec3(0.0f, 0.0f, -1.0f);

    m_right = glm::normalize(glm::cross(m_forward, m_world_up));
    m_up = glm::normalize(glm::cross(m_right, m_forward));
 
    m_translate = glm::translate(glm::mat4(1.0f), -m_position);
    m_view = m_rotate * m_translate;
    m_view_projection = m_projection * m_view;
}
