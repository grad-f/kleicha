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

    vkt::IndexedMesh generate_torus(uint32_t prec, float inner, float outer) {
        uint32_t vertexCount{ (prec + 1) * (prec + 1) };
        uint32_t triangleCount{ prec * prec * 2 };

        vkt::IndexedMesh mesh{};
        mesh.verts.resize(vertexCount);
        mesh.tInd.resize(triangleCount);

        // compute vertices of first slice of torus
        for (uint32_t i{ 0 }; i <= prec; ++i) {
            // get ith vertex radian measure
            float vertRadians{ glm::radians(i * 360.0f / prec) };

            // build rot matrix around z using vertex radian displacement
            glm::mat4 rot{ glm::rotate(glm::mat4{1.0f}, vertRadians, glm::vec3{0.0f, 0.0f, 1.0f}) };
            // init pos is the initial slice vertex position with outer radius
            glm::vec3 initPos{ rot * glm::vec4{0.0f, outer, 0.0f, 1.0f} };
            // store the init pos displaced by inner units in the x
            mesh.verts[i].position = initPos + glm::vec3{ inner, 0.0f, 0.0f };
            // all vertices that share this slice will map to a vertical stripe in the texture image
            mesh.verts[i].UV = glm::vec2{ 0.0f, static_cast<float>(i) / prec };
            // rotation about z by vertRadians + 90 degrees
            rot = glm::rotate(glm::mat4{ 1.0f }, vertRadians + glm::radians(90.0f), glm::vec3{0.0f, 0.0f, 1.0f});
            mesh.verts[i].tangent = rot * glm::vec4{0.0f, -1.0f, 0.0f, 1.0f};
            mesh.verts[i].bitangent = glm::vec3{ 0.0f, 0.0f, -1.0f };
            mesh.verts[i].normal = glm::cross(mesh.verts[i].tangent, mesh.verts[i].bitangent);
        }

        // for each of the vertices that make up the initial slice, we rotate them about the y axis
        for (uint32_t ring{ 1 }; ring < prec + 1; ++ring) {
            // compute rotation amount
            float ringRadians{ glm::radians(ring * 360.0f / prec) };
            for (uint32_t vert{ 0 }; vert < prec + 1; ++vert) {
                glm::mat4 rMat{ glm::rotate(glm::mat4{1.0f}, ringRadians, glm::vec3{0.0f, 1.0f, 0.0f}) };
                mesh.verts[ring * (prec + 1) + vert].position = rMat * glm::vec4{ mesh.verts[vert].position, 1.0f };
                mesh.verts[ring * (prec + 1) + vert].UV = glm::vec2{ring * 2.0f/prec, mesh.verts[vert].UV.t};

                // we're safe to rotate our direction vectors as the rotation matrix is orthonormal and the inverse transpose yields the same matrix
                mesh.verts[ring * (prec + 1) + vert].tangent = rMat * glm::vec4{mesh.verts[vert].tangent, 1.0f};
                mesh.verts[ring * (prec + 1) + vert].bitangent = rMat * glm::vec4{ mesh.verts[vert].bitangent, 1.0f };
                mesh.verts[ring * (prec + 1) + vert].normal = rMat * glm::vec4{ mesh.verts[vert].normal, 1.0f };
            }
        }

        for (uint32_t ring{0}; ring < prec; ++ring) {
            for (uint32_t vert{ 0 }; vert < prec; ++vert) {
                mesh.tInd[2 * (ring * prec + vert)] = { ring * (prec + 1) + vert, (ring + 1) * (prec + 1) + vert, (ring * (prec + 1) + vert + 1) };
                mesh.tInd[2 * (ring * prec + vert) + 1] = { (ring * (prec + 1) + vert + 1), (ring + 1) * (prec + 1) + vert, (ring + 1) * (prec + 1) + vert + 1 };
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