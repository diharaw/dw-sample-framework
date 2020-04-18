#include <application.h>
#include <camera.h>
#include <material.h>
#include <mesh.h>
#include <ogl.h>
#include <profiler.h>
#include <assimp/scene.h>
#include <gtc/matrix_transform.hpp>

// Embedded vertex shader source.
const char* g_sample_vs_src = R"(

layout (location = 0) in vec4 VS_IN_Position;
layout (location = 1) in vec4 VS_IN_TexCoord;
layout (location = 2) in vec4 VS_IN_Normal;
layout (location = 3) in vec4 VS_IN_Tangent;
layout (location = 4) in vec4 VS_IN_Bitangent;

layout (std140) uniform Transforms //#binding 0
{ 
	mat4 view;
	mat4 projection;
};

uniform mat4 model;

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

    PS_OUT_Color = vec4(color, 1.0);
}

)";

// Uniform buffer data structure.
struct Transforms
{
    DW_ALIGNED(16)
    glm::mat4 view;
    DW_ALIGNED(16)
    glm::mat4 projection;
};

class Sample : public dw::Application
{
protected:
    // Create a transform matrix that performs a relative transform from A to B
    glm::mat4 create_offset_transform(glm::mat4 a, glm::mat4 b)
    {
        // Create delta transform
        glm::vec3 pos_a = glm::vec3(a[3][0], a[3][1], a[3][2]);
        glm::vec3 pos_b = glm::vec3(b[3][0], b[3][1], b[3][2]);

        // Calculate the translation difference
        glm::vec3 translation_diff      = pos_b - pos_a;
        glm::mat4 translation_delta_mat = glm::mat4(1.0f);
        translation_delta_mat           = glm::translate(translation_delta_mat, translation_diff);

        // Calculate the rotation difference
        glm::quat rotation_diff      = glm::quat_cast(b) * glm::conjugate(glm::quat_cast(a));
        glm::mat4 rotation_delta_mat = glm::mat4(1.0f);
        rotation_delta_mat           = glm::mat4_cast(rotation_diff);

        return translation_delta_mat * rotation_delta_mat;
    }

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

        m_transform_1 = glm::mat4(1.0f);
        m_q1          = glm::angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        m_transform_1 = glm::mat4_cast(m_q1);

        m_transform_2 = glm::mat4(1.0f);
        m_transform_2 = glm::translate(m_transform_2, glm::vec3(50.0f, 70.0f, 0.0f));
        m_q2          = glm::angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        m_transform_2 = m_transform_2 * glm::mat4_cast(m_q2) * m_transform_1;

        // Create delta transform
        m_delta_mat = create_offset_transform(m_transform_1, m_transform_2);

        return true;
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void update(double delta) override
    {
        DW_SCOPED_SAMPLE("update");

        // Render profiler.
        dw::profiler::ui();

        // Update camera.
        update_camera();

        // Update uniforms.
        update_uniforms();

        // Bind framebuffer and set viewport.
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, m_width, m_height);

