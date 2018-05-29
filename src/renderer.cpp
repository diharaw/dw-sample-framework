#include <renderer.h>
#include <render_device.h>
#include <camera.h>
#include <Material.h>
#include <Mesh.h>
#include <logger.h>
#include <Scene.h>
#include <entity.h>
#include <trm_loader.h>
#include <utility.h>
#include <fstream>
#include <shadows.h>

namespace dw
{
	const float clear_color[] = { 0.1f, 0.1f, 0.1f, 1.0f };

	Renderer::Renderer(RenderDevice* device, uint16_t width, uint16_t height) : m_width(width), m_height(height)
	{
		m_device = device;

		BufferCreateDesc per_frame_ubo_desc;
		DW_ZERO_MEMORY(per_frame_ubo_desc);
		per_frame_ubo_desc.data = nullptr;
		per_frame_ubo_desc.data_type = DataType::FLOAT;
		per_frame_ubo_desc.size = sizeof(PerFrameUniforms);
		per_frame_ubo_desc.usage_type = BufferUsageType::DYNAMIC;

		BufferCreateDesc per_entity_ubo_desc;
		DW_ZERO_MEMORY(per_entity_ubo_desc);
		per_entity_ubo_desc.data = nullptr;
		per_entity_ubo_desc.data_type = DataType::FLOAT;
		per_entity_ubo_desc.size = 1024 * sizeof(PerEntityUniforms);
		per_entity_ubo_desc.usage_type = BufferUsageType::DYNAMIC;

		BufferCreateDesc per_scene_ubo_desc;
		DW_ZERO_MEMORY(per_scene_ubo_desc);
		per_scene_ubo_desc.data = nullptr;
		per_scene_ubo_desc.data_type = DataType::FLOAT;
		per_scene_ubo_desc.size = sizeof(PerSceneUniforms);
		per_scene_ubo_desc.usage_type = BufferUsageType::DYNAMIC;

		BufferCreateDesc per_frustum_split_ubo_desc;
		DW_ZERO_MEMORY(per_frustum_split_ubo_desc);
		per_frustum_split_ubo_desc.data = nullptr;
		per_frustum_split_ubo_desc.data_type = DataType::FLOAT;
		per_frustum_split_ubo_desc.size = 256 * 8;
		per_frustum_split_ubo_desc.usage_type = BufferUsageType::DYNAMIC;

		m_per_frame = m_device->create_uniform_buffer(per_frame_ubo_desc);
		m_per_entity = m_device->create_uniform_buffer(per_entity_ubo_desc);
		m_per_scene = m_device->create_uniform_buffer(per_scene_ubo_desc);
		m_per_frustum_split = m_device->create_uniform_buffer(per_frustum_split_ubo_desc);

		RasterizerStateCreateDesc rs_desc;
		DW_ZERO_MEMORY(rs_desc);
		rs_desc.cull_mode = CullMode::BACK;
		rs_desc.fill_mode = FillMode::SOLID;
		rs_desc.front_winding_ccw = true;
		rs_desc.multisample = true;
		rs_desc.scissor = false;

		m_standard_rs = m_device->create_rasterizer_state(rs_desc);

		rs_desc.front_winding_ccw = false;

		m_atmosphere_rs = m_device->create_rasterizer_state(rs_desc);

		DepthStencilStateCreateDesc ds_desc;
		DW_ZERO_MEMORY(ds_desc);
		ds_desc.depth_mask = true;
		ds_desc.enable_depth_test = true;
		ds_desc.enable_stencil_test = false;
		ds_desc.depth_cmp_func = ComparisonFunction::LESS;

		m_standard_ds = m_device->create_depth_stencil_state(ds_desc);

		ds_desc.depth_cmp_func = ComparisonFunction::LESS_EQUAL;

		m_atmosphere_ds = m_device->create_depth_stencil_state(ds_desc);

		SamplerStateCreateDesc ssDesc;
		DW_ZERO_MEMORY(ssDesc);

		ssDesc.max_anisotropy = 0;
		ssDesc.min_filter = TextureFilteringMode::LINEAR;
		ssDesc.mag_filter = TextureFilteringMode::LINEAR;
		ssDesc.wrap_mode_u = TextureWrapMode::CLAMP_TO_EDGE;
		ssDesc.wrap_mode_v = TextureWrapMode::CLAMP_TO_EDGE;
		ssDesc.wrap_mode_w = TextureWrapMode::CLAMP_TO_EDGE;

		m_bilinear_sampler = m_device->create_sampler_state(ssDesc);

		ssDesc.min_filter = TextureFilteringMode::LINEAR_ALL;
		ssDesc.mag_filter = TextureFilteringMode::LINEAR;
		ssDesc.wrap_mode_u = TextureWrapMode::REPEAT;
		ssDesc.wrap_mode_v = TextureWrapMode::REPEAT;
		ssDesc.wrap_mode_w = TextureWrapMode::REPEAT;

		m_trilinear_sampler = m_device->create_sampler_state(ssDesc);

		BlendStateCreateDesc bsDesc;
		DW_ZERO_MEMORY(bsDesc);

		bsDesc.enable = false;
		bsDesc.blend_op_alpha = BlendOp::ADD;
		bsDesc.src_func_alpha = BlendFunc::SRC_ALPHA;
		bsDesc.dst_func_alpha = BlendFunc::ONE_MINUS_SRC_ALPHA;
		bsDesc.blend_op = BlendOp::ADD;
		bsDesc.src_func = BlendFunc::SRC_ALPHA;
		bsDesc.dst_func = BlendFunc::ONE_MINUS_SRC_ALPHA;

		m_standard_bs = m_device->create_blend_state(bsDesc);

		Texture2DCreateDesc color_buffer_desc;
		DW_ZERO_MEMORY(color_buffer_desc);

		color_buffer_desc.height = m_height;
		color_buffer_desc.width = m_width;
		color_buffer_desc.mipmap_levels = 1;
		color_buffer_desc.format = TextureFormat::R8G8B8A8_SNORM;

		m_color_buffer = m_device->create_texture_2d(color_buffer_desc);

		Texture2DCreateDesc depth_buffer_desc;
		DW_ZERO_MEMORY(depth_buffer_desc);

		depth_buffer_desc.height = m_height;
		depth_buffer_desc.width = m_width;
		depth_buffer_desc.mipmap_levels = 1;
		depth_buffer_desc.format = TextureFormat::D32_FLOAT_S8_UINT;

		m_depth_buffer = m_device->create_texture_2d(depth_buffer_desc);

		FramebufferCreateDesc fbo_desc;
		DW_ZERO_MEMORY(fbo_desc);

		fbo_desc.depthStencilTarget.texture = m_depth_buffer;
		fbo_desc.depthStencilTarget.mipSlice = 0;
		fbo_desc.depthStencilTarget.arraySlice = 0;
		fbo_desc.renderTargetCount = 1;
		fbo_desc.renderTargets[0].texture = m_color_buffer;
		fbo_desc.renderTargets[0].arraySlice = 0;
		fbo_desc.renderTargets[0].mipSlice = 0;

		m_color_fbo = m_device->create_framebuffer(fbo_desc);

		m_brdfLUT = (Texture2D*)trm::load_image(Utility::executable_path() + "/assets/texture/brdfLUT.trm", TextureFormat::R16G16_FLOAT, m_device);

		create_cube();
		create_quad();

		// Load cubemap shaders
		{
			std::string path = Utility::executable_path() + "/assets/shader/cubemap_vs.glsl";
			m_cube_map_vs = load_shader(ShaderType::VERTEX, path, nullptr);
			path = Utility::executable_path() + "/assets/shader/cubemap_fs.glsl";
			m_cube_map_fs = load_shader(ShaderType::FRAGMENT, path, nullptr);

			Shader* shaders[] = { m_cube_map_vs, m_cube_map_fs };

			path = Utility::executable_path() + "/cubemap_vs.glslcubemap_fs.glsl";
			m_cube_map_program = load_program(path, 2, &shaders[0]);

			if (!m_cube_map_vs || !m_cube_map_fs || !m_cube_map_program)
			{
				LOG_ERROR("Failed to load cubemap shaders");
			}
		}

		// Load shadowmap shaders
		{
			std::string path = Utility::executable_path() + "/assets/shader/pssm_vs.glsl";
			m_pssm_vs = load_shader(ShaderType::VERTEX, path, nullptr);
			path = Utility::executable_path() + "/assets/shader/pssm_fs.glsl";
			m_pssm_fs = load_shader(ShaderType::FRAGMENT, path, nullptr);

			Shader* shaders[] = { m_pssm_vs, m_pssm_fs };

			path = Utility::executable_path() + "/pssm_vs.glslpssm_fs.glsl";
			m_pssm_program = load_program(path, 2, &shaders[0]);

			if (!m_pssm_vs || !m_pssm_fs || !m_pssm_program)
			{
				LOG_ERROR("Failed to load PSSM shaders");
			}
		}

		m_per_scene_uniforms.pointLightCount = 0;
		m_per_scene_uniforms.pointLights[0].position = glm::vec4(-10.0f, 20.0f, 10.0f, 1.0f);
		m_per_scene_uniforms.pointLights[0].color = glm::vec4(300.0f);
		m_per_scene_uniforms.pointLights[1].position = glm::vec4(10.0f, 20.0f, 10.0f, 1.0f);
		m_per_scene_uniforms.pointLights[1].color = glm::vec4(300.0f);
		m_per_scene_uniforms.pointLights[2].position = glm::vec4(-10.0f, -20.0f, 10.0f, 1.0f);
		m_per_scene_uniforms.pointLights[2].color = glm::vec4(300.0f);
		m_per_scene_uniforms.pointLights[3].position = glm::vec4(10.0f, -20.0f, 10.0f, 1.0f);
		m_per_scene_uniforms.pointLights[3].color = glm::vec4(300.0f);

		m_per_scene_uniforms.directionalLight.color = glm::vec4(1.0f);
		m_per_scene_uniforms.directionalLight.direction = glm::vec4(glm::normalize(glm::vec3(1.0f, -1.0f, 0.0f)), 1.0f);
	}

