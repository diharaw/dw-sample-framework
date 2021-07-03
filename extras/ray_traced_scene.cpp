#include "ray_traced_scene.h"
#include <macros.h>
#if defined(DWSF_VULKAN)
#    include <vk_mem_alloc.h>
#endif
#include <unordered_map>
#include <material.h>
#include <profiler.h>
#include <assimp/scene.h>

#define MAX_MATERIAL_COUNT 1024
#define MAX_INSTANCES 1024

#if defined(DWSF_VULKAN)
namespace dw
{
// -----------------------------------------------------------------------------------------------------------------------------------

struct InstanceData
{
    glm::mat4 model_matrix;
    uint32_t  mesh_index;
    float     padding[3];
};

// -----------------------------------------------------------------------------------------------------------------------------------

struct MaterialData
{
    glm::ivec4 texture_indices0 = glm::ivec4(-1); // x: albedo, y: normals, z: roughness, w: metallic
    glm::ivec4 texture_indices1 = glm::ivec4(-1); // x: emissive, z: roughness_channel, w: metallic_channel
    glm::vec4  albedo;
    glm::vec4  emissive;
    glm::vec4  roughness_metallic;
};

// -----------------------------------------------------------------------------------------------------------------------------------

static uint32_t g_last_scene_idx = 0;

// -----------------------------------------------------------------------------------------------------------------------------------

void transformed_aabb(const RayTracedScene::Instance& instance, glm::vec3& min_extents, glm::vec3& max_extents)
{
    auto mesh = instance.mesh.lock();

    min_extents = instance.transform * glm::vec4(mesh->min_extents(), 1.0f);
    max_extents = instance.transform * glm::vec4(mesh->max_extents(), 1.0f);
}

// -----------------------------------------------------------------------------------------------------------------------------------

RayTracedScene::Ptr RayTracedScene::create(vk::Backend::Ptr backend, std::vector<Instance> instances)
{
    return std::shared_ptr<RayTracedScene>(new RayTracedScene(backend, instances));
}

// -----------------------------------------------------------------------------------------------------------------------------------

RayTracedScene::RayTracedScene(vk::Backend::Ptr backend, std::vector<Instance> instances) :
    m_backend(backend), m_instances(instances), m_id(g_last_scene_idx++)
{
    // Allocate device instance buffer
    m_tlas_instance_buffer_device = vk::Buffer::create(backend, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, sizeof(VkAccelerationStructureInstanceKHR) * MAX_INSTANCES, VMA_MEMORY_USAGE_GPU_ONLY, 0);
    m_tlas_instance_buffer_device->set_name("TLAS Instance Buffer Device");

    VkDeviceOrHostAddressConstKHR instance_device_address {};
    instance_device_address.deviceAddress = m_tlas_instance_buffer_device->device_address();

    // Allocate host instance buffer
    m_tlas_instance_buffer_host = vk::Buffer::create(backend, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, sizeof(VkAccelerationStructureInstanceKHR) * MAX_INSTANCES, VMA_MEMORY_USAGE_CPU_ONLY, VMA_ALLOCATION_CREATE_MAPPED_BIT);
    m_tlas_instance_buffer_host->set_name("TLAS Instance Buffer Host");

    // Create TLAS
    VkAccelerationStructureGeometryKHR tlas_geometry;
    DW_ZERO_MEMORY(tlas_geometry);

    tlas_geometry.sType                              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    tlas_geometry.geometryType                       = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    tlas_geometry.geometry.instances.sType           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    tlas_geometry.geometry.instances.arrayOfPointers = VK_FALSE;
    tlas_geometry.geometry.instances.data            = instance_device_address;

    vk::AccelerationStructure::Desc desc;

    desc.set_geometry_count(1);
    desc.set_geometries({ tlas_geometry });
    desc.set_max_primitive_counts({ MAX_INSTANCES });
    desc.set_type(VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR);
    desc.set_flags(VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR);

    m_tlas = vk::AccelerationStructure::create(backend, desc);
    m_tlas->set_name("TLAS");

    // Allocate scratch buffer
    m_tlas_scratch_buffer = vk::Buffer::create(backend, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, m_tlas->build_sizes().buildScratchSize, VMA_MEMORY_USAGE_GPU_ONLY, 0);
    m_tlas_scratch_buffer->set_name("TLAS Scratch Buffer");

    // Create material data buffer
    m_material_data_buffer = vk::Buffer::create(backend, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(MaterialData) * MAX_MATERIAL_COUNT, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT);
    m_material_data_buffer->set_name("Material Data Buffer");

    // Create instance data buffer
    m_instance_data_buffer = vk::Buffer::create(backend, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(InstanceData) * MAX_INSTANCES, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT);
    m_instance_data_buffer->set_name("Instance Data Buffer");

    vk::DescriptorPool::Desc dp_desc;

    dp_desc.set_max_sets(1)
        .add_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10)
        .add_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2048)
        .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5 * MAX_INSTANCES)
        .add_pool_size(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 10);

    m_descriptor_pool = vk::DescriptorPool::create(backend, dp_desc);
    m_descriptor_pool->set_name("Scene Descriptor Pool");

    vk::DescriptorSetLayout::Desc scene_ds_layout_desc;

    std::vector<VkDescriptorBindingFlags> descriptor_binding_flags = {
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
    };

    VkDescriptorSetLayoutBindingFlagsCreateInfo set_layout_binding_flags;
    DW_ZERO_MEMORY(set_layout_binding_flags);

    set_layout_binding_flags.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    set_layout_binding_flags.bindingCount  = 7;
    set_layout_binding_flags.pBindingFlags = descriptor_binding_flags.data();

    scene_ds_layout_desc.set_next_ptr(&set_layout_binding_flags);
    // Material Data
    scene_ds_layout_desc.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT);
    // Instance Data
    scene_ds_layout_desc.add_binding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT);
    // Acceleration Structures
    scene_ds_layout_desc.add_binding(2, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT);
    // Vertex Buffers
    scene_ds_layout_desc.add_binding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_INSTANCES, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT);
    // Index Buffers
    scene_ds_layout_desc.add_binding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_INSTANCES, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT);
    // Material Indices Buffers
    scene_ds_layout_desc.add_binding(5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_INSTANCES, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT);
    // Textures
    scene_ds_layout_desc.add_binding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2048, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT);

    m_ds_layout = vk::DescriptorSetLayout::create(backend, scene_ds_layout_desc);
    m_ds_layout->set_name("Scene Descriptor Set Layout");

    m_ds = vk::DescriptorSet::create(backend, m_ds_layout, m_descriptor_pool);
    m_ds->set_name("Scene Descriptor Set");

    create_gpu_resources();

    // Compute scene bounds
    if (m_instances.size() > 0)
    {
        transformed_aabb(m_instances[0], m_min_extents, m_max_extents);

        for (auto instance : m_instances)
        {
            glm::vec3 min_extents, max_extents;

            transformed_aabb(instance, min_extents, max_extents);

            if (max_extents.x > m_max_extents.x)
                m_max_extents.x = max_extents.x;
            if (max_extents.y > m_max_extents.y)
                m_max_extents.y = max_extents.y;
            if (max_extents.z > m_max_extents.z)
                m_max_extents.z = max_extents.z;

            if (min_extents.x < m_min_extents.x)
                m_min_extents.x = min_extents.x;
            if (min_extents.y < m_min_extents.y)
                m_min_extents.y = min_extents.y;
            if (min_extents.z < m_min_extents.z)
                m_min_extents.z = min_extents.z;
        }
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

RayTracedScene::~RayTracedScene()
{
    m_ds.reset();
    m_ds_layout.reset();
    m_descriptor_pool.reset();
    m_material_data_buffer.reset();
    m_instance_data_buffer.reset();
    m_material_indices_buffers.clear();
    m_tlas_instance_buffer_host.reset();
    m_tlas_instance_buffer_device.reset();
    m_tlas_scratch_buffer.reset();
    m_tlas.reset();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void RayTracedScene::build_tlas(vk::CommandBuffer::Ptr cmd_buf)
{
    DW_SCOPED_SAMPLE("Build TLAS", cmd_buf);

    copy_tlas_data();

    if (m_instances.size() > 0)
    {
        VkBufferCopy copy_region;
        DW_ZERO_MEMORY(copy_region);

        copy_region.dstOffset = 0;
        copy_region.size      = sizeof(VkAccelerationStructureInstanceKHR) * m_instances.size();

        vkCmdCopyBuffer(cmd_buf->handle(), m_tlas_instance_buffer_host->handle(), m_tlas_instance_buffer_device->handle(), 1, &copy_region);
    }

    {
        VkMemoryBarrier memory_barrier;
        memory_barrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        memory_barrier.pNext         = nullptr;
        memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        memory_barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;

        vkCmdPipelineBarrier(cmd_buf->handle(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &memory_barrier, 0, nullptr, 0, nullptr);
    }

    VkAccelerationStructureGeometryKHR geometry;
    DW_ZERO_MEMORY(geometry);

    geometry.sType                                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.geometryType                          = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry.instances.sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry.geometry.instances.arrayOfPointers    = VK_FALSE;
    geometry.geometry.instances.data.deviceAddress = m_tlas_instance_buffer_device->device_address();

    VkAccelerationStructureBuildGeometryInfoKHR build_info;
    DW_ZERO_MEMORY(build_info);

    build_info.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    build_info.type                      = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    build_info.flags                     = m_tlas->flags();
    build_info.srcAccelerationStructure  = m_tlas_built ? m_tlas->handle() : VK_NULL_HANDLE;
    build_info.dstAccelerationStructure  = m_tlas->handle();
    build_info.geometryCount             = 1;
    build_info.pGeometries               = &geometry;
    build_info.scratchData.deviceAddress = m_tlas_scratch_buffer->device_address();

    VkAccelerationStructureBuildRangeInfoKHR build_range_info;

    build_range_info.primitiveCount  = m_instances.size();
    build_range_info.primitiveOffset = 0;
    build_range_info.firstVertex     = 0;
    build_range_info.transformOffset = 0;

    const VkAccelerationStructureBuildRangeInfoKHR* ptr_build_range_info = &build_range_info;

    vkCmdBuildAccelerationStructuresKHR(cmd_buf->handle(), 1, &build_info, &ptr_build_range_info);

    {
        VkMemoryBarrier memory_barrier;
        memory_barrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        memory_barrier.pNext         = nullptr;
        memory_barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
        memory_barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;

        vkCmdPipelineBarrier(cmd_buf->handle(), VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &memory_barrier, 0, 0, 0, 0);
    }

    m_tlas_built = true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

RayTracedScene::Instance& RayTracedScene::fetch_instance(const uint32_t& idx)
{
    return m_instances[idx];
}

// -----------------------------------------------------------------------------------------------------------------------------------

int32_t RayTracedScene::material_index(const uint32_t& id)
{
    if (m_local_to_global_mat_idx.find(id) != m_local_to_global_mat_idx.end())
        return m_local_to_global_mat_idx[id];
    else
        return -1;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void RayTracedScene::create_gpu_resources()
{
    auto backend = m_backend.lock();

    std::unordered_set<uint32_t> processed_meshes;
    std::unordered_set<uint32_t> processed_materials;
    std::vector<MaterialData>    material_datas;

    std::vector<VkDescriptorBufferInfo> vbo_descriptors;
    std::vector<VkDescriptorBufferInfo> ibo_descriptors;
    std::vector<VkDescriptorBufferInfo> material_indices_descriptors;
    std::vector<VkDescriptorImageInfo>  image_descriptors;

    m_material_indices_buffers.reserve(m_instances.size());

    for (auto& instance : m_instances)
    {
        auto mesh = instance.mesh.lock();

        const std::vector<SubMesh>& submeshes = mesh->sub_meshes();

        uint32_t mesh_id = mesh->id();

        if (processed_meshes.find(mesh_id) == processed_meshes.end())
        {
            processed_meshes.insert(mesh_id);
            m_local_to_global_mesh_idx[mesh_id] = m_meshes.size();
            m_meshes.push_back(instance.mesh);

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

            vk::Buffer::Ptr material_indices_buffer = vk::Buffer::create(backend, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(glm::uvec2) * submeshes.size(), VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT);
            glm::uvec2*     material_indices        = (glm::uvec2*)material_indices_buffer->mapped_ptr();

            VkDescriptorBufferInfo material_indice_info;

            material_indice_info.buffer = material_indices_buffer->handle();
            material_indice_info.offset = 0;
            material_indice_info.range  = VK_WHOLE_SIZE;

            material_indices_descriptors.push_back(material_indice_info);

            m_material_indices_buffers.push_back(material_indices_buffer);

            const auto& materials = mesh->materials();

            for (uint32_t submesh_idx = 0; submesh_idx < submeshes.size(); submesh_idx++)
            {
                const auto&   submesh = submeshes[submesh_idx];
                Material::Ptr mat     = materials[submesh.mat_idx];

                if (processed_materials.find(mat->id()) == processed_materials.end())
                {
                    processed_materials.insert(mat->id());
                    m_local_to_global_mat_idx[mat->id()] = material_datas.size();

                    MaterialData material_data;

                    material_data.albedo = mat->albedo_value();
                    // Covert from sRGB to Linear
                    material_data.albedo             = glm::vec4(glm::pow(glm::vec3(material_data.albedo[0], material_data.albedo[1], material_data.albedo[2]), glm::vec3(2.2f)), material_data.albedo.a);
                    material_data.roughness_metallic = glm::vec4(mat->roughness_value(), mat->metallic_value(), 0.0f, 0.0f);
                    material_data.emissive           = glm::vec4(mat->emissive_value(), 0.0f);

                    if (mat->albedo_image_view())
                    {
                        VkDescriptorImageInfo image_info;

                        image_info.sampler     = Material::common_sampler()->handle();
                        image_info.imageView   = mat->albedo_image_view()->handle();
                        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                        material_data.texture_indices0.x = image_descriptors.size();

                        image_descriptors.push_back(image_info);
                    }

                    if (mat->normal_image_view())
                    {
                        VkDescriptorImageInfo image_info;

                        image_info.sampler     = Material::common_sampler()->handle();
                        image_info.imageView   = mat->normal_image_view()->handle();
                        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                        material_data.texture_indices0.y = image_descriptors.size();

                        image_descriptors.push_back(image_info);
                    }

                    if (mat->roughness_image_view())
                    {
                        VkDescriptorImageInfo image_info;

                        image_info.sampler     = Material::common_sampler()->handle();
                        image_info.imageView   = mat->roughness_image_view()->handle();
                        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                        material_data.texture_indices0.z = image_descriptors.size();
                        material_data.texture_indices1.z = mat->roughness_channel();

                        image_descriptors.push_back(image_info);
                    }

                    if (mat->metallic_image_view())
                    {
                        VkDescriptorImageInfo image_info;

                        image_info.sampler     = Material::common_sampler()->handle();
                        image_info.imageView   = mat->metallic_image_view()->handle();
                        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                        material_data.texture_indices0.w = image_descriptors.size();
                        material_data.texture_indices1.w = mat->metallic_channel();

                        image_descriptors.push_back(image_info);
                    }

                    if (mat->emissive_image_view())
                    {
                        VkDescriptorImageInfo image_info;

                        image_info.sampler     = Material::common_sampler()->handle();
                        image_info.imageView   = mat->emissive_image_view()->handle();
                        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                        material_data.texture_indices1.x = image_descriptors.size();

                        image_descriptors.push_back(image_info);
                    }

                    material_datas.push_back(material_data);
                }

                glm::uvec2 pair               = glm::uvec2(submesh.base_index / 3, m_local_to_global_mat_idx[mat->id()]);
                material_indices[submesh_idx] = pair;
            }
        }
    }

    memcpy(m_material_data_buffer->mapped_ptr(), material_datas.data(), sizeof(MaterialData) * material_datas.size());

    std::vector<VkWriteDescriptorSet> write_datas;

    VkWriteDescriptorSet write_data;

    // ------------------------------------------------------------------------------------------
    // Material Data
    // ------------------------------------------------------------------------------------------

    DW_ZERO_MEMORY(write_data);

    VkDescriptorBufferInfo material_buffer_info;

    material_buffer_info.buffer = m_material_data_buffer->handle();
    material_buffer_info.offset = 0;
    material_buffer_info.range  = VK_WHOLE_SIZE;

    write_data.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_data.descriptorCount = 1;
    write_data.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write_data.pBufferInfo     = &material_buffer_info;
    write_data.dstBinding      = 0;
    write_data.dstSet          = m_ds->handle();

    write_datas.push_back(write_data);

    // ------------------------------------------------------------------------------------------
    // Instance Data
    // ------------------------------------------------------------------------------------------

    DW_ZERO_MEMORY(write_data);

    VkDescriptorBufferInfo instance_buffer_info;

    instance_buffer_info.buffer = m_instance_data_buffer->handle();
    instance_buffer_info.offset = 0;
    instance_buffer_info.range  = VK_WHOLE_SIZE;

    write_data.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_data.descriptorCount = 1;
    write_data.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write_data.pBufferInfo     = &instance_buffer_info;
    write_data.dstBinding      = 1;
    write_data.dstSet          = m_ds->handle();

    write_datas.push_back(write_data);

    // ------------------------------------------------------------------------------------------
    // Acceleration Structure
    // ------------------------------------------------------------------------------------------

    DW_ZERO_MEMORY(write_data);

    VkWriteDescriptorSetAccelerationStructureKHR descriptor_as;

    descriptor_as.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    descriptor_as.pNext                      = nullptr;
    descriptor_as.accelerationStructureCount = 1;
    descriptor_as.pAccelerationStructures    = &m_tlas->handle();

    DW_ZERO_MEMORY(write_data);

    write_data.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_data.pNext           = &descriptor_as;
    write_data.descriptorCount = 1;
    write_data.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    write_data.dstBinding      = 2;
    write_data.dstSet          = m_ds->handle();

    write_datas.push_back(write_data);

    // ------------------------------------------------------------------------------------------
    // Vertex Buffers
    // ------------------------------------------------------------------------------------------

    if (vbo_descriptors.size() > 0)
    {
        DW_ZERO_MEMORY(write_data);

        write_data.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_data.descriptorCount = vbo_descriptors.size();
        write_data.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write_data.pBufferInfo     = vbo_descriptors.data();
        write_data.dstBinding      = 3;
        write_data.dstSet          = m_ds->handle();

        write_datas.push_back(write_data);
    }

    // ------------------------------------------------------------------------------------------
    // Index Buffers
    // ------------------------------------------------------------------------------------------

    if (ibo_descriptors.size() > 0)
    {
        DW_ZERO_MEMORY(write_data);

        write_data.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_data.descriptorCount = ibo_descriptors.size();
        write_data.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write_data.pBufferInfo     = ibo_descriptors.data();
        write_data.dstBinding      = 4;
        write_data.dstSet          = m_ds->handle();

        write_datas.push_back(write_data);
    }

    // ------------------------------------------------------------------------------------------
    // Material Indices Buffers
    // ------------------------------------------------------------------------------------------

    if (material_indices_descriptors.size() > 0)
    {
        DW_ZERO_MEMORY(write_data);

        write_data.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_data.descriptorCount = material_indices_descriptors.size();
        write_data.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write_data.pBufferInfo     = material_indices_descriptors.data();
        write_data.dstBinding      = 5;
        write_data.dstSet          = m_ds->handle();

        write_datas.push_back(write_data);
    }

    // ------------------------------------------------------------------------------------------
    // Images
    // ------------------------------------------------------------------------------------------

    if (image_descriptors.size() > 0)
    {
        DW_ZERO_MEMORY(write_data);

        write_data.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_data.descriptorCount = image_descriptors.size();
        write_data.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_data.pImageInfo      = image_descriptors.data();
        write_data.dstBinding      = 6;
        write_data.dstSet          = m_ds->handle();

        write_datas.push_back(write_data);
    }

    if (write_datas.size() > 0)
        vkUpdateDescriptorSets(backend->device(), write_datas.size(), write_datas.data(), 0, nullptr);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void RayTracedScene::copy_tlas_data()
{
    std::vector<VkAccelerationStructureInstanceKHR> rt_instances;
    std::vector<InstanceData>                       instance_datas;

    for (uint32_t i = 0; i < m_instances.size(); i++)
    {
        const Instance& instance = m_instances[i];
        const auto&     mesh     = instance.mesh.lock();

        // ------------------------------------------------------------------------------------------
        // Instance Data
        // ------------------------------------------------------------------------------------------
        InstanceData instance_data;

        // Set mesh data index
        instance_data.mesh_index   = m_local_to_global_mesh_idx[mesh->id()];
        instance_data.model_matrix = instance.transform;

        instance_datas.push_back(instance_data);

        // ------------------------------------------------------------------------------------------
        // VkAccelerationStructureInstanceKHR
        // ------------------------------------------------------------------------------------------
        VkAccelerationStructureInstanceKHR rt_instance;

        glm::mat3x4 transform = glm::mat3x4(glm::transpose(instance.transform));

        memcpy(&rt_instance.transform, &transform, sizeof(rt_instance.transform));

        rt_instance.instanceCustomIndex                    = rt_instances.size();
        rt_instance.mask                                   = 0xFF;
        rt_instance.instanceShaderBindingTableRecordOffset = 0;
        rt_instance.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        rt_instance.accelerationStructureReference         = mesh->acceleration_structure()->device_address();

        rt_instances.push_back(rt_instance);
    }

    memcpy(m_instance_data_buffer->mapped_ptr(), instance_datas.data(), sizeof(InstanceData) * instance_datas.size());
    memcpy(m_tlas_instance_buffer_host->mapped_ptr(), rt_instances.data(), sizeof(VkAccelerationStructureInstanceKHR) * rt_instances.size());
}

// -----------------------------------------------------------------------------------------------------------------------------------
} // namespace dw
#endif