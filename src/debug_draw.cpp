#include <debug_draw.h>
#include <render_device.h>
#include <logger.h>
#include <utility.h>

namespace dd
{
	Renderer::Renderer()
	{
		m_world_vertices.resize(MAX_VERTICES);
		m_world_vertices.clear();

		m_draw_commands.resize(MAX_VERTICES);
		m_draw_commands.clear();
	}

	bool Renderer::init(RenderDevice* _device)
	{
		m_device = _device;

		std::string vs_str;
		Utility::ReadText("shader/debug_draw_vs.glsl", vs_str);

		std::string fs_str;
		Utility::ReadText("shader/debug_draw_fs.glsl", fs_str);

		m_line_vs = m_device->create_shader(vs_str.c_str(), ShaderType::VERTEX);
		m_line_fs = m_device->create_shader(fs_str.c_str(), ShaderType::FRAGMENT);

		if (!m_line_vs || !m_line_fs)
		{
			LOG_FATAL("Failed to create Shaders");
			return false;
		}

		Shader* shaders[] = { m_line_vs, m_line_fs };
		m_line_program = m_device->create_shader_program(shaders, 2);

		BufferCreateDesc bc;
		InputLayoutCreateDesc ilcd;
		VertexArrayCreateDesc vcd;

		DW_ZERO_MEMORY(bc);
		bc.data = nullptr;
		bc.data_type = DataType::FLOAT;
		bc.size = sizeof(VertexWorld) * MAX_VERTICES;
		bc.usage_type = BufferUsageType::DYNAMIC;

		m_line_vbo = m_device->create_vertex_buffer(bc);

		InputElement elements[] =
		{
			{ 3, DataType::FLOAT, false, 0, "POSITION" },
		{ 2, DataType::FLOAT, false, sizeof(float) * 3, "TEXCOORD" },
		{ 3, DataType::FLOAT, false, sizeof(float) * 5, "COLOR" }
		};

		DW_ZERO_MEMORY(ilcd);
		ilcd.elements = elements;
		ilcd.num_elements = 3;
		ilcd.vertex_size = sizeof(float) * 8;

		m_line_il = m_device->create_input_layout(ilcd);

		DW_ZERO_MEMORY(vcd);
		vcd.index_buffer = nullptr;
		vcd.vertex_buffer = m_line_vbo;
		vcd.layout = m_line_il;

		m_line_vao = m_device->create_vertex_array(vcd);

		if (!m_line_vao || !m_line_vbo)
		{
			LOG_FATAL("Failed to create Vertex Buffers/Arrays");
			return false;
		}

		RasterizerStateCreateDesc rs_desc;
		DW_ZERO_MEMORY(rs_desc);
		rs_desc.cull_mode = CullMode::NONE;
		rs_desc.fill_mode = FillMode::SOLID;
		rs_desc.front_winding_ccw = true;
		rs_desc.multisample = true;
		rs_desc.scissor = false;

		m_rs = m_device->create_rasterizer_state(rs_desc);

		DepthStencilStateCreateDesc ds_desc;
		DW_ZERO_MEMORY(ds_desc);
		ds_desc.depth_mask = true;
		ds_desc.enable_depth_test = true;
		ds_desc.enable_stencil_test = false;
		ds_desc.depth_cmp_func = ComparisonFunction::LESS_EQUAL;

		m_ds = m_device->create_depth_stencil_state(ds_desc);

		BufferCreateDesc uboDesc;
		DW_ZERO_MEMORY(uboDesc);
		uboDesc.data = nullptr;
		uboDesc.data_type = DataType::FLOAT;
		uboDesc.size = sizeof(CameraUniforms);
		uboDesc.usage_type = BufferUsageType::DYNAMIC;

		m_ubo = m_device->create_uniform_buffer(uboDesc);

		return true;
	}

