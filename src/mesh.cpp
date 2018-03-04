#include <mesh.h>
#include <macros.h>
#include <render_device.h>
#include <material.h>
#include <tsm_headers.h>
#include <logger.h>

#include <stdio.h>

namespace dw
{
	std::unordered_map<std::string, Mesh*> Mesh::m_Cache;

	Mesh* Mesh::Load(const std::string& path, RenderDevice* device, Material* overrideMat)
	{
		if (m_Cache.find(path) == m_Cache.end())
		{
			LOG_INFO("Mesh Asset not in cache. Loading from disk.");

			Mesh* mesh = new Mesh(path, device, overrideMat);
			m_Cache[path] = mesh;
			return mesh;
		}
		else
		{
			LOG_INFO("Mesh Asset already loaded. Retrieving from cache.");
			return m_Cache[path];
		}
	}

	void Mesh::Unload(Mesh*& mesh)
	{
		for (auto itr : m_Cache)
		{
			if (itr.second == mesh)
			{
				m_Cache.erase(itr.first);
				DW_SAFE_DELETE(mesh);
				return;
			}
		}
	}

	void Mesh::LoadFromDisk(const std::string & path)
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

		m_SubMeshCount = header.meshCount;
		m_SubMeshes = new SubMesh[m_SubMeshCount];
		m_Vertices = new TSM_Vertex[header.vertexCount];
		m_Indices = new uint32_t[header.indexCount];
		TSM_Material_Json* mats = new TSM_Material_Json[header.materialCount];
		TSM_MeshHeader* meshHeaders = new TSM_MeshHeader[header.meshCount];

		m_VertexCount = header.vertexCount;
		m_IndexCount = header.indexCount;

		memcpy(m_Vertices, data + offset, sizeof(TSM_Vertex) * header.vertexCount);
		offset += sizeof(TSM_Vertex) * header.vertexCount;

		memcpy(m_Indices, data + offset, sizeof(uint32_t) * header.indexCount);
		offset += sizeof(uint32_t) * header.indexCount;

		memcpy(meshHeaders, data + offset, sizeof(TSM_MeshHeader) * header.meshCount);
		offset += sizeof(TSM_MeshHeader) * header.meshCount;

		memcpy(mats, data + offset, sizeof(TSM_Material_Json) * header.materialCount);

		for (uint32_t i = 0; i < header.meshCount; i++)
		{
			m_SubMeshes[i].baseIndex = meshHeaders[i].baseIndex;
			m_SubMeshes[i].baseVertex = meshHeaders[i].baseVertex;
			m_SubMeshes[i].indexCount = meshHeaders[i].indexCount;
			m_SubMeshes[i].maxExtents = meshHeaders[i].maxExtents;
			m_SubMeshes[i].minExtents = meshHeaders[i].minExtents;

			if (header.materialCount > 0 && meshHeaders[i].materialIndex < header.materialCount)
			{
				std::string matName = mats[meshHeaders[i].materialIndex].material;

				if (!matName.empty() && matName != " ")
					m_SubMeshes[i].mat = Material::Load(matName, m_Device);
				else
					m_SubMeshes[i].mat = nullptr;
			}
			else
				m_SubMeshes[i].mat = nullptr;
		}

		delete[] mats;
		delete[] meshHeaders;

		free(data);
		fclose(file);
	}

	void Mesh::CreateGPUObjects()
	{
		BufferCreateDesc bc;
		DW_ZERO_MEMORY(bc);
		bc.data = m_Vertices;
		bc.data_type = DataType::FLOAT;
		bc.size = sizeof(TSM_Vertex) * m_VertexCount;
		bc.usage_type = BufferUsageType::STATIC;

		m_VBO = m_Device->create_vertex_buffer(bc);

		if (!m_VBO)
		{
			LOG_ERROR("Failed to create Vertex Buffer");
		}

		DW_ZERO_MEMORY(bc);
		bc.data = m_Indices;
		bc.data_type = DataType::UINT32;
		bc.size = sizeof(uint32_t) * m_IndexCount;
		bc.usage_type = BufferUsageType::STATIC;

		m_IBO = m_Device->create_index_buffer(bc);

		if (!m_IBO)
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

		m_IL = m_Device->create_input_layout(ilcd);

		VertexArrayCreateDesc vcd;
		DW_ZERO_MEMORY(vcd);
		vcd.index_buffer = m_IBO;
		vcd.vertex_buffer = m_VBO;
		vcd.layout = m_IL;

		m_VAO = m_Device->create_vertex_array(vcd);

		if (!m_VAO)
		{
			LOG_ERROR("Failed to create Vertex Array");
		}
	}

	Mesh::Mesh(const std::string& path, RenderDevice* device, Material* overrideMat) : m_OverrideMat(overrideMat), m_Device(device)
	{
		LoadFromDisk(path);
		CreateGPUObjects();
	}

	Mesh::~Mesh()
	{
		if (m_Device)
		{
			DW_SAFE_DELETE(m_IL);
			m_Device->destroy(m_VAO);
			m_Device->destroy(m_VBO);
			m_Device->destroy(m_IBO);
		}

		for (uint32_t i = 0; i < m_SubMeshCount; i++)
		{
			if (m_SubMeshes[i].mat)
				Material::Unload(m_SubMeshes[i].mat);
		}

		DW_SAFE_DELETE_ARRAY(m_SubMeshes);
		DW_SAFE_DELETE_ARRAY(m_Vertices);
		DW_SAFE_DELETE_ARRAY(m_Indices);
	}
}
