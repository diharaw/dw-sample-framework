#include "equirectangular_to_cubemap.h"
#include <glm.hpp>
#include <algorithm>
#include <macros.h>
#include <logger.h>
#include <profiler.h>
#include <gtc/matrix_transform.hpp>
#include <vk_mem_alloc.h>

#define _USE_MATH_DEFINES
#include <math.h>

#undef min
#undef max

namespace dw
{
// -----------------------------------------------------------------------------------------------------------------------------------

#if defined(DWSF_VULKAN)
static const unsigned int kCONVERT_VERT_SPIRV_size           = 1280;
static const unsigned int kCONVERT_VERT_SPIRV_data[1280 / 4] = {
    0x07230203,
    0x00010000,
    0x000d000a,
    0x00000026,
    0x00000000,
    0x00020011,
    0x00000001,
    0x0006000b,
    0x00000001,
    0x4c534c47,
    0x6474732e,
    0x3035342e,
    0x00000000,
    0x0003000e,
    0x00000000,
    0x00000001,
    0x0008000f,
    0x00000000,
    0x00000004,
    0x6e69616d,
    0x00000000,
    0x00000009,
    0x0000000b,
    0x00000013,
    0x00030003,
    0x00000002,
    0x000001c2,
    0x000a0004,
    0x475f4c47,
    0x4c474f4f,
    0x70635f45,
    0x74735f70,
    0x5f656c79,
    0x656e696c,
    0x7269645f,
    0x69746365,
    0x00006576,
    0x00080004,
    0x475f4c47,
    0x4c474f4f,
    0x6e695f45,
    0x64756c63,
    0x69645f65,
    0x74636572,
    0x00657669,
    0x00040005,
    0x00000004,
    0x6e69616d,
    0x00000000,
    0x00060005,
    0x00000009,
    0x495f5346,
    0x6f575f4e,
    0x50646c72,
    0x0000736f,
    0x00060005,
    0x0000000b,
    0x495f5356,
    0x6f505f4e,
    0x69746973,
    0x00006e6f,
    0x00060005,
    0x00000011,
    0x505f6c67,
    0x65567265,
    0x78657472,
    0x00000000,
    0x00060006,
    0x00000011,
    0x00000000,
    0x505f6c67,
    0x7469736f,
    0x006e6f69,
    0x00070006,
    0x00000011,
    0x00000001,
    0x505f6c67,
    0x746e696f,
    0x657a6953,
    0x00000000,
    0x00070006,
    0x00000011,
    0x00000002,
    0x435f6c67,
    0x4470696c,
    0x61747369,
    0x0065636e,
    0x00070006,
    0x00000011,
    0x00000003,
    0x435f6c67,
    0x446c6c75,
    0x61747369,
    0x0065636e,
    0x00030005,
    0x00000013,
    0x00000000,
    0x00060005,
    0x00000017,
    0x68737550,
    0x736e6f43,
    0x746e6174,
    0x00000073,
    0x00060006,
    0x00000017,
    0x00000000,
    0x77656976,
    0x6f72705f,
    0x0000006a,
    0x00060005,
    0x00000019,
    0x75505f75,
    0x6f436873,
    0x6174736e,
    0x0073746e,
    0x00040047,
    0x00000009,
    0x0000001e,
    0x00000000,
    0x00040047,
    0x0000000b,
    0x0000001e,
    0x00000000,
    0x00050048,
    0x00000011,
    0x00000000,
    0x0000000b,
    0x00000000,
    0x00050048,
    0x00000011,
    0x00000001,
    0x0000000b,
    0x00000001,
    0x00050048,
    0x00000011,
    0x00000002,
    0x0000000b,
    0x00000003,
    0x00050048,
    0x00000011,
    0x00000003,
    0x0000000b,
    0x00000004,
    0x00030047,
    0x00000011,
    0x00000002,
    0x00040048,
    0x00000017,
    0x00000000,
    0x00000005,
    0x00050048,
    0x00000017,
    0x00000000,
    0x00000023,
    0x00000000,
    0x00050048,
    0x00000017,
    0x00000000,
    0x00000007,
    0x00000010,
    0x00030047,
    0x00000017,
    0x00000002,
    0x00020013,
    0x00000002,
    0x00030021,
    0x00000003,
    0x00000002,
    0x00030016,
    0x00000006,
    0x00000020,
    0x00040017,
    0x00000007,
    0x00000006,
    0x00000003,
    0x00040020,
    0x00000008,
    0x00000003,
    0x00000007,
    0x0004003b,
    0x00000008,
    0x00000009,
    0x00000003,
    0x00040020,
    0x0000000a,
    0x00000001,
    0x00000007,
    0x0004003b,
    0x0000000a,
    0x0000000b,
    0x00000001,
    0x00040017,
    0x0000000d,
    0x00000006,
    0x00000004,
    0x00040015,
    0x0000000e,
    0x00000020,
    0x00000000,
    0x0004002b,
    0x0000000e,
    0x0000000f,
    0x00000001,
    0x0004001c,
    0x00000010,
    0x00000006,
    0x0000000f,
    0x0006001e,
    0x00000011,
    0x0000000d,
    0x00000006,
    0x00000010,
    0x00000010,
    0x00040020,
    0x00000012,
    0x00000003,
    0x00000011,
    0x0004003b,
    0x00000012,
    0x00000013,
    0x00000003,
    0x00040015,
    0x00000014,
    0x00000020,
    0x00000001,
    0x0004002b,
    0x00000014,
    0x00000015,
    0x00000000,
    0x00040018,
    0x00000016,
    0x0000000d,
    0x00000004,
    0x0003001e,
    0x00000017,
    0x00000016,
    0x00040020,
    0x00000018,
    0x00000009,
    0x00000017,
    0x0004003b,
    0x00000018,
    0x00000019,
    0x00000009,
    0x00040020,
    0x0000001a,
    0x00000009,
    0x00000016,
    0x0004002b,
    0x00000006,
    0x0000001e,
    0x3f800000,
    0x00040020,
    0x00000024,
    0x00000003,
    0x0000000d,
    0x00050036,
    0x00000002,
    0x00000004,
    0x00000000,
    0x00000003,
    0x000200f8,
    0x00000005,
    0x0004003d,
    0x00000007,
    0x0000000c,
    0x0000000b,
    0x0003003e,
    0x00000009,
    0x0000000c,
    0x00050041,
    0x0000001a,
    0x0000001b,
    0x00000019,
    0x00000015,
    0x0004003d,
    0x00000016,
    0x0000001c,
    0x0000001b,
    0x0004003d,
    0x00000007,
    0x0000001d,
    0x0000000b,
    0x00050051,
    0x00000006,
    0x0000001f,
    0x0000001d,
    0x00000000,
    0x00050051,
    0x00000006,
    0x00000020,
    0x0000001d,
    0x00000001,
    0x00050051,
    0x00000006,
    0x00000021,
    0x0000001d,
    0x00000002,
    0x00070050,
    0x0000000d,
    0x00000022,
    0x0000001f,
    0x00000020,
    0x00000021,
    0x0000001e,
    0x00050091,
    0x0000000d,
    0x00000023,
    0x0000001c,
    0x00000022,
    0x00050041,
    0x00000024,
    0x00000025,
    0x00000013,
    0x00000015,
    0x0003003e,
    0x00000025,
    0x00000023,
    0x000100fd,
    0x00010038,
};

// -----------------------------------------------------------------------------------------------------------------------------------

static const unsigned int kCONVERT_FRAG_SPIRV_size           = 1544;
static const unsigned int kCONVERT_FRAG_SPIRV_data[1544 / 4] = {
    0x07230203,
    0x00010000,
    0x000d000a,
    0x0000003e,
    0x00000000,
    0x00020011,
    0x00000001,
    0x0006000b,
    0x00000001,
    0x4c534c47,
    0x6474732e,
    0x3035342e,
    0x00000000,
    0x0003000e,
    0x00000000,
    0x00000001,
    0x0007000f,
    0x00000004,
    0x00000004,
    0x6e69616d,
    0x00000000,
    0x0000002c,
    0x0000003c,
    0x00030010,
    0x00000004,
    0x00000007,
    0x00030003,
    0x00000002,
    0x000001c2,
    0x000a0004,
    0x475f4c47,
    0x4c474f4f,
    0x70635f45,
    0x74735f70,
    0x5f656c79,
    0x656e696c,
    0x7269645f,
    0x69746365,
    0x00006576,
    0x00080004,
    0x475f4c47,
    0x4c474f4f,
    0x6e695f45,
    0x64756c63,
    0x69645f65,
    0x74636572,
    0x00657669,
    0x00040005,
    0x00000004,
    0x6e69616d,
    0x00000000,
    0x00090005,
    0x0000000c,
    0x706d6173,
    0x735f656c,
    0x72656870,
    0x6c616369,
    0x70616d5f,
    0x33667628,
    0x0000003b,
    0x00030005,
    0x0000000b,
    0x00000076,
    0x00030005,
    0x0000000f,
    0x00007675,
    0x00030005,
    0x0000002a,
    0x00007675,
    0x00060005,
    0x0000002c,
    0x495f5346,
    0x6f575f4e,
    0x50646c72,
    0x0000736f,
    0x00040005,
    0x0000002f,
    0x61726170,
    0x0000006d,
    0x00040005,
    0x00000031,
    0x6f6c6f63,
    0x00000072,
    0x00050005,
    0x00000035,
    0x6e455f73,
    0x70614d76,
    0x00000000,
    0x00060005,
    0x0000003c,
    0x4f5f5346,
    0x435f5455,
    0x726f6c6f,
    0x00000000,
    0x00040047,
    0x0000002c,
    0x0000001e,
    0x00000000,
    0x00040047,
    0x00000035,
    0x00000022,
    0x00000000,
    0x00040047,
    0x00000035,
    0x00000021,
    0x00000000,
    0x00040047,
    0x0000003c,
    0x0000001e,
    0x00000000,
    0x00020013,
    0x00000002,
    0x00030021,
    0x00000003,
    0x00000002,
    0x00030016,
    0x00000006,
    0x00000020,
    0x00040017,
    0x00000007,
    0x00000006,
    0x00000003,
    0x00040020,
    0x00000008,
    0x00000007,
    0x00000007,
    0x00040017,
    0x00000009,
    0x00000006,
    0x00000002,
    0x00040021,
    0x0000000a,
    0x00000009,
    0x00000008,
    0x00040020,
    0x0000000e,
    0x00000007,
    0x00000009,
    0x00040015,
    0x00000010,
    0x00000020,
    0x00000000,
    0x0004002b,
    0x00000010,
    0x00000011,
    0x00000002,
    0x00040020,
    0x00000012,
    0x00000007,
    0x00000006,
    0x0004002b,
    0x00000010,
    0x00000015,
    0x00000000,
    0x0004002b,
    0x00000010,
    0x00000019,
    0x00000001,
    0x0004002b,
    0x00000006,
    0x0000001e,
    0x3e22eb1c,
    0x0004002b,
    0x00000006,
    0x0000001f,
    0x3ea2f838,
    0x0005002c,
    0x00000009,
    0x00000020,
    0x0000001e,
    0x0000001f,
    0x0004002b,
    0x00000006,
    0x00000023,
    0x3f000000,
    0x00040020,
    0x0000002b,
    0x00000001,
    0x00000007,
    0x0004003b,
    0x0000002b,
    0x0000002c,
    0x00000001,
    0x00090019,
    0x00000032,
    0x00000006,
    0x00000001,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000001,
    0x00000000,
    0x0003001b,
    0x00000033,
    0x00000032,
    0x00040020,
    0x00000034,
    0x00000000,
    0x00000033,
    0x0004003b,
    0x00000034,
    0x00000035,
    0x00000000,
    0x00040017,
    0x00000038,
    0x00000006,
    0x00000004,
    0x00040020,
    0x0000003b,
    0x00000003,
    0x00000007,
    0x0004003b,
    0x0000003b,
    0x0000003c,
    0x00000003,
    0x00050036,
    0x00000002,
    0x00000004,
    0x00000000,
    0x00000003,
    0x000200f8,
    0x00000005,
    0x0004003b,
    0x0000000e,
    0x0000002a,
    0x00000007,
    0x0004003b,
    0x00000008,
    0x0000002f,
    0x00000007,
    0x0004003b,
    0x00000008,
    0x00000031,
    0x00000007,
    0x0004003d,
    0x00000007,
    0x0000002d,
    0x0000002c,
    0x0006000c,
    0x00000007,
    0x0000002e,
    0x00000001,
    0x00000045,
    0x0000002d,
    0x0003003e,
    0x0000002f,
    0x0000002e,
    0x00050039,
    0x00000009,
    0x00000030,
    0x0000000c,
    0x0000002f,
    0x0003003e,
    0x0000002a,
    0x00000030,
    0x0004003d,
    0x00000033,
    0x00000036,
    0x00000035,
    0x0004003d,
    0x00000009,
    0x00000037,
    0x0000002a,
    0x00050057,
    0x00000038,
    0x00000039,
    0x00000036,
    0x00000037,
    0x0008004f,
    0x00000007,
    0x0000003a,
    0x00000039,
    0x00000039,
    0x00000000,
    0x00000001,
    0x00000002,
    0x0003003e,
    0x00000031,
    0x0000003a,
    0x0004003d,
    0x00000007,
    0x0000003d,
    0x00000031,
    0x0003003e,
    0x0000003c,
    0x0000003d,
    0x000100fd,
    0x00010038,
    0x00050036,
    0x00000009,
    0x0000000c,
    0x00000000,
    0x0000000a,
    0x00030037,
    0x00000008,
    0x0000000b,
    0x000200f8,
    0x0000000d,
    0x0004003b,
    0x0000000e,
    0x0000000f,
    0x00000007,
    0x00050041,
    0x00000012,
    0x00000013,
    0x0000000b,
    0x00000011,
    0x0004003d,
    0x00000006,
    0x00000014,
    0x00000013,
    0x00050041,
    0x00000012,
    0x00000016,
    0x0000000b,
    0x00000015,
    0x0004003d,
    0x00000006,
    0x00000017,
    0x00000016,
    0x0007000c,
    0x00000006,
    0x00000018,
    0x00000001,
    0x00000019,
    0x00000014,
    0x00000017,
    0x00050041,
    0x00000012,
    0x0000001a,
    0x0000000b,
    0x00000019,
    0x0004003d,
    0x00000006,
    0x0000001b,
    0x0000001a,
    0x0006000c,
    0x00000006,
    0x0000001c,
    0x00000001,
    0x00000010,
    0x0000001b,
    0x00050050,
    0x00000009,
    0x0000001d,
    0x00000018,
    0x0000001c,
    0x0003003e,
    0x0000000f,
    0x0000001d,
    0x0004003d,
    0x00000009,
    0x00000021,
    0x0000000f,
    0x00050085,
    0x00000009,
    0x00000022,
    0x00000021,
    0x00000020,
    0x0003003e,
    0x0000000f,
    0x00000022,
    0x0004003d,
    0x00000009,
    0x00000024,
    0x0000000f,
    0x00050050,
    0x00000009,
    0x00000025,
    0x00000023,
    0x00000023,
    0x00050081,
    0x00000009,
    0x00000026,
    0x00000024,
    0x00000025,
    0x0003003e,
    0x0000000f,
    0x00000026,
    0x0004003d,
    0x00000009,
    0x00000027,
    0x0000000f,
    0x000200fe,
    0x00000027,
    0x00010038,
};

#else
static const char* g_vs_src = R"(
layout(location = 0) in vec3 VS_IN_Position;

// ------------------------------------------------------------------
// OUTPUT VARIABLES  ------------------------------------------------
// ------------------------------------------------------------------

out vec3 FS_IN_WorldPos;

// ------------------------------------------------------------------
// UNIFORMS  --------------------------------------------------------
// ------------------------------------------------------------------

uniform mat4 u_ViewProj;

// ------------------------------------------------------------------
// MAIN  ------------------------------------------------------------
// ------------------------------------------------------------------

void main(void)
{
    FS_IN_WorldPos = VS_IN_Position;
    gl_Position    = u_ViewProj * vec4(VS_IN_Position, 1.0);
}

// ------------------------------------------------------------------
)";

