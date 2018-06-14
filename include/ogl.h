#include <glad.h>
#include <vector>
#include <unordered_map>

namespace ezGL
{
    class Texture
    {
    public:
        Texture();
        virtual ~Texture();
        void bind();
        void unbind();
        
    private:
        GLuint m_gl_tex;
    };
    
    class Texture1D
    {
    };
    
    class Texture2D
    {
    public:
        Texture2D(uint32_t w, uint32_t h, GLenum format, void* data);
        ~Texture2D();
        void bind();
        void unbind();
        uint32_t width();
        uint32_t height();
    };
    
    class Texture3D
    {
    };
    
    class TextureCube
    {
    };
    
    class Framebuffer
    {
    public:
        Framebuffer(int count, Texture** render_targets, Texture* depth_target);
        ~Framebuffer();
        
    private:
        GLuint m_gl_fbo;
    };
    
    class Shader
    {
        friend class Program;
        
    public:
        Shader(GLenum type, const char* source);
        ~Shader();
        GLenum type();
        
    private:
        GLuint m_gl_shader;
        GLenum m_type;
    };
    
    class Program
    {
    public:
        Program(uint32_t count, Shader* shaders);
        ~Program();
        void bind();
        void unbind();
        
    private:
        GLuint m_gl_program;
    };
    
    class VertexBuffer
    {
        friend class VertexArray;
        
    public:
        VertexBuffer(GLenum type, size_t size, void* data);
        ~VertexBuffer();
        void bind();
        void unbind();
        
    private:
        GLuint m_gl_vbo;
    };
    
    class IndexBuffer
    {
        friend class VertexArray;

    public:
        IndexBuffer(GLenum type, size_t size, void* data);
        ~IndexBuffer();
        void bind();
        void unbind();
        
    private:
        GLuint m_gl_ibo;
    };
    
    class VertexArray
    {
    public:
        VertexArray(VertexBuffer* vbo, IndexBuffer* ibo);
        ~VertexArray();
        
    private:
        GLuint m_gl_vao;
    };
    
    class UniformBuffer
    {
    public:
        UniformBuffer(GLenum type, size_t size, void* data = nullptr);
        ~UniformBuffer();
        void bind(size_t offset = 0, size_t size = 0);
        void unbind();
        void* map(size_t offset = 0, size_t size = 0);
        void unmap();
        void set_data(size_t size, void* data);
        
    private:
        GLuint m_gl_ubo;
    };
    
    class ShaderStorageBuffer
    {
    public:
        ShaderStorageBuffer(GLenum type, size_t size, void* data = nullptr);
        ~ShaderStorageBuffer();
        void bind(size_t offset = 0, size_t size = 0);
        void unbind();
        void* map();
        void unmap();
        void set_data(size_t size, void* data);
        
    private:
        GLuint m_gl_ssbo;
    };
    
    extern bool load_extensions(GLADloadproc func);
    extern void draw();
    extern void draw_instanced();
    extern void draw_elements();
    extern void draw_elements_instanced();
    extern void draw_indirect();
    extern void draw_elements_indirect();
    extern void multi_draw_indirect();
    extern void multi_draw_elements_indirect();
}
