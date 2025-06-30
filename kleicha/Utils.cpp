#include "Utils.h"
#include "Initializers.h"

namespace utils {
    VkShaderModule create_shader_module(VkDevice device, const char* path) {
        // opens an input file stream. Specify stream open mode
        std::ifstream ifstrm{ path, std::ios::binary | std::ios::ate };
        if (!ifstrm.is_open())
            throw std::runtime_error{ "[Utils] Failed to open file stream with file: " + std::string{path} };

        // file size in bytes
        std::size_t fileSize{ static_cast<std::size_t>(ifstrm.tellg()) };

        // driver expects the provided intermediate spir-v data to be a contiguous array of uint32_t (4 bytes)
        std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

        // seek to top of file and read stream into our vector
        ifstrm.seekg(0);
        // std::vector::data returns pointer to beginning, read data into vector a byte at a time.
        ifstrm.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(fileSize));
        ifstrm.close();

        // create shader module
        VkShaderModuleCreateInfo shaderModuleInfo{ .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        shaderModuleInfo.codeSize = buffer.size() * sizeof(uint32_t);
        shaderModuleInfo.pCode = buffer.data();

        VkShaderModule shaderModule{};
        VK_CHECK(vkCreateShaderModule(device, &shaderModuleInfo, nullptr, &shaderModule));

        return shaderModule;
    }

    void image_memory_barrier(VkCommandBuffer cmdBuffer, VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask,
        VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout, VkImage image) {

        VkImageMemoryBarrier2 barrier{ init::create_image_barrier_info(srcStageMask, srcAccessMask, dstStageMask, dstAccessMask, oldLayout, newLayout, image) };

        VkDependencyInfo dependencyInfo{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
        dependencyInfo.pNext = nullptr;
        dependencyInfo.imageMemoryBarrierCount = 1;
        dependencyInfo.pImageMemoryBarriers = &barrier;

        vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);
    }

    void blit_image(VkCommandBuffer cmdBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, VkExtent2D srcExtent, VkExtent2D dstExtent) {
        VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2 };
        blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.srcSubresource.baseArrayLayer = 0;
        blitRegion.srcSubresource.layerCount = 1;
        blitRegion.srcSubresource.mipLevel = 0;

        blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.dstSubresource.baseArrayLayer = 0;
        blitRegion.dstSubresource.layerCount = 1;
        blitRegion.dstSubresource.mipLevel = 0;

        blitRegion.srcOffsets[1].x = static_cast<int32_t>(srcExtent.width);
        blitRegion.srcOffsets[1].y = static_cast<int32_t>(srcExtent.height);
        blitRegion.srcOffsets[1].z = 1;

        blitRegion.dstOffsets[1].x = static_cast<int32_t>(dstExtent.width);
        blitRegion.dstOffsets[1].y = static_cast<int32_t>(dstExtent.height);
        blitRegion.dstOffsets[1].z = 1;

        VkBlitImageInfo2 blitImageInfo{ .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2 };
        blitImageInfo.srcImage = srcImage;
        blitImageInfo.srcImageLayout = srcImageLayout;
        blitImageInfo.dstImage = dstImage;
        blitImageInfo.dstImageLayout = dstImageLayout;
        blitImageInfo.regionCount = 1;
        blitImageInfo.pRegions = &blitRegion;

        vkCmdBlitImage2(cmdBuffer, &blitImageInfo);
    }

    vkt::Buffer create_buffer(VmaAllocator allocator, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage,
        VmaMemoryUsage memoryUsage, VkMemoryPropertyFlags requiredFlags, VmaAllocationCreateFlags flags) {

        VkBufferCreateInfo bufferInfo{ init::create_buffer_info(bufferSize, bufferUsage) };
        VmaAllocationCreateInfo allocationInfo{};
        allocationInfo.flags = flags;
        allocationInfo.usage = memoryUsage;
        allocationInfo.requiredFlags = requiredFlags;
        vkt::Buffer buf{};
        VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &allocationInfo, &buf.buffer, &buf.allocation, &buf.allocationInfo));

        return buf;
    }

    vkt::IndexedMesh generate_cube_mesh() {
        return {
            .tInd { //triangles
                // top
                {2, 6, 7},
                {2, 3, 7},
                // bottom
                {0, 4, 5},
                {0, 1, 5},
                // left
                {0, 2, 6},
                {0, 4, 6},
                // right
                {1, 3, 7},
                {1, 5, 7},
                // front
                {0, 2, 3},
                {0, 1, 3},
                // back
                {4, 6, 7},
                {4, 5, 7},
        },

            .verts {
            {	{-0.5f, -0.5f, 0.75f}  },	//0
            {	{0.5f, -0.5f, 0.75f}  },	//1
            {	{-0.5f, 0.5f, 0.75f}  },	//2
            {	{0.5f, 0.5f, 0.75f}  },		//3
            {	{-0.5f, -0.5f, 0.25f}  },	//4
            {	{0.5f, -0.5f, 0.25f}  },	//5
            {	{-0.5f, 0.5f, 0.25f}  },	//6
            {	{0.5f, 0.5f, 0.25f}  },		//7
        }

        };
    }

    // depth reversed (far maps to 0, near maps to 1) and y flip is baked in. Effectively taking the left-hand coordinate system and mapping it to right-hand with y pointing down
    glm::mat4 orthographicProj(float left, float right, float bottom, float top, float near, float far) {
        // TO-DO: derive the orthographic view volume from the aspect ratio and vertical field-of-view
        return glm::mat4{
            2 / (right - left), 0, 0, -(right + left) / (right - left),
            0, 2 / (bottom - top), 0, (bottom + top) / (bottom - top),
            0, 0, 1 / (near - far), -near / (far - near),
            0, 0, 0, 1
        };
    }

    glm::mat4 orthographicProj(float vFov, float aspectRatio, float near, float far) {
        // far replaces near here as depth is reversed
        float top{ std::tan(vFov / 2.0f) * std::abs(far) };
        float right{ aspectRatio * top };

        return orthographicProj(-right, right, -top, top, near, far);
    }


}