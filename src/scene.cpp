#include <scene.h>
#include <macros.h>
#if defined(DWSF_VULKAN)
#    include <vk_mem_alloc.h>
#endif
#include <unordered_map>
#include <material.h>
#include <assimp/scene.h>

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
#if defined(DWSF_VULKAN)
    for (int i = 0; i < m_material_buffers.size(); i++)
        m_material_buffers[i].reset();

    m_material_ds_layout.reset();
    m_ray_tracing_geometry_ds_layout.reset();
    m_indirect_draw_geometry_ds_layout.reset();
    m_albedo_ds.reset();
    m_normal_ds.reset();
    m_roughness_ds.reset();
    m_metallic_ds.reset();
    m_ray_tracing_geometry_ds.reset();
    m_indirect_draw_geometry_ds.reset();
    m_vk_top_level_as.reset();
#endif
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Scene::add_instance(dw::Mesh::Ptr mesh, glm::mat4 transform)
{
    m_instances.push_back({ transform, mesh });
}

// -----------------------------------------------------------------------------------------------------------------------------------

#if defined(DWSF_VULKAN)

void Scene::initialize_for_indirect_draw(vk::Backend::Ptr backend)
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Scene::initialize_for_ray_tracing(vk::Backend::Ptr backend)
{
    gather_instance_data(backend, true);
    create_descriptor_sets(backend, true);
    build_acceleration_structure(backend);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Scene::gather_instance_data(vk::Backend::Ptr backend, bool ray_tracing)
{
    std::unordered_map<uint32_t, uint32_t> attrib_map;

    for (auto& instance : m_instances)
    {
        auto mesh = instance.mesh.lock();

        SubMesh* submeshes = mesh->sub_meshes();

        uint32_t mesh_id = mesh->id();

        if (attrib_map.find(mesh_id) == attrib_map.end())
        {
            attrib_map[mesh_id] = m_meshes.size();
            m_meshes.push_back(instance.mesh);
        }

        if (ray_tracing)
        {
            RTGeometryInstance rt_instance;

            rt_instance.transform                   = glm::mat3x4(instance.transform);
            rt_instance.instanceCustomIndex         = m_rt_instances.size();
            rt_instance.mask                        = 0xff;
            rt_instance.instanceOffset              = 0;
            rt_instance.flags                       = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
            rt_instance.accelerationStructureHandle = mesh->acceleration_structure()->opaque_handle();

            m_rt_instances.push_back(rt_instance);
        }
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Scene::create_descriptor_sets(vk::Backend::Ptr backend, bool ray_tracing)
{
    std::vector<VkDescriptorBufferInfo> vbo_descriptors;
    std::vector<VkDescriptorBufferInfo> ibo_descriptors;
    std::vector<VkDescriptorImageInfo>  albedo_image_descriptors;
    std::vector<VkDescriptorImageInfo>  metallic_image_descriptors;
    std::vector<VkDescriptorImageInfo>  normal_image_descriptors;
    std::vector<VkDescriptorImageInfo>  roughness_image_descriptors;
    std::vector<VkDescriptorBufferInfo> mat_id_descriptors;

    m_material_buffers.resize(m_meshes.size());

    for (int i = 0; i < m_meshes.size(); i++)
    {
        auto mesh = m_meshes[i].lock();

        if (ray_tracing)
        {
            VkDescriptorBufferInfo ibo_info;

            ibo_info.buffer = mesh->index_buffer()->handle();
            ibo_info.offset = 0;
            ibo_info.range  = VK_WHOLE_SIZE;

            ibo_descriptors.push_back(ibo_info);

            VkDescriptorBufferInfo vbo_info;

            vbo_info.buffer = mesh->vertex_buffer()->handle();
            vbo_info.offset = 0;
            vbo_info.range  = VK_WHOLE_SIZE;

            vbo_descriptors.push_back(vbo_info);
        }

        const auto&           materials = mesh->materials();
        std::vector<uint32_t> material_idx(mesh->material_count());
        uint32_t              mat_id_offset = albedo_image_descriptors.size();

        for (int j = 0; j < mesh->material_count(); j++)
        {
            material_idx[j] = mat_id_offset + j;

            VkDescriptorImageInfo image_info[4];

            image_info[0].sampler     = Material::common_sampler()->handle();
            image_info[0].imageView   = materials[j]->image_view(aiTextureType_DIFFUSE)->handle();
            image_info[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            albedo_image_descriptors.push_back(image_info[0]);

            image_info[1].sampler     = Material::common_sampler()->handle();
            image_info[1].imageView   = materials[j]->image_view(aiTextureType_HEIGHT)->handle();
            image_info[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            normal_image_descriptors.push_back(image_info[1]);

            image_info[2].sampler     = Material::common_sampler()->handle();
            image_info[2].imageView   = materials[j]->image_view(aiTextureType_SHININESS)->handle();
            image_info[2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            roughness_image_descriptors.push_back(image_info[2]);

            image_info[3].sampler     = Material::common_sampler()->handle();
            image_info[3].imageView   = materials[j]->image_view(aiTextureType_AMBIENT)->handle();
            image_info[3].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            metallic_image_descriptors.push_back(image_info[3]);
        }

        m_material_buffers[i] = vk::Buffer::create(backend, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(uint32_t) * material_idx.size(), VMA_MEMORY_USAGE_GPU_ONLY, 0, material_idx.data());

        VkDescriptorBufferInfo mat_info;

        mat_info.buffer = m_material_buffers[i]->handle();
        mat_info.offset = 0;
        mat_info.range  = VK_WHOLE_SIZE;

        mat_id_descriptors.push_back(mat_info);
    }

    // Create descriptor set layouts

    if (ray_tracing)
    {
        dw::vk::DescriptorSetLayout::Desc desc;

        desc.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_meshes.size(), VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);
        desc.add_binding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);
        desc.add_binding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_meshes.size(), VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);
        desc.add_binding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_meshes.size(), VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);

        m_ray_tracing_geometry_ds_layout = dw::vk::DescriptorSetLayout::create(backend, desc);

        m_ray_tracing_geometry_ds = backend->allocate_descriptor_set(m_ray_tracing_geometry_ds_layout);
    }

    dw::vk::DescriptorSetLayout::Desc desc;

    desc.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, albedo_image_descriptors.size(), VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);

    std::vector<VkDescriptorBindingFlagsEXT> descriptor_binding_flags = {
        VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT
    };

    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT set_layout_binding_flags;
    DW_ZERO_MEMORY(set_layout_binding_flags);

    set_layout_binding_flags.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
    set_layout_binding_flags.bindingCount  = 1;
    set_layout_binding_flags.pBindingFlags = descriptor_binding_flags.data();

    desc.set_next_ptr(&set_layout_binding_flags);

    m_material_ds_layout = dw::vk::DescriptorSetLayout::create(backend, desc);

    // Create descriptor sets

    m_albedo_ds    = backend->allocate_descriptor_set(m_material_ds_layout);
    m_normal_ds    = backend->allocate_descriptor_set(m_material_ds_layout);
    m_roughness_ds = backend->allocate_descriptor_set(m_material_ds_layout);
    m_metallic_ds  = backend->allocate_descriptor_set(m_material_ds_layout);

    if (ray_tracing)
    {
        VkWriteDescriptorSet geometry_write_data[3];

        DW_ZERO_MEMORY(geometry_write_data[0]);
        DW_ZERO_MEMORY(geometry_write_data[1]);
        DW_ZERO_MEMORY(geometry_write_data[2]);

        geometry_write_data[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        geometry_write_data[0].descriptorCount = mat_id_descriptors.size();
        geometry_write_data[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        geometry_write_data[0].pBufferInfo     = mat_id_descriptors.data();
        geometry_write_data[0].dstBinding      = 0;
        geometry_write_data[0].dstSet          = m_ray_tracing_geometry_ds->handle();

        geometry_write_data[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        geometry_write_data[1].descriptorCount = m_meshes.size();
        geometry_write_data[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        geometry_write_data[1].pBufferInfo     = vbo_descriptors.data();
        geometry_write_data[1].dstBinding      = 1;
        geometry_write_data[1].dstSet          = m_ray_tracing_geometry_ds->handle();

        geometry_write_data[2].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        geometry_write_data[2].descriptorCount = m_meshes.size();
        geometry_write_data[2].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        geometry_write_data[2].pBufferInfo     = ibo_descriptors.data();
        geometry_write_data[2].dstBinding      = 2;
        geometry_write_data[2].dstSet          = m_ray_tracing_geometry_ds->handle();

        vkUpdateDescriptorSets(backend->device(), 3, geometry_write_data, 0, nullptr);
    }

    VkWriteDescriptorSet material_write_data[4];

    DW_ZERO_MEMORY(material_write_data[0]);
    DW_ZERO_MEMORY(material_write_data[1]);
    DW_ZERO_MEMORY(material_write_data[2]);
    DW_ZERO_MEMORY(material_write_data[3]);

    material_write_data[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    material_write_data[0].descriptorCount = albedo_image_descriptors.size();
    material_write_data[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    material_write_data[0].pImageInfo      = albedo_image_descriptors.data();
    material_write_data[0].dstBinding      = 0;
    material_write_data[0].dstSet          = m_albedo_ds->handle();

    material_write_data[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    material_write_data[1].descriptorCount = normal_image_descriptors.size();
    material_write_data[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    material_write_data[1].pImageInfo      = normal_image_descriptors.data();
    material_write_data[1].dstBinding      = 0;
    material_write_data[1].dstSet          = m_normal_ds->handle();

    material_write_data[2].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    material_write_data[2].descriptorCount = roughness_image_descriptors.size();
    material_write_data[2].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    material_write_data[2].pImageInfo      = roughness_image_descriptors.data();
    material_write_data[2].dstBinding      = 0;
    material_write_data[2].dstSet          = m_roughness_ds->handle();

    material_write_data[3].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    material_write_data[3].descriptorCount = metallic_image_descriptors.size();
    material_write_data[3].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    material_write_data[3].pImageInfo      = metallic_image_descriptors.data();
    material_write_data[3].dstBinding      = 0;
    material_write_data[3].dstSet          = m_metallic_ds->handle();

    vkUpdateDescriptorSets(backend->device(), 4, material_write_data, 0, nullptr);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Scene::build_acceleration_structure(vk::Backend::Ptr backend)
{
    // Allocate instance buffer

    vk::Buffer::Ptr instance_buffer = vk::Buffer::create(backend, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV | VK_BUFFER_USAGE_TRANSFER_DST_BIT, sizeof(RTGeometryInstance) * m_rt_instances.size(), VMA_MEMORY_USAGE_GPU_ONLY, 0, m_rt_instances.data());

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

    for (uint32_t i = 0; i < m_meshes.size(); i++)
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

    for (size_t i = 0; i < m_rt_instances.size(); ++i)
    {
        auto mesh = m_meshes[i].lock();

        VkGeometryNV geometry = mesh->ray_tracing_geometry();

        VkAccelerationStructureInfoNV& info = mesh->acceleration_structure()->info();

        info.instanceCount = 0;
        info.geometryCount = 1;
        info.pGeometries   = &geometry;

        VkAccelerationStructureNV as = mesh->acceleration_structure()->handle();

        vkCmdBuildAccelerationStructureNV(cmd_buf->handle(), &mesh->acceleration_structure()->info(), VK_NULL_HANDLE, 0, VK_FALSE, as, VK_NULL_HANDLE, scratch_buffer->handle(), 0);

        vkCmdPipelineBarrier(cmd_buf->handle(), VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memory_barrier, 0, 0, 0, 0);
    }

    // Build top-level AS

    m_vk_top_level_as->info().instanceCount = static_cast<uint32_t>(m_rt_instances.size());
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