	void Renderer::shutdown()
	{
		m_device->destroy(m_ubo);
		m_device->destroy(m_line_program);
		m_device->destroy(m_line_vs);
		m_device->destroy(m_line_fs);
		m_device->destroy(m_line_vbo);
		m_device->destroy(m_line_vao);
		m_device->destroy(m_ds);
		m_device->destroy(m_rs);
	}

	void Renderer::capsule(const float& _height, const float& _radius, const glm::vec3& _pos, const glm::vec3& _c)
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

	void Renderer::aabb(const glm::vec3& _min, const glm::vec3& _max, const glm::vec3& _pos, const glm::vec3& _c)
	{
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

	void Renderer::obb(const glm::vec3& _min, const glm::vec3& _max, const glm::mat4& _model, const glm::vec3& _c)
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

	void Renderer::grid(const float& _x, const float& _z, const float& _y_level, const float& spacing, const glm::vec3& _c)
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

	void Renderer::line(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& c)
	{
		VertexWorld vw0, vw1;
		vw0.position = v0;
		vw0.color = c;

		vw1.position = v1;
		vw1.color = c;

		m_world_vertices.push_back(vw0);
		m_world_vertices.push_back(vw1);

		DrawCommand cmd;
		cmd.type = PrimitiveType::LINES;
		cmd.vertices = 2;

		m_draw_commands.push_back(cmd);
	}

	void Renderer::line_strip(glm::vec3* v, const int& count, const glm::vec3& c)
	{
		for (int i = 0; i < count; i++)
		{
			VertexWorld vert;
			vert.position = v[i];
			vert.color = c;

			m_world_vertices.push_back(vert);
		}

		DrawCommand cmd;
		cmd.type = PrimitiveType::LINE_STRIP;
		cmd.vertices = count;

		m_draw_commands.push_back(cmd);
	}

	void Renderer::circle_xy(float radius, const glm::vec3& pos, const glm::vec3& c)
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

	void Renderer::circle_xz(float radius, const glm::vec3& pos, const glm::vec3& c)
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

	void Renderer::circle_yz(float radius, const glm::vec3& pos, const glm::vec3& c)
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

	void Renderer::sphere(const float& radius, const glm::vec3& pos, const glm::vec3& c)
	{
		circle_xy(radius, pos, c);
		circle_xz(radius, pos, c);
		circle_yz(radius, pos, c);
	}

	void Renderer::frustum(const glm::mat4& proj, const glm::mat4& view, const glm::vec3& c)
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

	void Renderer::render(Framebuffer* fbo, int width, int height, const glm::mat4& view_proj)
	{
		m_uniforms.view_proj = view_proj;

		void* ptr = m_device->map_buffer(m_line_vbo, BufferMapType::WRITE);

		if (m_world_vertices.size() > MAX_VERTICES)
			std::cout << "Vertices are above limit" << std::endl;
		else
			memcpy(ptr, &m_world_vertices[0], sizeof(VertexWorld) * m_world_vertices.size());

		m_device->unmap_buffer(m_line_vbo);

		ptr = m_device->map_buffer(m_ubo, BufferMapType::WRITE);
		memcpy(ptr, &m_uniforms, sizeof(CameraUniforms));
		m_device->unmap_buffer(m_ubo);

		m_device->bind_rasterizer_state(m_rs);
		m_device->bind_depth_stencil_state(m_ds);
		m_device->bind_framebuffer(fbo);
		m_device->set_viewport(width, height, 0, 0);
		m_device->bind_shader_program(m_line_program);
		m_device->bind_uniform_buffer(m_ubo, ShaderType::VERTEX, 0);
		m_device->bind_vertex_array(m_line_vao);

		int v = 0;

		for (int i = 0; i < m_draw_commands.size(); i++)
		{
			DrawCommand& cmd = m_draw_commands[i];
			m_device->set_primitive_type(cmd.type);
			m_device->draw(v, cmd.vertices);
			v += cmd.vertices;
		}

		m_draw_commands.clear();
		m_world_vertices.clear();
	}
}