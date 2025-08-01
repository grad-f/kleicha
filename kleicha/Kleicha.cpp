#include "Kleicha.h"
#include "Utils.h"
#include "InstanceBuilder.h"
#include "DeviceBuilder.h"
#include "SwapchainBuilder.h"
#include "PipelineBuilder.h"
#include "Initializers.h"
#include "Types.h"

#pragma warning(push, 0)
#pragma warning(disable : 26819 26110 6387 26495 6386 26813 33010 28182 26495 6262 4365)
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_vulkan.h"

#pragma warning(pop)


static void key_callback(GLFWwindow* window, int key, [[maybe_unused]]int scancode, int action, [[maybe_unused]]int mods) {

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);

	if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {

		if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		else
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	}

}

static void cursor_callback(GLFWwindow* window, double xpos, double ypos) {

	if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
		// xpos and ypos are effectively measures of displacement
		Kleicha* kleicha{ reinterpret_cast<Kleicha*>(glfwGetWindowUserPointer(window)) };
		kleicha->m_camera.updateEulerAngles(static_cast<float>(xpos), static_cast<float>(ypos));
	}
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
	glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetWindowUserPointer(m_window, this);
	glfwSetCursorPos(m_window, m_windowExtent.width / 2.0f, m_windowExtent.height / 2.0f);
	glfwSetCursorPosCallback(m_window, cursor_callback);
	glfwSetKeyCallback(m_window, key_callback);

	init_vulkan();
	init_swapchain();
	init_command_buffers();
	init_sync_primitives();
	init_descriptors();
	init_graphics_pipelines();
	init_vma();
	init_imgui();
	init_draw_data();
	init_image_buffers();
	init_materials();
	init_lights();
	init_dynamic_buffers();
	init_write_descriptor_sets();
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
	std::vector<const char*> deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	vkt::DeviceFeatures deviceFeatures{};
	deviceFeatures.VkFeatures.features.samplerAnisotropy = true;
	deviceFeatures.Vk12Features.runtimeDescriptorArray = true;
	deviceFeatures.Vk12Features.timelineSemaphore = true;
	deviceFeatures.Vk12Features.bufferDeviceAddress = true;
	deviceFeatures.Vk12Features.descriptorIndexing = true;
	deviceFeatures.Vk12Features.descriptorBindingPartiallyBound = true;
	deviceFeatures.Vk12Features.descriptorBindingVariableDescriptorCount = true;
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
	VkPushConstantRange pushConstantRange{ .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, .offset = 0, .size = sizeof(vkt::PushConstants)};

	VkDescriptorSetLayout setLayouts[]{ m_globDescSetLayout, m_frameDescSetLayout };

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
	pipelineLayoutInfo.setLayoutCount = std::size(setLayouts);
	pipelineLayoutInfo.pSetLayouts = setLayouts;
	vkCreatePipelineLayout(m_device.device, &pipelineLayoutInfo, nullptr, &m_dummyPipelineLayout);

	/*VkShaderModule vertModule{utils::create_shader_module(m_device.device, "../shaders/vert_basic.spv")};
	VkShaderModule fragModule{ utils::create_shader_module(m_device.device, "../shaders/frag_basic.spv") };
	VkShaderModule vertModule{ utils::create_shader_module(m_device.device, "../shaders/vert_cubeInstanced.spv") };
	VkShaderModule vertModule{ utils::create_shader_module(m_device.device, "../shaders/vert_pyrTextured.spv") };
	VkShaderModule fragModule{ utils::create_shader_module(m_device.device, "../shaders/frag_pyrTextured.spv") };*/
	VkShaderModule vertModule{ utils::create_shader_module(m_device.device, "../shaders/vert_blinnPhong.spv") };
	VkShaderModule fragModule{ utils::create_shader_module(m_device.device, "../shaders/frag_blinnPhong.spv") };

	PipelineBuilder pipelineBuilder{ m_device.device };
	pipelineBuilder.pipelineLayout = m_dummyPipelineLayout;
	pipelineBuilder.set_shaders(vertModule, fragModule);								//ccw winding
	pipelineBuilder.set_rasterizer_state(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	pipelineBuilder.set_depth_stencil_state(VK_TRUE);
	pipelineBuilder.set_depth_attachment_format(DEPTH_IMAGE_FORMAT);
	pipelineBuilder.set_color_attachment_format(INTERMEDIATE_IMAGE_FORMAT);
	m_graphicsPipeline = pipelineBuilder.build();

	// we're free to destroy shader modules after pipeline creation
	vkDestroyShaderModule(m_device.device, vertModule, nullptr);
	vkDestroyShaderModule(m_device.device, fragModule, nullptr);
}