	Renderer::~Renderer()
	{
		m_device->destroy(m_color_fbo);
		m_device->destroy(m_depth_buffer);
		m_device->destroy(m_color_buffer);
		m_device->destroy(m_quad_vao);
		m_device->destroy(m_quad_vbo);
		delete m_quad_layout;
		m_device->destroy(m_cube_vao);
		m_device->destroy(m_cube_vbo);
		delete m_cube_layout;
		m_device->destroy(m_brdfLUT);
		m_device->destroy(m_trilinear_sampler);
		m_device->destroy(m_bilinear_sampler);
		m_device->destroy(m_atmosphere_ds);
		m_device->destroy(m_standard_ds);
		m_device->destroy(m_standard_rs);
		m_device->destroy(m_standard_bs);
		m_device->destroy(m_per_scene);
		m_device->destroy(m_per_entity);
		m_device->destroy(m_per_frame);

		for (auto itr : m_program_cache)
		{
			m_device->destroy(itr.second);
		}

		for (auto itr : m_shader_cache)
		{
			m_device->destroy(itr.second);
		}
	}

	void Renderer::create_cube()
	{
		float cubeVertices[] = 
		{
			// back face
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			// bottom face
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			// top face
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};

		BufferCreateDesc bc;
		InputLayoutCreateDesc ilcd;
		VertexArrayCreateDesc vcd;

		DW_ZERO_MEMORY(bc);
		bc.data = (float*)&cubeVertices[0];
		bc.data_type = DataType::FLOAT;
		bc.size = sizeof(cubeVertices);
		bc.usage_type = BufferUsageType::STATIC;

		m_cube_vbo = m_device->create_vertex_buffer(bc);

		InputElement elements[] =
		{
			{ 3, DataType::FLOAT, false, 0,					"POSITION" },
			{ 3, DataType::FLOAT, false, sizeof(float) * 3, "NORMAL"   },
			{ 2, DataType::FLOAT, false, sizeof(float) * 6, "TEXCOORD" }
		};

		DW_ZERO_MEMORY(ilcd);
		ilcd.elements = elements;
		ilcd.num_elements = 3;
		ilcd.vertex_size = sizeof(float) * 8;

		m_cube_layout = m_device->create_input_layout(ilcd);

		DW_ZERO_MEMORY(vcd);
		vcd.index_buffer = nullptr;
		vcd.vertex_buffer = m_cube_vbo;
		vcd.layout = m_cube_layout;

		m_cube_vao = m_device->create_vertex_array(vcd);

		if (!m_cube_vbo || !m_cube_vao)
		{
			LOG_FATAL("Failed to create Vertex Buffers/Arrays");
		}
	}

