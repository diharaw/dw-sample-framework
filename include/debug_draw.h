#pragma once

#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <macros.h>
#include <vector>
#include <memory>
#include <ogl.h>

// Hard-limit of vertices. Used to reserve space in vertex and draw command vectors. 
#define MAX_VERTICES 100000

namespace dw
{
	struct CameraUniforms
	{
		DW_ALIGNED(16) glm::mat4 view_proj;
	};

	struct VertexWorld
	{
		glm::vec3 position;
		glm::vec2 uv;
		glm::vec3 color;
	};

	struct DrawCommand
	{
		int type;
		int vertices;
	};

	const glm::vec4 kFrustumCorners[] = 
	{
		glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),  // Far-Bottom-Left
		glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f),   // Far-Top-Left
		glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),    // Far-Top-Right
		glm::vec4(1.0f, -1.0f, 1.0f, 1.0f),   // Far-Bottom-Right
		glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f), // Near-Bottom-Left
		glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f),  // Near-Top-Left
		glm::vec4(1.0f, 1.0f, -1.0f, 1.0f),   // Near-Top-Right
		glm::vec4(1.0f, -1.0f, -1.0f, 1.0f)	  // Near-Bottom-Right
	};

	class DebugDraw
	{
	public:
		DebugDraw();

		// Initialization and shutdown.
		bool init();
		void shutdown();

		// Debug shape drawing.
		void capsule(const float& _height, const float& _radius, const glm::vec3& _pos, const glm::vec3& _c);
		void aabb(const glm::vec3& _min, const glm::vec3& _max, const glm::vec3& _c);
		void obb(const glm::vec3& _min, const glm::vec3& _max, const glm::mat4& _model, const glm::vec3& _c);
		void grid(const float& _x, const float& _z, const float& _y_level, const float& spacing, const glm::vec3& _c);
		void line(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& c);
		void line_strip(glm::vec3* v, const int& count, const glm::vec3& c);
		void circle_xy(float radius, const glm::vec3& pos, const glm::vec3& c);
		void circle_xz(float radius, const glm::vec3& pos, const glm::vec3& c);
		void circle_yz(float radius, const glm::vec3& pos, const glm::vec3& c);
		void sphere(const float& radius, const glm::vec3& pos, const glm::vec3& c);
		void frustum(const glm::mat4& view_proj, const glm::vec3& c);
		void frustum(const glm::mat4& proj, const glm::mat4& view, const glm::vec3& c);
		void transform(const glm::mat4& trans, const float& axis_length = 5.0f);
		
		// Render method. Pass in target Framebuffer, viewport size and view-projection matrix.
		void render(Framebuffer* fbo, int width, int height, const glm::mat4& view_proj);

	private:
		// Vertex list to be uploaded to GPU.
		std::vector<VertexWorld> m_world_vertices;

		// Draw command list.
		std::vector<DrawCommand> m_draw_commands;

		// Camera matrix.
		CameraUniforms m_uniforms;

		// GPU resources.
		std::unique_ptr<VertexArray> m_line_vao;
		std::unique_ptr<VertexBuffer> m_line_vbo;
		std::unique_ptr<Shader> m_line_vs;
		std::unique_ptr<Shader> m_line_fs;
		std::unique_ptr<Program> m_line_program;
		std::unique_ptr<UniformBuffer> m_ubo;
	};
} // namespace dw