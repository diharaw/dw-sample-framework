#include "aftermath_callbacks.h"

#include <logger.h>
#include <utility.h>

#include <GFSDK_Aftermath.h>
#include <GFSDK_Aftermath_GpuCrashDump.h>
#include <GFSDK_Aftermath_GpuCrashDumpDecoding.h>

#include <vector>
#include <iomanip>

// -----------------------------------------------------------------------------------------------------------------------------------

namespace dw
{
namespace aftermath
{
void gpu_crash_dump_callback(const void* _gpu_crash_dump, const uint32_t _gpu_crash_dump_size, void* _user_data)
{
    // Create a GPU crash dump decoder object for the GPU crash dump.
    GFSDK_Aftermath_GpuCrashDump_Decoder decoder = {};

    if (GFSDK_Aftermath_GpuCrashDump_CreateDecoder(GFSDK_Aftermath_Version_API, _gpu_crash_dump, _gpu_crash_dump_size, &decoder) != GFSDK_Aftermath_Result_Success)
        DW_LOG_FATAL("Failed to create Nsight Aftermath decoder!");

    // Use the decoder object to read basic information, like application
    // name, PID, etc. from the GPU crash dump.
    GFSDK_Aftermath_GpuCrashDump_BaseInfo base_info = {};

    if (GFSDK_Aftermath_GpuCrashDump_GetBaseInfo(decoder, &base_info) != GFSDK_Aftermath_Result_Success)
        DW_LOG_FATAL("Failed to get Nsight Aftermath base info!");

    // Use the decoder object to query the application name that was set
    // in the GPU crash dump description.
    uint32_t    app_name_len = 0;
    std::string app_name     = "dwsf";

    auto result = GFSDK_Aftermath_GpuCrashDump_GetDescriptionSize(decoder, GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName, &app_name_len);

    if (result == GFSDK_Aftermath_Result_Success)
    {
        std::vector<char> app_name_buffer(app_name_len, '\0');

        if (GFSDK_Aftermath_GpuCrashDump_GetDescription(decoder, GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName, uint32_t(app_name_buffer.size()), app_name_buffer.data()) != GFSDK_Aftermath_Result_Success)
            DW_LOG_FATAL("Failed to get Nsight Aftermath description!");

        app_name = std::string(app_name_buffer.data());
    }

    // Create a unique file name for writing the crash dump data to a file.
    // Note: due to an Nsight Aftermath bug (will be fixed in an upcoming
    // driver release) we may see redundant crash dumps. As a workaround,
    // attach a unique count to each generated file name.
    static int        count          = 0;
    const std::string base_file_name = app_name + "-" + std::to_string(base_info.pid) + "-" + std::to_string(++count);

    // Write the crash dump data to a file using the .nv-gpudmp extension
    // registered with Nsight Graphics.
    const std::string crash_dump_filename = utility::executable_path() + "/" + base_file_name + ".nv-gpudmp";

    // Write crash dump to disk.
    FILE* fp = nullptr;

    fopen_s(&fp, crash_dump_filename.c_str(), "wb");

    if (fp)
    {
        fwrite((void*)_gpu_crash_dump, _gpu_crash_dump_size, 1, fp);

        fclose(fp);
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

void shader_debug_info_callback(const void* _shader_debug_info, const uint32_t _shader_debug_info_size, void* _user_data)
{
    static auto to_hex_string = [](uint64_t n) -> std::string
    {
        std::stringstream stream;
        stream << std::setfill('0') << std::setw(2 * sizeof(uint64_t)) << std::hex << n;
        return stream.str();
    };

    // Get shader debug information identifier
    GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier = {};

    if (GFSDK_Aftermath_GetShaderDebugInfoIdentifier(GFSDK_Aftermath_Version_API, _shader_debug_info, _shader_debug_info_size, &identifier) == GFSDK_Aftermath_Result_Success)
    {
        const std::string id_string = to_hex_string(identifier.id[0]) + "-" + to_hex_string(identifier.id[1]);
        const std::string file_path = utility::executable_path() + "/shader-" + id_string + ".nvdbg";

        FILE* fp = nullptr;

        fopen_s(&fp, file_path.c_str(), "wb");

        if (fp)
        {
            fwrite((void*)_shader_debug_info, _shader_debug_info_size, 1, fp);

            fclose(fp);
        }
    }
}
} // namespace aftermath
} // namespace dw