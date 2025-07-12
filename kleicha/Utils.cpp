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
        VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout, VkImage image, uint32_t mipLevels) {

        VkImageMemoryBarrier2 barrier{ init::create_image_barrier_info(srcStageMask, srcAccessMask, dstStageMask, dstAccessMask, oldLayout, newLayout, image, mipLevels) };

        VkDependencyInfo dependencyInfo{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
        dependencyInfo.pNext = nullptr;
        dependencyInfo.imageMemoryBarrierCount = 1;
        dependencyInfo.pImageMemoryBarriers = &barrier;

        vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);
    }

    void blit_image(VkCommandBuffer cmdBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, VkExtent2D srcExtent, VkExtent2D dstExtent, uint32_t srcMipLevel, uint32_t dstMipLevel) {
        VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2 };
        blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.srcSubresource.baseArrayLayer = 0;
        blitRegion.srcSubresource.layerCount = 1;
        blitRegion.srcSubresource.mipLevel = srcMipLevel;

        blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.dstSubresource.baseArrayLayer = 0;
        blitRegion.dstSubresource.layerCount = 1;
        blitRegion.dstSubresource.mipLevel = dstMipLevel;

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

/*    vkt::IndexedMesh generate_cube_mesh() {
        return {
            .tInd { //triangles
                // top
                {6, 2, 7},
                {2, 3, 7},
                // bottom
                {0, 4, 5},
                {1, 0, 5},
                // left
                {0, 2, 6},
                {4, 0, 6},
                // right
                {3, 1, 7},
                {1, 5, 7},
                // front
                {2, 0, 3},
                {0, 1, 3},
                // back
                {4, 6, 7},
                {5, 4, 7},
        },

            .verts {
            {	{-1.0f, -1.0f, 1.0f}  },	//0
            {	{1.0f, -1.0f, 1.0f}  },	    //1
            {	{-1.0f, 1.0f, 1.0f}  },	    //2
            {	{1.0f, 1.0f, 1.0f}  },		//3
            {	{-1.0f, -1.0f, -1.0f}  },	//4
            {	{1.0f, -1.0f, -1.0f}  },	//5
            {	{-1.0f, 1.0f, -1.0f}  },	//6
            {	{1.0f, 1.0f, -1.0f}  },		//7
        }
        };
    }*/

    vkt::IndexedMesh generate_pyramid_mesh() {
        return {
            .tInd {
                {6,2,5},
                {4,5,2},
                {2,6,3},
                {6,1,3},
                {1,0,3},
                {0,2,3},
            },

            .verts {
                {   {-1.0f, -1.0f, -1.0f},   {1.0f, 0.0f}      },      // base top left          0
                {   {1.0f, -1.0f, -1.0f},    {0.0f, 0.0f}      },      // base top right         1
                {   {-1.0f, -1.0f, 1.0f},    {0.0f, 0.0f}      },      // base bottom left       2
                {   {0.0f, 1.0f, 0.0f},     {0.5f, 1.0f}      },       // top                    3

                {   {-1.0f, -1.0f, -1.0f},   {0.0f, 1.0f}      },      // base top left          4
                {   {1.0f, -1.0f, -1.0f},    {1.0f, 1.0f}      },      // base top right         5
                {   {1.0f, -1.0f, 1.0f},     {1.0f, 0.0f}      },      // base bottom right      6

            }
        };
    }

    // procedurally generated sphere - prec defines the number of vertices per slice and number of slices
    vkt::IndexedMesh generate_sphere(uint32_t prec) {

        // + 1 here is the additional aliased horizontal and vertical slice of vertices with different texture coordinates to resolve continuity issues at the edges of the texture
        uint32_t vertexCount{ (prec + 1) * (prec + 1) };
        uint32_t triangleCount{ prec * prec * 2 };
        // pre-allocate
        vkt::IndexedMesh mesh{};
        mesh.verts.resize(vertexCount);
        mesh.tInd.resize(triangleCount);
        for (uint32_t i{ 0 }; i <= prec; ++i) { // traverse each slice
            for (uint32_t j{ 0 }; j <= prec; ++j) {
                // compute vertex position -> goes from -1 to 1 in increments defined by prec
                float y{ cos(glm::radians(180.0f - i * 180.0f / prec)) };
                // slice radius
                float x{ -cos(glm::radians(j * 360.0f / prec)) * abs(cos(asin(y))) };
                float z{ sin(glm::radians(j * 360.0f / prec)) * abs(cos(asin(y))) };
                mesh.verts[i * (prec + 1) + j].position = { x,y,z };
                mesh.verts[i * (prec + 1) + j].UV = { static_cast<float>(j) / prec, static_cast<float>(i) / prec };
                mesh.verts[i * (prec + 1) + j].normal = { x,y,z };
            }
        }

        // compute triangle indices
        for (uint32_t i{ 0 }; i < prec; ++i) { // traverse each slice
            for (uint32_t j{ 0 }; j < prec; ++j) {
                mesh.tInd[2 * (i * prec + j)] = { i * (prec + 1) + j, i * (prec + 1) + j + 1, (i + 1) * (prec + 1) + j };
                mesh.tInd[2 * (i * prec + j) + 1] = { i * (prec + 1) + j + 1, (i + 1) * (prec + 1) + j + 1, (i + 1) * (prec + 1) + j };
            }
        }

        return mesh;
    }

    glm::mat4 lookAt(glm::vec3 eye, glm::vec3 lookat, glm::vec3 up) {
        // create rh uvw basis
        glm::vec3 w{ -glm::normalize(lookat - eye) };
        glm::vec3 u{ glm::normalize(glm::cross(up, w)) };
        glm::vec3 v{ glm::cross(w, u) };

        glm::mat4 invTranslate{
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            -eye.x, -eye.y, -eye.z, 1.0f
        };

        glm::mat4 invRotate{
            u.x, v.x, -w.x, 0.0f,
            u.y, v.y, -w.y, 0.0f,
            u.z, v.z, -w.z, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };

        return invRotate * invTranslate;
    }

    glm::mat4 perspective(float near, float far) {
        return glm::mat4{
            near, 0.0f, 0.0f, 0.0f,
            0.0f, near, 0.0f, 0.0f,
            0.0f, 0.0f, near + far, 1.0f,
            0.0f, 0.0f, -far * near, 0.0f
        };
    }

    glm::mat4 orthographicProj(float left, float right, float bottom, float top, float near, float far) {
        return glm::mat4{
            2.0f / (right - left), 0.0f, 0.0f, 0.0f,
            0.0f, 2.0f / (bottom - top), 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f / (far-near), 0.0f,
            -(right + left)/(right-left), (bottom + top)/(bottom-top), -near/(far-near), 1.0f
        };
    }

    glm::mat4 orthographicProj(float vFov, float aspectRatio, float near, float far) {
        float top{ std::tan(vFov / 2.0f) * std::abs(near) };
        float right{ aspectRatio * top };

        return orthographicProj(-right, right, -top, top, near, far);
    }
}