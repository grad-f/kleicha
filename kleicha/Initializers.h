#ifndef INITIALIZERS_H
#define INITIALIZERS_H

#include "vulkan/vulkan.h"
#include "Types.h"

/*	 vulkan structure creation helpers	*/

namespace init {
	VkDebugUtilsMessengerCreateInfoEXT create_debug_utils_messenger_info(PFN_vkDebugUtilsMessengerCallbackEXT pfnUserCallack);

	VkImageMemoryBarrier2 create_image_barrier_info(VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask,
		VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout, VkImage image, uint32_t mipLevels);

	VkImageCreateInfo create_image_info(VkFormat format, VkExtent2D extent, VkImageUsageFlags usage, uint32_t mipLevels);
	VkImageViewCreateInfo create_image_view_info(VkImage image, VkFormat format, VkImageAspectFlags aspectMask, uint32_t mipLevels);

	VkBufferCreateInfo create_buffer_info(VkDeviceSize size, VkBufferUsageFlags usage);

	VkDescriptorBufferInfo create_descriptor_buffer_info(VkBuffer buffer, VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE);

	VkRenderingAttachmentInfo create_rendering_attachment_info(VkImageView imageView, VkImageLayout imageLayout, const VkClearValue* clearValue);

}
#endif // !INITIALIZERS_H