static const char* g_fs_src = R"(
// ------------------------------------------------------------------
// INPUT VARIABLES  -------------------------------------------------
// ------------------------------------------------------------------

in vec3 FS_IN_WorldPos;

// ------------------------------------------------------------------
// OUTPUT VARIABLES  ------------------------------------------------
// ------------------------------------------------------------------

out vec3 FS_OUT_Color;

// ------------------------------------------------------------------
// SAMPLERS ---------------------------------------------------------
// ------------------------------------------------------------------

uniform sampler2D s_EnvMap;

// ------------------------------------------------------------------
// FUNCTIONS  -------------------------------------------------------
// ------------------------------------------------------------------

const vec2 kInvATan = vec2(0.1591, 0.3183);

vec2 sample_spherical_map(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= kInvATan;
    uv += 0.5;
    return uv;
}

// ------------------------------------------------------------------
// MAIN  ------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    vec2 uv    = sample_spherical_map(normalize(FS_IN_WorldPos));
    vec3 color = texture(s_EnvMap, uv).rgb;

    FS_OUT_Color = color;
}

// ------------------------------------------------------------------
)";
#endif

// -----------------------------------------------------------------------------------------------------------------------------------

EquirectangularToCubemap::EquirectangularToCubemap(
#if defined(DWSF_VULKAN)
    vk::Backend::Ptr backend,
    VkFormat         image_format
#endif
)
{
    glm::mat4 capture_projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 capture_views[]    = {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
    };

    m_view_projection_mats.resize(6);

    for (int i = 0; i < 6; i++)
        m_view_projection_mats[i] = capture_projection * capture_views[i];

    float cube_vertices[] = {
        // back face
        -1.0f,
        -1.0f,
        -1.0f,
        // bottom-left
        1.0f,
        1.0f,
        -1.0f,
        // top-right
        1.0f,
        -1.0f,
        -1.0f,
        // bottom-right
        1.0f,
        1.0f,
        -1.0f,
        // top-right
        -1.0f,
        -1.0f,
        -1.0f,
        // bottom-left
        -1.0f,
        1.0f,
        -1.0f,
        // top-left
        // front face
        -1.0f,
        -1.0f,
        1.0f,
        // bottom-left
        1.0f,
        -1.0f,
        1.0f,
        // bottom-right
        1.0f,
        1.0f,
        1.0f,
        // top-right
        1.0f,
        1.0f,
        1.0f,
        // top-right
        -1.0f,
        1.0f,
        1.0f,
        // top-left
        -1.0f,
        -1.0f,
        1.0f,
        // bottom-left
        // left face
        -1.0f,
        1.0f,
        1.0f,
        // top-right
        -1.0f,
        1.0f,
        -1.0f,
        // top-left
        -1.0f,
        -1.0f,
        -1.0f,
        // bottom-left
        -1.0f,
        -1.0f,
        -1.0f,
        // bottom-left
        -1.0f,
        -1.0f,
        1.0f,
        // bottom-right
        -1.0f,
        1.0f,
        1.0f,
        // top-right
        // right face
        1.0f,
        1.0f,
        1.0f,
        // top-left
        1.0f,
        -1.0f,
        -1.0f,
        // bottom-right
        1.0f,
        1.0f,
        -1.0f,
        // top-right
        1.0f,
        -1.0f,
        -1.0f,
        // bottom-right
        1.0f,
        1.0f,
        1.0f,
        // top-left
        1.0f,
        -1.0f,
        1.0f,
        // bottom-left
        // bottom face
        -1.0f,
        -1.0f,
        -1.0f,
        // top-right
        1.0f,
        -1.0f,
        -1.0f,
        // top-left
        1.0f,
        -1.0f,
        1.0f,
        // bottom-left
        1.0f,
        -1.0f,
        1.0f,
        // bottom-left
        -1.0f,
        -1.0f,
        1.0f,
        // bottom-right
        -1.0f,
        -1.0f,
        -1.0f,
        // top-right
        // top face
        -1.0f,
        1.0f,
        -1.0f,
        // top-left
        1.0f,
        1.0f,
        1.0f,
        // bottom-right
        1.0f,
        1.0f,
        -1.0f,
        // top-right
        1.0f,
        1.0f,
        1.0f,
        // bottom-right
        -1.0f,
        1.0f,
        -1.0f,
        // top-left
        -1.0f,
        1.0f,
        1.0f // bottom-left
    };

#if defined(DWSF_VULKAN)
    m_backend = backend;

    VkAttachmentDescription attachment;
    DW_ZERO_MEMORY(attachment);

    // Color attachment
    attachment.format         = image_format;
    attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference color_reference;
    color_reference.attachment = 0;
    color_reference.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    std::vector<VkSubpassDescription> subpass_description(1);

    subpass_description[0].pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description[0].colorAttachmentCount    = 1;
    subpass_description[0].pColorAttachments       = &color_reference;
    subpass_description[0].pDepthStencilAttachment = nullptr;
    subpass_description[0].inputAttachmentCount    = 0;
    subpass_description[0].pInputAttachments       = nullptr;
    subpass_description[0].preserveAttachmentCount = 0;
    subpass_description[0].pPreserveAttachments    = nullptr;
    subpass_description[0].pResolveAttachments     = nullptr;

    // Subpass dependencies for layout transitions
    std::vector<VkSubpassDependency> dependencies(2);

    dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass      = 0;
    dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass      = 0;
    dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    m_cubemap_renderpass = vk::RenderPass::create(backend, { attachment }, subpass_description, dependencies);
    m_cube_vbo           = vk::Buffer::create(backend, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, sizeof(cube_vertices), VMA_MEMORY_USAGE_GPU_ONLY, 0, cube_vertices);

    vk::DescriptorSetLayout::Desc ds_layout_desc;

    ds_layout_desc.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);

    m_ds_layout = vk::DescriptorSetLayout::create(backend, ds_layout_desc);

    // ---------------------------------------------------------------------------
    // Create shader modules
    // ---------------------------------------------------------------------------

    std::vector<char> vert_spirv;

    vert_spirv.resize(kCONVERT_VERT_SPIRV_size);
    memcpy(&vert_spirv[0], &kCONVERT_VERT_SPIRV_data[0], kCONVERT_VERT_SPIRV_size);

    std::vector<char> frag_spirv;

    frag_spirv.resize(kCONVERT_FRAG_SPIRV_size);
    memcpy(&frag_spirv[0], &kCONVERT_FRAG_SPIRV_data[0], kCONVERT_FRAG_SPIRV_size);

    vk::ShaderModule::Ptr vs = vk::ShaderModule::create(backend, vert_spirv);
    vk::ShaderModule::Ptr fs = vk::ShaderModule::create(backend, frag_spirv);

    vk::GraphicsPipeline::Desc pso_desc;

    pso_desc.add_shader_stage(VK_SHADER_STAGE_VERTEX_BIT, vs, "main")
        .add_shader_stage(VK_SHADER_STAGE_FRAGMENT_BIT, fs, "main");

    // ---------------------------------------------------------------------------
    // Create vertex input state
    // ---------------------------------------------------------------------------

    vk::VertexInputStateDesc vertex_input_state_desc;

    vertex_input_state_desc.add_binding_desc(0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX);
    vertex_input_state_desc.add_attribute_desc(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);

    pso_desc.set_vertex_input_state(vertex_input_state_desc);

    // ---------------------------------------------------------------------------
    // Create pipeline input assembly state
    // ---------------------------------------------------------------------------

    vk::InputAssemblyStateDesc input_assembly_state_desc;

    input_assembly_state_desc.set_primitive_restart_enable(false);

    // ---------------------------------------------------------------------------
    // Create viewport state
    // ---------------------------------------------------------------------------

    vk::ViewportStateDesc vp_desc;

    vp_desc.add_viewport(0.0f, 0.0f, 1024, 1024, 0.0f, 1.0f)
        .add_scissor(0, 0, 1024, 1024);

    pso_desc.set_viewport_state(vp_desc);

    // ---------------------------------------------------------------------------
    // Create rasterization state
    // ---------------------------------------------------------------------------

    vk::RasterizationStateDesc rs_state;

    rs_state.set_depth_clamp(VK_FALSE)
        .set_rasterizer_discard_enable(VK_FALSE)
        .set_polygon_mode(VK_POLYGON_MODE_FILL)
        .set_line_width(1.0f)
        .set_cull_mode(VK_CULL_MODE_NONE)
        .set_front_face(VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .set_depth_bias(VK_FALSE);

    pso_desc.set_rasterization_state(rs_state);

    // ---------------------------------------------------------------------------
    // Create multisample state
    // ---------------------------------------------------------------------------

    vk::MultisampleStateDesc ms_state;

    ms_state.set_sample_shading_enable(VK_FALSE)
        .set_rasterization_samples(VK_SAMPLE_COUNT_1_BIT);

    pso_desc.set_multisample_state(ms_state);

    // ---------------------------------------------------------------------------
    // Create depth stencil state
    // ---------------------------------------------------------------------------

    vk::DepthStencilStateDesc ds_state;

    ds_state.set_depth_test_enable(VK_FALSE)
        .set_depth_write_enable(VK_FALSE)
        .set_depth_compare_op(VK_COMPARE_OP_LESS)
        .set_depth_bounds_test_enable(VK_FALSE)
        .set_stencil_test_enable(VK_FALSE);

    pso_desc.set_depth_stencil_state(ds_state);

    // ---------------------------------------------------------------------------
    // Create color blend state
    // ---------------------------------------------------------------------------

    vk::ColorBlendAttachmentStateDesc blend_att_desc;

    blend_att_desc.set_color_write_mask(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT)
        .set_src_color_blend_factor(VK_BLEND_FACTOR_SRC_ALPHA)
        .set_dst_color_blend_Factor(VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA)
        .set_src_alpha_blend_factor(VK_BLEND_FACTOR_ONE)
        .set_dst_alpha_blend_factor(VK_BLEND_FACTOR_ZERO)
        .set_color_blend_op(VK_BLEND_OP_ADD)
        .set_blend_enable(VK_FALSE);

    vk::ColorBlendStateDesc blend_state;

    blend_state.set_logic_op_enable(VK_FALSE)
        .set_logic_op(VK_LOGIC_OP_COPY)
        .set_blend_constants(0.0f, 0.0f, 0.0f, 0.0f)
        .add_attachment(blend_att_desc);

    pso_desc.set_color_blend_state(blend_state);

    // ---------------------------------------------------------------------------
    // Create pipeline layout
    // ---------------------------------------------------------------------------

    vk::PipelineLayout::Desc pl_desc;

    pl_desc.add_descriptor_set_layout(m_ds_layout);
    pl_desc.add_push_constant_range(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::mat4));

    m_cubemap_pipeline_layout = vk::PipelineLayout::create(backend, pl_desc);

    pso_desc.set_pipeline_layout(m_cubemap_pipeline_layout);

    // ---------------------------------------------------------------------------
    // Create dynamic state
    // ---------------------------------------------------------------------------

    pso_desc.add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT)
        .add_dynamic_state(VK_DYNAMIC_STATE_SCISSOR);

    pso_desc.set_render_pass(m_cubemap_renderpass);

    // ---------------------------------------------------------------------------
    // Create line list pipeline
    // ---------------------------------------------------------------------------

    input_assembly_state_desc.set_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    pso_desc.set_input_assembly_state(input_assembly_state_desc);

    m_cubemap_pipeline = vk::GraphicsPipeline::create(backend, pso_desc);

