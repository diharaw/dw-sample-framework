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
    glm::vec4 position;
    glm::vec4 tex_coord;
    glm::vec4 normal;
    glm::vec4 tangent;
    glm::vec4 bitangent;
};

// SubMesh structure. Currently limited to one Material.
struct SubMesh
{
    std::string name;
    uint32_t    mat_idx;
    uint32_t    index_count;
    uint32_t    base_vertex;
    uint32_t    base_index;
    uint32_t    vertex_count;
    glm::vec3   max_extents;
    glm::vec3   min_extents;
};

class Mesh
{
public:
    using Ptr = std::shared_ptr<Mesh>;

    static bool is_loaded(const std::string& name);

    // Static factory methods.
    static Mesh::Ptr load(
#if defined(DWSF_VULKAN)
        vk::Backend::Ptr backend,
#endif
        const std::string& path,
        bool               load_materials = true,
        bool               is_orca_mesh   = false);
    // Custom factory method for creating a mesh from provided data.
    static Mesh::Ptr load(
#if defined(DWSF_VULKAN)
        vk::Backend::Ptr backend,
#endif
        const std::string&                     name,
        std::vector<Vertex>                    vertices,
        std::vector<uint32_t>                  indices,
        std::vector<SubMesh>                   sub_meshes,
        std::vector<std::shared_ptr<Material>> materials,
        glm::vec3                              max_extents,
        glm::vec3                              min_extents);

    bool set_submesh_material(std::string name, std::shared_ptr<Material> material);
    bool set_submesh_material(uint32_t mesh_idx, std::shared_ptr<Material> material);
    void set_global_material(std::shared_ptr<Material> material);

#if defined(DWSF_VULKAN)
    void initialize_for_ray_tracing(vk::Backend::Ptr backend);

    // Rendering-related getters.
    inline vk::Buffer::Ptr                 vertex_buffer() { return m_vbo; }
    inline vk::Buffer::Ptr                 index_buffer() { return m_ibo; }
    inline const vk::VertexInputStateDesc& vertex_input_state_desc() { return m_vertex_input_state_desc; }
    inline vk::AccelerationStructure::Ptr  acceleration_structure() { return m_blas; }
#else
    inline gl::Buffer::Ptr vertex_buffer()
    {
        return m_vbo;
    }
    inline gl::Buffer::Ptr  index_buffer() { return m_ibo; }
    inline gl::VertexArray* mesh_vertex_array()
    {
        return m_vao.get();
    }
#endif

    inline uint32_t id()
    {
        return m_id;
    }
    inline const std::vector<std::shared_ptr<Material>>& materials() { return m_materials; }
    inline const std::vector<SubMesh>&                   sub_meshes() { return m_sub_meshes; }
    inline const std::vector<uint32_t>&                  indices() { return m_indices; }
    inline const std::vector<Vertex>&                    vertices() { return m_vertices; }
    inline std::shared_ptr<Material>&                    material(uint32_t idx) { return m_materials[idx]; }
    inline const glm::vec3&                              max_extents() { return m_max_extents; }
    inline const glm::vec3&                              min_extents() { return m_min_extents; }
    ~Mesh();

private:
    // Private constructor to prevent manual creation.
    Mesh();
    Mesh(
#if defined(DWSF_VULKAN)
        vk::Backend::Ptr backend,
#endif
        const std::string& path,
        bool               load_materials,
        bool               is_orca_mesh);

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
        bool               load_materials,
        bool               is_orca_mesh);

private:
    // Mesh cache. Used to prevent multiple loads.
    static std::unordered_map<std::string, std::weak_ptr<Mesh>> m_cache;

    // Mesh geometry.
    uint32_t                               m_id = 0;
    std::vector<std::shared_ptr<Material>> m_materials;
    std::vector<Vertex>                    m_vertices;
    std::vector<uint32_t>                  m_indices;
    std::vector<SubMesh>                   m_sub_meshes;
    glm::vec3                              m_max_extents;
    glm::vec3                              m_min_extents;

    // GPU resources.
#if defined(DWSF_VULKAN)
    vk::AccelerationStructure::Ptr       m_blas;
    VkAccelerationStructureCreateInfoKHR m_blas_info;
    vk::Buffer::Ptr                      m_vbo;
    vk::Buffer::Ptr                      m_ibo;
    vk::VertexInputStateDesc             m_vertex_input_state_desc;
#else
    gl::VertexArray::Ptr m_vao = nullptr;
    gl::Buffer::Ptr      m_vbo = nullptr;
    gl::Buffer::Ptr      m_ibo = nullptr;
#endif
};
} // namespace dw
