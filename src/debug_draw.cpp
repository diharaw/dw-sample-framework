#include <debug_draw.h>
#include <logger.h>
#include <utility.h>
#if defined(DWSF_VULKAN)
#    include <vk_mem_alloc.h>
#endif

namespace dw
{
#if defined(DWSF_VULKAN)

static const unsigned int kDebugDrawVertSPV_size           = 1532;
static const unsigned int kDebugDrawVertSPV_data[1532 / 4] = {
    0x07230203,
    0x00010000,
    0x000d0008,
    0x0000002c,
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
    0x000b000f,
    0x00000000,
    0x00000004,
    0x6e69616d,
    0x00000000,
    0x00000009,
    0x0000000b,
    0x0000000d,
    0x0000000e,
    0x00000016,
    0x0000002b,
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
    0x00050005,
    0x00000009,
    0x495f5346,
    0x6f435f4e,
    0x00726f6c,
    0x00050005,
    0x0000000b,
    0x495f5356,
    0x6f435f4e,
    0x00726f6c,
    0x00060005,
    0x0000000d,
    0x495f5346,
    0x72465f4e,
    0x6f506761,
    0x00000073,
    0x00060005,
    0x0000000e,
    0x495f5356,
    0x6f505f4e,
    0x69746973,
    0x00006e6f,
    0x00060005,
    0x00000014,
    0x505f6c67,
    0x65567265,
    0x78657472,
    0x00000000,
    0x00060006,
    0x00000014,
    0x00000000,
    0x505f6c67,
    0x7469736f,
    0x006e6f69,
    0x00070006,
    0x00000014,
    0x00000001,
    0x505f6c67,
    0x746e696f,
    0x657a6953,
    0x00000000,
    0x00070006,
    0x00000014,
    0x00000002,
    0x435f6c67,
    0x4470696c,
    0x61747369,
    0x0065636e,
    0x00070006,
    0x00000014,
    0x00000003,
    0x435f6c67,
    0x446c6c75,
    0x61747369,
    0x0065636e,
    0x00030005,
    0x00000016,
    0x00000000,
    0x00060005,
    0x0000001a,
    0x656d6143,
    0x6e556172,
    0x726f6669,
    0x0000736d,
    0x00060006,
    0x0000001a,
    0x00000000,
    0x77656976,
    0x6a6f7250,
    0x00000000,
    0x00030005,
    0x0000001c,
    0x00000000,
    0x00060005,
    0x0000002b,
    0x495f5356,
    0x65545f4e,
    0x6f6f4378,
    0x00006472,
    0x00040047,
    0x00000009,
    0x0000001e,
    0x00000000,
    0x00040047,
    0x0000000b,
    0x0000001e,
    0x00000002,
    0x00040047,
    0x0000000d,
    0x0000001e,
    0x00000001,
    0x00040047,
    0x0000000e,
    0x0000001e,
    0x00000000,
    0x00050048,
    0x00000014,
    0x00000000,
    0x0000000b,
    0x00000000,
    0x00050048,
    0x00000014,
    0x00000001,
    0x0000000b,
    0x00000001,
    0x00050048,
    0x00000014,
    0x00000002,
    0x0000000b,
    0x00000003,
    0x00050048,
    0x00000014,
    0x00000003,
    0x0000000b,
    0x00000004,
    0x00030047,
    0x00000014,
    0x00000002,
    0x00040048,
    0x0000001a,
    0x00000000,
    0x00000005,
    0x00050048,
    0x0000001a,
    0x00000000,
    0x00000023,
    0x00000000,
    0x00050048,
    0x0000001a,
    0x00000000,
    0x00000007,
    0x00000010,
    0x00030047,
    0x0000001a,
    0x00000002,
    0x00040047,
    0x0000001c,
    0x00000022,
    0x00000000,
    0x00040047,
    0x0000001c,
    0x00000021,
    0x00000000,
    0x00040047,
    0x0000002b,
    0x0000001e,
    0x00000001,
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
    0x0004003b,
    0x00000008,
    0x0000000d,
    0x00000003,
    0x0004003b,
    0x0000000a,
    0x0000000e,
    0x00000001,
    0x00040017,
    0x00000010,
    0x00000006,
    0x00000004,
    0x00040015,
    0x00000011,
    0x00000020,
    0x00000000,
    0x0004002b,
    0x00000011,
    0x00000012,
    0x00000001,
    0x0004001c,
    0x00000013,
    0x00000006,
    0x00000012,
    0x0006001e,
    0x00000014,
    0x00000010,
    0x00000006,
    0x00000013,
    0x00000013,
    0x00040020,
    0x00000015,
    0x00000003,
    0x00000014,
    0x0004003b,
    0x00000015,
    0x00000016,
    0x00000003,
    0x00040015,
    0x00000017,
    0x00000020,
    0x00000001,
    0x0004002b,
    0x00000017,
    0x00000018,
    0x00000000,
    0x00040018,
    0x00000019,
    0x00000010,
    0x00000004,
    0x0003001e,
    0x0000001a,
    0x00000019,
    0x00040020,
    0x0000001b,
    0x00000002,
    0x0000001a,
    0x0004003b,
    0x0000001b,
    0x0000001c,
    0x00000002,
    0x00040020,
    0x0000001d,
    0x00000002,
    0x00000019,
    0x0004002b,
    0x00000006,
    0x00000021,
    0x3f800000,
    0x00040020,
    0x00000027,
    0x00000003,
    0x00000010,
    0x00040017,
    0x00000029,
    0x00000006,
    0x00000002,
    0x00040020,
    0x0000002a,
    0x00000001,
    0x00000029,
    0x0004003b,
    0x0000002a,
    0x0000002b,
    0x00000001,
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
    0x0004003d,
    0x00000007,
    0x0000000f,
    0x0000000e,
    0x0003003e,
    0x0000000d,
    0x0000000f,
    0x00050041,
    0x0000001d,
    0x0000001e,
    0x0000001c,
    0x00000018,
    0x0004003d,
    0x00000019,
    0x0000001f,
    0x0000001e,
    0x0004003d,
    0x00000007,
    0x00000020,
    0x0000000e,
    0x00050051,
    0x00000006,
    0x00000022,
    0x00000020,
    0x00000000,
    0x00050051,
    0x00000006,
    0x00000023,
    0x00000020,
    0x00000001,
    0x00050051,
    0x00000006,
    0x00000024,
    0x00000020,
    0x00000002,
    0x00070050,
    0x00000010,
    0x00000025,
    0x00000022,
    0x00000023,
    0x00000024,
    0x00000021,
    0x00050091,
    0x00000010,
    0x00000026,
    0x0000001f,
    0x00000025,
    0x00050041,
    0x00000027,
    0x00000028,
    0x00000016,
    0x00000018,
    0x0003003e,
    0x00000028,
    0x00000026,
    0x000100fd,
    0x00010038,
};

static const unsigned int kDebugDrawFragSPV_size           = 1904;
static const unsigned int kDebugDrawFragSPV_data[1904 / 4] = {
    0x07230203,
    0x00010000,
    0x000d0008,
    0x00000047,
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
    0x00000004,
    0x00000004,
    0x6e69616d,
    0x00000000,
    0x0000001f,
    0x00000022,
    0x0000002b,
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
    0x00050005,
    0x00000008,
    0x65646166,
    0x6174735f,
    0x00007472,
    0x00060005,
    0x0000000a,
    0x68737550,
    0x736e6f43,
    0x746e6174,
    0x00000073,
    0x00060006,
    0x0000000a,
    0x00000000,
    0x656d6163,
    0x705f6172,
    0x0000736f,
    0x00060006,
    0x0000000a,
    0x00000001,
    0x65646166,
    0x7261705f,
    0x00736d61,
    0x00050005,
    0x0000000c,
    0x68737570,
    0x6e6f635f,
    0x00737473,
    0x00050005,
    0x00000014,
    0x65646166,
    0x646e655f,
    0x00000000,
    0x00060005,
    0x0000001f,
    0x4f5f5346,
    0x435f5455,
    0x726f6c6f,
    0x00000000,
    0x00050005,
    0x00000022,
    0x495f5346,
    0x6f435f4e,
    0x00726f6c,
    0x00050005,
    0x0000002a,
    0x74736964,
    0x65636e61,
    0x00000000,
    0x00060005,
    0x0000002b,
    0x495f5346,
    0x72465f4e,
    0x6f506761,
    0x00000073,
    0x00070005,
    0x00000034,
    0x65746661,
    0x61665f72,
    0x735f6564,
    0x74726174,
    0x00000000,
    0x00040005,
    0x00000039,
    0x6361706f,
    0x00797469,
    0x00050048,
    0x0000000a,
    0x00000000,
    0x00000023,
    0x00000000,
    0x00050048,
    0x0000000a,
    0x00000001,
    0x00000023,
    0x00000010,
    0x00030047,
    0x0000000a,
    0x00000002,
    0x00040047,
    0x0000001f,
    0x0000001e,
    0x00000000,
    0x00040047,
    0x00000022,
    0x0000001e,
    0x00000000,
    0x00040047,
    0x0000002b,
    0x0000001e,
    0x00000001,
    0x00020013,
    0x00000002,
    0x00030021,
    0x00000003,
    0x00000002,
    0x00030016,
    0x00000006,
    0x00000020,
    0x00040020,
    0x00000007,
    0x00000007,
    0x00000006,
    0x00040017,
    0x00000009,
    0x00000006,
    0x00000004,
    0x0004001e,
    0x0000000a,
    0x00000009,
    0x00000009,
    0x00040020,
    0x0000000b,
    0x00000009,
    0x0000000a,
    0x0004003b,
    0x0000000b,
    0x0000000c,
    0x00000009,
    0x00040015,
    0x0000000d,
    0x00000020,
    0x00000001,
    0x0004002b,
    0x0000000d,
    0x0000000e,
    0x00000001,
    0x00040015,
    0x0000000f,
    0x00000020,
    0x00000000,
    0x0004002b,
    0x0000000f,
    0x00000010,
    0x00000000,
    0x00040020,
    0x00000011,
    0x00000009,
    0x00000006,
    0x0004002b,
    0x0000000f,
    0x00000015,
    0x00000001,
    0x0004002b,
    0x00000006,
    0x00000019,
    0x00000000,
    0x00020014,
    0x0000001a,
    0x00040020,
    0x0000001e,
    0x00000003,
    0x00000009,
    0x0004003b,
    0x0000001e,
    0x0000001f,
    0x00000003,
    0x00040017,
    0x00000020,
    0x00000006,
    0x00000003,
    0x00040020,
    0x00000021,
    0x00000001,
    0x00000020,
    0x0004003b,
    0x00000021,
    0x00000022,
    0x00000001,
    0x0004002b,
    0x00000006,
    0x00000024,
    0x3f800000,
    0x0004003b,
    0x00000021,
    0x0000002b,
    0x00000001,
    0x0004002b,
    0x0000000d,
    0x0000002d,
    0x00000000,
    0x00040020,
    0x0000002e,
    0x00000009,
    0x00000009,
    0x00050036,
    0x00000002,
    0x00000004,
    0x00000000,
    0x00000003,
    0x000200f8,
    0x00000005,
    0x0004003b,
    0x00000007,
    0x00000008,
    0x00000007,
    0x0004003b,
    0x00000007,
    0x00000014,
    0x00000007,
    0x0004003b,
    0x00000007,
    0x0000002a,
    0x00000007,
    0x0004003b,
    0x00000007,
    0x00000034,
    0x00000007,
    0x0004003b,
    0x00000007,
    0x00000039,
    0x00000007,
    0x00060041,
    0x00000011,
    0x00000012,
    0x0000000c,
    0x0000000e,
    0x00000010,
    0x0004003d,
    0x00000006,
    0x00000013,
    0x00000012,
    0x0003003e,
    0x00000008,
    0x00000013,
    0x00060041,
    0x00000011,
    0x00000016,
    0x0000000c,
    0x0000000e,
    0x00000015,
    0x0004003d,
    0x00000006,
    0x00000017,
    0x00000016,
    0x0003003e,
    0x00000014,
    0x00000017,
    0x0004003d,
    0x00000006,
    0x00000018,
    0x00000008,
    0x000500b8,
    0x0000001a,
    0x0000001b,
    0x00000018,
    0x00000019,
    0x000300f7,
    0x0000001d,
    0x00000000,
    0x000400fa,
    0x0000001b,
    0x0000001c,
    0x00000029,
    0x000200f8,
    0x0000001c,
    0x0004003d,
    0x00000020,
    0x00000023,
    0x00000022,
    0x00050051,
    0x00000006,
    0x00000025,
    0x00000023,
    0x00000000,
    0x00050051,
    0x00000006,
    0x00000026,
    0x00000023,
    0x00000001,
    0x00050051,
    0x00000006,
    0x00000027,
    0x00000023,
    0x00000002,
    0x00070050,
    0x00000009,
    0x00000028,
    0x00000025,
    0x00000026,
    0x00000027,
    0x00000024,
    0x0003003e,
    0x0000001f,
    0x00000028,
    0x000200f9,
    0x0000001d,
    0x000200f8,
    0x00000029,
    0x0004003d,
    0x00000020,
    0x0000002c,
    0x0000002b,
    0x00050041,
    0x0000002e,
    0x0000002f,
    0x0000000c,
    0x0000002d,
    0x0004003d,
    0x00000009,
    0x00000030,
    0x0000002f,
    0x0008004f,
    0x00000020,
    0x00000031,
    0x00000030,
    0x00000030,
    0x00000000,
    0x00000001,
    0x00000002,
    0x00050083,
    0x00000020,
    0x00000032,
    0x0000002c,
    0x00000031,
    0x0006000c,
    0x00000006,
    0x00000033,
    0x00000001,
    0x00000042,
    0x00000032,
    0x0003003e,
    0x0000002a,
    0x00000033,
    0x0004003d,
    0x00000006,
    0x00000035,
    0x0000002a,
    0x0004003d,
    0x00000006,
    0x00000036,
    0x00000008,
    0x00050083,
    0x00000006,
    0x00000037,
    0x00000035,
    0x00000036,
    0x0007000c,
    0x00000006,
    0x00000038,
    0x00000001,
    0x00000028,
    0x00000037,
    0x00000019,
    0x0003003e,
    0x00000034,
    0x00000038,
    0x0004003d,
    0x00000006,
    0x0000003a,
    0x00000034,
    0x0004003d,
    0x00000006,
    0x0000003b,
    0x00000014,
    0x0004003d,
    0x00000006,
    0x0000003c,
    0x00000008,
    0x00050083,
    0x00000006,
    0x0000003d,
    0x0000003b,
    0x0000003c,
    0x00050088,
    0x00000006,
    0x0000003e,
    0x0000003a,
    0x0000003d,
    0x00050083,
    0x00000006,
    0x0000003f,
    0x00000024,
    0x0000003e,
    0x0008000c,
    0x00000006,
    0x00000040,
    0x00000001,
    0x0000002b,
    0x0000003f,
    0x00000019,
    0x00000024,
    0x0003003e,
    0x00000039,
    0x00000040,
    0x0004003d,
    0x00000020,
    0x00000041,
    0x00000022,
    0x0004003d,
    0x00000006,
    0x00000042,
    0x00000039,
    0x00050051,
    0x00000006,
    0x00000043,
    0x00000041,
    0x00000000,
    0x00050051,
    0x00000006,
    0x00000044,
    0x00000041,
    0x00000001,
    0x00050051,
    0x00000006,
    0x00000045,
    0x00000041,
    0x00000002,
    0x00070050,
    0x00000009,
    0x00000046,
    0x00000043,
    0x00000044,
    0x00000045,
    0x00000042,
    0x0003003e,
    0x0000001f,
    0x00000046,
    0x000200f9,
    0x0000001d,
    0x000200f8,
    0x0000001d,
    0x000100fd,
    0x00010038,
};

#else
const char* g_vs_src = R"(