#else
    m_vs = gl::Shader::create(GL_VERTEX_SHADER, g_vs_src);
    m_fs = gl::Shader::create(GL_FRAGMENT_SHADER, g_fs_src);

    if (!m_vs->compiled() || !m_fs->compiled())
        DW_LOG_FATAL("Failed to create Shaders");

    // Create general shader program
    m_program = gl::Program::create({ m_vs, m_fs });

    if (!m_program)
        DW_LOG_FATAL("Failed to create Shader Program");

    m_vbo = gl::VertexBuffer::create(GL_STATIC_DRAW, sizeof(cube_vertices), cube_vertices);

    if (!m_vbo)
        DW_LOG_ERROR("Failed to create Vertex Buffer");

    // Declare vertex attributes.
    gl::VertexAttrib attribs[] = {
        { 3, GL_FLOAT, false, 0 },
        { 3, GL_FLOAT, false, (3 * sizeof(float)) },
        { 2, GL_FLOAT, false, (6 * sizeof(float)) }
    };

    // Create vertex array.
    m_vao = gl::VertexArray::create(m_vbo, nullptr, (8 * sizeof(float)), 3, attribs);

#endif
}

// -----------------------------------------------------------------------------------------------------------------------------------

EquirectangularToCubemap::~EquirectangularToCubemap()
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

