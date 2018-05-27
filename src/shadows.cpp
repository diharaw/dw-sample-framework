#include "shadows.h"
#include <camera.h>
#include <render_device.h>
#include <gtc/matrix_transform.hpp>
#include <Macros.h>

Shadows::Shadows()
{
	m_shadow_maps = nullptr;

	for (int i = 0; i < 8; i++)
	{
		m_shadow_fbos[i] = nullptr;
	}
}

Shadows::~Shadows()
{
	for (int i = 0; i < 8; i++)
	{
		if (m_shadow_fbos[i])
			m_device->destroy(m_shadow_fbos[i]);
	}

	m_device->destroy(m_shadow_maps);
}

FrustumSplit* Shadows::frustum_splits()
{
	return &m_splits[0];
}

glm::mat4 Shadows::split_view_proj(int i)
{
	return m_crop_matrices[i];
}

void Shadows::initialize(RenderDevice* device, ShadowSettings settings, Camera* camera, int _width, int _height, glm::vec3 dir)
{
	m_device = device;
	m_settings = settings;

	m_device->destroy(m_shadow_maps);

	for (int i = 0; i < 8; i++)
	{
		if (m_shadow_fbos[i])
			m_device->destroy(m_shadow_fbos[i]);
	}

	Texture2DArrayCreateDesc desc;
	DW_ZERO_MEMORY(desc);

	desc.array_slices = m_settings.split_count;
	desc.format = TextureFormat::D32_FLOAT_S8_UINT;
	desc.height = m_settings.shadow_map_size;
	desc.width = m_settings.shadow_map_size;
	desc.mipmap_levels = 1;
	
	m_shadow_maps = device->create_texture_2d_array(desc);

	for (int i = 0; i < m_settings.split_count; i++)
	{
		DepthStencilTargetDesc ds_desc;

		ds_desc.arraySlice = i;
		ds_desc.mipSlice = 0;
		ds_desc.texture = m_shadow_maps;
		
		FramebufferCreateDesc fbo_desc;

		fbo_desc.renderTargetCount = 0;
		fbo_desc.depthStencilTarget = ds_desc;

		m_shadow_fbos[i] = device->create_framebuffer(fbo_desc);
	}

	float camera_fov = camera->m_fov;
	float width = _width;
	float height = _height;
	float ratio = width / height;

	// note that fov is in radians here and in OpenGL it is in degrees.
	// the 0.2f factor is important because we might get artifacts at
	// the screen borders.
	for (int i = 0; i < m_settings.split_count; i++) 
	{
		m_splits[i].fov = camera_fov / 57.2957795 + 0.2f;
		m_splits[i].ratio = ratio;
	}

	update(camera, dir);
}

