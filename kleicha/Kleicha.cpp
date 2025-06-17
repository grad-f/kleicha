#include "Kleicha.h"
#include "Utils.h"
#include "InstanceBuilder.h"
#include "DeviceBuilder.h"
#include "Types.h"
#include <iostream>


// init calls the required functions to initialize vulkan
void Kleicha::init() {
	if (!glfwInit()) {
		throw std::runtime_error{ "glfw failed to initialize." };
	}
	// disable context creation (used for opengl)
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_window = glfwCreateWindow(static_cast<int>(m_windowExtent.width), static_cast<int>(m_windowExtent.height), "kleicha", NULL, NULL);
	init_vulkan();
	init_swapchain();
}

// core vulkan init
void Kleicha::init_vulkan() {
	/*		create instance		*/	
	InstanceBuilder instanceBuilder{};
	std::vector<const char*> layers{};
	std::vector<const char*> instanceExtensions{};
	m_instance = instanceBuilder.add_layers(layers).add_extensions(instanceExtensions).use_validation_layer().build();

	/*		create surface		*/
	VK_CHECK(glfwCreateWindowSurface(m_instance.instance, m_window, nullptr, &m_surface));

	/*		create logical device		*/		
	std::vector<const char*> deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME, VK_EXT_PIPELINE_CREATION_CACHE_CONTROL_EXTENSION_NAME,
		VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME };
	vkt::DeviceFeatures deviceFeatures{};
	deviceFeatures.Vk12Features.bufferDeviceAddress = true;
	deviceFeatures.Vk12Features.descriptorIndexing = true;
	deviceFeatures.Vk13Features.dynamicRendering = true;
	deviceFeatures.Vk13Features.synchronization2 = true;
	deviceFeatures.Vk13Features.pipelineCreationCacheControl = true;
	DeviceBuilder device{m_instance.instance, m_surface};
	m_device = device.request_extensions(deviceExtensions).request_features(deviceFeatures).build();
}

void Kleicha::init_swapchain() {

}

void Kleicha::cleanup() const {
#ifdef _DEBUG
	m_instance.pfnDestroyMessenger(m_instance.instance, m_instance.debugMessenger, nullptr);
#endif
	vkDestroyDevice(m_device.device, nullptr);
	vkDestroySurfaceKHR(m_instance.instance, m_surface, nullptr);
	vkDestroyInstance(m_instance.instance, nullptr);
	glfwDestroyWindow(m_window);
	glfwTerminate();
}