#pragma once

#include <mesh.h>
#include <unordered_set>

namespace dw
{
struct RTGeometryInstance
{
    glm::mat3x4 transform;
    uint32_t    instanceCustomIndex : 24;
    uint32_t    mask : 8;
    uint32_t    instanceOffset : 24;
    uint32_t    flags : 8;
    uint64_t    accelerationStructureHandle;
};

struct Instance
{
    glm::mat4           transform;
    std::weak_ptr<Mesh> mesh;
};

class Scene
{
public:
    using Ptr = std::shared_ptr<Scene>;

    static Scene::Ptr create();

    ~Scene();

    void add_instance(dw::Mesh::Ptr mesh, glm::mat4 transform);

#if defined(DWSF_VULKAN)
    void initialize_for_indirect_draw(vk::Backend::Ptr backend);
    void initialize_for_ray_tracing(vk::Backend::Ptr backend);

    inline vk::DescriptorSetLayout::Ptr   ray_tracing_geometry_descriptor_set_layout() { return m_ray_tracing_geometry_ds_layout; }
    inline vk::DescriptorSetLayout::Ptr   indirect_draw_geometry_descriptor_set_layout() { return m_indirect_draw_geometry_ds_layout; }
    inline vk::DescriptorSetLayout::Ptr   material_descriptor_set_layout() { return m_material_ds_layout; }
    inline vk::DescriptorSet::Ptr         ray_tracing_geometry_descriptor_set() { return m_ray_tracing_geometry_ds; }
    inline vk::DescriptorSet::Ptr         indirect_draw_geometry_descriptor_set() { return m_indirect_draw_geometry_ds; }
    inline vk::DescriptorSet::Ptr         albedo_descriptor_set() { return m_albedo_ds; }
    inline vk::DescriptorSet::Ptr         normal_descriptor_set() { return m_normal_ds; }
    inline vk::DescriptorSet::Ptr         roughness_descriptor_set() { return m_roughness_ds; }
    inline vk::DescriptorSet::Ptr         metallic_descriptor_set() { return m_metallic_ds; }
    inline vk::AccelerationStructure::Ptr acceleration_structure() { return m_vk_top_level_as; }
#endif

private:
    Scene();

#if defined(DWSF_VULKAN)
    void gather_instance_data(vk::Backend::Ptr backend, bool ray_tracing);
    void create_descriptor_sets(vk::Backend::Ptr backend, bool ray_tracing);
    void build_acceleration_structure(vk::Backend::Ptr backend);
#endif

private:
#if defined(DWSF_VULKAN)
    vk::DescriptorSetLayout::Ptr     m_material_ds_layout;
    vk::DescriptorSetLayout::Ptr     m_ray_tracing_geometry_ds_layout;
    vk::DescriptorSetLayout::Ptr     m_indirect_draw_geometry_ds_layout;
    vk::DescriptorSet::Ptr           m_albedo_ds;
    vk::DescriptorSet::Ptr           m_normal_ds;
    vk::DescriptorSet::Ptr           m_roughness_ds;
    vk::DescriptorSet::Ptr           m_metallic_ds;
    vk::DescriptorSet::Ptr           m_ray_tracing_geometry_ds;
    vk::DescriptorSet::Ptr           m_indirect_draw_geometry_ds;
    std::vector<vk::Buffer::Ptr>     m_material_buffers;
    std::vector<RTGeometryInstance>  m_rt_instances;
    std::vector<std::weak_ptr<Mesh>> m_meshes;
    vk::AccelerationStructure::Ptr   m_vk_top_level_as;
#endif

    std::vector<Instance> m_instances;
};
} // namespace dw
