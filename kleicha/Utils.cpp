#include "Utils.h"
#include "Initializers.h"



#include <unordered_map>

#pragma warning(push)
#pragma warning(disable : 26495 6262 6054 4365)
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#pragma warning(pop)

namespace std {
    template<> struct hash<vkt::Vertex> {
        size_t operator()(vkt::Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.position) ^
                (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
                (hash<glm::vec2>()(vertex.UV) << 1);
        }
    };
}

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
        blitImageInfo.filter = VK_FILTER_LINEAR;

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

    void update_set_buffer_descriptor(VkDevice device, VkDescriptorSet set, uint32_t binding, VkDescriptorType descriptorType, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range) {
        VkDescriptorBufferInfo descriptorBufferInfo{ init::create_descriptor_buffer_info(buffer, offset, range) };

        VkWriteDescriptorSet writeDescriptorSet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        writeDescriptorSet.dstSet = set;
        writeDescriptorSet.dstBinding = binding;
        writeDescriptorSet.dstArrayElement = 0;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType = descriptorType;
        writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;

        vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
    }

    void update_set_image_sampler_descriptor(VkDevice device, VkDescriptorSet set, uint32_t binding, VkImageLayout imageSampledLayout, VkSampler sampler, const std::vector<vkt::Image>& images) {

        uint32_t imageCount{ static_cast<uint32_t>(images.size()) };

        std::vector<VkDescriptorImageInfo> imageInfos(imageCount);

        for (std::size_t i{ 0 }; i < imageCount; ++i) {
            imageInfos[i].sampler = sampler;
            imageInfos[i].imageView = images[i].imageView;
            imageInfos[i].imageLayout = imageSampledLayout;
        }
        VkWriteDescriptorSet writeDescriptorSetInfo{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        writeDescriptorSetInfo.dstSet = set;
        writeDescriptorSetInfo.dstBinding = binding;
        writeDescriptorSetInfo.dstArrayElement = 0;
        writeDescriptorSetInfo.descriptorCount = static_cast<uint32_t>(imageInfos.size());
        writeDescriptorSetInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSetInfo.pImageInfo = imageInfos.data();
        vkUpdateDescriptorSets(device, 1, &writeDescriptorSetInfo, 0, nullptr);
    }

    void update_set_image_sampler_descriptor(VkDevice device, VkDescriptorSet set, uint32_t binding, VkImageLayout imageSampledLayout, VkSampler sampler, const std::vector<vkt::CubeImage>& images) {

        uint32_t imageCount{ static_cast<uint32_t>(images.size()) };

        std::vector<VkDescriptorImageInfo> imageInfos(imageCount);

        for (std::size_t i{ 0 }; i < imageCount; ++i) {
            imageInfos[i].sampler = sampler;
            imageInfos[i].imageView = images[i].colorImage.imageView;
            imageInfos[i].imageLayout = imageSampledLayout;
        }
        VkWriteDescriptorSet writeDescriptorSetInfo{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        writeDescriptorSetInfo.dstSet = set;
        writeDescriptorSetInfo.dstBinding = binding;
        writeDescriptorSetInfo.dstArrayElement = 0;
        writeDescriptorSetInfo.descriptorCount = static_cast<uint32_t>(imageInfos.size());
        writeDescriptorSetInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSetInfo.pImageInfo = imageInfos.data();
        vkUpdateDescriptorSets(device, 1, &writeDescriptorSetInfo, 0, nullptr);
    }


    vkt::Mesh generate_cube_mesh() {
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
    }

    vkt::Mesh generate_pyramid_mesh() {
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
    vkt::Mesh generate_sphere(size_t prec) {

        // + 1 here is the additional aliased horizontal and vertical slice of vertices with different texture coordinates to resolve continuity issues at the edges of the texture
        size_t vertexCount{ (prec + 1) * (prec + 1) };
        size_t triangleCount{ prec * prec * 2 };
        // pre-allocate
        vkt::Mesh mesh{};
        mesh.verts.resize(vertexCount);
        mesh.tInd.resize(triangleCount);
        for (size_t i{ 0 }; i <= prec; ++i) { // traverse each slice
            for (size_t j{ 0 }; j <= prec; ++j) {
                // compute vertex position -> goes from -1 to 1 in increments defined by prec
                float y{ cos(glm::radians(180.0f - i * 180.0f / prec)) };
                // slice radius
                float x{ -cos(glm::radians(j * 360.0f / prec)) * abs(cos(asin(y))) };
                float z{ sin(glm::radians(j * 360.0f / prec)) * abs(cos(asin(y))) };
                mesh.verts[i * (prec + 1) + j].position = { x,y,z };
                mesh.verts[i * (prec + 1) + j].UV = { static_cast<float>(j) / prec, static_cast<float>(i) / prec };
                mesh.verts[i * (prec + 1) + j].normal = { x,y,z };

                //compute tangent
                if (((x == 0) && (y == 1) && (z == 0)) || ((x == 0) && (y == -1) && (z == 0)))
                    mesh.verts[i * (prec + 1) + j].tangent = glm::vec4{ 0.0f, 0.0f, -1.0f, 1.0f };
                else
                    mesh.verts[i * (prec + 1) + j].tangent = glm::vec4{ glm::cross(glm::vec3{ 0.0f, 1.0f, 0.0f }, glm::vec3{ x,y,z }), 1.0f };
            }
        }

        // compute triangle indices
        for (size_t i{ 0 }; i < prec; ++i) { // traverse each slice
            for (size_t j{ 0 }; j < prec; ++j) {
                mesh.tInd[2 * (i * prec + j)] = { i * (prec + 1) + j, i * (prec + 1) + j + 1, (i + 1) * (prec + 1) + j };
                mesh.tInd[2 * (i * prec + j) + 1] = { i * (prec + 1) + j + 1, (i + 1) * (prec + 1) + j + 1, (i + 1) * (prec + 1) + j };
            }
        }

        return mesh;
    }

    vkt::Mesh generate_torus(size_t prec, float inner, float outer) {
        size_t vertexCount{ (prec + 1) * (prec + 1) };
        size_t triangleCount{ prec * prec * 2 };

        vkt::Mesh mesh{};
        mesh.verts.resize(vertexCount);
        mesh.tInd.resize(triangleCount);

        // compute vertices of first slice of torus
        for (size_t i{ 0 }; i <= prec; ++i) {
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
            mesh.verts[i].normal = glm::cross(glm::vec3{ mesh.verts[i].tangent }, mesh.verts[i].bitangent);
        }

        // for each of the vertices that make up the initial slice, we rotate them about the y axis
        for (std::size_t ring{ 1 }; ring < prec + 1; ++ring) {
            // compute rotation amount
            float ringRadians{ glm::radians(ring * 360.0f / prec) };
            for (std::size_t vert{ 0 }; vert < prec + 1; ++vert) {
                glm::mat4 rMat{ glm::rotate(glm::mat4{1.0f}, ringRadians, glm::vec3{0.0f, 1.0f, 0.0f}) };
                mesh.verts[ring * (prec + 1) + vert].position = rMat * glm::vec4{ mesh.verts[vert].position, 1.0f };
                mesh.verts[ring * (prec + 1) + vert].UV = glm::vec2{static_cast<float>(ring * 3.0f) / prec, mesh.verts[vert].UV.t};

                // we're safe to rotate our direction vectors as the rotation matrix is orthonormal and the inverse transpose yields the same matrix
                mesh.verts[ring * (prec + 1) + vert].tangent = rMat * mesh.verts[vert].tangent;
                mesh.verts[ring * (prec + 1) + vert].bitangent = rMat * glm::vec4{ mesh.verts[vert].bitangent, 1.0f };
                mesh.verts[ring * (prec + 1) + vert].normal = rMat * glm::vec4{ mesh.verts[vert].normal, 1.0f };
            }
        }

        for (size_t ring{0}; ring < prec; ++ring) {
            for (size_t vert{ 0 }; vert < prec; ++vert) {
                mesh.tInd[2 * (ring * prec + vert)] = { ring * (prec + 1) + vert, (ring + 1) * (prec + 1) + vert, (ring * (prec + 1) + vert + 1) };
                mesh.tInd[2 * (ring * prec + vert) + 1] = { (ring * (prec + 1) + vert + 1), (ring + 1) * (prec + 1) + vert, (ring + 1) * (prec + 1) + vert + 1 };
            }
        }

        return mesh;
    }

    void compute_mesh_tangents(vkt::Mesh& mesh) {

        std::size_t vertsCount{ mesh.verts.size() };
        std::vector<glm::vec3> tan1(vertsCount * 2);
        glm::vec3* tan2{ tan1.data() + vertsCount };

        for (const auto& triangle : mesh.tInd) {

            // get indices to be used to index this triangle's vertices
            uint32_t i1 = triangle.x;
            uint32_t i2 = triangle.y;
            uint32_t i3 = triangle.z;

            // store references to triangle vertex positions and texture coords
            const glm::vec3& v1{ mesh.verts[i1].position };
            const glm::vec3& v2{ mesh.verts[i2].position };
            const glm::vec3& v3{ mesh.verts[i3].position };

            const glm::vec2& w1{ mesh.verts[i1].UV };
            const glm::vec2& w2{ mesh.verts[i2].UV };
            const glm::vec2& w3{ mesh.verts[i3].UV };

            float x1{ v2.x - v1.x };
            float x2{ v3.x - v1.x };
            float y1{ v2.y - v1.y };
            float y2{ v3.y - v1.y };
            float z1{ v2.z - v1.z };
            float z2{ v3.z - v1.z };
            float s1{ w2.x - w1.x };
            float s2{ w3.x - w1.x };
            float t1{ w2.y - w1.y };
            float t2{ w3.y - w1.y };

            float r{ 1.0f / (s1 * t2 - s2 * t1) };
            glm::vec3 sdir{ (t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r };
            glm::vec3 tdir{ (s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r };

            tan1[i1] += sdir;
            tan1[i2] += sdir;
            tan1[i3] += sdir;

            tan2[i1] += tdir;
            tan2[i2] += tdir;
            tan2[i3] += tdir;
        }

        for (std::size_t i{ 0 }; i < vertsCount; ++i) {
            const glm::vec3& N = mesh.verts[i].normal;
            const glm::vec3& T = tan1[i];

            // re-orthogonalize 
            mesh.verts[i].tangent = glm::vec4{ glm::normalize((T - N * glm::dot(N, T))), 0.0f };

            // calculuate tangent space handedness. we will factor this into our bitangent computations
            mesh.verts[i].tangent.w = (glm::dot(glm::cross(N, T), tan2[i]) < 0.0f) ? -1.0f : 1.0f;
        }
    }

    vkt::Mesh load_obj_mesh(const char* filePath) {
        // attrib stores arrays of vertex attributes as floats
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::string err{};

        vkt::Mesh mesh{};
        if (!tinyobj::LoadObj(&attrib, &shapes, nullptr, &err, filePath)) {
            throw std::runtime_error{ "[Kleicha] Failed to load mesh from file: " + std::string{filePath} + "\nError: " + err };
        }
        
        // hash map to store unique vertices and their indices
        std::unordered_map<vkt::Vertex, uint32_t> uniqueVertices{};
        std::vector<uint32_t> triangleIndices{};
        for (const auto& shape : shapes) {
            for (uint32_t vert{ 0 }; vert < shape.mesh.indices.size(); ++vert) {

                // get triangle vertex indices
                std::size_t posIndex{ static_cast<std::size_t>(shape.mesh.indices[vert].vertex_index) };
                std::size_t normIndex{ static_cast<std::size_t>(shape.mesh.indices[vert].normal_index) };
                std::size_t texIndex{ static_cast<std::size_t>(shape.mesh.indices[vert].texcoord_index) };
                vkt::Vertex vertex{};

                // index the vertex data stored in attrib using the indices to generate the vertex data
                vertex.position = { attrib.vertices[3 * posIndex + 0], attrib.vertices[3 * posIndex + 1], attrib.vertices[3 * posIndex + 2] };
                vertex.normal = { attrib.normals[3 * normIndex + 0], attrib.normals[3 * normIndex + 1], attrib.normals[3 * normIndex + 2] };
                vertex.UV = { attrib.texcoords[2 * texIndex + 0], attrib.texcoords[2 * texIndex + 1] };

                // check if vertex already exists in our unique vertices map
                if (uniqueVertices.count(vertex) == 0) {
                    // if unique vertex, add to unique vertex map
                    uniqueVertices[vertex] = static_cast<uint32_t>(mesh.verts.size());
                    mesh.verts.push_back(vertex);
                }

                switch (vert % 3) {
                case 0:
                    mesh.tInd.push_back(glm::uvec3{ uniqueVertices[vertex] });
                    break;
                case 1:
                    mesh.tInd[mesh.tInd.size() - 1].y = uniqueVertices[vertex];
                    break;
                case 2:
                    mesh.tInd[mesh.tInd.size() - 1].z = uniqueVertices[vertex];
                    break;
                }

            }
        }

        // compute tangents
        compute_mesh_tangents(mesh);

        fmt::println("Loaded Model: {}", filePath);

        return mesh;
    }

    bool load_gltf(const char* filePath, std::vector<vkt::Mesh>& meshes, std::vector<vkt::DrawData>& draws, std::vector<vkt::Transform>& transforms, std::vector<vkt::TextureIndices>& textureIndices, std::vector<std::string>& texturePaths) {
        cgltf_options options{};
        cgltf_data* data{};
        cgltf_result result{ cgltf_parse_file(&options, filePath, &data) };

        std::size_t materialOffset{ textureIndices.size() };

        uint32_t textureOffset{ 1 + static_cast<uint32_t>(texturePaths.size()) };

        if (result != cgltf_result_success) {
            cgltf_free(data);
            return false;
        }

        result = cgltf_load_buffers(&options, data, filePath);

        if (result != cgltf_result_success) {
            cgltf_free(data);
            return false;
        }

        cgltf_validate(data);

        // get default scene
        cgltf_scene* scene{ data->scene };

        for (std::size_t n{ 0 }; n < scene->nodes_count; ++n) {
            // get the scaling that should be applied to each vertex
            cgltf_node* node{ scene->nodes[n] };

            glm::mat4 model{ 1.0f };
            if (node->has_scale || node->has_rotation || node->has_scale) {
                cgltf_node_transform_local(node, &(model[0].x));
            }

            // traverse meshes (in the case of sponza it is a single mesh)
            for (std::size_t i{ 0 }; i < data->meshes_count; ++i) {

                //TODO: Our sponza stores meshes as primitives... this isn't the norm and so we handle the other case where meshes correspond to different meshes in the scene...
                const cgltf_mesh& mesh{ data->meshes[i] };

                for (std::size_t j{ 0 }; j < mesh.primitives_count; ++j) {

                    const cgltf_primitive& prim{ mesh.primitives[j] };

                    std::size_t vertexCount{ prim.attributes[0].data->count};
                    std::vector<vkt::Vertex> vertices(vertexCount);
                    std::vector<float> scratchData(vertexCount * 4);

                    if (const cgltf_accessor* pos = cgltf_find_accessor(&prim, cgltf_attribute_type_position, 0)) {

                        assert(cgltf_num_components(pos->type) == 3);

                        cgltf_accessor_unpack_floats(pos, scratchData.data(), vertexCount * 3);

                        for (std::size_t x{ 0 }; x < vertexCount; ++x) {
                            vertices[x].position = { scratchData[x * 3 + 0], scratchData[x * 3 + 1], scratchData[x * 3 + 2]};
                        }
                    }

                    if (const cgltf_accessor* tex = cgltf_find_accessor(&prim, cgltf_attribute_type_texcoord, 0)) {

                        assert(cgltf_num_components(tex->type) == 2);

                        cgltf_accessor_unpack_floats(tex, scratchData.data(), vertexCount * 2);

                        for (std::size_t x{ 0 }; x < vertexCount; ++x) {
                            vertices[x].UV = { scratchData[x * 2 + 0], scratchData[x * 2 + 1] };
                        }
                    }

                    if (const cgltf_accessor* norm = cgltf_find_accessor(&prim, cgltf_attribute_type_normal, 0)) {

                        assert(cgltf_num_components(norm->type) == 3);

                        cgltf_accessor_unpack_floats(norm, scratchData.data(), vertexCount * 3);

                        for (std::size_t x{ 0 }; x < vertexCount; ++x) {
                            vertices[x].normal = { scratchData[x * 3 + 0], scratchData[x * 3 + 1], scratchData[x * 3 + 2] };
                        }

                    }

                    bool computeTangent{ true };
                    if (const cgltf_accessor* tan = cgltf_find_accessor(&prim, cgltf_attribute_type_tangent, 0)) {

                        assert(cgltf_num_components(tan->type) == 4);
                        cgltf_accessor_unpack_floats(tan, scratchData.data(), vertexCount * 4);

                        for (std::size_t x{ 0 }; x < vertexCount; ++x) {
                            vertices[x].tangent = { scratchData[x * 4 + 0], scratchData[x * 4 + 1], scratchData[x*4+2], scratchData[x*4+3]};
                        }

                        computeTangent = false;
                    }

                    std::vector<glm::uvec3> indices(prim.indices->count / 3);
                    cgltf_accessor_unpack_indices(prim.indices, indices.data(), 4, indices.size() * 3);
                    cgltf_size cgltf_accessor_unpack_indices(const cgltf_accessor * accessor, void* out, cgltf_size out_component_size, cgltf_size index_count);

                    cgltf_size matIndex{ cgltf_material_index(data, prim.material) };
                        
                    bool useAlphaTest{ false };
                    if (prim.material->alpha_mode == cgltf_alpha_mode_mask)
                        useAlphaTest = true;

                    meshes.push_back(vkt::Mesh{ indices, vertices, useAlphaTest, computeTangent });
                    transforms.push_back(vkt::Transform{ model });
                    draws.push_back(vkt::DrawData{ 0, static_cast<uint32_t>(matIndex + materialOffset), static_cast<uint32_t>(transforms.size() - 1) });
                }
                
            }
        }

        for (std::size_t i{ 0 }; i < data->materials_count; ++i) {
            
            cgltf_material* material{ &data->materials[i] };
            vkt::TextureIndices indices{};
            
            if (material->has_pbr_metallic_roughness) {
                if (material->pbr_metallic_roughness.base_color_texture.texture)
                    indices.albedoTexture = textureOffset + static_cast<uint32_t>(cgltf_texture_index(data, material->pbr_metallic_roughness.base_color_texture.texture));

                if (material->pbr_metallic_roughness.metallic_roughness_texture.texture)
                    indices.heightTexture = textureOffset + static_cast<uint32_t>(cgltf_texture_index(data, material->pbr_metallic_roughness.metallic_roughness_texture.texture));
            }

            if (material->normal_texture.texture)
                indices.normalTexture = textureOffset + static_cast<uint32_t>(cgltf_texture_index(data, material->normal_texture.texture));

            if (material->emissive_texture.texture)
                indices.emissiveTexture = textureOffset + static_cast<uint32_t>(cgltf_texture_index(data, material->emissive_texture.texture));

            textureIndices.push_back(indices);
        }

        for (std::size_t i{ 0 }; i < data->textures_count; ++i) {
            cgltf_texture* texture{ &data->textures[i] };
            assert(texture->image);

            cgltf_image* image{ texture->image };
            assert(image->uri);

            std::string sPath{ filePath };
            std::size_t pos{ sPath.find_last_of("/\\") };
            // handle case where files are in the project folder
            if (pos == std::string::npos)
                sPath = "";
            else
                sPath = sPath.substr(0, pos + 1);

            std::string uri{ image->uri };
            // handle special characters
            uri.resize(cgltf_decode_uri(&uri[0]));

            texturePaths.push_back(sPath + uri);
        }

        cgltf_free(data);
        return true;
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

    void set_viewport_scissor(const vkt::Frame& frame, VkExtent2D extent) {
        VkViewport viewport{};
        viewport.x = 0;
        viewport.y = 0;
        viewport.y = 0;
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent = extent;

        vkCmdSetViewport(frame.cmdBuffer, 0, 1, &viewport);
        vkCmdSetScissor(frame.cmdBuffer, 0, 1, &scissor);
    }
}