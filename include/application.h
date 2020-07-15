#pragma once

#include <debug_draw.h>
#include <stdint.h>
#include <array>
#include <string>
#include <vector>
#ifdef __EMSCRIPTEN__
#    define GLFW_INCLUDE_ES3
#    include <GLFW/glfw3.h>
#else
#    include <GLFW/glfw3.h>
#endif
#include <imgui.h>
#include "vk.h"
#include "logger.h"
#include "timer.h"

// Main method macro. Use this at the bottom of any cpp file.
#define DW_DECLARE_MAIN(class_name)        \
    int main(int argc, const char* argv[]) \
    {                                      \
        class_name app;                    \
        return app.run(argc, argv);        \
    }

// Key and Mouse button limits.
#define MAX_KEYS 1024
#define MAX_MOUSE_BUTTONS 5

namespace dw
{
struct AppSettings
{
    bool        resizable    = true;
    bool        maximized    = false;
    int         refresh_rate = 60;
    int         major_ver    = 4;
    int         width        = 800;
    int         height       = 600;
    std::string title        = "dwSampleFramwork";

#if defined(DWSF_VULKAN)
    void*                    device_pnext = nullptr;
    std::vector<const char*> device_extensions;
    bool                     ray_tracing = false;
#else
    bool enable_debug_callback = false;
#endif
};

class Application
{
public:
#if defined(__EMSCRIPTEN__)
    static void run_frame(void* arg);
#endif

    Application();
    ~Application();

    // Run method. Command line arguments passed in.
    int run(int argc, const char* argv[]);

    // Internal callbacks used by GLFW static callback functions.
    void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
    void mouse_callback(GLFWwindow* window, double xpos, double ypos);
    void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    void window_size_callback(GLFWwindow* window, int width, int height);

    // GLFW static callback functions. Called from within GLFW's event loop.
    static void key_callback_glfw(GLFWwindow* window, int key, int scancode, int action, int mode);
    static void mouse_callback_glfw(GLFWwindow* window, double xpos, double ypos);
    static void scroll_callback_glfw(GLFWwindow* window, double xoffset, double yoffset);
    static void mouse_button_callback_glfw(GLFWwindow* window, int button, int action, int mods);
    static void char_callback_glfw(GLFWwindow* window, unsigned int c);
    static void window_size_callback_glfw(GLFWwindow* window, int width, int height);

protected:
    // Intial app settings. Override this to set defaults.
    virtual AppSettings intial_app_settings();

    // Window event callbacks. Override these!
    virtual void window_resized(int width, int height);
    virtual void key_pressed(int code);
    virtual void key_released(int code);
    virtual void mouse_scrolled(double xoffset, double yoffset);
    virtual void mouse_pressed(int code);
    virtual void mouse_released(int code);
    virtual void mouse_move(double x, double y, double deltaX, double deltaY);

    // Application exit related-methods. Self-explanatory.
    void request_exit() const;
    bool exit_requested() const;

    // Life cycle hooks. Override these!
    virtual bool init(int argc, const char* argv[]);
    virtual void update(double delta);
    virtual void shutdown();

#if defined(DWSF_VULKAN)
#    if defined(DWSF_IMGUI)
    void render_gui(vk::CommandBuffer::Ptr cmd_buf);
#    endif
    void submit_and_present(const std::vector<vk::CommandBuffer::Ptr>& cmd_bufs);
#endif

private:
    // Pre, Post frame methods for ImGUI updates, presentations etc.
    void begin_frame();
    void end_frame();

    // Internal lifecycle methods
    bool init_base(int argc, const char* argv[]);
    void update_base(double delta);
    void shutdown_base();

protected:
    uint32_t                            m_width;
    uint32_t                            m_height;
    double                              m_mouse_x;
    double                              m_mouse_y;
    double                              m_last_mouse_x;
    double                              m_last_mouse_y;
    double                              m_mouse_delta_x;
    double                              m_mouse_delta_y;
    double                              m_delta;
    double                              m_delta_seconds;
    std::string                         m_title;
    std::array<bool, MAX_KEYS>          m_keys;
    std::array<bool, MAX_MOUSE_BUTTONS> m_mouse_buttons;
    GLFWwindow*                         m_window;
    Timer                               m_timer;
    DebugDraw                           m_debug_draw;

#if defined(DWSF_VULKAN)
    bool                            m_should_recreate_swap_chain = false;
    vk::Backend::Ptr                m_vk_backend;
    std::vector<vk::Semaphore::Ptr> m_image_available_semaphores;
    std::vector<vk::Semaphore::Ptr> m_render_finished_semaphores;
#endif
};
} // namespace dw
