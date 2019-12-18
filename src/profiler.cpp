#include <imgui.h>
#include <macros.h>
#include <profiler.h>
#include <stack>
#include <vector>

#define BUFFER_COUNT 3
#define MAX_SAMPLES 100

namespace dw
{
namespace profiler
{
// -----------------------------------------------------------------------------------------------------------------------------------

struct Profiler
{
    struct Sample
    {
        std::string name;
        gl::Query   query;
        bool        start = true;
        double      cpu_time;
        Sample*     end_sample;
    };

    struct Buffer
    {
        std::vector<std::unique_ptr<Sample>> samples;
        int32_t                              index = 0;

        Buffer()
        {
            samples.resize(MAX_SAMPLES);

            for (uint32_t i = 0; i < MAX_SAMPLES; i++)
                samples[i] = nullptr;
        }
    };

    // -----------------------------------------------------------------------------------------------------------------------------------

    Profiler()
    {
#ifdef WIN32
        QueryPerformanceFrequency(&m_frequency);
#endif
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void begin_sample(std::string name)
    {
        int32_t idx = m_sample_buffers[m_write_buffer_idx].index++;

        if (!m_sample_buffers[m_write_buffer_idx].samples[idx])
            m_sample_buffers[m_write_buffer_idx].samples[idx] = std::make_unique<Sample>();

        auto& sample = m_sample_buffers[m_write_buffer_idx].samples[idx];

        sample->name = name;
        sample->query.query_counter(GL_TIMESTAMP);
        sample->end_sample = nullptr;
        sample->start      = true;

#ifdef WIN32
        LARGE_INTEGER cpu_time;
        QueryPerformanceCounter(&cpu_time);
        sample->cpu_time = cpu_time.QuadPart * (1000000.0 / m_frequency.QuadPart);
#else
        timeval cpu_time;
        gettimeofday(&cpu_time, nullptr);
        sample->cpu_time = (cpu_time.tv_sec * 1000000.0) + cpu_time.tv_usec;
#endif

        m_sample_stack.push(sample.get());
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void end_sample(std::string name)
    {
        int32_t idx = m_sample_buffers[m_write_buffer_idx].index++;

        if (!m_sample_buffers[m_write_buffer_idx].samples[idx])
            m_sample_buffers[m_write_buffer_idx].samples[idx] = std::make_unique<Sample>();

        auto& sample = m_sample_buffers[m_write_buffer_idx].samples[idx];

        sample->name  = name;
        sample->start = false;
        sample->query.query_counter(GL_TIMESTAMP);
        sample->end_sample = nullptr;

#ifdef WIN32
        LARGE_INTEGER cpu_time;
        QueryPerformanceCounter(&cpu_time);
        sample->cpu_time = cpu_time.QuadPart * (1000000.0 / m_frequency.QuadPart);
#else
        timeval cpu_time;
        gettimeofday(&cpu_time, nullptr);
        sample->cpu_time = (cpu_time.tv_sec * 1000000.0) + cpu_time.tv_usec;
#endif

        Sample* start = m_sample_stack.top();

        start->end_sample = sample.get();

        m_sample_stack.pop();
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void begin_frame()
    {
        m_read_buffer_idx++;
        m_write_buffer_idx++;

        if (m_read_buffer_idx == 3)
            m_read_buffer_idx = 0;

        if (m_write_buffer_idx == 3)
            m_write_buffer_idx = 0;
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void end_frame()
    {
        if (m_read_buffer_idx >= 0)
            m_sample_buffers[m_read_buffer_idx].index = 0;
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void ui()
    {
        if (m_read_buffer_idx >= 0)
        {
            for (int32_t i = 0; i < m_sample_buffers[m_read_buffer_idx].index; i++)
            {
                auto& sample = m_sample_buffers[m_read_buffer_idx].samples[i];

                if (sample->start)
                {
                    if (!m_should_pop_stack.empty())
                    {
                        if (!m_should_pop_stack.top())
                        {
                            m_should_pop_stack.push(false);
                            continue;
                        }
                    }

                    std::string id = std::to_string(i);

                    uint64_t start_time = 0;
                    uint64_t end_time   = 0;

                    sample->query.result_64(&start_time);
                    sample->end_sample->query.result_64(&end_time);

                    uint64_t gpu_time_diff = end_time - start_time;

                    float gpu_time = float(gpu_time_diff / 1000000.0);
                    float cpu_time = (sample->end_sample->cpu_time - sample->cpu_time) * 0.001f;

                    if (ImGui::TreeNode(id.c_str(), "%s | %f ms (CPU) | %f ms (GPU)", sample->name.c_str(), cpu_time, gpu_time))
                        m_should_pop_stack.push(true);
                    else
                        m_should_pop_stack.push(false);
                }
                else
                {
                    if (!m_should_pop_stack.empty())
                    {
                        bool should_pop = m_should_pop_stack.top();
                        m_should_pop_stack.pop();

                        if (should_pop)
                            ImGui::TreePop();
                    }
                }
            }
        }
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    int32_t             m_read_buffer_idx  = -3;
    int32_t             m_write_buffer_idx = -1;
    Buffer              m_sample_buffers[BUFFER_COUNT];
    std::stack<Sample*> m_sample_stack;
    std::stack<bool>    m_should_pop_stack;

#ifdef WIN32
    LARGE_INTEGER m_frequency;
#endif
};

Profiler* g_profiler = nullptr;

// -----------------------------------------------------------------------------------------------------------------------------------

ScopedProfile::ScopedProfile(std::string name) :
    m_name(name)
{
    begin_sample(m_name);
}

// -----------------------------------------------------------------------------------------------------------------------------------

ScopedProfile::~ScopedProfile() { end_sample(m_name); }

// -----------------------------------------------------------------------------------------------------------------------------------

void initialize() { g_profiler = new Profiler(); }

// -----------------------------------------------------------------------------------------------------------------------------------

void shutdown() { DW_SAFE_DELETE(g_profiler); }

// -----------------------------------------------------------------------------------------------------------------------------------

void begin_sample(std::string name) { g_profiler->begin_sample(name); }

// -----------------------------------------------------------------------------------------------------------------------------------

void end_sample(std::string name) { g_profiler->end_sample(name); }

// -----------------------------------------------------------------------------------------------------------------------------------

void begin_frame() { g_profiler->begin_frame(); }

// -----------------------------------------------------------------------------------------------------------------------------------

void end_frame() { g_profiler->end_frame(); }

// -----------------------------------------------------------------------------------------------------------------------------------

void ui() { g_profiler->ui(); }

// -----------------------------------------------------------------------------------------------------------------------------------
} // namespace profiler
} // namespace dw