void Shadows::update(Camera* camera, glm::vec3 dir)
{
	dir = glm::normalize(dir);

	glm::vec3 center = camera->m_position + camera->m_forward * 50.0f;
	glm::vec3 light_pos = center - dir * ((camera->m_far - camera->m_near) / 2.0f);
	glm::vec3 right = glm::cross(dir, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::vec3 up = glm::cross(right, dir);

	glm::mat4 modelview = glm::lookAt(light_pos, center, up);

	m_light_view = modelview;

	update_splits(camera);
	update_frustum_corners(camera);
	update_crop_matrices(modelview);
}

void Shadows::update_splits(Camera* camera)
{
	float nd = camera->m_near;
	float fd = camera->m_far;

	float lambda = m_settings.lambda;
	float ratio = fd / nd;
	m_splits[0].near_plane = nd;

	for (int i = 1; i < m_settings.split_count; i++)
	{
		float si = i / (float)m_settings.split_count;

		// Practical Split Scheme: https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
		float t_near = lambda * (nd * powf(ratio, si)) + (1 - lambda) * (nd + (fd - nd) * si);
		float t_far = t_near * 1.005f;
		m_splits[i].near_plane = t_near;
		m_splits[i - 1].far_plane = t_far;
	}

	m_splits[m_settings.split_count - 1].far_plane = fd;
}

void Shadows::update_frustum_corners(Camera* camera)
{
	glm::vec3 center = camera->m_position;
	glm::vec3 view_dir = camera->m_forward;

	glm::vec3 up(0.0f, 1.0f, 0.0f);
	glm::vec3 right = glm::cross(view_dir, up);

	for (int i = 0; i < m_settings.split_count; i++)
	{
		FrustumSplit& t_frustum = m_splits[i];

		glm::vec3 fc = center + view_dir * t_frustum.far_plane;
		glm::vec3 nc = center + view_dir * t_frustum.near_plane;

		right = glm::normalize(right);
		up = glm::normalize(glm::cross(right, view_dir));

		// these heights and widths are half the heights and widths of
		// the near and far plane rectangles
		float near_height = tan(t_frustum.fov / 2.0f) * t_frustum.near_plane;
		float near_width = near_height * t_frustum.ratio;
		float far_height = tan(t_frustum.fov / 2.0f) * t_frustum.far_plane;
		float far_width = far_height * t_frustum.ratio;

		t_frustum.corners[0] = nc - up * near_height - right * near_width; // near-bottom-left
		t_frustum.corners[1] = nc + up * near_height - right * near_width; // near-top-left
		t_frustum.corners[2] = nc + up * near_height + right * near_width; // near-top-right
		t_frustum.corners[3] = nc - up * near_height + right * near_width; // near-bottom-right

		t_frustum.corners[4] = fc - up * far_height - right * far_width; // far-bottom-left
		t_frustum.corners[5] = fc + up * far_height - right * far_width; // far-top-left
		t_frustum.corners[6] = fc + up * far_height + right * far_width; // far-top-right
		t_frustum.corners[7] = fc - up * far_height + right * far_width; // far-bottom-right
	}
}

void Shadows::update_crop_matrices(glm::mat4 t_modelview)
{
	glm::mat4 t_projection;
	for (int i = 0; i < m_settings.split_count; i++) 
	{
		FrustumSplit& t_frustum = m_splits[i];

		glm::vec3 tmax(-INFINITY, -INFINITY, -INFINITY);
		glm::vec3 tmin(INFINITY, INFINITY, INFINITY);

		// find the z-range of the current frustum as seen from the light
		// in order to increase precision

		// note that only the z-component is need and thus
		// the multiplication can be simplified
		// transf.z = shad_modelview[2] * f.point[0].x + shad_modelview[6] * f.point[0].y + shad_modelview[10] * f.point[0].z + shad_modelview[14];
		glm::vec4 t_transf = t_modelview * glm::vec4(t_frustum.corners[0], 1.0f);

		tmin.z = t_transf.z;
		tmax.z = t_transf.z;
		for (int j = 1; j < 8; j++) 
		{
			t_transf = t_modelview * glm::vec4(t_frustum.corners[j], 1.0f);
			if (t_transf.z > tmax.z) { tmax.z = t_transf.z; }
			if (t_transf.z < tmin.z) { tmin.z = t_transf.z; }
		}

		tmax.z += 50; // TODO: This solves the dissapearing shadow problem. but how to fix?

		glm::mat4 t_ortho = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -m_settings.near_offset, -tmin.z);
		glm::mat4 t_shad_mvp = t_ortho * t_modelview;

		if (m_stable_pssm)
		{
			// Calculate frustum split center
			glm::vec3 center(0.0f, 0.0f, 0.0f);

			for (int j = 0; j < 8; j++)
				center += t_frustum.corners[j];
			
			center /= 8.0f;

			// Calculate bounding sphere radius
			float radius = 0.0f;

			for (int j = 0; j < 8; j++)
			{
				float length = glm::length(t_frustum.corners[j] - center);
				radius = glm::max(radius, length);
			}

			radius = floor(radius);

			// Find bounding box that fits the sphere
			glm::vec3 radius3(radius, radius, radius);

			glm::vec4 max = glm::vec4(center + radius3, 1.0f);
			glm::vec4 min = glm::vec4(center - radius3, 1.0f);

			max = t_shad_mvp * max;
			max /= max.w;

			tmax = glm::vec3(max);

			min = t_shad_mvp * min;
			min /= min.w;

			tmin = glm::vec3(min);
		}
		else
		{
			// find the extends of the frustum slice as projected in light's homogeneous coordinates
			for (int j = 0; j < 8; j++)
			{
				t_transf = t_shad_mvp * glm::vec4(t_frustum.corners[j], 1.0f);

				t_transf.x /= t_transf.w;
				t_transf.y /= t_transf.w;

				if (t_transf.x > tmax.x) { tmax.x = t_transf.x; }
				if (t_transf.x < tmin.x) { tmin.x = t_transf.x; }
				if (t_transf.y > tmax.y) { tmax.y = t_transf.y; }
				if (t_transf.y < tmin.y) { tmin.y = t_transf.y; }
			}
		}
		
		glm::vec2 tscale(2.0f / (tmax.x - tmin.x), 2.0f / (tmax.y - tmin.y));
		glm::vec2 toffset(-0.5f * (tmax.x + tmin.x) * tscale.x, -0.5f * (tmax.y + tmin.y) * tscale.y);

		glm::mat4 t_shad_crop = glm::mat4(1.0f);
		t_shad_crop[0][0] = tscale.x;
		t_shad_crop[1][1] = tscale.y;
		t_shad_crop[0][3] = toffset.x;
		t_shad_crop[1][3] = toffset.y;
		t_shad_crop = glm::transpose(t_shad_crop);

		t_projection = t_shad_crop * t_ortho;

		// Store the projection matrix
		m_proj_matrices[i] = t_projection;
		m_crop_matrices[i] = t_projection * t_modelview;
	}
}