void Kleicha::init_descriptors() {

	{			// create global descriptor set layout	
		VkDescriptorBindingFlags bindingFlags[4]{
			{},
			{},
			{},
			{VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT }
		};
		VkDescriptorSetLayoutBindingFlagsCreateInfo layoutBindingFlagsInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
		layoutBindingFlagsInfo.bindingCount = std::size(bindingFlags);
		layoutBindingFlagsInfo.pBindingFlags = bindingFlags;

		VkDescriptorSetLayoutBinding bindings[4]{
			{0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 50, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
		};

		// create descriptor set layout
		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		descriptorSetLayoutInfo.pNext = &layoutBindingFlagsInfo;
		descriptorSetLayoutInfo.bindingCount = std::size(bindings);
		descriptorSetLayoutInfo.pBindings = bindings;
		VK_CHECK(vkCreateDescriptorSetLayout(m_device.device, &descriptorSetLayoutInfo, nullptr, &m_globDescSetLayout));
	}

	{		// create per frame descriptor set layout
		VkDescriptorSetLayoutBinding bindings[3]{
			{0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
		};

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		descriptorSetLayoutInfo.pNext = nullptr;
		descriptorSetLayoutInfo.bindingCount = std::size(bindings);
		descriptorSetLayoutInfo.pBindings = bindings;
		VK_CHECK(vkCreateDescriptorSetLayout(m_device.device, &descriptorSetLayoutInfo, nullptr, &m_frameDescSetLayout));
	}

	//create descriptor set pool
	VkDescriptorPoolSize poolDescriptorSizes[2]{
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5},
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 50}	// Textures
	};

	VkDescriptorPoolCreateInfo descriptorPoolInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	descriptorPoolInfo.pNext = nullptr;
	descriptorPoolInfo.maxSets = 10;
	descriptorPoolInfo.poolSizeCount = std::size(poolDescriptorSizes);
	descriptorPoolInfo.pPoolSizes = poolDescriptorSizes;

	VK_CHECK(vkCreateDescriptorPool(m_device.device, &descriptorPoolInfo, nullptr, &m_descPool));

	{
		uint32_t variableSizedDescriptorSize{ 50 };
		// we must specify the number of descriptors in our variable-sized descriptor binding in the descriptor set we are allocating
		VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescriptorCountAllocInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO };
		variableDescriptorCountAllocInfo.descriptorSetCount = 1;
		variableDescriptorCountAllocInfo.pDescriptorCounts = &variableSizedDescriptorSize;

		VkDescriptorSetAllocateInfo setAllocInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		setAllocInfo.pNext = &variableDescriptorCountAllocInfo;
		setAllocInfo.descriptorPool = m_descPool;
		setAllocInfo.descriptorSetCount = 1;
		setAllocInfo.pSetLayouts = &m_globDescSetLayout;
		VK_CHECK(vkAllocateDescriptorSets(m_device.device, &setAllocInfo, &m_globalDescSet));
	}

	{
		VkDescriptorSetAllocateInfo setAllocInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		setAllocInfo.pNext = nullptr;
		setAllocInfo.descriptorPool = m_descPool;
		setAllocInfo.descriptorSetCount = 1;
		setAllocInfo.pSetLayouts = &m_frameDescSetLayout;

		for (auto& frame : m_frames)
			VK_CHECK(vkAllocateDescriptorSets(m_device.device, &setAllocInfo, &frame.descriptorSet));
	}
	
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

void Kleicha::init_imgui() {

	// create imgui descriptor pool
	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE },
	};
	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 0;
	for (VkDescriptorPoolSize& pool_size : pool_sizes)
		pool_info.maxSets += pool_size.descriptorCount;
	pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	VK_CHECK(vkCreateDescriptorPool(m_device.device, &pool_info, nullptr, &m_imguiDescPool));

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io{ ImGui::GetIO() };
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	
	ImGui_ImplGlfw_InitForVulkan(m_window, true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = m_instance.instance;
	init_info.PhysicalDevice = m_device.physicalDevice.device;
	init_info.Device = m_device.device;
	init_info.QueueFamily = m_device.physicalDevice.queueFamilyIndex;
	init_info.Queue = m_device.queue;
	init_info.DescriptorPool = m_imguiDescPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.UseDynamicRendering = true;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	init_info.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_swapchain.imageFormat.format;

	ImGui_ImplVulkan_Init(&init_info);

	ImGui_ImplVulkan_CreateFontsTexture();
}

