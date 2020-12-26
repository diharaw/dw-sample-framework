// ------------------------------------------------------------------
// CONSTANTS --------------------------------------------------------
// ------------------------------------------------------------------

#define LOCAL_SIZE 8
#define ENVIRONMENT_MAP_SIZE 128
#define SH_INTERMEDIATE_SIZE (ENVIRONMENT_MAP_SIZE / LOCAL_SIZE)
#define CUBEMAP_MIP_LEVEL 2.0
#define POS_X 0
#define NEG_X 1
#define POS_Y 2
#define NEG_Y 3
#define POS_Z 4
#define NEG_Z 5

// ------------------------------------------------------------------
// INPUTS -----------------------------------------------------------
// ------------------------------------------------------------------

layout(local_size_x = LOCAL_SIZE, local_size_y = LOCAL_SIZE, local_size_z = 1) in;

// ------------------------------------------------------------------
// OUTPUTS ----------------------------------------------------------
// ------------------------------------------------------------------

layout(binding = 0, rgba32f) uniform image2DArray i_Cubemap;

// ------------------------------------------------------------------
// SAMPLERS ---------------------------------------------------------
// ------------------------------------------------------------------

uniform samplerCube s_Cubemap;
uniform float       u_Width;
uniform float       u_Height;

// ------------------------------------------------------------------
// STRUCTURES -------------------------------------------------------
// ------------------------------------------------------------------

struct SH9
{
    float c[9];
};

// ------------------------------------------------------------------

struct SH9Color
{
    vec3 c[9];
};

// ------------------------------------------------------------------
// FUNCTIONS --------------------------------------------------------
// ------------------------------------------------------------------

float area_integral(float x, float y)
{
    return atan(x * y, sqrt(x * x + y * y + 1));
}

// ------------------------------------------------------------------

float unlerp(float val, float max_val)
{
    return (val + 0.5) / max_val;
}

// ------------------------------------------------------------------

void project_onto_sh9(in vec3 dir, inout SH9 sh)
{
    // Band 0
    sh.c[0] = 0.282095;

    // Band 1
    sh.c[1] = -0.488603 * dir.y;
    sh.c[2] = 0.488603 * dir.z;
    sh.c[3] = -0.488603 * dir.x;

    // Band 2
    sh.c[4] = 1.092548 * dir.x * dir.y;
    sh.c[5] = -1.092548 * dir.y * dir.z;
    sh.c[6] = 0.315392 * (3.0 * dir.z * dir.z - 1.0);
    sh.c[7] = -1.092548 * dir.x * dir.z;
    sh.c[8] = 0.546274 * (dir.x * dir.x - dir.y * dir.y);
}

// ------------------------------------------------------------------

float calculate_solid_angle(uint x, uint y)
{
    float s = unlerp(float(x), u_Width) * 2.0 - 1.0;
    float t = unlerp(float(y), u_Height) * 2.0 - 1.0;

    // assumes square face
    float half_texel_size = 1.0 / u_Width;
    float x0              = s - half_texel_size;
    float y0              = t - half_texel_size;
    float x1              = s + half_texel_size;
    float y1              = t + half_texel_size;

    return area_integral(x0, y0) - area_integral(x0, y1) - area_integral(x1, y0) + area_integral(x1, y1);
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

// ------------------------------------------------------------------
// SHARED -----------------------------------------------------------
// ------------------------------------------------------------------

shared SH9Color g_sh_coeffs[LOCAL_SIZE][LOCAL_SIZE];
shared float    g_weights[LOCAL_SIZE][LOCAL_SIZE];

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    // Initialize shared memory
    for (int i = 0; i < 9; i++)
        g_sh_coeffs[gl_LocalInvocationID.x][gl_LocalInvocationID.y].c[i] = vec3(0.0);

    barrier();

    // Generate spherical harmonics basis
    SH9 basis;

    vec3  dir         = calculate_direction(gl_GlobalInvocationID.z, gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);
    float solid_angle = calculate_solid_angle(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);
    vec3  texel       = textureLod(s_Cubemap, dir, CUBEMAP_MIP_LEVEL).rgb;

    project_onto_sh9(dir, basis);

    g_weights[gl_LocalInvocationID.x][gl_LocalInvocationID.y] = solid_angle;

    for (int i = 0; i < 9; i++)
        g_sh_coeffs[gl_LocalInvocationID.x][gl_LocalInvocationID.y].c[i] += texel * basis.c[i] * solid_angle;

    barrier();

    // Add up the coefficients and weights along the X axis.
    if (gl_LocalInvocationID.x == 0)
    {
        for (int shared_idx = 1; shared_idx < LOCAL_SIZE; shared_idx++)
        {
            g_weights[0][gl_LocalInvocationID.y] += g_weights[shared_idx][gl_LocalInvocationID.y];

            for (int coef_idx = 0; coef_idx < 9; coef_idx++)
                g_sh_coeffs[0][gl_LocalInvocationID.y].c[coef_idx] += g_sh_coeffs[shared_idx][gl_LocalInvocationID.y].c[coef_idx];
        }
    }

    barrier();

    // Add up the coefficients and weights along the Y axis.
    if (gl_LocalInvocationID.x == 0 && gl_LocalInvocationID.y == 0)
    {
        for (int shared_idx = 1; shared_idx < LOCAL_SIZE; shared_idx++)
        {
            g_weights[0][0] += g_weights[0][shared_idx];

            for (int coef_idx = 0; coef_idx < 9; coef_idx++)
                g_sh_coeffs[0][0].c[coef_idx] += g_sh_coeffs[0][shared_idx].c[coef_idx];
        }

        // Write out the SH9 coefficients.
        for (int coef_idx = 0; coef_idx < 9; coef_idx++)
        {
            ivec3 p = ivec3((SH_INTERMEDIATE_SIZE * coef_idx) + (gl_GlobalInvocationID.x / LOCAL_SIZE), gl_GlobalInvocationID.y / LOCAL_SIZE, gl_GlobalInvocationID.z);
            imageStore(i_Cubemap, p, vec4(g_sh_coeffs[0][0].c[coef_idx], g_weights[0][0]));
        }
    }
}

// ------------------------------------------------------------------