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

    void      build_tlas(vk::CommandBuffer::Ptr cmd_buffer);
    Instance& fetch_instance(const uint32_t& idx);
    int32_t   material_index(const uint32_t& id);
    
    inline uint32_t id() { return m_id; }
    inline glm::vec3 min_extents() { return m_min_extents; }
    inline glm::vec3 max_extents() { return m_max_extents; }
    inline const std::vector<Instance>&   instances() { return m_instances; }
    inline vk::DescriptorSetLayout::Ptr   descriptor_set_layout() { return m_ds_layout; }
    inline vk::DescriptorSet::Ptr         descriptor_set() { return m_ds; }
    inline vk::AccelerationStructure::Ptr acceleration_structure() { return m_tlas; }

private:
    RayTracedScene(vk::Backend::Ptr backend, std::vector<Instance> instances);
    void create_gpu_resources();
    void copy_tlas_data();

private:
    std::weak_ptr<vk::Backend>             m_backend;
    uint32_t                               m_id = 0;
    glm::vec3                              m_min_extents;
    glm::vec3                              m_max_extents;
    vk::DescriptorPool::Ptr                m_descriptor_pool;
    vk::DescriptorSetLayout::Ptr           m_ds_layout;
    vk::DescriptorSet::Ptr                 m_ds;
    vk::Buffer::Ptr                        m_material_data_buffer;
    vk::Buffer::Ptr                        m_instance_data_buffer;
    std::vector<vk::Buffer::Ptr>           m_material_indices_buffers;
    std::vector<Instance>                  m_instances;
    std::vector<std::weak_ptr<Mesh>>       m_meshes;
    bool                                   m_tlas_built = false;
    vk::AccelerationStructure::Ptr         m_tlas;
    vk::Buffer::Ptr                        m_tlas_instance_buffer_host;
    vk::Buffer::Ptr                        m_tlas_instance_buffer_device;
    vk::Buffer::Ptr                        m_tlas_scratch_buffer;
    std::unordered_map<uint32_t, uint32_t> m_local_to_global_mat_idx;
    std::unordered_map<uint32_t, uint32_t> m_local_to_global_texture_idx;
    std::unordered_map<uint32_t, uint32_t> m_local_to_global_mesh_idx;
};
} // namespace dw

#endif
