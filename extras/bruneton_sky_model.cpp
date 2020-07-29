#include "sky_model.h"
#include <macros.h>
#include <utility.h>
#include <logger.h>
#include <stdio.h>
#include <gtc/matrix_transform.hpp>

namespace dw
{
#define CUBEMAP_SIZE 512

 static const char* g_copy_inscatter_1_src = R"(

#define NUM_THREADS 8

// ------------------------------------------------------------------
// INPUTS -----------------------------------------------------------
// ------------------------------------------------------------------

layout (local_size_x = NUM_THREADS, local_size_y = NUM_THREADS, local_size_z = 1) in;

// ------------------------------------------------------------------
// INPUT ------------------------------------------------------------
// ------------------------------------------------------------------

layout (binding = 0, rgba32f) uniform image3D i_InscatterWrite;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

uniform sampler3D s_DeltaSRRead; 
uniform sampler3D s_DeltaSMRead;

uniform int u_Layer;

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    vec4 ray = texelFetch(s_DeltaSRRead, ivec3(gl_GlobalInvocationID.xy, u_Layer), 0); 
    vec4 mie = texelFetch(s_DeltaSMRead, ivec3(gl_GlobalInvocationID.xy, u_Layer), 0); 
    
    // store only red component of single Mie scattering (cf. 'Angular precision') 
    imageStore(i_InscatterWrite, ivec3(gl_GlobalInvocationID.xy, u_Layer), vec4(ray.rgb, mie.r));
}

// ------------------------------------------------------------------

	)";

static const char* g_copy_inscatter_n_src = R"(

#define NUM_THREADS 8
 
// ---------------------------------------------------------------------------- 
// PHYSICAL MODEL PARAMETERS 
// ---------------------------------------------------------------------------- 
 
uniform float Rg;
uniform float Rt;
uniform float RL;
uniform float HR;
uniform float HM;
uniform float mieG;
uniform float AVERAGE_GROUND_REFLECTANCE;
uniform vec4 betaR;
uniform vec4 betaMSca;
uniform vec4 betaMEx;

// ---------------------------------------------------------------------------- 
// CONSTANT PARAMETERS 
// ---------------------------------------------------------------------------- 

uniform int first;
uniform int TRANSMITTANCE_H;
uniform int TRANSMITTANCE_W;
uniform int SKY_W;
uniform int SKY_H;
uniform int RES_R;
uniform int RES_MU;
uniform int RES_MU_S;
uniform int RES_NU;

// ---------------------------------------------------------------------------- 
// NUMERICAL INTEGRATION PARAMETERS 
// ---------------------------------------------------------------------------- 
 
#define TRANSMITTANCE_INTEGRAL_SAMPLES 500
#define INSCATTER_INTEGRAL_SAMPLES 50
#define IRRADIANCE_INTEGRAL_SAMPLES 32
#define INSCATTER_SPHERICAL_INTEGRAL_SAMPLES 16
 
#define M_PI 3.141592657
 
// ---------------------------------------------------------------------------- 
// PARAMETERIZATION OPTIONS 
// ---------------------------------------------------------------------------- 
 
#define TRANSMITTANCE_NON_LINEAR 
#define INSCATTER_NON_LINEAR 
 
// ---------------------------------------------------------------------------- 
// PARAMETERIZATION FUNCTIONS 
// ---------------------------------------------------------------------------- 

uniform sampler2D s_TransmittanceRead;

vec4 SamplePoint(sampler3D tex, vec3 uv, vec3 size)
{
	uv = clamp(uv, 0.0, 1.0);
	uv = uv * (size-1.0);
	return texelFetch(tex, ivec3(uv + 0.5), 0);
}

float mod(float x, float y) { return x - y * floor(x/y); }
  
vec2 GetTransmittanceUV(float r, float mu) 
{ 
    float uR, uMu; 
#ifdef TRANSMITTANCE_NON_LINEAR 
	uR = sqrt((r - Rg) / (Rt - Rg)); 
	uMu = atan((mu + 0.15) / (1.0 + 0.15) * tan(1.5)) / 1.5; 
#else 
	uR = (r - Rg) / (Rt - Rg); 
	uMu = (mu + 0.15) / (1.0 + 0.15); 
#endif 
    return vec2(uMu, uR); 
} 
 
void GetTransmittanceRMu(vec2 coord, out float r, out float muS) 
{ 
    r = coord.y / float(TRANSMITTANCE_H); 
    muS = coord.x / float(TRANSMITTANCE_W); 
#ifdef TRANSMITTANCE_NON_LINEAR 
    r = Rg + (r * r) * (Rt - Rg); 
    muS = -0.15 + tan(1.5 * muS) / tan(1.5) * (1.0 + 0.15); 
#else 
    r = Rg + r * (Rt - Rg); 
    muS = -0.15 + muS * (1.0 + 0.15); 
#endif 
}
 
vec2 GetIrradianceUV(float r, float muS) 
{ 
    float uR = (r - Rg) / (Rt - Rg); 
    float uMuS = (muS + 0.2) / (1.0 + 0.2); 
    return vec2(uMuS, uR); 
}  

void GetIrradianceRMuS(vec2 coord, out float r, out float muS) 
{ 
    r = Rg + (coord.y - 0.5) / (float(SKY_H) - 1.0) * (Rt - Rg); 
    muS = -0.2 + (coord.x - 0.5) / (float(SKY_W) - 1.0) * (1.0 + 0.2); 
}  

vec4 Texture4D(sampler3D tex, float r, float mu, float muS, float nu) 
{ 
    float H = sqrt(Rt * Rt - Rg * Rg); 
    float rho = sqrt(r * r - Rg * Rg); 
#ifdef INSCATTER_NON_LINEAR 
    float rmu = r * mu; 
    float delta = rmu * rmu - r * r + Rg * Rg; 
    vec4 cst = rmu < 0.0 && delta > 0.0 ? vec4(1.0, 0.0, 0.0, 0.5 - 0.5 / float(RES_MU)) : vec4(-1.0, H * H, H, 0.5 + 0.5 / float(RES_MU)); 
	float uR = 0.5 / float(RES_R) + rho / H * (1.0 - 1.0 / float(RES_R)); 
    float uMu = cst.w + (rmu * cst.x + sqrt(delta + cst.y)) / (rho + cst.z) * (0.5 - 1.0 / float(RES_MU)); 
    // paper formula 
    //float uMuS = 0.5 / float(RES_MU_S) + max((1.0 - exp(-3.0 * muS - 0.6)) / (1.0 - exp(-3.6)), 0.0) * (1.0 - 1.0 / float(RES_MU_S)); 
    // better formula 
    float uMuS = 0.5 / float(RES_MU_S) + (atan(max(muS, -0.1975) * tan(1.26 * 1.1)) / 1.1 + (1.0 - 0.26)) * 0.5 * (1.0 - 1.0 / float(RES_MU_S)); 
#else 
	float uR = 0.5 / float(RES_R) + rho / H * (1.0 - 1.0 / float(RES_R)); 
    float uMu = 0.5 / float(RES_MU) + (mu + 1.0) / 2.0 * (1.0 - 1.0 / float(RES_MU)); 
    float uMuS = 0.5 / float(RES_MU_S) + max(muS + 0.2, 0.0) / 1.2 * (1.0 - 1.0 / float(RES_MU_S)); 
#endif 
    float _lerp = (nu + 1.0) / 2.0 * (float(RES_NU) - 1.0); 
    float uNu = floor(_lerp); 
    _lerp = _lerp - uNu; 
    
    vec3 size = vec3(RES_MU_S*RES_NU,RES_MU,RES_R);
    
    return SamplePoint(tex, vec3((uNu + uMuS) / float(RES_NU), uMu, uR), size) * (1.0 - _lerp) + 
           SamplePoint(tex, vec3((uNu + uMuS + 1.0) / float(RES_NU), uMu, uR), size) * _lerp;

} 

void GetMuMuSNu(vec2 coord, float r, vec4 dhdH, out float mu, out float muS, out float nu) 
{ 
    float x = coord.x - 0.5; 
    float y = coord.y - 0.5; 
#ifdef INSCATTER_NON_LINEAR 
    if (y < float(RES_MU) / 2.0) 
    { 
        float d = 1.0 - y / (float(RES_MU) / 2.0 - 1.0); 
        d = min(max(dhdH.z, d * dhdH.w), dhdH.w * 0.999); 
        mu = (Rg * Rg - r * r - d * d) / (2.0 * r * d); 
        mu = min(mu, -sqrt(1.0 - (Rg / r) * (Rg / r)) - 0.001); 
    } 
    else 
    { 
        float d = (y - float(RES_MU) / 2.0) / (float(RES_MU) / 2.0 - 1.0); 
        d = min(max(dhdH.x, d * dhdH.y), dhdH.y * 0.999); 
        mu = (Rt * Rt - r * r - d * d) / (2.0 * r * d); 
    } 
    muS = mod(x, float(RES_MU_S)) / (float(RES_MU_S) - 1.0); 
    // paper formula 
    //muS = -(0.6 + log(1.0 - muS * (1.0 -  exp(-3.6)))) / 3.0; 
    // better formula 
    muS = tan((2.0 * muS - 1.0 + 0.26) * 1.1) / tan(1.26 * 1.1); 
    nu = -1.0 + floor(x / float(RES_MU_S)) / (float(RES_NU) - 1.0) * 2.0; 
#else 
    mu = -1.0 + 2.0 * y / (float(RES_MU) - 1.0); 
    muS = mod(x, float(RES_MU_S)) / (float(RES_MU_S) - 1.0); 
    muS = -0.2 + muS * 1.2; 
    nu = -1.0 + floor(x / float(RES_MU_S)) / (float(RES_NU) - 1.0) * 2.0; 
#endif 
} 

void GetLayer(int layer, out float r, out vec4 dhdH)
{
	r = float(layer) / (RES_R - 1.0);
	r = r * r;
	r = sqrt(Rg * Rg + r * (Rt * Rt - Rg * Rg)) + (layer == 0 ? 0.01 : (layer == RES_R - 1 ? -0.001 : 0.0));
	
	float dmin = Rt - r;
	float dmax = sqrt(r * r - Rg * Rg) + sqrt(Rt * Rt - Rg * Rg);
	float dminp = r - Rg;
	float dmaxp = sqrt(r * r - Rg * Rg);

	dhdH = vec4(dmin, dmax, dminp, dmaxp);	
}
 
// ---------------------------------------------------------------------------- 
// UTILITY FUNCTIONS 
// ---------------------------------------------------------------------------- 

// nearest intersection of ray r,mu with ground or top atmosphere boundary 
// mu=cos(ray zenith angle at ray origin) 
float Limit(float r, float mu) 
{ 
    float dout = -r * mu + sqrt(r * r * (mu * mu - 1.0) + RL * RL); 
    float delta2 = r * r * (mu * mu - 1.0) + Rg * Rg; 
    
    if (delta2 >= 0.0) 
    { 
        float din = -r * mu - sqrt(delta2); 
        if (din >= 0.0) 
        { 
            dout = min(dout, din); 
        } 
    } 
    
    return dout; 
}

// transmittance(=transparency) of atmosphere for infinite ray (r,mu) 
// (mu=cos(view zenith angle)), intersections with ground ignored 
vec3 Transmittance(float r, float mu) 
{ 
	vec2 uv = GetTransmittanceUV(r, mu);
    return textureLod(s_TransmittanceRead, uv, 0).rgb; 
} 

// transmittance(=transparency) of atmosphere between x and x0 
// assume segment x,x0 not intersecting ground 
// d = distance between x and x0, mu=cos(zenith angle of [x,x0) ray at x) 
vec3 Transmittance(float r, float mu, float d)
{ 
    vec3 result; 
    float r1 = sqrt(r * r + d * d + 2.0 * r * mu * d); 
    float mu1 = (r * mu + d) / r1; 
    if (mu > 0.0) { 
        result = min(Transmittance(r, mu) / Transmittance(r1, mu1), 1.0); 
    } else { 
        result = min(Transmittance(r1, -mu1) / Transmittance(r, -mu), 1.0); 
    } 
    return result; 
} 

vec3 Irradiance(sampler2D tex, float r, float muS) 
{ 
    vec2 uv = GetIrradianceUV(r, muS); 
    return textureLod(tex, uv, 0).rgb;
}  

// Rayleigh phase function 
float PhaseFunctionR(float mu) 
{ 
    return (3.0 / (16.0 * M_PI)) * (1.0 + mu * mu); 
} 

// Mie phase function 
float PhaseFunctionM(float mu) 
{ 
	return 1.5 * 1.0 / (4.0 * M_PI) * (1.0 - mieG*mieG) * pow(max(0.0, 1.0 + (mieG*mieG) - 2.0*mieG*mu), -3.0/2.0) * (1.0 + mu * mu) / (2.0 + mieG*mieG); 
} 
 
#define NUM_THREADS 8

// ------------------------------------------------------------------
// INPUTS -----------------------------------------------------------
// ------------------------------------------------------------------

layout (local_size_x = NUM_THREADS, local_size_y = NUM_THREADS, local_size_z = 1) in;

// ------------------------------------------------------------------
// INPUT ------------------------------------------------------------
// ------------------------------------------------------------------

layout (binding = 0, rgba32f) uniform image3D i_InscatterWrite;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

uniform sampler3D s_InscatterRead; 
uniform sampler3D s_DeltaSRead;

uniform int u_Layer;

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    vec4 dhdH;
    float mu, muS, nu, r;  
    vec2 coords = vec2(gl_GlobalInvocationID.xy) + 0.5; 
    
    GetLayer(u_Layer, r, dhdH); 
    GetMuMuSNu(coords, r, dhdH, mu, muS, nu); 
    
    ivec3 idx = ivec3(gl_GlobalInvocationID.xy, u_Layer);
    
    vec4 value = texelFetch(s_InscatterRead, idx, 0) + vec4(texelFetch(s_DeltaSRead, idx, 0).rgb / PhaseFunctionR(nu), 0.0);

    imageStore(i_InscatterWrite, idx, value); 
}

// ------------------------------------------------------------------

	)";

static const char* g_copy_irradiance_src = R"(

#define NUM_THREADS 8

// ------------------------------------------------------------------
// INPUTS -----------------------------------------------------------
// ------------------------------------------------------------------

layout (local_size_x = NUM_THREADS, local_size_y = NUM_THREADS, local_size_z = 1) in;

// ------------------------------------------------------------------
// INPUT ------------------------------------------------------------
// ------------------------------------------------------------------

layout (binding = 0, rgba32f) uniform image2D i_IrradianceWrite;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

uniform sampler2D s_DeltaERead; 
uniform sampler2D s_IrradianceRead;

uniform float u_K;

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    vec4 value = texelFetch(s_IrradianceRead, coord, 0) + u_K * texelFetch(s_DeltaERead, coord, 0);  
    imageStore(i_IrradianceWrite, coord, value); 
}

// ------------------------------------------------------------------

	)";

static const char* g_inscatter_1_src = R"(

#define NUM_THREADS 8
 
// ---------------------------------------------------------------------------- 
// PHYSICAL MODEL PARAMETERS 
// ---------------------------------------------------------------------------- 
 
uniform float Rg;
uniform float Rt;
uniform float RL;
uniform float HR;
uniform float HM;
uniform float mieG;
uniform float AVERAGE_GROUND_REFLECTANCE;
uniform vec4 betaR;
uniform vec4 betaMSca;
uniform vec4 betaMEx;

// ---------------------------------------------------------------------------- 
// CONSTANT PARAMETERS 
// ---------------------------------------------------------------------------- 

uniform int first;
uniform int TRANSMITTANCE_H;
uniform int TRANSMITTANCE_W;
uniform int SKY_W;
uniform int SKY_H;
uniform int RES_R;
uniform int RES_MU;
uniform int RES_MU_S;
uniform int RES_NU;

// ---------------------------------------------------------------------------- 
// NUMERICAL INTEGRATION PARAMETERS 
// ---------------------------------------------------------------------------- 
 
#define TRANSMITTANCE_INTEGRAL_SAMPLES 500
#define INSCATTER_INTEGRAL_SAMPLES 50
#define IRRADIANCE_INTEGRAL_SAMPLES 32
#define INSCATTER_SPHERICAL_INTEGRAL_SAMPLES 16
 
#define M_PI 3.141592657
 
// ---------------------------------------------------------------------------- 
// PARAMETERIZATION OPTIONS 
// ---------------------------------------------------------------------------- 
 
#define TRANSMITTANCE_NON_LINEAR 
#define INSCATTER_NON_LINEAR 
 
// ---------------------------------------------------------------------------- 
// PARAMETERIZATION FUNCTIONS 
// ---------------------------------------------------------------------------- 

uniform sampler2D s_TransmittanceRead;

vec4 SamplePoint(sampler3D tex, vec3 uv, vec3 size)
{
	uv = clamp(uv, 0.0, 1.0);
	uv = uv * (size-1.0);
	return texelFetch(tex, ivec3(uv + 0.5), 0);
}

float mod(float x, float y) { return x - y * floor(x/y); }
  
vec2 GetTransmittanceUV(float r, float mu) 
{ 
    float uR, uMu; 
#ifdef TRANSMITTANCE_NON_LINEAR 
	uR = sqrt((r - Rg) / (Rt - Rg)); 
	uMu = atan((mu + 0.15) / (1.0 + 0.15) * tan(1.5)) / 1.5; 
#else 
	uR = (r - Rg) / (Rt - Rg); 
	uMu = (mu + 0.15) / (1.0 + 0.15); 
#endif 
    return vec2(uMu, uR); 
} 
 
void GetTransmittanceRMu(vec2 coord, out float r, out float muS) 
{ 
    r = coord.y / float(TRANSMITTANCE_H); 
    muS = coord.x / float(TRANSMITTANCE_W); 
#ifdef TRANSMITTANCE_NON_LINEAR 
    r = Rg + (r * r) * (Rt - Rg); 
    muS = -0.15 + tan(1.5 * muS) / tan(1.5) * (1.0 + 0.15); 
#else 
    r = Rg + r * (Rt - Rg); 
    muS = -0.15 + muS * (1.0 + 0.15); 
#endif 
}
 
