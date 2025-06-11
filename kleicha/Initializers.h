#ifndef INITIALIZERS_H
#define INITIALIZERS_H

#include "vulkan/vulkan.h"

/*	 vulkan structure creation helpers	*/

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
}
#endif // !INITIALIZERS_H