        // Clear default framebuffer.
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render.
        m_transform_1 = glm::mat4(1.0f);
        m_transform_1 = glm::rotate(m_transform_1, (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
        m_transform_2 = m_transform_1 * m_delta_mat;

        render(m_transform_1);
        render(m_transform_2);

        if (m_debug_mode)
        {
            m_debug_draw.transform(m_model_mat, 10.0f);
            m_debug_draw.frustum(m_view_mat, glm::vec3(0.0f, 1.0f, 0.0f));
        }

        // Render debug draw.
        m_debug_draw.render(nullptr, m_width, m_height, m_debug_mode ? m_debug_camera->m_view_projection : m_main_camera->m_view_projection);
    }

    void key_pressed(int code) override
    {
        // Handle forward movement.
        if (code == GLFW_KEY_W)
            m_heading_speed = m_camera_speed;
        else if (code == GLFW_KEY_S)
            m_heading_speed = -m_camera_speed;

        // Handle sideways movement.
        if (code == GLFW_KEY_A)
            m_sideways_speed = -m_camera_speed;
        else if (code == GLFW_KEY_D)
            m_sideways_speed = m_camera_speed;

        if (code == GLFW_KEY_G)
            m_debug_mode = !m_debug_mode;
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void key_released(int code) override
    {
        // Handle forward movement.
        if (code == GLFW_KEY_W || code == GLFW_KEY_S)
            m_heading_speed = 0.0f;

        // Handle sideways movement.
        if (code == GLFW_KEY_A || code == GLFW_KEY_D)
            m_sideways_speed = 0.0f;
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void mouse_pressed(int code) override
    {
        // Enable mouse look.
        if (code == GLFW_MOUSE_BUTTON_LEFT)
            m_mouse_look = true;
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void mouse_released(int code) override
    {
        // Disable mouse look.
        if (code == GLFW_MOUSE_BUTTON_LEFT)
            m_mouse_look = false;
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

        settings.width  = 1920;
        settings.height = 1080;
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
        m_vs = std::make_unique<dw::gl::Shader>(GL_VERTEX_SHADER, g_sample_vs_src);
        m_fs = std::make_unique<dw::gl::Shader>(GL_FRAGMENT_SHADER, g_sample_fs_src);

        if (!m_vs || !m_fs)
        {
            DW_LOG_FATAL("Failed to create Shaders");
            return false;
        }

        // Create shader program
        dw::gl::Shader* ezshaders[] = { m_vs.get(), m_fs.get() };
        m_program                   = std::make_unique<dw::gl::Program>(2, ezshaders);

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
        m_ubo = std::make_unique<dw::gl::UniformBuffer>(GL_DYNAMIC_DRAW,
                                                        sizeof(Transforms),
                                                        nullptr);

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
        m_debug_camera = std::make_unique<dw::Camera>(60.0f, 0.1f, 1000.0f * 2.0f, float(m_width) / float(m_height), glm::vec3(0.0f, 5.0f, 20.0f), glm::vec3(0.0f, 0.0, -1.0f));
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void render(glm::mat4 transform)
    {
        DW_SCOPED_SAMPLE("render");

        // Bind shader program.
        m_program->use();

        m_program->set_uniform("model", transform);

        // Bind uniform buffer.
        m_ubo->bind_base(0);

        // Bind vertex array.
        m_mesh->mesh_vertex_array()->bind();

        // Set active texture unit uniform
        m_program->set_uniform("s_Diffuse", 0);

        for (uint32_t i = 0; i < m_mesh->sub_mesh_count(); i++)
        {
            auto& submesh = m_mesh->sub_meshes()[i];
            auto& mat     = m_mesh->material(submesh.mat_idx);

            // Bind texture.
            if (mat->texture(aiTextureType_DIFFUSE))
                mat->texture(aiTextureType_DIFFUSE)->bind(0);

            // Issue draw call.
            glDrawElementsBaseVertex(
                GL_TRIANGLES, submesh.index_count, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * submesh.base_index), submesh.base_vertex);
        }
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void update_uniforms()
    {
        DW_SCOPED_SAMPLE("update_uniforms");

        dw::Camera* current = m_main_camera.get();

        if (m_debug_mode)
            current = m_debug_camera.get();

        m_transforms.view       = current->m_view;
        m_transforms.projection = current->m_projection;

        void* ptr = m_ubo->map(GL_WRITE_ONLY);
        memcpy(ptr, &m_transforms, sizeof(Transforms));
        m_ubo->unmap();
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void update_camera()
    {
        dw::Camera* current = m_main_camera.get();

        if (m_debug_mode)
        {
            m_pitch += float(m_mouse_delta_y);
            m_pitch = glm::clamp(m_pitch, -90.0f, 90.0f);

            m_yaw += float(m_mouse_delta_x);

            current = m_debug_camera.get();

            m_model_mat = glm::mat4(1.0f);

            m_model_mat = glm::translate(m_model_mat, m_pos);

            /*         m_camera_ori = m_camera_ori * glm::angleAxis(glm::radians(-m_yaw), glm::vec3(0.0f, 1.0f, 0.0f));
            m_camera_ori = m_camera_ori * glm::angleAxis(glm::radians(-m_pitch), glm::vec3(1.0f, 0.0f, 0.0f));
            m_model_mat  = m_model_mat * glm::mat4_cast(m_camera_ori);*/

            glm::quat q = glm::angleAxis(glm::radians(-m_yaw), glm::vec3(0.0f, 1.0f, 0.0f));
            q           = q * glm::angleAxis(glm::radians(-m_pitch), glm::vec3(1.0f, 0.0f, 0.0f));
            m_model_mat = m_model_mat * glm::mat4_cast(q);

            //m_model_mat = glm::rotate(m_model_mat, glm::radians(-m_yaw), glm::vec3(0.0f, 1.0f, 0.0f));
            //m_model_mat = glm::rotate(m_model_mat, glm::radians(-m_pitch), glm::vec3(1.0f, 0.0f, 0.0f));

            m_parent_mat = glm::mat4(1.0f);
            m_parent_mat = glm::translate(m_parent_mat, m_main_camera->m_position);

            if (m_mouse_look)
                m_model_mat = glm::scale(m_model_mat, glm::vec3(0.5f));

            m_model_mat = m_parent_mat * m_model_mat;

            m_view_mat = m_main_camera.get()->m_projection * glm::inverse(m_model_mat);
        }

        float forward_delta = m_heading_speed * m_delta;
        float right_delta   = m_sideways_speed * m_delta;

        current->set_translation_delta(current->m_forward, forward_delta);
        current->set_translation_delta(current->m_right, right_delta);

        if (m_mouse_look)
        {
            // Activate Mouse Look
            current->set_rotatation_delta(glm::vec3((float)(m_mouse_delta_y * m_camera_sensitivity),
                                                    (float)(m_mouse_delta_x * m_camera_sensitivity),
                                                    (float)(0.0f)));
        }
        else
        {
            current->set_rotatation_delta(glm::vec3((float)(0),
                                                    (float)(0),
                                                    (float)(0)));
        }

        current->update();
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

private:
    // GPU resources.
    std::unique_ptr<dw::gl::Shader>        m_vs;
    std::unique_ptr<dw::gl::Shader>        m_fs;
    std::unique_ptr<dw::gl::Program>       m_program;
    std::unique_ptr<dw::gl::UniformBuffer> m_ubo;

    // Camera.
    std::unique_ptr<dw::Camera> m_main_camera;
    std::unique_ptr<dw::Camera> m_debug_camera;

    glm::mat4 m_transform_1;
    glm::mat4 m_transform_2;
    glm::mat4 m_delta_mat;

    glm::quat m_q1;
    glm::quat m_q2;

    // Assets.
    dw::Mesh::Ptr m_mesh;

    // Uniforms.
    Transforms m_transforms;

    // Camera controls.
    bool      m_mouse_look         = false;
    bool      m_debug_mode         = false;
    float     m_heading_speed      = 0.0f;
    float     m_sideways_speed     = 0.0f;
    float     m_camera_sensitivity = 0.05f;
    float     m_camera_speed       = 0.1f;
    glm::vec3 m_pos                = glm::vec3(50.0f, 0.0f, 30.0f);
    glm::mat4 m_model_mat;
    glm::mat4 m_view_mat;
    glm::mat4 m_parent_mat;
    glm::quat m_camera_ori;
    float     m_pitch = 0.0f;
    float     m_yaw   = 0.0f;
};

DW_DECLARE_MAIN(Sample)
