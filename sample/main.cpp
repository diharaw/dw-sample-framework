#include <application.h>
#include <mesh.h>
#include <camera.h>
#include <material.h>
#include <ogl.h>

// Embedded vertex shader source.
const char* g_sample_vs_src = R"(

layout (location = 0) in vec3 VS_IN_Position;
layout (location = 1) in vec2 VS_IN_TexCoord;
layout (location = 2) in vec3 VS_IN_Normal;
layout (location = 3) in vec3 VS_IN_Tangent;
layout (location = 4) in vec3 VS_IN_Bitangent;

layout (std140) uniform Transforms //#binding 0
{ 
	mat4 model;
	mat4 view;
	mat4 projection;
};

out vec3 PS_IN_FragPos;
out vec3 PS_IN_Normal;
out vec2 PS_IN_TexCoord;

void main()
{
    vec4 position = model * vec4(VS_IN_Position, 1.0);
	PS_IN_FragPos = position.xyz;
	PS_IN_Normal = mat3(model) * VS_IN_Normal;
	PS_IN_TexCoord = VS_IN_TexCoord;
    gl_Position = projection * view * position;
}

)";

// Embedded fragment shader source.
const char* g_sample_fs_src = R"(

precision mediump float;

out vec4 PS_OUT_Color;

in vec3 PS_IN_FragPos;
in vec3 PS_IN_Normal;
in vec2 PS_IN_TexCoord;

uniform sampler2D s_Diffuse; //#slot 0

void main()
{
	vec3 light_pos = vec3(-200.0, 200.0, 0.0);

	vec3 n = normalize(PS_IN_Normal);
	vec3 l = normalize(light_pos - PS_IN_FragPos);

	float lambert = max(0.0f, dot(n, l));

	vec3 diffuse = texture(s_Diffuse, PS_IN_TexCoord).xyz;
	vec3 ambient = diffuse * 0.03;

	vec3 color = diffuse * lambert + ambient;

    PS_OUT_Color = vec4(color, 1.0);
}

)";

// Uniform buffer data structure.
struct Transforms
{
	DW_ALIGNED(16) glm::mat4 model;
	DW_ALIGNED(16) glm::mat4 view;
	DW_ALIGNED(16) glm::mat4 projection;
};

class Sample : public dw::Application
{
protected:
    
    // -----------------------------------------------------------------------------------------------------------------------------------
    