vec2 GetIrradianceUV(float r, float muS) 
{ 
    float uR = (r - Rg) / (Rt - Rg); 
    float uMuS = (muS + 0.2) / (1.0 + 0.2); 
    return vec2(uMuS, uR); 
}  

void GetIrradianceRMuS(vec2 coord, out float r, out float muS) 
{ 
    r = Rg + (coord.y - 0.5) / (float(SKY_H) - 1.0) * (Rt - Rg); 
    muS = -0.2 + (coord.x - 0.5) / (float(SKY_W) - 1.0) * (1.0 + 0.2); 
}  

vec4 Texture4D(sampler3D tex, float r, float mu, float muS, float nu) 
{ 
    float H = sqrt(Rt * Rt - Rg * Rg); 
    float rho = sqrt(r * r - Rg * Rg); 
#ifdef INSCATTER_NON_LINEAR 
    float rmu = r * mu; 
    float delta = rmu * rmu - r * r + Rg * Rg; 
    vec4 cst = rmu < 0.0 && delta > 0.0 ? vec4(1.0, 0.0, 0.0, 0.5 - 0.5 / float(RES_MU)) : vec4(-1.0, H * H, H, 0.5 + 0.5 / float(RES_MU)); 
	float uR = 0.5 / float(RES_R) + rho / H * (1.0 - 1.0 / float(RES_R)); 
    float uMu = cst.w + (rmu * cst.x + sqrt(delta + cst.y)) / (rho + cst.z) * (0.5 - 1.0 / float(RES_MU)); 
    // paper formula 
    //float uMuS = 0.5 / float(RES_MU_S) + max((1.0 - exp(-3.0 * muS - 0.6)) / (1.0 - exp(-3.6)), 0.0) * (1.0 - 1.0 / float(RES_MU_S)); 
    // better formula 
    float uMuS = 0.5 / float(RES_MU_S) + (atan(max(muS, -0.1975) * tan(1.26 * 1.1)) / 1.1 + (1.0 - 0.26)) * 0.5 * (1.0 - 1.0 / float(RES_MU_S)); 
#else 
	float uR = 0.5 / float(RES_R) + rho / H * (1.0 - 1.0 / float(RES_R)); 
    float uMu = 0.5 / float(RES_MU) + (mu + 1.0) / 2.0 * (1.0 - 1.0 / float(RES_MU)); 
    float uMuS = 0.5 / float(RES_MU_S) + max(muS + 0.2, 0.0) / 1.2 * (1.0 - 1.0 / float(RES_MU_S)); 
#endif 
    float _lerp = (nu + 1.0) / 2.0 * (float(RES_NU) - 1.0); 
    float uNu = floor(_lerp); 
    _lerp = _lerp - uNu; 
    
    vec3 size = vec3(RES_MU_S*RES_NU,RES_MU,RES_R);
    
    return SamplePoint(tex, vec3((uNu + uMuS) / float(RES_NU), uMu, uR), size) * (1.0 - _lerp) + 
           SamplePoint(tex, vec3((uNu + uMuS + 1.0) / float(RES_NU), uMu, uR), size) * _lerp;

} 

void GetMuMuSNu(vec2 coord, float r, vec4 dhdH, out float mu, out float muS, out float nu) 
{ 
    float x = coord.x - 0.5; 
    float y = coord.y - 0.5; 
#ifdef INSCATTER_NON_LINEAR 
    if (y < float(RES_MU) / 2.0) 
    { 
        float d = 1.0 - y / (float(RES_MU) / 2.0 - 1.0); 
        d = min(max(dhdH.z, d * dhdH.w), dhdH.w * 0.999); 
        mu = (Rg * Rg - r * r - d * d) / (2.0 * r * d); 
        mu = min(mu, -sqrt(1.0 - (Rg / r) * (Rg / r)) - 0.001); 
    } 
    else 
    { 
        float d = (y - float(RES_MU) / 2.0) / (float(RES_MU) / 2.0 - 1.0); 
        d = min(max(dhdH.x, d * dhdH.y), dhdH.y * 0.999); 
        mu = (Rt * Rt - r * r - d * d) / (2.0 * r * d); 
    } 
    muS = mod(x, float(RES_MU_S)) / (float(RES_MU_S) - 1.0); 
    // paper formula 
    //muS = -(0.6 + log(1.0 - muS * (1.0 -  exp(-3.6)))) / 3.0; 
    // better formula 
    muS = tan((2.0 * muS - 1.0 + 0.26) * 1.1) / tan(1.26 * 1.1); 
    nu = -1.0 + floor(x / float(RES_MU_S)) / (float(RES_NU) - 1.0) * 2.0; 
#else 
    mu = -1.0 + 2.0 * y / (float(RES_MU) - 1.0); 
    muS = mod(x, float(RES_MU_S)) / (float(RES_MU_S) - 1.0); 
    muS = -0.2 + muS * 1.2; 
    nu = -1.0 + floor(x / float(RES_MU_S)) / (float(RES_NU) - 1.0) * 2.0; 
#endif 
} 

void GetLayer(int layer, out float r, out vec4 dhdH)
{
	r = float(layer) / (RES_R - 1.0);
	r = r * r;
	r = sqrt(Rg * Rg + r * (Rt * Rt - Rg * Rg)) + (layer == 0 ? 0.01 : (layer == RES_R - 1 ? -0.001 : 0.0));
	
	float dmin = Rt - r;
	float dmax = sqrt(r * r - Rg * Rg) + sqrt(Rt * Rt - Rg * Rg);
	float dminp = r - Rg;
	float dmaxp = sqrt(r * r - Rg * Rg);

	dhdH = vec4(dmin, dmax, dminp, dmaxp);	
}
 
// ---------------------------------------------------------------------------- 
// UTILITY FUNCTIONS 
// ---------------------------------------------------------------------------- 

// nearest intersection of ray r,mu with ground or top atmosphere boundary 
// mu=cos(ray zenith angle at ray origin) 
float Limit(float r, float mu) 
{ 
    float dout = -r * mu + sqrt(r * r * (mu * mu - 1.0) + RL * RL); 
    float delta2 = r * r * (mu * mu - 1.0) + Rg * Rg; 
    
    if (delta2 >= 0.0) 
    { 
        float din = -r * mu - sqrt(delta2); 
        if (din >= 0.0) 
        { 
            dout = min(dout, din); 
        } 
    } 
    
    return dout; 
}

// transmittance(=transparency) of atmosphere for infinite ray (r,mu) 
// (mu=cos(view zenith angle)), intersections with ground ignored 
vec3 Transmittance(float r, float mu) 
{ 
	vec2 uv = GetTransmittanceUV(r, mu);
    return textureLod(s_TransmittanceRead, uv, 0).rgb; 
} 

// transmittance(=transparency) of atmosphere between x and x0 
// assume segment x,x0 not intersecting ground 
// d = distance between x and x0, mu=cos(zenith angle of [x,x0) ray at x) 
vec3 Transmittance(float r, float mu, float d)
{ 
    vec3 result; 
    float r1 = sqrt(r * r + d * d + 2.0 * r * mu * d); 
    float mu1 = (r * mu + d) / r1; 
    if (mu > 0.0) { 
        result = min(Transmittance(r, mu) / Transmittance(r1, mu1), 1.0); 
    } else { 
        result = min(Transmittance(r1, -mu1) / Transmittance(r, -mu), 1.0); 
    } 
    return result; 
} 

vec3 Irradiance(sampler2D tex, float r, float muS) 
{ 
    vec2 uv = GetIrradianceUV(r, muS); 
    return textureLod(tex, uv, 0).rgb;
}  

// Rayleigh phase function 
float PhaseFunctionR(float mu) 
{ 
    return (3.0 / (16.0 * M_PI)) * (1.0 + mu * mu); 
} 

// Mie phase function 
float PhaseFunctionM(float mu) 
{ 
	return 1.5 * 1.0 / (4.0 * M_PI) * (1.0 - mieG*mieG) * pow(max(0.0, 1.0 + (mieG*mieG) - 2.0*mieG*mu), -3.0/2.0) * (1.0 + mu * mu) / (2.0 + mieG*mieG); 
} 

// ------------------------------------------------------------------
// INPUTS -----------------------------------------------------------
// ------------------------------------------------------------------

layout (local_size_x = NUM_THREADS, local_size_y = NUM_THREADS, local_size_z = 1) in;

// ------------------------------------------------------------------
// INPUT ------------------------------------------------------------
// ------------------------------------------------------------------

layout (binding = 0, rgba32f) uniform image3D i_DeltaSRWrite;
layout (binding = 1, rgba32f) uniform image3D i_DeltaSMWrite;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

uniform int u_Layer;

// ------------------------------------------------------------------
// FUNCTIONS --------------------------------------------------------
// ------------------------------------------------------------------

void Integrand(float r, float mu, float muS, float nu, float t, out vec3 ray, out vec3 mie) 
{ 
    ray = vec3(0,0,0); 
    mie = vec3(0,0,0); 
    float ri = sqrt(r * r + t * t + 2.0 * r * mu * t); 
    float muSi = (nu * t + muS * r) / ri; 
    ri = max(Rg, ri); 
    if (muSi >= -sqrt(1.0 - Rg * Rg / (ri * ri))) 
    { 
        vec3 ti = Transmittance(r, mu, t) * Transmittance(ri, muSi); 
        ray = exp(-(ri - Rg) / HR) * ti; 
        mie = exp(-(ri - Rg) / HM) * ti; 
    } 
} 

// ------------------------------------------------------------------

void Inscatter(float r, float mu, float muS, float nu, out vec3 ray, out vec3 mie) 
{ 
    ray = vec3(0,0,0); 
    mie = vec3(0,0,0);
    float dx = Limit(r, mu) / float(INSCATTER_INTEGRAL_SAMPLES); 
    float xi = 0.0; 
    vec3 rayi; 
    vec3 miei; 
    Integrand(r, mu, muS, nu, 0.0, rayi, miei); 
    
    for (int i = 1; i <= INSCATTER_INTEGRAL_SAMPLES; ++i) 
    { 
        float xj = float(i) * dx; 
        vec3 rayj; 
        vec3 miej; 
        Integrand(r, mu, muS, nu, xj, rayj, miej); 
        
        ray += (rayi + rayj) / 2.0 * dx; 
        mie += (miei + miej) / 2.0 * dx; 
        xi = xj; 
        rayi = rayj; 
        miei = miej; 
    } 
    
    ray *= betaR.xyz; 
    mie *= betaMSca.xyz; 
} 

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    vec3 ray; 
    vec3 mie;
    vec4 dhdH;
    float mu, muS, nu, r;
    vec2 coords = vec2(gl_GlobalInvocationID.xy) + 0.5; 
    
    GetLayer(u_Layer, r, dhdH); 
    GetMuMuSNu(coords, r, dhdH, mu, muS, nu); 
    
    Inscatter(r, mu, muS, nu, ray, mie); 
    
    // store separately Rayleigh and Mie contributions, WITHOUT the phase function factor 
    // (cf 'Angular precision') 
    imageStore(i_DeltaSRWrite, ivec3(gl_GlobalInvocationID.xy, u_Layer), vec4(ray,0));
    imageStore(i_DeltaSMWrite, ivec3(gl_GlobalInvocationID.xy, u_Layer), vec4(mie,0));
}

// ------------------------------------------------------------------

	)";

static const char* g_inscatter_n_src = R"(

#define NUM_THREADS 8
 
// ---------------------------------------------------------------------------- 
// PHYSICAL MODEL PARAMETERS 
// ---------------------------------------------------------------------------- 
 
uniform float Rg;
uniform float Rt;
uniform float RL;
uniform float HR;
uniform float HM;
uniform float mieG;
uniform float AVERAGE_GROUND_REFLECTANCE;
uniform vec4 betaR;
uniform vec4 betaMSca;
uniform vec4 betaMEx;

// ---------------------------------------------------------------------------- 
// CONSTANT PARAMETERS 
// ---------------------------------------------------------------------------- 

uniform int first;
uniform int TRANSMITTANCE_H;
uniform int TRANSMITTANCE_W;
uniform int SKY_W;
uniform int SKY_H;
uniform int RES_R;
uniform int RES_MU;
uniform int RES_MU_S;
uniform int RES_NU;

// ---------------------------------------------------------------------------- 
// NUMERICAL INTEGRATION PARAMETERS 
// ---------------------------------------------------------------------------- 
 
#define TRANSMITTANCE_INTEGRAL_SAMPLES 500
#define INSCATTER_INTEGRAL_SAMPLES 50
#define IRRADIANCE_INTEGRAL_SAMPLES 32
#define INSCATTER_SPHERICAL_INTEGRAL_SAMPLES 16
 
#define M_PI 3.141592657
 
// ---------------------------------------------------------------------------- 
// PARAMETERIZATION OPTIONS 
// ---------------------------------------------------------------------------- 
 
#define TRANSMITTANCE_NON_LINEAR 
#define INSCATTER_NON_LINEAR 
 
// ---------------------------------------------------------------------------- 
// PARAMETERIZATION FUNCTIONS 
// ---------------------------------------------------------------------------- 

uniform sampler2D s_TransmittanceRead;

vec4 SamplePoint(sampler3D tex, vec3 uv, vec3 size)
{
	uv = clamp(uv, 0.0, 1.0);
	uv = uv * (size-1.0);
	return texelFetch(tex, ivec3(uv + 0.5), 0);
}

float mod(float x, float y) { return x - y * floor(x/y); }
  
vec2 GetTransmittanceUV(float r, float mu) 
{ 
    float uR, uMu; 
#ifdef TRANSMITTANCE_NON_LINEAR 
	uR = sqrt((r - Rg) / (Rt - Rg)); 
	uMu = atan((mu + 0.15) / (1.0 + 0.15) * tan(1.5)) / 1.5; 
#else 
	uR = (r - Rg) / (Rt - Rg); 
	uMu = (mu + 0.15) / (1.0 + 0.15); 
#endif 
    return vec2(uMu, uR); 
} 
 
void GetTransmittanceRMu(vec2 coord, out float r, out float muS) 
{ 
    r = coord.y / float(TRANSMITTANCE_H); 
    muS = coord.x / float(TRANSMITTANCE_W); 
#ifdef TRANSMITTANCE_NON_LINEAR 
    r = Rg + (r * r) * (Rt - Rg); 
    muS = -0.15 + tan(1.5 * muS) / tan(1.5) * (1.0 + 0.15); 
#else 
    r = Rg + r * (Rt - Rg); 
    muS = -0.15 + muS * (1.0 + 0.15); 
#endif 
}
 
vec2 GetIrradianceUV(float r, float muS) 
{ 
    float uR = (r - Rg) / (Rt - Rg); 
    float uMuS = (muS + 0.2) / (1.0 + 0.2); 
    return vec2(uMuS, uR); 
}  

void GetIrradianceRMuS(vec2 coord, out float r, out float muS) 
{ 
    r = Rg + (coord.y - 0.5) / (float(SKY_H) - 1.0) * (Rt - Rg); 
    muS = -0.2 + (coord.x - 0.5) / (float(SKY_W) - 1.0) * (1.0 + 0.2); 
}  

vec4 Texture4D(sampler3D tex, float r, float mu, float muS, float nu) 
{ 
    float H = sqrt(Rt * Rt - Rg * Rg); 
    float rho = sqrt(r * r - Rg * Rg); 
#ifdef INSCATTER_NON_LINEAR 
    float rmu = r * mu; 
    float delta = rmu * rmu - r * r + Rg * Rg; 
    vec4 cst = rmu < 0.0 && delta > 0.0 ? vec4(1.0, 0.0, 0.0, 0.5 - 0.5 / float(RES_MU)) : vec4(-1.0, H * H, H, 0.5 + 0.5 / float(RES_MU)); 
	float uR = 0.5 / float(RES_R) + rho / H * (1.0 - 1.0 / float(RES_R)); 
    float uMu = cst.w + (rmu * cst.x + sqrt(delta + cst.y)) / (rho + cst.z) * (0.5 - 1.0 / float(RES_MU)); 
    // paper formula 
    //float uMuS = 0.5 / float(RES_MU_S) + max((1.0 - exp(-3.0 * muS - 0.6)) / (1.0 - exp(-3.6)), 0.0) * (1.0 - 1.0 / float(RES_MU_S)); 
    // better formula 
    float uMuS = 0.5 / float(RES_MU_S) + (atan(max(muS, -0.1975) * tan(1.26 * 1.1)) / 1.1 + (1.0 - 0.26)) * 0.5 * (1.0 - 1.0 / float(RES_MU_S)); 
#else 
	float uR = 0.5 / float(RES_R) + rho / H * (1.0 - 1.0 / float(RES_R)); 
    float uMu = 0.5 / float(RES_MU) + (mu + 1.0) / 2.0 * (1.0 - 1.0 / float(RES_MU)); 
    float uMuS = 0.5 / float(RES_MU_S) + max(muS + 0.2, 0.0) / 1.2 * (1.0 - 1.0 / float(RES_MU_S)); 
#endif 
    float _lerp = (nu + 1.0) / 2.0 * (float(RES_NU) - 1.0); 
    float uNu = floor(_lerp); 
    _lerp = _lerp - uNu; 
    
    vec3 size = vec3(RES_MU_S*RES_NU,RES_MU,RES_R);
    
    return SamplePoint(tex, vec3((uNu + uMuS) / float(RES_NU), uMu, uR), size) * (1.0 - _lerp) + 
           SamplePoint(tex, vec3((uNu + uMuS + 1.0) / float(RES_NU), uMu, uR), size) * _lerp;

} 

