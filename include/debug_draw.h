#pragma once

#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <Macros.h>
#include <vector>

struct Framebuffer;
class RenderDevice;
struct VertexArray;
struct VertexBuffer;
struct InputLayout;
struct Shader;
struct ShaderProgram;
struct UniformBuffer;
struct RenderDevice;
struct RasterizerState;
struct DepthStencilState;

namespace dd
{
	struct DW_ALIGNED(16) CameraUniforms
	{
		glm::mat4 view_proj;
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

#define MAX_VERTICES 100000

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

	class Renderer
	{
	private:
	private:
		CameraUniforms m_uniforms;
		VertexArray* m_line_vao;
		VertexBuffer* m_line_vbo;
		InputLayout* m_line_il;
		Shader* m_line_vs;
		Shader* m_line_fs;
		ShaderProgram* m_line_program;
		UniformBuffer* m_ubo;
		std::vector<VertexWorld> m_world_vertices;
		std::vector<DrawCommand> m_draw_commands;
		RenderDevice* m_device;
		RasterizerState* m_rs;
		DepthStencilState* m_ds;

	public:
		Renderer();
		bool init(RenderDevice* _device);
		void shutdown();
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
		void render(Framebuffer* fbo, int width, int height, const glm::mat4& view_proj);
	};
}