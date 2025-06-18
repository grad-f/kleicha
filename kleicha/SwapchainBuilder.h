#ifndef SWAPCHAINBUILDER_H
#define SWAPCHAINBUILDER_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <algorithm>

#include "Types.h"

class SwapchainBuilder {
public:
	SwapchainBuilder(VkInstance instance, GLFWwindow* window, VkSurfaceKHR surface, const vkt::Device& device)
		: m_instance{ instance }, m_window{ window }, m_surface{ surface }, 
		m_surfaceSupportDetails{ device.physicalDevice.surfaceSupportDetails }, m_device{ device.device }
	{}
	vkt::Swapchain build();

	SwapchainBuilder& desired_image_format(VkSurfaceFormatKHR format) {
		m_desiredImageFormat = format;
		return *this;
	}

	SwapchainBuilder& desired_image_usage(VkImageUsageFlags imageUsage) {
		m_desiredImageUsage = m_desiredImageUsage | imageUsage;
		return *this;
	}

	SwapchainBuilder& desired_present_mode(VkPresentModeKHR presentMode) {
		m_desiredPresentMode = presentMode;
		return *this;
	}

private:
	VkInstance m_instance{};
	GLFWwindow* m_window{};
	VkSurfaceKHR m_surface{};
	VkDevice m_device{};
	vkt::SurfaceSupportDetails m_surfaceSupportDetails{};

	VkSurfaceFormatKHR m_desiredImageFormat{};
	VkImageUsageFlags m_desiredImageUsage{ VK_IMAGE_USAGE_STORAGE_BIT };
	VkPresentModeKHR m_desiredPresentMode{};

	VkSurfaceFormatKHR get_swapchain_format() const;
	VkExtent2D get_swapchain_image_extent() const;
	VkPresentModeKHR get_swapchain_present_mode() const;
};

#endif // !SWAPCHAINBUILDER_H
