#include <debug_draw.h>
#include <logger.h>
#include <utility.h>

namespace dw
{
	const char* g_vs_src = R"(

	layout (location = 0) in vec3 VS_IN_Position;
	layout (location = 1) in vec2 VS_IN_TexCoord;
	layout (location = 2) in vec3 VS_IN_Color;
	
	layout (std140) uniform CameraUniforms //#binding 0
	{ 
		mat4 viewProj;
	};
	
	out vec3 PS_IN_Color;
	
	void main()
	{
	    PS_IN_Color = VS_IN_Color;
	    gl_Position = viewProj * vec4(VS_IN_Position, 1.0);
	}

	)";

	const char* g_fs_src = R"(

    precision mediump float;
    
	out vec4 PS_OUT_Color;

	in vec3 PS_IN_Color;
	
	void main()
	{
	    PS_OUT_Color = vec4(PS_IN_Color, 1.0);
	}

	)";

	// -----------------------------------------------------------------------------------------------------------------------------------

	DebugDraw::DebugDraw()
	{
		m_world_vertices.resize(MAX_VERTICES);
		m_world_vertices.clear();

		m_draw_commands.resize(MAX_VERTICES);
		m_draw_commands.clear();
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	bool DebugDraw::init()
	{
		// Create shaders
		m_line_vs = std::make_unique<Shader>(GL_VERTEX_SHADER, g_vs_src);
		m_line_fs = std::make_unique<Shader>(GL_FRAGMENT_SHADER, g_fs_src);

		if (!m_line_vs || !m_line_fs)
		{
			DW_LOG_FATAL("Failed to create Shaders");
			return false;
		}

		// Create shader program
		Shader* shaders[] = { m_line_vs.get(), m_line_fs.get() };
		m_line_program = std::make_unique<Program>(2, shaders);

		// Bind uniform block index
		m_line_program->uniform_block_binding("CameraUniforms", 0);

		// Create vertex buffer
		m_line_vbo = std::make_unique<VertexBuffer>(GL_DYNAMIC_DRAW, sizeof(VertexWorld) * MAX_VERTICES);

		// Declare vertex attributes
		VertexAttrib attribs[] =
		{
			{ 3, GL_FLOAT, false, 0 },
			{ 2, GL_FLOAT, false, sizeof(float) * 3 },
			{ 3, GL_FLOAT, false, sizeof(float) * 5 }
		};

		// Create vertex array
		m_line_vao = std::make_unique<VertexArray>(m_line_vbo.get(), nullptr, sizeof(float) * 8, 3, attribs);

		if (!m_line_vao || !m_line_vbo)
		{
			DW_LOG_FATAL("Failed to create Vertex Buffers/Arrays");
			return false;
		}

		// Create uniform buffer for matrix data
		m_ubo = std::make_unique<UniformBuffer>(GL_DYNAMIC_DRAW, sizeof(CameraUniforms));

		return true;
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void DebugDraw::shutdown()
	{
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void DebugDraw::capsule(const float& _height, const float& _radius, const glm::vec3& _pos, const glm::vec3& _c)
	{
		// Draw four lines
		line(glm::vec3(_pos.x, _pos.y + _radius, _pos.z - _radius), glm::vec3(_pos.x, _height - _radius, _pos.z - _radius), _c);
		line(glm::vec3(_pos.x, _pos.y + _radius, _pos.z + _radius), glm::vec3(_pos.x, _height - _radius, _pos.z + _radius), _c);
		line(glm::vec3(_pos.x - _radius, _pos.y + _radius, _pos.z), glm::vec3(_pos.x - _radius, _height - _radius, _pos.z), _c);
		line(glm::vec3(_pos.x + _radius, _pos.y + _radius, _pos.z), glm::vec3(_pos.x + _radius, _height - _radius, _pos.z), _c);

		glm::vec3 verts[10];

		int idx = 0;

		for (int i = 0; i <= 180; i += 20)
		{
			float degInRad = glm::radians((float)i);
			verts[idx++] = glm::vec3(_pos.x + cos(degInRad)*_radius, _height - _radius + sin(degInRad)*_radius, _pos.z);
		}

		line_strip(&verts[0], 10, _c);

		idx = 0;

		for (int i = 0; i <= 180; i += 20)
		{
			float degInRad = glm::radians((float)i);
			verts[idx++] = glm::vec3(_pos.x, _height - _radius + sin(degInRad)*_radius, _pos.z + cos(degInRad)*_radius);
		}

		line_strip(&verts[0], 10, _c);

		idx = 0;

		for (int i = 180; i <= 360; i += 20)
		{
			float degInRad = glm::radians((float)i);
			verts[idx++] = glm::vec3(_pos.x + cos(degInRad)*_radius, _radius + sin(degInRad)*_radius, _pos.z);
		}

		line_strip(&verts[0], 10, _c);

		idx = 0;

		for (int i = 180; i <= 360; i += 20)
		{
			float degInRad = glm::radians((float)i);
			verts[idx++] = glm::vec3(_pos.x, _radius + sin(degInRad)*_radius, _pos.z + cos(degInRad)*_radius);
		}

		line_strip(&verts[0], 10, _c);

		circle_xz(_radius, glm::vec3(_pos.x, _height - _radius, _pos.z), _c);
		circle_xz(_radius, glm::vec3(_pos.x, _radius, _pos.z), _c);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void DebugDraw::aabb(const glm::vec3& _min, const glm::vec3& _max, const glm::vec3& _c)
	{
		glm::vec3 _pos = (_max + _min) * 0.5f;

		glm::vec3 min = _pos + _min;
		glm::vec3 max = _pos + _max;

		line(min, glm::vec3(max.x, min.y, min.z), _c);
		line(glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, min.y, max.z), _c);
		line(glm::vec3(max.x, min.y, max.z), glm::vec3(min.x, min.y, max.z), _c);
		line(glm::vec3(min.x, min.y, max.z), min, _c);

		line(glm::vec3(min.x, max.y, min.z), glm::vec3(max.x, max.y, min.z), _c);
		line(glm::vec3(max.x, max.y, min.z), max, _c);
		line(max, glm::vec3(min.x, max.y, max.z), _c);
		line(glm::vec3(min.x, max.y, max.z), glm::vec3(min.x, max.y, min.z), _c);

		line(min, glm::vec3(min.x, max.y, min.z), _c);
		line(glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, max.y, min.z), _c);
		line(glm::vec3(max.x, min.y, max.z), max, _c);
		line(glm::vec3(min.x, min.y, max.z), glm::vec3(min.x, max.y, max.z), _c);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void DebugDraw::obb(const glm::vec3& _min, const glm::vec3& _max, const glm::mat4& _model, const glm::vec3& _c)
	{
		glm::vec3 verts[8];
		glm::vec3 size = _max - _min;
		int idx = 0;

		for (float x = _min.x; x <= _max.x; x += size.x)
		{
			for (float y = _min.y; y <= _max.y; y += size.y)
			{
				for (float z = _min.z; z <= _max.z; z += size.z)
				{
					glm::vec4 v = _model * glm::vec4(x, y, z, 1.0f);
					verts[idx++] = glm::vec3(v.x, v.y, v.z);
				}
			}
		}

		line(verts[0], verts[1], _c);
		line(verts[1], verts[5], _c);
		line(verts[5], verts[4], _c);
		line(verts[4], verts[0], _c);

		line(verts[2], verts[3], _c);
		line(verts[3], verts[7], _c);
		line(verts[7], verts[6], _c);
		line(verts[6], verts[2], _c);

		line(verts[2], verts[0], _c);
		line(verts[6], verts[4], _c);
		line(verts[3], verts[1], _c);
		line(verts[7], verts[5], _c);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void DebugDraw::grid(const float& _x, const float& _z, const float& _y_level, const float& spacing, const glm::vec3& _c)
	{
		int offset_x = floor((_x * spacing) / 2.0f);
		int offset_z = floor((_z * spacing) / 2.0f);

		for (int x = -offset_x; x <= offset_x; x += spacing)
		{
			line(glm::vec3(x, _y_level, -offset_z), glm::vec3(x, _y_level, offset_z), _c);
		}

		for (int z = -offset_z; z <= offset_z; z += spacing)
		{
			line(glm::vec3(-offset_x, _y_level, z), glm::vec3(offset_x, _y_level, z), _c);
		}
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void DebugDraw::line(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& c)
	{
		if (m_world_vertices.size() < MAX_VERTICES)
		{
			VertexWorld vw0, vw1;
			vw0.position = v0;
			vw0.color = c;

			vw1.position = v1;
			vw1.color = c;

			m_world_vertices.push_back(vw0);
			m_world_vertices.push_back(vw1);

			DrawCommand cmd;
			cmd.type = GL_LINES;
			cmd.vertices = 2;

			m_draw_commands.push_back(cmd);
		}
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void DebugDraw::line_strip(glm::vec3* v, const int& count, const glm::vec3& c)
	{
		for (int i = 0; i < count; i++)
		{
			VertexWorld vert;
			vert.position = v[i];
			vert.color = c;

			m_world_vertices.push_back(vert);
		}

		DrawCommand cmd;
		cmd.type = GL_LINE_STRIP;
		cmd.vertices = count;

		m_draw_commands.push_back(cmd);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void DebugDraw::circle_xy(float radius, const glm::vec3& pos, const glm::vec3& c)
	{
		glm::vec3 verts[19];

		int idx = 0;

		for (int i = 0; i <= 360; i += 20)
		{
			float degInRad = glm::radians((float)i);
			verts[idx++] = pos + glm::vec3(cos(degInRad)*radius, sin(degInRad)*radius, 0.0f);
		}

		line_strip(&verts[0], 19, c);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void DebugDraw::circle_xz(float radius, const glm::vec3& pos, const glm::vec3& c)
	{
		glm::vec3 verts[19];

		int idx = 0;

		for (int i = 0; i <= 360; i += 20)
		{
			float degInRad = glm::radians((float)i);
			verts[idx++] = pos + glm::vec3(cos(degInRad)*radius, 0.0f, sin(degInRad)*radius);
		}

		line_strip(&verts[0], 19, c);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void DebugDraw::circle_yz(float radius, const glm::vec3& pos, const glm::vec3& c)
	{
		glm::vec3 verts[19];

		int idx = 0;

		for (int i = 0; i <= 360; i += 20)
		{
			float degInRad = glm::radians((float)i);
			verts[idx++] = pos + glm::vec3(0.0f, cos(degInRad)*radius, sin(degInRad)*radius);
		}

		line_strip(&verts[0], 19, c);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void DebugDraw::sphere(const float& radius, const glm::vec3& pos, const glm::vec3& c)
	{
		circle_xy(radius, pos, c);
		circle_xz(radius, pos, c);
		circle_yz(radius, pos, c);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void DebugDraw::frustum(const glm::mat4& view_proj, const glm::vec3& c)
	{
		glm::mat4 inverse = glm::inverse(view_proj);
		glm::vec3 corners[8];

		for (int i = 0; i < 8; i++)
		{
			glm::vec4 v = inverse * kFrustumCorners[i];
			v = v / v.w;
			corners[i] = glm::vec3(v.x, v.y, v.z);
		}

		glm::vec3 _far[5] = {
			corners[0],
			corners[1],
			corners[2],
			corners[3],
			corners[0]
		};

		line_strip(&_far[0], 5, c);

		glm::vec3 _near[5] = {
			corners[4],
			corners[5],
			corners[6],
			corners[7],
			corners[4]
		};

		line_strip(&_near[0], 5, c);

		line(corners[0], corners[4], c);
		line(corners[1], corners[5], c);
		line(corners[2], corners[6], c);
		line(corners[3], corners[7], c);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void DebugDraw::frustum(const glm::mat4& proj, const glm::mat4& view, const glm::vec3& c)
	{
		glm::mat4 inverse = glm::inverse(proj * view);
		glm::vec3 corners[8];

		for (int i = 0; i < 8; i++)
		{
			glm::vec4 v = inverse * kFrustumCorners[i];
			v = v / v.w;
			corners[i] = glm::vec3(v.x, v.y, v.z);
		}

		glm::vec3 _far[5] = {
			corners[0],
			corners[1],
			corners[2],
			corners[3],
			corners[0]
		};

		line_strip(&_far[0], 5, c);

		glm::vec3 _near[5] = {
			corners[4],
			corners[5],
			corners[6],
			corners[7],
			corners[4]
		};

		line_strip(&_near[0], 5, c);

		line(corners[0], corners[4], c);
		line(corners[1], corners[5], c);
		line(corners[2], corners[6], c);
		line(corners[3], corners[7], c);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void DebugDraw::render(Framebuffer* fbo, int width, int height, const glm::mat4& view_proj)
	{
		if (m_world_vertices.size() > 0)
		{
			m_uniforms.view_proj = view_proj;

			void* ptr = m_line_vbo->map(GL_WRITE_ONLY);

			if (m_world_vertices.size() > MAX_VERTICES)
				DW_LOG_ERROR("Vertex count above allowed limit!");
			else
				memcpy(ptr, &m_world_vertices[0], sizeof(VertexWorld) * m_world_vertices.size());

			m_line_vbo->unmap();

			ptr = m_ubo->map(GL_WRITE_ONLY);
			memcpy(ptr, &m_uniforms, sizeof(CameraUniforms));
			m_ubo->unmap();

			// Get previous state
			GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
			GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);

			// Set initial state
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_CULL_FACE);

			if (fbo)
				fbo->bind();
			else
				glBindFramebuffer(GL_FRAMEBUFFER, 0);

			glViewport(0, 0, width, height);
			m_line_program->use();
			m_ubo->bind_base(0);
			m_line_vao->bind();

			int v = 0;

			for (int i = 0; i < m_draw_commands.size(); i++)
			{
				DrawCommand& cmd = m_draw_commands[i];
				glDrawArrays(cmd.type, v, cmd.vertices);
				v += cmd.vertices;
			}

			m_draw_commands.clear();
			m_world_vertices.clear();

			// Restore state
			if (last_enable_cull_face)
				glEnable(GL_CULL_FACE);

			if (last_enable_depth_test)
				glEnable(GL_DEPTH_TEST);
		}
	}

	// -----------------------------------------------------------------------------------------------------------------------------------
} // namespace dw
