#include "Initializers.h"

namespace init {
	VkDebugUtilsMessengerCreateInfoEXT create_debug_utils_messenger_info(PFN_vkDebugUtilsMessengerCallbackEXT pfnUserCallack) {

		VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo{ .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
		debugMessengerInfo.pNext = nullptr;
		debugMessengerInfo.flags = 0;
		debugMessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
		debugMessengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugMessengerInfo.pfnUserCallback = pfnUserCallack;
		debugMessengerInfo.pUserData = nullptr;

		return debugMessengerInfo;
	}

	VkImageMemoryBarrier2 create_image_barrier_info(VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask,
		VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout, VkImage image, uint32_t mipLevels) {

		VkImageMemoryBarrier2 imageBarrier{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
		imageBarrier.srcStageMask = srcStageMask;
		imageBarrier.srcAccessMask = srcAccessMask;
		imageBarrier.dstStageMask = dstStageMask;
		imageBarrier.dstAccessMask = dstAccessMask;
		imageBarrier.oldLayout = oldLayout;
		imageBarrier.newLayout = newLayout;
		imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.image = image;
		imageBarrier.subresourceRange.aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		imageBarrier.subresourceRange.baseArrayLayer = 0;
		imageBarrier.subresourceRange.layerCount = 1;
		imageBarrier.subresourceRange.baseMipLevel = 0;
		imageBarrier.subresourceRange.levelCount = mipLevels;

		return imageBarrier;
	}

	VkImageCreateInfo create_image_info(VkFormat format, VkExtent2D extent, VkImageUsageFlags usage, uint32_t mipLevels) {
		VkImageCreateInfo imageInfo{ .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		imageInfo.pNext = nullptr;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = format;
		imageInfo.extent = VkExtent3D{ extent.width, extent.height, 1U };
		imageInfo.mipLevels = mipLevels;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = usage;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		return imageInfo;
	}

	VkBufferCreateInfo create_buffer_info(VkDeviceSize size, VkBufferUsageFlags usage) {
		VkBufferCreateInfo bufferInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.pNext = nullptr;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		return bufferInfo;
	}


	VkImageViewCreateInfo create_image_view_info(VkImage image, VkFormat format, VkImageAspectFlags aspectMask, uint32_t mipLevels) {
		VkImageViewCreateInfo imageViewInfo{ .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		imageViewInfo.pNext = nullptr;
		imageViewInfo.image = image;
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewInfo.format = format;
		// allows us to remap image pixel components (changes how the image viewer interprets the image)
		// VK_COMPONENT_SWIZZLE_IDENTITY effectively means don't remap.
		imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		// specify which sub region of the image we'd like to make an imageview of
		imageViewInfo.subresourceRange.aspectMask = aspectMask;
		imageViewInfo.subresourceRange.baseMipLevel = 0;
		imageViewInfo.subresourceRange.levelCount = mipLevels;
		imageViewInfo.subresourceRange.baseArrayLayer = 0;
		imageViewInfo.subresourceRange.layerCount = 1;

		return imageViewInfo;
	}

	VkDescriptorBufferInfo create_descriptor_buffer_info(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = buffer;
		bufferInfo.offset = offset;
		bufferInfo.range = range;

		return bufferInfo;
	}

	VkRenderingAttachmentInfo create_rendering_attachment_info(VkImageView imageView, VkImageLayout imageLayout, const VkClearValue* clearValue, VkBool32 storeDepth) {
		VkRenderingAttachmentInfo renderingAttachmentInfo{ .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
		renderingAttachmentInfo.pNext = nullptr;
		renderingAttachmentInfo.imageView = imageView;
		renderingAttachmentInfo.imageLayout = imageLayout;
		renderingAttachmentInfo.loadOp = clearValue ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		renderingAttachmentInfo.storeOp = (imageLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) && !storeDepth ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;
		if (clearValue)
			renderingAttachmentInfo.clearValue = *clearValue;

		return renderingAttachmentInfo;
	}

	VkSamplerCreateInfo create_sampler_info(const vkt::Device& device, VkFilter magFilter, VkFilter minFilter, VkSamplerMipmapMode mipmapMode, 
		VkSamplerAddressMode addressMode, VkBool32 anisotropicFiltering, float maxLod, VkCompareOp compareOp) {

		VkSamplerCreateInfo samplerInfo{ .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		samplerInfo.pNext = nullptr;
		samplerInfo.magFilter = magFilter;
		samplerInfo.minFilter = minFilter;
		samplerInfo.mipmapMode = mipmapMode;
		samplerInfo.addressModeU = addressMode;
		samplerInfo.addressModeV = addressMode;
		samplerInfo.addressModeW = addressMode;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.anisotropyEnable = anisotropicFiltering;
		samplerInfo.maxAnisotropy = device.physicalDevice.deviceProperties.properties.limits.maxSamplerAnisotropy;
		samplerInfo.compareEnable = compareOp == VK_COMPARE_OP_NEVER ? VK_FALSE : VK_TRUE;
		samplerInfo.compareOp = compareOp;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = maxLod;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		
		return samplerInfo;
	}


}