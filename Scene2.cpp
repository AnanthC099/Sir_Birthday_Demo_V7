#include <string.h>

#include "SceneSwitcher.h"
#include "Scenes.h"

VkShaderModule gShaderModule_vertex_scene2 = VK_NULL_HANDLE;
VkShaderModule gShaderModule_fragment_scene2 = VK_NULL_HANDLE;

VkResult Scene2_UpdateUniformBuffer(void)
{
    GlobalContext_MyUniformData scene2;
    Scenes_InitUniformIdentity(&scene2);

    return Scenes_WriteUniformData(gCtx_Switcher.vkDevice, gCtx_Switcher.uniformData_scene2.vkDeviceMemory, &scene2, gCtx_Switcher.gpFile, "Scene2_UpdateUniformBuffer()");
}

