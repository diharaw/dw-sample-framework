#include <ogl.h>
#include <utility.h>
#include <logger.h>
#include <gtc/type_ptr.hpp>

namespace ezGL
{
	// -----------------------------------------------------------------------------------------------------------------------------------

	Shader::Shader(GLenum type, std::string path) : m_type(type)
	{
		GL_CHECK_ERROR(m_gl_shader = glCreateShader(type));

		std::string source;

		if (!dw::utility::read_text(path, source))
		{
			DW_LOG_ERROR("Failed to read GLSL shader source: " + path);

			// Force assertion failure for debug builds.
			assert(false);

			return;
		}

#if defined(__APPLE__)
		source = "#version 410 core\n" + std::string(source);
#elif defined(__EMSCRIPTEN__)
		source = "#version 200 es\n" + std::string(source);
#else
		source = "#version 430 core\n" + std::string(source);
#endif

		GLint success;
		GLchar log[512];

		const GLchar* src = source.c_str();

		GL_CHECK_ERROR(glShaderSource(m_gl_shader, 1, &src, NULL));
		GL_CHECK_ERROR(glCompileShader(m_gl_shader));
		GL_CHECK_ERROR(glGetShaderiv(m_gl_shader, GL_COMPILE_STATUS, &success));

		if (success == GL_FALSE)
		{
			glGetShaderInfoLog(m_gl_shader, 512, NULL, log);

			std::string log_error = "OPENGL: Shader compilation failed: ";
			log_error += std::string(log);

			DW_LOG_ERROR(log_error);
		}
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	Shader::~Shader()
	{
		GL_CHECK_ERROR(glDeleteShader(m_gl_shader));
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	GLenum Shader::type()
	{
		return m_type;
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	Program::Program(uint32_t count, Shader** shaders)
	{
#if !defined(__EMSCRIPTEN__)
		if (count == 1 && shaders[0]->type() != GL_COMPUTE_SHADER)
		{
			DW_LOG_ERROR("OPENGL: Compute shader programs can only have one shader.");
			assert(false);

			return;
		}
#endif

		GL_CHECK_ERROR(m_gl_program = glCreateProgram());

		for (int i = 0; i < count; i++)
		{
			GL_CHECK_ERROR(glAttachShader(m_gl_program, shaders[i]->m_gl_shader));
		}

		GL_CHECK_ERROR(glLinkProgram(m_gl_program));

		GLint success;
		char log[512];

		GL_CHECK_ERROR(glGetProgramiv(m_gl_program, GL_LINK_STATUS, &success));

		if (!success)
		{
			glGetProgramInfoLog(m_gl_program, 512, NULL, log);

			std::string log_error = "OPENGL: Shader program linking failed: ";
			log_error += std::string(log);

			DW_LOG_ERROR(log_error);

			return;
		}

		int uniform_count = 0;
		GL_CHECK_ERROR(glGetProgramiv(m_gl_program, GL_ACTIVE_UNIFORMS, &uniform_count));

		GLint size;
		GLenum type;
		GLsizei length; 
		const GLuint buf_size = 64;
		GLchar name[buf_size];

		for (int i = 0; i < uniform_count; i++)
		{
			GL_CHECK_ERROR(glGetActiveUniform(m_gl_program, i, buf_size, &length, &size, &type, name));
			GL_CHECK_ERROR(GLuint loc = glGetUniformLocation(m_gl_program, name));

			if (loc != GL_INVALID_INDEX)
				m_location_map[std::string(name)] = loc;
		}

#if defined(__EMSCRIPTEN__)
		// Bind attributes in OpenGL ES/WebGL versions.

		int attrib_count = 0;
		GL_CHECK_ERROR(glGetProgramiv(m_gl_program, GL_ACTIVE_ATTRIBUTES, &attrib_count));

		for (int i = 0; i < attrib_count; i++)
		{
			GL_CHECK_ERROR(glGetActiveAttrib(m_gl_program, (GLuint)i, buf_size, &length, &size, &type, name));
			GL_CHECK_ERROR(glBindAttribLocation(m_gl_program, i, name));
		}
#endif
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	Program::~Program()
	{
		glDeleteProgram(m_gl_program);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void Program::use()
	{
		glUseProgram(m_gl_program);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void Program::uniform_block_binding(std::string name, int binding)
	{
		GL_CHECK_ERROR(GLuint idx = glGetUniformBlockIndex(m_gl_program, name.c_str()));

		if (idx == GL_INVALID_INDEX)
		{
			std::string uniform_error = "OPENGL: Failed to get Uniform Block Index for Uniform Buffer : ";
			uniform_error += name;

			DW_LOG_ERROR(uniform_error);
		}
		else
			GL_CHECK_ERROR(glUniformBlockBinding(m_gl_program, idx, binding));
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void Program::set_uniform(std::string name, int value)
	{
		assert(m_location_map.find(name) != m_location_map.end());
		glUniform1i(m_location_map[name], value);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void Program::set_uniform(std::string name, float value)
	{
		assert(m_location_map.find(name) != m_location_map.end());
		glUniform1f(m_location_map[name], value);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void Program::set_uniform(std::string name, glm::vec2 value)
	{
		assert(m_location_map.find(name) != m_location_map.end());
		glUniform2f(m_location_map[name], value.x, value.y);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void Program::set_uniform(std::string name, glm::vec3 value)
	{
		assert(m_location_map.find(name) != m_location_map.end());
		glUniform3f(m_location_map[name], value.x, value.y, value.z);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void Program::set_uniform(std::string name, glm::vec4 value)
	{
		assert(m_location_map.find(name) != m_location_map.end());
		glUniform4f(m_location_map[name], value.x, value.y, value.z, value.w);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void Program::set_uniform(std::string name, glm::mat2 value)
	{
		assert(m_location_map.find(name) != m_location_map.end());
		glUniformMatrix2fv(m_location_map[name], 1, GL_FALSE, glm::value_ptr(value));
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void Program::set_uniform(std::string name, glm::mat3 value)
	{
		assert(m_location_map.find(name) != m_location_map.end());
		glUniformMatrix3fv(m_location_map[name], 1, GL_FALSE, glm::value_ptr(value));
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void Program::set_uniform(std::string name, glm::mat4 value)
	{
		assert(m_location_map.find(name) != m_location_map.end());
		glUniformMatrix4fv(m_location_map[name], 1, GL_FALSE, glm::value_ptr(value));
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void Program::set_uniform(std::string name, int count, int* value)
	{
		assert(m_location_map.find(name) != m_location_map.end());
		glUniform1iv(m_location_map[name], count, value);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void Program::set_uniform(std::string name, int count, float* value)
	{
		assert(m_location_map.find(name) != m_location_map.end());
		glUniform1fv(m_location_map[name], count, value);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void Program::set_uniform(std::string name, int count, glm::vec2* value)
	{
		assert(m_location_map.find(name) != m_location_map.end());
		glUniform2fv(m_location_map[name], count, glm::value_ptr(value[0]));
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void Program::set_uniform(std::string name, int count, glm::vec3* value)
	{
		assert(m_location_map.find(name) != m_location_map.end());
		glUniform3fv(m_location_map[name], count, glm::value_ptr(value[0]));
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void Program::set_uniform(std::string name, int count, glm::vec4* value)
	{
		assert(m_location_map.find(name) != m_location_map.end());
		glUniform4fv(m_location_map[name], count, glm::value_ptr(value[0]));
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void Program::set_uniform(std::string name, int count, glm::mat2* value)
	{
		assert(m_location_map.find(name) != m_location_map.end());
		glUniformMatrix2fv(m_location_map[name], count, GL_FALSE, glm::value_ptr(value[0]));
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void Program::set_uniform(std::string name, int count, glm::mat3* value)
	{
		assert(m_location_map.find(name) != m_location_map.end());
		glUniformMatrix3fv(m_location_map[name], count, GL_FALSE, glm::value_ptr(value[0]));
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void Program::set_uniform(std::string name, int count, glm::mat4* value)
	{
		assert(m_location_map.find(name) != m_location_map.end());
		glUniformMatrix4fv(m_location_map[name], count, GL_FALSE, glm::value_ptr(value[0]));
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	VertexBuffer::VertexBuffer(GLenum usage, size_t size, void* data)
	{
		GL_CHECK_ERROR(glGenBuffers(1, &m_gl_vbo));

		GL_CHECK_ERROR(glBindBuffer(GL_ARRAY_BUFFER, m_gl_vbo));
		GL_CHECK_ERROR(glBufferData(GL_ARRAY_BUFFER, size, data, usage));
		GL_CHECK_ERROR(glBindBuffer(GL_ARRAY_BUFFER, 0));
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	VertexBuffer::~VertexBuffer()
	{
		glDeleteBuffers(1, &m_gl_vbo);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void VertexBuffer::bind()
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_gl_vbo);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void VertexBuffer::unbind()
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	IndexBuffer::IndexBuffer(GLenum usage, size_t size, void* data)
	{
		GL_CHECK_ERROR(glGenBuffers(1, &m_gl_ibo));

		GL_CHECK_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_gl_ibo));
		GL_CHECK_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, usage));
		GL_CHECK_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	IndexBuffer::~IndexBuffer()
	{
		glDeleteBuffers(1, &m_gl_ibo);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void IndexBuffer::bind()
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_gl_ibo);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void IndexBuffer::unbind()
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	VertexArray::VertexArray(VertexBuffer* vbo, IndexBuffer* ibo, size_t vertex_size, int attrib_count, VertexAttrib* attribs)
	{
#if defined(__EMSCRIPTEN__)
		GL_CHECK_ERROR(glGenVertexArraysOES(1, &m_gl_vao));
		GL_CHECK_ERROR(glBindVertexArrayOES(m_gl_vao));
#else
		GL_CHECK_ERROR(glGenVertexArrays(1, &m_gl_vao));
		GL_CHECK_ERROR(glBindVertexArray(m_gl_vao));
#endif

		vbo->bind();

		if (ibo)
			ibo->bind();

		for (uint32_t i = 0; i < attrib_count; i++)
		{
			GL_CHECK_ERROR(glEnableVertexAttribArray(i));
			GL_CHECK_ERROR(glVertexAttribPointer(i,
												 attribs[i].num_sub_elements,
												 attribs[i].type,
												 attribs[i].normalized,
												 vertex_size,
												 (GLvoid*)((uint64_t)attribs[i].offset)));
		}

#if defined(__EMSCRIPTEN__)
		GL_CHECK_ERROR(glBindVertexArrayOES(0));
#else
		GL_CHECK_ERROR(glBindVertexArray(0));
#endif

		vbo->unbind();
		
		if (ibo)
			ibo->unbind();
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	VertexArray::~VertexArray()
	{
#if defined(__EMSCRIPTEN__)
		glDeleteVertexArraysOES(1, &m_gl_vao);
#else
		glDeleteVertexArrays(1, &m_gl_vao);
#endif
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void VertexArray::bind()
	{
#if defined(__EMSCRIPTEN__)
		GL_CHECK_ERROR(glBindVertexArrayOES(m_gl_vao));
#else
		GL_CHECK_ERROR(glBindVertexArray(m_gl_vao));
#endif
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void VertexArray::unbind()
	{
#if defined(__EMSCRIPTEN__)
		GL_CHECK_ERROR(glBindVertexArrayOES(0));
#else
		GL_CHECK_ERROR(glBindVertexArray(0));
#endif
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	UniformBuffer::UniformBuffer(GLenum usage, GLenum type, size_t size, void* data)
	{
		GL_CHECK_ERROR(glGenBuffers(1, &m_gl_ubo));

		GL_CHECK_ERROR(glBindBuffer(GL_UNIFORM_BUFFER, m_gl_ubo));
		GL_CHECK_ERROR(glBufferData(GL_UNIFORM_BUFFER, size, data, usage));
		GL_CHECK_ERROR(glBindBuffer(GL_UNIFORM_BUFFER, 0));

#if defined(__EMSCRIPTEN__)
		m_staging = malloc(size);
#endif
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	UniformBuffer::~UniformBuffer()
	{
#if defined(__EMSCRIPTEN__)
		free(m_staging);
#endif
		glDeleteBuffers(1, &m_gl_ubo);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void UniformBuffer::bind_base(int index)
	{
		GL_CHECK_ERROR(glBindBufferBase(GL_UNIFORM_BUFFER, index, m_gl_ubo));
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void UniformBuffer::bind_range(int index, size_t offset, size_t size)
	{
		GL_CHECK_ERROR(glBindBufferRange(GL_UNIFORM_BUFFER, index, m_gl_ubo, offset, size));
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void UniformBuffer::unbind()
	{
		GL_CHECK_ERROR(glBindBuffer(GL_UNIFORM_BUFFER, 0));
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void* UniformBuffer::map(GLenum access)
	{
#if defined(__EMSCRIPTEN__)
		return m_staging;
#else
		GL_CHECK_ERROR(glBindBuffer(GL_UNIFORM_BUFFER, m_gl_ubo));
		GL_CHECK_ERROR(void* ptr = glMapBuffer(GL_UNIFORM_BUFFER, access));
		GL_CHECK_ERROR(glBindBuffer(GL_UNIFORM_BUFFER, 0));
		return ptr;
#endif
	}

	void* UniformBuffer::map_range(GLenum access, size_t offset, size_t size)
	{
#if defined(__EMSCRIPTEN__)
		m_mapped_size = size;
		m_mapped_offset = offset;
		return static_cast<char*>(m_staging) + offset;
#else
		GL_CHECK_ERROR(glBindBuffer(GL_UNIFORM_BUFFER, m_gl_ubo));
		GL_CHECK_ERROR(void* ptr = glMapBufferRange(GL_UNIFORM_BUFFER, offset, size, access));
		GL_CHECK_ERROR(glBindBuffer(GL_UNIFORM_BUFFER, 0));
		return ptr;
#endif
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void UniformBuffer::unmap()
	{
#if defined(__EMSCRIPTEN__)
		GL_CHECK_ERROR(glBindBuffer(GL_UNIFORM_BUFFER, m_gl_ubo));
		glBufferSubData(GL_UNIFORM_BUFFER, m_mapped_offset, m_mapped_size, static_cast<char*>(m_staging) + m_mapped_offset);
		GL_CHECK_ERROR(glBindBuffer(GL_UNIFORM_BUFFER, 0));
#else
		GL_CHECK_ERROR(glBindBuffer(GL_UNIFORM_BUFFER, m_gl_ubo));
		GL_CHECK_ERROR(glUnmapBuffer(GL_UNIFORM_BUFFER));
		GL_CHECK_ERROR(glBindBuffer(GL_UNIFORM_BUFFER, 0));
#endif
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void UniformBuffer::set_data(size_t offset, size_t size, void* data)
	{
		GL_CHECK_ERROR(glBindBuffer(GL_UNIFORM_BUFFER, m_gl_ubo));
		glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
		GL_CHECK_ERROR(glBindBuffer(GL_UNIFORM_BUFFER, 0));
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

#if !defined(__EMSCRIPTEN__)
	ShaderStorageBuffer::ShaderStorageBuffer(GLenum usage, GLenum type, size_t size, void* data)
	{
		GL_CHECK_ERROR(glGenBuffers(1, &m_gl_ssbo));

		GL_CHECK_ERROR(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_gl_ssbo));
		GL_CHECK_ERROR(glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, usage));
		GL_CHECK_ERROR(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	ShaderStorageBuffer::~ShaderStorageBuffer()
	{
		glDeleteBuffers(1, &m_gl_ssbo);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void ShaderStorageBuffer::bind_base(int index)
	{
		GL_CHECK_ERROR(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, m_gl_ssbo));
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void ShaderStorageBuffer::bind_range(int index, size_t offset, size_t size)
	{
		GL_CHECK_ERROR(glBindBufferRange(GL_SHADER_STORAGE_BUFFER, index, m_gl_ssbo, offset, size));
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void ShaderStorageBuffer::unbind()
	{
		GL_CHECK_ERROR(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void* ShaderStorageBuffer::map(GLenum access)
	{
		GL_CHECK_ERROR(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_gl_ssbo));
		GL_CHECK_ERROR(void* ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, access));
		GL_CHECK_ERROR(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));
		return ptr;
	}

	void* ShaderStorageBuffer::map_range(GLenum access, size_t offset, size_t size)
	{
		GL_CHECK_ERROR(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_gl_ssbo));
		GL_CHECK_ERROR(void* ptr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, offset, size, access));
		GL_CHECK_ERROR(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));
		return ptr;
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void ShaderStorageBuffer::unmap()
	{
		GL_CHECK_ERROR(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_gl_ssbo));
		GL_CHECK_ERROR(glUnmapBuffer(GL_SHADER_STORAGE_BUFFER));
		GL_CHECK_ERROR(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void ShaderStorageBuffer::set_data(size_t offset, size_t size, void* data)
	{
		GL_CHECK_ERROR(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_gl_ssbo));
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, data);
		GL_CHECK_ERROR(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));
	}
#endif

	// -----------------------------------------------------------------------------------------------------------------------------------
}
