#pragma once

#include <string>

class RenderDevice;
struct Texture;

namespace trm
{
	extern Texture* load_image(std::string file, int format, RenderDevice* device);
}