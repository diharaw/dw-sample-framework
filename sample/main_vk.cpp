#include <application.h>
#include <camera.h>
#include <material.h>
#include <mesh.h>
#include <vk.h>
#include <profiler.h>
#include <assimp/scene.h>

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
        dw::Mesh::unload(m_mesh);
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    dw::AppSettings intial_app_settings() override
    {
        // Set custom settings here...
        dw::AppSettings settings;

        settings.width  = 1280;
        settings.height = 720;
        settings.title  = "Hello dwSampleFramework!";

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
        return true;
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    bool create_uniform_buffer()
    {
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
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void update_uniforms()
    {
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

private:
    // GPU resources.

    // Camera.
    std::unique_ptr<dw::Camera> m_main_camera;

    // Assets.
    dw::Mesh* m_mesh;

    // Uniforms.
    Transforms m_transforms;
};

DW_DECLARE_MAIN(Sample)