	void Renderer::create_quad()
	{
		const float vertices[] = {
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};

		BufferCreateDesc bc;
		InputLayoutCreateDesc ilcd;
		VertexArrayCreateDesc vcd;

		DW_ZERO_MEMORY(bc);
		bc.data = (float*)&vertices[0];
		bc.data_type = DataType::FLOAT;
		bc.size = sizeof(vertices);
		bc.usage_type = BufferUsageType::STATIC;

		m_quad_vbo = m_device->create_vertex_buffer(bc);

		InputElement quadElements[] =
		{
			{ 3, DataType::FLOAT, false, 0,					"POSITION" },
			{ 2, DataType::FLOAT, false, sizeof(float) * 3, "TEXCOORD" }
		};

		DW_ZERO_MEMORY(ilcd);
		ilcd.elements = quadElements;
		ilcd.num_elements = 2;
		ilcd.vertex_size = sizeof(float) * 5;

		m_quad_layout = m_device->create_input_layout(ilcd);

		DW_ZERO_MEMORY(vcd);
		vcd.index_buffer = nullptr; //ibo;
		vcd.vertex_buffer = m_quad_vbo;
		vcd.layout = m_quad_layout;

		m_quad_vao = m_device->create_vertex_array(vcd);

		if (!m_quad_vbo || !m_quad_vao)
		{
			LOG_FATAL("Failed to create Vertex Buffers/Arrays");
		}

		// Load quad shaders
		{
			std::string path = Utility::executable_path() + "/assets/shader/quad_vs.glsl";
			m_quad_vs = load_shader(ShaderType::VERTEX, path, nullptr);
			path = Utility::executable_path() + "/assets/shader/quad_fs.glsl";
			m_quad_fs = load_shader(ShaderType::FRAGMENT, path, nullptr);

			Shader* shaders[] = { m_quad_vs, m_quad_fs };

			path = Utility::executable_path() + "/quad_vs.glslquadfs.glsl";
			m_quad_program = load_program(path, 2, &shaders[0]);

			if (!m_quad_vs || !m_quad_fs || !m_quad_program)
			{
				LOG_ERROR("Failed to load Quad shaders");
			}
		}
	}