void GetMuMuSNu(vec2 coord, float r, vec4 dhdH, out float mu, out float muS, out float nu) 
{ 
    float x = coord.x - 0.5; 
    float y = coord.y - 0.5; 
#ifdef INSCATTER_NON_LINEAR 
    if (y < float(RES_MU) / 2.0) 
    { 
        float d = 1.0 - y / (float(RES_MU) / 2.0 - 1.0); 
        d = min(max(dhdH.z, d * dhdH.w), dhdH.w * 0.999); 
        mu = (Rg * Rg - r * r - d * d) / (2.0 * r * d); 
        mu = min(mu, -sqrt(1.0 - (Rg / r) * (Rg / r)) - 0.001); 
    } 
    else 
    { 
        float d = (y - float(RES_MU) / 2.0) / (float(RES_MU) / 2.0 - 1.0); 
        d = min(max(dhdH.x, d * dhdH.y), dhdH.y * 0.999); 
        mu = (Rt * Rt - r * r - d * d) / (2.0 * r * d); 
    } 
    muS = mod(x, float(RES_MU_S)) / (float(RES_MU_S) - 1.0); 
    // paper formula 
    //muS = -(0.6 + log(1.0 - muS * (1.0 -  exp(-3.6)))) / 3.0; 
    // better formula 
    muS = tan((2.0 * muS - 1.0 + 0.26) * 1.1) / tan(1.26 * 1.1); 
    nu = -1.0 + floor(x / float(RES_MU_S)) / (float(RES_NU) - 1.0) * 2.0; 
#else 
    mu = -1.0 + 2.0 * y / (float(RES_MU) - 1.0); 
    muS = mod(x, float(RES_MU_S)) / (float(RES_MU_S) - 1.0); 
    muS = -0.2 + muS * 1.2; 
    nu = -1.0 + floor(x / float(RES_MU_S)) / (float(RES_NU) - 1.0) * 2.0; 
#endif 
} 

void GetLayer(int layer, out float r, out vec4 dhdH)
{
	r = float(layer) / (RES_R - 1.0);
	r = r * r;
	r = sqrt(Rg * Rg + r * (Rt * Rt - Rg * Rg)) + (layer == 0 ? 0.01 : (layer == RES_R - 1 ? -0.001 : 0.0));
	
	float dmin = Rt - r;
	float dmax = sqrt(r * r - Rg * Rg) + sqrt(Rt * Rt - Rg * Rg);
	float dminp = r - Rg;
	float dmaxp = sqrt(r * r - Rg * Rg);

	dhdH = vec4(dmin, dmax, dminp, dmaxp);	
}
 
// ---------------------------------------------------------------------------- 
// UTILITY FUNCTIONS 
// ---------------------------------------------------------------------------- 

// nearest intersection of ray r,mu with ground or top atmosphere boundary 
// mu=cos(ray zenith angle at ray origin) 
float Limit(float r, float mu) 
{ 
    float dout = -r * mu + sqrt(r * r * (mu * mu - 1.0) + RL * RL); 
    float delta2 = r * r * (mu * mu - 1.0) + Rg * Rg; 
    
    if (delta2 >= 0.0) 
    { 
        float din = -r * mu - sqrt(delta2); 
        if (din >= 0.0) 
        { 
            dout = min(dout, din); 
        } 
    } 
    
    return dout; 
}

// transmittance(=transparency) of atmosphere for infinite ray (r,mu) 
// (mu=cos(view zenith angle)), intersections with ground ignored 
vec3 Transmittance(float r, float mu) 
{ 
	vec2 uv = GetTransmittanceUV(r, mu);
    return textureLod(s_TransmittanceRead, uv, 0).rgb; 
} 

// transmittance(=transparency) of atmosphere between x and x0 
// assume segment x,x0 not intersecting ground 
// d = distance between x and x0, mu=cos(zenith angle of [x,x0) ray at x) 
vec3 Transmittance(float r, float mu, float d)
{ 
    vec3 result; 
    float r1 = sqrt(r * r + d * d + 2.0 * r * mu * d); 
    float mu1 = (r * mu + d) / r1; 
    if (mu > 0.0) { 
        result = min(Transmittance(r, mu) / Transmittance(r1, mu1), 1.0); 
    } else { 
        result = min(Transmittance(r1, -mu1) / Transmittance(r, -mu), 1.0); 
    } 
    return result; 
} 

vec3 Irradiance(sampler2D tex, float r, float muS) 
{ 
    vec2 uv = GetIrradianceUV(r, muS); 
    return textureLod(tex, uv, 0).rgb;
}  

// Rayleigh phase function 
float PhaseFunctionR(float mu) 
{ 
    return (3.0 / (16.0 * M_PI)) * (1.0 + mu * mu); 
} 

// Mie phase function 
float PhaseFunctionM(float mu) 
{ 
	return 1.5 * 1.0 / (4.0 * M_PI) * (1.0 - mieG*mieG) * pow(max(0.0, 1.0 + (mieG*mieG) - 2.0*mieG*mu), -3.0/2.0) * (1.0 + mu * mu) / (2.0 + mieG*mieG); 
} 

// ------------------------------------------------------------------
// INPUTS -----------------------------------------------------------
// ------------------------------------------------------------------

layout (local_size_x = NUM_THREADS, local_size_y = NUM_THREADS, local_size_z = 1) in;

// ------------------------------------------------------------------
// INPUT ------------------------------------------------------------
// ------------------------------------------------------------------

layout (binding = 0, rgba32f) uniform image3D i_DeltaSRWrite;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

uniform sampler3D s_DeltaJRead; 

uniform int u_Layer;

// ------------------------------------------------------------------
// FUNCTIONS --------------------------------------------------------
// ------------------------------------------------------------------

vec3 Integrand(float r, float mu, float muS, float nu, float t) 
{ 
    float ri = sqrt(r * r + t * t + 2.0 * r * mu * t); 
    float mui = (r * mu + t) / ri; 
    float muSi = (nu * t + muS * r) / ri; 
    return Texture4D(s_DeltaJRead, ri, mui, muSi, nu).rgb * Transmittance(r, mu, t); 
} 
 
// ------------------------------------------------------------------

vec3 Inscatter(float r, float mu, float muS, float nu) 
{ 
    vec3 raymie = vec3(0,0,0); 
    float dx = Limit(r, mu) / float(INSCATTER_INTEGRAL_SAMPLES); 
    float xi = 0.0; 
    vec3 raymiei = Integrand(r, mu, muS, nu, 0.0); 
    
    for (int i = 1; i <= INSCATTER_INTEGRAL_SAMPLES; ++i) 
    { 
        float xj = float(i) * dx; 
        vec3 raymiej = Integrand(r, mu, muS, nu, xj); 
        raymie += (raymiei + raymiej) / 2.0 * dx; 
        xi = xj; 
        raymiei = raymiej; 
    } 
    
    return raymie; 
} 

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    vec4 dhdH;
    float mu, muS, nu, r; 
    vec2 coords = vec2(gl_GlobalInvocationID.xy) + 0.5;  
    
    GetLayer(u_Layer, r, dhdH); 
    GetMuMuSNu(coords, r, dhdH, mu, muS, nu); 

    imageStore(i_DeltaSRWrite, ivec3(gl_GlobalInvocationID.xy, u_Layer), vec4(Inscatter(r, mu, muS, nu), 0));
}

// ------------------------------------------------------------------

	)";

static const char* g_inscatter_s_src = R"(

	#define NUM_THREADS 8
 
// ---------------------------------------------------------------------------- 
// PHYSICAL MODEL PARAMETERS 
// ---------------------------------------------------------------------------- 
 
uniform float Rg;
uniform float Rt;
uniform float RL;
uniform float HR;
uniform float HM;
uniform float mieG;
uniform float AVERAGE_GROUND_REFLECTANCE;
uniform vec4 betaR;
uniform vec4 betaMSca;
uniform vec4 betaMEx;

// ---------------------------------------------------------------------------- 
// CONSTANT PARAMETERS 
// ---------------------------------------------------------------------------- 

uniform int first;
uniform int TRANSMITTANCE_H;
uniform int TRANSMITTANCE_W;
uniform int SKY_W;
uniform int SKY_H;
uniform int RES_R;
uniform int RES_MU;
uniform int RES_MU_S;
uniform int RES_NU;

// ---------------------------------------------------------------------------- 
// NUMERICAL INTEGRATION PARAMETERS 
// ---------------------------------------------------------------------------- 
 
#define TRANSMITTANCE_INTEGRAL_SAMPLES 500
#define INSCATTER_INTEGRAL_SAMPLES 50
#define IRRADIANCE_INTEGRAL_SAMPLES 32
#define INSCATTER_SPHERICAL_INTEGRAL_SAMPLES 16
 
#define M_PI 3.141592657
 
// ---------------------------------------------------------------------------- 
// PARAMETERIZATION OPTIONS 
// ---------------------------------------------------------------------------- 
 
#define TRANSMITTANCE_NON_LINEAR 
#define INSCATTER_NON_LINEAR 
 
// ---------------------------------------------------------------------------- 
// PARAMETERIZATION FUNCTIONS 
// ---------------------------------------------------------------------------- 

uniform sampler2D s_TransmittanceRead;

vec4 SamplePoint(sampler3D tex, vec3 uv, vec3 size)
{
	uv = clamp(uv, 0.0, 1.0);
	uv = uv * (size-1.0);
	return texelFetch(tex, ivec3(uv + 0.5), 0);
}

float mod(float x, float y) { return x - y * floor(x/y); }
  
vec2 GetTransmittanceUV(float r, float mu) 
{ 
    float uR, uMu; 
#ifdef TRANSMITTANCE_NON_LINEAR 
	uR = sqrt((r - Rg) / (Rt - Rg)); 
	uMu = atan((mu + 0.15) / (1.0 + 0.15) * tan(1.5)) / 1.5; 
#else 
	uR = (r - Rg) / (Rt - Rg); 
	uMu = (mu + 0.15) / (1.0 + 0.15); 
#endif 
    return vec2(uMu, uR); 
} 
 
void GetTransmittanceRMu(vec2 coord, out float r, out float muS) 
{ 
    r = coord.y / float(TRANSMITTANCE_H); 
    muS = coord.x / float(TRANSMITTANCE_W); 
#ifdef TRANSMITTANCE_NON_LINEAR 
    r = Rg + (r * r) * (Rt - Rg); 
    muS = -0.15 + tan(1.5 * muS) / tan(1.5) * (1.0 + 0.15); 
#else 
    r = Rg + r * (Rt - Rg); 
    muS = -0.15 + muS * (1.0 + 0.15); 
#endif 
}
 
vec2 GetIrradianceUV(float r, float muS) 
{ 
    float uR = (r - Rg) / (Rt - Rg); 
    float uMuS = (muS + 0.2) / (1.0 + 0.2); 
    return vec2(uMuS, uR); 
}  

void GetIrradianceRMuS(vec2 coord, out float r, out float muS) 
{ 
    r = Rg + (coord.y - 0.5) / (float(SKY_H) - 1.0) * (Rt - Rg); 
    muS = -0.2 + (coord.x - 0.5) / (float(SKY_W) - 1.0) * (1.0 + 0.2); 
}  

vec4 Texture4D(sampler3D tex, float r, float mu, float muS, float nu) 
{ 
    float H = sqrt(Rt * Rt - Rg * Rg); 
    float rho = sqrt(r * r - Rg * Rg); 
#ifdef INSCATTER_NON_LINEAR 
    float rmu = r * mu; 
    float delta = rmu * rmu - r * r + Rg * Rg; 
    vec4 cst = rmu < 0.0 && delta > 0.0 ? vec4(1.0, 0.0, 0.0, 0.5 - 0.5 / float(RES_MU)) : vec4(-1.0, H * H, H, 0.5 + 0.5 / float(RES_MU)); 
	float uR = 0.5 / float(RES_R) + rho / H * (1.0 - 1.0 / float(RES_R)); 
    float uMu = cst.w + (rmu * cst.x + sqrt(delta + cst.y)) / (rho + cst.z) * (0.5 - 1.0 / float(RES_MU)); 
    // paper formula 
    //float uMuS = 0.5 / float(RES_MU_S) + max((1.0 - exp(-3.0 * muS - 0.6)) / (1.0 - exp(-3.6)), 0.0) * (1.0 - 1.0 / float(RES_MU_S)); 
    // better formula 
    float uMuS = 0.5 / float(RES_MU_S) + (atan(max(muS, -0.1975) * tan(1.26 * 1.1)) / 1.1 + (1.0 - 0.26)) * 0.5 * (1.0 - 1.0 / float(RES_MU_S)); 
#else 
	float uR = 0.5 / float(RES_R) + rho / H * (1.0 - 1.0 / float(RES_R)); 
    float uMu = 0.5 / float(RES_MU) + (mu + 1.0) / 2.0 * (1.0 - 1.0 / float(RES_MU)); 
    float uMuS = 0.5 / float(RES_MU_S) + max(muS + 0.2, 0.0) / 1.2 * (1.0 - 1.0 / float(RES_MU_S)); 
#endif 
    float _lerp = (nu + 1.0) / 2.0 * (float(RES_NU) - 1.0); 
    float uNu = floor(_lerp); 
    _lerp = _lerp - uNu; 
    
    vec3 size = vec3(RES_MU_S*RES_NU,RES_MU,RES_R);
    
    return SamplePoint(tex, vec3((uNu + uMuS) / float(RES_NU), uMu, uR), size) * (1.0 - _lerp) + 
           SamplePoint(tex, vec3((uNu + uMuS + 1.0) / float(RES_NU), uMu, uR), size) * _lerp;

} 

void GetMuMuSNu(vec2 coord, float r, vec4 dhdH, out float mu, out float muS, out float nu) 
{ 
    float x = coord.x - 0.5; 
    float y = coord.y - 0.5; 
#ifdef INSCATTER_NON_LINEAR 
    if (y < float(RES_MU) / 2.0) 
    { 
        float d = 1.0 - y / (float(RES_MU) / 2.0 - 1.0); 
        d = min(max(dhdH.z, d * dhdH.w), dhdH.w * 0.999); 
        mu = (Rg * Rg - r * r - d * d) / (2.0 * r * d); 
        mu = min(mu, -sqrt(1.0 - (Rg / r) * (Rg / r)) - 0.001); 
    } 
    else 
    { 
        float d = (y - float(RES_MU) / 2.0) / (float(RES_MU) / 2.0 - 1.0); 
        d = min(max(dhdH.x, d * dhdH.y), dhdH.y * 0.999); 
        mu = (Rt * Rt - r * r - d * d) / (2.0 * r * d); 
    } 
    muS = mod(x, float(RES_MU_S)) / (float(RES_MU_S) - 1.0); 
    // paper formula 
    //muS = -(0.6 + log(1.0 - muS * (1.0 -  exp(-3.6)))) / 3.0; 
    // better formula 
    muS = tan((2.0 * muS - 1.0 + 0.26) * 1.1) / tan(1.26 * 1.1); 
    nu = -1.0 + floor(x / float(RES_MU_S)) / (float(RES_NU) - 1.0) * 2.0; 
#else 
    mu = -1.0 + 2.0 * y / (float(RES_MU) - 1.0); 
    muS = mod(x, float(RES_MU_S)) / (float(RES_MU_S) - 1.0); 
    muS = -0.2 + muS * 1.2; 
    nu = -1.0 + floor(x / float(RES_MU_S)) / (float(RES_NU) - 1.0) * 2.0; 
#endif 
} 

void GetLayer(int layer, out float r, out vec4 dhdH)
{
	r = float(layer) / (RES_R - 1.0);
	r = r * r;
	r = sqrt(Rg * Rg + r * (Rt * Rt - Rg * Rg)) + (layer == 0 ? 0.01 : (layer == RES_R - 1 ? -0.001 : 0.0));
	
	float dmin = Rt - r;
	float dmax = sqrt(r * r - Rg * Rg) + sqrt(Rt * Rt - Rg * Rg);
	float dminp = r - Rg;
	float dmaxp = sqrt(r * r - Rg * Rg);

	dhdH = vec4(dmin, dmax, dminp, dmaxp);	
}
 
// ---------------------------------------------------------------------------- 
// UTILITY FUNCTIONS 
// ---------------------------------------------------------------------------- 

// nearest intersection of ray r,mu with ground or top atmosphere boundary 
// mu=cos(ray zenith angle at ray origin) 
float Limit(float r, float mu) 
{ 
    float dout = -r * mu + sqrt(r * r * (mu * mu - 1.0) + RL * RL); 
    float delta2 = r * r * (mu * mu - 1.0) + Rg * Rg; 
    
    if (delta2 >= 0.0) 
    { 
        float din = -r * mu - sqrt(delta2); 
        if (din >= 0.0) 
        { 
            dout = min(dout, din); 
        } 
    } 
    
    return dout; 
}

// transmittance(=transparency) of atmosphere for infinite ray (r,mu) 
// (mu=cos(view zenith angle)), intersections with ground ignored 
vec3 Transmittance(float r, float mu) 
{ 
	vec2 uv = GetTransmittanceUV(r, mu);
    return textureLod(s_TransmittanceRead, uv, 0).rgb; 
} 

// transmittance(=transparency) of atmosphere between x and x0 
// assume segment x,x0 not intersecting ground 
// d = distance between x and x0, mu=cos(zenith angle of [x,x0) ray at x) 
vec3 Transmittance(float r, float mu, float d)
{ 
    vec3 result; 
    float r1 = sqrt(r * r + d * d + 2.0 * r * mu * d); 
    float mu1 = (r * mu + d) / r1; 
    if (mu > 0.0) { 
        result = min(Transmittance(r, mu) / Transmittance(r1, mu1), 1.0); 
    } else { 
        result = min(Transmittance(r1, -mu1) / Transmittance(r, -mu), 1.0); 
    } 
    return result; 
} 

vec3 Irradiance(sampler2D tex, float r, float muS) 
{ 
    vec2 uv = GetIrradianceUV(r, muS); 
    return textureLod(tex, uv, 0).rgb;
}  

// Rayleigh phase function 
float PhaseFunctionR(float mu) 
{ 
    return (3.0 / (16.0 * M_PI)) * (1.0 + mu * mu); 
} 

// Mie phase function 
float PhaseFunctionM(float mu) 
{ 
	return 1.5 * 1.0 / (4.0 * M_PI) * (1.0 - mieG*mieG) * pow(max(0.0, 1.0 + (mieG*mieG) - 2.0*mieG*mu), -3.0/2.0) * (1.0 + mu * mu) / (2.0 + mieG*mieG); 
} 

