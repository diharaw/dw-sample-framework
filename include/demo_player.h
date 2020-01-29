#pragma once

#include <camera.h>
#include <debug_draw.h>
#include <vector>

namespace dw
{
struct CubicSpline
{
    struct Coefficient
    {
        glm::vec3 coef[4];
    };

    std::vector<glm::vec3>   m_points;
    std::vector<Coefficient> m_coeffs;
    std::vector<float>       m_lengths;

    // Spline construction, Burden & Faires - Numerical Analysis 9th, algorithm 3.4
    void      initialize();
    void      add_point(glm::vec3 p);
    glm::vec3 spline_at_time(float t);
    float     arc_length_integrand_single_spline(int spline, float t);
    float     arc_length_integrand(float t);
    // Composite Simpson's Rule, Burden & Faires - Numerical Analysis 9th, algorithm 4.1
    float     simpsons_rule_single_spline(int spline, float t);
    float     simpsons_rule(float t0, float t1);
    glm::vec3 const_velocity_spline_at_time(float t, float speed);
    float     current_distance(float t, float speed);
    float     total_length();
};

class LerpSpline
{
public:
    std::vector<glm::vec3> m_points;

    void      add_point(glm::vec3 p);
    glm::vec3 value_at_time(float t);
};

class DemoPlayer
{
private:
    float                  m_time                = 0.0f;
    bool                   m_is_playing          = false;
    bool                   m_debug_visualization = false;
    float                  m_speed               = 10.0f;
    CubicSpline            m_position_spline;
    LerpSpline             m_forward_spline;
    LerpSpline             m_right_spline;
    glm::vec3              m_current_forward;
    glm::vec3              m_current_right;
    glm::vec3              m_current_position;
    std::vector<glm::vec3> m_debug_spline_segments;
    const uint32_t         SEGMENTS_PER_POINT = 60;

public:
    DemoPlayer();
    ~DemoPlayer();
    bool      load_from_file();
    void      edit_ui(Camera* camera);
    void      debug_visualization(DebugDraw& debug_draw);
    float     speed();
    void      play();
    void      stop();
    bool      is_playing();
    void      update(float dt, Camera* camera);
    glm::vec3 position();
    glm::vec3 forward();
    glm::vec3 right();
    void      initialize_debug();
};
} // namespace dw