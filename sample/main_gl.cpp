#include <application.h>
#include <camera.h>
#include <material.h>
#include <mesh.h>
#include <ogl.h>
#include <profiler.h>
#include <assimp/scene.h>

// Embedded vertex shader source.
const char* g_sample_vs_src = R"(
layout (location = 0) in vec4 VS_IN_Position;
layout (location = 1) in vec4 VS_IN_TexCoord;
layout (location = 2) in vec4 VS_IN_Normal;
layout (location = 3) in vec4 VS_IN_Tangent;
layout (location = 4) in vec4 VS_IN_Bitangent;
layout (std140) uniform Transforms //#binding 0
{ 
	mat4 model;
	mat4 view;
	mat4 projection;
};
out vec3 PS_IN_FragPos;
out vec3 PS_IN_Normal;
out vec2 PS_IN_TexCoord;
void main()
{
    vec4 position = model * vec4(VS_IN_Position.xyz, 1.0);
	PS_IN_FragPos = position.xyz;
	PS_IN_Normal = mat3(model) * VS_IN_Normal.xyz;
	PS_IN_TexCoord = VS_IN_TexCoord.xy;
    gl_Position = projection * view * position;
}
)";

// Embedded fragment shader source.
const char* g_sample_fs_src = R"(
precision mediump float;
out vec4 PS_OUT_Color;
in vec3 PS_IN_FragPos;
in vec3 PS_IN_Normal;
in vec2 PS_IN_TexCoord;
uniform sampler2D s_Diffuse; //#slot 0
void main()
{
	vec3 light_pos = vec3(-200.0, 200.0, 0.0);
	vec3 n = normalize(PS_IN_Normal);
	vec3 l = normalize(light_pos - PS_IN_FragPos);
	float lambert = max(0.0f, dot(n, l));
    vec3 diffuse = texture(s_Diffuse, PS_IN_TexCoord).xyz;// + vec3(1.0);
	vec3 ambient = diffuse * 0.03;
	vec3 color = diffuse * lambert + ambient;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0 / 2.2));

    PS_OUT_Color = vec4(color, 1.0);
}
)";

// Uniform buffer data structure.
struct Transforms
{
    DW_ALIGNED(16)
    glm::mat4 model;
    DW_ALIGNED(16)
    glm::mat4 view;
    DW_ALIGNED(16)
    glm::mat4 projection;
};

class Sample : public dw::Application
{
protected:
    // -----------------------------------------------------------------------------------------------------------------------------------

    bool init(int argc, const char* argv[]) override
    {
        // Set initial GPU states.
        set_initial_states();

        // Create GPU resources.
        if (!create_shaders())
            return false;

        if (!create_uniform_buffer())
            return false;

        // Load mesh.
        if (!load_mesh())
            return false;

        // Create camera.
        create_camera();

        return true;
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void update(double delta) override
    {
        DW_SCOPED_SAMPLE("update");

        // Render profiler.
        dw::profiler::ui();

        // Update camera.
        m_main_camera->update();

        // Update uniforms.
        update_uniforms();

        // Render.
        render();
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void shutdown() override
    {
        // Unload assets.
        m_mesh.reset();
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    dw::AppSettings intial_app_settings() override
    {
        // Set custom settings here...
        dw::AppSettings settings;

        settings.width  = 1280;
        settings.height = 720;
        settings.title  = "Hello dwSampleFramework (OpenGL)";

        return settings;
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void window_resized(int width, int height) override
    {
        // Override window resized method to update camera projection.
        m_main_camera->update_projection(60.0f, 0.1f, 1000.0f, float(m_width) / float(m_height));
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

private:
    // -----------------------------------------------------------------------------------------------------------------------------------

    bool create_shaders()
    {
        // Create shaders
        m_vs = dw::gl::Shader::create(GL_VERTEX_SHADER, g_sample_vs_src);
        m_fs = dw::gl::Shader::create(GL_FRAGMENT_SHADER, g_sample_fs_src);

        if (!m_vs || !m_fs)
        {
            DW_LOG_FATAL("Failed to create Shaders");
            return false;
        }

        // Create shader program
        m_program = dw::gl::Program::create({ m_vs, m_fs });

        if (!m_program)
        {
            DW_LOG_FATAL("Failed to create Shader Program");
            return false;
        }

        m_program->uniform_block_binding("Transforms", 0);

        return true;
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void set_initial_states()
    {
        glEnable(GL_DEPTH_TEST);
        glCullFace(GL_BACK);
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    bool create_uniform_buffer()
    {
        // Create uniform buffer for matrix data
        m_ubo = dw::gl::Buffer::create(GL_UNIFORM_BUFFER, GL_MAP_WRITE_BIT, sizeof(Transforms), nullptr);

        return true;
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    bool load_mesh()
    {
        m_mesh = dw::Mesh::load("teapot.obj");
        return m_mesh != nullptr;
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void create_camera()
    {
        m_main_camera = std::make_unique<dw::Camera>(
            60.0f, 0.1f, 1000.0f, float(m_width) / float(m_height), glm::vec3(0.0f, 0.0f, 100.0f), glm::vec3(0.0f, 0.0, -1.0f));
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void render()
    {
        DW_SCOPED_SAMPLE("render");

        // Bind framebuffer and set viewport.
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, m_width, m_height);

        // Clear default framebuffer.
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Bind shader program.
        m_program->use();

        // Bind uniform buffer.
        m_ubo->bind_base(0);

        // Bind vertex array.
        m_mesh->mesh_vertex_array()->bind();

        // Set active texture unit uniform
        m_program->set_uniform("s_Diffuse", 0);

        const auto& submeshes = m_mesh->sub_meshes();

        for (uint32_t i = 0; i < submeshes.size(); i++)
        {
            auto& submesh = submeshes[i];
            auto& mat     = m_mesh->material(submesh.mat_idx);

            // Bind texture.
            if (mat->albedo_texture())
                mat->albedo_texture()->bind(0);

            // Issue draw call.
            glDrawElementsBaseVertex(
                GL_TRIANGLES, submesh.index_count, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * submesh.base_index), submesh.base_vertex);
        }
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void update_uniforms()
    {
        DW_SCOPED_SAMPLE("update_uniforms");

        m_transforms.model      = glm::mat4(1.0f);
        m_transforms.model      = glm::translate(m_transforms.model, glm::vec3(0.0f, -20.0f, 0.0f));
        m_transforms.model      = glm::rotate(m_transforms.model, (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
        m_transforms.model      = glm::scale(m_transforms.model, glm::vec3(0.6f));
        m_transforms.view       = m_main_camera->m_view;
        m_transforms.projection = m_main_camera->m_projection;

        void* ptr = m_ubo->map(GL_WRITE_ONLY);
        memcpy(ptr, &m_transforms, sizeof(Transforms));
        m_ubo->unmap();
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

private:
    // GPU resources.
    dw::gl::Shader::Ptr  m_vs;
    dw::gl::Shader::Ptr  m_fs;
    dw::gl::Program::Ptr m_program;
    dw::gl::Buffer::Ptr  m_ubo;

    // Camera.
    std::unique_ptr<dw::Camera> m_main_camera;

    // Assets.
    dw::Mesh::Ptr m_mesh;

    // Uniforms.
    Transforms m_transforms;
};

DW_DECLARE_MAIN(Sample)