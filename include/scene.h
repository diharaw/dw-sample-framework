#pragma once

#include <mesh.h>

namespace dw
{
class Scene
{
public:
    Scene();
    ~Scene();

    void add_instance(dw::Mesh* mesh, glm::mat4 transform);

#if defined(DWSF_VULKAN)
    void build_acceleration_structure(vk::Backend::Ptr backend);
#endif

private:
#if defined(DWSF_VULKAN)
    std::vector<vk::GeometryInstanceNV> m_instances;
    std::vector<dw::Mesh*>              m_meshes;
    vk::AccelerationStructure::Ptr      m_vk_top_level_as;
#endif
};
} // namespace dw
