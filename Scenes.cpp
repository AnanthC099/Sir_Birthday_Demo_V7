#include "Scenes.h"

void Scenes_InitUniformIdentity(GlobalContext_MyUniformData* data)
{
    memset(data, 0, sizeof(GlobalContext_MyUniformData));
    data->modelMatrix = glm::mat4(1.0f);
    data->viewMatrix = glm::mat4(1.0f);
    data->projectionMatrix = glm::mat4(1.0f);
    data->fade = glm::vec4(0.0f);
    data->fireParams = glm::vec4(0.0f);
    data->fireScale = glm::vec4(0.0f);
    data->viewPosition = glm::vec4(0.0f); //!!! Imp !!!
}

VkResult Scenes_WriteUniformData(VkDevice device, VkDeviceMemory memory, const GlobalContext_MyUniformData* data, FILE* logFile, const char* tag)
{
    if (device == VK_NULL_HANDLE || memory == VK_NULL_HANDLE || data == NULL)
    {
        fprintf(logFile, "%s --> invalid uniform buffer parameters\n", tag);
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    void* mapped = NULL;
    VkResult vkResult = vkMapMemory(device, memory, 0, sizeof(GlobalContext_MyUniformData), 0, &mapped);
    if (vkResult != VK_SUCCESS)
    {
        fprintf(logFile, "%s --> vkMapMemory() failed %d\n", tag, vkResult);
        return vkResult;
    }

    if (mapped == NULL)
    {
        fprintf(logFile, "%s --> vkMapMemory() returned NULL pointer\n", tag);
        vkUnmapMemory(device, memory);
        return VK_ERROR_MEMORY_MAP_FAILED;
    }

    memcpy(mapped, data, sizeof(GlobalContext_MyUniformData));
    vkUnmapMemory(device, memory);
    return VK_SUCCESS;
}
