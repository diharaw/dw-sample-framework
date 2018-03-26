#include <mesh.h>
#include <macros.h>
#include <render_device.h>
#include <material.h>
#include <tsm_headers.h>
#include <logger.h>

#include <stdio.h>

namespace dw
{
	std::unordered_map<std::string, Mesh*> Mesh::m_cache;

	Mesh* Mesh::load(const std::string& path, RenderDevice* device)
	{
		if (m_cache.find(path) == m_cache.end())
		{
			LOG_INFO("Mesh Asset not in cache. Loading from disk.");

			Mesh* mesh = new Mesh(path, device);
			m_cache[path] = mesh;
			return mesh;
		}
		else
		{
			LOG_INFO("Mesh Asset already loaded. Retrieving from cache.");
			return m_cache[path];
		}
	}

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

	void Mesh::load_from_disk(const std::string & path)
	{
		char* data = nullptr;
		FILE *file = fopen(path.c_str(), "rb");
		fseek(file, 0, SEEK_END);
		long len = ftell(file);
		data = (char*)malloc(len);
		rewind(file);
		fread(data, len, 1, file);

		size_t offset = 0;

		TSM_FileHeader header;

		memcpy(&header, data, sizeof(TSM_FileHeader));
		offset += sizeof(TSM_FileHeader);

		m_sub_mesh_count = header.meshCount;
		m_sub_meshes = new SubMesh[m_sub_mesh_count];
		m_vertices = new TSM_Vertex[header.vertexCount];
		m_indices = new uint32_t[header.indexCount];
		TSM_Material_Json* mats = new TSM_Material_Json[header.materialCount];
		TSM_MeshHeader* meshHeaders = new TSM_MeshHeader[header.meshCount];

		m_vertex_count = header.vertexCount;
		m_index_count = header.indexCount;

		memcpy(m_vertices, data + offset, sizeof(TSM_Vertex) * header.vertexCount);
		offset += sizeof(TSM_Vertex) * header.vertexCount;

		memcpy(m_indices, data + offset, sizeof(uint32_t) * header.indexCount);
		offset += sizeof(uint32_t) * header.indexCount;

		memcpy(meshHeaders, data + offset, sizeof(TSM_MeshHeader) * header.meshCount);
		offset += sizeof(TSM_MeshHeader) * header.meshCount;

		memcpy(mats, data + offset, sizeof(TSM_Material_Json) * header.materialCount);

		for (uint32_t i = 0; i < header.meshCount; i++)
		{
			m_sub_meshes[i].baseIndex = meshHeaders[i].baseIndex;
			m_sub_meshes[i].baseVertex = meshHeaders[i].baseVertex;
			m_sub_meshes[i].indexCount = meshHeaders[i].indexCount;
			m_sub_meshes[i].maxExtents = meshHeaders[i].maxExtents;
			m_sub_meshes[i].minExtents = meshHeaders[i].minExtents;

			if (header.materialCount > 0 && meshHeaders[i].materialIndex < header.materialCount)
			{
				std::string matName = mats[meshHeaders[i].materialIndex].material;

				if (!matName.empty() && matName != " ")
					m_sub_meshes[i].mat = Material::load(matName, m_device);
				else
					m_sub_meshes[i].mat = nullptr;
			}
			else
				m_sub_meshes[i].mat = nullptr;
		}

		delete[] mats;
		delete[] meshHeaders;

		free(data);
		fclose(file);
	}

	void Mesh::create_gpu_objects()
	{
		BufferCreateDesc bc;
		DW_ZERO_MEMORY(bc);
		bc.data = m_vertices;
		bc.data_type = DataType::FLOAT;
		bc.size = sizeof(TSM_Vertex) * m_vertex_count;
		bc.usage_type = BufferUsageType::STATIC;

		m_vbo = m_device->create_vertex_buffer(bc);

		if (!m_vbo)
		{
			LOG_ERROR("Failed to create Vertex Buffer");
		}

		DW_ZERO_MEMORY(bc);
		bc.data = m_indices;
		bc.data_type = DataType::UINT32;
		bc.size = sizeof(uint32_t) * m_index_count;
		bc.usage_type = BufferUsageType::STATIC;

		m_ibo = m_device->create_index_buffer(bc);

		if (!m_ibo)
		{
			LOG_ERROR("Failed to create Index Buffer");
		}

		InputElement elements[] =
		{
			{ 3, DataType::FLOAT, false, 0, "POSITION" },
			{ 2, DataType::FLOAT, false, offsetof(TSM_Vertex, texCoord), "TEXCOORD" },
			{ 3, DataType::FLOAT, false, offsetof(TSM_Vertex, normal), "NORMAL" },
			{ 4, DataType::FLOAT, false, offsetof(TSM_Vertex, tangent), "TANGENT" }
		};

		InputLayoutCreateDesc ilcd;
		DW_ZERO_MEMORY(ilcd);
		ilcd.elements = elements;
		ilcd.num_elements = 4;
		ilcd.vertex_size = sizeof(TSM_Vertex);

		m_il = m_device->create_input_layout(ilcd);

		VertexArrayCreateDesc vcd;
		DW_ZERO_MEMORY(vcd);
		vcd.index_buffer = m_ibo;
		vcd.vertex_buffer = m_vbo;
		vcd.layout = m_il;

		m_vao = m_device->create_vertex_array(vcd);

		if (!m_vao)
		{
			LOG_ERROR("Failed to create Vertex Array");
		}
	}

	Mesh::Mesh(const std::string& path, RenderDevice* device) : m_device(device)
	{
		load_from_disk(path);
		create_gpu_objects();
	}

	Mesh::~Mesh()
	{
		if (m_device)
		{
			DW_SAFE_DELETE(m_il);
			m_device->destroy(m_vao);
			m_device->destroy(m_vbo);
			m_device->destroy(m_ibo);
		}

		for (uint32_t i = 0; i < m_sub_mesh_count; i++)
		{
			if (m_sub_meshes[i].mat)
				Material::unload(m_sub_meshes[i].mat);
		}

		DW_SAFE_DELETE_ARRAY(m_sub_meshes);
		DW_SAFE_DELETE_ARRAY(m_vertices);
		DW_SAFE_DELETE_ARRAY(m_indices);
	}
}
