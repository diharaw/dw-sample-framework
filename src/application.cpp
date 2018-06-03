#include <application.h>
#include <iostream>
#include <imgui_impl_glfw_gl3.h>
#include <json.hpp>

#include "utility.h"

namespace dw
{
	// -----------------------------------------------------------------------------------------------------------------------------------

    Application::Application() : m_mouse_x(0.0), m_mouse_y(0.0), m_last_mouse_x(0.0), m_last_mouse_y(0.0), m_mouse_delta_x(0.0), m_mouse_delta_y(0.0), m_delta(0.0), m_window(nullptr)
    {
        
    }

	// -----------------------------------------------------------------------------------------------------------------------------------
    
    Application::~Application()
    {
        
    }

	// -----------------------------------------------------------------------------------------------------------------------------------
    
    int Application::run(int argc, const char* argv[])
    {
        if(!init_base(argc, argv))
            return 1;
        
        while(!exit_requested())
            update_base(m_delta);
        
        shutdown_base();
        
        return 0;
    }

	// -----------------------------------------------------------------------------------------------------------------------------------
    
    bool Application::init(int argc, const char* argv[])
    {
        return true;
    }

	// -----------------------------------------------------------------------------------------------------------------------------------
    
    void Application::update(double delta)
    {
        
    }

	// -----------------------------------------------------------------------------------------------------------------------------------
    
    void Application::shutdown()
    {
        
    }

	// -----------------------------------------------------------------------------------------------------------------------------------
    
    bool Application::init_base(int argc, const char* argv[])
    {
        logger::initialize();
        logger::open_console_stream();
        logger::open_file_stream();
        
        std::string config;
        
        // Defaults
        bool resizable = true;
        bool maximized = false;
        int refresh_rate = 60;
        int major_ver = 4;
		m_width = 800;
		m_height = 600;
		m_title = "dwSampleFramwork";
        
#if defined(__APPLE__)
        int minor_ver = 1;
#else
        int minor_ver = 3;
#endif
        
        if (utility::read_text(utility::path_for_resource("config.json"), config))
        {
            DW_LOG_INFO("Loading configuration from json...");
            
            nlohmann::json json = nlohmann::json::parse(config.c_str());
            
            m_width = json["width"];
            m_height = json["height"];
            std::string title = json["title"];
            m_title = title;
            resizable = json["resizable"];
            maximized = json["maximized"];
            refresh_rate = json["refresh_rate"];
        }
        else
        {
            DW_LOG_WARNING("Failed to open config.json. Using default configuration...");
        }
        
        if (glfwInit() != GLFW_TRUE)
        {
            DW_LOG_FATAL("Failed to initialize GLFW");
            return false;
        }
        
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        glfwWindowHint(GLFW_RESIZABLE, resizable);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major_ver);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor_ver);
        glfwWindowHint(GLFW_MAXIMIZED, maximized);
        glfwWindowHint(GLFW_REFRESH_RATE, refresh_rate);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, 8);
        
#if __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        
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
        
		ImGui::CreateContext();
        ImGui_ImplGlfwGL3_Init(m_window, false);
		ImGui::StyleColorsDark();

        int display_w, display_h;
        glfwGetFramebufferSize(m_window, &display_w, &display_h);
        m_width = display_w;
        m_height = display_h;

		if (!m_device.init())
			return false;

		if (!m_debug_draw.init(&m_device))
			return false;
        
        if(!init(argc, argv))
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
    }

	// -----------------------------------------------------------------------------------------------------------------------------------
    
    void Application::end_frame()
    {
        ImGui::Render();
		ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(m_window);
        
        m_timer.stop();
        m_delta = m_timer.elapsed_time_milisec();
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
    
    void Application::window_resized(int width, int height)
    {
        
    }

	// -----------------------------------------------------------------------------------------------------------------------------------
    
    void Application::key_pressed(int code)
    {
        
    }

	// -----------------------------------------------------------------------------------------------------------------------------------
    
    void Application::key_released(int code)
    {
        
    }

	// -----------------------------------------------------------------------------------------------------------------------------------
    
    void Application::mouse_scrolled(double xoffset, double yoffset)
    {
        
    }

	// -----------------------------------------------------------------------------------------------------------------------------------
    
    void Application::mouse_button(int code)
    {
        
    }

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
    
    void Application::mouse_button_callback(GLFWwindow*window, int button, int action, int mods)
    {
        if (button >= 0 && button < MAX_MOUSE_BUTTONS)
        {
            if (action == GLFW_PRESS)
            {
                mouse_button(button);
                m_mouse_buttons[button] = true;
            }
            else if (action == GLFW_RELEASE)
                m_mouse_buttons[button] = false;
        }
    }

	// -----------------------------------------------------------------------------------------------------------------------------------
    
    void Application::window_size_callback(GLFWwindow* window, int width, int height)
    {
        m_width = width;
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
    
    void Application::mouse_button_callback_glfw(GLFWwindow*window, int button, int action, int mods)
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
