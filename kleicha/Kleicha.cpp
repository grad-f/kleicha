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
	init_intermediate_images();
	init_meshes();
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
		VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME };
	vkt::DeviceFeatures deviceFeatures{};
	deviceFeatures.Vk12Features.timelineSemaphore = true;
	deviceFeatures.Vk12Features.bufferDeviceAddress = true;
	deviceFeatures.Vk12Features.descriptorIndexing = true;
	deviceFeatures.Vk12Features.scalarBlockLayout = true;
	deviceFeatures.Vk13Features.dynamicRendering = true;
	deviceFeatures.Vk13Features.synchronization2 = true;
	deviceFeatures.Vk13Features.pipelineCreationCacheControl = true;
	DeviceBuilder device{m_instance.instance, m_surface};
	m_device = device.request_extensions(deviceExtensions).request_features(deviceFeatures).build();
}

void Kleicha::init_swapchain() {
	// create swapchain
	SwapchainBuilder swapchainBuilder{ m_instance.instance, m_window, m_surface, m_device };
	VkSurfaceFormatKHR surfaceFormat{ .format = VK_FORMAT_B8G8R8A8_SRGB, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
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

	// create immediate submit fence
	VK_CHECK(vkCreateFence(m_device.device, &fenceInfo, nullptr, &m_immFence));

	//allocate present semaphores for each swap chain image
	m_renderedSemaphores.resize(m_swapchain.imageCount);
	for (auto& renderedSemaphore : m_renderedSemaphores) {
		VK_CHECK(vkCreateSemaphore(m_device.device, &semaphoreInfo, nullptr, &renderedSemaphore));
	}
}

void Kleicha::init_graphics_pipelines() {

	// create dummy shader modules to test pipeline builder.
	VkPushConstantRange pushConstantRange{ .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .offset = 0, .size = sizeof(vkt::PushConstants)};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
	vkCreatePipelineLayout(m_device.device, &pipelineLayoutInfo, nullptr, &m_dummyPipelineLayout);

	/*VkShaderModule vertModule{utils::create_shader_module(m_device.device, "../shaders/vert_basic.spv")};
	VkShaderModule fragModule{ utils::create_shader_module(m_device.device, "../shaders/frag_basic.spv") };*/

	VkShaderModule vertModule{ utils::create_shader_module(m_device.device, "../shaders/vert_cube.spv") };
	VkShaderModule fragModule{ utils::create_shader_module(m_device.device, "../shaders/frag_cube.spv") };

	PipelineBuilder pipelineBuilder{ m_device.device };
	pipelineBuilder.pipelineLayout = m_dummyPipelineLayout;
	pipelineBuilder.set_shaders(vertModule, fragModule);								//ccw winding
	pipelineBuilder.set_rasterizer_state(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	pipelineBuilder.set_depth_stencil_state(VK_FALSE);
	pipelineBuilder.set_color_attachment_format(INTERMEDIATE_IMAGE_FORMAT);
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

void Kleicha::init_intermediate_images() {

	VkImageCreateInfo imageInfo{ init::create_image_info(INTERMEDIATE_IMAGE_FORMAT, m_swapchain.imageExtent,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)};
	VmaAllocationCreateInfo allocationInfo{};
	allocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	allocationInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	// create an intermediate image for each potential frame in flight and a corresponding image view
	for (auto& frame : m_frames) {
		VK_CHECK(vmaCreateImage(m_allocator, &imageInfo, &allocationInfo, &frame.rasterImage.image, &frame.rasterImage.allocation, &frame.rasterImage.allocationInfo));

		VkImageViewCreateInfo imageViewInfo{ init::create_image_view_info(frame.rasterImage.image, INTERMEDIATE_IMAGE_FORMAT, VK_IMAGE_ASPECT_COLOR_BIT) };
		VK_CHECK(vkCreateImageView(m_device.device, &imageViewInfo, nullptr, &frame.rasterImage.imageView));
	}
}

void Kleicha::init_meshes() {
	// for now let's upload a cube mesh to the gpu
	vkt::IndexedMesh cubeMesh{ utils::generate_cube_mesh() };
	m_cubeAllocation = upload_mesh_data(cubeMesh);
}

vkt::GPUMeshAllocation Kleicha::upload_mesh_data(const vkt::IndexedMesh& mesh) {
	// Create mesh vertex and index buffers
	vkt::Buffer GPUvertsAllocation{ utils::create_buffer(m_allocator, mesh.vertsBufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_COPY,
		VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) };

	vkt::Buffer GPUindAllocation{ utils::create_buffer(m_allocator, mesh.tIndBufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) };

	// get buffer device address
	VkBufferDeviceAddressInfo GPUvertexBufferAddr{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = GPUvertsAllocation.buffer };
	VkDeviceAddress vertexBufferDeviceAddress{ vkGetBufferDeviceAddress(m_device.device, &GPUvertexBufferAddr) };

	// Create staging buffers
	vkt::Buffer STGvertexBuffer{ utils::create_buffer(m_allocator, mesh.vertsBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,  VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT) };

	vkt::Buffer STGindexBuffer{ utils::create_buffer(m_allocator, mesh.tIndBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,  VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT) };

	// copy to mapped device visible memory
	memcpy(STGvertexBuffer.allocation->GetMappedData(), mesh.verts.data(), mesh.vertsBufferSize);
	memcpy(STGindexBuffer.allocation->GetMappedData(), mesh.tInd.data(), mesh.tIndBufferSize);

	VkCommandBufferBeginInfo cmdBufferBeginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	vkResetFences(m_device.device, 1, &m_immFence);
	// transition to recording state
	VK_CHECK(vkBeginCommandBuffer(m_immCmdBuffer, &cmdBufferBeginInfo));
	VkBufferCopy bufferCopy{ .srcOffset = 0, .dstOffset = 0, .size = mesh.vertsBufferSize };
	vkCmdCopyBuffer(m_immCmdBuffer, STGvertexBuffer.buffer, GPUvertsAllocation.buffer, 1, &bufferCopy);
	bufferCopy.size = mesh.tIndBufferSize;
	vkCmdCopyBuffer(m_immCmdBuffer, STGindexBuffer.buffer, GPUindAllocation.buffer, 1, &bufferCopy);
	VK_CHECK(vkEndCommandBuffer(m_immCmdBuffer));

	VkCommandBufferSubmitInfo cmdBufferSubmitInfo{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO };
	cmdBufferSubmitInfo.commandBuffer = m_immCmdBuffer;
	cmdBufferSubmitInfo.deviceMask = 0;

	VkSubmitInfo2 submitInfo{ .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2 };
	submitInfo.pNext = nullptr;
	submitInfo.commandBufferInfoCount = 1;
	submitInfo.pCommandBufferInfos = &cmdBufferSubmitInfo;
	vkQueueSubmit2(m_device.queue, 1, &submitInfo, m_immFence);

	{/*
		glm::ivec3* pFloats{ reinterpret_cast<glm::ivec3*>(stagingBuffer.indexAllocation->GetMappedData()) };

		for (std::size_t i{ 0 }; i < cube.tInd.size(); ++i) {
			fmt::println("{0} | {1} | {2}", pFloats[i].x, pFloats[i].y, pFloats[i].z);
		}

	*/
	}

	vkWaitForFences(m_device.device, 1, &m_immFence, VK_TRUE, std::numeric_limits<uint64_t>::max());
	vmaDestroyBuffer(m_allocator, STGvertexBuffer.buffer, STGvertexBuffer.allocation);
	vmaDestroyBuffer(m_allocator, STGindexBuffer.buffer, STGindexBuffer.allocation);

	return { .vertsAllocation = GPUvertsAllocation, .indAllocation = GPUindAllocation, .indexCount = mesh.indexCount, .vertsBufferAddress = vertexBufferDeviceAddress };
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

	for (const auto& frame : m_frames) {
		vmaDestroyImage(m_allocator, frame.rasterImage.image, frame.rasterImage.allocation);
		vkDestroyImageView(m_device.device, frame.rasterImage.imageView, nullptr);
	}

	// get updated surface support details
	DeviceBuilder builder{ m_instance.instance, m_surface };
	std::optional<vkt::SurfaceSupportDetails> supportDetails{ builder.get_surface_support_details(m_device.physicalDevice.device).value() };
	if (!supportDetails.has_value())
		throw std::runtime_error{ "[Kleicha] Failed to recreate swapchain - surface support details weren't retrieved from the physical device." };

	set_window_extent(supportDetails.value().capabilities.currentExtent);
	m_device.physicalDevice.surfaceSupportDetails = supportDetails.value();
	init_swapchain();
	init_intermediate_images();
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

	// forms a dependency chain with vkAcquireNextImageKHR signal semaphore. when semaphores are signaled, all pending writes are made available. i dont need to do this manually here
	VkImageMemoryBarrier2 rastertoTransferDst{init::create_image_barrier_info(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_NONE, 
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, 
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, frame.rasterImage.image)};

	VkImageMemoryBarrier2 scToTransferDst{ init::create_image_barrier_info(VK_PIPELINE_STAGE_2_TRANSFER_BIT, 0,
		VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_swapchain.images[imageIndex])};

	// batch this to avoid unnecessary driver overhead
	VkImageMemoryBarrier2 imageBarriers[]{ rastertoTransferDst, scToTransferDst };

	// transition swapchain image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	VkDependencyInfo dependencyInfo{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
	dependencyInfo.pNext = nullptr;
	dependencyInfo.imageMemoryBarrierCount = 2;
	dependencyInfo.pImageMemoryBarriers = imageBarriers;

	// image memory barrier
	vkCmdPipelineBarrier2(frame.cmdBuffer, &dependencyInfo);

	VkRenderingAttachmentInfo colorAttachment{.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
	colorAttachment.pNext = nullptr;
	colorAttachment.imageView = frame.rasterImage.imageView;
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.clearValue.color = { {0.2f, 0.5f, 0.7f} };

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

	vkCmdBindPipeline(frame.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

	// will be used to compute the viewport transformation (NDC to screen space)
	VkViewport viewport{};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = static_cast<float>(m_swapchain.imageExtent.width);
	viewport.height = static_cast<float>(m_swapchain.imageExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent = m_swapchain.imageExtent;

	vkCmdSetViewport(frame.cmdBuffer, 0, 1, &viewport);
	vkCmdSetScissor(frame.cmdBuffer, 0, 1, &scissor);

	// push constants

	vkt::PushConstants pushConstants{ .vertexBufferAddress = m_cubeAllocation.vertsBufferAddress};
	pushConstants.matix = utils::orthographicProj(glm::radians(60.0f), static_cast<float>(m_windowExtent.width) / m_windowExtent.height, 100.0f, 0.1f) *
		utils::perspective(100.0f, 0.1f) * utils::lookAt(glm::vec3{ 1.0f, 1.0f, 6.0f }, glm::vec3{ 0.0f, 0.0f, -1.0f }, glm::vec3{ 0.0f, 1.0f, 0.0f }) * glm::translate(glm::mat4{ 1.0f }, glm::vec3{ 0.0f, 0.0f, -3.0f }) *
		glm::rotate(glm::mat4{ 1.0f }, glm::radians(m_framesRendered / 100.0f), glm::vec3{ 0.5f, 1.0f, 0.3f }) /** glm::scale(glm::mat4{ 1.0f }, glm::vec3{ 0.02f,0.02f,0.02f })*/;
	
	glm::lookAt(glm::vec3{ 0.0f, 0.0f, 2.0f }, glm::vec3{ 0.0f, 0.0f, -1.0f }, glm::vec3{ 0.0f, 1.0f, 0.0f });

	vkCmdPushConstants(frame.cmdBuffer, m_dummyPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(vkt::PushConstants), &pushConstants);

	vkCmdBindIndexBuffer(frame.cmdBuffer, m_cubeAllocation.indAllocation.buffer, 0, VK_INDEX_TYPE_UINT32);

	// invoke the vertex shader 3 times.
	vkCmdDrawIndexed(frame.cmdBuffer, m_cubeAllocation.indexCount, 1, 0, 0, 0);

	vkCmdEndRendering(frame.cmdBuffer);

	// transition image to transfer src
	utils::image_memory_barrier(frame.cmdBuffer, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, 
		VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT, 
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, frame.rasterImage.image);

	// blit from intermediate raster image to swapchain image
	utils::blit_image(frame.cmdBuffer, frame.rasterImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_swapchain.images[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_swapchain.imageExtent, m_swapchain.imageExtent);

	utils::image_memory_barrier(frame.cmdBuffer, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
		VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, m_swapchain.images[imageIndex]);

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
	renderedSemSubmitInfo.stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
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

	// model cleanup
	vmaDestroyBuffer(m_allocator, m_cubeAllocation.vertsAllocation.buffer, m_cubeAllocation.vertsAllocation.allocation);
	vmaDestroyBuffer(m_allocator, m_cubeAllocation.indAllocation.buffer, m_cubeAllocation.indAllocation.allocation);

	vkDestroyFence(m_device.device, m_immFence, nullptr);

	for (const auto& frame : m_frames) {
		vkDestroyFence(m_device.device, frame.inFlightFence, nullptr);
		vkDestroySemaphore(m_device.device, frame.acquiredSemaphore, nullptr);
		vmaDestroyImage(m_allocator, frame.rasterImage.image, frame.rasterImage.allocation);
		vkDestroyImageView(m_device.device, frame.rasterImage.imageView, nullptr);
	}

	vmaDestroyAllocator(m_allocator);

	vkDestroyPipeline(m_device.device, m_graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_device.device, m_dummyPipelineLayout, nullptr);

	for (const auto& renderedSemaphore : m_renderedSemaphores) {
		vkDestroySemaphore(m_device.device, renderedSemaphore, nullptr);
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