#pragma once

#include <stdint.h>
#include <string>

namespace dw
{
	class Scene;
	class Camera;
	class RenderDevice;
	class Renderer;
	class RenderNode;

#define MAX_RENDER_NODES 32

	class RenderGraph
	{
	public:
		static RenderGraph* load(std::string json, 
								 RenderDevice* device, 
								 Renderer* renderer, 
								 uint16_t w, 
								 uint16_t h);

		void execute(Scene* scene, Camera* camera);
		void create_render_targets(uint16_t w, uint16_t h);

	private:
		int m_num_nodes;
		RenderNode* m_nodes[MAX_RENDER_NODES];
	};
}