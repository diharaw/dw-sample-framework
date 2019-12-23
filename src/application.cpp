#include <application.h>
#include <imgui_impl_glfw_gl3.h>
#include <profiler.h>
#include <iostream>

#if defined(__EMSCRIPTEN__)
#    include <emscripten/emscripten.h>
#endif

#include "utility.h"

namespace dw
{
#if defined(__EMSCRIPTEN__)
void Application::run_frame(void* arg)
{
    dw::Application* app = (dw::Application*)arg;
    app->update_base(app->m_delta);
}
#endif

// -----------------------------------------------------------------------------------------------------------------------------------

Application::Application() :
    m_mouse_x(0.0), m_mouse_y(0.0), m_last_mouse_x(0.0), m_last_mouse_y(0.0),
    m_mouse_delta_x(0.0), m_mouse_delta_y(0.0), m_delta(0.0),
    m_delta_seconds(0.0), m_window(nullptr) {}

// -----------------------------------------------------------------------------------------------------------------------------------

Application::~Application() {}

// -----------------------------------------------------------------------------------------------------------------------------------

int Application::run(int argc, const char* argv[])
{
    if (!init_base(argc, argv))
        return 1;

#if defined(__EMSCRIPTEN__)
    emscripten_set_main_loop_arg(run_frame, this, 0, 1);
#else
    while (!exit_requested())
        update_base(m_delta);
#endif

    shutdown_base();

    return 0;
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Application::init(int argc, const char* argv[]) { return true; }

// -----------------------------------------------------------------------------------------------------------------------------------

void Application::update(double delta) {}

// -----------------------------------------------------------------------------------------------------------------------------------

void Application::shutdown() {}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Application::init_base(int argc, const char* argv[])
{
    logger::initialize();
    logger::open_console_stream();
    logger::open_file_stream();

    std::string config;

    // Defaults
    AppSettings settings = intial_app_settings();

    bool resizable    = settings.resizable;
    bool maximized    = settings.maximized;
    int  refresh_rate = settings.refresh_rate;
    m_width           = settings.width;
    m_height          = settings.height;
    m_title           = settings.title;

    int major_ver = 4;
#if defined(__APPLE__)
    int minor_ver = 1;
#elif defined(__EMSCRIPTEN__)
    major_ver     = 3;
    int minor_ver = 0;
#else
    int minor_ver = 3;
#endif

    if (glfwInit() != GLFW_TRUE)
    {
        DW_LOG_FATAL("Failed to initialize GLFW");
        return false;
    }

#if defined(DWSF_VULKAN)
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#else
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);

#    if !defined(__EMSCRIPTEN__)
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 8);
#    endif

#    if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#    endif

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major_ver);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor_ver);

#endif
    glfwWindowHint(GLFW_RESIZABLE, resizable);
    glfwWindowHint(GLFW_MAXIMIZED, maximized);
    glfwWindowHint(GLFW_REFRESH_RATE, refresh_rate);

    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);

    if (!m_window)
    {
        DW_LOG_FATAL("Failed to create GLFW window!");
        return false;
    }

    glfwSetKeyCallback(m_window, key_callback_glfw);
    glfwSetCursorPosCallback(m_window, mouse_callback_glfw);
    glfwSetScrollCallback(m_window, scroll_callback_glfw);
    glfwSetMouseButtonCallback(m_window, mouse_button_callback_glfw);
    glfwSetCharCallback(m_window, char_callback_glfw);
    glfwSetWindowSizeCallback(m_window, window_size_callback_glfw);
    glfwSetWindowUserPointer(m_window, this);

    glfwMakeContextCurrent(m_window);

    DW_LOG_INFO("Successfully initialized platform!");