	void Renderer::set_scene(Scene* scene)
	{
		m_scene = scene;
	}

	Scene* Renderer::scene()
	{
		return m_scene;
	}

	Shader* Renderer::load_shader(int type, std::string& path, Material* mat)
	{
		std::string source; 

		if (!Utility::ReadText(path, source))
		{
			LOG_ERROR("Failed to read shader source");
			return nullptr;
		}

		if (m_shader_cache.find(path) == m_shader_cache.end())
		{
			LOG_INFO("Shader Asset not in cache. Loading from disk.");

			Shader* shader = m_device->create_shader(source.c_str(), type);
			m_shader_cache[path] = shader;
			return shader;
		}
		else
		{
			LOG_INFO("Shader Asset already loaded. Retrieving from cache.");
			return m_shader_cache[path];
		}
	}

	ShaderProgram* Renderer::load_program(std::string& combined_name, uint32_t count, Shader** shaders)
	{
		if (m_program_cache.find(combined_name) == m_program_cache.end())
		{
			LOG_INFO("Shader Program Asset not in cache. Loading from disk.");

			ShaderProgram* program = m_device->create_shader_program(shaders, count);
			m_program_cache[combined_name] = program;

			return program;
		}
		else
		{
			LOG_INFO("Shader Program Asset already loaded. Retrieving from cache.");
			return m_program_cache[combined_name];
		}
	}

