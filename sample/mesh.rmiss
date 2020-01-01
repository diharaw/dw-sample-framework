#version 460
#extension GL_NV_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#include "common.glsl"

layout(location = PRIMARY_RAY_PAYLOAD_LOC) rayPayloadInNV RayPayload PrimaryRay;

void main() 
{
    PrimaryRay.color_dist = vec4(1.0, 0.0, 0.0, 1.0);
}