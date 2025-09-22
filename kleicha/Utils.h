#ifndef UTILS_H
#define UTILS_H

#include "Types.h"

#pragma warning(push, 0)
#pragma warning(disable : 6285 26498)
#include "format.h"
#pragma warning(pop)
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
        VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout, VkImage image, uint32_t mipLevels);

    void blit_image(VkCommandBuffer cmdBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout,
        VkExtent2D srcExtent, VkExtent2D dstExtent, uint32_t srcMipLevel, uint32_t dstMipLevel);

    vkt::Mesh generate_cube_mesh();
    vkt::Mesh generate_pyramid_mesh();
    vkt::Mesh generate_sphere(size_t prec);
    vkt::Mesh generate_torus(size_t prec, float inner, float outer);
    vkt::Mesh load_obj_mesh(const char* filePath);

    vkt::Buffer create_buffer(VmaAllocator allocator, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage,
                                VmaMemoryUsage memoryUsage, VkMemoryPropertyFlags requiredFlags, VmaAllocationCreateFlags flags = 0);

    void update_set_buffer_descriptor(VkDevice device, VkDescriptorSet set, uint32_t binding, VkDescriptorType descriptorType,
        VkBuffer buffer, VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE);

    void update_set_image_sampler_descriptor(VkDevice device, VkDescriptorSet set, uint32_t binding, VkImageLayout imageSampledLayout, VkSampler sampler, const std::vector<vkt::Image>& images);
    void update_set_image_sampler_descriptor(VkDevice device, VkDescriptorSet set, uint32_t binding, VkImageLayout imageSampledLayout, VkSampler sampler, const std::vector<vkt::CubeImage>& images);


    glm::mat4 lookAt(glm::vec3 eye, glm::vec3 lookat, glm::vec3 up);
    glm::mat4 perspective(float near, float far);
    glm::mat4 orthographicProj(float left, float right, float bottom, float top, float near, float far);
    glm::mat4 orthographicProj(float vFov, float aspectRatio, float near, float far);

    void set_viewport_scissor(const vkt::Frame& frame, VkExtent2D extent);

    void compute_mesh_tangents(vkt::Mesh& mesh);

    bool load_gltf(const char* filePath, std::vector<vkt::Mesh>& meshes, std::vector<vkt::DrawData>& draws, std::vector<vkt::Transform>& transforms, std::vector<vkt::Material>& materials, std::vector<std::string>& texturePaths);

}
#endif // !UTILS_H
