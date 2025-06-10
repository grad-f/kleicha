#ifndef INITIALIZERS_H
#define INITIALIZERS_H

#include "vulkan/vulkan.h"

/*	 vulkan structure creation helpers	*/

VkDebugUtilsMessengerCreateInfoEXT create_debug_utils_messenger_info() {

	VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerInfo{.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	debugUtilsMessengerInfo.pNext = nullptr;
}


#endif // !INITIALIZERS_H
