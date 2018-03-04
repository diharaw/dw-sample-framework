#pragma once 

#include <stdint.h>
#include <glm.hpp>

struct TSM_FileHeader
{
	uint8_t   meshType;
	uint16_t  meshCount;
	uint16_t  materialCount;
	uint32_t  vertexCount;
	uint32_t  indexCount;
	glm::vec3 maxExtents;
	glm::vec3 minExtents;
	char 	name[50];
};

struct TSM_MeshHeader
{
	uint8_t materialIndex;
	uint32_t indexCount;
	uint32_t baseVertex;
	uint32_t baseIndex;
	glm::vec3  maxExtents;
	glm::vec3  minExtents;
};

struct TSM_Material_Json
{
	char material[50];
};