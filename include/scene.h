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
    void build_acceleration_structure(vk::Backend::Ptr backend);
#endif

private:
    Scene();

private:
#if defined(DWSF_VULKAN)
    std::vector<vk::GeometryInstanceNV> m_instances;
    std::vector<std::weak_ptr<dw::Mesh>> m_meshes;
    vk::AccelerationStructure::Ptr      m_vk_top_level_as;
#endif
};
} // namespace dw