#if !defined(DWSF_VULKAN) && !defined(__EMSCRIPTEN__)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        return false;
#endif

    ImGui::CreateContext();
    ImGui_ImplGlfwGL3_Init(m_window, false);
    ImGui::StyleColorsDark();

    GLFWmonitor* primary = glfwGetPrimaryMonitor();

    float xscale, yscale;
    glfwGetMonitorContentScale(primary, &xscale, &yscale);

    ImGuiStyle* style = &ImGui::GetStyle();

    style->ScaleAllSizes(xscale > yscale ? xscale : yscale);

    ImGuiIO& io        = ImGui::GetIO();
    io.FontGlobalScale = xscale > yscale ? xscale : yscale;

    int display_w, display_h;
    glfwGetFramebufferSize(m_window, &display_w, &display_h);
    m_width  = display_w;
    m_height = display_h;

    if (!m_debug_draw.init())
        return false;

    profiler::initialize();

    if (!init(argc, argv))
        return false;

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Application::update_base(double delta)
{
    begin_frame();
    update(delta);
    end_frame();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Application::shutdown_base()
{
    // Execute user-side shutdown method.
    shutdown();

    // Shutdown profiler.
    profiler::shutdown();

    // Shutdown debug draw.
    m_debug_draw.shutdown();

    // Shutdown ImGui.
    ImGui_ImplGlfwGL3_Shutdown();
    ImGui::DestroyContext();

    // Shutdown GLFW.
    glfwDestroyWindow(m_window);
    glfwTerminate();

    // Close logger streams.
    logger::close_file_stream();
    logger::close_console_stream();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Application::begin_frame()
{
    m_timer.start();

    glfwPollEvents();
    ImGui_ImplGlfwGL3_NewFrame();

    m_mouse_delta_x = m_mouse_x - m_last_mouse_x;
    m_mouse_delta_y = m_mouse_y - m_last_mouse_y;

    m_last_mouse_x = m_mouse_x;
    m_last_mouse_y = m_mouse_y;

    profiler::begin_frame();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Application::end_frame()
{
    profiler::end_frame();

    ImGui::Render();
    ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(m_window);

    m_timer.stop();
    m_delta         = m_timer.elapsed_time_milisec();
    m_delta_seconds = m_timer.elapsed_time_sec();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Application::request_exit() const
{
    glfwSetWindowShouldClose(m_window, true);
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Application::exit_requested() const
{
    return glfwWindowShouldClose(m_window);
}

// -----------------------------------------------------------------------------------------------------------------------------------

AppSettings Application::intial_app_settings() { return AppSettings(); }

// -----------------------------------------------------------------------------------------------------------------------------------

void Application::window_resized(int width, int height) {}

// -----------------------------------------------------------------------------------------------------------------------------------

void Application::key_pressed(int code) {}

// -----------------------------------------------------------------------------------------------------------------------------------

void Application::key_released(int code) {}

// -----------------------------------------------------------------------------------------------------------------------------------

void Application::mouse_scrolled(double xoffset, double yoffset) {}

// -----------------------------------------------------------------------------------------------------------------------------------

void Application::mouse_pressed(int code) {}

// -----------------------------------------------------------------------------------------------------------------------------------

void Application::mouse_released(int code) {}

// -----------------------------------------------------------------------------------------------------------------------------------

void Application::mouse_move(double x, double y, double deltaX, double deltaY)
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Application::key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
    if (key >= 0 && key < MAX_KEYS)
    {
        if (action == GLFW_PRESS)
        {
            key_pressed(key);
            m_keys[key] = true;
        }
        else if (action == GLFW_RELEASE)
        {
            key_released(key);
            m_keys[key] = false;
        }
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Application::mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    m_mouse_x = xpos;
    m_mouse_y = ypos;
    mouse_move(xpos, ypos, m_mouse_delta_x, m_mouse_delta_y);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Application::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    mouse_scrolled(xoffset, yoffset);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Application::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button >= 0 && button < MAX_MOUSE_BUTTONS)
    {
        if (action == GLFW_PRESS)
        {
            mouse_pressed(button);
            m_mouse_buttons[button] = true;
        }
        else if (action == GLFW_RELEASE)
        {
            mouse_released(button);
            m_mouse_buttons[button] = false;
        }
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Application::window_size_callback(GLFWwindow* window, int width, int height)
{
    m_width  = width;
    m_height = height;
    window_resized(width, height);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Application::key_callback_glfw(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mode);
    Application* app = (Application*)glfwGetWindowUserPointer(window);
    app->key_callback(window, key, scancode, action, mode);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Application::mouse_callback_glfw(GLFWwindow* window, double xpos, double ypos)
{
    Application* app = (Application*)glfwGetWindowUserPointer(window);
    app->mouse_callback(window, xpos, ypos);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Application::scroll_callback_glfw(GLFWwindow* window, double xoffset, double yoffset)
{
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
    Application* app = (Application*)glfwGetWindowUserPointer(window);
    app->scroll_callback(window, xoffset, yoffset);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Application::mouse_button_callback_glfw(GLFWwindow* window, int button, int action, int mods)
{
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    Application* app = (Application*)glfwGetWindowUserPointer(window);
    app->mouse_button_callback(window, button, action, mods);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Application::char_callback_glfw(GLFWwindow* window, unsigned int c)
{
    ImGui_ImplGlfw_CharCallback(window, c);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Application::window_size_callback_glfw(GLFWwindow* window, int width, int height)
{
    Application* app = (Application*)glfwGetWindowUserPointer(window);
    app->window_size_callback(window, width, height);
}

// -----------------------------------------------------------------------------------------------------------------------------------
} // namespace dw
