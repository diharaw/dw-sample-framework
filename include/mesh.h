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
		void load_from_disk(const std::string& path);
		void create_gpu_objects();

	public:
		static Mesh* load(const std::string& path, RenderDevice* device, Material* overrideMat = nullptr);
		static void unload(Mesh*& mesh);
		~Mesh();

		inline Material* override_material()	  { return m_override_mat;   }
		inline VertexArray* mesh_vertex_array()	  { return m_vao;			 }
		inline uint32_t sub_mesh_count()		  { return m_sub_mesh_count; }
		inline SubMesh* sub_meshes()			  { return m_sub_meshes;	 }

	private:
		static std::unordered_map<std::string, Mesh*> m_cache;
		uint32_t m_vertex_count;
		uint32_t m_index_count;
		VertexArray * m_vao = nullptr;
		VertexBuffer* m_vbo = nullptr;
		IndexBuffer* m_ibo = nullptr;
		InputLayout* m_il = nullptr;
		Material* m_override_mat = nullptr;
		uint32_t m_sub_mesh_count;
		SubMesh* m_sub_meshes;
		RenderDevice* m_device = nullptr; // Temp
		TSM_Vertex* m_vertices = nullptr;
		uint32_t* m_indices = nullptr;
	};
}
