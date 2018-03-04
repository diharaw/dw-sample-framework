#pragma once

#include <string>
#include <stdint.h>
#include <glm.hpp>
#include <unordered_map>

#include "tsm_vertex.h"

class  RenderDevice;
struct VertexArray;
struct VertexBuffer;
struct IndexBuffer;
struct InputLayout;

namespace dw
{
	class Material;

	struct SubMesh
	{
		Material* mat;
		uint32_t  indexCount;
		uint32_t  baseVertex;
		uint32_t  baseIndex;
		glm::vec3 maxExtents;
		glm::vec3 minExtents;
	};

	class Mesh
	{
	private:
		Mesh(const std::string& path, RenderDevice* device, Material* overrideMat = nullptr);
		void LoadFromDisk(const std::string& path);
		void CreateGPUObjects();

	public:
		static Mesh* Load(const std::string& path, RenderDevice* device, Material* overrideMat = nullptr);
		static void Unload(Mesh*& mesh);
		~Mesh();

		inline Material* OverrideMaterial()	  { return m_OverrideMat;  }
		inline VertexArray* MeshVertexArray() { return m_VAO; }
		inline uint32_t SubMeshCount()		  { return m_SubMeshCount; }
		inline SubMesh* SubMeshes()			  { return m_SubMeshes;	 }

	private:
		static std::unordered_map<std::string, Mesh*> m_Cache;
		uint32_t m_VertexCount;
		uint32_t m_IndexCount;
		VertexArray * m_VAO = nullptr;
		VertexBuffer* m_VBO = nullptr;
		IndexBuffer* m_IBO = nullptr;
		InputLayout* m_IL = nullptr;
		Material* m_OverrideMat = nullptr;
		uint32_t m_SubMeshCount;
		SubMesh* m_SubMeshes;
		RenderDevice* m_Device = nullptr; // Temp
		TSM_Vertex* m_Vertices = nullptr;
		uint32_t* m_Indices = nullptr;
	};
}
