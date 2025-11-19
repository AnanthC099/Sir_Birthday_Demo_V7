#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <math.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/constants.hpp"

#include "SceneSwitcher.h"
#include "Scenes.h"

extern VkResult CreateTexture2D(const char* path, VkImage* outImg, VkDeviceMemory* outMem, VkImageView* outView, VkSampler* outSampler);

/* ------------------------- Scene 4 fire effect data ------------------------ */

const uint32_t SCENE4_SLICE_COUNT_DEFAULT = 8;
const uint32_t SCENE4_SLICE_COUNT_MIN     = 8;
const uint32_t SCENE4_SLICE_COUNT_MAX     = 256;
const uint32_t SCENE4_SLICE_COUNT_STEP    = 8;
const float    SCENE4_DEFAULT_RADIUS      = 1.25f;
const float    SCENE4_RADIUS_MIN          = 0.1f;
const float    SCENE4_RADIUS_MAX          = 3.0f;
const float    SCENE4_RADIUS_STEP         = 0.05f;
const float    SCENE4_DEFAULT_HEIGHT      = 4.2f;
const float    SCENE4_HEIGHT_MIN          = 0.5f;
const float    SCENE4_HEIGHT_MAX          = 5.0f;
const float    SCENE4_HEIGHT_STEP         = 0.1f;
const float    SCENE4_DEFAULT_SCALE_X     = 1.25f;
const float    SCENE4_DEFAULT_SCALE_Y     = 2.75f;
const float    SCENE4_DEFAULT_SCALE_Z     = 1.25f;
const float    SCENE4_SCALE_MIN           = 0.1f;
const float    SCENE4_SCALE_MAX           = 5.0f;
const float    SCENE4_SCALE_STEP          = 0.1f;
const float    SCENE4_DEFAULT_SCROLL      = 0.85f;
const float    SCENE4_SCROLL_MIN          = 0.0f;
const float    SCENE4_SCROLL_MAX          = 3.0f;
const float    SCENE4_SCROLL_STEP         = 0.05f;
const float    SCENE4_DEFAULT_TURBULENCE  = 2.0f;
 const float    SCENE4_TURBULENCE_MIN      = 0.0f;
 const float    SCENE4_TURBULENCE_MAX      = 3.0f;
 const float    SCENE4_TURBULENCE_STEP     = 0.1f;
 const float    SCENE4_DEFAULT_TIME_SPEED  = 0.02f;
 const float    SCENE4_TIME_SPEED_MIN      = 0.0f;
 const float    SCENE4_TIME_SPEED_MAX      = 5.0f;
 const float    SCENE4_TIME_SPEED_STEP     = 0.02f;
 const float    SCENE4_DEFAULT_OFFSET_X    = 0.0f;
 const float    SCENE4_DEFAULT_OFFSET_Y    = 0.0f;
 const float    SCENE4_DEFAULT_OFFSET_Z    = -10.0f;
 const float    SCENE4_OFFSET_X_MIN        = -10.0f;
 const float    SCENE4_OFFSET_X_MAX        =  10.0f;
 const float    SCENE4_OFFSET_Z_MIN        = -10.0f;
 const float    SCENE4_OFFSET_Z_MAX        =  10.0f;
 const float    SCENE4_OFFSET_Y_MIN        =  -2.0f;
 const float    SCENE4_OFFSET_Y_MAX        =   4.0f;
 const float    SCENE4_OFFSET_STEP_XZ      =  0.25f;
 const float    SCENE4_OFFSET_STEP_Y       =  0.25f;
 const float    SCENE4_BASE_DELTA_TIME     = 0.016f;
 const float    SCENE4_TIME_WRAP           = 1000.0f;
 const uint32_t SCENE4_FIRE_COLUMN_COUNT   = 3;
 const float    SCENE4_FIRE_DUPLICATE_GAP  = 6.2f;

uint32_t Scene4_ClampSliceCount(uint32_t sliceCount)
{
    if (sliceCount == 0)
    {
        return 0;
    }
    if (sliceCount < SCENE4_SLICE_COUNT_MIN)
    {
        return SCENE4_SLICE_COUNT_MIN;
    }
    if (sliceCount > SCENE4_SLICE_COUNT_MAX)
    {
        return SCENE4_SLICE_COUNT_MAX;
    }
    return sliceCount;
}

typedef struct Scene4Vec3ArrayTag
{
    glm::vec3* data;
    size_t size;
    size_t capacity;
} Scene4Vec3Array;

 void Scene4Vec3Array_Clear(Scene4Vec3Array* array)
{
    if (!array)
    {
        return;
    }
    array->size = 0;
}

 void Scene4Vec3Array_Free(Scene4Vec3Array* array)
{
    if (!array)
    {
        return;
    }
    if (array->data)
    {
        free(array->data);
        array->data = NULL;
    }
    array->size = 0;
    array->capacity = 0;
}

BOOL Scene4Vec3Array_Reserve(Scene4Vec3Array* array, size_t requiredCapacity)
{
    if (!array)
    {
        return FALSE;
    }
    if (requiredCapacity <= array->capacity)
    {
        return TRUE;
    }

    size_t newCapacity = (array->capacity == 0) ? 1U : array->capacity;
    while (newCapacity < requiredCapacity)
    {
        if (newCapacity > (SIZE_MAX / 2U))
        {
            newCapacity = requiredCapacity;
            break;
        }
        newCapacity *= 2U;
    }

    glm::vec3* newData = (glm::vec3*)realloc(array->data, newCapacity * sizeof(glm::vec3));
    if (!newData)
    {
        return FALSE;
    }

    array->data = newData;
    array->capacity = newCapacity;
    return TRUE;
}

BOOL Scene4Vec3Array_Push(Scene4Vec3Array* array, const glm::vec3* value)
{
    if (!array || !value)
    {
        return FALSE;
    }
    if (array->size >= array->capacity)
    {
        if (!Scene4Vec3Array_Reserve(array, array->size + 1U))
        {
            return FALSE;
        }
    }
    array->data[array->size] = *value;
    array->size += 1U;
    return TRUE;
}

const int SCENE4_EDGE_QUEUE_CAPACITY = 256;
const int SCENE4_FIRE_MAX_ACTIVE_EDGES = 12;
const int SCENE4_FIRE_CORNER_COUNT = 8;
const int SCENE4_FIRE_CORNER_NEIGHBOR_COUNT = 3;

typedef struct Scene4EdgeExpirationTag
{
    float priority;
    int index;
} Scene4EdgeExpiration;

typedef struct Scene4EdgeQueueTag
{
    Scene4EdgeExpiration entries[SCENE4_EDGE_QUEUE_CAPACITY];
    int count;
} Scene4EdgeQueue;

typedef struct Scene4FireActiveEdgeTag
{
    BOOL expired;
    int startIndex;
    int endIndex;
    glm::vec3 deltaPos;
    glm::vec3 deltaTex;
    glm::vec3 pos;
    glm::vec3 tex;
    int prev;
    int next;
} Scene4FireActiveEdge;

typedef struct Scene4StateTag
{
    Scene4Vec3Array positions;
    Scene4Vec3Array localCoords;
    uint32_t fireSliceCount;
    uint32_t vertexCapacity;
    uint32_t recordedVertexCount;
    float fireRadius;
    float fireHeight;
    float fireScaleX;
    float fireScaleY;
    float fireScaleZ;
    float fireScrollSpeed;
    float fireTurbulence;
    float fireTimeSpeed;
    float fireTime;
    float fireOffsetX;
    float fireOffsetY;
    float fireOffsetZ;
    glm::vec3 cameraPosition;
    VkDrawIndirectCommand drawCmd;
    BOOL geometryDirty;
} Scene4State;

