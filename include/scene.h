#pragma once

#include <mesh.h>

namespace dw
{
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
    inline vk::DescriptorSet::Ptr         material_descriptor_set() { return m_material_ds; }
    inline vk::AccelerationStructure::Ptr acceleration_structure() { return m_vk_top_level_as; }
#endif

private:
    Scene();
    void build_acceleration_structure(vk::Backend::Ptr backend);

private:
#if defined(DWSF_VULKAN)
    vk::DescriptorSetLayout::Ptr         m_material_ds_layout;
    vk::DescriptorSetLayout::Ptr         m_ray_tracing_geometry_ds_layout;
    vk::DescriptorSetLayout::Ptr         m_indirect_draw_geometry_ds_layout;
    vk::DescriptorSet::Ptr               m_material_ds;
    vk::DescriptorSet::Ptr               m_ray_tracing_geometry_ds;
    vk::DescriptorSet::Ptr               m_indirect_draw_geometry_ds;
    std::vector<vk::GeometryInstanceNV>  m_instances;
    std::vector<std::weak_ptr<dw::Mesh>> m_meshes;
    vk::AccelerationStructure::Ptr       m_vk_top_level_as;
#endif
};
} // namespace dw
