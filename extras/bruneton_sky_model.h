#pragma once

#include <ogl.h>
#include <glm.hpp>
#include <memory>

namespace dw
{
class BrunetonSkyModel
{
private:
    //Dont change these
    const int   NUM_THREADS = 8;
    const int   READ        = 0;
    const int   WRITE       = 1;
    const float SCALE       = 1000.0f;

    //Will save the tables as 8 bit png files so they can be
    //viewed in photoshop. Used for debugging.
    const bool WRITE_DEBUG_TEX = false;

    //You can change these
    //The radius of the planet (Rg), radius of the atmosphere (Rt)
    const float Rg = 6360.0f;
    const float Rt = 6420.0f;
    const float RL = 6421.0f;

    //Dimensions of the tables
    const int TRANSMITTANCE_W = 256;
    const int TRANSMITTANCE_H = 64;

    const int IRRADIANCE_W = 64;
    const int IRRADIANCE_H = 16;

    const int INSCATTER_R    = 32;
    const int INSCATTER_MU   = 128;
    const int INSCATTER_MU_S = 32;
    const int INSCATTER_NU   = 8;

    //Physical settings, Mie and Rayliegh values
    const float     AVERAGE_GROUND_REFLECTANCE = 0.1f;
    const glm::vec4 BETA_R                     = glm::vec4(5.8e-3f, 1.35e-2f, 3.31e-2f, 0.0f);
    const glm::vec4 BETA_MSca                  = glm::vec4(4e-3f, 4e-3f, 4e-3f, 0.0f);
    const glm::vec4 BETA_MEx                   = glm::vec4(4.44e-3f, 4.44e-3f, 4.44e-3f, 0.0f);

    //Asymmetry factor for the mie phase function
    //A higher number meands more light is scattered in the forward direction
    const float MIE_G = 0.8f;

    //Half heights for the atmosphere air density (HR) and particle density (HM)
    //This is the height in km that half the particles are found below
    const float HR = 8.0f;
    const float HM = 1.2f;

    glm::vec3 m_beta_r        = glm::vec3(0.0058f, 0.0135f, 0.0331f);
    float     m_mie_g         = 0.75f;
    float     m_sun_intensity = 100.0f;

    gl::Texture2D* m_transmittance_t;
    gl::Texture2D* m_delta_et;
    gl::Texture3D* m_delta_srt;
    gl::Texture3D* m_delta_smt;
    gl::Texture3D* m_delta_jt;
    gl::Texture2D* m_irradiance_t[2];
    gl::Texture3D* m_inscatter_t[2];

    std::unique_ptr<gl::Shader>  m_copy_inscatter_1_cs;
    std::unique_ptr<gl::Shader>  m_copy_inscatter_n_cs;
    std::unique_ptr<gl::Shader>  m_copy_irradiance_cs;
    std::unique_ptr<gl::Shader>  m_inscatter_1_cs;
    std::unique_ptr<gl::Shader>  m_inscatter_n_cs;
    std::unique_ptr<gl::Shader>  m_inscatter_s_cs;
    std::unique_ptr<gl::Shader>  m_irradiance_1_cs;
    std::unique_ptr<gl::Shader>  m_irradiance_n_cs;
    std::unique_ptr<gl::Shader>  m_transmittance_cs;

    std::unique_ptr<gl::Shader>  m_sky_envmap_vs;
    std::unique_ptr<gl::Shader>  m_sky_envmap_fs;
    std::unique_ptr<gl::Shader>  m_cubemap_vs;
    std::unique_ptr<gl::Shader>  m_cubemap_fs;

    std::unique_ptr<gl::Program> m_copy_inscatter_1_program;
    std::unique_ptr<gl::Program> m_copy_inscatter_n_program;
    std::unique_ptr<gl::Program> m_copy_irradiance_program;
    std::unique_ptr<gl::Program> m_inscatter_1_program;
    std::unique_ptr<gl::Program> m_inscatter_n_program;
    std::unique_ptr<gl::Program> m_inscatter_s_program;
    std::unique_ptr<gl::Program> m_irradiance_1_program;
    std::unique_ptr<gl::Program> m_irradiance_n_program;
    std::unique_ptr<gl::Program> m_transmittance_program;
    std::unique_ptr<gl::Program> m_sky_envmap_program;
    std::unique_ptr<gl::Program> m_cubemap_program;

    float                                         m_sun_angle = 0.0f;
    std::unique_ptr<gl::TextureCube>              m_env_cubemap;
    std::vector<std::unique_ptr<gl::Framebuffer>> m_cubemap_fbos;
    std::unique_ptr<gl::VertexBuffer>             m_cube_vbo;
    std::unique_ptr<gl::VertexArray>              m_cube_vao;
    std::vector<glm::mat4>                        m_capture_views;
    glm::mat4                                     m_capture_projection;

    glm::vec3 m_direction = glm::vec3(0.0f, 0.0f, 1.0f);
    float     m_normalized_sun_y = 1.15f;
    float     m_albedo           = 0.1f;
    float     m_turbidity        = 4.0f;

public:
    BrunetonSkyModel();
    ~BrunetonSkyModel();

    bool initialize();
    void shutdown();

    void update_cubemap();
    void render_skybox(uint32_t x, uint32_t y, uint32_t w, uint32_t h, glm::mat4 view, glm::mat4 proj, gl::Framebuffer* fbo);

    inline glm::vec3 direction() { return m_direction; }
    inline void      set_direction(glm::vec3 dir) { m_direction = -dir; }

    inline float turbidity() { return m_turbidity; }
    inline void  set_turbidity(float t) { m_turbidity = t; }

    inline float sun_angle() { return m_sun_angle; }
    inline void  set_sun_angle(float angle) 
    { 
        m_sun_angle = angle; 
        m_direction = glm::normalize(glm::vec3(0.0f, sin(m_sun_angle), cos(m_sun_angle)));
    }

private:

    void           set_render_uniforms(gl::Program* program);
    void           set_uniforms(gl::Program* program);
    bool           load_cached_textures();
    void           write_textures();
    void           precompute();
    gl::Texture2D* new_texture_2d(int width, int height);
    gl::Texture3D* new_texture_3d(int width, int height, int depth);
    void           swap(gl::Texture2D** arr);
    void           swap(gl::Texture3D** arr);
};
} // namespace dw