// ------------------------------------------------------------------
// INPUTS -----------------------------------------------------------
// ------------------------------------------------------------------

layout (local_size_x = NUM_THREADS, local_size_y = NUM_THREADS, local_size_z = 1) in;

// ------------------------------------------------------------------
// INPUT ------------------------------------------------------------
// ------------------------------------------------------------------

layout (binding = 0, rgba32f) uniform image3D i_DeltaJWrite;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

uniform sampler2D s_DeltaERead; 
uniform sampler3D s_DeltaSRRead;
uniform sampler3D s_DeltaSMRead;

uniform int u_Layer;

// ------------------------------------------------------------------
// FUNCTIONS --------------------------------------------------------
// ------------------------------------------------------------------


void Inscatter(float r, float mu, float muS, float nu, out vec3 raymie) 
{ 
	float dphi = M_PI / float(INSCATTER_SPHERICAL_INTEGRAL_SAMPLES); 
	float dtheta = M_PI / float(INSCATTER_SPHERICAL_INTEGRAL_SAMPLES);

    r = clamp(r, Rg, Rt); 
    mu = clamp(mu, -1.0, 1.0); 
    muS = clamp(muS, -1.0, 1.0); 
    float var = sqrt(1.0 - mu * mu) * sqrt(1.0 - muS * muS); 
    nu = clamp(nu, muS * mu - var, muS * mu + var); 
 
    float cthetamin = -sqrt(1.0 - (Rg / r) * (Rg / r)); 
 
    vec3 v = vec3(sqrt(1.0 - mu * mu), 0.0, mu); 
    float sx = v.x == 0.0 ? 0.0 : (nu - muS * mu) / v.x; 
    vec3 s = vec3(sx, sqrt(max(0.0, 1.0 - sx * sx - muS * muS)), muS); 
 
    raymie = vec3(0,0,0); 
 
    // integral over 4.PI around x with two nested loops over w directions (theta,phi) -- Eq (7) 
    for (int itheta = 0; itheta < INSCATTER_SPHERICAL_INTEGRAL_SAMPLES; ++itheta) 
    { 
        float theta = (float(itheta) + 0.5) * dtheta; 
        float ctheta = cos(theta); 
 
        float greflectance = 0.0; 
        float dground = 0.0; 
        vec3 gtransp = vec3(0,0,0);
         
        if (ctheta < cthetamin) 
        { 	// if ground visible in direction w 
            // compute transparency gtransp between x and ground 
            greflectance = AVERAGE_GROUND_REFLECTANCE / M_PI; 
            dground = -r * ctheta - sqrt(r * r * (ctheta * ctheta - 1.0) + Rg * Rg); 
            gtransp = Transmittance(Rg, -(r * ctheta + dground) / Rg, dground); 
        } 
 
        for (int iphi = 0; iphi < 2 * INSCATTER_SPHERICAL_INTEGRAL_SAMPLES; ++iphi) 
        { 
            float phi = (float(iphi) + 0.5) * dphi; 
            float dw = dtheta * dphi * sin(theta); 
            vec3 w = vec3(cos(phi) * sin(theta), sin(phi) * sin(theta), ctheta); 
 
            float nu1 = dot(s, w); 
            float nu2 = dot(v, w); 
            float pr2 = PhaseFunctionR(nu2); 
            float pm2 = PhaseFunctionM(nu2); 
 
            // compute irradiance received at ground in direction w (if ground visible) =deltaE 
            vec3 gnormal = (vec3(0.0, 0.0, r) + dground * w) / Rg; 
            vec3 girradiance = Irradiance(s_DeltaERead, Rg, dot(gnormal, s)); 
 
            vec3 raymie1; // light arriving at x from direction w 
 
            // first term = light reflected from the ground and attenuated before reaching x, =T.alpha/PI.deltaE 
            raymie1 = greflectance * girradiance * gtransp; 
 
            // second term = inscattered light, =deltaS 
            if (first == 1) 
            { 
                // first iteration is special because Rayleigh and Mie were stored separately, 
                // without the phase functions factors; they must be reintroduced here 
                float pr1 = PhaseFunctionR(nu1); 
                float pm1 = PhaseFunctionM(nu1); 
                vec3 ray1 = Texture4D(s_DeltaSRRead, r, w.z, muS, nu1).rgb; 
                vec3 mie1 = Texture4D(s_DeltaSMRead, r, w.z, muS, nu1).rgb; 
                raymie1 += ray1 * pr1 + mie1 * pm1; 
            } 
            else 
            { 
                raymie1 += Texture4D(s_DeltaSRRead, r, w.z, muS, nu1).rgb; 
            } 
 
            // light coming from direction w and scattered in direction v 
            // = light arriving at x from direction w (raymie1) * SUM(scattering coefficient * phaseFunction) 
            // see Eq (7) 
            raymie += raymie1 * (betaR.xyz * exp(-(r - Rg) / HR) * pr2 + betaMSca.xyz * exp(-(r - Rg) / HM) * pm2) * dw; 
        } 
    }
 
    // output raymie = J[T.alpha/PI.deltaE + deltaS] (line 7 in algorithm 4.1) 
} 

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    vec3 raymie; 
    vec4 dhdH;
    float mu, muS, nu, r;  
    vec2 coords = vec2(gl_GlobalInvocationID.xy) + 0.5; 
    
    GetLayer(u_Layer, r, dhdH); 
    GetMuMuSNu(coords, r, dhdH, mu, muS, nu); 
    
    Inscatter(r, mu, muS, nu, raymie); 

    imageStore(i_DeltaJWrite, ivec3(gl_GlobalInvocationID.xy, u_Layer), vec4(raymie, 0.0));
}

// ------------------------------------------------------------------

	)";

static const char* g_irradiance_1_src = R"(

#define NUM_THREADS 8
 
// ---------------------------------------------------------------------------- 
// PHYSICAL MODEL PARAMETERS 
// ---------------------------------------------------------------------------- 
 
uniform float Rg;
uniform float Rt;
uniform float RL;
uniform float HR;
uniform float HM;
uniform float mieG;
uniform float AVERAGE_GROUND_REFLECTANCE;
uniform vec4 betaR;
uniform vec4 betaMSca;
uniform vec4 betaMEx;

// ---------------------------------------------------------------------------- 
// CONSTANT PARAMETERS 
// ---------------------------------------------------------------------------- 

uniform int first;
uniform int TRANSMITTANCE_H;
uniform int TRANSMITTANCE_W;
uniform int SKY_W;
uniform int SKY_H;
uniform int RES_R;
uniform int RES_MU;
uniform int RES_MU_S;
uniform int RES_NU;

// ---------------------------------------------------------------------------- 
// NUMERICAL INTEGRATION PARAMETERS 
// ---------------------------------------------------------------------------- 
 
#define TRANSMITTANCE_INTEGRAL_SAMPLES 500
#define INSCATTER_INTEGRAL_SAMPLES 50
#define IRRADIANCE_INTEGRAL_SAMPLES 32
#define INSCATTER_SPHERICAL_INTEGRAL_SAMPLES 16
 
#define M_PI 3.141592657
 
// ---------------------------------------------------------------------------- 
// PARAMETERIZATION OPTIONS 
// ---------------------------------------------------------------------------- 
 
#define TRANSMITTANCE_NON_LINEAR 
#define INSCATTER_NON_LINEAR 
 
// ---------------------------------------------------------------------------- 
// PARAMETERIZATION FUNCTIONS 
// ---------------------------------------------------------------------------- 

uniform sampler2D s_TransmittanceRead;

vec4 SamplePoint(sampler3D tex, vec3 uv, vec3 size)
{
	uv = clamp(uv, 0.0, 1.0);
	uv = uv * (size-1.0);
	return texelFetch(tex, ivec3(uv + 0.5), 0);
}

float mod(float x, float y) { return x - y * floor(x/y); }
  
vec2 GetTransmittanceUV(float r, float mu) 
{ 
    float uR, uMu; 
#ifdef TRANSMITTANCE_NON_LINEAR 
	uR = sqrt((r - Rg) / (Rt - Rg)); 
	uMu = atan((mu + 0.15) / (1.0 + 0.15) * tan(1.5)) / 1.5; 
#else 
	uR = (r - Rg) / (Rt - Rg); 
	uMu = (mu + 0.15) / (1.0 + 0.15); 
#endif 
    return vec2(uMu, uR); 
} 
 
void GetTransmittanceRMu(vec2 coord, out float r, out float muS) 
{ 
    r = coord.y / float(TRANSMITTANCE_H); 
    muS = coord.x / float(TRANSMITTANCE_W); 
#ifdef TRANSMITTANCE_NON_LINEAR 
    r = Rg + (r * r) * (Rt - Rg); 
    muS = -0.15 + tan(1.5 * muS) / tan(1.5) * (1.0 + 0.15); 
#else 
    r = Rg + r * (Rt - Rg); 
    muS = -0.15 + muS * (1.0 + 0.15); 
#endif 
}
 
vec2 GetIrradianceUV(float r, float muS) 
{ 
    float uR = (r - Rg) / (Rt - Rg); 
    float uMuS = (muS + 0.2) / (1.0 + 0.2); 
    return vec2(uMuS, uR); 
}  

void GetIrradianceRMuS(vec2 coord, out float r, out float muS) 
{ 
    r = Rg + (coord.y - 0.5) / (float(SKY_H) - 1.0) * (Rt - Rg); 
    muS = -0.2 + (coord.x - 0.5) / (float(SKY_W) - 1.0) * (1.0 + 0.2); 
}  

vec4 Texture4D(sampler3D tex, float r, float mu, float muS, float nu) 
{ 
    float H = sqrt(Rt * Rt - Rg * Rg); 
    float rho = sqrt(r * r - Rg * Rg); 
#ifdef INSCATTER_NON_LINEAR 
    float rmu = r * mu; 
    float delta = rmu * rmu - r * r + Rg * Rg; 
    vec4 cst = rmu < 0.0 && delta > 0.0 ? vec4(1.0, 0.0, 0.0, 0.5 - 0.5 / float(RES_MU)) : vec4(-1.0, H * H, H, 0.5 + 0.5 / float(RES_MU)); 
	float uR = 0.5 / float(RES_R) + rho / H * (1.0 - 1.0 / float(RES_R)); 
    float uMu = cst.w + (rmu * cst.x + sqrt(delta + cst.y)) / (rho + cst.z) * (0.5 - 1.0 / float(RES_MU)); 
    // paper formula 
    //float uMuS = 0.5 / float(RES_MU_S) + max((1.0 - exp(-3.0 * muS - 0.6)) / (1.0 - exp(-3.6)), 0.0) * (1.0 - 1.0 / float(RES_MU_S)); 
    // better formula 
    float uMuS = 0.5 / float(RES_MU_S) + (atan(max(muS, -0.1975) * tan(1.26 * 1.1)) / 1.1 + (1.0 - 0.26)) * 0.5 * (1.0 - 1.0 / float(RES_MU_S)); 
#else 
	float uR = 0.5 / float(RES_R) + rho / H * (1.0 - 1.0 / float(RES_R)); 
    float uMu = 0.5 / float(RES_MU) + (mu + 1.0) / 2.0 * (1.0 - 1.0 / float(RES_MU)); 
    float uMuS = 0.5 / float(RES_MU_S) + max(muS + 0.2, 0.0) / 1.2 * (1.0 - 1.0 / float(RES_MU_S)); 
#endif 
    float _lerp = (nu + 1.0) / 2.0 * (float(RES_NU) - 1.0); 
    float uNu = floor(_lerp); 
    _lerp = _lerp - uNu; 
    
    vec3 size = vec3(RES_MU_S*RES_NU,RES_MU,RES_R);
    
    return SamplePoint(tex, vec3((uNu + uMuS) / float(RES_NU), uMu, uR), size) * (1.0 - _lerp) + 
           SamplePoint(tex, vec3((uNu + uMuS + 1.0) / float(RES_NU), uMu, uR), size) * _lerp;

} 

void GetMuMuSNu(vec2 coord, float r, vec4 dhdH, out float mu, out float muS, out float nu) 
{ 
    float x = coord.x - 0.5; 
    float y = coord.y - 0.5; 
#ifdef INSCATTER_NON_LINEAR 
    if (y < float(RES_MU) / 2.0) 
    { 
        float d = 1.0 - y / (float(RES_MU) / 2.0 - 1.0); 
        d = min(max(dhdH.z, d * dhdH.w), dhdH.w * 0.999); 
        mu = (Rg * Rg - r * r - d * d) / (2.0 * r * d); 
        mu = min(mu, -sqrt(1.0 - (Rg / r) * (Rg / r)) - 0.001); 
    } 
    else 
    { 
        float d = (y - float(RES_MU) / 2.0) / (float(RES_MU) / 2.0 - 1.0); 
        d = min(max(dhdH.x, d * dhdH.y), dhdH.y * 0.999); 
        mu = (Rt * Rt - r * r - d * d) / (2.0 * r * d); 
    } 
    muS = mod(x, float(RES_MU_S)) / (float(RES_MU_S) - 1.0); 
    // paper formula 
    //muS = -(0.6 + log(1.0 - muS * (1.0 -  exp(-3.6)))) / 3.0; 
    // better formula 
    muS = tan((2.0 * muS - 1.0 + 0.26) * 1.1) / tan(1.26 * 1.1); 
    nu = -1.0 + floor(x / float(RES_MU_S)) / (float(RES_NU) - 1.0) * 2.0; 
#else 
    mu = -1.0 + 2.0 * y / (float(RES_MU) - 1.0); 
    muS = mod(x, float(RES_MU_S)) / (float(RES_MU_S) - 1.0); 
    muS = -0.2 + muS * 1.2; 
    nu = -1.0 + floor(x / float(RES_MU_S)) / (float(RES_NU) - 1.0) * 2.0; 
#endif 
} 

void GetLayer(int layer, out float r, out vec4 dhdH)
{
	r = float(layer) / (RES_R - 1.0);
	r = r * r;
	r = sqrt(Rg * Rg + r * (Rt * Rt - Rg * Rg)) + (layer == 0 ? 0.01 : (layer == RES_R - 1 ? -0.001 : 0.0));
	
	float dmin = Rt - r;
	float dmax = sqrt(r * r - Rg * Rg) + sqrt(Rt * Rt - Rg * Rg);
	float dminp = r - Rg;
	float dmaxp = sqrt(r * r - Rg * Rg);

	dhdH = vec4(dmin, dmax, dminp, dmaxp);	
}
 
// ---------------------------------------------------------------------------- 
// UTILITY FUNCTIONS 
// ---------------------------------------------------------------------------- 

// nearest intersection of ray r,mu with ground or top atmosphere boundary 
// mu=cos(ray zenith angle at ray origin) 
float Limit(float r, float mu) 
{ 
    float dout = -r * mu + sqrt(r * r * (mu * mu - 1.0) + RL * RL); 
    float delta2 = r * r * (mu * mu - 1.0) + Rg * Rg; 
    
    if (delta2 >= 0.0) 
    { 
        float din = -r * mu - sqrt(delta2); 
        if (din >= 0.0) 
        { 
            dout = min(dout, din); 
        } 
    } 
    
    return dout; 
}

// transmittance(=transparency) of atmosphere for infinite ray (r,mu) 
// (mu=cos(view zenith angle)), intersections with ground ignored 
vec3 Transmittance(float r, float mu) 
{ 
	vec2 uv = GetTransmittanceUV(r, mu);
    return textureLod(s_TransmittanceRead, uv, 0).rgb; 
} 

// transmittance(=transparency) of atmosphere between x and x0 
// assume segment x,x0 not intersecting ground 
// d = distance between x and x0, mu=cos(zenith angle of [x,x0) ray at x) 
vec3 Transmittance(float r, float mu, float d)
{ 
    vec3 result; 
    float r1 = sqrt(r * r + d * d + 2.0 * r * mu * d); 
    float mu1 = (r * mu + d) / r1; 
    if (mu > 0.0) { 
        result = min(Transmittance(r, mu) / Transmittance(r1, mu1), 1.0); 
    } else { 
        result = min(Transmittance(r1, -mu1) / Transmittance(r, -mu), 1.0); 
    } 
    return result; 
} 

vec3 Irradiance(sampler2D tex, float r, float muS) 
{ 
    vec2 uv = GetIrradianceUV(r, muS); 
    return textureLod(tex, uv, 0).rgb;
}  

// Rayleigh phase function 
float PhaseFunctionR(float mu) 
{ 
    return (3.0 / (16.0 * M_PI)) * (1.0 + mu * mu); 
} 

// Mie phase function 
float PhaseFunctionM(float mu) 
{ 
	return 1.5 * 1.0 / (4.0 * M_PI) * (1.0 - mieG*mieG) * pow(max(0.0, 1.0 + (mieG*mieG) - 2.0*mieG*mu), -3.0/2.0) * (1.0 + mu * mu) / (2.0 + mieG*mieG); 
} 

// ------------------------------------------------------------------
// INPUTS -----------------------------------------------------------
// ------------------------------------------------------------------

layout (local_size_x = NUM_THREADS, local_size_y = NUM_THREADS, local_size_z = 1) in;

// ------------------------------------------------------------------
// INPUT ------------------------------------------------------------
// ------------------------------------------------------------------

layout (binding = 0, rgba32f) uniform image2D i_DeltaEWrite;

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    float r, muS; 
    vec2 coords = vec2(gl_GlobalInvocationID.xy) + 0.5; 
    
    GetIrradianceRMuS(coords, r, muS); 
    
    imageStore(i_DeltaEWrite, ivec2(gl_GlobalInvocationID.xy), vec4(Transmittance(r, muS) * max(muS, 0.0), 0.0)); 
}

// ------------------------------------------------------------------

	)";

static const char* g_irradiance_n_src = R"(

#define NUM_THREADS 8
 
// ---------------------------------------------------------------------------- 
// PHYSICAL MODEL PARAMETERS 
// ---------------------------------------------------------------------------- 
 