	layout (location = 0) in vec3 VS_IN_Position;
	layout (location = 1) in vec2 VS_IN_TexCoord;
	layout (location = 2) in vec3 VS_IN_Color;
	
	layout (std140) uniform CameraUniforms //#binding 0
	{ 
		mat4 viewProj;
	};
	
	out vec3 FS_IN_Color;
    out vec3 FS_IN_FragPos;
	
	void main()
	{
	    FS_IN_Color = VS_IN_Color;
        FS_IN_FragPos = VS_IN_Position;
	    gl_Position = viewProj * vec4(VS_IN_Position, 1.0);
	}

	)";

const char* g_fs_src = R"(

    precision mediump float;
    
	out vec4 FS_OUT_Color;

	in vec3 FS_IN_Color;
    in vec3 FS_IN_FragPos;

    uniform vec4 camera_pos;
    uniform vec4 fade_params;
	
	void main()
	{
	    float fade_start = fade_params.x;
        float fade_end = fade_params.y;

        if (fade_start < 0.0)
            FS_OUT_Color = vec4(FS_IN_Color, 1.0);
        else
        {
            float distance = length(FS_IN_FragPos - camera_pos.xyz);
            float after_fade_start = max(distance - fade_start, 0.0);
            float opacity = clamp(1.0 - after_fade_start/(fade_end - fade_start), 0.0, 1.0);

            FS_OUT_Color = vec4(FS_IN_Color, opacity);
        }
	}

	)";
