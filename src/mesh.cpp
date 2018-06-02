#include <mesh.h>
#include <macros.h>
#include <render_device.h>
#include <material.h>
#include <logger.h>
#include <stdio.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace dw
{
	std::unordered_map<std::string, Mesh*> Mesh::m_cache;

	// Assimp texture enum lookup table.
	static const aiTextureType kTextureTypes[] =
	{
		aiTextureType_DIFFUSE,
		aiTextureType_SPECULAR,
		aiTextureType_AMBIENT,
		aiTextureType_EMISSIVE,
		aiTextureType_HEIGHT,
		aiTextureType_NORMALS,
		aiTextureType_SHININESS,
		aiTextureType_OPACITY,
		aiTextureType_DISPLACEMENT,
		aiTextureType_LIGHTMAP,
		aiTextureType_REFLECTION
	};

	// Assimp texture enum string table.
	static std::string kTextureTypeStrings[] =
	{
		"aiTextureType_DIFFUSE",
		"aiTextureType_SPECULAR",
		"aiTextureType_AMBIENT",
		"aiTextureType_EMISSIVE",
		"aiTextureType_HEIGHT",
		"aiTextureType_NORMALS",
		"aiTextureType_SHININESS",
		"aiTextureType_OPACITY",
		"aiTextureType_DISPLACEMENT",
		"aiTextureType_LIGHTMAP",
		"aiTextureType_REFLECTION"
	};

	// -----------------------------------------------------------------------------------------------------------------------------------
	// Assimp loader helper method declarations.
	// -----------------------------------------------------------------------------------------------------------------------------------

	std::string assimp_get_texture_path(aiMaterial* material, aiTextureType texture_type);
	bool assimp_does_material_exist(std::vector<unsigned int> &materials, unsigned int &current_material);

	// -----------------------------------------------------------------------------------------------------------------------------------

	Mesh* Mesh::load(const std::string& path, RenderDevice* device)
	{
		if (m_cache.find(path) == m_cache.end())
		{
			DW_LOG_INFO("Mesh Asset not in cache. Loading from disk.");

			Mesh* mesh = new Mesh(path, device);
			m_cache[path] = mesh;
			return mesh;
		}
		else
		{
			DW_LOG_INFO("Mesh Asset already loaded. Retrieving from cache.");
			return m_cache[path];
		}
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void Mesh::unload(Mesh*& mesh)
	{
		for (auto itr : m_cache)
		{
			if (itr.second == mesh)
			{
				m_cache.erase(itr.first);
				DW_SAFE_DELETE(mesh);
				return;
			}
		}
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void Mesh::load_from_disk(const std::string & path)
	{
		const aiScene* Scene;
		Assimp::Importer importer;
		Scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

		m_sub_mesh_count = Scene->mNumMeshes;
		m_sub_meshes = new SubMesh[m_sub_mesh_count];

		// Temporary variables
		aiMaterial* temp_material;
		uint8_t material_index = 0;
		std::vector<unsigned int> processed_mat_id;
		std::unordered_map<unsigned int, Material*> mat_id_mapping;
		uint32_t unamed_mats = 1;

		// Iterate over submeshes and find materials
		for (int i = 0; i < m_sub_mesh_count; i++)
		{
			bool has_least_one_texture = false;

			m_sub_meshes[i].index_count = Scene->mMeshes[i]->mNumFaces * 3;
			m_sub_meshes[i].base_index = m_index_count;
			m_sub_meshes[i].base_vertex = m_vertex_count;

			m_vertex_count += Scene->mMeshes[i]->mNumVertices;
			m_index_count += m_sub_meshes[i].index_count;

			std::string material_paths[16];

			if (mat_id_mapping.find(Scene->mMeshes[i]->mMaterialIndex) == mat_id_mapping.end())
			{
				std::string current_mat_name;

				temp_material = Scene->mMaterials[Scene->mMeshes[i]->mMaterialIndex];
				current_mat_name = path + std::to_string(i);

				for (uint32_t i = 0; i < 11; i++)
				{
					std::string texture = assimp_get_texture_path(temp_material, kTextureTypes[i]);

					if (texture != "")
					{
						std::replace(texture.begin(), texture.end(), '\\', '/');

						if (texture.length() > 4 && texture[0] != ' ')
						{
							DW_LOG_INFO("Found " + kTextureTypeStrings[i] + ": " + texture);
							material_paths[i] = texture;
							has_least_one_texture = true;
						}
					}
				}

				if (has_least_one_texture)
				{
					m_sub_meshes[i].mat = Material::load(current_mat_name, &material_paths[0], m_device);
					mat_id_mapping[Scene->mMeshes[i]->mMaterialIndex] = m_sub_meshes[i].mat;
				}

			}
			else // if already exists, find the pointer.
				m_sub_meshes[i].mat = mat_id_mapping[Scene->mMeshes[i]->mMaterialIndex];
		}

		m_vertices = new Vertex[m_vertex_count];
		m_indices = new uint32_t[m_index_count];

		aiMesh* temp_mesh;
		int idx = 0;
		int vertexIndex = 0;

		// Iterate over submeshes...
		for (int i = 0; i < m_sub_mesh_count; i++)
		{
			temp_mesh = Scene->mMeshes[i];
			m_sub_meshes[i].max_extents = glm::vec3(temp_mesh->mVertices[0].x, temp_mesh->mVertices[0].y, temp_mesh->mVertices[0].z);
			m_sub_meshes[i].min_extents = glm::vec3(temp_mesh->mVertices[0].x, temp_mesh->mVertices[0].y, temp_mesh->mVertices[0].z);

			// Iterate over vertices in submesh...
			for (int k = 0; k < Scene->mMeshes[i]->mNumVertices; k++)
			{
				// Assign vertex values.
				m_vertices[vertexIndex].position = glm::vec3(temp_mesh->mVertices[k].x, temp_mesh->mVertices[k].y, temp_mesh->mVertices[k].z);
				glm::vec3 n = glm::vec3(temp_mesh->mNormals[k].x, temp_mesh->mNormals[k].y, temp_mesh->mNormals[k].z);
				glm::vec3 t = glm::vec3(temp_mesh->mTangents[k].x, temp_mesh->mTangents[k].y, temp_mesh->mTangents[k].z);
				glm::vec3 b = glm::vec3(temp_mesh->mBitangents[k].x, temp_mesh->mBitangents[k].y, temp_mesh->mBitangents[k].z);

				// Assuming right handed coordinate space
				if (glm::dot(glm::cross(n, t), b) < 0.0f)
					t *= -1.0f; // Flip tangent

				m_vertices[vertexIndex].normal = n;
				m_vertices[vertexIndex].tangent = t;

				// Find submesh bounding box extents.
				if (m_vertices[vertexIndex].position.x > m_sub_meshes[i].max_extents.x)
					m_sub_meshes[i].max_extents.x = m_vertices[vertexIndex].position.x;
				if (m_vertices[vertexIndex].position.y > m_sub_meshes[i].max_extents.y)
					m_sub_meshes[i].max_extents.y = m_vertices[vertexIndex].position.y;
				if (m_vertices[vertexIndex].position.z > m_sub_meshes[i].max_extents.z)
					m_sub_meshes[i].max_extents.z = m_vertices[vertexIndex].position.z;

				if (m_vertices[vertexIndex].position.x < m_sub_meshes[i].min_extents.x)
					m_sub_meshes[i].min_extents.x = m_vertices[vertexIndex].position.x;
				if (m_vertices[vertexIndex].position.y < m_sub_meshes[i].min_extents.y)
					m_sub_meshes[i].min_extents.y = m_vertices[vertexIndex].position.y;
				if (m_vertices[vertexIndex].position.z < m_sub_meshes[i].min_extents.z)
					m_sub_meshes[i].min_extents.z = m_vertices[vertexIndex].position.z;

				// Assign texture coordinates if it has any. Only the first channel is considered.
				if (temp_mesh->HasTextureCoords(0))
					m_vertices[vertexIndex].tex_coord = glm::vec2(temp_mesh->mTextureCoords[0][k].x, temp_mesh->mTextureCoords[0][k].y);

				vertexIndex++;
			}

			// Assign indices.
			for (int j = 0; j < temp_mesh->mNumFaces; j++)
			{
				m_indices[idx] = temp_mesh->mFaces[j].mIndices[0];
				idx++;
				m_indices[idx] = temp_mesh->mFaces[j].mIndices[1];
				idx++;
				m_indices[idx] = temp_mesh->mFaces[j].mIndices[2];
				idx++;
			}
		}

		m_max_extents = m_sub_meshes[0].max_extents;
		m_min_extents = m_sub_meshes[0].min_extents;

		// Find bounding box extents of entire mesh.
		for (int i = 0; i < m_sub_mesh_count; i++)
		{
			if (m_sub_meshes[i].max_extents.x > m_max_extents.x)
				m_max_extents.x = m_sub_meshes[i].max_extents.x;
			if (m_sub_meshes[i].max_extents.y > m_max_extents.y)
				m_max_extents.y = m_sub_meshes[i].max_extents.y;
			if (m_sub_meshes[i].max_extents.z > m_max_extents.z)
				m_max_extents.z = m_sub_meshes[i].max_extents.z;

			if (m_sub_meshes[i].min_extents.x <m_min_extents.x)
				m_min_extents.x = m_sub_meshes[i].min_extents.x;
			if (m_sub_meshes[i].min_extents.y < m_min_extents.y)
				m_min_extents.y = m_sub_meshes[i].min_extents.y;
			if (m_sub_meshes[i].min_extents.z < m_min_extents.z)
				m_min_extents.z = m_sub_meshes[i].min_extents.z;
		}
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void Mesh::create_gpu_objects()
	{
		// Create vertex buffer.
		BufferCreateDesc bc;
		DW_ZERO_MEMORY(bc);
		bc.data = m_vertices;
		bc.data_type = DataType::FLOAT;
		bc.size = sizeof(Vertex) * m_vertex_count;
		bc.usage_type = BufferUsageType::STATIC;

		m_vbo = m_device->create_vertex_buffer(bc);

		if (!m_vbo)
			DW_LOG_ERROR("Failed to create Vertex Buffer");

		// Create index buffer.
		DW_ZERO_MEMORY(bc);
		bc.data = m_indices;
		bc.data_type = DataType::UINT32;
		bc.size = sizeof(uint32_t) * m_index_count;
		bc.usage_type = BufferUsageType::STATIC;

		m_ibo = m_device->create_index_buffer(bc);

		if (!m_ibo)
			DW_LOG_ERROR("Failed to create Index Buffer");

		// Declare vertex attributes.
		InputElement elements[] =
		{
			{ 3, DataType::FLOAT, false, 0, "POSITION" },
			{ 2, DataType::FLOAT, false, offsetof(Vertex, tex_coord), "TEXCOORD" },
			{ 3, DataType::FLOAT, false, offsetof(Vertex, normal), "NORMAL" },
			{ 3, DataType::FLOAT, false, offsetof(Vertex, tangent), "TANGENT" },
			{ 3, DataType::FLOAT, false, offsetof(Vertex, bitangent), "BITANGENT" }
		};

		// Create input layout.
		InputLayoutCreateDesc ilcd;
		DW_ZERO_MEMORY(ilcd);
		ilcd.elements = elements;
		ilcd.num_elements = 4;
		ilcd.vertex_size = sizeof(Vertex);

		m_il = m_device->create_input_layout(ilcd);

		// Create vertex array.
		VertexArrayCreateDesc vcd;
		DW_ZERO_MEMORY(vcd);
		vcd.index_buffer = m_ibo;
		vcd.vertex_buffer = m_vbo;
		vcd.layout = m_il;

		m_vao = m_device->create_vertex_array(vcd);

		if (!m_vao)
			DW_LOG_ERROR("Failed to create Vertex Array");
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	Mesh::Mesh(const std::string& path, RenderDevice* device) : m_device(device)
	{
		load_from_disk(path);
		create_gpu_objects();
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	Mesh::~Mesh()
	{
		// Free GPU resources.
		if (m_device)
		{
			DW_SAFE_DELETE(m_il);
			m_device->destroy(m_vao);
			m_device->destroy(m_vbo);
			m_device->destroy(m_ibo);
		}

		// Unload submesh materials.
		for (uint32_t i = 0; i < m_sub_mesh_count; i++)
		{
			if (m_sub_meshes[i].mat)
				Material::unload(m_sub_meshes[i].mat);
		}

		// Delete geometry data.
		DW_SAFE_DELETE_ARRAY(m_sub_meshes);
		DW_SAFE_DELETE_ARRAY(m_vertices);
		DW_SAFE_DELETE_ARRAY(m_indices);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------
	// Assimp loader helper method definitions
	// -----------------------------------------------------------------------------------------------------------------------------------

	std::string assimp_get_texture_path(aiMaterial* material, aiTextureType texture_type)
	{
		aiString path;
		aiReturn result = material->GetTexture(texture_type, 0, &path);

		if (result == aiReturn_FAILURE)
			return "";
		else
		{
			std::string cppStr = std::string(path.C_Str());

			if (cppStr == "")
				return "";

			return cppStr;
		}
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	bool assimp_does_material_exist(std::vector<unsigned int> &materials, unsigned int &current_material)
	{
		for (auto it : materials)
		{
			if (it == current_material)
				return true;
		}

		return false;
	}
} // namespace dw