void EquirectangularToCubemap::convert(
#if defined(DWSF_VULKAN)
    vk::Image::Ptr input_image,
    vk::Image::Ptr output_image
#else
    gl::Texture2D::Ptr input_image,
    gl::Texture2D::Ptr output_image
#endif
)
{
#if defined(DWSF_VULKAN)
    auto backend = m_backend.lock();

    auto ds               = backend->allocate_descriptor_set(m_ds_layout);
    auto input_image_view = vk::ImageView::create(backend, input_image, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1);

    VkDescriptorImageInfo image_info;
    DW_ZERO_MEMORY(image_info);

    image_info.sampler     = backend->bilinear_sampler()->handle();
    image_info.imageView   = input_image_view->handle();
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write_data;
    DW_ZERO_MEMORY(write_data);

    write_data.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_data.descriptorCount = 1;
    write_data.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_data.pImageInfo      = &image_info;
    write_data.dstBinding      = 0;
    write_data.dstSet          = ds->handle();

    vkUpdateDescriptorSets(backend->device(), 1, &write_data, 0, nullptr);

    std::vector<vk::ImageView::Ptr>   face_image_views(6);
    std::vector<vk::Framebuffer::Ptr> face_framebuffers(6);

    for (int i = 0; i < 6; i++)
    {
        face_image_views[i]  = vk::ImageView::create(backend, output_image, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, i, 1);
        face_framebuffers[i] = vk::Framebuffer::create(backend, m_cubemap_renderpass, { face_image_views[i] }, output_image->width(), output_image->height(), 1);
    }

    auto cmd_buf = backend->allocate_graphics_command_buffer(true);

    vkCmdBindPipeline(cmd_buf->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_cubemap_pipeline->handle());

    const VkDescriptorSet sets[] = { ds->handle() };

    vkCmdBindDescriptorSets(cmd_buf->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_cubemap_pipeline_layout->handle(), 0, 1, sets, 0, nullptr);

    for (int i = 0; i < 6; i++)
    {
        VkClearValue clear_value;

        clear_value.color.float32[0] = 0.0f;
        clear_value.color.float32[1] = 0.0f;
        clear_value.color.float32[2] = 0.0f;
        clear_value.color.float32[3] = 1.0f;

        VkRenderPassBeginInfo info    = {};
        info.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass               = m_cubemap_renderpass->handle();
        info.framebuffer              = face_framebuffers[i]->handle();
        info.renderArea.extent.width  = output_image->width();
        info.renderArea.extent.height = output_image->height();
        info.clearValueCount          = 1;
        info.pClearValues             = &clear_value;

        vkCmdBeginRenderPass(cmd_buf->handle(), &info, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport vp;

        vp.x        = 0.0f;
        vp.y        = 0.0f;
        vp.width    = (float)output_image->width();
        vp.height   = (float)output_image->height();
        vp.minDepth = 0.0f;
        vp.maxDepth = 1.0f;

        vkCmdSetViewport(cmd_buf->handle(), 0, 1, &vp);

        VkRect2D scissor_rect;

        scissor_rect.extent.width  = output_image->width();
        scissor_rect.extent.height = output_image->height();
        scissor_rect.offset.x      = 0;
        scissor_rect.offset.y      = 0;

        vkCmdSetScissor(cmd_buf->handle(), 0, 1, &scissor_rect);

        vkCmdPushConstants(cmd_buf->handle(), m_cubemap_pipeline_layout->handle(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::mat4), &m_view_projection_mats[i]);

        const VkBuffer     buffer = m_cube_vbo->handle();
        const VkDeviceSize size   = 0;
        vkCmdBindVertexBuffers(cmd_buf->handle(), 0, 1, &buffer, &size);

        vkCmdDraw(cmd_buf->handle(), 36, 1, 0, 0);

        vkCmdEndRenderPass(cmd_buf->handle());
    }

    vkEndCommandBuffer(cmd_buf->handle());

    backend->flush_graphics({ cmd_buf });
#else

#endif
}

// -----------------------------------------------------------------------------------------------------------------------------------
} // namespace dw