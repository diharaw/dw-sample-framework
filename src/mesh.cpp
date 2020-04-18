#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <logger.h>
#include <macros.h>
#include <material.h>
#include <mesh.h>
#include <stdio.h>
#include <ogl.h>
#if defined(DWSF_VULKAN)
#    include <vk_mem_alloc.h>
#endif

namespace dw
{
std::unordered_map<std::string, std::weak_ptr<Mesh>> Mesh::m_cache;

// Assimp texture enum lookup table.
static const aiTextureType kTextureTypes[] = {
    aiTextureType_DIFFUSE, aiTextureType_SPECULAR, aiTextureType_AMBIENT, aiTextureType_EMISSIVE, aiTextureType_HEIGHT, aiTextureType_NORMALS, aiTextureType_SHININESS, aiTextureType_OPACITY, aiTextureType_DISPLACEMENT, aiTextureType_LIGHTMAP, aiTextureType_REFLECTION
};

// Assimp texture enum string table.
static std::string kTextureTypeStrings[] = {
    "aiTextureType_DIFFUSE", "aiTextureType_SPECULAR", "aiTextureType_AMBIENT", "aiTextureType_EMISSIVE", "aiTextureType_HEIGHT", "aiTextureType_NORMALS", "aiTextureType_SHININESS", "aiTextureType_OPACITY", "aiTextureType_DISPLACEMENT", "aiTextureType_LIGHTMAP", "aiTextureType_REFLECTION"
};

static uint32_t g_last_mesh_idx = 0;

// -----------------------------------------------------------------------------------------------------------------------------------
// Assimp loader helper method declarations.
// -----------------------------------------------------------------------------------------------------------------------------------

std::string assimp_get_texture_path(aiMaterial*   material,
                                    aiTextureType texture_type);
bool        assimp_does_material_exist(std::vector<unsigned int>& materials,
                                       unsigned int&              current_material);

// -----------------------------------------------------------------------------------------------------------------------------------

Mesh::Ptr Mesh::load(
#if defined(DWSF_VULKAN)
    vk::Backend::Ptr backend,
#endif
    const std::string& path,
    bool               load_materials)
{
    if (m_cache.find(path) == m_cache.end() || m_cache[path].expired())
    {
        Mesh::Ptr mesh = std::shared_ptr<Mesh>(new Mesh(
#if defined(DWSF_VULKAN)
            backend,
#endif
            path,
            load_materials));
        m_cache[path] = mesh;
        return mesh;
    }
    else
    {
        auto ptr = m_cache[path];
        return ptr.lock();
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

Mesh::Ptr Mesh::load(
#if defined(DWSF_VULKAN)
    vk::Backend::Ptr backend,
#endif
    const std::string& name,
    int                num_vertices,
    Vertex*            vertices,
    int                num_indices,
    uint32_t*          indices,
    int                num_sub_meshes,
    SubMesh*           sub_meshes,
    glm::vec3          max_extents,
    glm::vec3          min_extents)
{
    if (m_cache.find(name) == m_cache.end() || m_cache[name].expired())
    {
        Mesh::Ptr mesh = std::shared_ptr<Mesh>(new Mesh());

        // Manually assign properties...
        mesh->m_vertices       = vertices;
        mesh->m_vertex_count   = num_vertices;
        mesh->m_indices        = indices;
        mesh->m_index_count    = num_indices;
        mesh->m_sub_meshes     = sub_meshes;
        mesh->m_sub_mesh_count = num_sub_meshes;
        mesh->m_max_extents    = max_extents;
        mesh->m_min_extents    = min_extents;

        // ...then manually call the method to create GPU objects.
        mesh->create_gpu_objects(
#if defined(DWSF_VULKAN)
            backend
#endif
        );

        m_cache[name] = mesh;
        return mesh;
    }
    else
    {
        auto ptr = m_cache[name];
        return ptr.lock();
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

#if defined(DWSF_VULKAN)

void Mesh::initialize_for_ray_tracing(vk::Backend::Ptr backend)
{
    VkGeometryNV current = {};

    current.sType                              = VK_STRUCTURE_TYPE_GEOMETRY_NV;
    current.pNext                              = nullptr;
    current.geometryType                       = VK_GEOMETRY_TYPE_TRIANGLES_NV;
    current.geometry.triangles.sType           = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
    current.geometry.triangles.pNext           = nullptr;
    current.geometry.triangles.vertexData      = m_vbo->handle();
    current.geometry.triangles.vertexOffset    = 0;
    current.geometry.triangles.vertexCount     = m_vertex_count;
    current.geometry.triangles.vertexStride    = sizeof(Vertex);
    current.geometry.triangles.vertexFormat    = VK_FORMAT_R32G32B32_SFLOAT;
    current.geometry.triangles.indexData       = m_ibo->handle();
    current.geometry.triangles.indexOffset     = 0;
    current.geometry.triangles.indexCount      = m_index_count;
    current.geometry.triangles.indexType       = VK_INDEX_TYPE_UINT32;
    current.geometry.triangles.transformData   = VK_NULL_HANDLE;
    current.geometry.triangles.transformOffset = 0;
    current.geometry.aabbs                     = {};
    current.geometry.aabbs.sType               = VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV;
    current.flags                              = VK_GEOMETRY_OPAQUE_BIT_NV;

    m_rt_geometry = current;

    vk::AccelerationStructure::Desc desc;

    desc.set_geometries({ m_rt_geometry });
    desc.set_instance_count(0);
    desc.set_type(VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV);

    m_rt_as = vk::AccelerationStructure::create(backend, desc);
}

#endif

// -----------------------------------------------------------------------------------------------------------------------------------

bool Mesh::is_loaded(const std::string& name)
{
    return m_cache.find(name) != m_cache.end();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Mesh::load_from_disk(
#if defined(DWSF_VULKAN)
    vk::Backend::Ptr backend,
#endif
    const std::string& path,
    bool               load_materials)
{
    const aiScene*   Scene;
    Assimp::Importer importer;
    Scene = importer.ReadFile(path,
                              aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    m_sub_mesh_count = Scene->mNumMeshes;
    m_sub_meshes     = new SubMesh[m_sub_mesh_count];

    // Temporary variables
    aiMaterial*                                 temp_material;
    std::vector<uint32_t>                       processed_mat_id;
    std::unordered_map<uint32_t, Material::Ptr> mat_id_mapping;
    std::unordered_map<uint32_t, uint32_t>      local_mat_idx_mapping;

    // Iterate over submeshes and find materials
    for (int i = 0; i < m_sub_mesh_count; i++)
    {
        bool has_least_one_texture = false;

        m_sub_meshes[i].name        = std::string(Scene->mMeshes[i]->mName.C_Str());
        m_sub_meshes[i].index_count = Scene->mMeshes[i]->mNumFaces * 3;
        m_sub_meshes[i].base_index  = m_index_count;
        m_sub_meshes[i].base_vertex = m_vertex_count;

        m_vertex_count += Scene->mMeshes[i]->mNumVertices;
        m_index_count += m_sub_meshes[i].index_count;

        if (load_materials)
        {
            std::string material_paths[16];

            if (mat_id_mapping.find(Scene->mMeshes[i]->mMaterialIndex) == mat_id_mapping.end())
            {
                std::string current_mat_name;

                temp_material    = Scene->mMaterials[Scene->mMeshes[i]->mMaterialIndex];
                current_mat_name = path + std::to_string(i);

                aiColor3D diffuse         = aiColor3D(1.0f, 1.0f, 1.0f);
                bool      has_diifuse_val = false;

                for (uint32_t i = 0; i < 11; i++)
                {
                    std::string texture = assimp_get_texture_path(temp_material, kTextureTypes[i]);

                    if (texture != "")
                    {
                        std::replace(texture.begin(), texture.end(), '\\', '/');

                        if (texture.length() > 4 && texture[0] != ' ')
                        {
                            DW_LOG_INFO("Found " + kTextureTypeStrings[i] + ": " + texture);
                            material_paths[kTextureTypes[i]] = texture;
                            has_least_one_texture            = true;
                        }
                    }
                    else if (i == 0)
                    {
                        // Try loading in a Diffuse material property
                        has_diifuse_val = temp_material->Get(AI_MATKEY_COLOR_DIFFUSE,
                                                             diffuse)
                            == aiReturn_SUCCESS;
                    }
                }

                if (has_least_one_texture)
                {
                    Material::Ptr mat = Material::load(
#if defined(DWSF_VULKAN)
                        backend,
#endif
                        current_mat_name,
                        &material_paths[0]);

                    mat_id_mapping[Scene->mMeshes[i]->mMaterialIndex]        = mat;
                    local_mat_idx_mapping[Scene->mMeshes[i]->mMaterialIndex] = m_materials.size();

                    m_sub_meshes[i].mat_idx = m_materials.size();

                    m_materials.push_back(mat);
                }
                else if (has_diifuse_val)
                {
                    Material::Ptr mat = Material::load(
#if defined(DWSF_VULKAN)
                        backend,
#endif
                        current_mat_name,
                        0,
                        nullptr,
                        glm::vec4(diffuse.r, diffuse.g, diffuse.b, 1.0f));

                    mat_id_mapping[Scene->mMeshes[i]->mMaterialIndex]        = mat;
                    local_mat_idx_mapping[Scene->mMeshes[i]->mMaterialIndex] = m_materials.size();

                    m_sub_meshes[i].mat_idx = m_materials.size();

                    m_materials.push_back(mat);
                }
                else
                    m_sub_meshes[i].mat_idx = UINT32_MAX;
            }
            else // if already exists, find the pointer.
                m_sub_meshes[i].mat_idx = local_mat_idx_mapping[Scene->mMeshes[i]->mMaterialIndex];
        }
    }

    m_vertices = new Vertex[m_vertex_count];
    m_indices  = new uint32_t[m_index_count];

    std::vector<uint32_t> temp_indices(m_index_count);

    aiMesh* temp_mesh;
    int     idx         = 0;
    int     vertexIndex = 0;

    // Iterate over submeshes...
    for (int i = 0; i < m_sub_mesh_count; i++)
    {
        temp_mesh                   = Scene->mMeshes[i];
        m_sub_meshes[i].max_extents = glm::vec3(temp_mesh->mVertices[0].x, temp_mesh->mVertices[0].y, temp_mesh->mVertices[0].z);
        m_sub_meshes[i].min_extents = glm::vec3(temp_mesh->mVertices[0].x, temp_mesh->mVertices[0].y, temp_mesh->mVertices[0].z);

        uint32_t mat_id = 0;

        if (mat_id_mapping[Scene->mMeshes[i]->mMaterialIndex])
            mat_id = mat_id_mapping[Scene->mMeshes[i]->mMaterialIndex]->id();
        mat_id = m_sub_meshes[i].mat_idx;

        // Iterate over vertices in submesh...
        for (int k = 0; k < Scene->mMeshes[i]->mNumVertices; k++)
        {
            // Assign vertex values.
            m_vertices[vertexIndex].position = glm::vec4(temp_mesh->mVertices[k].x, temp_mesh->mVertices[k].y, temp_mesh->mVertices[k].z, float(mat_id));
            glm::vec3 n                      = glm::vec3(temp_mesh->mNormals[k].x, temp_mesh->mNormals[k].y, temp_mesh->mNormals[k].z);
            m_vertices[vertexIndex].normal   = glm::vec4(n, 0.0f);

            if (temp_mesh->mTangents && temp_mesh->mBitangents)
            {
                glm::vec3 t = glm::vec3(temp_mesh->mTangents[k].x, temp_mesh->mTangents[k].y, temp_mesh->mTangents[k].z);
                glm::vec3 b = glm::vec3(temp_mesh->mBitangents[k].x, temp_mesh->mBitangents[k].y, temp_mesh->mBitangents[k].z);

                // Assuming right handed coordinate space
                if (glm::dot(glm::cross(n, t), b) < 0.0f)
                    t *= -1.0f; // Flip tangent

                m_vertices[vertexIndex].tangent   = glm::vec4(t, 0.0f);
                m_vertices[vertexIndex].bitangent = glm::vec4(b, 0.0f);
            }

            // Find submesh bounding box extents.
            if (m_vertices[vertexIndex].position.x > m_sub_meshes[i].max_extents.x)
                m_sub_meshes[i].max_extents.x = m_vertices[vertexIndex].position.x;
            if (m_vertices[vertexIndex].position.y > m_sub_meshes[i].max_extents.y)
                m_sub_meshes[i].max_extents.y = m_vertices[vertexIndex].position.y;
            if (m_vertices[vertexIndex].position.z > m_sub_meshes[i].max_extents.z)
                m_sub_meshes[i].max_extents.z = m_vertices[vertexIndex].position.z;

            if (m_vertices[vertexIndex].position.x < m_sub_meshes[i].min_extents.x)
                m_sub_meshes[i].min_extents.x = m_vertices[vertexIndex].position.x;
            if (m_vertices[vertexIndex].position.y < m_sub_meshes[i].min_extents.y)
                m_sub_meshes[i].min_extents.y = m_vertices[vertexIndex].position.y;
            if (m_vertices[vertexIndex].position.z < m_sub_meshes[i].min_extents.z)
                m_sub_meshes[i].min_extents.z = m_vertices[vertexIndex].position.z;

            // Assign texture coordinates if it has any. Only the first channel is considered.
            if (temp_mesh->HasTextureCoords(0))
                m_vertices[vertexIndex].tex_coord = glm::vec4(temp_mesh->mTextureCoords[0][k].x, temp_mesh->mTextureCoords[0][k].y, 0.0f, 0.0f);

            vertexIndex++;
        }

        // Assign indices.
        for (int j = 0; j < temp_mesh->mNumFaces; j++)
        {
            temp_indices[idx] = temp_mesh->mFaces[j].mIndices[0];
            idx++;
            temp_indices[idx] = temp_mesh->mFaces[j].mIndices[1];
            idx++;
            temp_indices[idx] = temp_mesh->mFaces[j].mIndices[2];
            idx++;
        }
    }

    int count = 0;

    for (int i = 0; i < m_sub_mesh_count; i++)
    {
        SubMesh& submesh = m_sub_meshes[i];

        for (int idx = submesh.base_index; idx < (submesh.base_index + submesh.index_count); idx++)
            m_indices[count++] = submesh.base_vertex + temp_indices[idx];

        submesh.base_vertex = 0;
    }

    m_max_extents = m_sub_meshes[0].max_extents;
    m_min_extents = m_sub_meshes[0].min_extents;

    // Find bounding box extents of entire mesh.
    for (int i = 0; i < m_sub_mesh_count; i++)
    {
        if (m_sub_meshes[i].max_extents.x > m_max_extents.x)
            m_max_extents.x = m_sub_meshes[i].max_extents.x;
        if (m_sub_meshes[i].max_extents.y > m_max_extents.y)
            m_max_extents.y = m_sub_meshes[i].max_extents.y;
        if (m_sub_meshes[i].max_extents.z > m_max_extents.z)
            m_max_extents.z = m_sub_meshes[i].max_extents.z;

        if (m_sub_meshes[i].min_extents.x < m_min_extents.x)
            m_min_extents.x = m_sub_meshes[i].min_extents.x;
        if (m_sub_meshes[i].min_extents.y < m_min_extents.y)
            m_min_extents.y = m_sub_meshes[i].min_extents.y;
        if (m_sub_meshes[i].min_extents.z < m_min_extents.z)
            m_min_extents.z = m_sub_meshes[i].min_extents.z;
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Mesh::create_gpu_objects(
#if defined(DWSF_VULKAN)
    vk::Backend::Ptr backend
#endif
)
{
#if defined(DWSF_VULKAN)
    m_vbo = vk::Buffer::create(backend, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(Vertex) * m_vertex_count, VMA_MEMORY_USAGE_GPU_ONLY, 0, &m_vertices[0]);
    m_ibo = vk::Buffer::create(backend, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(uint32_t) * m_index_count, VMA_MEMORY_USAGE_GPU_ONLY, 0, &m_indices[0]);

    m_vertex_input_state_desc.add_binding_desc(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX);

    m_vertex_input_state_desc.add_attribute_desc(0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0);
    m_vertex_input_state_desc.add_attribute_desc(1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, tex_coord));
    m_vertex_input_state_desc.add_attribute_desc(2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, normal));
    m_vertex_input_state_desc.add_attribute_desc(3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, tangent));
    m_vertex_input_state_desc.add_attribute_desc(4, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, bitangent));
#else
    // Create vertex buffer.
    m_vbo = std::make_unique<gl::VertexBuffer>(
        GL_STATIC_DRAW, sizeof(Vertex) * m_vertex_count, m_vertices);

    if (!m_vbo)
        DW_LOG_ERROR("Failed to create Vertex Buffer");

    // Create index buffer.
    m_ibo = std::make_unique<gl::IndexBuffer>(
        GL_STATIC_DRAW, sizeof(uint32_t) * m_index_count, m_indices);

    if (!m_ibo)
        DW_LOG_ERROR("Failed to create Index Buffer");

    // Declare vertex attributes.
    gl::VertexAttrib attribs[] = { { 4, GL_FLOAT, false, 0 },
                                   { 4, GL_FLOAT, false, offsetof(Vertex, tex_coord) },
                                   { 4, GL_FLOAT, false, offsetof(Vertex, normal) },
                                   { 4, GL_FLOAT, false, offsetof(Vertex, tangent) },
                                   { 4, GL_FLOAT, false, offsetof(Vertex, bitangent) } };

    // Create vertex array.
    m_vao = std::make_unique<gl::VertexArray>(m_vbo.get(), m_ibo.get(), sizeof(Vertex), 5, attribs);

    if (!m_vao)
        DW_LOG_ERROR("Failed to create Vertex Array");
#endif
}

// -----------------------------------------------------------------------------------------------------------------------------------

Mesh::Mesh()
{
    m_id = g_last_mesh_idx++;
}

// -----------------------------------------------------------------------------------------------------------------------------------

Mesh::Mesh(
#if defined(DWSF_VULKAN)
    vk::Backend::Ptr backend,
#endif
    const std::string& path,
    bool               load_materials)
{
    m_id = g_last_mesh_idx++;

    load_from_disk(
#if defined(DWSF_VULKAN)
        backend,
#endif
        path,
        load_materials);
    create_gpu_objects(
#if defined(DWSF_VULKAN)
        backend
#endif
    );
}

// -----------------------------------------------------------------------------------------------------------------------------------

Mesh::~Mesh()
{
    // Unload submesh materials.
    for (uint32_t i = 0; i < m_materials.size(); i++)
        m_materials[i].reset();

    m_ibo.reset();
    m_vbo.reset();

    // Delete geometry data.
    DW_SAFE_DELETE_ARRAY(m_sub_meshes);
    DW_SAFE_DELETE_ARRAY(m_vertices);
    DW_SAFE_DELETE_ARRAY(m_indices);
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Mesh::set_submesh_material(std::string name, std::shared_ptr<Material> material)
{
    for (int i = 0; i < m_sub_mesh_count; i++)
    {
        if (name == m_sub_meshes[i].name)
        {
            m_sub_meshes[i].mat_idx = m_materials.size();
            m_materials.push_back(material);

            return true;
        }
    }

    return false;
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Mesh::set_submesh_material(uint32_t mesh_idx, std::shared_ptr<Material> material)
{
    if (mesh_idx >= m_sub_mesh_count)
        return false;

    m_sub_meshes[mesh_idx].mat_idx = m_materials.size();
    m_materials.push_back(material);

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Mesh::set_global_material(std::shared_ptr<Material> material)
{
    for (int i = 0; i < m_sub_mesh_count; i++)
        m_sub_meshes[i].mat_idx = m_materials.size();

    m_materials.push_back(material);
}

// -----------------------------------------------------------------------------------------------------------------------------------
// Assimp loader helper method definitions
// -----------------------------------------------------------------------------------------------------------------------------------

std::string assimp_get_texture_path(aiMaterial*   material,
                                    aiTextureType texture_type)
{
    aiString path;
    aiReturn result = material->GetTexture(texture_type, 0, &path);

    if (result == aiReturn_FAILURE)
        return "";
    else
    {
        std::string cppStr = std::string(path.C_Str());

        if (cppStr == "")
            return "";

        return cppStr;
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool assimp_does_material_exist(std::vector<unsigned int>& materials,
                                unsigned int&              current_material)
{
    for (auto it : materials)
    {
        if (it == current_material)
            return true;
    }

    return false;
}

// -----------------------------------------------------------------------------------------------------------------------------------
} // namespace dw
