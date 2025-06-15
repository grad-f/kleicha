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

	// chained and encapsulated device features struct
	struct DeviceFeatures {
		VkPhysicalDeviceFeatures2 VkFeatures{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &Vk11Features };
		VkPhysicalDeviceVulkan11Features Vk11Features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES, .pNext = &Vk12Features };
		VkPhysicalDeviceVulkan12Features Vk12Features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, .pNext = &Vk13Features };
		VkPhysicalDeviceVulkan13Features Vk13Features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES, .pNext = &Vk14Features };
		VkPhysicalDeviceVulkan14Features Vk14Features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES };
	};
}
#endif // !TYPES_H
