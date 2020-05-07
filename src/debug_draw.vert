#version 450

layout (location = 0) in vec3 VS_IN_Position;
layout (location = 1) in vec2 VS_IN_TexCoord;
layout (location = 2) in vec3 VS_IN_Color;

layout (set = 0, binding = 0) uniform CameraUniforms
{ 
	mat4 viewProj;
};

layout (location = 0) out vec3 FS_IN_Color;
layout (location = 1) out vec3 FS_IN_FragPos;

void main()
{
    FS_IN_Color = VS_IN_Color;
    FS_IN_FragPos = VS_IN_Position;
    gl_Position = viewProj * vec4(VS_IN_Position, 1.0);
}