//Given by Sachin Aher (Verify)
static Scene4State gScene4State = {
    /* positions        */ { NULL, 0U, 0U },
    /* localCoords      */ { NULL, 0U, 0U },
    /* fireSliceCount   */ SCENE4_SLICE_COUNT_DEFAULT,
    /* vertexCapacity   */ 0U,
    /* recordedVertexCount */ 0U,
    /* fireRadius       */ SCENE4_DEFAULT_RADIUS,
    /* fireHeight       */ SCENE4_DEFAULT_HEIGHT,
    /* fireScaleX       */ SCENE4_DEFAULT_SCALE_X,
    /* fireScaleY       */ SCENE4_DEFAULT_SCALE_Y,
    /* fireScaleZ       */ SCENE4_DEFAULT_SCALE_Z,
    /* fireScrollSpeed  */ SCENE4_DEFAULT_SCROLL,
    /* fireTurbulence   */ SCENE4_DEFAULT_TURBULENCE,
    /* fireTimeSpeed    */ SCENE4_DEFAULT_TIME_SPEED,
    /* fireTime         */ 0.0f,
    /* fireOffsetX      */ SCENE4_DEFAULT_OFFSET_X,
    /* fireOffsetY      */ SCENE4_DEFAULT_OFFSET_Y,
    /* fireOffsetZ      */ SCENE4_DEFAULT_OFFSET_Z,
    /* cameraPosition   */ glm::vec3(0.0f, 0.5f, 5.0f),
    /* drawCmd          */ { 0u, 1u, 0u, 0u },
    /* geometryDirty    */ TRUE
};

int Scene4_NormalizeDirection(int direction)
{
    if (direction > 0)
    {
        return 1;
    }
    if (direction < 0)
    {
        return -1;
    }
    return 0;
}

 BOOL Scene4_AdjustUint32Step(uint32_t* value,
                                    int direction,
                                    uint32_t step,
                                    uint32_t minValue,
                                    uint32_t maxValue)
{
    if (!value || step == 0)
    {
        return FALSE;
    }

    direction = Scene4_NormalizeDirection(direction);
    if (direction == 0)
    {
        return FALSE;
    }

    long long newValue = (long long)(*value) + ((long long)direction * (long long)step);
    if (newValue < (long long)minValue)
    {
        newValue = (long long)minValue;
    }
    if (newValue > (long long)maxValue)
    {
        newValue = (long long)maxValue;
    }

    if ((uint32_t)newValue == *value)
    {
        return FALSE;
    }

    *value = (uint32_t)newValue;
    return TRUE;
}

BOOL Scene4_AdjustFloatStep(float* value,
                                   int direction,
                                   float step,
                                   float minValue,
                                   float maxValue)
{
    if (!value || step <= 0.0f)
    {
        return FALSE;
    }

    direction = Scene4_NormalizeDirection(direction);
    if (direction == 0)
    {
        return FALSE;
    }

    float newValue = *value + ((float)direction * step);
    if (newValue < minValue)
    {
        newValue = minValue;
    }
    if (newValue > maxValue)
    {
        newValue = maxValue;
    }

    if (fabsf(newValue - *value) < 1.0e-6f)
    {
        return FALSE;
    }

    *value = newValue;
    return TRUE;
}

BOOL Scene4_SliceCountExceedsCapacity(uint32_t sliceCount)
{
    if (sliceCount == 0)
    {
        return FALSE;
    }
    if (gScene4State.vertexCapacity == 0)
    {
        return TRUE;
    }
    uint64_t requiredVertices = (uint64_t)sliceCount * 12ULL * (uint64_t)SCENE4_FIRE_COLUMN_COUNT;
    return requiredVertices > (uint64_t)gScene4State.vertexCapacity;
}

void Scene4_RequestGeometryRefresh(void)
{
    gScene4State.geometryDirty = TRUE;
}

void Scene4_LogParameterFloat(const char* label, float value)
{
    if (gCtx_Switcher.gpFile && label)
    {
        fprintf(gCtx_Switcher.gpFile, "Scene4 %s = %.3f\n", label, value);
    }
}

void Scene4_LogParameterUint(const char* label, uint32_t value)
{
    if (gCtx_Switcher.gpFile && label)
    {
        fprintf(gCtx_Switcher.gpFile, "Scene4 %s = %u\n", label, value);
    }
}

void Scene4_LogFireParameters(const char* tag)
{
    if (!gCtx_Switcher.gpFile)
    {
        return;
    }

    fprintf(gCtx_Switcher.gpFile,
              "%s -> slices=%u radius=%.2f height=%.2f scale=(%.2f, %.2f, %.2f) scroll=%.2f turbulence=%.2f timeSpeed=%.3f offset=(%.2f, %.2f, %.2f)\n",
            (tag != NULL) ? tag : "Scene4 fire params",
            gScene4State.fireSliceCount,
            gScene4State.fireRadius,
            gScene4State.fireHeight,
            gScene4State.fireScaleX,
            gScene4State.fireScaleY,
            gScene4State.fireScaleZ,
            gScene4State.fireScrollSpeed,
            gScene4State.fireTurbulence,
              gScene4State.fireTimeSpeed,
              gScene4State.fireOffsetX,
              gScene4State.fireOffsetY,
              gScene4State.fireOffsetZ);
}

glm::vec3 Scene4_GetFireColumnOffset(uint32_t columnIndex)
{
    if (columnIndex == 0)
    {
        return glm::vec3(0.0f);
    }

    float spacing = (gScene4State.fireRadius * 2.0f) + SCENE4_FIRE_DUPLICATE_GAP;
    if (spacing < 0.0f)
    {
        spacing = 0.0f;
    }
    uint32_t pairIndex = (columnIndex + 1U) / 2U;
    float offsetMagnitude = spacing * (float)pairIndex;
    float direction = ((columnIndex & 1U) != 0U) ? 1.0f : -1.0f;
    return glm::vec3(direction * offsetMagnitude, 0.0f, 0.0f);
}

void Scene4EdgeQueue_Init(Scene4EdgeQueue* queue)
{
    if (queue)
    {
        queue->count = 0;
    }
}

BOOL Scene4EdgeQueue_Push(Scene4EdgeQueue* queue, float priority, int index)
{
    if (!queue || queue->count >= SCENE4_EDGE_QUEUE_CAPACITY)
    {
        return FALSE;
    }
    Scene4EdgeExpiration value;
    value.priority = priority;
    value.index = index;

    int insertPosition = queue->count;
    while ((insertPosition > 0) && (queue->entries[insertPosition - 1].priority < priority))
    {
        queue->entries[insertPosition] = queue->entries[insertPosition - 1];
        insertPosition -= 1;
    }
    queue->entries[insertPosition] = value;
    queue->count += 1;
    return TRUE;
}

Scene4EdgeExpiration Scene4EdgeQueue_Peek(const Scene4EdgeQueue* queue)
{
    Scene4EdgeExpiration result;
    result.priority = 0.0f;
    result.index = -1;
    if (!queue || queue->count <= 0)
    {
        return result;
    }
    return queue->entries[0];
}

void Scene4EdgeQueue_Pop(Scene4EdgeQueue* queue)
{
    if (!queue || queue->count <= 0)
    {
        return;
    }
    for (int i = 1; i < queue->count; ++i)
    {
        queue->entries[i - 1] = queue->entries[i];
    }
    queue->count -= 1;
}

typedef struct alignas(16) Scene4ShaderUniformLegacyTag
{
    glm::mat4 modelMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    glm::vec4 fireParams;
    glm::vec4 fireScale;
    glm::vec3 viewPos;
    float _padding; // ensures std140 alignment for the vec3 viewPos
} Scene4ShaderUniformLegacy;

