#include <iostream>

#include "Kleicha.h"
#include "Utils.h"
#include "InstanceBuilder.h"
#include "DeviceBuilder.h"
#include "SwapchainBuilder.h"
#include "PipelineBuilder.h"
#include "Initializers.h"
#include "Types.h"

#pragma warning(push, 0)
#pragma warning(disable : 26819 26110 6387 26495 6386 26813 33010 28182 26495)
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#pragma warning(pop)

static void key_callback(GLFWwindow* window, int key, [[maybe_unused]]int scancode, int action, [[maybe_unused]]int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

// init calls the required functions to initialize vulkan
void Kleicha::init() {
	if (!glfwInit()) {
		throw std::runtime_error{ "glfw failed to initialize." };
	}
	// disable context creation (used for opengl)
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	// set glfw key callback function
	m_window = glfwCreateWindow(static_cast<int>(m_windowExtent.width), static_cast<int>(m_windowExtent.height), "kleicha", NULL, NULL);
	glfwSetKeyCallback(m_window, key_callback);

	init_vulkan();
	init_swapchain();
	init_command_buffers();
	init_sync_primitives();
	init_graphics_pipelines();
	init_vma();
	init_images();
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

	for (auto& frame : m_frames) {
		VK_CHECK(vkCreateFence(m_device.device, &fenceInfo, nullptr, &frame.inFlightFence));
		VK_CHECK(vkCreateSemaphore(m_device.device, &semaphoreInfo, nullptr, &frame.acquiredSemaphore));
	}

	//allocate present semaphores for each swap chain image
	m_renderedSemaphores.resize(m_swapchain.imageCount);
	for (auto& renderedSemaphore : m_renderedSemaphores) {
		VK_CHECK(vkCreateSemaphore(m_device.device, &semaphoreInfo, nullptr, &renderedSemaphore));
	}
}

void Kleicha::init_graphics_pipelines() {

	// create dummy shader modules to test pipeline builder.

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	vkCreatePipelineLayout(m_device.device, &pipelineLayoutInfo, nullptr, &m_dummyPipelineLayout);

	VkShaderModule vertModule{ utils::create_shader_module(m_device.device, "../shaders/vert_colored_mesh.spv") };
	VkShaderModule fragModule{ utils::create_shader_module(m_device.device, "../shaders/frag_colored_mesh.spv") };
	PipelineBuilder pipelineBuilder{ m_device.device };
	pipelineBuilder.pipelineLayout = m_dummyPipelineLayout;
	pipelineBuilder.set_shaders(vertModule, fragModule);								//ccw winding
	pipelineBuilder.set_rasterizer_state(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	pipelineBuilder.set_depth_stencil_state(VK_FALSE);
	pipelineBuilder.set_color_attachment_format(VK_FORMAT_R8G8B8A8_UNORM);
	m_graphicsPipeline = pipelineBuilder.build();

	// we're free to destroy shader modules after pipeline creation
	vkDestroyShaderModule(m_device.device, vertModule, nullptr);
	vkDestroyShaderModule(m_device.device, fragModule, nullptr);
}

void Kleicha::init_vma() {
	VmaAllocatorCreateInfo allocatorInfo{};
	allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_4;
	allocatorInfo.physicalDevice = m_device.physicalDevice.device;
	allocatorInfo.device = m_device.device;
	allocatorInfo.instance = m_instance.instance;

	VK_CHECK(vmaCreateAllocator(&allocatorInfo, &m_allocator));
}

void Kleicha::init_images() {

	// TO-DO: Abstract image create info creation into its own function

	VkImageCreateInfo imageInfo{ .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageInfo.pNext = nullptr;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	imageInfo.extent = VkExtent3D{ m_windowExtent.width, m_windowExtent.height, 1U };
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.queueFamilyIndexCount = 1;
	imageInfo.pQueueFamilyIndices = &m_device.physicalDevice.queueFamilyIndex;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VmaAllocationCreateInfo allocationInfo{};
	allocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	allocationInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VK_CHECK(vmaCreateImage(m_allocator, &imageInfo, &allocationInfo, &m_rasterImage.image, &m_rasterImage.allocation, &m_rasterImage.allocationInfo));
}

void Kleicha::recreate_swapchain() {

	// handle case where window is minimized
	int width{}, height{};
	glfwGetFramebufferSize(m_window, &width, &height);

	// keep polling 
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(m_window, &width, &height);
		glfwPollEvents();
	}

	// host waits for all work in queue to conclude
	VK_CHECK(vkDeviceWaitIdle(m_device.device));

	// cleanup swapchain and raster images
	for (const auto& view : m_swapchain.imageViews)
		vkDestroyImageView(m_device.device, view, nullptr);

	vkDestroySwapchainKHR(m_device.device, m_swapchain.swapchain, nullptr);
	vmaDestroyImage(m_allocator, m_rasterImage.image, m_rasterImage.allocation);

	// get updated surface support details
	DeviceBuilder builder{ m_instance.instance, m_surface };
	std::optional<vkt::SurfaceSupportDetails> supportDetails{ builder.get_surface_support_details(m_device.physicalDevice.device).value() };
	if (!supportDetails.has_value())
		throw std::runtime_error{ "[Kleicha] Failed to recreate swapchain - surface support details weren't retrieved from the physical device." };

	set_window_extent(supportDetails.value().capabilities.currentExtent);
	m_device.physicalDevice.surfaceSupportDetails = supportDetails.value();
	init_swapchain();
	init_images();
}

void Kleicha::start() {

	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();
		draw();
	}
	// wait for all driver access to conclude before cleanup
	VK_CHECK(vkDeviceWaitIdle(m_device.device));
}

void Kleicha::draw() {
	// get references to current frame
	vkt::Frame frame{ get_current_frame() };
	VK_CHECK(vkWaitForFences(m_device.device, 1, &frame.inFlightFence, VK_TRUE, std::numeric_limits<uint64_t>::max()));
	uint32_t imageIndex{};
	// acquire image from swapchain
	VkResult acquireResult{ vkAcquireNextImageKHR(m_device.device, m_swapchain.swapchain, std::numeric_limits<uint64_t>::max(), frame.acquiredSemaphore, VK_NULL_HANDLE, &imageIndex) };

	if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
		recreate_swapchain();

	// we should only set fence to unsignaled when we know the command buffer will be submitted to the queue.
	VK_CHECK(vkResetFences(m_device.device, 1, &frame.inFlightFence));

	VkCommandBufferBeginInfo cmdBufferBeginInfo{ 
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	// implicitly resets command buffer and places it in recording state
	VK_CHECK(vkBeginCommandBuffer(frame.cmdBuffer, &cmdBufferBeginInfo));

	// forms a dependency chain with vkAcquireNextImageKHR signal semaphore
	VkImageMemoryBarrier2 toTransferDst{init::create_image_barrier_info(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, 
		VK_PIPELINE_STAGE_2_TRANSFER_BIT,VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_rasterImage.image)};

	VkImageMemoryBarrier2 presentToTransferDst{ init::create_image_barrier_info(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
		VK_PIPELINE_STAGE_2_TRANSFER_BIT,VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_swapchain.images[imageIndex])};

	VkImageMemoryBarrier2 imageBarriers[]{ toTransferDst, presentToTransferDst };

	// transition swapchain image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	VkDependencyInfo dependencyInfo{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
	dependencyInfo.pNext = nullptr;
	dependencyInfo.imageMemoryBarrierCount = 2;
	dependencyInfo.pImageMemoryBarriers = imageBarriers;

	// image memory barrier
	vkCmdPipelineBarrier2(frame.cmdBuffer, &dependencyInfo);

	VkImageSubresourceRange subresourceRange{};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseArrayLayer = 0;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.layerCount = 1;
	subresourceRange.levelCount = 1;


	VkClearColorValue clearColor{ { std::sinf(static_cast<float>(m_framesRendered)/1000.0f), 0.0f, 0.0f, 1.0f}};

	vkCmdClearColorImage(frame.cmdBuffer, m_rasterImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &subresourceRange);

	// transition image to transfer src
	VkImageMemoryBarrier2 toTransferSrc{ init::create_image_barrier_info(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, 
		VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_rasterImage.image)};

	// transition swapchain image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	VkDependencyInfo dependencyInfo2{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
	dependencyInfo2.pNext = nullptr;
	dependencyInfo2.imageMemoryBarrierCount = 1;
	dependencyInfo2.pImageMemoryBarriers = &toTransferSrc;
	// image memory barrier
	vkCmdPipelineBarrier2(frame.cmdBuffer, &dependencyInfo2);

	VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2 };
	blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.srcSubresource.baseArrayLayer = 0;
	blitRegion.srcSubresource.layerCount = 1;
	blitRegion.srcSubresource.mipLevel = 0;

	blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.dstSubresource.baseArrayLayer = 0;
	blitRegion.dstSubresource.layerCount = 1;
	blitRegion.dstSubresource.mipLevel = 0;

	blitRegion.srcOffsets[1].x = static_cast<int32_t>(m_windowExtent.width);
	blitRegion.srcOffsets[1].y = static_cast<int32_t>(m_windowExtent.height);
	blitRegion.srcOffsets[1].z = 1;

	blitRegion.dstOffsets[1].x = static_cast<int32_t>(m_windowExtent.width);
	blitRegion.dstOffsets[1].y = static_cast<int32_t>(m_windowExtent.height);
	blitRegion.dstOffsets[1].z = 1;

	VkBlitImageInfo2 blitImageInfo{ .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2 };
	blitImageInfo.srcImage = m_rasterImage.image;
	blitImageInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	blitImageInfo.dstImage = m_swapchain.images[imageIndex];
	blitImageInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	blitImageInfo.regionCount = 1;
	blitImageInfo.pRegions = &blitRegion;

	vkCmdBlitImage2(frame.cmdBuffer, &blitImageInfo);

	VkImageMemoryBarrier2 transferDstToPresent{ init::create_image_barrier_info(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
		VK_PIPELINE_STAGE_2_TRANSFER_BIT,VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, m_swapchain.images[imageIndex]) };

	// transition swapchain image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	VkDependencyInfo dependencyInfoPresent{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
	dependencyInfoPresent.pNext = nullptr;
	dependencyInfoPresent.imageMemoryBarrierCount = 1;
	dependencyInfoPresent.pImageMemoryBarriers = &transferDstToPresent;
	
	vkCmdPipelineBarrier2(frame.cmdBuffer, &dependencyInfoPresent);


	{
	/* boilerplate for when we use a graphics pipeline
	VkRenderingAttachmentInfo colorAttachment{.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
	colorAttachment.pNext = nullptr;
	colorAttachment.imageView = m_swapchain.imageViews[imageIndex];
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_GENERAL;;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	VkRenderingInfo renderingInfo{ .sType = VK_STRUCTURE_TYPE_RENDERING_INFO };
	renderingInfo.pNext = nullptr;
	renderingInfo.renderArea.extent = m_swapchain.imageExtent;
	renderingInfo.renderArea.offset = { 0,0 };
	renderingInfo.layerCount = 1;
	renderingInfo.viewMask = 0; //we're not using multiview
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachment;
	// begin a render pass
	vkCmdBeginRendering(frame.cmdBuffer, &renderingInfo);

	vkCmdEndRendering(frame.cmdBuffer);*/
	}

	VK_CHECK(vkEndCommandBuffer(frame.cmdBuffer));
	
	VkSemaphoreSubmitInfo acquiredSemSubmitInfo{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO };
	acquiredSemSubmitInfo.pNext = nullptr;
	acquiredSemSubmitInfo.semaphore = frame.acquiredSemaphore;
	// forms a dependency chain with the image memory barrier at the beginning of the batch to ensure the image transition happens before vkCmdClearColorImage
	acquiredSemSubmitInfo.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
	acquiredSemSubmitInfo.deviceIndex = 0;

	VkSemaphoreSubmitInfo renderedSemSubmitInfo{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO };
	renderedSemSubmitInfo.pNext = nullptr;
	renderedSemSubmitInfo.semaphore = m_renderedSemaphores[imageIndex];
	renderedSemSubmitInfo.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
	renderedSemSubmitInfo.deviceIndex = 0;

	VkCommandBufferSubmitInfo cmdBufferSubmitInfo{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO };
	cmdBufferSubmitInfo.commandBuffer = frame.cmdBuffer;
	cmdBufferSubmitInfo.deviceMask = 0;

	VkSubmitInfo2 submitInfo{ .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2 };
	submitInfo.pNext = nullptr;
	submitInfo.waitSemaphoreInfoCount = 1;
	submitInfo.pWaitSemaphoreInfos = &acquiredSemSubmitInfo;
	submitInfo.commandBufferInfoCount = 1;
	submitInfo.pCommandBufferInfos = &cmdBufferSubmitInfo;
	submitInfo.signalSemaphoreInfoCount = 1;
	submitInfo.pSignalSemaphoreInfos = &renderedSemSubmitInfo;
	VK_CHECK(vkQueueSubmit2(m_device.queue, 1, &submitInfo, frame.inFlightFence));


	VkPresentInfoKHR presentInfo{ .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.pNext = nullptr;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_renderedSemaphores[imageIndex];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_swapchain.swapchain;
	presentInfo.pImageIndices = &imageIndex;
	VkResult presentResult{ vkQueuePresentKHR(m_device.queue, &presentInfo) };

	if (presentResult == VK_ERROR_OUT_OF_DATE_KHR)
		recreate_swapchain();

	++m_framesRendered;
}

void Kleicha::cleanup() const {

	vmaDestroyImage(m_allocator, m_rasterImage.image, m_rasterImage.allocation);

	vmaDestroyAllocator(m_allocator);

	vkDestroyPipeline(m_device.device, m_graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_device.device, m_dummyPipelineLayout, nullptr);

	for (const auto& renderedSemaphore : m_renderedSemaphores) {
		vkDestroySemaphore(m_device.device, renderedSemaphore, nullptr);
	}

	for (const auto& frame : m_frames) {
		vkDestroyFence(m_device.device, frame.inFlightFence, nullptr);
		vkDestroySemaphore(m_device.device, frame.acquiredSemaphore, nullptr);
	}

	vkDestroyCommandPool(m_device.device, m_commandPool, nullptr);
	for (const auto& view: m_swapchain.imageViews)
		vkDestroyImageView(m_device.device, view, nullptr);

	vkDestroySwapchainKHR(m_device.device, m_swapchain.swapchain, nullptr);
	vkDestroyDevice(m_device.device, nullptr);
	vkDestroySurfaceKHR(m_instance.instance, m_surface, nullptr);
#ifdef _DEBUG
	m_instance.pfnDestroyMessenger(m_instance.instance, m_instance.debugMessenger, nullptr);
#endif
	vkDestroyInstance(m_instance.instance, nullptr);
	glfwDestroyWindow(m_window);
	glfwTerminate();
}