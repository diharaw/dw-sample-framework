#pragma once

#include <ogl.h>
#include <timer.h>
#include <memory>
#include <string>

#define DW_SCOPED_SAMPLE(name) dw::profiler::ScopedProfile __FILE__##__LINE__(name)

namespace dw
{
namespace profiler
{
struct ScopedProfile
{
    ScopedProfile(std::string name);
    ~ScopedProfile();

    std::string m_name;
};

extern void initialize();
extern void shutdown();
extern void begin_sample(std::string name);
extern void end_sample(std::string name);
extern void begin_frame();
extern void end_frame();
extern void ui();
}; // namespace profiler
} // namespace dw