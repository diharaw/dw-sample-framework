#version 450

layout (location = 0) out vec4 FS_OUT_Color;

layout (location = 0) in vec3 FS_IN_Color;
layout (location = 1) in vec3 FS_IN_FragPos;

layout (push_constant) uniform PushConstants 
{
	vec4 camera_pos;
    vec4 fade_params;
} push_consts;

void main()
{
    float fade_start = push_consts.fade_params.x;
    float fade_end = push_consts.fade_params.y;

    if (fade_start < 0.0)
        FS_OUT_Color = vec4(FS_IN_Color, 1.0);
    else
    {
        float distance = length(FS_IN_FragPos - push_consts.camera_pos.xyz);
        float after_fade_start = max(distance - fade_start, 0.0);
        float opacity = clamp(1.0 - after_fade_start/(fade_end - fade_start), 0.0, 1.0);

        FS_OUT_Color = vec4(FS_IN_Color, opacity);
    }
}