#pragma once 

#include "types.h"

struct TSM_Material
{
	char albedo[50];
	char normal[50];
	char roughness[50];
	char metalness[50];
	char displacement[50];
};

struct Assimp_Material
{
	char albedo[50];
	char normal[50];
	bool has_metalness;
	bool has_roughness;
	char metalness[50];
	char roughness[50];
	String mesh_name;
};

struct TSM_Material_Json
{
	char material[50];
};