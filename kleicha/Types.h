#ifndef TYPES_H
#define TYPES_H

#include "vulkan/vulkan.h"

namespace vkt {
	struct Instance {
		VkInstance instance{};
		VkDebugUtilsMessengerEXT debugMessenger{};
		PFN_vkCreateDebugUtilsMessengerEXT pfnCreateMessenger{};
		PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroyMessenger{};
	};
}
#endif // !TYPES_H
