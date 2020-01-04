#include <scene.h>
#include <macros.h>
#include <vk_mem_alloc.h>

namespace dw
{
// -----------------------------------------------------------------------------------------------------------------------------------

Scene::Ptr Scene::create()
{
    return std::shared_ptr<Scene>(new Scene());
}

// -----------------------------------------------------------------------------------------------------------------------------------

Scene::Scene()
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

Scene::~Scene()
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Scene::add_instance(dw::Mesh::Ptr mesh, glm::mat4 transform)
{
    const float xform[12] = {
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
    };

    vk::GeometryInstanceNV instance;

    std::memcpy(instance.transform, xform, sizeof(xform));

    instance.instanceCustomIndex         = 0;
    instance.mask                        = 0xff;
    instance.instanceOffset              = 0;
    instance.flags                       = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
    instance.accelerationStructureHandle = mesh->acceleration_structure()->opaque_handle();

    m_meshes.push_back(mesh);
    m_instances.push_back(instance);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Scene::initialize_for_indirect_draw(vk::Backend::Ptr backend)
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Scene::initialize_for_ray_tracing(vk::Backend::Ptr backend)
{
    build_acceleration_structure(backend);
}

// -----------------------------------------------------------------------------------------------------------------------------------

#if defined(DWSF_VULKAN)

void Scene::build_acceleration_structure(vk::Backend::Ptr backend)
{
    // Allocate instance buffer

    vk::Buffer::Ptr instance_buffer = vk::Buffer::create(backend, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV | VK_BUFFER_USAGE_TRANSFER_DST_BIT, sizeof(vk::GeometryInstanceNV) * m_instances.size(), VMA_MEMORY_USAGE_GPU_ONLY, 0, m_instances.data());

    // Create top-level acceleration structure

    vk::AccelerationStructure::Desc desc;

    desc.set_instance_count(1);
    desc.set_type(VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV);

    m_vk_top_level_as = vk::AccelerationStructure::create(backend, desc);

    // Allocate scratch buffer

    VkAccelerationStructureMemoryRequirementsInfoNV memory_requirements_info;
    memory_requirements_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
    memory_requirements_info.pNext = nullptr;

    VkDeviceSize maximum_blas_size = 0;

    for (uint32_t i = 0; i < m_instances.size(); i++)
    {
        auto mesh = m_meshes[i].lock();

        memory_requirements_info.type                  = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;
        memory_requirements_info.accelerationStructure = mesh->acceleration_structure()->handle();

        VkMemoryRequirements2 mem_req_blas;
        vkGetAccelerationStructureMemoryRequirementsNV(backend->device(), &memory_requirements_info, &mem_req_blas);

        maximum_blas_size = glm::max(maximum_blas_size, mem_req_blas.memoryRequirements.size);
    }

    VkMemoryRequirements2 mem_req_tlas;
    memory_requirements_info.type                  = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;
    memory_requirements_info.accelerationStructure = m_vk_top_level_as->handle();
    vkGetAccelerationStructureMemoryRequirementsNV(backend->device(), &memory_requirements_info, &mem_req_tlas);

    const VkDeviceSize scratch_buffer_size = glm::max(maximum_blas_size, mem_req_tlas.memoryRequirements.size);

    vk::Buffer::Ptr scratch_buffer = vk::Buffer::create(backend, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, scratch_buffer_size, VMA_MEMORY_USAGE_GPU_ONLY, 0);

    // Build acceleration structures

    vk::CommandBuffer::Ptr cmd_buf = backend->allocate_graphics_command_buffer();

    VkCommandBufferBeginInfo begin_info;
    DW_ZERO_MEMORY(begin_info);

    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    vkBeginCommandBuffer(cmd_buf->handle(), &begin_info);

    VkMemoryBarrier memory_barrier;
    memory_barrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memory_barrier.pNext         = nullptr;
    memory_barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
    memory_barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;

    // Build bottom-level AS

    for (size_t i = 0; i < m_meshes.size(); ++i)
    {
        auto mesh = m_meshes[i].lock();

        const std::vector<VkGeometryNV>& geometry = mesh->ray_tracing_geometry();

        VkAccelerationStructureInfoNV& info = mesh->acceleration_structure()->info();

        info.instanceCount = 0;
        info.geometryCount = geometry.size();
        info.pGeometries   = geometry.data();

        VkAccelerationStructureNV as = mesh->acceleration_structure()->handle();

        vkCmdBuildAccelerationStructureNV(cmd_buf->handle(), &mesh->acceleration_structure()->info(), VK_NULL_HANDLE, 0, VK_FALSE, as, VK_NULL_HANDLE, scratch_buffer->handle(), 0);

        vkCmdPipelineBarrier(cmd_buf->handle(), VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memory_barrier, 0, 0, 0, 0);
    }

    // Build top-level AS

    m_vk_top_level_as->info().instanceCount = static_cast<uint32_t>(m_instances.size());
    m_vk_top_level_as->info().geometryCount = 0;
    m_vk_top_level_as->info().pGeometries   = nullptr;

    VkAccelerationStructureNV as = m_vk_top_level_as->handle();

    vkCmdBuildAccelerationStructureNV(cmd_buf->handle(), &m_vk_top_level_as->info(), instance_buffer->handle(), 0, VK_FALSE, as, VK_NULL_HANDLE, scratch_buffer->handle(), 0);

    vkCmdPipelineBarrier(cmd_buf->handle(), VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memory_barrier, 0, 0, 0, 0);

    vkEndCommandBuffer(cmd_buf->handle());

    backend->flush_graphics({ cmd_buf });
}

#endif

// -----------------------------------------------------------------------------------------------------------------------------------
} // namespace dw
