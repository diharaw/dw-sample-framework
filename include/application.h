#pragma once

#include <render_device.h>

#include <stdint.h>
#include <array>
#include <string>

#include <GLFW/glfw3.h>
#include <imgui.h>

#include "logger.h"
#include "timer.h"

#define DW_DECLARE_MAIN(class_name)  \
int main()                           \
{                                    \
    class_name app;                  \
    return app.run();                \
}

#define MAX_KEYS 1024
#define MAX_MOUSE_BUTTONS 5

namespace dw
{
    class Application
    {
    public:
        Application();
        ~Application();
        int run();
        
        virtual void window_resized(int width, int height);
        virtual void key_pressed(int code);
        virtual void key_released(int code);
        virtual void mouse_scrolled(double xoffset, double yoffset);
        virtual void mouse_button(int code);
        virtual void mouse_move(double x, double y, double deltaX, double deltaY);
        
        void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
        void mouse_callback(GLFWwindow* window, double xpos, double ypos);
        void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
        void mouse_button_callback(GLFWwindow*window, int button, int action, int mods);
        void window_size_callback(GLFWwindow* window, int width, int height);
        
        static void key_callback_glfw(GLFWwindow* window, int key, int scancode, int action, int mode);
        static void mouse_callback_glfw(GLFWwindow* window, double xpos, double ypos);
        static void scroll_callback_glfw(GLFWwindow* window, double xoffset, double yoffset);
        static void mouse_button_callback_glfw(GLFWwindow*window, int button, int action, int mods);
        static void char_callback_glfw(GLFWwindow* window, unsigned int c);
        static void window_size_callback_glfw(GLFWwindow* window, int width, int height);
        
    protected:
        void begin_frame();
        void end_frame();
        void request_exit() const;
        bool exit_requested() const;
        
        virtual bool init();
        virtual void update(double delta);
        virtual void shutdown();
        
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
        std::string                         m_title;
        std::array<bool, MAX_KEYS>          m_keys;
        std::array<bool, MAX_MOUSE_BUTTONS> m_mouse_buttons;
        GLFWwindow*                         m_window;
        Timer                               m_timer;
		RenderDevice						m_device;
        
    private:
        bool init_base();
        void update_base(double delta);
        void shutdown_base();
    };
}

