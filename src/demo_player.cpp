#include <demo_player.h>
#include <imgui.h>
#include <fstream>

namespace dw
{
// -----------------------------------------------------------------------------------------------------------------------------------

void CubicSpline::initialize()
{
    int n = m_points.size() - 1;
    m_coeffs.resize(m_points.size());
    m_lengths.resize(n);

    std::vector<glm::vec3> a(m_points.size());
    for (int i = 1; i <= n - 1; i++)
        a[i] = 3.0f * ((m_points[i + 1] - 2.0f * m_points[i] + m_points[i - 1]));

    std::vector<float>     l(m_points.size());
    std::vector<float>     mu(m_points.size());
    std::vector<glm::vec3> z(m_points.size());

    l[0] = l[n] = 1;
    mu[0]       = 0;
    z[0] = z[n]         = glm::vec3(0.0f);
    m_coeffs[n].coef[2] = glm::vec3(0.0f);

    for (int i = 1; i <= n - 1; i++)
    {
        l[i]  = 4 - mu[i - 1];
        mu[i] = 1 / l[i];
        z[i]  = (a[i] - z[i - 1]) / l[i];
    }

    for (int i = 0; i < m_points.size() - 1; i++)
        m_coeffs[i].coef[0] = m_points[i];

    for (int j = n - 1; j >= 0; j--)
    {
        m_coeffs[j].coef[2] = z[j] - mu[j] * m_coeffs[j + 1].coef[2];
        m_coeffs[j].coef[3] = (1.0f / 3.0f) * (m_coeffs[j + 1].coef[2] - m_coeffs[j].coef[2]);
        m_coeffs[j].coef[1] = m_points[j + 1] - m_points[j] - m_coeffs[j].coef[2] - m_coeffs[j].coef[3];
    }

    for (int k = 0; k < m_points.size() - 1; k++)
        m_lengths[k] = simpsons_rule_single_spline(k, 1);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void CubicSpline::add_point(glm::vec3 p)
{
    m_points.push_back(p);
}

// -----------------------------------------------------------------------------------------------------------------------------------

glm::vec3 CubicSpline::spline_at_time(float t)
{
    if (t > m_points.size())
        t = m_points.size();

    if (t == m_points.size())
        t = m_points.size() - 0.0000f;

    int   spline     = (int)t;
    float fractional = t - spline;

    spline = spline % (m_points.size() - 1);

    float x   = fractional;
    float xx  = x * x;
    float xxx = x * xx;

    glm::vec3 result = m_coeffs[spline].coef[0] + m_coeffs[spline].coef[1] * x + m_coeffs[spline].coef[2] * xx + m_coeffs[spline].coef[3] * xxx;
    return result;
}

// -----------------------------------------------------------------------------------------------------------------------------------

float CubicSpline::arc_length_integrand_single_spline(int spline, float t)
{
    assert(t >= -1);
    assert(t <= 2);

    float tt = t * t;

    glm::vec3 dv = m_coeffs[spline].coef[1] + 2.0f * m_coeffs[spline].coef[2] * t + 3.0f * m_coeffs[spline].coef[3] * tt;
    float     xx = dv.x * dv.x;
    float     yy = dv.y * dv.y;
    float     zz = dv.z * dv.z;

    return sqrt(xx + yy + zz);
}

// -----------------------------------------------------------------------------------------------------------------------------------

float CubicSpline::arc_length_integrand(float t)
{
    if (t < 0)
        return arc_length_integrand_single_spline(0, t);

    if (t >= m_points.size() - 1)
        return arc_length_integrand_single_spline(m_points.size() - 2, fmod(t, 1));

    return arc_length_integrand_single_spline((int)t, t - (int)t);
}

// -----------------------------------------------------------------------------------------------------------------------------------

// Composite Simpson's Rule, Burden & Faires - Numerical Analysis 9th, algorithm 4.1
float CubicSpline::simpsons_rule_single_spline(int spline, float t)
{
    assert(t >= -1);
    assert(t <= 2);

    int   n   = 16;
    float h   = t / n;
    float XI0 = arc_length_integrand_single_spline(spline, t);
    float XI1 = 0;
    float XI2 = 0;

    for (int i = 0; i < n; i += 2)
        XI2 += arc_length_integrand_single_spline(spline, i * h);

    for (int i = 1; i < n; i += 2)
        XI1 += arc_length_integrand_single_spline(spline, i * h);

    float XI = h * (XI0 + 2 * XI2 + 4 * XI1) * (1.0f / 3);
    return XI;
}

// -----------------------------------------------------------------------------------------------------------------------------------

float CubicSpline::simpsons_rule(float t0, float t1)
{
    float multiplier = 1;
    if (t0 > t1)
    {
        std::swap(t0, t1);
        multiplier = -1;
    }

    int first_spline = (int)t0;
    if (first_spline < 0)
        first_spline = 0;

    int last_spline = (int)t1;
    if (last_spline >= m_points.size() - 1)
        last_spline = m_points.size() - 2;

    if (first_spline == last_spline)
        return (simpsons_rule_single_spline(last_spline, t1 - last_spline) - simpsons_rule_single_spline(first_spline, t0 - first_spline)) * multiplier;

    float sum = m_lengths[first_spline] - simpsons_rule_single_spline(first_spline, t0);

    for (int k = first_spline + 1; k < last_spline; k++)
        sum += m_lengths[k];

    sum += simpsons_rule_single_spline(last_spline, t1 - last_spline);

    return sum * multiplier;
}

// -----------------------------------------------------------------------------------------------------------------------------------

glm::vec3 CubicSpline::const_velocity_spline_at_time(float t, float speed)
{
    float l                = total_length();
    float desired_distance = fmod(t * speed, l);

    float t_last = m_points.size() * desired_distance / l;

    if (t_last > (m_points.size() - 1))
        t_last = 0.0f;

    float t_next = t_last;

    auto g = [this, desired_distance](float t) -> float {
        return simpsons_rule(0, t) - desired_distance;
    };

    auto L = [this](float t) -> float {
        return arc_length_integrand(t);
    };

    float t_max = m_points.size();
    float t_min = -0.1f;

    while (t_max - t_min > 0.5f)
    {
        float t_mid = (t_max + t_min) / 2;
        if (g(t_min) * g(t_mid) < 0)
            t_max = t_mid;
        else
            t_min = t_mid;
    }

    t_next = (t_max + t_min) / 2;

    do
    {
        t_last = t_next;
        t_next = t_last - g(t_last) / L(t_last);
    } while (fabs(t_last - t_next) > 0.001f);

    // Because of root finding it may be slightly negative sometimes.
    assert(t_next >= -0.1f && t_next <= 999999);

    return spline_at_time(t_next);
}

// -----------------------------------------------------------------------------------------------------------------------------------

float CubicSpline::current_distance(float t, float speed)
{
    float l = total_length();
    return fmod(t * speed, l);
}

// -----------------------------------------------------------------------------------------------------------------------------------

float CubicSpline::total_length()
{
    float sum = 0;

    for (int k = 0; k < m_lengths.size(); k++)
        sum += m_lengths[k];

    return sum;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void LerpSpline::add_point(glm::vec3 p)
{
    m_points.push_back(p);
}

// -----------------------------------------------------------------------------------------------------------------------------------

glm::vec3 LerpSpline::value_at_time(float t)
{
    float frame      = float(m_points.size() - 1) * t;
    float next_frame = ceil(frame);
    float fractional = frame - (int)frame;

    return glm::mix(m_points[int(frame)], m_points[int(next_frame)], fractional);
}

// -----------------------------------------------------------------------------------------------------------------------------------

DemoPlayer::DemoPlayer()
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

DemoPlayer::~DemoPlayer()
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool DemoPlayer::load_from_file()
{
    std::fstream f("camera_path.bin", std::ios::in | std::ios::binary);

    if (!f.is_open())
        return false;

    long offset = 0;
    f.seekp(offset);

    size_t size = sizeof(uint32_t);

    uint32_t count = 0;

    f.read((char*)&count, size);
    offset += size;
    f.seekg(offset);

    size = sizeof(glm::vec3) * count;

    std::vector<glm::vec3> buffer(count);

    f.read((char*)buffer.data(), size);
    offset += size;
    f.seekg(offset);

    for (int i = 0; i < count; i++)
        m_position_spline.add_point(buffer[i]);

    f.read((char*)buffer.data(), size);
    offset += size;
    f.seekg(offset);

    for (int i = 0; i < count; i++)
        m_forward_spline.add_point(buffer[i]);

    f.read((char*)buffer.data(), size);
    offset += size;
    f.seekg(offset);

    for (int i = 0; i < count; i++)
        m_right_spline.add_point(buffer[i]);

    f.close();

    if (m_position_spline.m_points.size() > 4)
    {
        m_position_spline.initialize();
        initialize_debug();
    }

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void DemoPlayer::edit_ui(Camera* camera)
{
    if (ImGui::Button("Add Frame"))
    {
        m_position_spline.add_point(camera->m_position);
        m_forward_spline.add_point(camera->m_forward);
        m_right_spline.add_point(camera->m_right);

        if (m_position_spline.m_points.size() > 4)
        {
            m_position_spline.initialize();
            initialize_debug();
        }
    }

    ImGui::SliderFloat("Camera Speed", &m_speed, 0.1f, 20.0f);

    ImGui::Checkbox("Debug Visualization", &m_debug_visualization);

    if (ImGui::Checkbox("Is Playing", &m_is_playing))
    {
        if (m_is_playing)
            play();
        else
            stop();
    }

    if (ImGui::Button("Save"))
    {
        std::fstream f("camera_path.bin", std::ios::out | std::ios::binary);

        long offset = 0;
        f.seekp(offset);

        size_t size = sizeof(uint32_t);

        uint32_t count = m_position_spline.m_points.size();

        f.write((char*)&count, size);
        offset += size;
        f.seekg(offset);

        size = sizeof(glm::vec3) * m_position_spline.m_points.size();

        f.write((char*)m_position_spline.m_points.data(), size);
        offset += size;
        f.seekg(offset);

        f.write((char*)m_forward_spline.m_points.data(), size);
        offset += size;
        f.seekg(offset);

        f.write((char*)m_right_spline.m_points.data(), size);
        offset += size;
        f.seekg(offset);

        f.close();
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

void DemoPlayer::debug_visualization(DebugDraw& debug_draw)
{
    if (m_debug_visualization)
    {
        if (m_position_spline.m_points.size() > 0)
        {
            for (int i = 0; i < m_position_spline.m_points.size(); i++)
                debug_draw.sphere(1.0f, m_position_spline.m_points[i], glm::vec3(0.0f, 0.0f, 1.0f));
        }

        if (m_position_spline.m_points.size() > 4)
        {
            for (int k = 0; k < m_debug_spline_segments.size() - 1; k++)
            {
                glm::vec3 p0 = m_debug_spline_segments[k];
                glm::vec3 p1 = m_debug_spline_segments[k + 1];

                debug_draw.line(p0, p1, glm::vec3(1.0f, 0.0f, 0.0f));
            }
        }
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

float DemoPlayer::speed()
{
    return m_speed;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void DemoPlayer::play()
{
    m_time       = 0.0f;
    m_is_playing = true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void DemoPlayer::stop()
{
    m_is_playing = false;
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool DemoPlayer::is_playing()
{
    return m_is_playing;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void DemoPlayer::update(float dt, Camera* camera)
{
    if (m_is_playing)
    {
        m_time += dt / 1000;
        m_current_position = m_position_spline.const_velocity_spline_at_time(m_time, m_speed);

        float t = float(m_position_spline.current_distance(m_time, m_speed)) / float(m_position_spline.total_length());

        m_current_forward = m_forward_spline.value_at_time(t);
        m_current_right   = m_right_spline.value_at_time(t);

        camera->update_from_frame(m_current_position, glm::normalize(m_current_forward), glm::normalize(m_current_right));
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

glm::vec3 DemoPlayer::position()
{
    return m_current_position;
}

// -----------------------------------------------------------------------------------------------------------------------------------

glm::vec3 DemoPlayer::forward()
{
    return m_current_forward;
}

// -----------------------------------------------------------------------------------------------------------------------------------

glm::vec3 DemoPlayer::right()
{
    return m_current_right;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void DemoPlayer::initialize_debug()
{
    float total_length = m_position_spline.total_length();

    m_debug_spline_segments.resize((m_position_spline.m_points.size() + 1) * SEGMENTS_PER_POINT);

    for (int k = 0; k < m_debug_spline_segments.size(); k++)
        m_debug_spline_segments[k] = m_position_spline.const_velocity_spline_at_time(total_length * k / m_debug_spline_segments.size() / m_speed, m_speed);
}

// -----------------------------------------------------------------------------------------------------------------------------------
} // namespace dw