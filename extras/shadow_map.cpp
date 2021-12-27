#include "shadow_map.h"
#include <gtc/matrix_transform.hpp>
#include <macros.h>
#include <imgui.h>

#if defined(DWSF_VULKAN)
#    include <vk_mem_alloc.h>
#endif

namespace dw
{
// -----------------------------------------------------------------------------------------------------------------------------------

ShadowMap::ShadowMap(
#if defined(DWSF_VULKAN)
    vk::Backend::Ptr backend,
#endif
    uint32_t size) :
    m_size(size)
{
#if defined(DWSF_VULKAN)
    m_image = vk::Image::create(backend, VK_IMAGE_TYPE_2D, m_size, m_size, 1, 1, 1, VK_FORMAT_D32_SFLOAT, VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED);
    m_image->set_name("Shadow Map Image");

    m_image_view = vk::ImageView::create(backend, m_image, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1);
    m_image_view->set_name("Shadow Map Image View");

    VkAttachmentDescription attachment;
    DW_ZERO_MEMORY(attachment);

    // Depth attachment
    attachment.format         = VK_FORMAT_D32_SFLOAT;
    attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    VkAttachmentReference depth_reference;
    depth_reference.attachment = 0;
    depth_reference.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::vector<VkSubpassDescription> subpass_description(1);

    subpass_description[0].pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description[0].colorAttachmentCount    = 0;
    subpass_description[0].pColorAttachments       = nullptr;
    subpass_description[0].pDepthStencilAttachment = &depth_reference;
    subpass_description[0].inputAttachmentCount    = 0;
    subpass_description[0].pInputAttachments       = nullptr;
    subpass_description[0].preserveAttachmentCount = 0;
    subpass_description[0].pPreserveAttachments    = nullptr;
    subpass_description[0].pResolveAttachments     = nullptr;

    // Subpass dependencies for layout transitions
    std::vector<VkSubpassDependency> dependencies(2);

    dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass      = 0;
    dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass      = 0;
    dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    m_render_pass = vk::RenderPass::create(backend, { attachment }, subpass_description, dependencies);
    m_framebuffer = vk::Framebuffer::create(backend, m_render_pass, { m_image_view }, m_size, m_size, 1);
#else
    m_shadow_map     = gl::Texture2D::create(m_size, m_size, 1, 1, 1, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT);
    m_shadow_map_fbo = gl::Framebuffer::create({}, m_shadow_map);
#endif

    update();
}

// -----------------------------------------------------------------------------------------------------------------------------------

ShadowMap::~ShadowMap()
{
#if defined(DWSF_VULKAN)
    m_framebuffer.reset();
    m_render_pass.reset();
    m_image_view.reset();
    m_image.reset();
#else
    m_shadow_map_fbo.reset();
    m_shadow_map.reset();
#endif
}

// -----------------------------------------------------------------------------------------------------------------------------------

void ShadowMap::begin_render(
#if defined(DWSF_VULKAN)
    vk::CommandBuffer::Ptr cmd_buf
#endif
)
{
#if defined(DWSF_VULKAN)
    VkClearValue clear_value;

    clear_value.depthStencil.depth = 1.0f;

    VkRenderPassBeginInfo info    = {};
    info.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.renderPass               = m_render_pass->handle();
    info.framebuffer              = m_framebuffer->handle();
    info.renderArea.extent.width  = m_size;
    info.renderArea.extent.height = m_size;
    info.clearValueCount          = 1;
    info.pClearValues             = &clear_value;

    vkCmdBeginRenderPass(cmd_buf->handle(), &info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport vp;

    vp.x        = 0.0f;
    vp.y        = 0.0f;
    vp.width    = (float)m_size;
    vp.height   = (float)m_size;
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;

    vkCmdSetViewport(cmd_buf->handle(), 0, 1, &vp);

    VkRect2D scissor_rect;

    scissor_rect.extent.width  = m_size;
    scissor_rect.extent.height = m_size;
    scissor_rect.offset.x      = 0;
    scissor_rect.offset.y      = 0;

    vkCmdSetScissor(cmd_buf->handle(), 0, 1, &scissor_rect);
#else
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    m_shadow_map_fbo->bind();

    glViewport(0, 0, m_size, m_size);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(1.0);
    glClear(GL_DEPTH_BUFFER_BIT);
#endif
}

// -----------------------------------------------------------------------------------------------------------------------------------

void ShadowMap::end_render(
#if defined(DWSF_VULKAN)
    vk::CommandBuffer::Ptr cmd_buf
#endif
)
{
#if defined(DWSF_VULKAN)
    vkCmdEndRenderPass(cmd_buf->handle());
#else
    glEnable(GL_CULL_FACE);
#endif
}

// -----------------------------------------------------------------------------------------------------------------------------------

void ShadowMap::gui()
{
    ImGui::SliderFloat("Extents", &m_extents, 1.0f, 10000.0f);
    ImGui::SliderFloat("Near Plane", &m_near_plane, 1.0f, 10000.0f);
    ImGui::SliderFloat("Far Plane", &m_far_plane, 1.0f, 10000.0f);
    ImGui::SliderFloat("Back Off Distance", &m_backoff_distance, 1.0f, 10000.0f);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void ShadowMap::set_direction(const glm::vec3& d)
{
    m_light_direction = d;
    update();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void ShadowMap::set_target(const glm::vec3& t)
{
    m_light_target = t;
    update();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void ShadowMap::set_color(const glm::vec3& c)
{
    m_light_color = c;
    update();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void ShadowMap::set_near_plane(const float& n)
{
    m_near_plane = n;
    update();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void ShadowMap::set_far_plane(const float& f)
{
    m_far_plane = f;
    update();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void ShadowMap::set_extents(const float& e)
{
    m_extents = e;
    update();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void ShadowMap::set_backoff_distance(const float& d)
{
    m_backoff_distance = d;
    update();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void ShadowMap::update()
{
    glm::vec3 light_camera_pos = m_light_target - m_light_direction * m_backoff_distance;
    m_view                     = glm::lookAt(light_camera_pos, m_light_target, glm::vec3(0.0f, 1.0f, 0.0f));
    m_projection               = glm::ortho(-m_extents, m_extents, -m_extents, m_extents, m_near_plane, m_far_plane);
}

// -----------------------------------------------------------------------------------------------------------------------------------
} // namespace dw