#ifndef UTILS_H
#define UTILS_H

#include "Types.h"

#include <format.h>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <fstream>
#include <vector>


#define VK_CHECK(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
            throw std::runtime_error(string_VkResult(err));				\
        }                                                               \
    } while (0)

namespace utils {
    VkShaderModule create_shader_module(VkDevice device, const char* path);

    void image_memory_barrier(VkCommandBuffer cmdBuffer, VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask,
        VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout, VkImage image);

    void blit_image(VkCommandBuffer cmdBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout,
        VkExtent2D srcExtent, VkExtent2D dstExtent);

    vkt::IndexedMesh generate_cube_mesh();

    vkt::Buffer create_buffer(VmaAllocator allocator, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage,
                                VmaMemoryUsage memoryUsage, VkMemoryPropertyFlags requiredFlags, VmaAllocationCreateFlags flags = 0);

    glm::mat4 orthographicProj(float left, float right, float bottom, float top, float near, float far);
    glm::mat4 orthographicProj(float vFov, float aspectRatio, float near, float far);

}
#endif // !UTILS_H
