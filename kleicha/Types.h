#ifndef TYPES_H
#define TYPES_H

#include <vector>
#include "vk_mem_alloc.h"
#include "vulkan/vulkan.h"

namespace vkt {
	struct Instance {
		VkInstance instance{};
		VkDebugUtilsMessengerEXT debugMessenger{};
		PFN_vkCreateDebugUtilsMessengerEXT pfnCreateMessenger{};
		PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroyMessenger{};
	};

	// stores the surface support for our chosen physical device
	struct SurfaceSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities{};
		std::vector<VkSurfaceFormatKHR> formats{};
		std::vector<VkPresentModeKHR> presentModes{};
	};

	struct PhysicalDevice {
		VkPhysicalDevice device{};
		VkPhysicalDeviceProperties2 deviceProperties{};
		// supports graphics, compute, transfer, and presentation
		uint32_t queueFamilyIndex{};
		vkt::SurfaceSupportDetails surfaceSupportDetails{};
	};

	struct Device {
		PhysicalDevice physicalDevice{};
		VkDevice device{};
		VkQueue queue{};
	};

	struct Swapchain {
		VkSwapchainKHR swapchain{};
		std::vector<VkImage> images{};
		std::size_t imageCount{};
		std::vector<VkImageView> imageViews{};
		VkExtent2D imageExtent{};
	};

	// encapsulates data needed for each frame
	struct Frame {
		VkCommandBuffer cmdBuffer{};
		VkFence inFlightFence{};
		VkSemaphore acquiredSemaphore{};
	};

	// chained and encapsulated device features struct
	struct DeviceFeatures {
		VkPhysicalDeviceFeatures2 VkFeatures{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &Vk11Features };
		VkPhysicalDeviceVulkan11Features Vk11Features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES, .pNext = &Vk12Features };
		VkPhysicalDeviceVulkan12Features Vk12Features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, .pNext = &Vk13Features };
		VkPhysicalDeviceVulkan13Features Vk13Features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES, .pNext = &Vk14Features };
		VkPhysicalDeviceVulkan14Features Vk14Features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES };
	};

	struct Image {
		VkImage image{};
		VmaAllocation allocation{};
		VmaAllocationInfo allocationInfo{};
	};
}
#endif // !TYPES_H
