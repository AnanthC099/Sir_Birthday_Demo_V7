#pragma once

#include "SceneSwitcher.h"
#include <string.h>

typedef enum ActiveSceneTag
{
    ACTIVE_SCENE_NONE   = -1,
    ACTIVE_SCENE_SCENE0 = 0,
    ACTIVE_SCENE_SCENE1 = 1,
    ACTIVE_SCENE_SCENE2 = 2,
    ACTIVE_SCENE_SCENE3 = 3,
    ACTIVE_SCENE_SCENE4 = 4,
    ACTIVE_SCENE_SCENE2_SCENE4 = 5
} ActiveScene;

extern ActiveScene gActiveScene;

void Scenes_InitUniformIdentity(GlobalContext_MyUniformData* data);

VkResult Scenes_WriteUniformData(VkDevice device, VkDeviceMemory memory, const GlobalContext_MyUniformData* data, FILE* logFile, const char* tag);

	// Scene 0 sathi
	VkResult Scene0_UpdateUniformBuffer(void);

	// Scene 1 sathi
	extern VkShaderModule gShaderModule_vertex_scene1;
	extern VkShaderModule gShaderModule_fragment_scene1;

	// Scene 2 sathi
	extern VkShaderModule gShaderModule_vertex_scene2;
	extern VkShaderModule gShaderModule_fragment_scene2;

	// Scene 3 sathi
	extern VkShaderModule gShaderModule_vertex_scene3;
	extern VkShaderModule gShaderModule_fragment_scene3;

	// Scene 4 sathi
	extern VkShaderModule gShaderModule_vertex_scene4;
	extern VkShaderModule gShaderModule_fragment_scene4;

	// Scene 1 cha Quaternion  ani textures sathi lagnaari aahet
	VkResult Scene1_CreateTextures(void);
	void Scene1_DestroyTextures(void);
	void Scene1_BeginNewPan(void);
	void Scene1_UpdateCameraAnim(void);
	VkResult Scene1_UpdateUniformBuffer(void);
	void Scene1_StartSequence(void);
	void Scene1_StopSequence(void);
	void Scene1_UpdateSequence(void);
	BOOL Scene1_IsSequenceActive(void);
	void Scene1_AdjustPanSpeed(float delta);
	void Scene1_AdjustOverlaySize(float delta);
	float Scene1_GetPendingBlendFade(void);
	void Scene1_CommitPendingBlendFade(void);
	void Scene1_UpdateBlendFade(float fade);
	BOOL Scene1_IsCommandBufferDirty(void);
	void Scene1_ClearCommandBufferDirty(void);
	void Scene1_MarkCommandBufferDirty(void);
	BOOL Scene1_HasPendingOverlay(void);
	VkResult Scene1_BindPendingOverlay(VkDescriptorSet descriptorSet);
	void Scene1_GetSkyDescriptor(VkDescriptorImageInfo* info);
	void Scene1_GetOverlayDescriptor(VkDescriptorImageInfo* info);

	// Scene2 sathi
	VkResult Scene2_UpdateUniformBuffer(void);

	//  Scene3 sathi
	VkResult Scene3_UpdateUniformBuffer(void);

	// Scene4 sathi
	void Scene4_ResetState(void);
	VkResult Scene4_CreateTextures(void);
	void Scene4_DestroyTextures(void);
	VkResult Scene4_CreateVertexResources(void);
	void Scene4_DestroyVertexResources(void);
	VkResult Scene4_UpdateGeometryBuffers(void);
	void Scene4_Tick(void);
	BOOL Scene4_IsGeometryDirty(void);
	VkResult Scene4_UpdateUniformBuffer(void);

	BOOL Scene4_AdjustFireSliceCount(int direction, BOOL* outRequiresRebuild);
	BOOL Scene4_AdjustFireRadius(int direction);
	BOOL Scene4_AdjustFireHeight(int direction);
	BOOL Scene4_AdjustFireScaleX(int direction);
	BOOL Scene4_AdjustFireScaleY(int direction);
	BOOL Scene4_AdjustFireScaleZ(int direction);
	BOOL Scene4_AdjustFireScrollSpeed(int direction);
	BOOL Scene4_AdjustFireTurbulence(int direction);
	BOOL Scene4_AdjustFireTimeSpeed(int direction);
	BOOL Scene4_AdjustFireOffsetX(int direction);
	BOOL Scene4_AdjustFireOffsetY(int direction);
	BOOL Scene4_AdjustFireOffsetZ(int direction);
	BOOL Scene4_ResetFireParameters(BOOL* outRequiresRebuild);


