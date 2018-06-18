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

		void generate_mipmaps();
        
    protected:
        GLuint m_gl_tex;
		GLenum m_target;
		GLenum m_internal_format;
		GLenum m_format; 
		GLenum m_type;
    };
 
#if !defined(__EMSCRIPTEN__)
    class Texture1D : public Texture
    {
	public:
		Texture1D(uint32_t w, uint32_t array_size, int32_t mip_levels, GLenum internal_format, GLenum format, GLenum type);
		~Texture1D();
		void set_data(int array_index, int mip_level, void* data);
		uint32_t width();
		uint32_t array_size();
		uint32_t mip_levels();

	private:
		uint32_t m_width;
		uint32_t m_array_size;
		uint32_t m_mip_levels;
		
    };
#endif
    
    class Texture2D : public Texture
    {
    public:
		Texture2D(std::string path, bool srgb = true);
        Texture2D(uint32_t w, uint32_t h, uint32_t array_size, int32_t mip_levels, uint32_t num_samples, GLenum internal_format, GLenum format, GLenum type);
        ~Texture2D();
		void set_data(int array_index, int mip_level, void* data);
        uint32_t width();
        uint32_t height();
		uint32_t array_size();
		uint32_t mip_levels();
		uint32_t num_samples();

	private:
		uint32_t m_width;
		uint32_t m_height;
		uint32_t m_array_size;
		uint32_t m_mip_levels;
		uint32_t m_num_samples;
    };
    
    class Texture3D : public Texture
    {
	public:
		Texture3D(uint32_t w, uint32_t h, uint32_t d, int mip_levels, GLenum internal_format, GLenum format, GLenum type);
		~Texture3D();
		void set_data(int mip_level, void* data);
		uint32_t width();
		uint32_t height();
		uint32_t depth();
		uint32_t mip_levels();

	private:
		uint32_t m_width;
		uint32_t m_height;
		uint32_t m_depth;
		uint32_t m_mip_levels;
    };
    
    class TextureCube : public Texture
    {
	public:
		TextureCube(std::string path[], bool srgb = true);
		TextureCube(uint32_t w, uint32_t h, uint32_t array_size, int32_t mip_levels, GLenum internal_format, GLenum format, GLenum type);
		~TextureCube();
		void set_data(int face_index, int mip_level, void* data);
		uint32_t width();
		uint32_t height();
		uint32_t array_size();
		uint32_t mip_levels();

	private:
		uint32_t m_width;
		uint32_t m_height;
		uint32_t m_array_size;
		uint32_t m_mip_levels;
    };
    
    class Framebuffer
    {
    public:
        Framebuffer(int count, Texture* render_targets[], Texture* depth_target);
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