uniform float Rg;
uniform float Rt;
uniform float RL;
uniform float HR;
uniform float HM;
uniform float mieG;
uniform float AVERAGE_GROUND_REFLECTANCE;
uniform vec4 betaR;
uniform vec4 betaMSca;
uniform vec4 betaMEx;

// ---------------------------------------------------------------------------- 
// CONSTANT PARAMETERS 
// ---------------------------------------------------------------------------- 

uniform int first;
uniform int TRANSMITTANCE_H;
uniform int TRANSMITTANCE_W;
uniform int SKY_W;
uniform int SKY_H;
uniform int RES_R;
uniform int RES_MU;
uniform int RES_MU_S;
uniform int RES_NU;

// ---------------------------------------------------------------------------- 
// NUMERICAL INTEGRATION PARAMETERS 
// ---------------------------------------------------------------------------- 
 
#define TRANSMITTANCE_INTEGRAL_SAMPLES 500
#define INSCATTER_INTEGRAL_SAMPLES 50
#define IRRADIANCE_INTEGRAL_SAMPLES 32
#define INSCATTER_SPHERICAL_INTEGRAL_SAMPLES 16
 
#define M_PI 3.141592657
 
// ---------------------------------------------------------------------------- 
// PARAMETERIZATION OPTIONS 
// ---------------------------------------------------------------------------- 
 
#define TRANSMITTANCE_NON_LINEAR 
#define INSCATTER_NON_LINEAR 
 
// ---------------------------------------------------------------------------- 
// PARAMETERIZATION FUNCTIONS 
// ---------------------------------------------------------------------------- 

uniform sampler2D s_TransmittanceRead;

vec4 SamplePoint(sampler3D tex, vec3 uv, vec3 size)
{
	uv = clamp(uv, 0.0, 1.0);
	uv = uv * (size-1.0);
	return texelFetch(tex, ivec3(uv + 0.5), 0);
}

float mod(float x, float y) { return x - y * floor(x/y); }
  
vec2 GetTransmittanceUV(float r, float mu) 
{ 
    float uR, uMu; 
#ifdef TRANSMITTANCE_NON_LINEAR 
	uR = sqrt((r - Rg) / (Rt - Rg)); 
	uMu = atan((mu + 0.15) / (1.0 + 0.15) * tan(1.5)) / 1.5; 
#else 
	uR = (r - Rg) / (Rt - Rg); 
	uMu = (mu + 0.15) / (1.0 + 0.15); 
#endif 
    return vec2(uMu, uR); 
} 
 
void GetTransmittanceRMu(vec2 coord, out float r, out float muS) 
{ 
    r = coord.y / float(TRANSMITTANCE_H); 
    muS = coord.x / float(TRANSMITTANCE_W); 
#ifdef TRANSMITTANCE_NON_LINEAR 
    r = Rg + (r * r) * (Rt - Rg); 
    muS = -0.15 + tan(1.5 * muS) / tan(1.5) * (1.0 + 0.15); 
#else 
    r = Rg + r * (Rt - Rg); 
    muS = -0.15 + muS * (1.0 + 0.15); 
#endif 
}
 
vec2 GetIrradianceUV(float r, float muS) 
{ 
    float uR = (r - Rg) / (Rt - Rg); 
    float uMuS = (muS + 0.2) / (1.0 + 0.2); 
    return vec2(uMuS, uR); 
}  

void GetIrradianceRMuS(vec2 coord, out float r, out float muS) 
{ 
    r = Rg + (coord.y - 0.5) / (float(SKY_H) - 1.0) * (Rt - Rg); 
    muS = -0.2 + (coord.x - 0.5) / (float(SKY_W) - 1.0) * (1.0 + 0.2); 
}  

vec4 Texture4D(sampler3D tex, float r, float mu, float muS, float nu) 
{ 
    float H = sqrt(Rt * Rt - Rg * Rg); 
    float rho = sqrt(r * r - Rg * Rg); 
#ifdef INSCATTER_NON_LINEAR 
    float rmu = r * mu; 
    float delta = rmu * rmu - r * r + Rg * Rg; 
    vec4 cst = rmu < 0.0 && delta > 0.0 ? vec4(1.0, 0.0, 0.0, 0.5 - 0.5 / float(RES_MU)) : vec4(-1.0, H * H, H, 0.5 + 0.5 / float(RES_MU)); 
	float uR = 0.5 / float(RES_R) + rho / H * (1.0 - 1.0 / float(RES_R)); 
    float uMu = cst.w + (rmu * cst.x + sqrt(delta + cst.y)) / (rho + cst.z) * (0.5 - 1.0 / float(RES_MU)); 
    // paper formula 
    //float uMuS = 0.5 / float(RES_MU_S) + max((1.0 - exp(-3.0 * muS - 0.6)) / (1.0 - exp(-3.6)), 0.0) * (1.0 - 1.0 / float(RES_MU_S)); 
    // better formula 
    float uMuS = 0.5 / float(RES_MU_S) + (atan(max(muS, -0.1975) * tan(1.26 * 1.1)) / 1.1 + (1.0 - 0.26)) * 0.5 * (1.0 - 1.0 / float(RES_MU_S)); 
#else 
	float uR = 0.5 / float(RES_R) + rho / H * (1.0 - 1.0 / float(RES_R)); 
    float uMu = 0.5 / float(RES_MU) + (mu + 1.0) / 2.0 * (1.0 - 1.0 / float(RES_MU)); 
    float uMuS = 0.5 / float(RES_MU_S) + max(muS + 0.2, 0.0) / 1.2 * (1.0 - 1.0 / float(RES_MU_S)); 
#endif 
    float _lerp = (nu + 1.0) / 2.0 * (float(RES_NU) - 1.0); 
    float uNu = floor(_lerp); 
    _lerp = _lerp - uNu; 
    
    vec3 size = vec3(RES_MU_S*RES_NU,RES_MU,RES_R);
    
    return SamplePoint(tex, vec3((uNu + uMuS) / float(RES_NU), uMu, uR), size) * (1.0 - _lerp) + 
           SamplePoint(tex, vec3((uNu + uMuS + 1.0) / float(RES_NU), uMu, uR), size) * _lerp;

} 

void GetMuMuSNu(vec2 coord, float r, vec4 dhdH, out float mu, out float muS, out float nu) 
{ 
    float x = coord.x - 0.5; 
    float y = coord.y - 0.5; 
#ifdef INSCATTER_NON_LINEAR 
    if (y < float(RES_MU) / 2.0) 
    { 
        float d = 1.0 - y / (float(RES_MU) / 2.0 - 1.0); 
        d = min(max(dhdH.z, d * dhdH.w), dhdH.w * 0.999); 
        mu = (Rg * Rg - r * r - d * d) / (2.0 * r * d); 
        mu = min(mu, -sqrt(1.0 - (Rg / r) * (Rg / r)) - 0.001); 
    } 
    else 
    { 
        float d = (y - float(RES_MU) / 2.0) / (float(RES_MU) / 2.0 - 1.0); 
        d = min(max(dhdH.x, d * dhdH.y), dhdH.y * 0.999); 
        mu = (Rt * Rt - r * r - d * d) / (2.0 * r * d); 
    } 
    muS = mod(x, float(RES_MU_S)) / (float(RES_MU_S) - 1.0); 
    // paper formula 
    //muS = -(0.6 + log(1.0 - muS * (1.0 -  exp(-3.6)))) / 3.0; 
    // better formula 
    muS = tan((2.0 * muS - 1.0 + 0.26) * 1.1) / tan(1.26 * 1.1); 
    nu = -1.0 + floor(x / float(RES_MU_S)) / (float(RES_NU) - 1.0) * 2.0; 
#else 
    mu = -1.0 + 2.0 * y / (float(RES_MU) - 1.0); 
    muS = mod(x, float(RES_MU_S)) / (float(RES_MU_S) - 1.0); 
    muS = -0.2 + muS * 1.2; 
    nu = -1.0 + floor(x / float(RES_MU_S)) / (float(RES_NU) - 1.0) * 2.0; 
#endif 
} 

void GetLayer(int layer, out float r, out vec4 dhdH)
{
	r = float(layer) / (RES_R - 1.0);
	r = r * r;
	r = sqrt(Rg * Rg + r * (Rt * Rt - Rg * Rg)) + (layer == 0 ? 0.01 : (layer == RES_R - 1 ? -0.001 : 0.0));
	
	float dmin = Rt - r;
	float dmax = sqrt(r * r - Rg * Rg) + sqrt(Rt * Rt - Rg * Rg);
	float dminp = r - Rg;
	float dmaxp = sqrt(r * r - Rg * Rg);

	dhdH = vec4(dmin, dmax, dminp, dmaxp);	
}
 
// ---------------------------------------------------------------------------- 
// UTILITY FUNCTIONS 
// ---------------------------------------------------------------------------- 

// nearest intersection of ray r,mu with ground or top atmosphere boundary 
// mu=cos(ray zenith angle at ray origin) 
float Limit(float r, float mu) 
{ 
    float dout = -r * mu + sqrt(r * r * (mu * mu - 1.0) + RL * RL); 
    float delta2 = r * r * (mu * mu - 1.0) + Rg * Rg; 
    
    if (delta2 >= 0.0) 
    { 
        float din = -r * mu - sqrt(delta2); 
        if (din >= 0.0) 
        { 
            dout = min(dout, din); 
        } 
    } 
    
    return dout; 
}

// transmittance(=transparency) of atmosphere for infinite ray (r,mu) 
// (mu=cos(view zenith angle)), intersections with ground ignored 
vec3 Transmittance(float r, float mu) 
{ 
	vec2 uv = GetTransmittanceUV(r, mu);
    return textureLod(s_TransmittanceRead, uv, 0).rgb; 
} 

// transmittance(=transparency) of atmosphere between x and x0 
// assume segment x,x0 not intersecting ground 
// d = distance between x and x0, mu=cos(zenith angle of [x,x0) ray at x) 
vec3 Transmittance(float r, float mu, float d)
{ 
    vec3 result; 
    float r1 = sqrt(r * r + d * d + 2.0 * r * mu * d); 
    float mu1 = (r * mu + d) / r1; 
    if (mu > 0.0) { 
        result = min(Transmittance(r, mu) / Transmittance(r1, mu1), 1.0); 
    } else { 
        result = min(Transmittance(r1, -mu1) / Transmittance(r, -mu), 1.0); 
    } 
    return result; 
} 

vec3 Irradiance(sampler2D tex, float r, float muS) 
{ 
    vec2 uv = GetIrradianceUV(r, muS); 
    return textureLod(tex, uv, 0).rgb;
}  

// Rayleigh phase function 
float PhaseFunctionR(float mu) 
{ 
    return (3.0 / (16.0 * M_PI)) * (1.0 + mu * mu); 
} 

// Mie phase function 
float PhaseFunctionM(float mu) 
{ 
	return 1.5 * 1.0 / (4.0 * M_PI) * (1.0 - mieG*mieG) * pow(max(0.0, 1.0 + (mieG*mieG) - 2.0*mieG*mu), -3.0/2.0) * (1.0 + mu * mu) / (2.0 + mieG*mieG); 
} 

// ------------------------------------------------------------------
// INPUTS -----------------------------------------------------------
// ------------------------------------------------------------------

layout (local_size_x = NUM_THREADS, local_size_y = NUM_THREADS, local_size_z = 1) in;

// ------------------------------------------------------------------
// INPUT ------------------------------------------------------------
// ------------------------------------------------------------------

layout (binding = 0, rgba32f) uniform image2D i_DeltaEWrite;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

uniform sampler3D s_DeltaSRRead; 
uniform sampler3D s_DeltaSMRead;

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    float r, muS;
    vec2 coords = vec2(gl_GlobalInvocationID.xy) + 0.5; 
    
    GetIrradianceRMuS(coords, r, muS); 
    
    vec3 s = vec3(sqrt(max(1.0 - muS * muS, 0.0)), 0.0, muS); 
 
    vec3 result = vec3(0,0,0); 
    // integral over 2.PI around x with two nested loops over w directions (theta,phi) -- Eq (15) 
    
    float dphi = M_PI / float(IRRADIANCE_INTEGRAL_SAMPLES); 
	float dtheta = M_PI / float(IRRADIANCE_INTEGRAL_SAMPLES); 

    for (int iphi = 0; iphi < 2 * IRRADIANCE_INTEGRAL_SAMPLES; ++iphi) { 

        float phi = (float(iphi) + 0.5) * dphi; 

        for (int itheta = 0; itheta < IRRADIANCE_INTEGRAL_SAMPLES / 2; ++itheta) { 

            float theta = (float(itheta) + 0.5) * dtheta; 

            float dw = dtheta * dphi * sin(theta); 

            vec3 w = vec3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta)); 

            float nu = dot(s, w); 

            if (first == 1) 
            { 
                // first iteration is special because Rayleigh and Mie were stored separately, 
                // without the phase functions factors; they must be reintroduced here 
                float pr1 = PhaseFunctionR(nu); 
                float pm1 = PhaseFunctionM(nu); 
                vec3 ray1 = Texture4D(s_DeltaSRRead, r, w.z, muS, nu).rgb; 
                vec3 mie1 = Texture4D(s_DeltaSMRead, r, w.z, muS, nu).rgb; 
                result += (ray1 * pr1 + mie1 * pm1) * w.z * dw; 
            } 
            else { 
                result += Texture4D(s_DeltaSRRead, r, w.z, muS, nu).rgb * w.z * dw; 

            } 
        } 
    } 
 
    imageStore(i_DeltaEWrite, ivec2(gl_GlobalInvocationID.xy), vec4(result, 1.0)); 
}

// ------------------------------------------------------------------

	)";

static const char* g_transmittance_src = R"(

#define NUM_THREADS 8
 
// ---------------------------------------------------------------------------- 
// PHYSICAL MODEL PARAMETERS 
// ---------------------------------------------------------------------------- 
 
uniform float Rg;
uniform float Rt;
uniform float RL;
uniform float HR;
uniform float HM;
uniform float mieG;
uniform float AVERAGE_GROUND_REFLECTANCE;
uniform vec4 betaR;
uniform vec4 betaMSca;
uniform vec4 betaMEx;

// ---------------------------------------------------------------------------- 
// CONSTANT PARAMETERS 
// ---------------------------------------------------------------------------- 

uniform int first;
uniform int TRANSMITTANCE_H;
uniform int TRANSMITTANCE_W;
uniform int SKY_W;
uniform int SKY_H;
uniform int RES_R;
uniform int RES_MU;
uniform int RES_MU_S;
uniform int RES_NU;

// ---------------------------------------------------------------------------- 
// NUMERICAL INTEGRATION PARAMETERS 
// ---------------------------------------------------------------------------- 
 
#define TRANSMITTANCE_INTEGRAL_SAMPLES 500
#define INSCATTER_INTEGRAL_SAMPLES 50
#define IRRADIANCE_INTEGRAL_SAMPLES 32
#define INSCATTER_SPHERICAL_INTEGRAL_SAMPLES 16
 
#define M_PI 3.141592657
 
// ---------------------------------------------------------------------------- 
// PARAMETERIZATION OPTIONS 
// ---------------------------------------------------------------------------- 
 
#define TRANSMITTANCE_NON_LINEAR 
#define INSCATTER_NON_LINEAR 
 
// ---------------------------------------------------------------------------- 
// PARAMETERIZATION FUNCTIONS 
// ---------------------------------------------------------------------------- 

uniform sampler2D s_TransmittanceRead;

vec4 SamplePoint(sampler3D tex, vec3 uv, vec3 size)
{
	uv = clamp(uv, 0.0, 1.0);
	uv = uv * (size-1.0);
	return texelFetch(tex, ivec3(uv + 0.5), 0);
}

float mod(float x, float y) { return x - y * floor(x/y); }
  
vec2 GetTransmittanceUV(float r, float mu) 
{ 
    float uR, uMu; 
#ifdef TRANSMITTANCE_NON_LINEAR 
	uR = sqrt((r - Rg) / (Rt - Rg)); 
	uMu = atan((mu + 0.15) / (1.0 + 0.15) * tan(1.5)) / 1.5; 
#else 
	uR = (r - Rg) / (Rt - Rg); 
	uMu = (mu + 0.15) / (1.0 + 0.15); 
#endif 
    return vec2(uMu, uR); 
} 
 
void GetTransmittanceRMu(vec2 coord, out float r, out float muS) 
{ 
    r = coord.y / float(TRANSMITTANCE_H); 
    muS = coord.x / float(TRANSMITTANCE_W); 
#ifdef TRANSMITTANCE_NON_LINEAR 
    r = Rg + (r * r) * (Rt - Rg); 
    muS = -0.15 + tan(1.5 * muS) / tan(1.5) * (1.0 + 0.15); 
#else 
    r = Rg + r * (Rt - Rg); 
    muS = -0.15 + muS * (1.0 + 0.15); 
#endif 
}
 
vec2 GetIrradianceUV(float r, float muS) 
{ 
    float uR = (r - Rg) / (Rt - Rg); 
    float uMuS = (muS + 0.2) / (1.0 + 0.2); 
    return vec2(uMuS, uR); 
}  

void GetIrradianceRMuS(vec2 coord, out float r, out float muS) 
{ 
    r = Rg + (coord.y - 0.5) / (float(SKY_H) - 1.0) * (Rt - Rg); 
    muS = -0.2 + (coord.x - 0.5) / (float(SKY_W) - 1.0) * (1.0 + 0.2); 
}  