#endif

// -----------------------------------------------------------------------------------------------------------------------------------

DebugDraw::DebugDraw()
{
    m_world_vertices.resize(MAX_VERTICES);
    m_world_vertices.clear();

    m_draw_commands.resize(MAX_VERTICES);
    m_draw_commands.clear();
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool DebugDraw::init(
#if defined(DWSF_VULKAN)
    vk::Backend::Ptr    backend,
    vk::RenderPass::Ptr render_pass
#endif
)
{
#if defined(DWSF_VULKAN)
    create_uniform_buffer(backend);
    create_vertex_buffer(backend);
    create_descriptor_set_layout(backend);
    create_descriptor_set(backend);
    create_pipeline_states(backend, render_pass);

    return true;
#else
    // Create shaders
    m_line_vs = std::make_unique<gl::Shader>(GL_VERTEX_SHADER, g_vs_src);
    m_line_fs = std::make_unique<gl::Shader>(GL_FRAGMENT_SHADER, g_fs_src);

    if (!m_line_vs || !m_line_fs)
    {
        DW_LOG_FATAL("Failed to create Shaders");
        return false;
    }

    // Create shader program
    gl::Shader* shaders[] = { m_line_vs.get(), m_line_fs.get() };
    m_line_program        = std::make_unique<gl::Program>(2, shaders);

    // Bind uniform block index
    m_line_program->uniform_block_binding("CameraUniforms", 0);

    // Create vertex buffer
    m_line_vbo = std::make_unique<gl::VertexBuffer>(
        GL_DYNAMIC_DRAW, sizeof(VertexWorld) * MAX_VERTICES);

    // Declare vertex attributes
    gl::VertexAttrib attribs[] = { { 3, GL_FLOAT, false, 0 },
                                   { 2, GL_FLOAT, false, sizeof(float) * 3 },
                                   { 3, GL_FLOAT, false, sizeof(float) * 5 } };

    // Create vertex array
    m_line_vao = std::make_unique<gl::VertexArray>(m_line_vbo.get(), nullptr, sizeof(float) * 8, 3, attribs);

    if (!m_line_vao || !m_line_vbo)
    {
        DW_LOG_FATAL("Failed to create Vertex Buffers/Arrays");
        return false;
    }

    // Create uniform buffer for matrix data
    m_ubo = std::make_unique<gl::UniformBuffer>(GL_DYNAMIC_DRAW, sizeof(CameraUniforms));

    return true;
#endif
}

// -----------------------------------------------------------------------------------------------------------------------------------

void DebugDraw::shutdown()
{
#if defined(DWSF_VULKAN)
    m_line_strip_depth_pipeline.reset();
    m_line_strip_no_depth_pipeline.reset();
    m_line_depth_pipeline.reset();
    m_line_no_depth_pipeline.reset();
    m_pipeline_layout.reset();
    m_ds.reset();
    m_ds_layout.reset();
#endif
    m_line_vbo.reset();
    m_ubo.reset();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void DebugDraw::begin_batch()
{
    m_batched_mode = true;
    m_batch_start  = m_world_vertices.size();
    m_batch_end    = m_world_vertices.size();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void DebugDraw::end_batch()
{
    m_batched_mode = false;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void DebugDraw::capsule(const float& _height, const float& _radius, const glm::vec3& _pos, const glm::vec3& _c)
{
    // Draw four lines
    line(glm::vec3(_pos.x, _pos.y + _radius, _pos.z - _radius),
         glm::vec3(_pos.x, _height - _radius, _pos.z - _radius),
         _c);
    line(glm::vec3(_pos.x, _pos.y + _radius, _pos.z + _radius),
         glm::vec3(_pos.x, _height - _radius, _pos.z + _radius),
         _c);
    line(glm::vec3(_pos.x - _radius, _pos.y + _radius, _pos.z),
         glm::vec3(_pos.x - _radius, _height - _radius, _pos.z),
         _c);
    line(glm::vec3(_pos.x + _radius, _pos.y + _radius, _pos.z),
         glm::vec3(_pos.x + _radius, _height - _radius, _pos.z),
         _c);

    glm::vec3 verts[10];

    int idx = 0;

    for (int i = 0; i <= 180; i += 20)
    {
        float degInRad = glm::radians((float)i);
        verts[idx++]   = glm::vec3(_pos.x + cos(degInRad) * _radius,
                                 _height - _radius + sin(degInRad) * _radius,
                                 _pos.z);
    }

    line_strip(&verts[0], 10, _c);

    idx = 0;

    for (int i = 0; i <= 180; i += 20)
    {
        float degInRad = glm::radians((float)i);
        verts[idx++]   = glm::vec3(_pos.x, _height - _radius + sin(degInRad) * _radius, _pos.z + cos(degInRad) * _radius);
    }

    line_strip(&verts[0], 10, _c);

    idx = 0;

    for (int i = 180; i <= 360; i += 20)
    {
        float degInRad = glm::radians((float)i);
        verts[idx++]   = glm::vec3(_pos.x + cos(degInRad) * _radius,
                                 _radius + sin(degInRad) * _radius,
                                 _pos.z);
    }

    line_strip(&verts[0], 10, _c);

    idx = 0;

    for (int i = 180; i <= 360; i += 20)
    {
        float degInRad = glm::radians((float)i);
        verts[idx++]   = glm::vec3(_pos.x, _radius + sin(degInRad) * _radius, _pos.z + cos(degInRad) * _radius);
    }

    line_strip(&verts[0], 10, _c);

    circle_xz(_radius, glm::vec3(_pos.x, _height - _radius, _pos.z), _c);
    circle_xz(_radius, glm::vec3(_pos.x, _radius, _pos.z), _c);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void DebugDraw::aabb(const glm::vec3& _min, const glm::vec3& _max, const glm::vec3& _c)
{
    glm::vec3 _pos = (_max + _min) * 0.5f;

    glm::vec3 min = _pos + _min;
    glm::vec3 max = _pos + _max;

    begin_batch();

    line(min, glm::vec3(max.x, min.y, min.z), _c);
    line(glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, min.y, max.z), _c);
    line(glm::vec3(max.x, min.y, max.z), glm::vec3(min.x, min.y, max.z), _c);
    line(glm::vec3(min.x, min.y, max.z), min, _c);

    line(glm::vec3(min.x, max.y, min.z), glm::vec3(max.x, max.y, min.z), _c);
    line(glm::vec3(max.x, max.y, min.z), max, _c);
    line(max, glm::vec3(min.x, max.y, max.z), _c);
    line(glm::vec3(min.x, max.y, max.z), glm::vec3(min.x, max.y, min.z), _c);

    line(min, glm::vec3(min.x, max.y, min.z), _c);
    line(glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, max.y, min.z), _c);
    line(glm::vec3(max.x, min.y, max.z), max, _c);
    line(glm::vec3(min.x, min.y, max.z), glm::vec3(min.x, max.y, max.z), _c);

    end_batch();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void DebugDraw::obb(const glm::vec3& _min, const glm::vec3& _max, const glm::mat4& _model, const glm::vec3& _c)
{
    glm::vec3 verts[8];
    glm::vec3 size = _max - _min;
    int       idx  = 0;

    for (float x = _min.x; x <= _max.x; x += size.x)
    {
        for (float y = _min.y; y <= _max.y; y += size.y)
        {
            for (float z = _min.z; z <= _max.z; z += size.z)
            {
                glm::vec4 v  = _model * glm::vec4(x, y, z, 1.0f);
                verts[idx++] = glm::vec3(v.x, v.y, v.z);
            }
        }
    }

    line(verts[0], verts[1], _c);
    line(verts[1], verts[5], _c);
    line(verts[5], verts[4], _c);
    line(verts[4], verts[0], _c);

    line(verts[2], verts[3], _c);
    line(verts[3], verts[7], _c);
    line(verts[7], verts[6], _c);
    line(verts[6], verts[2], _c);

    line(verts[2], verts[0], _c);
    line(verts[6], verts[4], _c);
    line(verts[3], verts[1], _c);
    line(verts[7], verts[5], _c);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void DebugDraw::grid(const float& _x, const float& _z, const float& _y_level, const float& spacing, const glm::vec3& _c)
{
    int offset_x = floor((_x * spacing) / 2.0f);
    int offset_z = floor((_z * spacing) / 2.0f);

    for (int x = -offset_x; x <= offset_x; x += spacing)
    {
        line(glm::vec3(x, _y_level, -offset_z), glm::vec3(x, _y_level, offset_z), _c);
    }

    for (int z = -offset_z; z <= offset_z; z += spacing)
    {
        line(glm::vec3(-offset_x, _y_level, z), glm::vec3(offset_x, _y_level, z), _c);
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

float closest_divisable(float a, float b)
{
    int c1 = int(a) - (int(a) % int(b));
    int c2 = (a + b) - (int(a) % int(b));

    if (int(a) - c1 > c2 - int(a))
        return float(c2);
    else
        return float(c1);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void DebugDraw::grid(const glm::mat4& view_proj, const float& unit_size, const float& highlight_unit_size)
{
    // Get world space frustum corners
    glm::mat4 inverse = glm::inverse(view_proj);
    glm::vec3 corners[8];

    for (int i = 0; i < 8; i++)
    {
        glm::vec4 v = inverse * kFrustumCorners[i];
        v           = v / v.w;
        corners[i]  = glm::vec3(v.x, v.y, v.z);
    }

    // Find min and max
    glm::vec3 min = corners[0];
    glm::vec3 max = corners[0];

    for (int i = 0; i < 8; i++)
    {
        if (corners[i].x < min.x)
            min.x = corners[i].x;
        if (corners[i].y < min.y)
            min.y = corners[i].y;
        if (corners[i].z < min.z)
            min.z = corners[i].z;

        if (corners[i].x > max.x)
            max.x = corners[i].x;
        if (corners[i].y > max.y)
            max.y = corners[i].y;
        if (corners[i].z > max.z)
            max.z = corners[i].z;
    }

    glm::vec3 min_mod = glm::vec3(closest_divisable(min.x, unit_size), closest_divisable(min.y, unit_size), closest_divisable(min.z, unit_size));
    glm::vec3 max_mod = glm::vec3(closest_divisable(max.x, unit_size), closest_divisable(max.y, unit_size), closest_divisable(max.z, unit_size));

    float x = min_mod.x;

    begin_batch();

    while (x < max_mod.x)
    {
        glm::vec3 color = glm::vec3(1.0f);

        float coord = int(x) % int(highlight_unit_size);

        if (coord != 0.0f)
            color = glm::vec3(0.2f);

        if (x != 0.0f)
            line(glm::vec3(x, 0.0f, min.z), glm::vec3(x, 0.0f, max.z), color);

        x += unit_size;
    }

    float z = min_mod.z;

    while (z < max_mod.z)
    {
        glm::vec3 color = glm::vec3(1.0f);

        float coord = int(z) % int(highlight_unit_size);

        if (coord != 0.0f)
            color = glm::vec3(0.2f);

        if (z != 0.0f)
            line(glm::vec3(min.x, 0.0f, z), glm::vec3(max.x, 0.0f, z), color);

        z += unit_size;
    }

    // X-axis = Red
    line(glm::vec3(0.0f, 0.0f, min.z), glm::vec3(0.0f, 0.0f, max.z), glm::vec3(1.0f, 0.0f, 0.0f));

    // Y-axis = Green
    line(glm::vec3(0.0f, min.y, 0.0f), glm::vec3(0.0f, max.y, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Z-axis = Blue
    line(glm::vec3(min.x, 0.0f, 0.0f), glm::vec3(max.x, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    end_batch();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void DebugDraw::line(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& c)
{
    if (m_world_vertices.size() < MAX_VERTICES)
    {
        VertexWorld vw0, vw1;
        vw0.position = v0;
        vw0.color    = c;

        vw1.position = v1;
        vw1.color    = c;

        m_world_vertices.push_back(vw0);
        m_world_vertices.push_back(vw1);

        if (!m_batched_mode || m_batch_start == m_batch_end)
        {
            DrawCommand cmd;

            cmd.depth_test    = m_depth_test;
            cmd.distance_fade = m_distance_fade;
            cmd.fade_start    = m_fade_start;
            cmd.fade_end      = m_fade_end;

#if defined(DWSF_VULKAN)
            cmd.type = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
#else
            cmd.type = GL_LINES;
#endif
            cmd.vertices = 2;

            m_draw_commands.push_back(cmd);
        }
        else
        {
            DrawCommand& cmd = m_draw_commands.back();
            cmd.vertices += 2;
        }

        if (m_batched_mode)
            m_batch_end = m_world_vertices.size();
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

void DebugDraw::line_strip(glm::vec3* v, const int& count, const glm::vec3& c)
{
    for (int i = 0; i < count; i++)
    {
        VertexWorld vert;
        vert.position = v[i];
        vert.color    = c;

        m_world_vertices.push_back(vert);
    }

    DrawCommand cmd;

    cmd.depth_test    = m_depth_test;
    cmd.distance_fade = m_distance_fade;
    cmd.fade_start    = m_fade_start;
    cmd.fade_end      = m_fade_end;

#if defined(DWSF_VULKAN)
    cmd.type = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
#else
    cmd.type = GL_LINE_STRIP;
#endif
    cmd.vertices = count;

    m_draw_commands.push_back(cmd);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void DebugDraw::circle_xy(float radius, const glm::vec3& pos, const glm::vec3& c)
{
    glm::vec3 verts[19];

    int idx = 0;

    for (int i = 0; i <= 360; i += 20)
    {
        float degInRad = glm::radians((float)i);
        verts[idx++]   = pos + glm::vec3(cos(degInRad) * radius, sin(degInRad) * radius, 0.0f);
    }

    line_strip(&verts[0], 19, c);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void DebugDraw::circle_xz(float radius, const glm::vec3& pos, const glm::vec3& c)
{
    glm::vec3 verts[19];

    int idx = 0;

    for (int i = 0; i <= 360; i += 20)
    {
        float degInRad = glm::radians((float)i);
        verts[idx++]   = pos + glm::vec3(cos(degInRad) * radius, 0.0f, sin(degInRad) * radius);
    }

    line_strip(&verts[0], 19, c);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void DebugDraw::circle_yz(float radius, const glm::vec3& pos, const glm::vec3& c)
{
    glm::vec3 verts[19];

    int idx = 0;

    for (int i = 0; i <= 360; i += 20)
    {
        float degInRad = glm::radians((float)i);
        verts[idx++]   = pos + glm::vec3(0.0f, cos(degInRad) * radius, sin(degInRad) * radius);
    }

    line_strip(&verts[0], 19, c);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void DebugDraw::sphere(const float& radius, const glm::vec3& pos, const glm::vec3& c)
{
    circle_xy(radius, pos, c);
    circle_xz(radius, pos, c);
    circle_yz(radius, pos, c);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void DebugDraw::frustum(const glm::mat4& view_proj, const glm::vec3& c)
{
    glm::mat4 inverse = glm::inverse(view_proj);
    glm::vec3 corners[8];

    for (int i = 0; i < 8; i++)
    {
        glm::vec4 v = inverse * kFrustumCorners[i];
        v           = v / v.w;
        corners[i]  = glm::vec3(v.x, v.y, v.z);
    }

    glm::vec3 _far[5] = { corners[0], corners[1], corners[2], corners[3], corners[0] };

    line_strip(&_far[0], 5, c);

    glm::vec3 _near[5] = { corners[4], corners[5], corners[6], corners[7], corners[4] };

    line_strip(&_near[0], 5, c);

    line(corners[0], corners[4], c);
    line(corners[1], corners[5], c);
    line(corners[2], corners[6], c);
    line(corners[3], corners[7], c);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void DebugDraw::frustum(const glm::mat4& proj, const glm::mat4& view, const glm::vec3& c)
{
    glm::mat4 inverse = glm::inverse(proj * view);
    glm::vec3 corners[8];

    for (int i = 0; i < 8; i++)
    {
        glm::vec4 v = inverse * kFrustumCorners[i];
        v           = v / v.w;
        corners[i]  = glm::vec3(v.x, v.y, v.z);
    }

    glm::vec3 _far[5] = { corners[0], corners[1], corners[2], corners[3], corners[0] };

    line_strip(&_far[0], 5, c);

    glm::vec3 _near[5] = { corners[4], corners[5], corners[6], corners[7], corners[4] };

    line_strip(&_near[0], 5, c);

    line(corners[0], corners[4], c);
    line(corners[1], corners[5], c);
    line(corners[2], corners[6], c);
    line(corners[3], corners[7], c);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void DebugDraw::transform(const glm::mat4& trans, const float& axis_length)
{
    glm::vec3 p = glm::vec3(trans[3][0], trans[3][1], trans[3][2]);
    glm::vec3 x = glm::vec3(trans[0][0], trans[0][1], trans[0][2]);
    glm::vec3 y = glm::vec3(trans[1][0], trans[1][1], trans[1][2]);
    glm::vec3 z = glm::vec3(trans[2][0], trans[2][1], trans[2][2]);

    // Draw X axis
    line(p, p + x * axis_length, glm::vec3(1.0f, 0.0f, 0.0f));

    // Draw Y axis
    line(p, p + y * axis_length, glm::vec3(0.0f, 1.0f, 0.0f));

    // Draw Z axis
    line(p, p + z * axis_length, glm::vec3(0.0f, 0.0f, 1.0f));
}

// -----------------------------------------------------------------------------------------------------------------------------------

#if defined(DWSF_VULKAN)
void DebugDraw::render(vk::Backend::Ptr backend, vk::CommandBuffer::Ptr cmd_buffer, int width, int height, const glm::mat4& view_proj, const glm::vec3& view_pos)
{
    if (m_world_vertices.size() > 0)
    {
        m_uniforms.view_proj = view_proj;

        uint8_t* ptr = (uint8_t*)m_ubo->mapped_ptr();
        memcpy(ptr + m_ubo_size * backend->current_frame_idx(), &m_uniforms, sizeof(CameraUniforms));

        ptr = (uint8_t*)m_line_vbo->mapped_ptr();

        if (m_world_vertices.size() > MAX_VERTICES)
            DW_LOG_ERROR("Vertex count above allowed limit!");
        else
            memcpy(ptr + m_vbo_size * backend->current_frame_idx(), &m_world_vertices[0], sizeof(VertexWorld) * m_world_vertices.size());

        const uint32_t dynamic_offset = m_ubo_size * backend->current_frame_idx();

        vkCmdBindDescriptorSets(cmd_buffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout->handle(), 0, 1, &m_ds->handle(), 1, &dynamic_offset);

        const VkDeviceSize vbo_offset = m_vbo_size * backend->current_frame_idx();
        vkCmdBindVertexBuffers(cmd_buffer->handle(), 0, 1, &m_line_vbo->handle(), &vbo_offset);

        int v = 0;

        for (int i = 0; i < m_draw_commands.size(); i++)
        {
            DrawCommand& cmd = m_draw_commands[i];

            VkPipeline pipeline;

            if (cmd.type == VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
            {
                if (cmd.depth_test)
                    pipeline = m_line_depth_pipeline->handle();
                else
                    pipeline = m_line_no_depth_pipeline->handle();
            }
            else
            {
                if (cmd.depth_test)
                    pipeline = m_line_strip_depth_pipeline->handle();
                else
                    pipeline = m_line_strip_no_depth_pipeline->handle();
            }

            vkCmdBindPipeline(cmd_buffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

            glm::vec4 params[2];

            params[0] = glm::vec4(view_pos, 0.0f);
            params[1] = glm::vec4(cmd.distance_fade ? cmd.fade_start : -1.0f, cmd.fade_end, 0.0f, 0.0f);

            vkCmdPushConstants(cmd_buffer->handle(), m_pipeline_layout->handle(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(params), &params);

            vkCmdDraw(cmd_buffer->handle(), cmd.vertices, 1, v, 0);
            v += cmd.vertices;
        }

        m_draw_commands.clear();
        m_world_vertices.clear();
    }
}
#else
void DebugDraw::render(gl::Framebuffer* fbo, int width, int height, const glm::mat4& view_proj, const glm::vec3& view_pos)
{
    if (m_world_vertices.size() > 0)
    {
        m_uniforms.view_proj = view_proj;

#    if defined(__EMSCRIPTEN__)
        void* ptr            = m_line_vbo->map(0);
#    else
        void* ptr = m_line_vbo->map(GL_WRITE_ONLY);
#    endif

        if (m_world_vertices.size() > MAX_VERTICES)
            DW_LOG_ERROR("Vertex count above allowed limit!");
        else
            memcpy(ptr, &m_world_vertices[0], sizeof(VertexWorld) * m_world_vertices.size());

        m_line_vbo->unmap();

#    if defined(__EMSCRIPTEN__)
        ptr = m_ubo->map(0);
#    else
        ptr       = m_ubo->map(GL_WRITE_ONLY);
#    endif

        memcpy(ptr, &m_uniforms, sizeof(CameraUniforms));
        m_ubo->unmap();

        // Get previous state
        GLenum last_blend_src_rgb;
        glGetIntegerv(GL_BLEND_SRC_RGB, (GLint*)&last_blend_src_rgb);
        GLenum last_blend_dst_rgb;
        glGetIntegerv(GL_BLEND_DST_RGB, (GLint*)&last_blend_dst_rgb);
        GLenum last_blend_src_alpha;
        glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint*)&last_blend_src_alpha);
        GLenum last_blend_dst_alpha;
        glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint*)&last_blend_dst_alpha);
        GLenum last_blend_equation_rgb;
        glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint*)&last_blend_equation_rgb);
        GLenum last_blend_equation_alpha;
        glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint*)&last_blend_equation_alpha);
        GLboolean last_enable_blend        = glIsEnabled(GL_BLEND);
        GLboolean last_enable_cull_face    = glIsEnabled(GL_CULL_FACE);
        GLboolean last_enable_depth_test   = glIsEnabled(GL_DEPTH_TEST);
        GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

        // Set initial state
        glDisable(GL_CULL_FACE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        if (fbo)
            fbo->bind();
        else
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glViewport(0, 0, width, height);
        m_line_program->use();
        m_ubo->bind_base(0);
        m_line_vao->bind();

        int v = 0;

        for (int i = 0; i < m_draw_commands.size(); i++)
        {
            DrawCommand& cmd = m_draw_commands[i];

            if (cmd.distance_fade)
                glEnable(GL_BLEND);
            else
                glDisable(GL_BLEND);

            if (cmd.depth_test)
                glEnable(GL_DEPTH_TEST);
            else
                glDisable(GL_DEPTH_TEST);

            glm::vec4 params[2];

            params[0] = glm::vec4(view_pos, 0.0f);
            params[1] = glm::vec4(cmd.distance_fade ? cmd.fade_start : -1.0f, cmd.fade_end, 0.0f, 0.0f);

            m_line_program->set_uniform("camera_pos", params[0]);
            m_line_program->set_uniform("fade_params", params[1]);

            glDrawArrays(cmd.type, v, cmd.vertices);
            v += cmd.vertices;
        }

        m_draw_commands.clear();
        m_world_vertices.clear();

        // Restore state
        //glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
        glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
        glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
        if (last_enable_blend)
            glEnable(GL_BLEND);
        else
            glDisable(GL_BLEND);
        if (last_enable_cull_face)
            glEnable(GL_CULL_FACE);
        else
            glDisable(GL_CULL_FACE);
        if (last_enable_depth_test)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
        if (last_enable_scissor_test)
            glEnable(GL_SCISSOR_TEST);
        else
            glDisable(GL_SCISSOR_TEST);
    }
}
#endif

// -----------------------------------------------------------------------------------------------------------------------------------

#if defined(DWSF_VULKAN)
void DebugDraw::create_descriptor_set_layout(vk::Backend::Ptr backend)
{
    dw::vk::DescriptorSetLayout::Desc desc;

    desc.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT);

    m_ds_layout = dw::vk::DescriptorSetLayout::create(backend, desc);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void DebugDraw::create_descriptor_set(vk::Backend::Ptr backend)
{
    m_ds = backend->allocate_descriptor_set(m_ds_layout);

    VkDescriptorBufferInfo buffer_info;

    buffer_info.buffer = m_ubo->handle();
    buffer_info.offset = 0;
    buffer_info.range  = sizeof(glm::mat4);

    VkWriteDescriptorSet write_data;
    DW_ZERO_MEMORY(write_data);

    write_data.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_data.descriptorCount = 1;
    write_data.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    write_data.pBufferInfo     = &buffer_info;
    write_data.dstBinding      = 0;
    write_data.dstSet          = m_ds->handle();

    vkUpdateDescriptorSets(backend->device(), 1, &write_data, 0, nullptr);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void DebugDraw::create_uniform_buffer(vk::Backend::Ptr backend)
{
    m_ubo_size = backend->aligned_dynamic_ubo_size(sizeof(CameraUniforms));
    m_ubo      = dw::vk::Buffer::create(backend, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, m_ubo_size * dw::vk::Backend::kMaxFramesInFlight, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void DebugDraw::create_vertex_buffer(vk::Backend::Ptr backend)
{
    m_vbo_size = sizeof(VertexWorld) * MAX_VERTICES;
    m_line_vbo = vk::Buffer::create(backend, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, m_vbo_size * dw::vk::Backend::kMaxFramesInFlight, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void DebugDraw::create_pipeline_states(vk::Backend::Ptr backend, vk::RenderPass::Ptr render_pass)
{
    // ---------------------------------------------------------------------------
    // Create shader modules
    // ---------------------------------------------------------------------------

    std::vector<char> spirv;

    spirv.resize(kDebugDrawVertSPV_size);
    memcpy(&spirv[0], &kDebugDrawVertSPV_data[0], kDebugDrawVertSPV_size);

    dw::vk::ShaderModule::Ptr vs = dw::vk::ShaderModule::create(backend, spirv);

    spirv.resize(kDebugDrawFragSPV_size);
    memcpy(&spirv[0], &kDebugDrawFragSPV_data[0], kDebugDrawFragSPV_size);

    dw::vk::ShaderModule::Ptr fs = dw::vk::ShaderModule::create(backend, spirv);

    dw::vk::GraphicsPipeline::Desc pso_desc;

    pso_desc.add_shader_stage(VK_SHADER_STAGE_VERTEX_BIT, vs, "main")
        .add_shader_stage(VK_SHADER_STAGE_FRAGMENT_BIT, fs, "main");

    // ---------------------------------------------------------------------------
    // Create vertex input state
    // ---------------------------------------------------------------------------

    vk::VertexInputStateDesc vertex_input_state_desc;

    vertex_input_state_desc.add_binding_desc(0, sizeof(VertexWorld), VK_VERTEX_INPUT_RATE_VERTEX);

    vertex_input_state_desc.add_attribute_desc(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);
    vertex_input_state_desc.add_attribute_desc(1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexWorld, uv));
    vertex_input_state_desc.add_attribute_desc(2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexWorld, color));

    pso_desc.set_vertex_input_state(vertex_input_state_desc);

    // ---------------------------------------------------------------------------
    // Create pipeline input assembly state
    // ---------------------------------------------------------------------------

    dw::vk::InputAssemblyStateDesc input_assembly_state_desc;

    input_assembly_state_desc.set_primitive_restart_enable(false);

    // ---------------------------------------------------------------------------
    // Create viewport state
    // ---------------------------------------------------------------------------

    dw::vk::ViewportStateDesc vp_desc;

    vp_desc.add_viewport(0.0f, 0.0f, 1024, 1024, 0.0f, 1.0f)
        .add_scissor(0, 0, 1024, 1024);

    pso_desc.set_viewport_state(vp_desc);

    // ---------------------------------------------------------------------------
    // Create rasterization state
    // ---------------------------------------------------------------------------

    dw::vk::RasterizationStateDesc rs_state;

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

    dw::vk::MultisampleStateDesc ms_state;

    ms_state.set_sample_shading_enable(VK_FALSE)
        .set_rasterization_samples(VK_SAMPLE_COUNT_1_BIT);

    pso_desc.set_multisample_state(ms_state);

    // ---------------------------------------------------------------------------
    // Create depth stencil state
    // ---------------------------------------------------------------------------

    dw::vk::DepthStencilStateDesc ds_state;

    ds_state.set_depth_test_enable(VK_TRUE)
        .set_depth_write_enable(VK_TRUE)
        .set_depth_compare_op(VK_COMPARE_OP_LESS)
        .set_depth_bounds_test_enable(VK_FALSE)
        .set_stencil_test_enable(VK_FALSE);

    pso_desc.set_depth_stencil_state(ds_state);

    // ---------------------------------------------------------------------------
    // Create color blend state
    // ---------------------------------------------------------------------------

    dw::vk::ColorBlendAttachmentStateDesc blend_att_desc;

    blend_att_desc.set_color_write_mask(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT)
        .set_src_color_blend_factor(VK_BLEND_FACTOR_SRC_ALPHA)
        .set_dst_color_blend_Factor(VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA)
        .set_src_alpha_blend_factor(VK_BLEND_FACTOR_ONE)
        .set_dst_alpha_blend_factor(VK_BLEND_FACTOR_ZERO)
        .set_color_blend_op(VK_BLEND_OP_ADD)
        .set_blend_enable(VK_TRUE);

    dw::vk::ColorBlendStateDesc blend_state;

    blend_state.set_logic_op_enable(VK_FALSE)
        .set_logic_op(VK_LOGIC_OP_COPY)
        .set_blend_constants(0.0f, 0.0f, 0.0f, 0.0f)
        .add_attachment(blend_att_desc);

    pso_desc.set_color_blend_state(blend_state);

    // ---------------------------------------------------------------------------
    // Create pipeline layout
    // ---------------------------------------------------------------------------

    dw::vk::PipelineLayout::Desc pl_desc;

    pl_desc.add_descriptor_set_layout(m_ds_layout);
    pl_desc.add_push_constant_range(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::vec4) * 2);

    m_pipeline_layout = dw::vk::PipelineLayout::create(backend, pl_desc);

    pso_desc.set_pipeline_layout(m_pipeline_layout);

    // ---------------------------------------------------------------------------
    // Create dynamic state
    // ---------------------------------------------------------------------------

    pso_desc.add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT)
        .add_dynamic_state(VK_DYNAMIC_STATE_SCISSOR);

    pso_desc.set_render_pass(render_pass);

    // ---------------------------------------------------------------------------
    // Create line list depth test enable pipeline
    // ---------------------------------------------------------------------------

    input_assembly_state_desc.set_topology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);

    pso_desc.set_input_assembly_state(input_assembly_state_desc);

    m_line_depth_pipeline = dw::vk::GraphicsPipeline::create(backend, pso_desc);

    // ---------------------------------------------------------------------------
    // Create depth  list test disabled pipeline
    // ---------------------------------------------------------------------------

    ds_state.set_depth_test_enable(VK_FALSE)
        .set_depth_write_enable(VK_FALSE);

    pso_desc.set_depth_stencil_state(ds_state);

    m_line_no_depth_pipeline = dw::vk::GraphicsPipeline::create(backend, pso_desc);

    // ---------------------------------------------------------------------------
    // Create line strip depth test enable pipeline
    // ---------------------------------------------------------------------------

    ds_state.set_depth_test_enable(VK_TRUE)
        .set_depth_write_enable(VK_TRUE);

    pso_desc.set_depth_stencil_state(ds_state);

    input_assembly_state_desc.set_topology(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);

    pso_desc.set_input_assembly_state(input_assembly_state_desc);

    m_line_strip_depth_pipeline = dw::vk::GraphicsPipeline::create(backend, pso_desc);

    // ---------------------------------------------------------------------------
    // Create depth  list test disabled pipeline
    // ---------------------------------------------------------------------------

    ds_state.set_depth_test_enable(VK_FALSE)
        .set_depth_write_enable(VK_FALSE);

    pso_desc.set_depth_stencil_state(ds_state);

    m_line_strip_no_depth_pipeline = dw::vk::GraphicsPipeline::create(backend, pso_desc);
}
#endif

// -----------------------------------------------------------------------------------------------------------------------------------
} // namespace dw
