#pragma once

#include <glm.hpp>
#include <gtc/quaternion.hpp>

struct Camera
{
    float m_fov;
    float m_near;
    float m_far;
    float m_aspect_ratio;
    glm::vec3 m_position;
    glm::vec3 m_forward;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_world_up;
    glm::quat m_orientation;
    float m_yaw;
    float m_pitch;
    float m_roll;
    
    glm::mat4 m_view;
    glm::mat4 m_projection;
    glm::mat4 m_view_projection;
    glm::mat4 m_rotate;
    glm::mat4 m_translate;
    
    Camera(float fov, float near, float far, float aspect_ratio, glm::vec3 position, glm::vec3 forward);
    void set_translation_delta(glm::vec3 direction, float amount);
    void set_rotatation_delta(glm::vec3 angles);
    void set_position(glm::vec3 position);
    void update();
	void update_projection(float fov, float near, float far, float aspect_ratio);
};