	void Renderer::render(Camera* camera, uint16_t w, uint16_t h, Shadows* shadows, Framebuffer* fbo)
	{
		Entity* entities = m_scene->entities();
		int entity_count = m_scene->entity_count();

		m_per_frame_uniforms.projMat = camera->m_projection;
		m_per_frame_uniforms.viewMat = camera->m_view;
		m_per_frame_uniforms.viewProj = camera->m_view_projection;
		m_per_frame_uniforms.viewDir = glm::vec4(camera->m_forward.x, camera->m_forward.y, camera->m_forward.z, 0.0f);
		m_per_frame_uniforms.viewPos = glm::vec4(camera->m_position.x, camera->m_position.y, camera->m_position.z, 0.0f);
		m_per_frame_uniforms.numCascades = shadows->frustum_split_count();
		memcpy(&m_per_frame_uniforms.shadowFrustums[0], shadows->frustum_splits(), sizeof(ShadowFrustum) * m_per_frame_uniforms.numCascades);

		for (int i = 0; i < entity_count; i++)
		{
			Entity& entity = entities[i];
			m_per_entity_uniforms[i].modalMat = entity.m_transform;
			m_per_entity_uniforms[i].mvpMat = camera->m_view_projection * entity.m_transform;
			m_per_entity_uniforms[i].worldPos = glm::vec4(entity.m_position.x, entity.m_position.y, entity.m_position.z, 0.0f);
		}

		void* mem = m_device->map_buffer(m_per_frame, BufferMapType::WRITE);

		if (mem)
		{
			memcpy(mem, &m_per_frame_uniforms, sizeof(PerFrameUniforms));
			m_device->unmap_buffer(m_per_frame);
		}

		mem = m_device->map_buffer(m_per_scene, BufferMapType::WRITE);

		if (mem)
		{
			memcpy(mem, &m_per_scene_uniforms, sizeof(PerSceneUniforms));
			m_device->unmap_buffer(m_per_scene);
		}

		mem = m_device->map_buffer(m_per_entity, BufferMapType::WRITE);

		if (mem)
		{
			memcpy(mem, &m_per_entity_uniforms[0], sizeof(PerEntityUniforms) * entity_count);
			m_device->unmap_buffer(m_per_entity);
		}

		render_shadow_maps(shadows);
		render_scene(w, h, fbo);
		render_atmosphere();
		render_post_process(shadows);
	}

	void Renderer::render_shadow_maps(Shadows* shadows)
	{
		char* ptr = (char*)m_device->map_buffer(m_per_frustum_split, BufferMapType::WRITE);

		for (uint32_t i = 0; i < shadows->frustum_split_count(); i++)
		{
			glm::mat4 crop = shadows->split_view_proj(i);
			memcpy(ptr + (i * 256), &crop, sizeof(glm::mat4));
		}

		m_device->unmap_buffer(m_per_frustum_split);

		ShadowSettings settings = shadows->settings();

		for (uint32_t j = 0; j < shadows->frustum_split_count(); j++)
		{
			m_device->bind_framebuffer(shadows->framebuffers()[j]);
			m_device->set_viewport(settings.shadow_map_size, settings.shadow_map_size, 0, 0);
			m_device->clear_framebuffer(ClearTarget::ALL, (float*)clear_color);

			Entity* entities = m_scene->entities();
			int entity_count = m_scene->entity_count();

			for (int i = 0; i < entity_count; i++)
			{
				Entity& entity = entities[i];

				m_device->bind_shader_program(m_pssm_program);
				m_device->bind_vertex_array(entity.m_mesh->mesh_vertex_array());

				m_device->bind_rasterizer_state(m_standard_rs);
				m_device->bind_depth_stencil_state(m_standard_ds);
				m_device->bind_blend_state(m_standard_bs);

				m_device->bind_uniform_buffer(m_per_frame, ShaderType::VERTEX, 0);
				m_device->bind_uniform_buffer_range(m_per_frustum_split, ShaderType::VERTEX, 2, j * 256, sizeof(PerFrustumSplitUniforms));
				
				dw::SubMesh* submeshes = entity.m_mesh->sub_meshes();

				m_device->set_primitive_type(PrimitiveType::TRIANGLES);

				for (uint32_t j = 0; j < entity.m_mesh->sub_mesh_count(); j++)
				{
					m_device->bind_uniform_buffer_range(m_per_entity, ShaderType::VERTEX, 1, i * sizeof(PerEntityUniforms), sizeof(PerEntityUniforms));
					m_device->draw_indexed_base_vertex(submeshes[j].indexCount, submeshes[j].baseIndex, submeshes[j].baseVertex);
				}
			}
		}
	}

	void Renderer::render_atmosphere()
	{
		m_device->bind_rasterizer_state(m_atmosphere_rs);
		m_device->bind_depth_stencil_state(m_atmosphere_ds);
		m_device->bind_shader_program(m_cube_map_program);
		m_device->bind_uniform_buffer(m_per_frame, ShaderType::VERTEX, 0);
		m_device->bind_sampler_state(m_bilinear_sampler, ShaderType::FRAGMENT, 0);
		m_device->bind_texture(m_scene->env_map(), ShaderType::FRAGMENT, 0);
		m_device->bind_vertex_array(m_cube_vao);
		m_device->set_primitive_type(PrimitiveType::TRIANGLES);
		m_device->draw(0, 36);
	}

