#include <string.h>

#include "SceneSwitcher.h"
#include "Scenes.h"

VkShaderModule gShaderModule_vertex_scene3 = VK_NULL_HANDLE;
VkShaderModule gShaderModule_fragment_scene3 = VK_NULL_HANDLE;

VkResult Scene3_UpdateUniformBuffer(void)
{
    GlobalContext_MyUniformData scene3;
    Scenes_InitUniformIdentity(&scene3);

    return Scenes_WriteUniformData(gCtx_Switcher.vkDevice, gCtx_Switcher.uniformData_scene3.vkDeviceMemory, &scene3, gCtx_Switcher.gpFile, "Scene3_UpdateUniformBuffer()");
}
