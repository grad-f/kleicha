#include "Kleicha.h"
#include "InstanceBuilder.h"
#include "DeviceBuilder.h"
#include <iostream>


// init calls the required functions to initialize vulkan
void Kleicha::init() {
	// create vulkan instance
	init_create_instance();

	// pick physical device
	DeviceBuilder device{};
	std::vector<const char*> deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME, VK_EXT_PIPELINE_CREATION_CACHE_CONTROL_EXTENSION_NAME,
		VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, "Poop2"};
	device.add_extensions(deviceExtensions).build(m_instance.instance);

}

void Kleicha::init_glfw() {
	if (!glfwInit()) {
		throw std::runtime_error{ "glfw failed to initialize." };
	}
	// disable context creation (used for opengl)
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_window = glfwCreateWindow(static_cast<int>(m_windowExtent.width), static_cast<int>(m_windowExtent.height), "kleicha", NULL, NULL);

}

// creates a vulkan instance
void Kleicha::init_create_instance() {
	InstanceBuilder instanceBuilder{};
	std::vector<const char*> layers{};
	std::vector<const char*> instanceExtensions{};
	m_instance = instanceBuilder.add_layers(layers).add_extensions(instanceExtensions).use_validation_layer().build();
}

void Kleicha::cleanup() const {
#ifdef _DEBUG
	m_instance.pfnDestroyMessenger(m_instance.instance, m_instance.debugMessenger, nullptr);
#endif
	vkDestroyInstance(m_instance.instance, nullptr);
	glfwDestroyWindow(m_window);
	glfwTerminate();
}