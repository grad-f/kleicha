#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "InstanceBuilder.h"
#include "Initializers.h"
#include "Utils.h"


// debug messenger callback function
#ifdef _DEBUG 
static VkBool32 VKAPI_PTR vkDebugUtilsMessengerCallback(
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
#endif

// checks if the supplied layers are available
void InstanceBuilder::check_layers_support() const {
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
				fmt::println("[InstanceBuilder] Requested layer {0} is supported by this implementation.", requestedLayer);
				break;
			}
		}
		if (!layerFound)
			throw std::runtime_error{ "[InstanceBuilder] Requested layer " + std::string{requestedLayer} + " is not supported by this implementation." };
	}
}

// checks if the supplied instance extensions are available
void InstanceBuilder::check_extensions_support() const {
	uint32_t extensionCount{};
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));
	std::vector<VkExtensionProperties> supportedExtensions(extensionCount);
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, supportedExtensions.data()));

	for (const auto& requestedExt : m_extensions) {
		bool extFound{ false };
		for (const auto& supportedExt : supportedExtensions) {
			if (strcmp(requestedExt, supportedExt.extensionName) == 0) {
				extFound = true;
				fmt::println("[InstanceBuilder] Requested extension {0} is supported by this implementation.", requestedExt);
				break;
			}
		}
		if (!extFound)
			throw std::runtime_error{ "[InstanceBuilder] Requested extension " + std::string{requestedExt} + " is not supported by this implementation." };
	}
}

// adds default extensions to the user specified extensions
void InstanceBuilder::add_glfw_instance_extensions() {
	// add glfw extensions
	uint32_t glfwExtensionCount{};
	const char** glfwExtensions{ glfwGetRequiredInstanceExtensions(&glfwExtensionCount) };

	for (uint32_t i{ 0 }; i < glfwExtensionCount; ++i)
		m_extensions.emplace_back(glfwExtensions[i]);
}

vkt::Instance InstanceBuilder::build() {

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
	instanceInfo.pApplicationInfo = &appInfo;

	// enable validation layer and debug utils if in debug mode
#ifdef _DEBUG
	VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo{ init::create_debug_utils_messenger_info(vkDebugUtilsMessengerCallback) };
	// provide debugging during vkCreateInstance and vkDestroyInstance
	instanceInfo.pNext = &debugMessengerInfo;
	if (m_useValidationLayer)
		m_layers.push_back("VK_LAYER_KHRONOS_validation");
	m_extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

	check_layers_support();
	instanceInfo.enabledLayerCount = static_cast<uint32_t>(m_layers.size());
	instanceInfo.ppEnabledLayerNames = m_layers.data();

	add_glfw_instance_extensions();
	check_extensions_support();
	instanceInfo.enabledExtensionCount = static_cast<uint32_t>(m_extensions.size());
	instanceInfo.ppEnabledExtensionNames = m_extensions.data();

	VkInstance instance{};
	VK_CHECK(vkCreateInstance(&instanceInfo, nullptr, &instance));

	VkDebugUtilsMessengerEXT debugMessenger{};
	PFN_vkCreateDebugUtilsMessengerEXT pfnCreateMessenger{};
	PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroyMessenger{};
#ifdef _DEBUG
	// load debug messenger procedures (since they're part of an extension)
	pfnCreateMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
	pfnDestroyMessenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

	if (pfnCreateMessenger == nullptr || pfnDestroyMessenger == nullptr)
		throw std::runtime_error{ "[InstanceBuilder] Failed to load debug messenger create and destroy procedures." };

	VK_CHECK(pfnCreateMessenger(instance, &debugMessengerInfo, nullptr, &debugMessenger));
#endif

	return vkt::Instance{ .instance = instance, .debugMessenger = debugMessenger, .pfnCreateMessenger = pfnCreateMessenger, .pfnDestroyMessenger = pfnDestroyMessenger };
}