#pragma once

#include <string>
#include <stdint.h>
#include <glm.hpp>
#include <unordered_map>
#include <memory>
#include <ogl.h>

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
    static Mesh* load(const std::string& path, bool load_materials = true);
    // Custom factory method for creating a mesh from provided data.
    static Mesh* load(const std::string& name, int num_vertices, Vertex* vertices, int num_indices, uint32_t* indices, int num_sub_meshes, SubMesh* sub_meshes, glm::vec3 max_extents, glm::vec3 min_extents);
    static bool  is_loaded(const std::string& name);
    static void  unload(Mesh*& mesh);

    // Rendering-related getters.
    inline VertexArray* mesh_vertex_array() { return m_vao.get(); }
    inline uint32_t     sub_mesh_count() { return m_sub_mesh_count; }
    inline SubMesh*     sub_meshes() { return m_sub_meshes; }
    inline uint32_t     vertex_count() { return m_vertex_count; }
    inline uint32_t     index_count() { return m_index_count; }
    inline uint32_t*    indices() { return m_indices; }
    inline Vertex*      vertices() { return m_vertices; }

private:
    // Private constructor and destructor to prevent manual creation.
    Mesh();
    Mesh(const std::string& path, bool load_materials);
    ~Mesh();

    // Internal initialization methods.
    void load_from_disk(const std::string& path, bool load_materials);
    void create_gpu_objects();

private:
    // Mesh cache. Used to prevent multiple loads.
    static std::unordered_map<std::string, Mesh*> m_cache;

    // Mesh geometry.
    uint32_t  m_vertex_count   = 0;
    uint32_t  m_index_count    = 0;
    uint32_t  m_sub_mesh_count = 0;
    SubMesh*  m_sub_meshes     = nullptr;
    Vertex*   m_vertices       = nullptr;
    uint32_t* m_indices        = nullptr;
    glm::vec3 m_max_extents;
    glm::vec3 m_min_extents;

    // GPU resources.
    std::unique_ptr<VertexArray>  m_vao = nullptr;
    std::unique_ptr<VertexBuffer> m_vbo = nullptr;
    std::unique_ptr<IndexBuffer>  m_ibo = nullptr;
};
} // namespace dw
