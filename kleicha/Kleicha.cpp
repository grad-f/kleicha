#include "Kleicha.h"
#include "Utils.h"
#include "InstanceBuilder.h"
#include "DeviceBuilder.h"
#include "SwapchainBuilder.h"
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
	init_command_buffers();
	init_sync_primitives();
	init_graphics_pipelines();
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
	deviceFeatures.Vk12Features.timelineSemaphore = true;
	deviceFeatures.Vk12Features.bufferDeviceAddress = true;
	deviceFeatures.Vk12Features.descriptorIndexing = true;
	deviceFeatures.Vk13Features.dynamicRendering = true;
	deviceFeatures.Vk13Features.synchronization2 = true;
	deviceFeatures.Vk13Features.pipelineCreationCacheControl = true;
	DeviceBuilder device{m_instance.instance, m_surface};
	m_device = device.request_extensions(deviceExtensions).request_features(deviceFeatures).build();
}

void Kleicha::init_swapchain() {
	// create swapchain
	SwapchainBuilder swapchainBuilder{ m_instance.instance, m_window, m_surface, m_device };
	VkSurfaceFormatKHR surfaceFormat{ .format = VK_FORMAT_R8G8B8A8_UNORM, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	m_swapchain = swapchainBuilder.desired_image_usage(VK_IMAGE_USAGE_TRANSFER_DST_BIT).desired_image_format(surfaceFormat).desired_present_mode(VK_PRESENT_MODE_FIFO_KHR).build();
}

// creates a command pool and command buffers for each frame
void Kleicha::init_command_buffers() {
	VkCommandPoolCreateInfo cmdPoolInfo{ .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	cmdPoolInfo.queueFamilyIndex = m_device.physicalDevice.queueFamilyIndex;
	VK_CHECK(vkCreateCommandPool(m_device.device, &cmdPoolInfo, nullptr, &m_commandPool));
	fmt::println("[Kleicha] Created command pool.");


	VkCommandBufferAllocateInfo cmdBufferInfo{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	cmdBufferInfo.pNext = nullptr;
	cmdBufferInfo.commandPool = m_commandPool;
	cmdBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufferInfo.commandBufferCount = 1;
	// allocate command buffers for each potential frame in flight
	for (auto& frame : m_frames)
		VK_CHECK(vkAllocateCommandBuffers(m_device.device, &cmdBufferInfo, &frame.cmdBuffer));

	// create a command buffer that'll be used for immediate submissions like uploading buffers to the device
	VK_CHECK(vkAllocateCommandBuffers(m_device.device, &cmdBufferInfo, &m_immCmdBuffer));
	fmt::println("[Kleicha] Allocated command buffers.");
}

void Kleicha::init_sync_primitives() {

	// create fence in signaled state as we will wait at the beginning of the render loop
	VkFenceCreateInfo fenceInfo{ .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fenceInfo.pNext = nullptr;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkSemaphoreCreateInfo semaphoreInfo{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	for (auto& frame : m_frames)
	{
		vkCreateFence(m_device.device, &fenceInfo, nullptr, &frame.inFlightFence);
		VK_CHECK(vkCreateSemaphore(m_device.device, &semaphoreInfo, nullptr, &frame.acquiredSemaphore));
		VK_CHECK(vkCreateSemaphore(m_device.device, &semaphoreInfo, nullptr, &frame.renderedSemaphore));
	}

}

void Kleicha::init_graphics_pipelines() {

}

void Kleicha::cleanup() const {
#ifdef _DEBUG
	m_instance.pfnDestroyMessenger(m_instance.instance, m_instance.debugMessenger, nullptr);
#endif

	for (const auto& frame : m_frames) {
		vkDestroyFence(m_device.device, frame.inFlightFence, nullptr);
		vkDestroySemaphore(m_device.device, frame.acquiredSemaphore, nullptr);
		vkDestroySemaphore(m_device.device, frame.renderedSemaphore, nullptr);
	}

	vkDestroyCommandPool(m_device.device, m_commandPool, nullptr);
	for (const auto& view: m_swapchain.imageViews)
		vkDestroyImageView(m_device.device, view, nullptr);

	vkDestroySwapchainKHR(m_device.device, m_swapchain.swapchain, nullptr);
	vkDestroyDevice(m_device.device, nullptr);
	vkDestroySurfaceKHR(m_instance.instance, m_surface, nullptr);
	vkDestroyInstance(m_instance.instance, nullptr);
	glfwDestroyWindow(m_window);
	glfwTerminate();
}