void Kleicha::init_image_buffers() {

	VkImageCreateInfo rasterImageInfo{ init::create_image_info(INTERMEDIATE_IMAGE_FORMAT, m_swapchain.imageExtent,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 1)};

	VkImageCreateInfo depthImageInfo{ init::create_image_info(DEPTH_IMAGE_FORMAT, m_swapchain.imageExtent,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 1) };

	VmaAllocationCreateInfo allocationInfo{};
	allocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	allocationInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VK_CHECK(vmaCreateImage(m_allocator, &rasterImageInfo, &allocationInfo, &rasterImage.image, &rasterImage.allocation, &rasterImage.allocationInfo));
	VkImageViewCreateInfo rasterViewInfo{ init::create_image_view_info(rasterImage.image, INTERMEDIATE_IMAGE_FORMAT, VK_IMAGE_ASPECT_COLOR_BIT, 1) };
	VK_CHECK(vkCreateImageView(m_device.device, &rasterViewInfo, nullptr, &rasterImage.imageView));

	VK_CHECK(vmaCreateImage(m_allocator, &depthImageInfo, &allocationInfo, &depthImage.image, &depthImage.allocation, &depthImage.allocationInfo));
	VkImageViewCreateInfo depthViewInfo{ init::create_image_view_info(depthImage.image, DEPTH_IMAGE_FORMAT, VK_IMAGE_ASPECT_DEPTH_BIT, 1) };
	VK_CHECK(vkCreateImageView(m_device.device, &depthViewInfo, nullptr, &depthImage.imageView));

	// transition depth image layouts
	immediate_submit([&](VkCommandBuffer cmdBuffer) {
		utils::image_memory_barrier(cmdBuffer, VK_PIPELINE_STAGE_NONE, VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_NONE, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, depthImage.image, depthImage.mipLevels);
		});
}

