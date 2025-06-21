#ifndef UTILS_H
#define UTILS_H

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
    inline VkShaderModule create_shader_module([[maybe_unused]]VkDevice device, const char* path) {
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

}
#endif // !UTILS_H
