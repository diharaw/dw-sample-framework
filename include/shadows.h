#pragma once

#include <glm.hpp>

#define MAX_FRUSTUM_SPLITS 8

class Camera;
class RenderDevice;
struct Texture2D;
struct Framebuffer;

struct FrustumSplit
{
	float near_plane;
	float far_plane;
	float ratio;
	float fov;
	glm::vec3 corners[8];
};

struct ShadowSettings
{
	float lambda;
	float near_offset;
	int split_count;
	int shadow_map_size;
};

class Shadows
{
private:
	//Texture2D* m_shadow_maps;
	Texture2D* m_shadow_maps;
	Framebuffer* m_shadow_fbos[MAX_FRUSTUM_SPLITS];
	ShadowSettings m_settings;
	FrustumSplit m_splits[MAX_FRUSTUM_SPLITS];
	RenderDevice* m_device;
	glm::mat4 m_light_view;
	glm::mat4 m_crop_matrices[MAX_FRUSTUM_SPLITS]; // crop * proj * view
	glm::mat4 m_proj_matrices[MAX_FRUSTUM_SPLITS]; // crop * proj * light_view * inv_view

public:
	bool m_stable_pssm = true;

	Shadows();
	~Shadows();
	void initialize(RenderDevice* device, ShadowSettings settings, Camera* camera, int _width, int _height, glm::vec3 dir);
	void update(Camera* camera, glm::vec3 dir);
	void update_splits(Camera* camera);
	void update_frustum_corners(Camera* camera);
	void update_crop_matrices(glm::mat4 t_modelview);
	FrustumSplit* frustum_splits();
	glm::mat4 split_view_proj(int i);

	inline Texture2D* shadow_map() { return m_shadow_maps; }
	inline Framebuffer** framebuffers() { return &m_shadow_fbos[0]; }
	inline uint32_t frustum_split_count() { return m_settings.split_count; }
	inline ShadowSettings settings() { return m_settings; }
};