#pragma once

#include <stdint.h>
#include <string>

struct Texture;
struct Framebuffer;

namespace dw
{
	class Scene;
	class Camera;
	class RenderDevice;
	class Renderer;

	class RenderNode
	{
	public:
		struct Input
		{
			std::string id;
			Texture* texture;
			int binding;
			int format;
		};

		static RenderNode* load(std::string json,
								RenderDevice* device,
								Renderer* renderer,
								uint16_t w,
								uint16_t h);

		void execute(Scene* scene, Camera* camera);
		void create_render_targets(uint16_t w, uint16_t h);

	private:
		Framebuffer* m_fbo;
		int			 m_num_render_target_inputs;
		Input		 m_render_target_inputs[8];
		int			 m_num_texture_inputs;
		Input		 m_texture_inputs[8];
	};
}