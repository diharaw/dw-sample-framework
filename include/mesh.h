#pragma once

#include <string>
#include <stdint.h>
#include <glm.hpp>
#include <unordered_map>
#include <memory>
#include <ogl.h>
#include <vk.h>

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
    static bool is_loaded(const std::string& name);
    static void unload(Mesh*& mesh);

    // Static factory methods.
    static Mesh* load(
#if defined(DWSF_VULKAN)
        vk::Backend::Ptr backend,
#endif
        const std::string& path,
        bool               load_materials = true);
    // Custom factory method for creating a mesh from provided data.
    static Mesh* load(
#if defined(DWSF_VULKAN)
        vk::Backend::Ptr backend,
#endif
        const std::string& name,
        int                num_vertices,
        Vertex*            vertices,
        int                num_indices,
        uint32_t*          indices,
        int                num_sub_meshes,
        SubMesh*           sub_meshes,
        glm::vec3          max_extents,
        glm::vec3          min_extents);

#if defined(DWSF_VULKAN)
    void initialize_for_ray_tracing(vk::Backend::Ptr backend);

    // Rendering-related getters.
    inline vk::Buffer::Ptr vertex_buffer()
    {
        return m_vbo;
    }
    inline vk::Buffer::Ptr                  index_buffer() { return m_ibo; }
    inline const vk::VertexInputStateDesc&  vertex_input_state_desc() { return m_vertex_input_state_desc; }
    inline const std::vector<VkGeometryNV>& ray_tracing_geometry() { return m_rt_geometries; }
    inline vk::AccelerationStructure::Ptr   acceleration_structure() { return m_rt_as; }
#else
    inline gl::VertexArray* mesh_vertex_array()
    {
        return m_vao.get();
    }
#endif

    inline uint32_t sub_mesh_count()
    {
        return m_sub_mesh_count;
    }
    inline SubMesh*  sub_meshes() { return m_sub_meshes; }
    inline uint32_t  vertex_count() { return m_vertex_count; }
    inline uint32_t  index_count() { return m_index_count; }
    inline uint32_t* indices() { return m_indices; }
    inline Vertex*   vertices() { return m_vertices; }

private:
    // Private constructor and destructor to prevent manual creation.
    Mesh();
    Mesh(
#if defined(DWSF_VULKAN)
        vk::Backend::Ptr backend,
#endif
        const std::string& path,
        bool               load_materials);

    // Internal initialization methods.
    void create_gpu_objects(
#if defined(DWSF_VULKAN)
        vk::Backend::Ptr backend
#endif
    );

    void load_from_disk(
#if defined(DWSF_VULKAN)
        vk::Backend::Ptr backend,
#endif
        const std::string& path,
        bool               load_materials);

    ~Mesh();

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
#if defined(DWSF_VULKAN)
    std::vector<VkGeometryNV>      m_rt_geometries;
    vk::AccelerationStructure::Ptr m_rt_as;
    vk::Buffer::Ptr                m_vbo;
    vk::Buffer::Ptr                m_ibo;
    vk::VertexInputStateDesc       m_vertex_input_state_desc;
#else
    std::unique_ptr<gl::VertexArray>  m_vao = nullptr;
    std::unique_ptr<gl::VertexBuffer> m_vbo = nullptr;
    std::unique_ptr<gl::IndexBuffer>  m_ibo = nullptr;
#endif
};
} // namespace dw