vec4 Texture4D(sampler3D tex, float r, float mu, float muS, float nu) 
{ 
    float H = sqrt(Rt * Rt - Rg * Rg); 
    float rho = sqrt(r * r - Rg * Rg); 
#ifdef INSCATTER_NON_LINEAR 
    float rmu = r * mu; 
    float delta = rmu * rmu - r * r + Rg * Rg; 
    vec4 cst = rmu < 0.0 && delta > 0.0 ? vec4(1.0, 0.0, 0.0, 0.5 - 0.5 / float(RES_MU)) : vec4(-1.0, H * H, H, 0.5 + 0.5 / float(RES_MU)); 
	float uR = 0.5 / float(RES_R) + rho / H * (1.0 - 1.0 / float(RES_R)); 
    float uMu = cst.w + (rmu * cst.x + sqrt(delta + cst.y)) / (rho + cst.z) * (0.5 - 1.0 / float(RES_MU)); 
    // paper formula 
    //float uMuS = 0.5 / float(RES_MU_S) + max((1.0 - exp(-3.0 * muS - 0.6)) / (1.0 - exp(-3.6)), 0.0) * (1.0 - 1.0 / float(RES_MU_S)); 
    // better formula 
    float uMuS = 0.5 / float(RES_MU_S) + (atan(max(muS, -0.1975) * tan(1.26 * 1.1)) / 1.1 + (1.0 - 0.26)) * 0.5 * (1.0 - 1.0 / float(RES_MU_S)); 
#else 
	float uR = 0.5 / float(RES_R) + rho / H * (1.0 - 1.0 / float(RES_R)); 
    float uMu = 0.5 / float(RES_MU) + (mu + 1.0) / 2.0 * (1.0 - 1.0 / float(RES_MU)); 
    float uMuS = 0.5 / float(RES_MU_S) + max(muS + 0.2, 0.0) / 1.2 * (1.0 - 1.0 / float(RES_MU_S)); 
#endif 
    float _lerp = (nu + 1.0) / 2.0 * (float(RES_NU) - 1.0); 
    float uNu = floor(_lerp); 
    _lerp = _lerp - uNu; 
    
    vec3 size = vec3(RES_MU_S*RES_NU,RES_MU,RES_R);
    
    return SamplePoint(tex, vec3((uNu + uMuS) / float(RES_NU), uMu, uR), size) * (1.0 - _lerp) + 
           SamplePoint(tex, vec3((uNu + uMuS + 1.0) / float(RES_NU), uMu, uR), size) * _lerp;

} 

void GetMuMuSNu(vec2 coord, float r, vec4 dhdH, out float mu, out float muS, out float nu) 
{ 
    float x = coord.x - 0.5; 
    float y = coord.y - 0.5; 
#ifdef INSCATTER_NON_LINEAR 
    if (y < float(RES_MU) / 2.0) 
    { 
        float d = 1.0 - y / (float(RES_MU) / 2.0 - 1.0); 
        d = min(max(dhdH.z, d * dhdH.w), dhdH.w * 0.999); 
        mu = (Rg * Rg - r * r - d * d) / (2.0 * r * d); 
        mu = min(mu, -sqrt(1.0 - (Rg / r) * (Rg / r)) - 0.001); 
    } 
    else 
    { 
        float d = (y - float(RES_MU) / 2.0) / (float(RES_MU) / 2.0 - 1.0); 
        d = min(max(dhdH.x, d * dhdH.y), dhdH.y * 0.999); 
        mu = (Rt * Rt - r * r - d * d) / (2.0 * r * d); 
    } 
    muS = mod(x, float(RES_MU_S)) / (float(RES_MU_S) - 1.0); 
    // paper formula 
    //muS = -(0.6 + log(1.0 - muS * (1.0 -  exp(-3.6)))) / 3.0; 
    // better formula 
    muS = tan((2.0 * muS - 1.0 + 0.26) * 1.1) / tan(1.26 * 1.1); 
    nu = -1.0 + floor(x / float(RES_MU_S)) / (float(RES_NU) - 1.0) * 2.0; 
#else 
    mu = -1.0 + 2.0 * y / (float(RES_MU) - 1.0); 
    muS = mod(x, float(RES_MU_S)) / (float(RES_MU_S) - 1.0); 
    muS = -0.2 + muS * 1.2; 
    nu = -1.0 + floor(x / float(RES_MU_S)) / (float(RES_NU) - 1.0) * 2.0; 
#endif 
} 

void GetLayer(int layer, out float r, out vec4 dhdH)
{
	r = float(layer) / (RES_R - 1.0);
	r = r * r;
	r = sqrt(Rg * Rg + r * (Rt * Rt - Rg * Rg)) + (layer == 0 ? 0.01 : (layer == RES_R - 1 ? -0.001 : 0.0));
	
	float dmin = Rt - r;
	float dmax = sqrt(r * r - Rg * Rg) + sqrt(Rt * Rt - Rg * Rg);
	float dminp = r - Rg;
	float dmaxp = sqrt(r * r - Rg * Rg);

	dhdH = vec4(dmin, dmax, dminp, dmaxp);	
}
 
// ---------------------------------------------------------------------------- 
// UTILITY FUNCTIONS 
// ---------------------------------------------------------------------------- 

// nearest intersection of ray r,mu with ground or top atmosphere boundary 
// mu=cos(ray zenith angle at ray origin) 
float Limit(float r, float mu) 
{ 
    float dout = -r * mu + sqrt(r * r * (mu * mu - 1.0) + RL * RL); 
    float delta2 = r * r * (mu * mu - 1.0) + Rg * Rg; 
    
    if (delta2 >= 0.0) 
    { 
        float din = -r * mu - sqrt(delta2); 
        if (din >= 0.0) 
        { 
            dout = min(dout, din); 
        } 
    } 
    
    return dout; 
}

// transmittance(=transparency) of atmosphere for infinite ray (r,mu) 
// (mu=cos(view zenith angle)), intersections with ground ignored 
vec3 Transmittance(float r, float mu) 
{ 
	vec2 uv = GetTransmittanceUV(r, mu);
    return textureLod(s_TransmittanceRead, uv, 0).rgb; 
} 

// transmittance(=transparency) of atmosphere between x and x0 
// assume segment x,x0 not intersecting ground 
// d = distance between x and x0, mu=cos(zenith angle of [x,x0) ray at x) 
vec3 Transmittance(float r, float mu, float d)
{ 
    vec3 result; 
    float r1 = sqrt(r * r + d * d + 2.0 * r * mu * d); 
    float mu1 = (r * mu + d) / r1; 
    if (mu > 0.0) { 
        result = min(Transmittance(r, mu) / Transmittance(r1, mu1), 1.0); 
    } else { 
        result = min(Transmittance(r1, -mu1) / Transmittance(r, -mu), 1.0); 
    } 
    return result; 
} 

vec3 Irradiance(sampler2D tex, float r, float muS) 
{ 
    vec2 uv = GetIrradianceUV(r, muS); 
    return textureLod(tex, uv, 0).rgb;
}  

// Rayleigh phase function 
float PhaseFunctionR(float mu) 
{ 
    return (3.0 / (16.0 * M_PI)) * (1.0 + mu * mu); 
} 

// Mie phase function 
float PhaseFunctionM(float mu) 
{ 
	return 1.5 * 1.0 / (4.0 * M_PI) * (1.0 - mieG*mieG) * pow(max(0.0, 1.0 + (mieG*mieG) - 2.0*mieG*mu), -3.0/2.0) * (1.0 + mu * mu) / (2.0 + mieG*mieG); 
} 

// ------------------------------------------------------------------
// INPUTS -----------------------------------------------------------
// ------------------------------------------------------------------

layout (local_size_x = NUM_THREADS, local_size_y = NUM_THREADS, local_size_z = 1) in;

// ------------------------------------------------------------------
// INPUT ------------------------------------------------------------
// ------------------------------------------------------------------

layout (binding = 0, rgba32f) uniform image2D i_TransmittanceWrite;

// ------------------------------------------------------------------
// FUNCTIONS --------------------------------------------------------
// ------------------------------------------------------------------

float OpticalDepth(float H, float r, float mu) 
{ 
    float result = 0.0; 
    float dx = Limit(r, mu) / float(TRANSMITTANCE_INTEGRAL_SAMPLES); 
    float xi = 0.0; 
    float yi = exp(-(r - Rg) / H); 
    
    for (int i = 1; i <= TRANSMITTANCE_INTEGRAL_SAMPLES; ++i) 
    { 
        float xj = float(i) * dx; 
        float yj = exp(-(sqrt(r * r + xj * xj + 2.0 * xj * r * mu) - Rg) / H); 
        result += (yi + yj) / 2.0 * dx; 
        xi = xj; 
        yi = yj; 
    }
     
    return mu < -sqrt(1.0 - (Rg / r) * (Rg / r)) ? 1e9 : result; 
} 

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    float r, muS; 
    GetTransmittanceRMu(vec2(gl_GlobalInvocationID.xy), r, muS); 
    
    vec4 depth = betaR * OpticalDepth(HR, r, muS) + betaMEx * OpticalDepth(HM, r, muS); 

	imageStore(i_TransmittanceWrite, ivec2(gl_GlobalInvocationID.xy), exp(-depth)); // Eq (5);
}

// ------------------------------------------------------------------

	)";

static const char* g_envmap_vs_src = R"(

	layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;

uniform mat4 u_Projection;
uniform mat4 u_View;

out vec3 PS_IN_WorldPos;

void main(void)
{
    PS_IN_WorldPos = position;
    gl_Position    = u_Projection * u_View * vec4(position, 1.0f);
}
	)";

static const char* g_envmap_fs_src = R"(

uniform sampler2D s_Transmittance;
uniform sampler2D s_Irradiance;
uniform sampler3D s_Inscatter;

uniform vec3  EARTH_POS;
uniform vec3  SUN_DIR;
uniform float SUN_INTENSITY;
uniform vec3  betaR;
uniform float mieG;

#define M_PI 3.141592
#define Rg 6360000.0
#define Rt 6420000.0
#define RL 6421000.0
#define RES_R 32.0
#define RES_MU 128.0
#define RES_MU_S 32.0
#define RES_NU 8.0

vec3 hdr(vec3 L)
{
    L   = L * 0.4;
    L.r = L.r < 1.413 ? pow(L.r * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.r);
    L.g = L.g < 1.413 ? pow(L.g * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.g);
    L.b = L.b < 1.413 ? pow(L.b * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.b);
    return L;
}

vec4 Texture4D(sampler3D table, float r, float mu, float muS, float nu)
{
    float H   = sqrt(Rt * Rt - Rg * Rg);
    float rho = sqrt(r * r - Rg * Rg);

    float rmu   = r * mu;
    float delta = rmu * rmu - r * r + Rg * Rg;
    vec4  cst   = rmu < 0.0 && delta > 0.0 ? vec4(1.0, 0.0, 0.0, 0.5 - 0.5 / RES_MU) : vec4(-1.0, H * H, H, 0.5 + 0.5 / RES_MU);
    float uR    = 0.5 / RES_R + rho / H * (1.0 - 1.0 / RES_R);
    float uMu   = cst.w + (rmu * cst.x + sqrt(delta + cst.y)) / (rho + cst.z) * (0.5 - 1.0 / float(RES_MU));
    // paper formula
    //float uMuS = 0.5 / RES_MU_S + max((1.0 - exp(-3.0 * muS - 0.6)) / (1.0 - exp(-3.6)), 0.0) * (1.0 - 1.0 / RES_MU_S);
    // better formula
    float uMuS = 0.5 / RES_MU_S + (atan(max(muS, -0.1975) * tan(1.26 * 1.1)) / 1.1 + (1.0 - 0.26)) * 0.5 * (1.0 - 1.0 / RES_MU_S);

    float lep = (nu + 1.0) / 2.0 * (RES_NU - 1.0);
    float uNu = floor(lep);
    lep       = lep - uNu;

    return texture(table, vec3((uNu + uMuS) / RES_NU, uMu, uR)) * (1.0 - lep) + texture(table, vec3((uNu + uMuS + 1.0) / RES_NU, uMu, uR)) * lep;
}

vec3 GetMie(vec4 rayMie)
{
    // approximated single Mie scattering (cf. approximate Cm in paragraph "Angular precision")
    // rayMie.rgb=C*, rayMie.w=Cm,r
    return rayMie.rgb * rayMie.w / max(rayMie.r, 1e-4) * (betaR.r / betaR);
}

float PhaseFunctionR(float mu)
{
    // Rayleigh phase function
    return (3.0 / (16.0 * M_PI)) * (1.0 + mu * mu);
}

float PhaseFunctionM(float mu)
{
    // Mie phase function
    return 1.5 * 1.0 / (4.0 * M_PI) * (1.0 - mieG * mieG) * pow(1.0 + (mieG * mieG) - 2.0 * mieG * mu, -3.0 / 2.0) * (1.0 + mu * mu) / (2.0 + mieG * mieG);
}

vec3 Transmittance(float r, float mu)
{
    // transmittance(=transparency) of atmosphere for infinite ray (r,mu)
    // (mu=cos(view zenith angle)), intersections with ground ignored
    float uR, uMu;
    uR  = sqrt((r - Rg) / (Rt - Rg));
    uMu = atan((mu + 0.15) / (1.0 + 0.15) * tan(1.5)) / 1.5;

    return texture(s_Transmittance, vec2(uMu, uR)).rgb;
}

vec3 TransmittanceWithShadow(float r, float mu)
{
    // transmittance(=transparency) of atmosphere for infinite ray (r,mu)
    // (mu=cos(view zenith angle)), or zero if ray intersects ground

    return mu < -sqrt(1.0 - (Rg / r) * (Rg / r)) ? vec3(0, 0, 0) : Transmittance(r, mu);
}

vec3 Irradiance(float r, float muS)
{
    float uR   = (r - Rg) / (Rt - Rg);
    float uMuS = (muS + 0.2) / (1.0 + 0.2);

    return texture(s_Irradiance, vec2(uMuS, uR)).rgb;
}

vec3 SunRadiance(vec3 worldPos)
{
    vec3  worldV = normalize(worldPos + EARTH_POS); // vertical vector
    float r      = length(worldPos + EARTH_POS);
    float muS    = dot(worldV, SUN_DIR);

    return TransmittanceWithShadow(r, muS) * SUN_INTENSITY;
}

vec3 SkyIrradiance(float r, float muS)
{
    return Irradiance(r, muS) * SUN_INTENSITY;
}

vec3 SkyIrradiance(vec3 worldPos)
{
    vec3  worldV = normalize(worldPos + EARTH_POS); // vertical vector
    float r      = length(worldPos + EARTH_POS);
    float muS    = dot(worldV, SUN_DIR);

    return Irradiance(r, muS) * SUN_INTENSITY;
}

vec3 SkyRadiance(vec3 camera, vec3 viewdir, out vec3 extinction)
{
    // scattered sunlight between two points
    // camera=observer
    // viewdir=unit vector towards observed point
    // sundir=unit vector towards the sun
    // return scattered light

    camera += EARTH_POS;

    vec3  result = vec3(0, 0, 0);
    float r      = length(camera);
    float rMu    = dot(camera, viewdir);
    float mu     = rMu / r;
    float r0     = r;
    float mu0    = mu;

    float deltaSq = sqrt(rMu * rMu - r * r + Rt * Rt);
    float din     = max(-rMu - deltaSq, 0.0);
    if (din > 0.0)
    {
        camera += din * viewdir;
        rMu += din;
        mu = rMu / Rt;
        r  = Rt;
    }

    float nu  = dot(viewdir, SUN_DIR);
    float muS = dot(camera, SUN_DIR) / r;

    vec4 inScatter = Texture4D(s_Inscatter, r, rMu / r, muS, nu);
    extinction     = Transmittance(r, mu);

    if (r <= Rt)
    {
        vec3  inScatterM = GetMie(inScatter);
        float phase      = PhaseFunctionR(nu);
        float phaseM     = PhaseFunctionM(nu);
        result           = inScatter.rgb * phase + inScatterM * phaseM;
    }
    else
    {
        result     = vec3(0, 0, 0);
        extinction = vec3(1, 1, 1);
    }

    return result * SUN_INTENSITY;
}

vec3 InScattering(vec3 camera, vec3 _point, out vec3 extinction, float shaftWidth)
{
    // single scattered sunlight between two points
    // camera=observer
    // point=point on the ground
    // sundir=unit vector towards the sun
    // return scattered light and extinction coefficient

    vec3 result = vec3(0, 0, 0);
    extinction  = vec3(1, 1, 1);

    vec3  viewdir = _point - camera;
    float d       = length(viewdir);
    viewdir       = viewdir / d;
    float r       = length(camera);

    if (r < 0.9 * Rg)
    {
        camera.y += Rg;
        _point.y += Rg;
        r = length(camera);
    }
    float rMu = dot(camera, viewdir);
    float mu  = rMu / r;
    float r0  = r;
    float mu0 = mu;
    _point -= viewdir * clamp(shaftWidth, 0.0, d);

    float deltaSq = sqrt(rMu * rMu - r * r + Rt * Rt);
    float din     = max(-rMu - deltaSq, 0.0);

    if (din > 0.0 && din < d)
    {
        camera += din * viewdir;
        rMu += din;
        mu = rMu / Rt;
        r  = Rt;
        d -= din;
    }

    if (r <= Rt)
    {
        float nu  = dot(viewdir, SUN_DIR);
        float muS = dot(camera, SUN_DIR) / r;

        vec4 inScatter;

        if (r < Rg + 600.0)
        {
            // avoids imprecision problems in aerial perspective near ground
            float f = (Rg + 600.0) / r;
            r       = r * f;
            rMu     = rMu * f;
            _point  = _point * f;
        }

        float r1   = length(_point);
        float rMu1 = dot(_point, viewdir);
        float mu1  = rMu1 / r1;
        float muS1 = dot(_point, SUN_DIR) / r1;

        if (mu > 0.0)
            extinction = min(Transmittance(r, mu) / Transmittance(r1, mu1), 1.0);
        else
            extinction = min(Transmittance(r1, -mu1) / Transmittance(r, -mu), 1.0);

        const float EPS = 0.004;
        float       lim = -sqrt(1.0 - (Rg / r) * (Rg / r));

        if (abs(mu - lim) < EPS)
        {
            float a = ((mu - lim) + EPS) / (2.0 * EPS);

            mu  = lim - EPS;
            r1  = sqrt(r * r + d * d + 2.0 * r * d * mu);
            mu1 = (r * mu + d) / r1;

            vec4 inScatter0 = Texture4D(s_Inscatter, r, mu, muS, nu);
            vec4 inScatter1 = Texture4D(s_Inscatter, r1, mu1, muS1, nu);
            vec4 inScatterA = max(inScatter0 - inScatter1 * extinction.rgbr, 0.0);

            mu  = lim + EPS;
            r1  = sqrt(r * r + d * d + 2.0 * r * d * mu);
            mu1 = (r * mu + d) / r1;

            inScatter0      = Texture4D(s_Inscatter, r, mu, muS, nu);
            inScatter1      = Texture4D(s_Inscatter, r1, mu1, muS1, nu);
            vec4 inScatterB = max(inScatter0 - inScatter1 * extinction.rgbr, 0.0);

            inScatter = mix(inScatterA, inScatterB, a);
        }
        else
        {
            vec4 inScatter0 = Texture4D(s_Inscatter, r, mu, muS, nu);
            vec4 inScatter1 = Texture4D(s_Inscatter, r1, mu1, muS1, nu);
            inScatter       = max(inScatter0 - inScatter1 * extinction.rgbr, 0.0);
        }

        // avoids imprecision problems in Mie scattering when sun is below horizon
        inScatter.w *= smoothstep(0.00, 0.02, muS);

        vec3  inScatterM = GetMie(inScatter);
        float phase      = PhaseFunctionR(nu);
        float phaseM     = PhaseFunctionM(nu);
        result           = inScatter.rgb * phase + inScatterM * phaseM;
    }

    return result * SUN_INTENSITY;
}

