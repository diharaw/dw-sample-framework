#pragma once

#include <stdint.h>

namespace dw
{
namespace aftermath
{
extern void gpu_crash_dump_callback(const void* _gpu_crash_dump, const uint32_t _gpu_crash_dump_size, void* _user_data);
extern void shader_debug_info_callback(const void* _shader_debug_info, const uint32_t _shader_debug_info_size, void* _user_data);
} // namespace aftermath
} // namespace dw