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
#if defined(DWSF_VULKAN)
        uint32_t query_index;
#else
        gl::Query query;
#endif
        bool    start = true;
        double  cpu_time;
        Sample* end_sample;
    };

    struct Buffer
    {
        std::vector<std::unique_ptr<Sample>> samples;
        int32_t                              index = 0;
#if defined(DWSF_VULKAN)
        vk::QueryPool::Ptr query_pool;
        uint32_t           query_index = 0;
#endif

        Buffer()
        {
            samples.resize(MAX_SAMPLES);

            for (uint32_t i = 0; i < MAX_SAMPLES; i++)
                samples[i] = nullptr;
        }
    };

    // -----------------------------------------------------------------------------------------------------------------------------------

    Profiler(
#if defined(DWSF_VULKAN)
        vk::Backend::Ptr backend
#endif
    )
    {
#ifdef WIN32
        QueryPerformanceFrequency(&m_frequency);
#endif

#if defined(DWSF_VULKAN)
        for (int i = 0; i < BUFFER_COUNT; i++)
            m_sample_buffers[i].query_pool = vk::QueryPool::create(backend, VK_QUERY_TYPE_TIMESTAMP, MAX_SAMPLES);
#endif
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    ~Profiler()
    {
#if defined(DWSF_VULKAN)
        for (int i = 0; i < BUFFER_COUNT; i++)
            m_sample_buffers[i].query_pool.reset();
#endif
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void begin_sample(std::string name
#if defined(DWSF_VULKAN)
                      ,
                      vk::CommandBuffer::Ptr cmd_buf
#endif
    )
    {
#if defined(DWSF_VULKAN)
        if (m_should_reset)
        {
            m_sample_buffers[m_write_buffer_idx].query_index = 0;
            vkCmdResetQueryPool(cmd_buf->handle(), m_sample_buffers[m_write_buffer_idx].query_pool->handle(), 0, MAX_SAMPLES);
            m_should_reset = false;
        }
#endif

        int32_t idx = m_sample_buffers[m_write_buffer_idx].index++;

        if (!m_sample_buffers[m_write_buffer_idx].samples[idx])
            m_sample_buffers[m_write_buffer_idx].samples[idx] = std::make_unique<Sample>();

        auto& sample = m_sample_buffers[m_write_buffer_idx].samples[idx];

        sample->name = name;
#if defined(DWSF_VULKAN)
        sample->query_index = m_sample_buffers[m_write_buffer_idx].query_index++;
        vkCmdWriteTimestamp(cmd_buf->handle(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_sample_buffers[m_write_buffer_idx].query_pool->handle(), sample->query_index);
#else
        sample->query.query_counter(GL_TIMESTAMP);
#endif
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

    void end_sample(std::string name
#if defined(DWSF_VULKAN)
                    ,
                    vk::CommandBuffer::Ptr cmd_buf
#endif
    )
    {
        int32_t idx = m_sample_buffers[m_write_buffer_idx].index++;

        if (!m_sample_buffers[m_write_buffer_idx].samples[idx])
            m_sample_buffers[m_write_buffer_idx].samples[idx] = std::make_unique<Sample>();

        auto& sample = m_sample_buffers[m_write_buffer_idx].samples[idx];

        sample->name  = name;
        sample->start = false;
#if defined(DWSF_VULKAN)
        sample->query_index = m_sample_buffers[m_write_buffer_idx].query_index++;
        vkCmdWriteTimestamp(cmd_buf->handle(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_sample_buffers[m_write_buffer_idx].query_pool->handle(), sample->query_index);
#else
        sample->query.query_counter(GL_TIMESTAMP);
#endif
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
#if defined(DWSF_VULKAN)
        m_should_reset = true;
#endif
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

#if defined(DWSF_IMGUI)
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

#    if defined(DWSF_VULKAN)
                    m_sample_buffers[m_read_buffer_idx].query_pool->results(sample->query_index, 1, sizeof(uint64_t), &start_time, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);
                    m_sample_buffers[m_read_buffer_idx].query_pool->results(sample->end_sample->query_index, 1, sizeof(uint64_t), &end_time, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);
#    else
                    sample->query.result_64(&start_time);
                    sample->end_sample->query.result_64(&end_time);
#    endif

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
#endif

    // -----------------------------------------------------------------------------------------------------------------------------------

    int32_t             m_read_buffer_idx  = -3;
    int32_t             m_write_buffer_idx = -1;
    Buffer              m_sample_buffers[BUFFER_COUNT];
    std::stack<Sample*> m_sample_stack;
    std::stack<bool>    m_should_pop_stack;

#if defined(DWSF_VULKAN)
    bool m_should_reset = true;
#endif

#ifdef WIN32
    LARGE_INTEGER m_frequency;
#endif
};

Profiler* g_profiler = nullptr;

// -----------------------------------------------------------------------------------------------------------------------------------

ScopedProfile::ScopedProfile(std::string name
#if defined(DWSF_VULKAN)
                             ,
                             vk::CommandBuffer::Ptr cmd_buf
#endif
                             ) :
    m_name(name)
{
    begin_sample(m_name
#if defined(DWSF_VULKAN)
                 ,
                 cmd_buf
#endif
    );

#if defined(DWSF_VULKAN)
    m_cmd_buf = cmd_buf;
#endif
}

// -----------------------------------------------------------------------------------------------------------------------------------

ScopedProfile::~ScopedProfile()
{
    end_sample(m_name
#if defined(DWSF_VULKAN)
               ,
               m_cmd_buf
#endif
    );
}

// -----------------------------------------------------------------------------------------------------------------------------------

void initialize(
#if defined(DWSF_VULKAN)
    vk::Backend::Ptr backend
#endif
)
{
    g_profiler = new Profiler(
#if defined(DWSF_VULKAN)
        backend
#endif
    );
}

// -----------------------------------------------------------------------------------------------------------------------------------

void shutdown() { DW_SAFE_DELETE(g_profiler); }

// -----------------------------------------------------------------------------------------------------------------------------------

void begin_sample(std::string name
#if defined(DWSF_VULKAN)
                  ,
                  vk::CommandBuffer::Ptr cmd_buf
#endif
)
{
    g_profiler->begin_sample(name
#if defined(DWSF_VULKAN)
                             ,
                             cmd_buf
#endif
    );
}

// -----------------------------------------------------------------------------------------------------------------------------------

void end_sample(std::string name
#if defined(DWSF_VULKAN)
                ,
                vk::CommandBuffer::Ptr cmd_buf
#endif
)
{
    g_profiler->end_sample(name
#if defined(DWSF_VULKAN)
                           ,
                           cmd_buf
#endif
    );
}

// -----------------------------------------------------------------------------------------------------------------------------------

void begin_frame() { g_profiler->begin_frame(); }

// -----------------------------------------------------------------------------------------------------------------------------------

void end_frame() { g_profiler->end_frame(); }

// -----------------------------------------------------------------------------------------------------------------------------------

#if defined(DWSF_IMGUI)
void ui()
{
    g_profiler->ui();
}
#endif

// -----------------------------------------------------------------------------------------------------------------------------------
} // namespace profiler
} // namespace dw