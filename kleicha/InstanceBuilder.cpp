
#include "InstanceBuilder.h"
#include "format.h"
#include "Utils.h"
#include "Initializers.h"
#include "vulkan/vk_enum_string_helper.h"

// debug messenger callback function
VkBool32 VKAPI_PTR vkDebugUtilsMessengerCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	[[maybe_unused]]VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	[[maybe_unused]]void* pUserData) {

	switch (messageSeverity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			fmt::println("[Debug Messenger] WARNING: {0}", pCallbackData->pMessage);
			return VK_FALSE;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			fmt::println("[Debug Messenger] ERROR: {0}", pCallbackData->pMessage);
			return VK_FALSE;
	}
	return VK_FALSE;
}


// checks if the supplied layers are available
void InstanceBuilder::check_layers_supported() const {
	// get layers available by the implementation
	uint32_t layerCount{};
	VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));
	std::vector<VkLayerProperties> supportedLayers(layerCount);
	VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, supportedLayers.data()));

	// traverse each requested layer
	for (const auto& requestedLayer : m_layers) {
		bool layerFound{ false };
		//traverse each supported layer
		for (const auto& supportedLayer : supportedLayers) {
			if (strcmp(requestedLayer, supportedLayer.layerName) == 0) {
				layerFound = true;
				fmt::println("[InstanceBuilder] Requested Layer {0} is supported by this implementation.", requestedLayer);
				break;
			}
		}
		if (!layerFound) {
			throw std::runtime_error{ "[InstanceBuilder] Requested Layer " + std::string{requestedLayer} + " is not supported by this implementation." };
		}
	}
}

VkInstance InstanceBuilder::build_instance() {

	// query instance-level functionality supported by the implementation
	uint32_t apiVersion{};
	vkEnumerateInstanceVersion(&apiVersion);

	VkApplicationInfo appInfo{ .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.pNext = nullptr;
	appInfo.pApplicationName = "Kleicha";
	appInfo.applicationVersion = 0;
	appInfo.pEngineName = nullptr;
	appInfo.engineVersion = 0;
	appInfo.apiVersion = apiVersion;

	VkInstanceCreateInfo instanceInfo{ .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	

	// enable validation layer and debug messenger if in debug mode
#ifdef _DEBUG
	VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo{ init::create_debug_utils_messenger_info(vkDebugUtilsMessengerCallback) };
	instanceInfo.pNext = &debugMessengerInfo;
	if (m_useValidationLayer)
		m_layers.push_back("VK_LAYER_KHRONOS_validation");
#endif // _DEBUG

	instanceInfo.pApplicationInfo = &appInfo;

	check_layers_supported();
	instanceInfo.enabledLayerCount = static_cast<uint32_t>(m_layers.size());
	instanceInfo.ppEnabledLayerNames = m_layers.data();

	instanceInfo.enabledExtensionCount = 0;
	instanceInfo.ppEnabledExtensionNames = nullptr;

	VkInstance instance{};
	VK_CHECK(vkCreateInstance(&instanceInfo, nullptr, &instance));

	return instance;
}

