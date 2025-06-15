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

	struct DeviceFeatures {
		VkPhysicalDeviceVulkan11Features Vk11Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
		VkPhysicalDeviceVulkan12Features Vk12Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
		VkPhysicalDeviceVulkan13Features Vk13Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
		VkPhysicalDeviceVulkan14Features Vk14Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES };
	};
}
#endif // !TYPES_H