	void Renderer::render_scene(uint16_t w, uint16_t h, Framebuffer* fbo)
	{
		m_device->bind_framebuffer(m_color_fbo);
		m_device->set_viewport(w, h, 0, 0);
		m_device->clear_framebuffer(ClearTarget::ALL, (float*)clear_color);

		Entity* entities = m_scene->entities();
		int entity_count = m_scene->entity_count();

		for (int i = 0; i < entity_count; i++)
		{
			Entity& entity = entities[i];

			if (!entity.m_mesh || !entity.m_program)
				continue;

			m_device->bind_shader_program(entity.m_program);

			m_device->bind_rasterizer_state(m_standard_rs);
			m_device->bind_depth_stencil_state(m_standard_ds);
			m_device->bind_blend_state(m_standard_bs);

			m_device->bind_uniform_buffer(m_per_frame, ShaderType::VERTEX, 0);
			m_device->bind_uniform_buffer(m_per_scene, ShaderType::FRAGMENT, 2);

			dw::SubMesh* submeshes = entity.m_mesh->sub_meshes();

			m_device->bind_sampler_state(m_bilinear_sampler, ShaderType::FRAGMENT, 4);
			m_device->bind_texture(m_scene->irradiance_map(), ShaderType::FRAGMENT, 4);

			m_device->bind_sampler_state(m_trilinear_sampler, ShaderType::FRAGMENT, 5);
			m_device->bind_texture(m_scene->prefiltered_map(), ShaderType::FRAGMENT, 5);

			m_device->bind_sampler_state(m_bilinear_sampler, ShaderType::FRAGMENT, 6);
			m_device->bind_texture(m_brdfLUT, ShaderType::FRAGMENT, 6);

			m_device->set_primitive_type(PrimitiveType::TRIANGLES);

			for (uint32_t j = 0; j < entity.m_mesh->sub_mesh_count(); j++)
			{
				dw::Material* mat = submeshes[j].mat;

				if (!mat)
					mat = entity.m_override_mat;

				m_device->bind_vertex_array(entity.m_mesh->mesh_vertex_array());

				if (mat)
				{
					Texture2D* albedo = mat->texture_albedo();

					if (albedo)
					{
						m_device->bind_sampler_state(m_trilinear_sampler, ShaderType::FRAGMENT, 0);
						m_device->bind_texture(albedo, ShaderType::FRAGMENT, 0);
					}

					Texture2D* normal = mat->texture_normal();

					if (normal)
					{
						m_device->bind_sampler_state(m_trilinear_sampler, ShaderType::FRAGMENT, 1);
						m_device->bind_texture(normal, ShaderType::FRAGMENT, 1);
					}

					Texture2D* metalness = mat->texture_metalness();

					if (metalness)
					{
						m_device->bind_sampler_state(m_trilinear_sampler, ShaderType::FRAGMENT, 2);
						m_device->bind_texture(metalness, ShaderType::FRAGMENT, 2);
					}

					Texture2D* roughness = mat->texture_roughness();

					if (roughness)
					{
						m_device->bind_sampler_state(m_trilinear_sampler, ShaderType::FRAGMENT, 3);
						m_device->bind_texture(roughness, ShaderType::FRAGMENT, 3);
					}
				}

				m_device->bind_uniform_buffer_range(m_per_entity, ShaderType::VERTEX, 1, i * sizeof(PerEntityUniforms), sizeof(PerEntityUniforms));
				m_device->draw_indexed_base_vertex(submeshes[j].indexCount, submeshes[j].baseIndex, submeshes[j].baseVertex);
			}
		}
	}

	void Renderer::render_post_process(Shadows* shadows)
	{
		m_device->bind_framebuffer(nullptr);
		m_device->set_viewport(m_width, m_height, 0, 0);
		m_device->clear_framebuffer(ClearTarget::ALL, (float*)clear_color);

		m_device->bind_shader_program(m_quad_program);
		m_device->bind_uniform_buffer(m_per_frame, ShaderType::FRAGMENT, 0);

		m_device->bind_sampler_state(m_bilinear_sampler, ShaderType::FRAGMENT, 0);
		m_device->bind_texture(m_color_buffer, ShaderType::FRAGMENT, 0);

		m_device->bind_sampler_state(m_bilinear_sampler, ShaderType::FRAGMENT, 1);
		m_device->bind_texture(shadows->shadow_map(), ShaderType::FRAGMENT, 1);

		m_device->bind_vertex_array(m_quad_vao);
		m_device->set_primitive_type(PrimitiveType::TRIANGLES);
		m_device->draw(0, 6);
	}
}