// ------------------------------------------------------------------
// OUPUT ------------------------------------------------------------
// ------------------------------------------------------------------

out vec4 PS_OUT_Color;

// ------------------------------------------------------------------
// INPUT ------------------------------------------------------------
// ------------------------------------------------------------------

in vec3 PS_IN_WorldPos;

// ------------------------------------------------------------------
// UNIFORM ----------------------------------------------------------
// ------------------------------------------------------------------

uniform vec3 u_CameraPos;

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    vec3 dir = normalize(PS_IN_WorldPos);

    float sun = step(cos(M_PI / 360.0), dot(dir, SUN_DIR));

    vec3 sunColor = vec3(sun, sun, sun) * SUN_INTENSITY;

    vec3 extinction;
    vec3 inscatter = SkyRadiance(u_CameraPos, dir, extinction);
    vec3 col       = sunColor * extinction + inscatter;

    PS_OUT_Color = vec4(col, 1.0);
}

// ------------------------------------------------------------------

	)";

static const char* g_skybox_vs_src = R"(

	layout(location = 0) in vec3 VS_IN_Position;

// ------------------------------------------------------------------
// OUTPUT VARIABLES  ------------------------------------------------
// ------------------------------------------------------------------

out vec3 FS_IN_WorldPos;

// ------------------------------------------------------------------
// UNIFORMS  --------------------------------------------------------
// ------------------------------------------------------------------

uniform mat4 u_Projection;
uniform mat4 u_View;

// ------------------------------------------------------------------
// MAIN  ------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    FS_IN_WorldPos = VS_IN_Position;

    mat4 rotView = mat4(mat3(u_View));
    vec4 clipPos = u_Projection * rotView * vec4(VS_IN_Position, 1.0);

    gl_Position = clipPos.xyww;
}

	)";

static const char* g_skybox_fs_src = R"(

	out vec3 PS_OUT_Color;

in vec3 FS_IN_WorldPos;

uniform samplerCube s_Cubemap;

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    vec3 env_color = texture(s_Cubemap, FS_IN_WorldPos).rgb;

    // HDR tonemap and gamma correct
    env_color = env_color / (env_color + vec3(1.0));
    env_color = pow(env_color, vec3(1.0 / 2.2));

    PS_OUT_Color = env_color;
}

// ------------------------------------------------------------------

	)";

// -----------------------------------------------------------------------------------------------------------------------------------

BrunetonSkyModel::BrunetonSkyModel()
{
    m_transmittance_t = nullptr;
    m_irradiance_t[0] = nullptr;
    m_irradiance_t[1] = nullptr;
    m_inscatter_t[0]  = nullptr;
    m_inscatter_t[1]  = nullptr;
    m_delta_et        = nullptr;
    m_delta_srt       = nullptr;
    m_delta_smt       = nullptr;
    m_delta_jt        = nullptr;
}

// -----------------------------------------------------------------------------------------------------------------------------------

BrunetonSkyModel::~BrunetonSkyModel()
{

}

// -----------------------------------------------------------------------------------------------------------------------------------

