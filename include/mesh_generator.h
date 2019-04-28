#pragma once

#include <stdint.h>
#include <glm.hpp>

struct VertexBuffer;
struct IndexBuffer;
struct VertexArray;
struct InputLayout;

namespace dw
{
struct PrimitiveMesh
{
    VertexBuffer* vb;
    IndexBuffer*  ib;
    VertexArray*  va;
    InputLayout*  il;
    uint16_t      index_count;
    uint16_t      vertex_count;
    glm::vec3     position;
};

struct CubeMesh : public PrimitiveMesh
{
    glm::vec3 local_min;
    glm::vec3 local_max;
};

struct SphereMesh : public PrimitiveMesh
{
    float radius;
};

struct PlaneMesh : public PrimitiveMesh
{
    glm::vec2 size;
};

namespace mesh_generator
{
}
} // namespace dw