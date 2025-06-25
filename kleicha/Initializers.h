#ifndef INITIALIZERS_H
#define INITIALIZERS_H

#include "vulkan/vulkan.h"

/*	 vulkan structure creation helpers	*/

namespace init {
	VkDebugUtilsMessengerCreateInfoEXT create_debug_utils_messenger_info(PFN_vkDebugUtilsMessengerCallbackEXT pfnUserCallack);

	VkImageMemoryBarrier2 create_image_barrier_info(VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask,
		VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout, VkImage image);

	VkImageCreateInfo create_image_info(VkFormat format, VkExtent2D extent, VkImageUsageFlags usage);

	VkImageViewCreateInfo create_image_view_info(VkImage image, VkFormat format, VkImageAspectFlags aspectMask);
}
#endif // !INITIALIZERS_H
