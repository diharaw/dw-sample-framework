#pragma once

#include <vk.h>
#include <ogl.h>

namespace dw
{
class BRDFIntegrateLUT
{
public:
    BRDFIntegrateLUT(
#if defined(DWSF_VULKAN)
        vk::Backend::Ptr backend
#endif 
    );
    ~BRDFIntegrateLUT();

#if defined(DWSF_VULKAN)
    inline vk::Image::Ptr image() { return m_image; }
    inline vk::ImageView::Ptr image_view() { return m_image_view; }
#else
    inline gl::Texture2D::Ptr texture() { return m_texture; }
#endif

private:
#if defined(DWSF_VULKAN)
    vk::Image::Ptr               m_image;
    vk::ImageView::Ptr           m_image_view;
#else
    gl::Texture2D::Ptr m_texture;
#endif
};
} // namespace dw