std::vector<vkt::MeshDrawData> Kleicha::load_mesh_data() {

	std::vector<vkt::Mesh> meshes{};
	meshes.emplace_back(utils::generate_pyramid_mesh());
	meshes.emplace_back(utils::generate_sphere(48));
	meshes.emplace_back(utils::generate_torus(48, 1.2f, 0.45f));
	meshes.emplace_back(utils::load_obj_mesh("../models/shuttle.obj", vkt::MeshType::SHUTTLE));
	meshes.emplace_back(utils::load_obj_mesh("../models/icosphere.obj", vkt::MeshType::ICOSPHERE));
	meshes.emplace_back(utils::load_obj_mesh("../models/dolphin.obj", vkt::MeshType::DOLPHIN));

	std::vector<vkt::MeshDrawData> meshDrawData(meshes.size());
	// create unified vertex and index buffers for upload. meshDrawData will keep track of mesh buffer offsets within the unified buffer.
	std::vector<vkt::Vertex> unifiedVertices{};
	std::vector<glm::uvec3> unifiedTriangles{};

	for (std::size_t i{ 0 }; i < meshes.size(); ++i) {

		// store the current vertex and triangle counts
		std::uint32_t vertexCount{ static_cast<uint32_t>(unifiedVertices.size()) };
		std::uint32_t triangleCount{ static_cast<uint32_t>(unifiedTriangles.size()) };

		// store vertex and index buffer mesh start positions
		meshDrawData[i].meshType = meshes[i].meshType;
		meshDrawData[i].vertexOffset = vertexCount;
		meshDrawData[i].indicesOffset = triangleCount * glm::uvec3::length();
		meshDrawData[i].indicesCount = static_cast<uint32_t>(meshes[i].tInd.size() * glm::uvec3::length());

		// allocate memory for mesh vertex data and insert it
		unifiedVertices.reserve(vertexCount + meshes[i].verts.size());
		unifiedVertices.insert(unifiedVertices.end(), meshes[i].verts.begin(), meshes[i].verts.end());

		// allocate memory for mesh index data and insert it
		unifiedTriangles.reserve(triangleCount + meshes[i].tInd.size());
		unifiedTriangles.insert(unifiedTriangles.end(), meshes[i].tInd.begin(), meshes[i].tInd.end());
	}

	m_vertexBuffer = upload_data(unifiedVertices.data(), unifiedVertices.size() * sizeof(vkt::Vertex), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	m_indexBuffer = upload_data(unifiedTriangles.data(), unifiedTriangles.size() * sizeof(glm::uvec3), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

	return meshDrawData;
}

vkt::DrawData Kleicha::create_draw(const std::vector<vkt::MeshDrawData>& canonicalMeshes, vkt::MeshType meshType, vkt::MaterialType materialType, vkt::TextureType textureType) {
	for (const auto& mesh : canonicalMeshes) {
		if (mesh.meshType == meshType) {
			m_meshDrawData.push_back(mesh);

			vkt::DrawData drawDatum{ .materialIndex = static_cast<uint32_t>(materialType), .textureIndex = static_cast<uint32_t>(textureType), .transformIndex = static_cast<uint32_t>(m_meshDrawData.size()) - 1 };

			return drawDatum;
		}
	}

	throw std::runtime_error{ "[Kleicha] Requested mesh type was not found in the canonical meshes" };
}

void Kleicha::init_draw_data() {

	// an array where each element is a mesh i have loaded in. it stores the indices/reference data into the unified array on the graphics card
	std::vector<vkt::MeshDrawData> canonicalMeshes{ load_mesh_data() };

	std::vector<vkt::DrawData> drawData{};
	drawData.push_back(create_draw(canonicalMeshes, vkt::MeshType::SPHERE, vkt::MaterialType::SILVER, vkt::TextureType::NONE));
	drawData.push_back(create_draw(canonicalMeshes, vkt::MeshType::DOLPHIN, vkt::MaterialType::GOLD, vkt::TextureType::NONE));
	drawData.push_back(create_draw(canonicalMeshes, vkt::MeshType::SHUTTLE, vkt::MaterialType::NONE, vkt::TextureType::SHUTTLE));

	m_drawBuffer = upload_data(drawData.data(), drawData.size() * sizeof(vkt::DrawData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

	m_meshTransforms.resize(m_meshDrawData.size());

	vkt::GlobalData globalData{ .ambientLight = glm::vec4{0.7f, 0.7f, 0.7f, 1.0f} };
	m_globalsBuffer = upload_data(&globalData, sizeof(globalData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

void Kleicha::init_dynamic_buffers() {
	
	// allocate per frame buffers such as transform buffer
	for (auto& frame : m_frames) {
		frame.transformBuffer = utils::create_buffer(m_allocator, sizeof(vkt::Transform) * m_meshDrawData.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

		frame.lightBuffer = utils::create_buffer(m_allocator, sizeof(vkt::Light) * m_lights.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

		frame.materialBuffer = utils::create_buffer(m_allocator, sizeof(vkt::Material) * m_materials.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
	}
}

void Kleicha::init_lights() {

	// create a standard white light 
	vkt::Light pointLight{
		.ambient = {0.05f, 0.05f, 0.05f, 1.0f},
		.diffuse = {0.6f, 0.6f, 0.6f, 1.0f},
		.specular = {1.0f, 1.0f, 1.0f, 1.0f},
		.mPos = {0.0f, 0.0f, 0.0f}
	};
	m_lights.push_back(pointLight);
}

void Kleicha::init_materials() {
	m_textures.push_back(upload_texture_image("../textures/empty.jpg"));		//0
	m_textures.push_back(upload_texture_image("../textures/brick.png"));		//0
	m_textures.push_back(upload_texture_image("../textures/earth.jpg"));		//1
	m_textures.push_back(upload_texture_image("../textures/concrete.png"));		//2
	m_textures.push_back(upload_texture_image("../textures/shuttle.jpg"));		//3

	m_materials.push_back(vkt::Material::none());
	m_materials.push_back(vkt::Material::gold());
	m_materials.push_back(vkt::Material::jade());
	m_materials.push_back(vkt::Material::pearl());
	m_materials.push_back(vkt::Material::silver());
}

vkt::Buffer Kleicha::upload_data(void* data, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage, VkBool32 bdaUsage) {

	// Create mesh vertex and index buffers
	vkt::Buffer deviceBuffer{ utils::create_buffer(m_allocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | bufferUsage,
		VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) };

	// get buffer device address
	VkBufferDeviceAddressInfo bdaInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = deviceBuffer.buffer };
	if (bdaUsage) {
		deviceBuffer.deviceAddress = vkGetBufferDeviceAddress(m_device.device, &bdaInfo);
	}

	// Create staging buffers
	vkt::Buffer stageBuffer{ utils::create_buffer(m_allocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,  VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT) };

	// copy to mapped device visible memory
	memcpy(stageBuffer.allocation->GetMappedData(), data, bufferSize);

	VkBufferCopy bufferCopy{ .srcOffset = 0, .dstOffset = 0, .size = bufferSize };

	// copy staging buffers to device
	immediate_submit([&](VkCommandBuffer cmdBuffer) {
		vkCmdCopyBuffer(cmdBuffer, stageBuffer.buffer, deviceBuffer.buffer, 1, &bufferCopy);
		});

	 /* {
		glm::vec3* pLocations{ reinterpret_cast<glm::vec3*>(stageBuffer.allocation->GetMappedData()) };

		for (std::size_t i{ 0 }; i < bufferSize/3; ++i) {
			fmt::println("{0} | {1} | {2}", pLocations[i].x, pLocations[i].y, pLocations[i].z);
		}
	}*/
	vmaDestroyBuffer(m_allocator, stageBuffer.buffer, stageBuffer.allocation);

	return deviceBuffer;
}

void Kleicha::init_write_descriptor_sets() {

	utils::update_set_buffer_descriptor(m_device.device, m_globalDescSet, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_vertexBuffer.buffer);
	utils::update_set_buffer_descriptor(m_device.device, m_globalDescSet, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_drawBuffer.buffer);
	utils::update_set_buffer_descriptor(m_device.device, m_globalDescSet, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_globalsBuffer.buffer);

	// per frame descriptor set writes
	for (auto& frame : m_frames) {
		utils::update_set_buffer_descriptor(m_device.device, frame.descriptorSet, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frame.transformBuffer.buffer);
		utils::update_set_buffer_descriptor(m_device.device, frame.descriptorSet, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frame.materialBuffer.buffer);
		utils::update_set_buffer_descriptor(m_device.device, frame.descriptorSet, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frame.lightBuffer.buffer);
	}

	VkSamplerCreateInfo samplerInfo{ .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	samplerInfo.pNext = nullptr;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = m_device.physicalDevice.deviceProperties.properties.limits.maxSamplerAnisotropy;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 16.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	VK_CHECK(vkCreateSampler(m_device.device, &samplerInfo, nullptr, &m_textureSampler));

	std::vector<VkDescriptorImageInfo> imageInfos{};
	VkDescriptorImageInfo imageInfo{};
	imageInfo.sampler = m_textureSampler;
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	for (const auto& texture : m_textures) {
		imageInfo.imageView = texture.imageView;
		imageInfos.emplace_back(imageInfo);
	}
	VkWriteDescriptorSet texSamplerWriteDescSet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	texSamplerWriteDescSet.dstSet = m_globalDescSet;
	texSamplerWriteDescSet.dstBinding = 3;
	texSamplerWriteDescSet.dstArrayElement = 0;
	texSamplerWriteDescSet.descriptorCount = static_cast<uint32_t>(imageInfos.size());
	texSamplerWriteDescSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	texSamplerWriteDescSet.pImageInfo = imageInfos.data();
	vkUpdateDescriptorSets(m_device.device, 1, &texSamplerWriteDescSet, 0, nullptr);
}

vkt::Image Kleicha::upload_texture_image(const char* filePath) {

	stbi_set_flip_vertically_on_load(true);

	int width, height;
	stbi_uc* textureData{ stbi_load(filePath, &width, &height, nullptr, STBI_rgb_alpha) };
	if (!textureData) {
		throw std::runtime_error{ "[Kleicha] Failed to load texture image: " + std::string{ stbi_failure_reason() } };
	}

	VkExtent2D textureExtent{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
	VkDeviceSize bufferSize{ static_cast<VkDeviceSize>(width * height * 4) };

	VmaAllocationCreateInfo allocationInfo{};
	allocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	allocationInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	vkt::Image textureImage{};
	// compute mip levels from longest edge
	textureImage.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
	// allocate device_local memory to store the texture image
	VkImageCreateInfo textureImageInfo{ init::create_image_info(VK_FORMAT_R8G8B8A8_SRGB, textureExtent, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, textureImage.mipLevels) };
	VK_CHECK(vmaCreateImage(m_allocator, &textureImageInfo, &allocationInfo, &textureImage.image, &textureImage.allocation, &textureImage.allocationInfo));
	VkImageViewCreateInfo imageViewInfo{ init::create_image_view_info(textureImage.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, textureImage.mipLevels) };
	VK_CHECK(vkCreateImageView(m_device.device, &imageViewInfo, nullptr, &textureImage.imageView));

	// allocate host visible mapped memory
	vkt::Buffer stagingBuffer{ utils::create_buffer(m_allocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT) };
	memcpy(stagingBuffer.allocationInfo.pMappedData, textureData, bufferSize);

	// safe to deallocate now
	stbi_image_free(textureData);

	// transition texture image and issue copy from staging
	immediate_submit([&](VkCommandBuffer cmdBuffer) {
		utils::image_memory_barrier(cmdBuffer, VK_PIPELINE_STAGE_2_NONE_KHR, VK_ACCESS_2_NONE, 
			VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, textureImage.image, textureImage.mipLevels);

		VkBufferImageCopy imageCopy{};
		imageCopy.bufferOffset = 0;
		imageCopy.bufferRowLength = 0;
		imageCopy.bufferImageHeight = 0;
		imageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopy.imageSubresource.mipLevel = 0;
		imageCopy.imageSubresource.layerCount = 1;
		imageCopy.imageSubresource.baseArrayLayer = 0;
		imageCopy.imageOffset = { .x=0,.y=0,.z=0 };
		imageCopy.imageExtent = { .width = textureExtent.width, .height = textureExtent.height, .depth = 1 };
		vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer.buffer, textureImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);

		// generate mip copies
		VkImageMemoryBarrier2 imageBarrier{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
		imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.image = textureImage.image;
		imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBarrier.subresourceRange.baseArrayLayer = 0;
		imageBarrier.subresourceRange.layerCount = 1;
		imageBarrier.subresourceRange.levelCount = 1;

		VkDependencyInfo dependencyInfo{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
		dependencyInfo.pNext = nullptr;
		dependencyInfo.imageMemoryBarrierCount = 1;
		dependencyInfo.pImageMemoryBarriers = &imageBarrier;

		uint32_t srcMipWidth{ textureExtent.width };
		uint32_t srcMipHeight{ textureExtent.height };

		for (uint32_t i{ 1 }; i < textureImage.mipLevels; ++i) {
			imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			imageBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			imageBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
			imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			imageBarrier.subresourceRange.baseMipLevel = i - 1;

			vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);

			VkExtent2D dstMipExtent{ srcMipWidth, srcMipHeight };
			if (srcMipWidth > 1)
				dstMipExtent.width /= 2;
			if (srcMipHeight > 1)
				dstMipExtent.height /= 2;

			// copy from mip - 1 to mip then transition mip - 1 layout
			utils::blit_image(cmdBuffer, textureImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, textureImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, { srcMipWidth, srcMipHeight }, dstMipExtent, i - 1, i);

			imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			imageBarrier.srcAccessMask = VK_ACCESS_2_NONE_KHR;
			imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_NONE_KHR;
			imageBarrier.dstAccessMask = VK_ACCESS_2_NONE_KHR;
			imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			imageBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);

			if (srcMipWidth > 1)
				srcMipWidth /= 2;
			if (srcMipHeight > 1)
				srcMipHeight /= 2;
		}

		});

	vmaDestroyBuffer(m_allocator, stagingBuffer.buffer, stagingBuffer.allocation);
	return textureImage;
}

void Kleicha::deallocate_frame_images() const {
	vmaDestroyImage(m_allocator, rasterImage.image, rasterImage.allocation);
	vkDestroyImageView(m_device.device, rasterImage.imageView, nullptr);

	vmaDestroyImage(m_allocator, depthImage.image, depthImage.allocation);
	vkDestroyImageView(m_device.device, depthImage.imageView, nullptr);
}

void Kleicha::immediate_submit(std::function<void(VkCommandBuffer cmdBuffer)>&& func) const {
	vkResetFences(m_device.device, 1, &m_immFence);
	vkResetCommandBuffer(m_immCmdBuffer, 0);

	VkCommandBufferBeginInfo cmdBufferBeginInfo{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	cmdBufferBeginInfo.pNext = nullptr;
	cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VK_CHECK(vkBeginCommandBuffer(m_immCmdBuffer, &cmdBufferBeginInfo));

	func(m_immCmdBuffer);

	VK_CHECK(vkEndCommandBuffer(m_immCmdBuffer));

	VkCommandBufferSubmitInfo cmdBufferSubmitInfo{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO };
	cmdBufferSubmitInfo.pNext = nullptr;
	cmdBufferSubmitInfo.commandBuffer = m_immCmdBuffer;
	cmdBufferSubmitInfo.deviceMask = 0;

	VkSubmitInfo2 submitInfo{ .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2 };
	submitInfo.commandBufferInfoCount = 1;
	submitInfo.pCommandBufferInfos = &cmdBufferSubmitInfo;

	VK_CHECK(vkQueueSubmit2(m_device.queue, 1, &submitInfo, m_immFence));

	vkWaitForFences(m_device.device, 1, &m_immFence, VK_TRUE, std::numeric_limits<uint64_t>::max());
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

	deallocate_frame_images();

	// get updated surface support details
	DeviceBuilder builder{ m_instance.instance, m_surface };
	std::optional<vkt::SurfaceSupportDetails> supportDetails{ builder.get_surface_support_details(m_device.physicalDevice.device).value() };
	if (!supportDetails.has_value())
		throw std::runtime_error{ "[Kleicha] Failed to recreate swapchain - surface support details weren't retrieved from the physical device." };

	set_window_extent(supportDetails.value().capabilities.currentExtent);
	m_device.physicalDevice.surfaceSupportDetails = supportDetails.value();
	// recompute perspective proj matrix as the aspect ratio may have changed
	m_perspProj = utils::orthographicProj(glm::radians(60.0f),
		static_cast<float>(m_windowExtent.width) / m_windowExtent.height, 1000.0f, 0.1f) * utils::perspective(1000.0f, 0.1f);
	init_swapchain();
	init_image_buffers();
}

void Kleicha::start() {

	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();
		float currentTime{ static_cast<float>(glfwGetTime()) };
		m_deltaTime = currentTime - m_lastFrame;
		m_lastFrame = currentTime;

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame(); 
		ImGui::NewFrame();

		ImGui::Text("Light");
		ImGui::SliderFloat3("Light World Pos", &m_lights[0].mPos.x, -10.0f, 10.0f);
		ImGui::SliderFloat3("Light Ambient", &m_lights[0].ambient.r, 0.0f, 1.0f);
		ImGui::SliderFloat3("Light Diffuse", &m_lights[0].diffuse.r, 0.0f, 1.0f);
		ImGui::SliderFloat3("Light Specular", &m_lights[0].specular.r, 0.0f, 1.0f);

		ImGui::NewLine();
		ImGui::Text("Materials");
		ImGui::SliderFloat3("Material Ambient", &m_materials[1].ambient.r, 0.0f, 1.0f);
		ImGui::SliderFloat3("Material Diffuse", &m_materials[1].diffuse.r, 0.0f, 1.0f);
		ImGui::SliderFloat3("Material Specular", &m_materials[1].specular.r, 0.0f, 1.0f);
		ImGui::SliderFloat("Shininess", &m_materials[1].shininess, 0.0f, 100.0f);

		ImGui::Render();

		draw(currentTime);
	}
	// wait for all driver access to conclude before cleanup
	VK_CHECK(vkDeviceWaitIdle(m_device.device));
}

void Kleicha::draw([[maybe_unused]] float currentTime) {
	// get references to current frame
	const vkt::Frame frame{ get_current_frame() };
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
	VkImageMemoryBarrier2 rastertoTransferDst{ init::create_image_barrier_info(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_NONE,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, rasterImage.image, rasterImage.mipLevels) };

	VkImageMemoryBarrier2 scToTransferDst{ init::create_image_barrier_info(VK_PIPELINE_STAGE_2_TRANSFER_BIT, 0,
		VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_swapchain.images[imageIndex], 1) };

	// batch this to avoid unnecessary driver overhead
	VkImageMemoryBarrier2 imageBarriers[]{ rastertoTransferDst, scToTransferDst };

	// transition swapchain image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	VkDependencyInfo dependencyInfo{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
	dependencyInfo.pNext = nullptr;
	dependencyInfo.imageMemoryBarrierCount = 2;
	dependencyInfo.pImageMemoryBarriers = imageBarriers;

	// image memory barrier
	vkCmdPipelineBarrier2(frame.cmdBuffer, &dependencyInfo);


	VkClearValue colorDepthClearValue{};
	colorDepthClearValue.color = { {0.0f, 0.0f, 0.0f, 0.0f} };
	colorDepthClearValue.depthStencil = { {0.0f} };
	// specify the attachments to be used during the rendering pass
	VkRenderingAttachmentInfo colorAttachment{ init::create_rendering_attachment_info(rasterImage.imageView, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &colorDepthClearValue)};
	VkRenderingAttachmentInfo depthAttachment{ init::create_rendering_attachment_info(depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, &colorDepthClearValue) };

	VkRenderingInfo renderingInfo{ .sType = VK_STRUCTURE_TYPE_RENDERING_INFO };
	renderingInfo.pNext = nullptr;
	renderingInfo.renderArea.extent = m_swapchain.imageExtent;
	renderingInfo.renderArea.offset = { 0,0 };
	renderingInfo.layerCount = 1;
	renderingInfo.viewMask = 0; //we're not using multiview
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachment;
	renderingInfo.pDepthAttachment = &depthAttachment;

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

	VkDescriptorSet descSets[]{ m_globalDescSet, frame.descriptorSet };
	vkCmdBindDescriptorSets(frame.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_dummyPipelineLayout, 0, std::size(descSets), descSets, 0, nullptr);
	m_pushConstants.perspectiveProjection = m_perspProj;
	glm::mat4 view{ m_camera.getViewMatrix() };

	// TODO: Only update if there was an updated to a buffer
	m_meshTransforms[0].mv = view * glm::translate(glm::mat4{ 1.0f }, glm::vec3{ -3.0f, 0.0f, -3.0f }) * glm::rotate(glm::mat4{ 1.0f }, currentTime, glm::vec3{0.0f, 1.0f, 0.0f}) /** glm::scale(glm::mat4{ 1.0f }, glm::vec3{ 2.0f, 2.0f, 2.0f })*/;
	m_meshTransforms[0].mvInvTr = glm::transpose(glm::inverse(m_meshTransforms[0].mv));

	m_meshTransforms[1].mv = view * glm::translate(glm::mat4{ 1.0f }, glm::vec3{ 0.0f, 0.0f, -3.0f }) * glm::scale(glm::mat4{ 1.0f }, glm::vec3{ 2.0f, 2.0f, 2.0f }) /* glm::rotate(glm::mat4{ 1.0f }, currentTime * 0.2f, glm::vec3{ 1.0f, 0.0f, 0.0f })*/;
	m_meshTransforms[1].mvInvTr = glm::transpose(glm::inverse(m_meshTransforms[1].mv));

	m_meshTransforms[2].mv = view * glm::translate(glm::mat4{ 1.0f }, glm::vec3{ 5.0f, 0.0f, -3.0f }) * glm::rotate(glm::mat4{ 1.0f }, glm::radians(90.0f), glm::vec3{1.0f, 0.0f, 0.0f}) /** glm::scale(glm::mat4{ 1.0f }, glm::vec3{ 2.0f, 2.0f, 2.0f })*/;
	m_meshTransforms[2].mvInvTr = glm::transpose(glm::inverse(m_meshTransforms[2].mv));


	//m_lights[0].mPos.x = 8.0f * cos(currentTime*0.8f);

	// compute light posiiton in camera coordinate frame
	for (auto& light : m_lights)
		light.mvPos = view * glm::vec4{ light.mPos, 1.0f };

	// update per frame buffers
	memcpy(frame.transformBuffer.allocation->GetMappedData(), m_meshTransforms.data(), sizeof(vkt::Transform) * m_meshDrawData.size());
	memcpy(frame.materialBuffer.allocation->GetMappedData(), m_materials.data(), sizeof(vkt::Material) * m_materials.size());
	memcpy(frame.lightBuffer.allocation->GetMappedData(), m_lights.data(), sizeof(vkt::Light) * m_lights.size());

	//TODO: On our graphice device, all host-visible device memory is cache coherent. However, this is not guaranteed on other devices. On devices where this memory
	// does not have the property 'VK_MEMORY_PROPERTY_HOST_COHERENT_BIT', we should make all host writes visible before
	// the below draw calls using a pipeline barrier.

	vkCmdBindIndexBuffer(frame.cmdBuffer, m_indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

	for (std::uint32_t i{ 0 }; i < m_meshDrawData.size(); ++i) {
		m_pushConstants.drawId = i;
		vkCmdPushConstants(frame.cmdBuffer, m_dummyPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(vkt::PushConstants), &m_pushConstants);
		vkCmdDrawIndexed(frame.cmdBuffer, m_meshDrawData[i].indicesCount, 1, m_meshDrawData[i].indicesOffset, static_cast<int32_t>(m_meshDrawData[i].vertexOffset), 0);
	}

	vkCmdEndRendering(frame.cmdBuffer);

	// transition image to transfer src
	utils::image_memory_barrier(frame.cmdBuffer, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, 
		VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, 
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, rasterImage.image, rasterImage.mipLevels);

	// blit from intermediate raster image to swapchain image
	utils::blit_image(frame.cmdBuffer, rasterImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_swapchain.images[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_swapchain.imageExtent, m_swapchain.imageExtent, 0, 0);

	utils::image_memory_barrier(frame.cmdBuffer, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_swapchain.images[imageIndex], 1);

	draw_imgui(frame.cmdBuffer, m_swapchain.imageViews[imageIndex]);

	utils::image_memory_barrier(frame.cmdBuffer, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, m_swapchain.images[imageIndex], 1);

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
	renderedSemSubmitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
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
	process_inputs();
}

void Kleicha::draw_imgui(VkCommandBuffer frameCmdBuffer, VkImageView swapchainImage) const {

	VkRenderingAttachmentInfo colorAttachment{ init::create_rendering_attachment_info(swapchainImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, nullptr) };

	VkRenderingInfo renderingInfo{ .sType = VK_STRUCTURE_TYPE_RENDERING_INFO };
	renderingInfo.pNext = nullptr;
	renderingInfo.renderArea.extent = m_swapchain.imageExtent;
	renderingInfo.renderArea.offset = { 0,0 };
	renderingInfo.layerCount = 1;
	renderingInfo.viewMask = 0; //we're not using multiview
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachment;
	renderingInfo.pDepthAttachment = nullptr;

	// start imgui render pass -- imgui is drawn directly into the swap chain image after the intermediate raster image is copied into it
	vkCmdBeginRendering(frameCmdBuffer, &renderingInfo);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), frameCmdBuffer);

	vkCmdEndRendering(frameCmdBuffer);
}

void Kleicha::process_inputs() {



	if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
		m_camera.moveCameraPosition(FORWARD, m_deltaTime);
	if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
		m_camera.moveCameraPosition(BACKWARD, m_deltaTime);
	if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
		m_camera.moveCameraPosition(RIGHT, m_deltaTime);
	if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
		m_camera.moveCameraPosition(LEFT, m_deltaTime);
}

void Kleicha::cleanup() const {

	vmaDestroyBuffer(m_allocator, m_drawBuffer.buffer, m_drawBuffer.allocation);

	vkDestroySampler(m_device.device, m_textureSampler, nullptr);
	for (const auto& texture : m_textures) {
		vkDestroyImageView(m_device.device, texture.imageView, nullptr);
		vmaDestroyImage(m_allocator, texture.image, texture.allocation);
	}

	vmaDestroyBuffer(m_allocator, m_vertexBuffer.buffer, m_vertexBuffer.allocation);
	vmaDestroyBuffer(m_allocator, m_indexBuffer.buffer, m_indexBuffer.allocation);
	vmaDestroyBuffer(m_allocator, m_globalsBuffer.buffer, m_globalsBuffer.allocation);

	vkDestroyFence(m_device.device, m_immFence, nullptr);

	for (const auto& frame : m_frames) {
		vmaDestroyBuffer(m_allocator, frame.transformBuffer.buffer, frame.transformBuffer.allocation);
		vmaDestroyBuffer(m_allocator, frame.materialBuffer.buffer, frame.materialBuffer.allocation);
		vmaDestroyBuffer(m_allocator, frame.lightBuffer.buffer, frame.lightBuffer.allocation);
		vkDestroyFence(m_device.device, frame.inFlightFence, nullptr);
		vkDestroySemaphore(m_device.device, frame.acquiredSemaphore, nullptr);
	}

	deallocate_frame_images();

	vmaDestroyAllocator(m_allocator);

	vkDestroyDescriptorPool(m_device.device, m_descPool, nullptr);
	ImGui_ImplVulkan_Shutdown();
	vkDestroyDescriptorPool(m_device.device, m_imguiDescPool, nullptr);
	vkDestroyDescriptorSetLayout(m_device.device, m_globDescSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(m_device.device, m_frameDescSetLayout, nullptr);

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