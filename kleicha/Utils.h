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
    VkShaderModule create_shader_module(VkDevice device, const char* path);
}
#endif // !UTILS_H
