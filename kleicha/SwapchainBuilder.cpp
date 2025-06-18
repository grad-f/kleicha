#include <stdexcept>
#include "Utils.h"
#include "SwapchainBuilder.h"

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


vkt::Swapchain SwapchainBuilder::build() {

	// choose number of swapchain images
	uint32_t swapchainImageCount{ m_surfaceSupportDetails.capabilities.minImageCount + 1 };
	if (swapchainImageCount > m_surfaceSupportDetails.capabilities.maxImageCount)
		swapchainImageCount = m_surfaceSupportDetails.capabilities.maxImageCount;

	VkSurfaceFormatKHR swapchainImageFormat{ get_swapchain_format() };

	// check surface current extent and handle retina display case where a unit screen coordinate could represent 2 or more pixels.
	VkExtent2D swapchainImageExtent{ get_swapchain_image_extent() };

	// check if requested image usage is supported
	if ((m_surfaceSupportDetails.capabilities.supportedUsageFlags & m_desiredImageUsage) != m_desiredImageUsage)
		throw std::runtime_error{ "[SwapchainBuilder] Requested swapchain image usage is not supported by the physical device and surface." };

	// choose present mode -- default to FIFO
	VkPresentModeKHR swapchainPresentMode{ get_swapchain_present_mode() };

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

	fmt::println("[SwapchainBuilder] Created swapchain.");

	// get swapchain images since we only specified the minimum it could be.
	uint32_t imageCount{};
	VK_CHECK(vkGetSwapchainImagesKHR(m_device, swapchain, &imageCount, nullptr));
	std::vector<VkImage> images(imageCount);
	VK_CHECK(vkGetSwapchainImagesKHR(m_device, swapchain, &imageCount, images.data()));

	// create image views
	std::vector<VkImageView> imageViews(imageCount);
	for (std::size_t i{ 0 }; i < imageCount; ++i) {
		VkImageViewCreateInfo imageViewInfo{ .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		imageViewInfo.pNext = nullptr;
		imageViewInfo.image = images[i];
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewInfo.format = swapchainImageFormat.format;
		// allows us to remap image pixel components (changes how the image viewer interprets the image)
		// VK_COMPONENT_SWIZZLE_IDENTITY effectively means don't remap.
		imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		// specify which sub region of the image we'd like to make an imageview of
		imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewInfo.subresourceRange.baseMipLevel = 0;
		imageViewInfo.subresourceRange.levelCount = 1;
		imageViewInfo.subresourceRange.baseArrayLayer = 0;
		imageViewInfo.subresourceRange.layerCount = 1;
		VK_CHECK(vkCreateImageView(m_device, &imageViewInfo, nullptr, &imageViews[i]));
	}
	return vkt::Swapchain{ .swapchain = swapchain, .images = images, .imageViews = imageViews };
}