#pragma once

#include <imgui.h>
#include <render_device.h>

namespace dw
{
#if defined(TE_RENDER_DEVICE_GL)
	inline void imageWithTexture(Texture2D* texture, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0))
	{
		ImGui::Image((ImTextureID)texture->id, size, uv0, uv1, tint_col, border_col);
	}
#endif
}
