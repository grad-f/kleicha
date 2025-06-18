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
		m_surfaceSupportDetails{ device.physicalDevice.surfaceSupportDetails }, m_device{device.device}
	{}
	VkSwapchainKHR build();

	SwapchainBuilder& desired_image_format(VkSurfaceFormatKHR format) {
		m_desiredImageFormat = format;
		return *this;
	}

	SwapchainBuilder& desired_image_usage(VkImageUsageFlags imageUsage) {
		m_desiredImageUsage = imageUsage;
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
	VkImageUsageFlags m_desiredImageUsage{};
	VkPresentModeKHR m_desiredPresentMode{};

	VkSurfaceFormatKHR get_swapchain_format() const;
	VkExtent2D get_swapchain_image_extent() const;
	VkPresentModeKHR get_swapchain_present_mode() const;
};

VkSurfaceFormatKHR SwapchainBuilder::get_swapchain_format() const {
	// choose image format
	for (const auto& deviceCompatibleFormat : m_surfaceSupportDetails.formats) {
		// use desired image format if supported by the physical device and surface
		if (deviceCompatibleFormat.format == m_desiredImageFormat.format && deviceCompatibleFormat.colorSpace == m_desiredImageFormat.colorSpace)
			return m_desiredImageFormat;
	}
	return m_surfaceSupportDetails.formats[0];
}

VkExtent2D SwapchainBuilder::get_swapchain_image_extent() const {
	if (m_surfaceSupportDetails.capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
		// get window framebuffer size 
		int x{}, y{};
		glfwGetFramebufferSize(m_window, &x, &y);

		// clamp in case the frame buffer size exceeds the surface and device supported bounds
		return VkExtent2D{ std::clamp(static_cast<uint32_t>(x), m_surfaceSupportDetails.capabilities.minImageExtent.width, m_surfaceSupportDetails.capabilities.maxImageExtent.width),
			std::clamp(static_cast<uint32_t>(y), m_surfaceSupportDetails.capabilities.minImageExtent.height, m_surfaceSupportDetails.capabilities.maxImageExtent.height) };
	}

	return m_surfaceSupportDetails.capabilities.currentExtent;
}

VkPresentModeKHR SwapchainBuilder::get_swapchain_present_mode() const {
	for (const auto& supportedPresentMode : m_surfaceSupportDetails.presentModes) {
		if (supportedPresentMode == m_desiredPresentMode) {
			return m_desiredPresentMode;
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}


VkSwapchainKHR SwapchainBuilder::build() {

	// choose number of swapchain images
	uint32_t swapchainImageCount{ m_surfaceSupportDetails.capabilities.minImageCount + 1 };
	if (swapchainImageCount > m_surfaceSupportDetails.capabilities.maxImageCount)
		swapchainImageCount = m_surfaceSupportDetails.capabilities.maxImageCount;

	VkSurfaceFormatKHR swapchainImageFormat{ get_swapchain_format() };

	// check surface current extent and handle retina display case where a unit screen coordinate could represent 2 or more pixels.
	VkExtent2D swapchainImageExtent{ get_swapchain_image_extent()};

	// check if requested image usage is supported
	if ((m_surfaceSupportDetails.capabilities.supportedUsageFlags & m_desiredImageUsage) != m_desiredImageUsage)
		throw std::runtime_error{ "[SwapchainBuilder] Requested swapchain image usage is not supported by the physical device and surface." };

	// choose present mode -- default to FIFO
	VkPresentModeKHR swapchainPresentMode{ get_swapchain_present_mode()};

	// build swapchain
	VkSwapchainCreateInfoKHR swapchainInfo{ .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	swapchainInfo.pNext = nullptr;
	swapchainInfo.surface = m_surface;
	swapchainInfo.minImageCount = swapchainImageCount;
	swapchainInfo.imageFormat = swapchainImageFormat.format;
	swapchainInfo.imageColorSpace = swapchainImageFormat.colorSpace;
	swapchainInfo.imageExtent = swapchainImageExtent;
	swapchainInfo.imageArrayLayers = 1;
	swapchainInfo.imageUsage = m_desiredImageUsage;
	swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	// number of queue families that have access to the swapchain images
	swapchainInfo.queueFamilyIndexCount = 0;	// queue family info only required when sharing mode is not exclusive
	swapchainInfo.pQueueFamilyIndices = nullptr;
	swapchainInfo.preTransform = m_surfaceSupportDetails.capabilities.currentTransform;
	swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainInfo.presentMode = swapchainPresentMode;
	swapchainInfo.clipped = VK_TRUE;
	swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

	VkSwapchainKHR swapchain{};
	VK_CHECK(vkCreateSwapchainKHR(m_device, &swapchainInfo, nullptr, &swapchain));

	return swapchain;
}

#endif // !SWAPCHAINBUILDER_H