bool BrunetonSkyModel::initialize()
{
    m_copy_inscatter_1_cs = std::unique_ptr<dw::gl::Shader>(new gl::Shader(GL_COMPUTE_SHADER, g_copy_inscatter_1_src));
    m_copy_inscatter_n_cs = std::unique_ptr<dw::gl::Shader>(new gl::Shader(GL_COMPUTE_SHADER, g_copy_inscatter_n_src));
    m_copy_irradiance_cs  = std::unique_ptr<dw::gl::Shader>(new gl::Shader(GL_COMPUTE_SHADER, g_copy_irradiance_src));
    m_inscatter_1_cs      = std::unique_ptr<dw::gl::Shader>(new gl::Shader(GL_COMPUTE_SHADER, g_inscatter_1_src));
    m_inscatter_n_cs      = std::unique_ptr<dw::gl::Shader>(new gl::Shader(GL_COMPUTE_SHADER, g_inscatter_n_src));
    m_inscatter_s_cs      = std::unique_ptr<dw::gl::Shader>(new gl::Shader(GL_COMPUTE_SHADER, g_inscatter_s_src));
    m_irradiance_1_cs     = std::unique_ptr<dw::gl::Shader>(new gl::Shader(GL_COMPUTE_SHADER, g_irradiance_1_src));
    m_irradiance_n_cs     = std::unique_ptr<dw::gl::Shader>(new gl::Shader(GL_COMPUTE_SHADER, g_irradiance_n_src));
    m_transmittance_cs    = std::unique_ptr<dw::gl::Shader>(new gl::Shader(GL_COMPUTE_SHADER, g_transmittance_src));
    m_sky_envmap_vs       = std::unique_ptr<dw::gl::Shader>(new gl::Shader(GL_VERTEX_SHADER, g_envmap_vs_src));
    m_sky_envmap_fs       = std::unique_ptr<dw::gl::Shader>(new gl::Shader(GL_FRAGMENT_SHADER, g_envmap_fs_src));
    m_cubemap_vs          = std::unique_ptr<dw::gl::Shader>(new gl::Shader(GL_VERTEX_SHADER, g_skybox_vs_src));
    m_cubemap_fs          = std::unique_ptr<dw::gl::Shader>(new gl::Shader(GL_FRAGMENT_SHADER, g_skybox_fs_src));

    {
        if (!m_sky_envmap_vs || !m_sky_envmap_fs)
        {
            DW_LOG_FATAL("Failed to create Shaders");
            return false;
        }

        // Create general shader program
        dw::gl::Shader* shaders[] = { m_sky_envmap_vs.get(), m_sky_envmap_fs.get() };
        m_sky_envmap_program      = std::make_unique<dw::gl::Program>(2, shaders);

        if (!m_sky_envmap_program)
        {
            DW_LOG_FATAL("Failed to create Shader Program");
            return false;
        }
    }

    {
        if (!m_cubemap_vs || !m_cubemap_fs)
        {
            DW_LOG_FATAL("Failed to create Shaders");
            return false;
        }

        // Create general shader program
        dw::gl::Shader* shaders[] = { m_cubemap_vs.get(), m_cubemap_fs.get() };
        m_cubemap_program         = std::make_unique<dw::gl::Program>(2, shaders);

        if (!m_cubemap_program)
        {
            DW_LOG_FATAL("Failed to create Shader Program");
            return false;
        }
    }

    {
        if (!m_copy_inscatter_1_cs)
        {
            DW_LOG_FATAL("Failed to create Shaders");
            return false;
        }

        // Create general shader program
        dw::gl::Shader* shaders[]  = { m_copy_inscatter_1_cs.get() };
        m_copy_inscatter_1_program = std::make_unique<dw::gl::Program>(1, shaders);

        if (!m_copy_inscatter_1_program)
        {
            DW_LOG_FATAL("Failed to create Shader Program");
            return false;
        }
    }

    {
        if (!m_copy_inscatter_n_cs)
        {
            DW_LOG_FATAL("Failed to create Shaders");
            return false;
        }

        // Create general shader program
        dw::gl::Shader* shaders[]  = { m_copy_inscatter_n_cs.get() };
        m_copy_inscatter_n_program = std::make_unique<dw::gl::Program>(1, shaders);

        if (!m_copy_inscatter_n_program)
        {
            DW_LOG_FATAL("Failed to create Shader Program");
            return false;
        }
    }

    {
        if (!m_copy_irradiance_cs)
        {
            DW_LOG_FATAL("Failed to create Shaders");
            return false;
        }

        // Create general shader program
        dw::gl::Shader* shaders[] = { m_copy_irradiance_cs.get() };
        m_copy_irradiance_program = std::make_unique<dw::gl::Program>(1, shaders);

        if (!m_copy_irradiance_program)
        {
            DW_LOG_FATAL("Failed to create Shader Program");
            return false;
        }
    }

    {
        if (!m_inscatter_1_cs)
        {
            DW_LOG_FATAL("Failed to create Shaders");
            return false;
        }

        // Create general shader program
        dw::gl::Shader* shaders[] = { m_inscatter_1_cs.get() };
        m_inscatter_1_program     = std::make_unique<dw::gl::Program>(1, shaders);

        if (!m_inscatter_1_program)
        {
            DW_LOG_FATAL("Failed to create Shader Program");
            return false;
        }
    }

    {
        if (!m_inscatter_n_cs)
        {
            DW_LOG_FATAL("Failed to create Shaders");
            return false;
        }

        // Create general shader program
        dw::gl::Shader* shaders[] = { m_inscatter_n_cs.get() };
        m_inscatter_n_program     = std::make_unique<dw::gl::Program>(1, shaders);

        if (!m_inscatter_n_program)
        {
            DW_LOG_FATAL("Failed to create Shader Program");
            return false;
        }
    }

    {
        if (!m_inscatter_s_cs)
        {
            DW_LOG_FATAL("Failed to create Shaders");
            return false;
        }

        // Create general shader program
        dw::gl::Shader* shaders[] = { m_inscatter_s_cs.get() };
        m_inscatter_s_program     = std::make_unique<dw::gl::Program>(1, shaders);

        if (!m_inscatter_s_program)
        {
            DW_LOG_FATAL("Failed to create Shader Program");
            return false;
        }
    }

    {
        if (!m_irradiance_1_cs)
        {
            DW_LOG_FATAL("Failed to create Shaders");
            return false;
        }

        // Create general shader program
        dw::gl::Shader* shaders[] = { m_irradiance_1_cs.get() };
        m_irradiance_1_program    = std::make_unique<dw::gl::Program>(1, shaders);

        if (!m_irradiance_1_program)
        {
            DW_LOG_FATAL("Failed to create Shader Program");
            return false;
        }
    }

    {
        if (!m_irradiance_n_cs)
        {
            DW_LOG_FATAL("Failed to create Shaders");
            return false;
        }

        // Create general shader program
        dw::gl::Shader* shaders[] = { m_irradiance_n_cs.get() };
        m_irradiance_n_program    = std::make_unique<dw::gl::Program>(1, shaders);

        if (!m_irradiance_n_program)
        {
            DW_LOG_FATAL("Failed to create Shader Program");
            return false;
        }
    }

    {
        if (!m_transmittance_cs)
        {
            DW_LOG_FATAL("Failed to create Shaders");
            return false;
        }

        // Create general shader program
        dw::gl::Shader* shaders[] = { m_transmittance_cs.get() };
        m_transmittance_program   = std::make_unique<dw::gl::Program>(1, shaders);

        if (!m_transmittance_program)
        {
            DW_LOG_FATAL("Failed to create Shader Program");
            return false;
        }
    }

    m_transmittance_t = new_texture_2d(TRANSMITTANCE_W, TRANSMITTANCE_H);

    m_irradiance_t[0] = new_texture_2d(IRRADIANCE_W, IRRADIANCE_H);
    m_irradiance_t[1] = new_texture_2d(IRRADIANCE_W, IRRADIANCE_H);

    m_inscatter_t[0] = new_texture_3d(INSCATTER_MU_S * INSCATTER_NU, INSCATTER_MU, INSCATTER_R);
    m_inscatter_t[1] = new_texture_3d(INSCATTER_MU_S * INSCATTER_NU, INSCATTER_MU, INSCATTER_R);

    m_delta_et  = new_texture_2d(IRRADIANCE_W, IRRADIANCE_H);
    m_delta_srt = new_texture_3d(INSCATTER_MU_S * INSCATTER_NU, INSCATTER_MU, INSCATTER_R);
    m_delta_smt = new_texture_3d(INSCATTER_MU_S * INSCATTER_NU, INSCATTER_MU, INSCATTER_R);
    m_delta_jt  = new_texture_3d(INSCATTER_MU_S * INSCATTER_NU, INSCATTER_MU, INSCATTER_R);

    if (!load_cached_textures())
        precompute();

    float vertices[] = {
        // back face
        -1.0f,
        -1.0f,
        -1.0f,
        0.0f,
        0.0f,
        -1.0f,
        0.0f,
        0.0f, // bottom-left
        1.0f,
        1.0f,
        -1.0f,
        0.0f,
        0.0f,
        -1.0f,
        1.0f,
        1.0f, // top-right
        1.0f,
        -1.0f,
        -1.0f,
        0.0f,
        0.0f,
        -1.0f,
        1.0f,
        0.0f, // bottom-right
        1.0f,
        1.0f,
        -1.0f,
        0.0f,
        0.0f,
        -1.0f,
        1.0f,
        1.0f, // top-right
        -1.0f,
        -1.0f,
        -1.0f,
        0.0f,
        0.0f,
        -1.0f,
        0.0f,
        0.0f, // bottom-left
        -1.0f,
        1.0f,
        -1.0f,
        0.0f,
        0.0f,
        -1.0f,
        0.0f,
        1.0f, // top-left
        // front face
        -1.0f,
        -1.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f, // bottom-left
        1.0f,
        -1.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        1.0f,
        0.0f, // bottom-right
        1.0f,
        1.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        1.0f,
        1.0f, // top-right
        1.0f,
        1.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        1.0f,
        1.0f, // top-right
        -1.0f,
        1.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        1.0f, // top-left
        -1.0f,
        -1.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f, // bottom-left
        // left face
        -1.0f,
        1.0f,
        1.0f,
        -1.0f,
        0.0f,
        0.0f,
        1.0f,
        0.0f, // top-right
        -1.0f,
        1.0f,
        -1.0f,
        -1.0f,
        0.0f,
        0.0f,
        1.0f,
        1.0f, // top-left
        -1.0f,
        -1.0f,
        -1.0f,
        -1.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f, // bottom-left
        -1.0f,
        -1.0f,
        -1.0f,
        -1.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f, // bottom-left
        -1.0f,
        -1.0f,
        1.0f,
        -1.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f, // bottom-right
        -1.0f,
        1.0f,
        1.0f,
        -1.0f,
        0.0f,
        0.0f,
        1.0f,
        0.0f, // top-right
              // right face
        1.0f,
        1.0f,
        1.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        0.0f, // top-left
        1.0f,
        -1.0f,
        -1.0f,
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f, // bottom-right
        1.0f,
        1.0f,
        -1.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        1.0f, // top-right
        1.0f,
        -1.0f,
        -1.0f,
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f, // bottom-right
        1.0f,
        1.0f,
        1.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        0.0f, // top-left
        1.0f,
        -1.0f,
        1.0f,
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f, // bottom-left
        // bottom face
        -1.0f,
        -1.0f,
        -1.0f,
        0.0f,
        -1.0f,
        0.0f,
        0.0f,
        1.0f, // top-right
        1.0f,
        -1.0f,
        -1.0f,
        0.0f,
        -1.0f,
        0.0f,
        1.0f,
        1.0f, // top-left
        1.0f,
        -1.0f,
        1.0f,
        0.0f,
        -1.0f,
        0.0f,
        1.0f,
        0.0f, // bottom-left
        1.0f,
        -1.0f,
        1.0f,
        0.0f,
        -1.0f,
        0.0f,
        1.0f,
        0.0f, // bottom-left
        -1.0f,
        -1.0f,
        1.0f,
        0.0f,
        -1.0f,
        0.0f,
        0.0f,
        0.0f, // bottom-right
        -1.0f,
        -1.0f,
        -1.0f,
        0.0f,
        -1.0f,
        0.0f,
        0.0f,
        1.0f, // top-right
        // top face
        -1.0f,
        1.0f,
        -1.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f, // top-left
        1.0f,
        1.0f,
        1.0f,
        0.0f,
        1.0f,
        0.0f,
        1.0f,
        0.0f, // bottom-right
        1.0f,
        1.0f,
        -1.0f,
        0.0f,
        1.0f,
        0.0f,
        1.0f,
        1.0f, // top-right
        1.0f,
        1.0f,
        1.0f,
        0.0f,
        1.0f,
        0.0f,
        1.0f,
        0.0f, // bottom-right
        -1.0f,
        1.0f,
        -1.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f, // top-left
        -1.0f,
        1.0f,
        1.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        0.0f // bottom-left
    };

    m_cube_vbo = std::make_unique<gl::VertexBuffer>(GL_STATIC_DRAW, sizeof(vertices), vertices);

    if (!m_cube_vbo)
        DW_LOG_ERROR("Failed to create Vertex Buffer");

    // Declare vertex attributes.
    gl::VertexAttrib attribs[] = {
        { 3, GL_FLOAT, false, 0 },
        { 3, GL_FLOAT, false, (3 * sizeof(float)) },
        { 2, GL_FLOAT, false, (6 * sizeof(float)) }
    };

    // Create vertex array.
    m_cube_vao = std::make_unique<gl::VertexArray>(m_cube_vbo.get(), nullptr, (8 * sizeof(float)), 3, attribs);

    m_capture_projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    m_capture_views      = {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
    };

    m_env_cubemap = std::make_unique<dw::gl::TextureCube>(CUBEMAP_SIZE, CUBEMAP_SIZE, 1, 1, GL_RGB16F, GL_RGB, GL_HALF_FLOAT);

    m_env_cubemap->set_wrapping(GL_REPEAT, GL_REPEAT, GL_REPEAT);
    m_env_cubemap->set_min_filter(GL_LINEAR);
    m_env_cubemap->set_mag_filter(GL_LINEAR);

    for (int i = 0; i < 6; i++)
    {
        m_cubemap_fbos.push_back(std::make_unique<gl::Framebuffer>());
        m_cubemap_fbos[i]->attach_render_target(0, m_env_cubemap.get(), i, 0, 0, true, true);
    }

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void BrunetonSkyModel::shutdown()
{
    DW_SAFE_DELETE(m_transmittance_t);
    DW_SAFE_DELETE(m_delta_et);
    DW_SAFE_DELETE(m_delta_srt);
    DW_SAFE_DELETE(m_delta_smt);
    DW_SAFE_DELETE(m_delta_jt);
    DW_SAFE_DELETE(m_irradiance_t[0]);
    DW_SAFE_DELETE(m_irradiance_t[0]);
    DW_SAFE_DELETE(m_inscatter_t[0]);
    DW_SAFE_DELETE(m_inscatter_t[1]);

    m_cubemap_fbos.clear();
    m_env_cubemap.reset();
    m_cube_vbo.reset();
    m_cube_vao.reset();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void BrunetonSkyModel::update_cubemap()
{
    m_sky_envmap_program->use();
    set_render_uniforms(m_sky_envmap_program.get());

    for (int i = 0; i < 6; i++)
    {
        m_sky_envmap_program->set_uniform("u_Projection", m_capture_projection);
        m_sky_envmap_program->set_uniform("u_View", m_capture_views[i]);
        m_sky_envmap_program->set_uniform("u_CameraPos", glm::vec3(0.0f));

        m_cubemap_fbos[i]->bind();
        glViewport(0, 0, CUBEMAP_SIZE, CUBEMAP_SIZE);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_cube_vao->bind();

        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    m_env_cubemap->generate_mipmaps();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void BrunetonSkyModel::render_skybox(uint32_t x, uint32_t y, uint32_t w, uint32_t h, glm::mat4 view, glm::mat4 proj, gl::Framebuffer* fbo)
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);

    m_cubemap_program->use();
    m_cube_vao->bind();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(x, y, w, h);

    m_cubemap_program->set_uniform("u_View", view);
    m_cubemap_program->set_uniform("u_Projection", proj);

    if (m_cubemap_program->set_uniform("s_Cubemap", 0))
        m_env_cubemap->bind(0);

    glDrawArrays(GL_TRIANGLES, 0, 36);

    glDepthFunc(GL_LESS);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void BrunetonSkyModel::set_render_uniforms(gl::Program* program)
{
    program->set_uniform("betaR", m_beta_r / SCALE);
    program->set_uniform("mieG", m_mie_g);
    program->set_uniform("SUN_INTENSITY", m_sun_intensity);
    program->set_uniform("EARTH_POS", glm::vec3(0.0f, 6360010.0f, 0.0f));
    program->set_uniform("SUN_DIR", m_direction * 1.0f);

    if (program->set_uniform("s_Transmittance", 0))
        m_transmittance_t->bind(0);

    if (program->set_uniform("s_Irradiance", 1))
        m_irradiance_t[READ]->bind(1);

    if (program->set_uniform("s_Inscatter", 2))
        m_inscatter_t[READ]->bind(2);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void BrunetonSkyModel::set_uniforms(gl::Program* program)
{
    program->set_uniform("Rg", Rg);
    program->set_uniform("Rt", Rt);
    program->set_uniform("RL", RL);
    program->set_uniform("TRANSMITTANCE_W", TRANSMITTANCE_W);
    program->set_uniform("TRANSMITTANCE_H", TRANSMITTANCE_H);
    program->set_uniform("SKY_W", IRRADIANCE_W);
    program->set_uniform("SKY_H", IRRADIANCE_H);
    program->set_uniform("RES_R", INSCATTER_R);
    program->set_uniform("RES_MU", INSCATTER_MU);
    program->set_uniform("RES_MU_S", INSCATTER_MU_S);
    program->set_uniform("RES_NU", INSCATTER_NU);
    program->set_uniform("AVERAGE_GROUND_REFLECTANCE", AVERAGE_GROUND_REFLECTANCE);
    program->set_uniform("HR", HR);
    program->set_uniform("HM", HM);
    program->set_uniform("betaR", BETA_R);
    program->set_uniform("betaMSca", BETA_MSca);
    program->set_uniform("betaMEx", BETA_MEx);
    program->set_uniform("mieG", glm::clamp(MIE_G, 0.0f, 0.99f));
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool BrunetonSkyModel::load_cached_textures()
{
    FILE* transmittance = fopen("transmittance.raw", "r");

    if (transmittance)
    {
        size_t n = sizeof(float) * TRANSMITTANCE_W * TRANSMITTANCE_H * 4;

        void* data = malloc(n);
        fread(data, n, 1, transmittance);

        m_transmittance_t->set_data(0, 0, data);

        fclose(transmittance);
        free(data);
    }
    else
        return false;

    FILE* irradiance = fopen("irradiance.raw", "r");

    if (irradiance)
    {
        size_t n = sizeof(float) * IRRADIANCE_W * IRRADIANCE_H * 4;

        void* data = malloc(n);
        fread(data, n, 1, irradiance);

        m_irradiance_t[READ]->set_data(0, 0, data);

        fclose(irradiance);
        free(data);
    }
    else
        return false;

    FILE* inscatter = fopen("inscatter.raw", "r");

    if (inscatter)
    {
        size_t n = sizeof(float) * INSCATTER_MU_S * INSCATTER_NU * INSCATTER_MU * INSCATTER_R * 4;

        void* data = malloc(n);
        fread(data, n, 1, inscatter);

        m_inscatter_t[READ]->set_data(0, data);

        fclose(inscatter);
        free(data);
    }
    else
        return false;

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void BrunetonSkyModel::write_textures()
{
    {
        FILE* transmittance = fopen("transmittance.raw", "wb");

        size_t n    = sizeof(float) * TRANSMITTANCE_W * TRANSMITTANCE_H * 4;
        void*  data = malloc(n);

        m_transmittance_t->set_data(0, 0, data);

        fwrite(data, n, 1, transmittance);

        fclose(transmittance);
        free(data);
    }

    {
        FILE* irradiance = fopen("irradiance.raw", "wb");

        size_t n    = sizeof(float) * IRRADIANCE_W * IRRADIANCE_H * 4;
        void*  data = malloc(n);

        m_irradiance_t[READ]->set_data(0, 0, data);

        fwrite(data, n, 1, irradiance);

        fclose(irradiance);
        free(data);
    }

    {
        FILE* inscatter = fopen("inscatter.raw", "wb");

        size_t n    = sizeof(float) * INSCATTER_MU_S * INSCATTER_NU * INSCATTER_MU * INSCATTER_R * 4;
        void*  data = malloc(n);

        m_inscatter_t[READ]->set_data(0, data);

        fwrite(data, n, 1, inscatter);

        fclose(inscatter);
        free(data);
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

void BrunetonSkyModel::precompute()
{
    // -----------------------------------------------------------------------------
    // 1. Compute Transmittance Texture T
    // -----------------------------------------------------------------------------

    m_transmittance_program->use();
    set_uniforms(m_transmittance_program.get());

    m_transmittance_t->bind_image(0, 0, 0, GL_READ_WRITE, m_transmittance_t->internal_format());

    GL_CHECK_ERROR(glDispatchCompute(TRANSMITTANCE_W / NUM_THREADS, TRANSMITTANCE_H / NUM_THREADS, 1));
    GL_CHECK_ERROR(glFinish());

    // -----------------------------------------------------------------------------
    // 2. Compute Irradiance Texture deltaE
    // -----------------------------------------------------------------------------

    m_irradiance_1_program->use();
    set_uniforms(m_irradiance_1_program.get());

    m_delta_et->bind_image(0, 0, 0, GL_READ_WRITE, m_delta_et->internal_format());

    if (m_irradiance_1_program->set_uniform("s_TransmittanceRead", 0))
        m_transmittance_t->bind(0);

    GL_CHECK_ERROR(glDispatchCompute(IRRADIANCE_W / NUM_THREADS, IRRADIANCE_H / NUM_THREADS, 1));
    GL_CHECK_ERROR(glFinish());

    // -----------------------------------------------------------------------------
    // 3. Compute Single Scattering Texture
    // -----------------------------------------------------------------------------

    m_inscatter_1_program->use();
    set_uniforms(m_inscatter_1_program.get());

    m_delta_srt->bind_image(0, 0, 0, GL_READ_WRITE, m_delta_srt->internal_format());
    m_delta_smt->bind_image(1, 0, 0, GL_READ_WRITE, m_delta_smt->internal_format());

    if (m_inscatter_1_program->set_uniform("s_TransmittanceRead", 0))
        m_transmittance_t->bind(0);

    for (int i = 0; i < INSCATTER_R; i++)
    {
        m_inscatter_1_program->set_uniform("u_Layer", i);
        GL_CHECK_ERROR(glDispatchCompute((INSCATTER_MU_S * INSCATTER_NU) / NUM_THREADS, INSCATTER_MU / NUM_THREADS, 1));
        GL_CHECK_ERROR(glFinish());
    }

    // -----------------------------------------------------------------------------
    // 4. Copy deltaE into Irradiance Texture E
    // -----------------------------------------------------------------------------

    m_copy_irradiance_program->use();
    set_uniforms(m_copy_irradiance_program.get());

    m_copy_irradiance_program->set_uniform("u_K", 0.0f);

    m_irradiance_t[WRITE]->bind_image(0, 0, 0, GL_READ_WRITE, m_irradiance_t[WRITE]->internal_format());

    if (m_copy_irradiance_program->set_uniform("s_DeltaERead", 0))
        m_delta_et->bind(0);

    if (m_copy_irradiance_program->set_uniform("s_IrradianceRead", 1))
        m_irradiance_t[READ]->bind(1);

    GL_CHECK_ERROR(glDispatchCompute(IRRADIANCE_W / NUM_THREADS, IRRADIANCE_H / NUM_THREADS, 1));
    GL_CHECK_ERROR(glFinish());

    for (int order = 2; order < 4; order++)
    {
        // -----------------------------------------------------------------------------
        // 5. Copy deltaS into Inscatter Texture S
        // -----------------------------------------------------------------------------

        m_copy_inscatter_1_program->use();
        set_uniforms(m_copy_inscatter_1_program.get());

        m_inscatter_t[WRITE]->bind_image(0, 0, 0, GL_READ_WRITE, m_inscatter_t[WRITE]->internal_format());

        if (m_copy_inscatter_1_program->set_uniform("s_DeltaSRRead", 0))
            m_delta_srt->bind(0);

        if (m_copy_inscatter_1_program->set_uniform("s_DeltaSMRead", 1))
            m_delta_smt->bind(1);

        for (int i = 0; i < INSCATTER_R; i++)
        {
            m_copy_inscatter_1_program->set_uniform("u_Layer", i);
            GL_CHECK_ERROR(glDispatchCompute((INSCATTER_MU_S * INSCATTER_NU) / NUM_THREADS, INSCATTER_MU / NUM_THREADS, 1));
            GL_CHECK_ERROR(glFinish());
        }

        swap(m_inscatter_t);

        // -----------------------------------------------------------------------------
        // 6. Compute deltaJ
        // -----------------------------------------------------------------------------

        m_inscatter_s_program->use();
        set_uniforms(m_inscatter_s_program.get());

        m_inscatter_s_program->set_uniform("first", (order == 2) ? 1 : 0);

        m_delta_jt->bind_image(0, 0, 0, GL_READ_WRITE, m_delta_jt->internal_format());

        if (m_inscatter_s_program->set_uniform("s_TransmittanceRead", 0))
            m_transmittance_t->bind(0);

        if (m_inscatter_s_program->set_uniform("s_DeltaERead", 1))
            m_delta_et->bind(1);

        if (m_inscatter_s_program->set_uniform("s_DeltaSRRead", 2))
            m_delta_srt->bind(2);

        if (m_inscatter_s_program->set_uniform("s_DeltaSMRead", 3))
            m_delta_smt->bind(3);

        for (int i = 0; i < INSCATTER_R; i++)
        {
            m_inscatter_s_program->set_uniform("u_Layer", i);
            GL_CHECK_ERROR(glDispatchCompute((INSCATTER_MU_S * INSCATTER_NU) / NUM_THREADS, INSCATTER_MU / NUM_THREADS, 1));
            GL_CHECK_ERROR(glFinish());
        }

        // -----------------------------------------------------------------------------
        // 7. Compute deltaE
        // -----------------------------------------------------------------------------

        m_irradiance_n_program->use();
        set_uniforms(m_irradiance_n_program.get());

        m_irradiance_n_program->set_uniform("first", (order == 2) ? 1 : 0);

        m_delta_et->bind_image(0, 0, 0, GL_READ_WRITE, m_delta_et->internal_format());

        if (m_irradiance_n_program->set_uniform("s_DeltaSRRead", 0))
            m_delta_srt->bind(0);

        if (m_irradiance_n_program->set_uniform("s_DeltaSMRead", 1))
            m_delta_smt->bind(1);

        GL_CHECK_ERROR(glDispatchCompute(IRRADIANCE_W / NUM_THREADS, IRRADIANCE_H / NUM_THREADS, 1));
        GL_CHECK_ERROR(glFinish());

        // -----------------------------------------------------------------------------
        // 8. Compute deltaS
        // -----------------------------------------------------------------------------

        m_inscatter_n_program->use();
        set_uniforms(m_inscatter_n_program.get());

        m_inscatter_n_program->set_uniform("first", (order == 2) ? 1 : 0);

        m_delta_srt->bind_image(0, 0, 0, GL_READ_WRITE, m_delta_srt->internal_format());

        if (m_inscatter_n_program->set_uniform("s_TransmittanceRead", 0))
            m_transmittance_t->bind(0);

        if (m_inscatter_n_program->set_uniform("s_DeltaJRead", 1))
            m_delta_jt->bind(1);

        for (int i = 0; i < INSCATTER_R; i++)
        {
            m_inscatter_n_program->set_uniform("u_Layer", i);
            GL_CHECK_ERROR(glDispatchCompute((INSCATTER_MU_S * INSCATTER_NU) / NUM_THREADS, INSCATTER_MU / NUM_THREADS, 1));
            GL_CHECK_ERROR(glFinish());
        }

        // -----------------------------------------------------------------------------
        // 9. Adds deltaE into Irradiance Texture E
        // -----------------------------------------------------------------------------

        m_copy_irradiance_program->use();
        set_uniforms(m_copy_irradiance_program.get());

        m_copy_irradiance_program->set_uniform("u_K", 1.0f);

        m_irradiance_t[WRITE]->bind_image(0, 0, 0, GL_READ_WRITE, m_irradiance_t[WRITE]->internal_format());

        if (m_copy_irradiance_program->set_uniform("s_DeltaERead", 0))
            m_delta_et->bind(0);

        if (m_copy_irradiance_program->set_uniform("s_IrradianceRead", 1))
            m_irradiance_t[READ]->bind(1);

        GL_CHECK_ERROR(glDispatchCompute(IRRADIANCE_W / NUM_THREADS, IRRADIANCE_H / NUM_THREADS, 1));
        GL_CHECK_ERROR(glFinish());

        swap(m_irradiance_t);

        // -----------------------------------------------------------------------------
        // 10. Adds deltaS into Inscatter Texture S
        // -----------------------------------------------------------------------------

        m_copy_inscatter_n_program->use();
        set_uniforms(m_copy_inscatter_n_program.get());

        m_inscatter_t[WRITE]->bind_image(0, 0, 0, GL_READ_WRITE, m_inscatter_t[WRITE]->internal_format());

        if (m_copy_inscatter_n_program->set_uniform("s_InscatterRead", 0))
            m_inscatter_t[READ]->bind(0);

        if (m_copy_inscatter_n_program->set_uniform("s_DeltaSRead", 1))
            m_delta_srt->bind(1);

        for (int i = 0; i < INSCATTER_R; i++)
        {
            m_copy_inscatter_n_program->set_uniform("u_Layer", i);
            GL_CHECK_ERROR(glDispatchCompute((INSCATTER_MU_S * INSCATTER_NU) / NUM_THREADS, INSCATTER_MU / NUM_THREADS, 1));
            GL_CHECK_ERROR(glFinish());
        }

        swap(m_inscatter_t);
    }

    // -----------------------------------------------------------------------------
    // 11. Save to disk
    // -----------------------------------------------------------------------------

    write_textures();
}

// -----------------------------------------------------------------------------------------------------------------------------------

gl::Texture2D* BrunetonSkyModel::new_texture_2d(int width, int height)
{
    gl::Texture2D* texture = new gl::Texture2D(width, height, 1, 1, 1, GL_RGBA32F, GL_RGBA, GL_FLOAT);
    texture->set_min_filter(GL_LINEAR);
    texture->set_wrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

    return texture;
}

// -----------------------------------------------------------------------------------------------------------------------------------

gl::Texture3D* BrunetonSkyModel::new_texture_3d(int width, int height, int depth)
{
    gl::Texture3D* texture = new gl::Texture3D(width, height, depth, 1, GL_RGBA32F, GL_RGBA, GL_FLOAT);
    texture->set_min_filter(GL_LINEAR);
    texture->set_wrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

    return texture;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void BrunetonSkyModel::swap(gl::Texture2D** arr)
{
    gl::Texture2D* tmp = arr[READ];
    arr[READ]          = arr[WRITE];
    arr[WRITE]         = tmp;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void BrunetonSkyModel::swap(gl::Texture3D** arr)
{
    gl::Texture3D* tmp = arr[READ];
    arr[READ]          = arr[WRITE];
    arr[WRITE]         = tmp;
}

// -----------------------------------------------------------------------------------------------------------------------------------
} // namespace dw