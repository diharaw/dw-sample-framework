#pragma once

#include <string>
#include <stdint.h>
#include <glm.hpp>
#include <unordered_map>

// Forward declarations.
class  RenderDevice;
struct VertexArray;
struct VertexBuffer;
struct IndexBuffer;
struct InputLayout;

namespace dw
{
	class Material;

	// Non-skeletal vertex structure. 
	struct Vertex
	{
		glm::vec3 position;
		glm::vec2 tex_coord;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec3 bitangent;
	};

	// SubMesh structure. Currently limited to one Material.
	struct SubMesh
	{
		Material* mat;
		uint32_t  index_count;
		uint32_t  base_vertex;
		uint32_t  base_index;
		glm::vec3 max_extents;
		glm::vec3 min_extents;
	};

	class Mesh
	{
	public:
		// Static factory methods.
		static Mesh* load(const std::string& path, RenderDevice* device, bool load_materials = true);
		static void unload(Mesh*& mesh);

		// Rendering-related getters.
		inline VertexArray* mesh_vertex_array()	{ return m_vao;			 }
		inline uint32_t sub_mesh_count()		{ return m_sub_mesh_count; }
		inline SubMesh* sub_meshes()			{ return m_sub_meshes;	 }

	private:
		// Private constructor and destructor to prevent manual creation.
		Mesh(const std::string& path, RenderDevice* device, bool load_materials);
		~Mesh();

		// Internal initialization methods.
		void load_from_disk(const std::string& path, bool load_materials);
		void create_gpu_objects();

	private:
		// Mesh cache. Used to prevent multiple loads.
		static std::unordered_map<std::string, Mesh*> m_cache;

		// Mesh geometry.
		uint32_t m_vertex_count = 0;
		uint32_t m_index_count = 0;
		uint32_t m_sub_mesh_count = 0;
		SubMesh* m_sub_meshes = nullptr;
		Vertex* m_vertices = nullptr;
		uint32_t* m_indices = nullptr;
		glm::vec3 m_max_extents;
		glm::vec3 m_min_extents;

		// GPU resources.
		VertexArray * m_vao = nullptr;
		VertexBuffer* m_vbo = nullptr;
		IndexBuffer* m_ibo = nullptr;
		InputLayout* m_il = nullptr;

		// Cached RenderDevice pointer used in destructor for GPU resource clean up.
		RenderDevice* m_device = nullptr;
	};
} // namespace dw
