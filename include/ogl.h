#include <gl/gl_core_4_5.h>
//#include <glad.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <glm.hpp>

// OpenGL error checking macro.
#ifdef DW_ENABLE_GL_ERROR_CHECK
#define GL_CHECK_ERROR(x)																		  \
x; {                                                                                              \
GLenum err(glGetError());																		  \
																								  \
while (err != GL_NO_ERROR)																		  \
{																								  \
std::string error;																				  \
																								  \
switch (err)																					  \
{																								  \
case GL_INVALID_OPERATION:      error = "INVALID_OPERATION";      break;						  \
case GL_INVALID_ENUM:           error = "INVALID_ENUM";           break;						  \
case GL_INVALID_VALUE:          error = "INVALID_VALUE";          break;						  \
case GL_OUT_OF_MEMORY:          error = "OUT_OF_MEMORY";          break;						  \
case GL_INVALID_FRAMEBUFFER_OPERATION:  error = "INVALID_FRAMEBUFFER_OPERATION";  break;		  \
}																								  \
																								  \
std::string formatted_error = "OPENGL: ";														  \
formatted_error = formatted_error + error;														  \
DW_LOG_ERROR(formatted_error);																	  \
err = glGetError();																				  \
}																								  \
}
#else
#define GL_CHECK_ERROR(x)	x
#endif

namespace ezGL
{
	// Texture base class.
    class Texture
    {
    public:
        Texture();
        virtual ~Texture();

		// Bind texture to specified texture unit i.e GL_TEXTURE<unit>.
        void bind(uint32_t unit);

		// Unbind the whatever texture was bound to the specified texture unit.
        void unbind(uint32_t unit);
        
    private:
        GLuint m_gl_tex;
    };
 
#if !defined(__EMSCRIPTEN__)
    class Texture1D
    {
    };
#endif
    
    class Texture2D
    {
    public:
        Texture2D(uint32_t w, uint32_t h, GLenum format, void* data);
        ~Texture2D();
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
		void bind();
		void unbind();
        
    private:
        GLuint m_gl_fbo;
    };
    
    class Shader
    {
        friend class Program;
        
    public:
        Shader(GLenum type, std::string path);
        ~Shader();
        GLenum type();
        
    private:
        GLuint m_gl_shader;
        GLenum m_type;
    };
    
    class Program
    {
    public:
        Program(uint32_t count, Shader** shaders);
        ~Program();
        void use();
		void uniform_block_binding(std::string name, int binding);
		void set_uniform(std::string name, int value);
		void set_uniform(std::string name, float value);
		void set_uniform(std::string name, glm::vec2 value);
		void set_uniform(std::string name, glm::vec3 value);
		void set_uniform(std::string name, glm::vec4 value);
		void set_uniform(std::string name, glm::mat2 value);
		void set_uniform(std::string name, glm::mat3 value);
		void set_uniform(std::string name, glm::mat4 value);
		void set_uniform(std::string name, int count, int* value);
		void set_uniform(std::string name, int count, float* value);
		void set_uniform(std::string name, int count, glm::vec2* value);
		void set_uniform(std::string name, int count, glm::vec3* value);
		void set_uniform(std::string name, int count, glm::vec4* value);
		void set_uniform(std::string name, int count, glm::mat2* value);
		void set_uniform(std::string name, int count, glm::mat3* value);
		void set_uniform(std::string name, int count, glm::mat4* value);
        
    private:
        GLuint m_gl_program;
		std::unordered_map<std::string, GLuint> m_location_map;
    };
    
    class VertexBuffer
    {
        friend class VertexArray;
        
    public:
		VertexBuffer(GLenum usage, size_t size, void* data);
        ~VertexBuffer();
        void bind();
        void unbind();
        
    private:
        GLuint m_gl_vbo;
    };
    
    class IndexBuffer
    {
        friend class VAO;

    public:
		IndexBuffer(GLenum usage, size_t size, void* data);
        ~IndexBuffer();
        void bind();
        void unbind();
        
    private:
        GLuint m_gl_ibo;
    };

	struct VertexAttrib
	{
		uint32_t num_sub_elements;
		uint32_t type;
		bool	 normalized;
		uint32_t offset;
	};
    
    class VertexArray
    {
    public:
		VertexArray(VertexBuffer* vbo, IndexBuffer* ibo, size_t vertex_size, int attrib_count, VertexAttrib* attribs);
        ~VertexArray();
		void bind();
		void unbind();
        
    private:
        GLuint m_gl_vao;
    };
    
    class UniformBuffer
    {
    public:
		UniformBuffer(GLenum usage, GLenum type, size_t size, void* data = nullptr);
        ~UniformBuffer();
        void bind_base(int index);
		void bind_range(int index, size_t offset, size_t size);
        void unbind();
        void* map(GLenum access);
		void* map_range(GLenum access, size_t offset, size_t size);
        void unmap();
        void set_data(size_t offset, size_t size, void* data);
        
    private:
        GLuint m_gl_ubo;
		size_t m_size;
#if defined(__EMSCRIPTEN__)
		void* m_staging;
		size_t m_mapped_size;
		size_t m_mapped_offset;
#endif
    };
    
#if !defined(__EMSCRIPTEN__)
	class ShaderStorageBuffer
	{
	public:
		ShaderStorageBuffer(GLenum usage, GLenum type, size_t size, void* data = nullptr);
		~ShaderStorageBuffer();
		void bind_base(int index);
		void bind_range(int index, size_t offset, size_t size);
		void unbind();
		void* map(GLenum access);
		void* map_range(GLenum access, size_t offset, size_t size);
		void unmap();
		void set_data(size_t offset, size_t size, void* data);

	private:
		GLuint m_gl_ssbo;
		size_t m_size;
	};
#endif
}
