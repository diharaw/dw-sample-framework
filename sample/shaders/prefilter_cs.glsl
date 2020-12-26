// ------------------------------------------------------------------
// CONSTANTS --------------------------------------------------------
// ------------------------------------------------------------------

#define LOCAL_SIZE 8
#define POS_X 0
#define NEG_X 1
#define POS_Y 2
#define NEG_Y 3
#define POS_Z 4
#define NEG_Z 5
#define PI 3.14159265359
#define MAX_SAMPLES 64

// ------------------------------------------------------------------
// INPUTS -----------------------------------------------------------
// ------------------------------------------------------------------

layout(local_size_x = LOCAL_SIZE, local_size_y = LOCAL_SIZE, local_size_z = 1) in;

// ------------------------------------------------------------------
// OUTPUTS ----------------------------------------------------------
// ------------------------------------------------------------------

layout(binding = 0, rgba16f) uniform imageCube i_Prefiltered;

// ------------------------------------------------------------------
// UNIFORM BUFFERS --------------------------------------------------
// ------------------------------------------------------------------

layout(std140) uniform u_SampleDirections
{
    vec4 sample_directions[MAX_SAMPLES];
};

// ------------------------------------------------------------------
// SAMPLERS ---------------------------------------------------------
// ------------------------------------------------------------------

uniform samplerCube s_EnvMap;
uniform float       u_Roughness;
uniform float       u_Width;
uniform float       u_Height;
uniform int         u_StartMipLevel;
uniform int         u_SampleCount;

// ------------------------------------------------------------------
// FUNCTIONS --------------------------------------------------------
// ------------------------------------------------------------------

float unlerp(float val, float max_val)
{
    return (val + 0.5) / max_val;
}

// ------------------------------------------------------------------

vec3 calculate_direction(uint face, uint face_x, uint face_y)
{
    float s = unlerp(float(face_x), u_Width) * 2.0 - 1.0;
    float t = unlerp(float(face_y), u_Height) * 2.0 - 1.0;
    float x, y, z;

    switch (face)
    {
        case POS_Z:
            x = s;
            y = -t;
            z = 1;
            break;
        case NEG_Z:
            x = -s;
            y = -t;
            z = -1;
            break;
        case NEG_X:
            x = -1;
            y = -t;
            z = s;
            break;
        case POS_X:
            x = 1;
            y = -t;
            z = -s;
            break;
        case POS_Y:
            x = s;
            y = 1;
            z = t;
            break;
        case NEG_Y:
            x = s;
            y = -1;
            z = -t;
            break;
    }

    vec3  d;
    float inv_len = 1.0 / sqrt(x * x + y * y + z * z);
    d.x           = x * inv_len;
    d.y           = y * inv_len;
    d.z           = z * inv_len;

    return d;
}

// ----------------------------------------------------------------------------

float distribution_ggx(vec3 N, vec3 H, float roughness)
{
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom       = PI * denom * denom;

    return nom / denom;
}

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    vec3 N = calculate_direction(gl_GlobalInvocationID.z, gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);

    // make the simplyfying assumption that V equals R equals the normal
    vec3  R          = N;
    vec3  V          = R;
    ivec2 size       = textureSize(s_EnvMap, u_StartMipLevel);
    float resolution = float(size.x);

    vec3  prefiltered_color = vec3(0.0);
    float total_weight      = 0.0;

    // Compute a matrix to rotate the samples
    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);

    mat3 tangent_to_world = mat3(tangent, bitangent, N);

    for (uint i = 0u; i < u_SampleCount; ++i)
    {
        // generates a sample vector that's biased towards the preferred alignment direction (importance sampling).
        vec3 H = tangent_to_world * sample_directions[i].xyz;
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);

        if (NdotL > 0.0)
        {
            // sample from the environment's mip level based on roughness/pdf
            float D     = distribution_ggx(N, H, u_Roughness);
            float NdotH = max(dot(N, H), 0.0);
            float HdotV = max(dot(H, V), 0.0);
            float pdf   = D * NdotH / (4.0 * HdotV) + 0.0001;

            float sa_texel  = 4.0 * PI / (6.0 * resolution * resolution);
            float sa_sample = 1.0 / (float(u_SampleCount) * pdf + 0.0001);

            float mip_level = u_Roughness == 0.0 ? 0.0 : 0.5 * log2(sa_sample / sa_texel);

            prefiltered_color += textureLod(s_EnvMap, L, u_StartMipLevel + mip_level).rgb * NdotL;
            total_weight += NdotL;
        }
    }

    prefiltered_color = prefiltered_color / total_weight;

    imageStore(i_Prefiltered, ivec3(gl_GlobalInvocationID), vec4(prefiltered_color, 1.0));
}

// ------------------------------------------------------------------