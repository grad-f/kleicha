#ifndef UTILS_H
#define UTILS_H

#include "vulkan/vk_enum_string_helper.h"

#define VK_CHECK(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
            throw std::runtime_error(string_VkResult(err));				\
        }                                                               \
    } while (0)



#endif // !UTILS_H