	bool init(int argc, const char* argv[]) override
	{
		// Create GPU resources.
		if (!create_states())
			return false;

		if (!create_shaders())
			return false;

		if (!create_uniform_buffer())
			return false;

		// Load mesh.
		if (!load_mesh())
			return false;

		// Create camera.
		create_camera();

		return true;
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void update(double delta) override
	{
		// Update camera.
		m_main_camera->update();

		// Update uniforms.
		update_uniforms();

		// Render.
		render();
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void shutdown() override
	{
		// Destroy camera.
		DW_SAFE_DELETE(m_main_camera);

		// Unload assets.
		dw::Mesh::unload(m_mesh);

		// Cleanup GPU resources.
		m_device.destroy(m_ubo);
		m_device.destroy(m_program);
		m_device.destroy(m_fs);
		m_device.destroy(m_vs);
		m_device.destroy(m_ds);
		m_device.destroy(m_rs);
		m_device.destroy(m_sampler);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void window_resized(int width, int height) override
	{
		// Override window resized method to update camera projection.
		m_main_camera->update_projection(60.0f, 0.1f, 1000.0f, float(m_width) / float(m_height));
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

private:
    
    // -----------------------------------------------------------------------------------------------------------------------------------
    
	bool create_shaders()
	{
		// Create shaders
		m_vs = m_device.create_shader(g_sample_vs_src, ShaderType::VERTEX);
		m_fs = m_device.create_shader(g_sample_fs_src, ShaderType::FRAGMENT);

		if (!m_vs || !m_fs)
		{
			DW_LOG_FATAL("Failed to create Shaders");
			return false;
		}

		// Create shader program
		Shader* shaders[] = { m_vs, m_fs };
		m_program = m_device.create_shader_program(shaders, 2);

		if (!m_program)
		{
			DW_LOG_FATAL("Failed to create Shader Program");
			return false;
		}

		// TEMP
		ezGL::Shader* vs = new ezGL::Shader(GL_VERTEX_SHADER, "pbr_vs.glsl");
		ezGL::Shader* fs = new ezGL::Shader(GL_FRAGMENT_SHADER, "pbr_fs.glsl");

		ezGL::Shader* ezshaders[] = { vs, fs };

		ezGL::Program* program = new ezGL::Program(2, ezshaders);

		delete program;
		delete vs;
		delete fs;

		return true;
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	bool create_states()
	{
		// Create rasterizer state
		RasterizerStateCreateDesc rs_desc;
		DW_ZERO_MEMORY(rs_desc);
		rs_desc.cull_mode = CullMode::BACK;
		rs_desc.fill_mode = FillMode::SOLID;
		rs_desc.front_winding_ccw = true;
		rs_desc.multisample = true;
		rs_desc.scissor = false;

		m_rs = m_device.create_rasterizer_state(rs_desc);

		// Create depth stencil state
		DepthStencilStateCreateDesc ds_desc;
		DW_ZERO_MEMORY(ds_desc);
		ds_desc.depth_mask = true;
		ds_desc.enable_depth_test = true;
		ds_desc.enable_stencil_test = false;
		ds_desc.depth_cmp_func = ComparisonFunction::LESS_EQUAL;

		m_ds = m_device.create_depth_stencil_state(ds_desc);

		// Create sampler state.
		SamplerStateCreateDesc ss_desc;
		DW_ZERO_MEMORY(ss_desc);
		ss_desc.min_filter = TextureFilteringMode::LINEAR_ALL;
		ss_desc.mag_filter = TextureFilteringMode::LINEAR;
		ss_desc.wrap_mode_u = TextureWrapMode::REPEAT;
		ss_desc.wrap_mode_v = TextureWrapMode::REPEAT;
		ss_desc.wrap_mode_w = TextureWrapMode::REPEAT;

		m_sampler = m_device.create_sampler_state(ss_desc);

		return true;
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	bool create_uniform_buffer()
	{
		// Create uniform buffer for matrix data
		BufferCreateDesc uboDesc;
		DW_ZERO_MEMORY(uboDesc);
		uboDesc.data = nullptr;
		uboDesc.data_type = DataType::FLOAT;
		uboDesc.size = sizeof(Transforms);
		uboDesc.usage_type = BufferUsageType::DYNAMIC;

		m_ubo = m_device.create_uniform_buffer(uboDesc);

		return true;
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	bool load_mesh()
	{
		m_mesh = dw::Mesh::load("teapot.obj", &m_device);
		return m_mesh != nullptr;
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void create_camera()
	{
		m_main_camera = new dw::Camera(60.0f, 0.1f, 1000.0f, float(m_width)/float(m_height), glm::vec3(0.0f, 0.0f, 100.0f), glm::vec3(0.0f, 0.0, -1.0f));
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void render()
	{
		// Bind and clear framebuffer.
		m_device.bind_framebuffer(nullptr);
		m_device.set_viewport(m_width, m_height, 0, 0);

		// Clear default framebuffer.
		m_device.clear_framebuffer(ClearTarget::ALL, m_clear_color);

		// Bind states.
		m_device.bind_rasterizer_state(m_rs);
		m_device.bind_depth_stencil_state(m_ds);

		// Bind shader program.
		m_device.bind_shader_program(m_program);

		// Bind uniform buffer.
		m_device.bind_uniform_buffer(m_ubo, ShaderType::VERTEX, 0);

		// Bind vertex array.
		m_device.bind_vertex_array(m_mesh->mesh_vertex_array());

		// Set primitive type.
		m_device.set_primitive_type(PrimitiveType::TRIANGLES);

		// Bind sampler.
		m_device.bind_sampler_state(m_sampler, ShaderType::FRAGMENT, 0);

		for (uint32_t i = 0; i < m_mesh->sub_mesh_count(); i++)
		{
			dw::SubMesh& submesh = m_mesh->sub_meshes()[i];

			// Bind texture.
			m_device.bind_texture(submesh.mat->texture(0), ShaderType::FRAGMENT, 0);

			// Issue draw call.
			m_device.draw_indexed_base_vertex(submesh.index_count, submesh.base_index, submesh.base_vertex);
		}
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void update_uniforms()
	{
		m_transforms.model = glm::mat4(1.0f);
		m_transforms.model = glm::translate(m_transforms.model, glm::vec3(0.0f, -20.0f, 0.0f));
		m_transforms.model = glm::rotate(m_transforms.model,(float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
		m_transforms.model = glm::scale(m_transforms.model, glm::vec3(0.6f));
		m_transforms.view = m_main_camera->m_view;
		m_transforms.projection = m_main_camera->m_projection;

		void* ptr = m_device.map_buffer(m_ubo, BufferMapType::WRITE);
		memcpy(ptr, &m_transforms, sizeof(Transforms));
		m_device.unmap_buffer(m_ubo);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

private:
	// Clear color.
	float m_clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

	// GPU resources.
	Shader* m_vs;
	Shader* m_fs;
	ShaderProgram* m_program;
	UniformBuffer* m_ubo;
	RasterizerState* m_rs;
	DepthStencilState* m_ds;
	SamplerState* m_sampler;

	// Assets.
	dw::Mesh* m_mesh;

	// Camera.
	dw::Camera* m_main_camera;

	// Uniforms.
	Transforms m_transforms;
};

DW_DECLARE_MAIN(Sample)
