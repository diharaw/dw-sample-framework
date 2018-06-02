#pragma once

namespace dw
{
	struct AssimpImportData
	{
		TSM_FileHeader      header;
		TSM_MeshHeader*     meshes;
		Assimp_Material*    materials;
		TSM_Vertex*			vertices;
		TSM_SkeletalVertex* skeletal_vertices;
		uint*			    indices;
		bool			    skeletal;
		std::string         mesh_path;
		std::string         filename;
	};


} // namespace dw