BOOL Scene4_FindMemoryTypeIndex(uint32_t typeBits,
                                       VkMemoryPropertyFlags properties,
                                       uint32_t* outIndex)
{
    if (!outIndex)
    {
        return FALSE;
    }

    for (uint32_t i = 0; i < gCtx_Switcher.vkPhysicalDeviceMemoryProperties.memoryTypeCount; ++i)
    {
        if ((typeBits & (1u << i)) &&
            (gCtx_Switcher.vkPhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            *outIndex = i;
            return TRUE;
        }
    }

    return FALSE;
}

VkResult Scene4_RebuildSampler(VkSampler* sampler, VkSamplerAddressMode addressMode)
{
  if (!sampler)
  {
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  if (gCtx_Switcher.vkDevice == VK_NULL_HANDLE)
  {
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  if (*sampler != VK_NULL_HANDLE)
  {
    vkDestroySampler(gCtx_Switcher.vkDevice, *sampler, NULL);
    *sampler = VK_NULL_HANDLE;
  }

  VkSamplerCreateInfo samplerInfo;
  memset(&samplerInfo, 0, sizeof(samplerInfo));
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.addressModeU = addressMode;
  samplerInfo.addressModeV = addressMode;
  samplerInfo.addressModeW = addressMode;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.maxAnisotropy = 1.0f;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;

  VkResult result = vkCreateSampler(gCtx_Switcher.vkDevice, &samplerInfo, NULL, sampler);
  if (result != VK_SUCCESS && gCtx_Switcher.gpFile)
  {
    fprintf(gCtx_Switcher.gpFile,
            "Scene4_RebuildSampler() --> vkCreateSampler() failed %d\n",
            result);
  }
  return result;
}

static void Scene4_DestroyCpuGeometry(void)
{
    Scene4Vec3Array_Free(&gScene4State.positions);
    Scene4Vec3Array_Free(&gScene4State.localCoords);
    gScene4State.vertexCapacity = 0;
    gScene4State.recordedVertexCount = 0;
}

void Scene4_ResetState(void)
{
    Scene4_DestroyCpuGeometry();
    gScene4State.fireSliceCount = Scene4_ClampSliceCount(SCENE4_SLICE_COUNT_DEFAULT);
    gScene4State.fireRadius = SCENE4_DEFAULT_RADIUS;
    gScene4State.fireHeight = SCENE4_DEFAULT_HEIGHT;
    gScene4State.fireScaleX = SCENE4_DEFAULT_SCALE_X;
    gScene4State.fireScaleY = SCENE4_DEFAULT_SCALE_Y;
    gScene4State.fireScaleZ = SCENE4_DEFAULT_SCALE_Z;
    gScene4State.fireScrollSpeed = SCENE4_DEFAULT_SCROLL;
    gScene4State.fireTurbulence = SCENE4_DEFAULT_TURBULENCE;
    gScene4State.fireTimeSpeed = SCENE4_DEFAULT_TIME_SPEED;
    gScene4State.fireTime = 0.0f;
    gScene4State.fireOffsetX = SCENE4_DEFAULT_OFFSET_X;
    gScene4State.fireOffsetY = SCENE4_DEFAULT_OFFSET_Y;
    gScene4State.fireOffsetZ = SCENE4_DEFAULT_OFFSET_Z;
    gScene4State.cameraPosition = glm::vec3(0.0f, 0.5f, 5.0f);
    gScene4State.drawCmd.vertexCount = 0;
    gScene4State.drawCmd.instanceCount = 1;
    gScene4State.drawCmd.firstVertex = 0;
    gScene4State.drawCmd.firstInstance = 0;
    gScene4State.geometryDirty = TRUE;
}

BOOL Scene4_IsGeometryDirty(void)
{
    return gScene4State.geometryDirty;
}



BOOL Scene4_AdjustFireSliceCount(int direction, BOOL* outRequiresRebuild)
{
    if (outRequiresRebuild)
    {
        *outRequiresRebuild = FALSE;
    }

    if (!Scene4_AdjustUint32Step(&gScene4State.fireSliceCount, direction, SCENE4_SLICE_COUNT_STEP, SCENE4_SLICE_COUNT_MIN, SCENE4_SLICE_COUNT_MAX))
    {
        return FALSE;
    }

    Scene4_RequestGeometryRefresh();
    if (outRequiresRebuild && Scene4_SliceCountExceedsCapacity(gScene4State.fireSliceCount))
    {
        *outRequiresRebuild = TRUE;
    }

    Scene4_LogParameterUint("fireSliceCount", gScene4State.fireSliceCount);
    return TRUE;
}

BOOL Scene4_AdjustFireRadius(int direction)
{
    if (!Scene4_AdjustFloatStep(&gScene4State.fireRadius, direction, SCENE4_RADIUS_STEP, SCENE4_RADIUS_MIN, SCENE4_RADIUS_MAX))
    {
        return FALSE;
    }

    Scene4_RequestGeometryRefresh();
    Scene4_LogParameterFloat("fireRadius", gScene4State.fireRadius);
    return TRUE;
}

BOOL Scene4_AdjustFireHeight(int direction)
{
    if (!Scene4_AdjustFloatStep(&gScene4State.fireHeight,
                                direction,
                                SCENE4_HEIGHT_STEP,
                                SCENE4_HEIGHT_MIN,
                                SCENE4_HEIGHT_MAX))
    {
        return FALSE;
    }

    Scene4_RequestGeometryRefresh();
    Scene4_LogParameterFloat("fireHeight", gScene4State.fireHeight);
    return TRUE;
}

BOOL Scene4_AdjustFireScaleX(int direction)
{
    if (!Scene4_AdjustFloatStep(&gScene4State.fireScaleX,
                                direction,
                                SCENE4_SCALE_STEP,
                                SCENE4_SCALE_MIN,
                                SCENE4_SCALE_MAX))
    {
        return FALSE;
    }

    Scene4_RequestGeometryRefresh();
    Scene4_LogParameterFloat("fireScaleX", gScene4State.fireScaleX);
    return TRUE;
}

BOOL Scene4_AdjustFireScaleY(int direction)
{
    if (!Scene4_AdjustFloatStep(&gScene4State.fireScaleY,
                                direction,
                                SCENE4_SCALE_STEP,
                                SCENE4_SCALE_MIN,
                                SCENE4_SCALE_MAX))
    {
        return FALSE;
    }

    Scene4_RequestGeometryRefresh();
    Scene4_LogParameterFloat("fireScaleY", gScene4State.fireScaleY);
    return TRUE;
}

BOOL Scene4_AdjustFireScaleZ(int direction)
{
    if (!Scene4_AdjustFloatStep(&gScene4State.fireScaleZ,
                                direction,
                                SCENE4_SCALE_STEP,
                                SCENE4_SCALE_MIN,
                                SCENE4_SCALE_MAX))
    {
        return FALSE;
    }

    Scene4_RequestGeometryRefresh();
    Scene4_LogParameterFloat("fireScaleZ", gScene4State.fireScaleZ);
    return TRUE;
}

BOOL Scene4_AdjustFireScrollSpeed(int direction)
{
    if (!Scene4_AdjustFloatStep(&gScene4State.fireScrollSpeed,
                                direction,
                                SCENE4_SCROLL_STEP,
                                SCENE4_SCROLL_MIN,
                                SCENE4_SCROLL_MAX))
    {
        return FALSE;
    }

    Scene4_LogParameterFloat("fireScrollSpeed", gScene4State.fireScrollSpeed);
    return TRUE;
}

BOOL Scene4_AdjustFireTurbulence(int direction)
{
    if (!Scene4_AdjustFloatStep(&gScene4State.fireTurbulence,
                                direction,
                                SCENE4_TURBULENCE_STEP,
                                SCENE4_TURBULENCE_MIN,
                                SCENE4_TURBULENCE_MAX))
    {
        return FALSE;
    }

    Scene4_LogParameterFloat("fireTurbulence", gScene4State.fireTurbulence);
    return TRUE;
}

BOOL Scene4_AdjustFireTimeSpeed(int direction)
{
    if (!Scene4_AdjustFloatStep(&gScene4State.fireTimeSpeed,
                                direction,
                                SCENE4_TIME_SPEED_STEP,
                                SCENE4_TIME_SPEED_MIN,
                                SCENE4_TIME_SPEED_MAX))
    {
        return FALSE;
    }

    Scene4_LogParameterFloat("fireTimeSpeed", gScene4State.fireTimeSpeed);
    return TRUE;
}

BOOL Scene4_AdjustFireOffsetX(int direction)
{
    if (!Scene4_AdjustFloatStep(&gScene4State.fireOffsetX,
                                direction,
                                SCENE4_OFFSET_STEP_XZ,
                                SCENE4_OFFSET_X_MIN,
                                SCENE4_OFFSET_X_MAX))
    {
        return FALSE;
    }

    Scene4_RequestGeometryRefresh();
    Scene4_LogParameterFloat("fireOffsetX", gScene4State.fireOffsetX);
    return TRUE;
}

BOOL Scene4_AdjustFireOffsetY(int direction)
{
    if (!Scene4_AdjustFloatStep(&gScene4State.fireOffsetY,
                                direction,
                                SCENE4_OFFSET_STEP_Y,
                                SCENE4_OFFSET_Y_MIN,
                                SCENE4_OFFSET_Y_MAX))
    {
        return FALSE;
    }

    Scene4_RequestGeometryRefresh();
    Scene4_LogParameterFloat("fireOffsetY", gScene4State.fireOffsetY);
    return TRUE;
}

BOOL Scene4_AdjustFireOffsetZ(int direction)
{
    if (!Scene4_AdjustFloatStep(&gScene4State.fireOffsetZ,
                                direction,
                                SCENE4_OFFSET_STEP_XZ,
                                SCENE4_OFFSET_Z_MIN,
                                SCENE4_OFFSET_Z_MAX))
    {
        return FALSE;
    }

    Scene4_RequestGeometryRefresh();
    Scene4_LogParameterFloat("fireOffsetZ", gScene4State.fireOffsetZ);
    return TRUE;
}

BOOL Scene4_ResetFireParameters(BOOL* outRequiresRebuild)
{
    if (outRequiresRebuild)
    {
        *outRequiresRebuild = FALSE;
    }

    BOOL changed = FALSE;
    uint32_t clampedSliceCount = Scene4_ClampSliceCount(SCENE4_SLICE_COUNT_DEFAULT);
    if (gScene4State.fireSliceCount != clampedSliceCount)
    {
        gScene4State.fireSliceCount = clampedSliceCount;
        changed = TRUE;
    }

    if (gScene4State.fireRadius != SCENE4_DEFAULT_RADIUS)
    {
        gScene4State.fireRadius = SCENE4_DEFAULT_RADIUS;
        changed = TRUE;
    }
    if (gScene4State.fireHeight != SCENE4_DEFAULT_HEIGHT)
    {
        gScene4State.fireHeight = SCENE4_DEFAULT_HEIGHT;
        changed = TRUE;
    }
    if (gScene4State.fireScaleX != SCENE4_DEFAULT_SCALE_X)
    {
        gScene4State.fireScaleX = SCENE4_DEFAULT_SCALE_X;
        changed = TRUE;
    }
    if (gScene4State.fireScaleY != SCENE4_DEFAULT_SCALE_Y)
    {
        gScene4State.fireScaleY = SCENE4_DEFAULT_SCALE_Y;
        changed = TRUE;
    }
      if (gScene4State.fireScaleZ != SCENE4_DEFAULT_SCALE_Z)
      {
          gScene4State.fireScaleZ = SCENE4_DEFAULT_SCALE_Z;
          changed = TRUE;
      }
      if (gScene4State.fireScrollSpeed != SCENE4_DEFAULT_SCROLL)
      {
          gScene4State.fireScrollSpeed = SCENE4_DEFAULT_SCROLL;
          changed = TRUE;
      }
      if (gScene4State.fireTurbulence != SCENE4_DEFAULT_TURBULENCE)
      {
          gScene4State.fireTurbulence = SCENE4_DEFAULT_TURBULENCE;
          changed = TRUE;
      }
      if (gScene4State.fireTimeSpeed != SCENE4_DEFAULT_TIME_SPEED)
      {
          gScene4State.fireTimeSpeed = SCENE4_DEFAULT_TIME_SPEED;
          changed = TRUE;
      }
      if (gScene4State.fireOffsetX != SCENE4_DEFAULT_OFFSET_X)
      {
          gScene4State.fireOffsetX = SCENE4_DEFAULT_OFFSET_X;
          changed = TRUE;
      }
      if (gScene4State.fireOffsetY != SCENE4_DEFAULT_OFFSET_Y)
      {
          gScene4State.fireOffsetY = SCENE4_DEFAULT_OFFSET_Y;
          changed = TRUE;
      }
      if (gScene4State.fireOffsetZ != SCENE4_DEFAULT_OFFSET_Z)
      {
          gScene4State.fireOffsetZ = SCENE4_DEFAULT_OFFSET_Z;
          changed = TRUE;
      }

      gScene4State.fireTime = 0.0f;

    if (!changed)
    {
        return FALSE;
    }

    Scene4_RequestGeometryRefresh();
    if (outRequiresRebuild && Scene4_SliceCountExceedsCapacity(gScene4State.fireSliceCount))
    {
        *outRequiresRebuild = TRUE;
    }
    Scene4_LogFireParameters("Scene4_ResetFireParameters()");
    return TRUE;
}


 int Scene4_FireCreateEdge(int startIndex,
                                 int endIndex,
                                 float sliceDistance,
                                 float sliceSpacing,
                                 const glm::vec3* posCorners,
                                 const glm::vec3* texCorners,
                                 const float* cornerDistance,
                                 Scene4FireActiveEdge* activeEdges,
                                 int maxEdges,
                                 int* nextEdgeIndex,
                                 Scene4EdgeQueue* expirations)
{
    if ((endIndex < 0) || !activeEdges || !nextEdgeIndex || !posCorners || !texCorners || !cornerDistance)
    {
        return -1;
    }

    if (*nextEdgeIndex >= maxEdges)
    {
        return -1;
    }

    int createdIndex = *nextEdgeIndex;
    Scene4FireActiveEdge* activeEdge = &activeEdges[createdIndex];
    activeEdge->expired = FALSE;
    activeEdge->startIndex = startIndex;
    activeEdge->endIndex = endIndex;
    activeEdge->prev = -1;
    activeEdge->next = -1;
    activeEdge->deltaPos = glm::vec3(0.0f, 0.0f, 0.0f);
    activeEdge->deltaTex = glm::vec3(0.0f, 0.0f, 0.0f);
    activeEdge->pos = posCorners[startIndex];
    activeEdge->tex = texCorners[startIndex];

    float range = cornerDistance[startIndex] - cornerDistance[endIndex];
    if (fabsf(range) > 1.0e-6f)
    {
        float invRange = 1.0f / range;
        glm::vec3 deltaPos = (posCorners[endIndex] - posCorners[startIndex]) * invRange;
        glm::vec3 deltaTex = (texCorners[endIndex] - texCorners[startIndex]) * invRange;
        float step = cornerDistance[startIndex] - sliceDistance;
        activeEdge->pos = posCorners[startIndex] + (deltaPos * step);
        activeEdge->tex = texCorners[startIndex] + (deltaTex * step);
        activeEdge->deltaPos = deltaPos * sliceSpacing;
        activeEdge->deltaTex = deltaTex * sliceSpacing;
    }

    *nextEdgeIndex += 1;

    if (!Scene4EdgeQueue_Push(expirations, cornerDistance[endIndex], createdIndex))
    {
        activeEdge->expired = TRUE;
        return -1;
    }

    return createdIndex;
}

 VkResult Scene4_EmitFireSlices(const glm::vec3& columnWorldCenter,
                                      const glm::vec3& columnOffset,
                                      float widthHalf,
                                      float depthHalf,
                                      float heightHalf,
                                      uint32_t sliceCount)
{
    if (sliceCount == 0)
    {
        return VK_SUCCESS;
    }

    glm::vec3 posCorners[SCENE4_FIRE_CORNER_COUNT] = {
        glm::vec3(-widthHalf, -heightHalf, -depthHalf) + columnOffset,
        glm::vec3( widthHalf, -heightHalf, -depthHalf) + columnOffset,
        glm::vec3(-widthHalf,  heightHalf, -depthHalf) + columnOffset,
        glm::vec3( widthHalf,  heightHalf, -depthHalf) + columnOffset,
        glm::vec3(-widthHalf, -heightHalf,  depthHalf) + columnOffset,
        glm::vec3( widthHalf, -heightHalf,  depthHalf) + columnOffset,
        glm::vec3(-widthHalf,  heightHalf,  depthHalf) + columnOffset,
        glm::vec3( widthHalf,  heightHalf,  depthHalf) + columnOffset
    };

    glm::vec3 texCorners[SCENE4_FIRE_CORNER_COUNT] = {
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(1.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec3(1.0f, 0.0f, 1.0f),
        glm::vec3(0.0f, 1.0f, 1.0f),
        glm::vec3(1.0f, 1.0f, 1.0f)
    };

    glm::vec3 viewVector = columnWorldCenter - gScene4State.cameraPosition;
    if (glm::length(viewVector) < 0.0001f)
    {
        viewVector = glm::vec3(0.0f, 0.0f, -1.0f);
    }
    viewVector = glm::normalize(viewVector);

    float cornerDistance[SCENE4_FIRE_CORNER_COUNT];
    cornerDistance[0] = glm::dot(posCorners[0], viewVector);
    float minDistance = cornerDistance[0];
    float maxDistance = cornerDistance[0];
    int maxCorner = 0;

    for (int i = 1; i < SCENE4_FIRE_CORNER_COUNT; ++i)
    {
        cornerDistance[i] = glm::dot(posCorners[i], viewVector);
        if (cornerDistance[i] > maxDistance)
        {
            maxDistance = cornerDistance[i];
            maxCorner = i;
        }
        if (cornerDistance[i] < minDistance)
        {
            minDistance = cornerDistance[i];
        }
    }

    float distanceRange = maxDistance - minDistance;
    if (distanceRange <= 0.0f)
    {
        return VK_SUCCESS;
    }

    float sliceSpacing = distanceRange / (float)sliceCount;
    if (sliceSpacing <= 0.0f)
    {
        return VK_SUCCESS;
    }

    float sliceDistance = floorf(maxDistance / sliceSpacing) * sliceSpacing;

    Scene4FireActiveEdge activeEdges[SCENE4_FIRE_MAX_ACTIVE_EDGES];
    int nextEdgeIndex = 0;
    Scene4EdgeQueue expirations;
    Scene4EdgeQueue_Init(&expirations);

    static const int cornerNeighbors[SCENE4_FIRE_CORNER_COUNT][SCENE4_FIRE_CORNER_NEIGHBOR_COUNT] = {
        { 1, 2, 4 },
        { 0, 5, 3 },
        { 0, 3, 6 },
        { 1, 7, 2 },
        { 0, 6, 5 },
        { 1, 4, 7 },
        { 2, 7, 4 },
        { 3, 5, 6 }
    };

    static const int incomingEdges[SCENE4_FIRE_CORNER_COUNT][SCENE4_FIRE_CORNER_COUNT] = {
        { -1,  2,  4, -1,  1, -1, -1, -1 },
        {  5, -1, -1,  0, -1,  3, -1, -1 },
        {  3, -1, -1,  6, -1, -1,  0, -1 },
        { -1,  7,  1, -1, -1, -1, -1,  2 },
        {  6, -1, -1, -1, -1,  0,  5, -1 },
        { -1,  4, -1, -1,  7, -1, -1,  1 },
        { -1, -1,  7, -1,  2, -1, -1,  4 },
        { -1, -1, -1,  5, -1,  6,  3, -1 }
    };

    int initialEdges[SCENE4_FIRE_CORNER_NEIGHBOR_COUNT];
    for (int i = 0; i < SCENE4_FIRE_CORNER_NEIGHBOR_COUNT; ++i)
    {
        int neighborIndex = cornerNeighbors[maxCorner][i];
        initialEdges[i] = Scene4_FireCreateEdge(maxCorner,
                                                neighborIndex,
                                                sliceDistance,
                                                sliceSpacing,
                                                posCorners,
                                                texCorners,
                                                cornerDistance,
                                                activeEdges,
                                                SCENE4_FIRE_MAX_ACTIVE_EDGES,
                                                &nextEdgeIndex,
                                                &expirations);
    }

    for (int i = 0; i < SCENE4_FIRE_CORNER_NEIGHBOR_COUNT; ++i)
    {
        int edgeIndex = initialEdges[i];
        int prevIndex = initialEdges[(i + 2) % SCENE4_FIRE_CORNER_NEIGHBOR_COUNT];
        int nextIndex = initialEdges[(i + 1) % SCENE4_FIRE_CORNER_NEIGHBOR_COUNT];

        if ((edgeIndex >= 0) && (prevIndex >= 0) && (nextIndex >= 0))
        {
            activeEdges[edgeIndex].prev = prevIndex;
            activeEdges[edgeIndex].next = nextIndex;
        }
    }

    int firstEdge = (initialEdges[0] >= 0) ? initialEdges[0] : 0;

    while (sliceDistance > minDistance)
    {
        while (expirations.count > 0)
        {
            Scene4EdgeExpiration top = Scene4EdgeQueue_Peek(&expirations);
            if (top.priority < sliceDistance)
            {
                break;
            }

            Scene4EdgeQueue_Pop(&expirations);

            if ((top.index < 0) || (top.index >= nextEdgeIndex))
            {
                continue;
            }

            Scene4FireActiveEdge* edge = &activeEdges[top.index];
            if (edge->expired)
            {
                continue;
            }

            int prevIndex = edge->prev;
            int nextIndex = edge->next;
            if ((prevIndex < 0) || (nextIndex < 0))
            {
                edge->expired = TRUE;
                continue;
            }

            Scene4FireActiveEdge* prevEdge = &activeEdges[prevIndex];
            Scene4FireActiveEdge* nextEdgeRef = &activeEdges[nextIndex];

            if ((edge->endIndex != prevEdge->endIndex) && (edge->endIndex != nextEdgeRef->endIndex))
            {
                edge->expired = TRUE;

                int edge1 = Scene4_FireCreateEdge(edge->endIndex,
                                                  incomingEdges[edge->endIndex][edge->startIndex],
                                                  sliceDistance,
                                                  sliceSpacing,
                                                  posCorners,
                                                  texCorners,
                                                  cornerDistance,
                                                  activeEdges,
                                                  SCENE4_FIRE_MAX_ACTIVE_EDGES,
                                                  &nextEdgeIndex,
                                                  &expirations);
                if (edge1 < 0)
                {
                    continue;
                }
                activeEdges[edge1].prev = prevIndex;
                activeEdges[prevIndex].next = edge1;

                int nextTarget = incomingEdges[edge->endIndex][activeEdges[edge1].endIndex];
                int edge2 = Scene4_FireCreateEdge(edge->endIndex,
                                                  nextTarget,
                                                  sliceDistance,
                                                  sliceSpacing,
                                                  posCorners,
                                                  texCorners,
                                                  cornerDistance,
                                                  activeEdges,
                                                  SCENE4_FIRE_MAX_ACTIVE_EDGES,
                                                  &nextEdgeIndex,
                                                  &expirations);
                if (edge2 < 0)
                {
                    continue;
                }
                activeEdges[edge1].next = edge2;
                activeEdges[edge2].prev = edge1;
                activeEdges[edge2].next = nextIndex;
                activeEdges[nextIndex].prev = edge2;
                firstEdge = edge1;
            }
            else
            {
                Scene4FireActiveEdge* prevPtr = NULL;
                Scene4FireActiveEdge* nextPtr = NULL;

                if (edge->endIndex == prevEdge->endIndex)
                {
                    prevPtr = prevEdge;
                    nextPtr = edge;
                }
                else
                {
                    prevPtr = edge;
                    nextPtr = nextEdgeRef;
                }

                if (!prevPtr || !nextPtr)
                {
                    continue;
                }

                prevPtr->expired = TRUE;
                nextPtr->expired = TRUE;

                int merged = Scene4_FireCreateEdge(edge->endIndex,
                                                   incomingEdges[edge->endIndex][prevPtr->startIndex],
                                                   sliceDistance,
                                                   sliceSpacing,
                                                   posCorners,
                                                   texCorners,
                                                   cornerDistance,
                                                   activeEdges,
                                                   SCENE4_FIRE_MAX_ACTIVE_EDGES,
                                                   &nextEdgeIndex,
                                                   &expirations);
                if (merged < 0)
                {
                    continue;
                }

                int prevPrev = prevPtr->prev;
                int nextNext = nextPtr->next;
                if ((prevPrev < 0) || (nextNext < 0))
                {
                    activeEdges[merged].expired = TRUE;
                    continue;
                }

                activeEdges[merged].prev = prevPrev;
                activeEdges[prevPrev].next = merged;
                activeEdges[merged].next = nextNext;
                activeEdges[nextNext].prev = merged;
                firstEdge = merged;
            }
        }

        if ((firstEdge < 0) || (firstEdge >= nextEdgeIndex))
        {
            break;
        }

        glm::vec3 polygonPositions[SCENE4_FIRE_MAX_ACTIVE_EDGES];
        glm::vec3 polygonTexCoords[SCENE4_FIRE_MAX_ACTIVE_EDGES];
        int count = 0;
        int current = firstEdge;

        do
        {
            Scene4FireActiveEdge* activeEdge = &activeEdges[current];
            if (!activeEdge->expired && (count < SCENE4_FIRE_MAX_ACTIVE_EDGES))
            {
                polygonPositions[count] = activeEdge->pos;
                polygonTexCoords[count] = activeEdge->tex;
                count += 1;
            }

            activeEdge->pos = activeEdge->pos + activeEdge->deltaPos;
            activeEdge->tex = activeEdge->tex + activeEdge->deltaTex;
            current = activeEdge->next;

            if ((current < 0) || (current >= nextEdgeIndex))
            {
                break;
            }
        } while ((current != firstEdge) && (count < SCENE4_FIRE_MAX_ACTIVE_EDGES));

        if (count >= 3)
        {
            for (int i = 2; i < count; ++i)
            {
                if (!Scene4Vec3Array_Push(&gScene4State.positions, &polygonPositions[0]) ||
                    !Scene4Vec3Array_Push(&gScene4State.positions, &polygonPositions[i - 1]) ||
                    !Scene4Vec3Array_Push(&gScene4State.positions, &polygonPositions[i]) ||
                    !Scene4Vec3Array_Push(&gScene4State.localCoords, &polygonTexCoords[0]) ||
                    !Scene4Vec3Array_Push(&gScene4State.localCoords, &polygonTexCoords[i - 1]) ||
                    !Scene4Vec3Array_Push(&gScene4State.localCoords, &polygonTexCoords[i]))
                {
                    return VK_ERROR_OUT_OF_HOST_MEMORY;
                }
            }
        }

        sliceDistance -= sliceSpacing;
    }

    return VK_SUCCESS;
}

 VkResult Scene4_GenerateFireSlices(void)
{
    Scene4Vec3Array_Clear(&gScene4State.positions);
    Scene4Vec3Array_Clear(&gScene4State.localCoords);
    gScene4State.recordedVertexCount = 0;

    uint32_t sliceCount = Scene4_ClampSliceCount(gScene4State.fireSliceCount);
    if (sliceCount != gScene4State.fireSliceCount)
    {
        gScene4State.fireSliceCount = sliceCount;
    }

    if (sliceCount == 0)
    {
        return VK_SUCCESS;
    }

    size_t requiredVertexCapacity = (size_t)sliceCount * 12U * (size_t)SCENE4_FIRE_COLUMN_COUNT;
    if (!Scene4Vec3Array_Reserve(&gScene4State.positions, requiredVertexCapacity))
    {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    if (!Scene4Vec3Array_Reserve(&gScene4State.localCoords, requiredVertexCapacity))
    {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    const float widthHalf = gScene4State.fireRadius;
    const float depthHalf = gScene4State.fireRadius;
    const float heightHalf = gScene4State.fireHeight * 0.5f;

    const glm::vec3 baseFireCenter = glm::vec3(0.0f, gScene4State.fireHeight * 0.5f, 0.0f);
    const glm::vec3 translation = glm::vec3(gScene4State.fireOffsetX,
                                            gScene4State.fireOffsetY,
                                            gScene4State.fireOffsetZ);
    const glm::vec3 fireCenterWorld = baseFireCenter + translation;

    for (uint32_t columnIndex = 0; columnIndex < SCENE4_FIRE_COLUMN_COUNT; ++columnIndex)
    {
        glm::vec3 columnOffset = Scene4_GetFireColumnOffset(columnIndex);
        glm::vec3 columnWorldCenter = fireCenterWorld + columnOffset;
        VkResult columnResult = Scene4_EmitFireSlices(columnWorldCenter,
                                                      columnOffset,
                                                      widthHalf,
                                                      depthHalf,
                                                      heightHalf,
                                                      sliceCount);
        if (columnResult != VK_SUCCESS)
        {
            return columnResult;
        }
    }

    gScene4State.recordedVertexCount = (uint32_t)gScene4State.positions.size;
    return VK_SUCCESS;
}

VkResult Scene4_UpdateGeometryBuffers(void)
{
    if (gCtx_Switcher.vertexData_scene4_position.vkBuffer == VK_NULL_HANDLE ||
        gCtx_Switcher.vertexData_scene4_local.vkBuffer == VK_NULL_HANDLE ||
        gCtx_Switcher.vkBuffer_scene4_drawIndirect == VK_NULL_HANDLE)
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    VkResult vkResult = Scene4_GenerateFireSlices();
    if (vkResult != VK_SUCCESS)
    {
        return vkResult;
    }

    VkDeviceSize positionBufferSize = sizeof(glm::vec3) * gScene4State.recordedVertexCount;
    void* data = NULL;
    if (positionBufferSize > 0)
    {
        vkResult = vkMapMemory(gCtx_Switcher.vkDevice,
                               gCtx_Switcher.vertexData_scene4_position.vkDeviceMemory,
                               0,
                               positionBufferSize,
                               0,
                               &data);
        if (vkResult != VK_SUCCESS)
        {
            return vkResult;
        }
        memcpy(data, gScene4State.positions.data, (size_t)positionBufferSize);
        vkUnmapMemory(gCtx_Switcher.vkDevice, gCtx_Switcher.vertexData_scene4_position.vkDeviceMemory);
    }

    VkDeviceSize localBufferSize = sizeof(glm::vec3) * gScene4State.recordedVertexCount;
    if (localBufferSize > 0)
    {
        vkResult = vkMapMemory(gCtx_Switcher.vkDevice,
                               gCtx_Switcher.vertexData_scene4_local.vkDeviceMemory,
                               0,
                               localBufferSize,
                               0,
                               &data);
        if (vkResult != VK_SUCCESS)
        {
            return vkResult;
        }
        memcpy(data, gScene4State.localCoords.data, (size_t)localBufferSize);
        vkUnmapMemory(gCtx_Switcher.vkDevice, gCtx_Switcher.vertexData_scene4_local.vkDeviceMemory);
    }

    gScene4State.drawCmd.vertexCount = gScene4State.recordedVertexCount;
    gScene4State.drawCmd.instanceCount = 1;
    gScene4State.drawCmd.firstVertex = 0;
    gScene4State.drawCmd.firstInstance = 0;

    vkResult = vkMapMemory(gCtx_Switcher.vkDevice,
                           gCtx_Switcher.vkDeviceMemory_scene4_drawIndirect,
                           0,
                           sizeof(VkDrawIndirectCommand),
                           0,
                           &data);
    if (vkResult != VK_SUCCESS)
    {
        return vkResult;
    }
    memcpy(data, &gScene4State.drawCmd, sizeof(VkDrawIndirectCommand));
    vkUnmapMemory(gCtx_Switcher.vkDevice, gCtx_Switcher.vkDeviceMemory_scene4_drawIndirect);

    gScene4State.geometryDirty = FALSE;
    return VK_SUCCESS;
}

void Scene4_Tick(void)
{
    gScene4State.fireTime += (SCENE4_BASE_DELTA_TIME * gScene4State.fireTimeSpeed);
    if (gScene4State.fireTime > SCENE4_TIME_WRAP)
    {
        gScene4State.fireTime -= SCENE4_TIME_WRAP;
    }
    gScene4State.geometryDirty = TRUE;
}

VkResult Scene4_UpdateUniformBuffer(void)
{
    if (gCtx_Switcher.uniformData_scene4.vkDeviceMemory == VK_NULL_HANDLE)
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    Scene4ShaderUniformLegacy data;
    memset(&data, 0, sizeof(Scene4ShaderUniformLegacy));

    const glm::vec3 baseFireCenter = glm::vec3(0.0f, gScene4State.fireHeight * 0.5f, 0.0f);
    glm::vec3 translation = glm::vec3(gScene4State.fireOffsetX,
                                      gScene4State.fireOffsetY,
                                      gScene4State.fireOffsetZ);
    glm::vec3 fireCenter = baseFireCenter + translation;

    data.modelMatrix = glm::translate(glm::mat4(1.0f), fireCenter);
    data.viewMatrix = glm::lookAt(gScene4State.cameraPosition,
                                  baseFireCenter,
                                  glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 perspectiveProjectionMatrix = glm::perspective(glm::radians(45.0f),
                                                             (float)gCtx_Switcher.winWidth / (float)gCtx_Switcher.winHeight,
                                                             0.1f,
                                                             100.0f);
    perspectiveProjectionMatrix[1][1] *= -1.0f;
    data.projectionMatrix = perspectiveProjectionMatrix;

    data.fireParams = glm::vec4(gScene4State.fireTime,
                                gScene4State.fireScaleX,
                                gScene4State.fireScaleY,
                                gScene4State.fireScaleZ);

    data.fireScale = glm::vec4(gScene4State.fireScrollSpeed,
                               gScene4State.fireTurbulence,
                               0.0f,
                               0.0f);

    data.viewPos = gScene4State.cameraPosition;

    void* mapped = NULL;
    VkResult vkResult = vkMapMemory(gCtx_Switcher.vkDevice,
                                    gCtx_Switcher.uniformData_scene4.vkDeviceMemory,
                                    0,
                                    sizeof(Scene4ShaderUniformLegacy),
                                    0,
                                    &mapped);
    if (vkResult != VK_SUCCESS)
    {
        if (gCtx_Switcher.gpFile != NULL)
        {
            fprintf(gCtx_Switcher.gpFile,
                    "Scene4_UpdateUniformBuffer() --> vkMapMemory() failed %d\n",
                    vkResult);
        }
        return vkResult;
    }

    if (mapped == NULL)
    {
        if (gCtx_Switcher.gpFile != NULL)
        {
            fprintf(gCtx_Switcher.gpFile,
                    "Scene4_UpdateUniformBuffer() --> vkMapMemory() returned NULL pointer\n");
        }
        vkUnmapMemory(gCtx_Switcher.vkDevice, gCtx_Switcher.uniformData_scene4.vkDeviceMemory);
        return VK_ERROR_MEMORY_MAP_FAILED;
    }

    memcpy(mapped, &data, sizeof(Scene4ShaderUniformLegacy));
    vkUnmapMemory(gCtx_Switcher.vkDevice, gCtx_Switcher.uniformData_scene4.vkDeviceMemory);
    return VK_SUCCESS;
}
void Scene4_DestroyTextures(void)
{
    if (gCtx_Switcher.vkSampler_texture_scene4_fire)
    {
        vkDestroySampler(gCtx_Switcher.vkDevice, gCtx_Switcher.vkSampler_texture_scene4_fire, NULL);
        gCtx_Switcher.vkSampler_texture_scene4_fire = VK_NULL_HANDLE;
    }
    if (gCtx_Switcher.vkImageView_texture_scene4_fire)
    {
        vkDestroyImageView(gCtx_Switcher.vkDevice, gCtx_Switcher.vkImageView_texture_scene4_fire, NULL);
        gCtx_Switcher.vkImageView_texture_scene4_fire = VK_NULL_HANDLE;
    }
    if (gCtx_Switcher.vkDeviceMemory_texture_scene4_fire)
    {
        vkFreeMemory(gCtx_Switcher.vkDevice, gCtx_Switcher.vkDeviceMemory_texture_scene4_fire, NULL);
        gCtx_Switcher.vkDeviceMemory_texture_scene4_fire = VK_NULL_HANDLE;
    }
    if (gCtx_Switcher.vkImage_texture_scene4_fire)
    {
        vkDestroyImage(gCtx_Switcher.vkDevice, gCtx_Switcher.vkImage_texture_scene4_fire, NULL);
        gCtx_Switcher.vkImage_texture_scene4_fire = VK_NULL_HANDLE;
    }

    if (gCtx_Switcher.vkSampler_texture_scene4_noise)
    {
        vkDestroySampler(gCtx_Switcher.vkDevice, gCtx_Switcher.vkSampler_texture_scene4_noise, NULL);
        gCtx_Switcher.vkSampler_texture_scene4_noise = VK_NULL_HANDLE;
    }
    if (gCtx_Switcher.vkImageView_texture_scene4_noise)
    {
        vkDestroyImageView(gCtx_Switcher.vkDevice, gCtx_Switcher.vkImageView_texture_scene4_noise, NULL);
        gCtx_Switcher.vkImageView_texture_scene4_noise = VK_NULL_HANDLE;
    }
    if (gCtx_Switcher.vkDeviceMemory_texture_scene4_noise)
    {
        vkFreeMemory(gCtx_Switcher.vkDevice, gCtx_Switcher.vkDeviceMemory_texture_scene4_noise, NULL);
        gCtx_Switcher.vkDeviceMemory_texture_scene4_noise = VK_NULL_HANDLE;
    }
    if (gCtx_Switcher.vkImage_texture_scene4_noise)
    {
        vkDestroyImage(gCtx_Switcher.vkDevice, gCtx_Switcher.vkImage_texture_scene4_noise, NULL);
        gCtx_Switcher.vkImage_texture_scene4_noise = VK_NULL_HANDLE;
    }
}

VkResult Scene4_CreateTextures(void)
{
    VkResult vkResult = CreateTexture2D("firetex.png",
                                        &gCtx_Switcher.vkImage_texture_scene4_fire,
                                        &gCtx_Switcher.vkDeviceMemory_texture_scene4_fire,
                                        &gCtx_Switcher.vkImageView_texture_scene4_fire,
                                        &gCtx_Switcher.vkSampler_texture_scene4_fire);
    if (vkResult != VK_SUCCESS)
    {
        Scene4_DestroyTextures();
        return vkResult;
    }

  vkResult = Scene4_RebuildSampler(&gCtx_Switcher.vkSampler_texture_scene4_fire,
                                   VK_SAMPLER_ADDRESS_MODE_REPEAT);
  if (vkResult != VK_SUCCESS)
  {
    Scene4_DestroyTextures();
    return vkResult;
  }

    vkResult = CreateTexture2D("nzw.png",
                               &gCtx_Switcher.vkImage_texture_scene4_noise,
                               &gCtx_Switcher.vkDeviceMemory_texture_scene4_noise,
                               &gCtx_Switcher.vkImageView_texture_scene4_noise,
                               &gCtx_Switcher.vkSampler_texture_scene4_noise);
    if (vkResult != VK_SUCCESS)
    {
        Scene4_DestroyTextures();
        return vkResult;
    }

  vkResult = Scene4_RebuildSampler(&gCtx_Switcher.vkSampler_texture_scene4_noise,
                                   VK_SAMPLER_ADDRESS_MODE_REPEAT);
  if (vkResult != VK_SUCCESS)
  {
    Scene4_DestroyTextures();
    return vkResult;
  }

    return VK_SUCCESS;
}

void Scene4_DestroyVertexResources(void)
{
    if (gCtx_Switcher.vertexData_scene4_local.vkDeviceMemory)
    {
        vkFreeMemory(gCtx_Switcher.vkDevice, gCtx_Switcher.vertexData_scene4_local.vkDeviceMemory, NULL);
        gCtx_Switcher.vertexData_scene4_local.vkDeviceMemory = VK_NULL_HANDLE;
    }
    if (gCtx_Switcher.vertexData_scene4_local.vkBuffer)
    {
        vkDestroyBuffer(gCtx_Switcher.vkDevice, gCtx_Switcher.vertexData_scene4_local.vkBuffer, NULL);
        gCtx_Switcher.vertexData_scene4_local.vkBuffer = VK_NULL_HANDLE;
    }

    if (gCtx_Switcher.vertexData_scene4_position.vkDeviceMemory)
    {
        vkFreeMemory(gCtx_Switcher.vkDevice, gCtx_Switcher.vertexData_scene4_position.vkDeviceMemory, NULL);
        gCtx_Switcher.vertexData_scene4_position.vkDeviceMemory = VK_NULL_HANDLE;
    }
    if (gCtx_Switcher.vertexData_scene4_position.vkBuffer)
    {
        vkDestroyBuffer(gCtx_Switcher.vkDevice, gCtx_Switcher.vertexData_scene4_position.vkBuffer, NULL);
        gCtx_Switcher.vertexData_scene4_position.vkBuffer = VK_NULL_HANDLE;
    }

    if (gCtx_Switcher.vkDeviceMemory_scene4_drawIndirect)
    {
        vkFreeMemory(gCtx_Switcher.vkDevice, gCtx_Switcher.vkDeviceMemory_scene4_drawIndirect, NULL);
        gCtx_Switcher.vkDeviceMemory_scene4_drawIndirect = VK_NULL_HANDLE;
    }
    if (gCtx_Switcher.vkBuffer_scene4_drawIndirect)
    {
        vkDestroyBuffer(gCtx_Switcher.vkDevice, gCtx_Switcher.vkBuffer_scene4_drawIndirect, NULL);
        gCtx_Switcher.vkBuffer_scene4_drawIndirect = VK_NULL_HANDLE;
    }

    Scene4_DestroyCpuGeometry();
    gScene4State.drawCmd.vertexCount = 0;
    gScene4State.geometryDirty = TRUE;
}

VkResult Scene4_CreateVertexResources(void)
{
    Scene4_DestroyVertexResources();

    uint32_t sliceCount = Scene4_ClampSliceCount(gScene4State.fireSliceCount);
    if (sliceCount != gScene4State.fireSliceCount)
    {
        gScene4State.fireSliceCount = sliceCount;
    }

    uint32_t vertexCount = sliceCount * 12U * SCENE4_FIRE_COLUMN_COUNT;
    if (vertexCount == 0)
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    VkBufferCreateInfo bufferInfo;
    memset(&bufferInfo, 0, sizeof(bufferInfo));
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(glm::vec3) * vertexCount;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    VkResult vkResult = vkCreateBuffer(gCtx_Switcher.vkDevice,
                                       &bufferInfo,
                                       NULL,
                                       &gCtx_Switcher.vertexData_scene4_position.vkBuffer);
    if (vkResult != VK_SUCCESS)
    {
        Scene4_DestroyVertexResources();
        return vkResult;
    }

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(gCtx_Switcher.vkDevice,
                                  gCtx_Switcher.vertexData_scene4_position.vkBuffer,
                                  &memReq);

    VkMemoryAllocateInfo allocInfo;
    memset(&allocInfo, 0, sizeof(allocInfo));
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;

    uint32_t memoryTypeIndex = 0;
    if (!Scene4_FindMemoryTypeIndex(memReq.memoryTypeBits,
                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                    &memoryTypeIndex))
    {
        Scene4_DestroyVertexResources();
        return VK_ERROR_FEATURE_NOT_PRESENT;
    }
    allocInfo.memoryTypeIndex = memoryTypeIndex;
    vkResult = vkAllocateMemory(gCtx_Switcher.vkDevice,
                                &allocInfo,
                                NULL,
                                &gCtx_Switcher.vertexData_scene4_position.vkDeviceMemory);
    if (vkResult != VK_SUCCESS)
    {
        Scene4_DestroyVertexResources();
        return vkResult;
    }

    vkResult = vkBindBufferMemory(gCtx_Switcher.vkDevice,
                                  gCtx_Switcher.vertexData_scene4_position.vkBuffer,
                                  gCtx_Switcher.vertexData_scene4_position.vkDeviceMemory,
                                  0);
    if (vkResult != VK_SUCCESS)
    {
        Scene4_DestroyVertexResources();
        return vkResult;
    }

    vkResult = vkCreateBuffer(gCtx_Switcher.vkDevice,
                              &bufferInfo,
                              NULL,
                              &gCtx_Switcher.vertexData_scene4_local.vkBuffer);
    if (vkResult != VK_SUCCESS)
    {
        Scene4_DestroyVertexResources();
        return vkResult;
    }

    vkGetBufferMemoryRequirements(gCtx_Switcher.vkDevice,
                                  gCtx_Switcher.vertexData_scene4_local.vkBuffer,
                                  &memReq);
    allocInfo.allocationSize = memReq.size;

    if (!Scene4_FindMemoryTypeIndex(memReq.memoryTypeBits,
                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                    &memoryTypeIndex))
    {
        Scene4_DestroyVertexResources();
        return VK_ERROR_FEATURE_NOT_PRESENT;
    }
    allocInfo.memoryTypeIndex = memoryTypeIndex;
    vkResult = vkAllocateMemory(gCtx_Switcher.vkDevice,
                                &allocInfo,
                                NULL,
                                &gCtx_Switcher.vertexData_scene4_local.vkDeviceMemory);
    if (vkResult != VK_SUCCESS)
    {
        Scene4_DestroyVertexResources();
        return vkResult;
    }

    vkResult = vkBindBufferMemory(gCtx_Switcher.vkDevice,
                                  gCtx_Switcher.vertexData_scene4_local.vkBuffer,
                                  gCtx_Switcher.vertexData_scene4_local.vkDeviceMemory,
                                  0);
    if (vkResult != VK_SUCCESS)
    {
        Scene4_DestroyVertexResources();
        return vkResult;
    }

    // Indirect draw buffer
    memset(&bufferInfo, 0, sizeof(bufferInfo));
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(VkDrawIndirectCommand);
    bufferInfo.usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

    vkResult = vkCreateBuffer(gCtx_Switcher.vkDevice,
                              &bufferInfo,
                              NULL,
                              &gCtx_Switcher.vkBuffer_scene4_drawIndirect);
    if (vkResult != VK_SUCCESS)
    {
        Scene4_DestroyVertexResources();
        return vkResult;
    }

    vkGetBufferMemoryRequirements(gCtx_Switcher.vkDevice,
                                  gCtx_Switcher.vkBuffer_scene4_drawIndirect,
                                  &memReq);
    allocInfo.allocationSize = memReq.size;

    if (!Scene4_FindMemoryTypeIndex(memReq.memoryTypeBits,
                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                    &memoryTypeIndex))
    {
        Scene4_DestroyVertexResources();
        return VK_ERROR_FEATURE_NOT_PRESENT;
    }
    allocInfo.memoryTypeIndex = memoryTypeIndex;
    vkResult = vkAllocateMemory(gCtx_Switcher.vkDevice,
                                &allocInfo,
                                NULL,
                                &gCtx_Switcher.vkDeviceMemory_scene4_drawIndirect);
    if (vkResult != VK_SUCCESS)
    {
        Scene4_DestroyVertexResources();
        return vkResult;
    }

    vkResult = vkBindBufferMemory(gCtx_Switcher.vkDevice,
                                  gCtx_Switcher.vkBuffer_scene4_drawIndirect,
                                  gCtx_Switcher.vkDeviceMemory_scene4_drawIndirect,
                                  0);
    if (vkResult != VK_SUCCESS)
    {
        Scene4_DestroyVertexResources();
        return vkResult;
    }

    gScene4State.vertexCapacity = vertexCount;
    gScene4State.geometryDirty = TRUE;
    return Scene4_UpdateGeometryBuffers();
}

