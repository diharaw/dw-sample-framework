#pragma once

#include <mesh.h>
#include <unordered_set>

#if defined(DWSF_VULKAN)

namespace dw
{

class RayTracedScene
{
public:
    using Ptr = std::shared_ptr<RayTracedScene>;

    struct Instance
    {
        glm::mat4           transform;
        std::weak_ptr<Mesh> mesh;
    };

    static RayTracedScene::Ptr create(vk::Backend::Ptr backend, std::vector<Instance> instances);

    ~RayTracedScene();

    void rebuild();
    Instance& fetch_instance(const uint32_t& idx);
    uint32_t local_to_global_material_idx(Mesh::Ptr mesh, uint32_t local_mat_idx);

    inline vk::DescriptorSetLayout::Ptr   descriptor_set_layout() { return m_ds_layout; }
    inline vk::DescriptorSet::Ptr   descriptor_set() { return m_ds; }
    inline vk::AccelerationStructure::Ptr acceleration_structure() { return m_vk_top_level_as; }

private:
    RayTracedScene(vk::Backend::Ptr backend, std::vector<Instance> instances);
    void gather_instance_data();
    void create_descriptor_sets();
    void build_acceleration_structure();

private:
    std::weak_ptr<vk::Backend>       m_backend;
    vk::DescriptorSetLayout::Ptr     m_ds_layout;
    vk::DescriptorSet::Ptr           m_ds;
    vk::Buffer::Ptr                  m_material_data_buffer;
    vk::Buffer::Ptr                  m_instance_data_buffer;
    std::vector<vk::Buffer::Ptr>     m_material_indices_buffers;
    std::vector<Instance>            m_instances;
    std::vector<std::weak_ptr<Mesh>> m_meshes;
    vk::AccelerationStructure::Ptr   m_vk_top_level_as;
    std::unordered_map<uint32_t, std::unordered_map<uint32_t, uint32_t>> m_local_to_global_mat_idx;
    std::unordered_map<uint32_t, uint32_t>                               m_local_to_global_texture_idx;
    std::unordered_map<uint32_t, uint32_t>                               m_local_to_global_mesh_idx;
};
} // namespace dw

#endif
