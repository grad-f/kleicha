#include "Kleicha.h"
#include "Utils.h"
#include "InstanceBuilder.h"
#include "DeviceBuilder.h"
#include "SwapchainBuilder.h"
#include "PipelineBuilder.h"
#include "Initializers.h"
#include "Types.h"

#pragma warning(push)
#pragma warning(disable : 26819 6262 26110 26813 26495 6386 4100 4365 4127 4189 6387 33010)
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
		throw std::runtime_error{ "[Kleicha] GLFW failed to initialize." };
	}
	// disable context creation (used for opengl)
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	// set glfw key callback function
	//m_window = glfwCreateWindow(static_cast<int>(m_windowExtent.width), static_cast<int>(m_windowExtent.height), "kleicha", glfwGetPrimaryMonitor(), NULL);
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
	init_vma();
	init_imgui();
	init_load_scene();
	init_lights();
	init_image_buffers();
	init_dynamic_buffers();
	init_samplers();
	init_descriptors();
	init_graphics_pipelines();
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
	deviceFeatures.VkFeatures.features.multiDrawIndirect = true;
	deviceFeatures.VkFeatures.features.tessellationShader = true;
	deviceFeatures.Vk11Features.multiview = true;
	deviceFeatures.Vk12Features.runtimeDescriptorArray = true;
	deviceFeatures.Vk12Features.bufferDeviceAddress = true;
	deviceFeatures.Vk12Features.descriptorIndexing = true;
	deviceFeatures.Vk12Features.descriptorBindingPartiallyBound = true;
	deviceFeatures.Vk12Features.descriptorBindingVariableDescriptorCount = true;
	deviceFeatures.Vk13Features.dynamicRendering = true;
	deviceFeatures.Vk13Features.synchronization2 = true;
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
	VkPushConstantRange pushConstantRange{ .stageFlags = VK_SHADER_STAGE_ALL, .offset = 0, .size = sizeof(vkt::PushConstants)};

	VkDescriptorSetLayout setLayouts[]{ m_globDescSetLayout, m_frameDescSetLayout };

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
	pipelineLayoutInfo.setLayoutCount = std::size(setLayouts);
	pipelineLayoutInfo.pSetLayouts = setLayouts;
	vkCreatePipelineLayout(m_device.device, &pipelineLayoutInfo, nullptr, &m_dummyPipelineLayout);

	VkShaderModule lightShadowVertModule{ utils::create_shader_module(m_device.device, "../shaders/vert_blinnPhongShadows.spv") };
	VkShaderModule lightShadowFragModule{ utils::create_shader_module(m_device.device, "../shaders/frag_blinnPhongShadowsPCF.spv") };

	VkShaderModule lightCubeShadowFragModule{ utils::create_shader_module(m_device.device, "../shaders/frag_blinnPhongCubeShadowsPCF.spv") };
	VkShaderModule lightCubeShadowPCSSFragModule{ utils::create_shader_module(m_device.device, "../shaders/frag_blinnPhongCubeShadowsPCSS.spv") };

	VkShaderModule lightVertModule{ utils::create_shader_module(m_device.device, "../shaders/vert_blinnPhong.spv") };
	VkShaderModule lightFragModule{ utils::create_shader_module(m_device.device, "../shaders/frag_blinnPhong.spv") };

	VkShaderModule shadowVertModule{ utils::create_shader_module(m_device.device, "../shaders/vert_shadow.spv") };
	VkShaderModule shadowFragModule{ utils::create_shader_module(m_device.device, "../shaders/frag_shadow.spv") };

	VkShaderModule cubeShadowVertModule{ utils::create_shader_module(m_device.device, "../shaders/vert_omniShadow.spv") };
	VkShaderModule cubeShadowFragModule{ utils::create_shader_module(m_device.device, "../shaders/frag_omniShadow.spv") };

	//VkShaderModule lightDrawsVertModule{utils::create_shader_module(m_device.device, "../shaders/vert_lightDraws.spv")};
	//VkShaderModule lightDrawsFragModule{utils::create_shader_module(m_device.device, "../shaders/frag_lightDraws.spv")};

	VkShaderModule skyboxVertModule{ utils::create_shader_module(m_device.device, "../shaders/vert_skybox.spv") };
	VkShaderModule skyboxFragModule{ utils::create_shader_module(m_device.device, "../shaders/frag_skybox.spv") };

	VkShaderModule environmentMapVertModule{ utils::create_shader_module(m_device.device, "../shaders/vert_environmentMapping.spv") };
	VkShaderModule reflectFragModule{ utils::create_shader_module(m_device.device, "../shaders/frag_environmentMappingReflect.spv") };
	VkShaderModule refractFragModule{ utils::create_shader_module(m_device.device, "../shaders/frag_environmentMappingRefract.spv") };


	VkShaderModule bezierVertModule{ utils::create_shader_module(m_device.device, "../shaders/vert_bezier.spv") };
	VkShaderModule bezierTCSModule{ utils::create_shader_module(m_device.device, "../shaders/tesc_bezier.spv") };
	VkShaderModule bezierTESModule{ utils::create_shader_module(m_device.device, "../shaders/tese_bezier.spv") };
	VkShaderModule bezierFragModule{ utils::create_shader_module(m_device.device, "../shaders/frag_bezier.spv") };

	PipelineBuilder pipelineBuilder{ m_device.device };
	pipelineBuilder.pipelineLayout = m_dummyPipelineLayout;
	pipelineBuilder.set_input_assembly_state(VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
	pipelineBuilder.set_rasterizer_state(VK_POLYGON_MODE_LINE , VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	pipelineBuilder.set_color_blend_state(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, false);
	pipelineBuilder.set_depth_stencil_state(VK_TRUE);
	pipelineBuilder.set_depth_attachment_format(DEPTH_IMAGE_FORMAT);
	pipelineBuilder.set_color_attachment_format(INTERMEDIATE_IMAGE_FORMAT);
	pipelineBuilder.set_shaders(&bezierVertModule, nullptr, &bezierFragModule, nullptr, &bezierTCSModule, &bezierTESModule);
	m_bezierPipeline = pipelineBuilder.build();

	pipelineBuilder.set_input_assembly_state(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.set_shaders(&lightVertModule, nullptr, &lightFragModule);
	m_lightPipeline = pipelineBuilder.build();

	//	2D pcf
	pipelineBuilder.set_shaders(&lightShadowVertModule, nullptr, &lightShadowFragModule);
	m_lightShadowPipeline = pipelineBuilder.build();

	//	PCF
	pipelineBuilder.set_shaders(&lightShadowVertModule, nullptr, &lightCubeShadowFragModule);
	m_lightCubeShadowPipeline = pipelineBuilder.build();

	//	PCSS
	pipelineBuilder.set_shaders(&lightShadowVertModule, nullptr, &lightCubeShadowPCSSFragModule);
	m_lightCubeShadowPCSSPipeline = pipelineBuilder.build();

	/*	the below defined speciailization constant allows the driver compiler to optimize around our alpha testing
	allowing us to retain early - z occlusion culling when rendering meshes that aren't performing alpha testing	*/
	VkSpecializationMapEntry mapEntry{};
	mapEntry.constantID = 0;
	mapEntry.offset = 0;
	mapEntry.size = sizeof(uint32_t);

	// create specialization constant that'll allow the compiler to remove the alpha testing discard rather than introducing a new shader
	uint32_t useAlphaTest{ 1 };
	VkSpecializationInfo specializationInfo{};
	specializationInfo.mapEntryCount = 1;
	specializationInfo.pMapEntries = &mapEntry;
	specializationInfo.dataSize = sizeof(uint32_t);
	specializationInfo.pData = &useAlphaTest;

	pipelineBuilder.set_shaders(&lightVertModule, nullptr, &lightFragModule, &specializationInfo);
	m_lightAlphaPipeline = pipelineBuilder.build();

	pipelineBuilder.set_shaders(&lightShadowVertModule, nullptr, &lightShadowFragModule, &specializationInfo);
	m_lightShadowAlphaPipeline = pipelineBuilder.build();

	pipelineBuilder.set_shaders(&lightShadowVertModule, nullptr, &lightCubeShadowPCSSFragModule, &specializationInfo);
	m_lightCubeShadowPCSSAlphaPipeline = pipelineBuilder.build();

	pipelineBuilder.set_shaders(&lightShadowVertModule, nullptr, &lightCubeShadowFragModule, &specializationInfo);
	m_lightCubeShadowAlphaPipeline = pipelineBuilder.build();

	pipelineBuilder.set_rasterizer_state(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, -1.25f, -1.75f);
	pipelineBuilder.set_shaders(&shadowVertModule, nullptr, &shadowFragModule);
	pipelineBuilder.disable_color_output();
	m_shadowPipeline = pipelineBuilder.build();

	pipelineBuilder.set_shaders(&cubeShadowVertModule, nullptr, &cubeShadowFragModule);
	pipelineBuilder.set_color_blend_state(VK_COLOR_COMPONENT_R_BIT);
	pipelineBuilder.set_color_attachment_format(VK_FORMAT_R32_SFLOAT);
	pipelineBuilder.enable_color_output();
	pipelineBuilder.set_view_mask(0b111111);
	m_cubeShadowPipeline = pipelineBuilder.build();

	/* {
		pipelineBuilder.set_shaders(environmentMapVertModule, reflectFragModule);
		m_reflectPipeline = pipelineBuilder.build();

		pipelineBuilder.set_shaders(environmentMapVertModule, refractFragModule);
		m_refractPipeline = pipelineBuilder.build();

		pipelineBuilder.set_rasterizer_state(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
		pipelineBuilder.set_shaders(skyboxVertModule, skyboxFragModule);
		m_skyboxPipeline = pipelineBuilder.build();
	}*/


	// TO-DO: we should find a better way of doing this. Perhaps pipeline builder should deallocate these automatically after creating the pipeline?
	// we're free to destroy shader modules after pipeline creation
	vkDestroyShaderModule(m_device.device, lightShadowVertModule, nullptr);
	vkDestroyShaderModule(m_device.device, lightShadowFragModule, nullptr);
	vkDestroyShaderModule(m_device.device, lightCubeShadowFragModule, nullptr);
	vkDestroyShaderModule(m_device.device, lightCubeShadowPCSSFragModule, nullptr);
	vkDestroyShaderModule(m_device.device, lightVertModule, nullptr);
	vkDestroyShaderModule(m_device.device, lightFragModule, nullptr);
	vkDestroyShaderModule(m_device.device, shadowVertModule, nullptr);
	vkDestroyShaderModule(m_device.device, shadowFragModule, nullptr);
	vkDestroyShaderModule(m_device.device, cubeShadowVertModule, nullptr);
	vkDestroyShaderModule(m_device.device, cubeShadowFragModule, nullptr);
	//vkDestroyShaderModule(m_device.device, lightDrawsVertModule, nullptr);
	//vkDestroyShaderModule(m_device.device, lightDrawsFragModule, nullptr);
	vkDestroyShaderModule(m_device.device, skyboxVertModule, nullptr);
	vkDestroyShaderModule(m_device.device, skyboxFragModule, nullptr);

	vkDestroyShaderModule(m_device.device, environmentMapVertModule, nullptr);
	vkDestroyShaderModule(m_device.device, reflectFragModule, nullptr);
	vkDestroyShaderModule(m_device.device, refractFragModule, nullptr);

	vkDestroyShaderModule(m_device.device, bezierVertModule, nullptr);	
	vkDestroyShaderModule(m_device.device, bezierTCSModule, nullptr);	
	vkDestroyShaderModule(m_device.device, bezierTESModule, nullptr);
	vkDestroyShaderModule(m_device.device, bezierFragModule, nullptr);
}

void Kleicha::init_descriptors() {

	{			// create global descriptor set layout	
		VkDescriptorBindingFlags bindingFlags[5]{
			{},
			{},
			{},
			{},
			{VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT }
		};
		VkDescriptorSetLayoutBindingFlagsCreateInfo layoutBindingFlagsInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
		layoutBindingFlagsInfo.bindingCount = std::size(bindingFlags);
		layoutBindingFlagsInfo.pBindingFlags = bindingFlags;

		VkDescriptorSetLayoutBinding bindings[5]{
			{0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr},
			{1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr},
			{2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,VK_SHADER_STAGE_ALL, nullptr},
			{3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr},
			{4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 200, VK_SHADER_STAGE_ALL, nullptr}
		};

		// create descriptor set layout
		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		descriptorSetLayoutInfo.pNext = &layoutBindingFlagsInfo;
		descriptorSetLayoutInfo.bindingCount = std::size(bindings);
		descriptorSetLayoutInfo.pBindings = bindings;
		VK_CHECK(vkCreateDescriptorSetLayout(m_device.device, &descriptorSetLayoutInfo, nullptr, &m_globDescSetLayout));
	}

	{		// create per frame descriptor set layout
		VkDescriptorSetLayoutBinding bindings[5]{
			{0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr},
			{1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr},
			{2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr},
			{3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(m_lights.size()), VK_SHADER_STAGE_ALL, nullptr}, // 2D shadow map
			{4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(m_lights.size()), VK_SHADER_STAGE_ALL, nullptr}, // cube shadow map
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
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 200}	// Textures
	};

	VkDescriptorPoolCreateInfo descriptorPoolInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	descriptorPoolInfo.pNext = nullptr;
	descriptorPoolInfo.maxSets = 10;
	descriptorPoolInfo.poolSizeCount = std::size(poolDescriptorSizes);
	descriptorPoolInfo.pPoolSizes = poolDescriptorSizes;

	VK_CHECK(vkCreateDescriptorPool(m_device.device, &descriptorPoolInfo, nullptr, &m_descPool));

	{
		uint32_t variableSizedDescriptorSize{ 200 };
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
	init_info.MinImageCount = m_device.physicalDevice.surfaceSupportDetails.capabilities.minImageCount;
	init_info.ImageCount = static_cast<uint32_t>(m_swapchain.images.size());
	init_info.UseDynamicRendering = true;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	init_info.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_swapchain.imageFormat.format;

	ImGui_ImplVulkan_Init(&init_info);

	ImGui_ImplVulkan_CreateFontsTexture();
}

void Kleicha::init_load_scene() {

	std::vector<vkt::Mesh> meshes{};
	std::vector<vkt::DrawData> draws{};
	std::vector<vkt::TextureIndices> texIndices{};
	std::vector<std::string> texturePaths{};
	
	if (!utils::load_gltf("../data/Sponza/glTF/Sponza.gltf", meshes, draws, m_meshTransforms, texIndices, texturePaths)) {
		throw std::runtime_error{ "[Kleicha] Failed to load scene!" };
	}

	if (!utils::load_gltf("../data/DamagedHelmet/glTF/DamagedHelmet.gltf", meshes, draws, m_meshTransforms, texIndices, texturePaths)) {
		throw std::runtime_error{ "[Kleicha] Failed to load scene!" };
	}

	if (!utils::load_gltf("../data/BoomBox/glTF/BoomBox.gltf", meshes, draws, m_meshTransforms, texIndices, texturePaths)) {
		throw std::runtime_error{ "[Kleicha] Failed to load scene!" };
	}


	// now we would like to traverse meshes and build the unified vertex and indices buffer where host draws will keep track of our different meshes
	std::vector<vkt::Vertex> unifiedVertices{};
	std::vector<glm::uvec3> unifiedTriangles{};
	for (std::size_t i{ 0 }; i < meshes.size(); ++i) {

		if (meshes[i].computeTangent)
			utils::compute_mesh_tangents(meshes[i]);

		// store the current vertex and triangle counts
		int32_t vertexCount{ static_cast<int32_t>(unifiedVertices.size()) };
		uint32_t triangleCount{ static_cast<uint32_t>(unifiedTriangles.size()) };

		// store vertex and index buffer mesh start positions
		vkt::HostDrawData hostDraw{};
		hostDraw.drawId = static_cast<uint32_t>(i);
		hostDraw.vertexOffset = vertexCount;
		hostDraw.indicesOffset = triangleCount * glm::uvec3::length();
		hostDraw.indicesCount = static_cast<uint32_t>(meshes[i].tInd.size() * glm::uvec3::length());

		if (meshes[i].useAlphaTest)
			m_alphaDraws.push_back(hostDraw);
		else
			m_draws.push_back(hostDraw);

		// allocate memory for mesh vertex data and insert it
		unifiedVertices.reserve(vertexCount + meshes[i].verts.size());
		unifiedVertices.insert(unifiedVertices.end(), meshes[i].verts.begin(), meshes[i].verts.end());

		// allocate memory for mesh index data and insert it
		unifiedTriangles.reserve(triangleCount + meshes[i].tInd.size());
		unifiedTriangles.insert(unifiedTriangles.end(), meshes[i].tInd.begin(), meshes[i].tInd.end());
	}

	m_meshTransforms.push_back(vkt::Transform{ .model = glm::translate(glm::mat4{1.0f}, glm::vec3{0.0f, -5.0f, -3.0f}) * glm::scale(glm::mat4{1.0f}, glm::vec3{5.0f, 5.0f, 5.0f})});
	draws.push_back(vkt::DrawData{ .transformIndex = static_cast<uint32_t>(m_meshTransforms.size() - 1) });
	m_bezierDraw = vkt::HostDrawData{ .drawId = static_cast<uint32_t>(draws.size() - 1)};

	m_vertexBuffer = upload_data(unifiedVertices.data(), unifiedVertices.size() * sizeof(vkt::Vertex), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	m_indexBuffer = upload_data(unifiedTriangles.data(), unifiedTriangles.size() * sizeof(glm::uvec3), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	m_drawBuffer = upload_data(draws.data(), sizeof(vkt::DrawData) * draws.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	m_textureIndicesBuffer = upload_data(texIndices.data(), sizeof(vkt::TextureIndices) * texIndices.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	
	m_textures.push_back(upload_texture_image("../textures/empty.jpg"));

	for (std::size_t i{ 0 }; i < texturePaths.size(); ++i) {
		// check if the texture we're processing is a normal map
		bool isNormalMap{ false };

		for (std::size_t j{ 0 }; j < texIndices.size(); ++j) {
			if ((i + 1) == texIndices[j].normalTexture) {
				isNormalMap = true;
				break;
			}
		}
		if(isNormalMap)
			m_textures.push_back(upload_texture_image(texturePaths[i].c_str(), VK_FORMAT_R8G8B8A8_UNORM));
		else
			m_textures.push_back(upload_texture_image(texturePaths[i].c_str()));
	}

}

void Kleicha::init_image_buffers(bool windowResized) {


	VkImageCreateInfo rasterImageInfo{ init::create_image_info(INTERMEDIATE_IMAGE_FORMAT, m_swapchain.imageExtent,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 1)};

	VkImageCreateInfo depthImageInfo{ init::create_image_info(DEPTH_IMAGE_FORMAT, m_swapchain.imageExtent,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 1) };

	VkImageCreateInfo shadowImageInfo{ init::create_image_info(DEPTH_IMAGE_FORMAT, m_swapchain.imageExtent, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 1) };

	VmaAllocationCreateInfo allocationInfo{};
	allocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	allocationInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VK_CHECK(vmaCreateImage(m_allocator, &rasterImageInfo, &allocationInfo, &rasterImage.image, &rasterImage.allocation, &rasterImage.allocationInfo));
	VkImageViewCreateInfo rasterViewInfo{ init::create_image_view_info(rasterImage.image, INTERMEDIATE_IMAGE_FORMAT, VK_IMAGE_ASPECT_COLOR_BIT, 1) };
	VK_CHECK(vkCreateImageView(m_device.device, &rasterViewInfo, nullptr, &rasterImage.imageView));

	VK_CHECK(vmaCreateImage(m_allocator, &depthImageInfo, &allocationInfo, &depthImage.image, &depthImage.allocation, &depthImage.allocationInfo));
	VkImageViewCreateInfo depthViewInfo{ init::create_image_view_info(depthImage.image, DEPTH_IMAGE_FORMAT, VK_IMAGE_ASPECT_DEPTH_BIT, 1) };
	VK_CHECK(vkCreateImageView(m_device.device, &depthViewInfo, nullptr, &depthImage.imageView));

	// allocate per frame shadow maps that'll be used as depth attachments in their own passes
	for (auto& frame : m_frames) {
		frame.shadowMaps.resize(m_lights.size());
		for (auto& shadowMap : frame.shadowMaps) {
			// create regular (depth) shadow maps
			VK_CHECK(vmaCreateImage(m_allocator, &shadowImageInfo, &allocationInfo, &shadowMap.image, &shadowMap.allocation, &shadowMap.allocationInfo));
			VkImageViewCreateInfo shadowViewInfo{ init::create_image_view_info(shadowMap.image, DEPTH_IMAGE_FORMAT, VK_IMAGE_ASPECT_DEPTH_BIT, shadowMap.mipLevels)};
			VK_CHECK(vkCreateImageView(m_device.device, &shadowViewInfo, nullptr, &shadowMap.imageView));
		}
	}

	if (!windowResized) {

		VkImageCreateInfo cubeShadowColorImageInfo{ init::create_image_info(VK_FORMAT_R32_SFLOAT, SHADOW_CUBE_EXTENT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 1, 6) };
		VkImageCreateInfo cubeShadowDepthImageInfo{ init::create_image_info(DEPTH_IMAGE_FORMAT, SHADOW_CUBE_EXTENT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 1, 6) };

		for (auto& frame : m_frames) {
			frame.cubeShadowMaps.resize(m_lights.size());

			// create cube shadow maps that wioll store world space distance between surface point and light
			for (auto& cubeShadowMap : frame.cubeShadowMaps) {
				VK_CHECK(vmaCreateImage(m_allocator, &cubeShadowColorImageInfo, &allocationInfo, &cubeShadowMap.colorImage.image, &cubeShadowMap.colorImage.allocation, &cubeShadowMap.colorImage.allocationInfo));
				VkImageViewCreateInfo shadowCubeViewInfo{ init::create_image_view_info(cubeShadowMap.colorImage.image, VK_FORMAT_R32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_REMAINING_ARRAY_LAYERS) };
				VK_CHECK(vkCreateImageView(m_device.device, &shadowCubeViewInfo, nullptr, &cubeShadowMap.colorImage.imageView));

				VK_CHECK(vmaCreateImage(m_allocator, &cubeShadowDepthImageInfo, &allocationInfo, &cubeShadowMap.depthImage.image, &cubeShadowMap.depthImage.allocation, &cubeShadowMap.depthImage.allocationInfo));
				VkImageViewCreateInfo shadowCubeDepthViewInfo{ init::create_image_view_info(cubeShadowMap.depthImage.image, DEPTH_IMAGE_FORMAT, VK_IMAGE_ASPECT_DEPTH_BIT, cubeShadowMap.depthImage.mipLevels) };
				vkCreateImageView(m_device.device, &shadowCubeDepthViewInfo, nullptr, &cubeShadowMap.depthImage.imageView);

			}
		}
	}
		// transition depth image layouts
		immediate_submit([&](VkCommandBuffer cmdBuffer) {
			utils::image_memory_barrier(cmdBuffer, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, depthImage.image, depthImage.mipLevels);

			for (auto& frame : m_frames) {
				for (auto& shadowMap : frame.shadowMaps) {
					utils::image_memory_barrier(cmdBuffer, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, shadowMap.image, shadowMap.mipLevels);
				}

				if (!windowResized) {
					for (auto& cubeShadowMap : frame.cubeShadowMaps) {
						utils::image_memory_barrier(cmdBuffer, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, cubeShadowMap.depthImage.image, cubeShadowMap.depthImage.mipLevels);
					}
				}
				
			}
			});
	}

/*std::vector<vkt::GPUMesh> Kleicha::load_mesh_data() {


	std::vector<vkt::Mesh> meshes{};
	meshes.emplace_back(utils::generate_pyramid_mesh());
	meshes.emplace_back(utils::generate_sphere(64));
	meshes.emplace_back(utils::generate_torus(64, 1.2f, 0.45f));
	meshes.emplace_back(utils::generate_cube_mesh());
	meshes.emplace_back(utils::load_obj_mesh("../models/shuttle.obj", vkt::MeshType::SHUTTLE));
	meshes.emplace_back(utils::load_obj_mesh("../models/icosphere.obj", vkt::MeshType::ICOSPHERE));
	meshes.emplace_back(utils::load_obj_mesh("../models/dolphinHighPoly.obj", vkt::MeshType::DOLPHIN));
	meshes.emplace_back(utils::load_obj_mesh("../models/grid.obj", vkt::MeshType::PLANE));
	//utils::load_gltf("F:\\Projects\\glTF Sample Models\\glTF-Sample-Models-main\\glTF-Sample-Models-main\\2.0\\Sponza\\glTF\\Sponza.gltf", meshes);
	meshes.emplace_back(utils::load_obj_mesh("../models/sponza.obj", vkt::MeshType::SPONZA));


	std::vector<vkt::GPUMesh> GPUMeshes(meshes.size());
	// create unified vertex and index buffers for upload. meshDrawData will keep track of mesh buffer offsets within the unified buffer.
	std::vector<vkt::Vertex> unifiedVertices{};
	std::vector<glm::uvec3> unifiedTriangles{};

	for (std::size_t i{ 0 }; i < meshes.size(); ++i) {

		// store the current vertex and triangle counts
		int32_t vertexCount{ static_cast<int32_t>(unifiedVertices.size()) };
		uint32_t triangleCount{ static_cast<uint32_t>(unifiedTriangles.size()) };

		// store vertex and index buffer mesh start positions
		GPUMeshes[i].meshType = meshes[i].meshType;
		GPUMeshes[i].vertexOffset = vertexCount;
		GPUMeshes[i].indicesOffset = triangleCount * glm::uvec3::length();
		GPUMeshes[i].indicesCount = static_cast<uint32_t>(meshes[i].tInd.size() * glm::uvec3::length());

		// allocate memory for mesh vertex data and insert it
		unifiedVertices.reserve(vertexCount + meshes[i].verts.size());
		unifiedVertices.insert(unifiedVertices.end(), meshes[i].verts.begin(), meshes[i].verts.end());

		// allocate memory for mesh index data and insert it
		unifiedTriangles.reserve(triangleCount + meshes[i].tInd.size());
		unifiedTriangles.insert(unifiedTriangles.end(), meshes[i].tInd.begin(), meshes[i].tInd.end());
	}

	m_vertexBuffer = upload_data(unifiedVertices.data(), unifiedVertices.size() * sizeof(vkt::Vertex), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	m_indexBuffer = upload_data(unifiedTriangles.data(), unifiedTriangles.size() * sizeof(glm::uvec3), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

	return GPUMeshes;
}*/

void Kleicha::init_dynamic_buffers() {

	using namespace vkt;

	GlobalData globalData{};
	globalData.ambientLight = glm::vec4{ 0.22f, 0.22f, 0.22f, 1.0f };
	globalData.bias = glm::mat4{
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f
	};
	globalData.lightCount = static_cast<uint32_t>(m_lights.size());
	m_globalsBuffer = upload_data(&globalData, sizeof(globalData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

	// allocate per frame buffers such as transform buffer
	for (auto& frame : m_frames) {
		frame.transformBuffer = utils::create_buffer(m_allocator, sizeof(Transform) * m_meshTransforms.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

		frame.lightBuffer = utils::create_buffer(m_allocator, sizeof(Light) * m_lights.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

		frame.materialBuffer = utils::create_buffer(m_allocator, sizeof(Material) * m_materials.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
	}
}

void Kleicha::init_lights() {		
	m_materials.push_back(vkt::Material::none());
	// create a standard white light 
	vkt::Light pointLight{
		.ambient = {0.24f, 0.24f, 0.24f, 1.0f},
		.diffuse = {0.6f, 0.6f, 0.6f, 1.0f},
		.specular = {1.0f, 1.0f, 1.0f, 1.0f},
		.attenuationFactors = {1.0f, 0.133f, 0.050f},
		.lightSize = {4.0f},
		.mPos = { -3.0f, 5.0f, 0.0f },
		.frustumWidth = {3.75f},
	};

	m_lights.push_back(pointLight);

	//pointLight.mPos = {3.0f, 5.0f, 0.0f};
	//m_lights.push_back(pointLight);

	/*pointLight.mPos = {-6.0f, 1.5f, -5.0f};
	m_lights.push_back(pointLight);*/

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

void Kleicha::init_samplers() {
	VkSamplerCreateInfo textureSamplerInfo{ init::create_sampler_info(m_device, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_TRUE, VK_LOD_CLAMP_NONE) };
	VK_CHECK(vkCreateSampler(m_device.device, &textureSamplerInfo, nullptr, &m_textureSampler));


	VkSamplerCreateInfo shadowSamplerInfo{ init::create_sampler_info(m_device, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE) };
	VK_CHECK(vkCreateSampler(m_device.device, &shadowSamplerInfo, nullptr, &m_shadowSampler));
}

void Kleicha::init_write_descriptor_sets() {

	utils::update_set_buffer_descriptor(m_device.device, m_globalDescSet, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_vertexBuffer.buffer);
	utils::update_set_buffer_descriptor(m_device.device, m_globalDescSet, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_drawBuffer.buffer);
	utils::update_set_buffer_descriptor(m_device.device, m_globalDescSet, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_globalsBuffer.buffer);
	utils::update_set_buffer_descriptor(m_device.device, m_globalDescSet, 3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_textureIndicesBuffer.buffer);
	utils::update_set_image_sampler_descriptor(m_device.device, m_globalDescSet, 4, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_textureSampler, m_textures);

	// per frame descriptor set writes
	for (auto& frame : m_frames) {

		utils::update_set_buffer_descriptor(m_device.device, frame.descriptorSet, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frame.transformBuffer.buffer);
		utils::update_set_buffer_descriptor(m_device.device, frame.descriptorSet, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frame.materialBuffer.buffer);
		utils::update_set_buffer_descriptor(m_device.device, frame.descriptorSet, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frame.lightBuffer.buffer);

		utils::update_set_image_sampler_descriptor(m_device.device, frame.descriptorSet, 3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_shadowSampler, frame.shadowMaps);
		utils::update_set_image_sampler_descriptor(m_device.device, frame.descriptorSet, 4, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_shadowSampler, frame.cubeShadowMaps);
	}

}

// for uploading texture cube maps
vkt::Image Kleicha::upload_texture_image(const char** filePaths) {

	stbi_set_flip_vertically_on_load(true);
	
	int width, height;
	stbi_uc* faces[6]{};
	for (std::size_t i{ 0 }; i < 6; ++i) {
		faces[i] = stbi_load(filePaths[i], &width, &height, nullptr, STBI_rgb_alpha);
		if (!faces[i])
		{
			stbi_image_free(faces[i]);
			throw std::runtime_error{ "[Kleicha] Failed to load cube texture image" + std::string{ stbi_failure_reason() } };
		}
		fmt::println("Loaded Texture: {}", filePaths[i]);
	}

	VkExtent2D textureExtent{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
	[[maybe_unused]]VkDeviceSize bufferSize{ static_cast<VkDeviceSize>(width * height * 4 * 6) };

	VmaAllocationCreateInfo allocationInfo{};
	allocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	allocationInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	vkt::Image textureImage{};
	textureImage.mipLevels = 1;
	VkImageCreateInfo textureImageInfo{ init::create_image_info(VK_FORMAT_R8G8B8A8_SRGB, textureExtent, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, textureImage.mipLevels, 6) };
	VK_CHECK(vmaCreateImage(m_allocator, &textureImageInfo, &allocationInfo, &textureImage.image, &textureImage.allocation, &textureImage.allocationInfo));
	VkImageViewCreateInfo imageViewInfo{ init::create_image_view_info(textureImage.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, textureImage.mipLevels, 6) };
	VK_CHECK(vkCreateImageView(m_device.device, &imageViewInfo, nullptr, &textureImage.imageView));

	vkt::Buffer stagingBuffer{ utils::create_buffer(m_allocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT) };

	// unify cube texture faces into staging buffer
	for (std::size_t i{ 0 }; i < 6; ++i) {
		memcpy(reinterpret_cast<char*>(stagingBuffer.allocationInfo.pMappedData) + static_cast<std::size_t>(width * height) * 4 * i, faces[i], static_cast<std::size_t>(width * height) * 4);
		stbi_image_free(faces[i]);
	}

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
		imageCopy.imageSubresource.layerCount = 6;
		imageCopy.imageSubresource.baseArrayLayer = 0;
		imageCopy.imageOffset = { .x = 0,.y = 0,.z = 0 };
		imageCopy.imageExtent = { .width = textureExtent.width, .height = textureExtent.height, .depth = 1 };
		vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer.buffer, textureImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);

		});

	vmaDestroyBuffer(m_allocator, stagingBuffer.buffer, stagingBuffer.allocation);

	return textureImage;
}

vkt::Image Kleicha::upload_texture_image(const char* filePath, VkFormat format) {

	stbi_set_flip_vertically_on_load(false);

	int width, height;
	stbi_uc* textureData{ stbi_load(filePath, &width, &height, nullptr, STBI_rgb_alpha) };
	if (!textureData) {
		throw std::runtime_error{ "[Kleicha] Failed to load texture image: " + std::string{ stbi_failure_reason() } };
	}

	fmt::println("Loaded Texture: {}", filePath);


	VkExtent2D textureExtent{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
	VkDeviceSize bufferSize{ static_cast<VkDeviceSize>(width * height * 4) };

	VmaAllocationCreateInfo allocationInfo{};
	allocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	allocationInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	vkt::Image textureImage{};
	// compute mip levels from longest edge
	textureImage.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
	// allocate device_local memory to store the texture image
	VkImageCreateInfo textureImageInfo{ init::create_image_info(format, textureExtent, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, textureImage.mipLevels) };
	VK_CHECK(vmaCreateImage(m_allocator, &textureImageInfo, &allocationInfo, &textureImage.image, &textureImage.allocation, &textureImage.allocationInfo));
	VkImageViewCreateInfo imageViewInfo{ init::create_image_view_info(textureImage.image, format, VK_IMAGE_ASPECT_COLOR_BIT, textureImage.mipLevels) };
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
		imageCopy.imageOffset = { .x = 0,.y = 0,.z = 0 };
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
				dstMipExtent.width = dstMipExtent.width >> 1;
			if (srcMipHeight > 1)
				dstMipExtent.height = dstMipExtent.height >> 1;

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
				srcMipWidth = srcMipWidth >> 1;
			if (srcMipHeight > 1)
				srcMipHeight = srcMipHeight >> 1;
		}

		});

	vmaDestroyBuffer(m_allocator, stagingBuffer.buffer, stagingBuffer.allocation);
	return textureImage;
}

void Kleicha::deallocate_frame_images() const {

	vkDestroyImageView(m_device.device, rasterImage.imageView, nullptr);
	vmaDestroyImage(m_allocator, rasterImage.image, rasterImage.allocation);

	vkDestroyImageView(m_device.device, depthImage.imageView, nullptr);
	vmaDestroyImage(m_allocator, depthImage.image, depthImage.allocation);

	for (const auto& frame : m_frames) {
		for (const auto& shadowMap : frame.shadowMaps) {
			vmaDestroyImage(m_allocator, shadowMap.image, shadowMap.allocation);
			vkDestroyImageView(m_device.device, shadowMap.imageView, nullptr);
		}
	}

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
	m_perspProj = utils::orthographicProj(glm::radians(90.0f),
		static_cast<float>(m_windowExtent.width) / m_windowExtent.height, 1000.0f, 0.1f) * utils::perspective(1000.0f, 0.1f);
	init_swapchain();
	init_image_buffers(true);

	// update per frame shadow map descriptors to reference the new image buffers
	for (const auto& frame : m_frames) {
		utils::update_set_image_sampler_descriptor(m_device.device, frame.descriptorSet, 3, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL, m_shadowSampler, frame.shadowMaps);
	}
}

void Kleicha::update_dynamic_buffers(const vkt::Frame& frame, [[maybe_unused]] float currentTime, const glm::mat4& shadowCubePerspProj) {

	// update light views
	for (auto& light : m_lights) {
		light.viewProj = m_perspProj * utils::lookAt(light.mPos, glm::vec3{ 0.0f, 0.0f, 0.0f }, WORLD_UP);

		light.cubeViewProjs[1] = shadowCubePerspProj * glm::lookAt(light.mPos, light.mPos + glm::vec3{ 1.0f, 0.0f, 0.0f }, glm::vec3{ 0.0f, 1.0f, 0.0f });	// +X
		light.cubeViewProjs[0] = shadowCubePerspProj * glm::lookAt(light.mPos, light.mPos + glm::vec3{ -1.0f, 0.0f, 0.0f }, glm::vec3{ 0.0f, 1.0f, 0.0f }); // -X

		light.cubeViewProjs[2] = shadowCubePerspProj * glm::lookAt(light.mPos, light.mPos + glm::vec3{ 0.0f, -1.0f, 0.0f }, glm::vec3{ 0.0f, 0.0f, -1.0f });  // +Y
		light.cubeViewProjs[3] = shadowCubePerspProj * glm::lookAt(light.mPos, light.mPos + glm::vec3{ 0.0f, 1.0f, 0.0f }, glm::vec3{ 0.0f, 0.0f, 1.0f });	  // -Y

		light.cubeViewProjs[5] = shadowCubePerspProj * glm::lookAt(light.mPos, light.mPos + glm::vec3{ 0.0f, 0.0f, 1.0f }, glm::vec3{ 0.0f, 1.0f, 0.0f });	  // -Z
		light.cubeViewProjs[4] = shadowCubePerspProj * glm::lookAt(light.mPos, light.mPos + glm::vec3{ 0.0f, 0.0f, -1.0f }, glm::vec3{ 0.0f, 1.0f, 0.0f });	  // +Z
	}

	// update mesh transforms
	glm::mat4 view{ m_camera.getViewMatrix() };

	for (auto& transform : m_meshTransforms) {
		transform.modelInvTr = glm::transpose(glm::inverse(transform.model));
		transform.modelView = view * transform.model;
		transform.modelViewInvTr = glm::transpose(glm::inverse(transform.modelView));
	}

	// This is somewhat okay but not very robust as it depends on the light meshes being added together. This may be fine, it may not... We'll see and provide an alternative solution if needed.
	for (std::size_t i{0}; i < m_lights.size(); ++i) {
		m_lights[i].mvPos = view * glm::vec4{ m_lights[i].mPos, 1.0f };

		/*m_meshTransforms[m_lightDrawData[0].transformIndex + i].modelView = view * glm::translate(glm::mat4{1.0f}, m_lights[i].mPos) * glm::scale(glm::mat4{1.0f}, glm::vec3{0.1f, 0.1f, 0.1f});
		m_meshTransforms[m_lightDrawData[0].transformIndex + i].modelViewInvTr = glm::transpose(glm::inverse(m_meshTransforms[m_lightDrawData[0].transformIndex + i].modelView));*/
	}

	// update per frame buffers
	memcpy(frame.transformBuffer.allocation->GetMappedData(), m_meshTransforms.data(), sizeof(vkt::Transform) * m_meshTransforms.size());
	memcpy(frame.materialBuffer.allocation->GetMappedData(), m_materials.data(), sizeof(vkt::Material) * m_materials.size());
	memcpy(frame.lightBuffer.allocation->GetMappedData(), m_lights.data(), sizeof(vkt::Light) * m_lights.size());

	//TODO: On our graphice device, all host-visible device memory is cache coherent. However, this is not guaranteed on other devices. On devices where this memory
	// does not have the property 'VK_MEMORY_PROPERTY_HOST_COHERENT_BIT', we should make all host writes visible before
	// the below draw calls using a pipeline barrier.
}

void Kleicha::record_draws(const vkt::Frame& frame, VkPipeline* opaquePipeline, VkPipeline* alphaPipeline) {

	assert(opaquePipeline);

	vkCmdBindPipeline(frame.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *opaquePipeline);
	for (const auto& draw : m_draws) {
		m_pushConstants.drawId = draw.drawId;
		vkCmdPushConstants(frame.cmdBuffer, m_dummyPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(vkt::PushConstants), &m_pushConstants);
		vkCmdDrawIndexed(frame.cmdBuffer, draw.indicesCount, 1, draw.indicesOffset, draw.vertexOffset, 0);
	}

	if (alphaPipeline)
		vkCmdBindPipeline(frame.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *alphaPipeline);

	for (const auto& draw : m_alphaDraws) {
		m_pushConstants.drawId = draw.drawId;
		vkCmdPushConstants(frame.cmdBuffer, m_dummyPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(vkt::PushConstants), &m_pushConstants);
		vkCmdDrawIndexed(frame.cmdBuffer, draw.indicesCount, 1, draw.indicesOffset, draw.vertexOffset, 0);
	}
}

void Kleicha::shadow_cube_pass(const vkt::Frame& frame) {

	utils::set_viewport_scissor(frame, SHADOW_CUBE_EXTENT);

	VkClearValue colorClearValue{ {{FLT_MAX, 0.0f, 0.0f, 1.0f}} };
	VkClearValue depthClearValue{ .depthStencil = {0.0f, 0U} };

	VkRenderingInfo cubeShadowRenderingInfo{ .sType = VK_STRUCTURE_TYPE_RENDERING_INFO };
	cubeShadowRenderingInfo.pNext = nullptr;
	cubeShadowRenderingInfo.renderArea.extent = SHADOW_CUBE_EXTENT;
	cubeShadowRenderingInfo.renderArea.offset = { 0,0 };
	cubeShadowRenderingInfo.layerCount = 1;
	cubeShadowRenderingInfo.viewMask = 0b111111; // each bit of this bitfield specifies which views are active during rendering. our cubemap requires 6 views thus we enable views 0 through to 5.
	cubeShadowRenderingInfo.colorAttachmentCount = 1;

	vkCmdBindPipeline(frame.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_cubeShadowPipeline);

	for (uint32_t j{ 0 }; j < m_lights.size(); ++j) {
		VkRenderingAttachmentInfo cubeColorAttachment{ init::create_rendering_attachment_info(frame.cubeShadowMaps[j].colorImage.imageView, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &colorClearValue) };
		VkRenderingAttachmentInfo cubeDepthAttachment{ init::create_rendering_attachment_info(frame.cubeShadowMaps[j].depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, &depthClearValue) };

		cubeShadowRenderingInfo.pColorAttachments = &cubeColorAttachment;
		cubeShadowRenderingInfo.pDepthAttachment = &cubeDepthAttachment;

		m_pushConstants.lightId = j;

		vkCmdBeginRendering(frame.cmdBuffer, &cubeShadowRenderingInfo);

		for (const auto& draw : m_draws) {
			m_pushConstants.drawId = draw.drawId;
			vkCmdPushConstants(frame.cmdBuffer, m_dummyPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(vkt::PushConstants), &m_pushConstants);
			vkCmdDrawIndexed(frame.cmdBuffer, draw.indicesCount, 1, draw.indicesOffset, draw.vertexOffset, 0);
		}

		for (const auto& draw : m_alphaDraws) {
			m_pushConstants.drawId = draw.drawId;
			vkCmdPushConstants(frame.cmdBuffer, m_dummyPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(vkt::PushConstants), &m_pushConstants);
			vkCmdDrawIndexed(frame.cmdBuffer, draw.indicesCount, 1, draw.indicesOffset, draw.vertexOffset, 0);
		}

		vkCmdEndRendering(frame.cmdBuffer);
		// transition shadow cube map
		utils::image_memory_barrier(frame.cmdBuffer, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, frame.cubeShadowMaps[j].colorImage.image, 1);
	}
}

void Kleicha::shadow_2D_pass(const vkt::Frame& frame) {

	VkClearValue colorClearValue{ {{0.0f, 0.0f, 0.0f, 1.0f}} };
	VkClearValue depthClearValue{ .depthStencil = {0.0f, 0U} };
	utils::set_viewport_scissor(frame, m_swapchain.imageExtent);

	VkRenderingInfo renderingInfo{ .sType = VK_STRUCTURE_TYPE_RENDERING_INFO };
	renderingInfo.pNext = nullptr;
	renderingInfo.renderArea.extent = m_swapchain.imageExtent;
	renderingInfo.renderArea.offset = { 0,0 };
	renderingInfo.layerCount = 1;
	renderingInfo.viewMask = 0; //we're not using multiview
	renderingInfo.colorAttachmentCount = 0;
	renderingInfo.pColorAttachments = nullptr;

	m_pushConstants.perspectiveProjection = m_perspProj;

	vkCmdBindPipeline(frame.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_shadowPipeline);

	// shadow passes
	for (uint32_t j{ 0 }; j < m_lights.size(); ++j) {
		VkRenderingAttachmentInfo shadowDepthAttachment{ init::create_rendering_attachment_info(frame.shadowMaps[j].imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, &depthClearValue, VK_TRUE) };
		renderingInfo.pDepthAttachment = &shadowDepthAttachment;
		
		vkCmdBeginRendering(frame.cmdBuffer, &renderingInfo);

		m_pushConstants.lightId = j;

		for (const auto& draw : m_draws) {
			m_pushConstants.drawId = draw.drawId;
			vkCmdPushConstants(frame.cmdBuffer, m_dummyPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(vkt::PushConstants), &m_pushConstants);
			vkCmdDrawIndexed(frame.cmdBuffer, draw.indicesCount, 1, draw.indicesOffset, draw.vertexOffset, 0);
		}

		for (const auto& draw : m_alphaDraws) {
			m_pushConstants.drawId = draw.drawId;
			vkCmdPushConstants(frame.cmdBuffer, m_dummyPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(vkt::PushConstants), &m_pushConstants);
			vkCmdDrawIndexed(frame.cmdBuffer, draw.indicesCount, 1, draw.indicesOffset, draw.vertexOffset, 0);
		}

		vkCmdEndRendering(frame.cmdBuffer);
	}
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

		ImGui::Text("Shadows: ");
		ImGui::Checkbox("Shadow Mapping (2D) PCF", &m_enableShadows);
		ImGui::Checkbox("Shadow Mapping (Cube) PCF", &m_enableCubeShadows);
		ImGui::Checkbox("Shadow Mapping (Cube) PCSS", &m_enableCubeShadowsPCSS);
		ImGui::NewLine();
		ImGui::Text("Surface Detail: ");
		ImGui::Checkbox("Bump Mapping", &m_enableBumpMapping);
		ImGui::Checkbox("Height Mapping", &m_enableHeightMapping);

		if (ImGui::CollapsingHeader("Lights")) {

			for (std::size_t i{ 0 }; i < m_lights.size(); ++i) {
				ImGui::PushID(static_cast<int>(i));
				ImGui::Text("Light %d", i);
				ImGui::SliderFloat3("Light World Pos", &m_lights[i].mPos.x, -10.0f, 50.0f);
				//ImGui::ColorPicker3("Light Ambient", &m_lights[i].ambient.r);)
				ImGui::SliderFloat3("Light Ambient", &m_lights[i].ambient.r, 0.0f, 1.0f);
				ImGui::SliderFloat3("Light Diffuse", &m_lights[i].diffuse.r, 0.0f, 1.0f);
				ImGui::SliderFloat3("Light Specular", &m_lights[i].specular.r, 0.0f, 1.0f);
				ImGui::SliderFloat3("Attenuation Factors", &m_lights[i].attenuationFactors.s, 0.0f, 10.0f);
				ImGui::SliderFloat("Light Size", &m_lights[i].lightSize, 0.0f, 20.0f);
				ImGui::SliderFloat("Frustum Width", &m_lights[i].frustumWidth, 0.0f, 20.0f);
				ImGui::NewLine();
				ImGui::PopID();
			}
		}


		if (ImGui::CollapsingHeader("Materials")) {
			for (std::size_t i{ 0 }; i < m_materials.size(); ++i) {
				ImGui::PushID(static_cast<int>(i));
				ImGui::Text("Material %d", i - 1);
				ImGui::SliderFloat3("Material Ambient", &m_materials[i].ambient.r, 0.0f, 1.0f);
				ImGui::SliderFloat3("Material Diffuse", &m_materials[i].diffuse.r, 0.0f, 1.0f);
				ImGui::SliderFloat3("Material Specular", &m_materials[i].specular.r, 0.0f, 1.0f);
				ImGui::SliderFloat("Shininess", &m_materials[i].shininess, 0.0f, 100.0f);
				ImGui::SliderFloat("Refractive Index", &m_materials[i].refractiveIndex, 0.0f, 5.0f);
				ImGui::NewLine();
				ImGui::PopID();
			}
		}
		ImGui::Render();

		draw(currentTime);
	}
	// wait for all driver access to conclude before cleanup
	VK_CHECK(vkDeviceWaitIdle(m_device.device));
}

void Kleicha::draw(float currentTime) {

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

	std::vector<VkImageMemoryBarrier2> imageMemoryBarriers{};
	// forms a dependency chain with vkAcquireNextImageKHR signal semaphore. when semaphores are signaled, all pending writes are made available. i dont need to do this manually here
	VkImageMemoryBarrier2 rastertoTransferDst{ init::create_image_barrier_info(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_NONE,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, rasterImage.image, rasterImage.mipLevels) };
	imageMemoryBarriers.emplace_back(rastertoTransferDst);

	VkImageMemoryBarrier2 scToTransferDst{ init::create_image_barrier_info(VK_PIPELINE_STAGE_2_TRANSFER_BIT, 0,
		VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_swapchain.images[imageIndex], 1) };
	imageMemoryBarriers.emplace_back(scToTransferDst);

	// transition frame shadow cube map layouts to color attachment output
	for (auto& cubeShadowMap : frame.cubeShadowMaps) {
		VkImageMemoryBarrier2 cubeMapToColorAttachment{ init::create_image_barrier_info(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, cubeShadowMap.colorImage.image, 1) };
		imageMemoryBarriers.emplace_back(cubeMapToColorAttachment);
	}

	// transition swapchain image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	VkDependencyInfo dependencyInfo{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
	dependencyInfo.pNext = nullptr;
	dependencyInfo.imageMemoryBarrierCount = static_cast<uint32_t>(imageMemoryBarriers.size());
	dependencyInfo.pImageMemoryBarriers = imageMemoryBarriers.data();

	// image memory barrier
	vkCmdPipelineBarrier2(frame.cmdBuffer, &dependencyInfo);

	VkDescriptorSet descSets[]{ m_globalDescSet, frame.descriptorSet };
	vkCmdBindDescriptorSets(frame.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_dummyPipelineLayout, 0, std::size(descSets), descSets, 0, nullptr);

	m_perspProj = utils::orthographicProj(glm::radians(90.0f),
		static_cast<float>(m_windowExtent.width) / m_windowExtent.height, 1000.0f, 0.1f) * m_persp;

	// cube shadow pass perspective proj
	glm::mat4 shadowCubePerspProj{ utils::orthographicProj(glm::radians(90.0f), static_cast<float>(SHADOW_CUBE_EXTENT.width) / SHADOW_CUBE_EXTENT.height, 1000.0f, 0.1f) * m_persp };
	m_pushConstants.perspectiveProjection = shadowCubePerspProj;
	update_dynamic_buffers(frame, currentTime, shadowCubePerspProj);

	m_pushConstants.viewWorldPos = m_camera.get_world_pos();
	m_pushConstants.enableBumpMapping = m_enableBumpMapping;
	m_pushConstants.enableHeightMapping = m_enableHeightMapping;

	vkCmdBindIndexBuffer(frame.cmdBuffer, m_indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

	// will be used to compute the viewport transformation (NDC to screen space)
	if (m_enableCubeShadows || m_enableCubeShadowsPCSS) {
		shadow_cube_pass(frame);
	}	

	if (m_enableShadows) {
		shadow_2D_pass(frame);
	}

	VkClearValue colorClearValue{ {{0.0f, 0.0f, 0.0f, 1.0f}} };
	VkClearValue depthClearValue{ .depthStencil = {0.0f, 0U} };
	utils::set_viewport_scissor(frame, m_swapchain.imageExtent);
	m_pushConstants.perspectiveProjection = m_perspProj;
	/*		main pass		*/
	// specify the attachments for second pass
	VkRenderingAttachmentInfo colorAttachment{ init::create_rendering_attachment_info(rasterImage.imageView, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &colorClearValue) };
	VkRenderingAttachmentInfo depthAttachment{ init::create_rendering_attachment_info(depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, &depthClearValue) };

	VkRenderingInfo renderingInfo{ .sType = VK_STRUCTURE_TYPE_RENDERING_INFO };
	renderingInfo.pNext = nullptr;
	renderingInfo.renderArea.extent = m_swapchain.imageExtent;
	renderingInfo.renderArea.offset = { 0,0 };
	renderingInfo.layerCount = 1;
	renderingInfo.viewMask = 0; //we're not using multiview
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachment;
	renderingInfo.pDepthAttachment = &depthAttachment;

	vkCmdBeginRendering(frame.cmdBuffer, &renderingInfo);

	if (m_enableShadows)
		record_draws(frame, &m_lightShadowPipeline, &m_lightShadowAlphaPipeline);
	else if (m_enableCubeShadows)
		record_draws(frame, &m_lightCubeShadowPipeline, &m_lightCubeShadowAlphaPipeline);
	else if (m_enableCubeShadowsPCSS)
		record_draws(frame, &m_lightCubeShadowPCSSPipeline, &m_lightCubeShadowPCSSAlphaPipeline);
	else
		record_draws(frame, &m_lightPipeline, &m_lightAlphaPipeline);

	m_pushConstants.drawId = m_bezierDraw.drawId;
	vkCmdPushConstants(frame.cmdBuffer, m_dummyPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(vkt::PushConstants), &m_pushConstants);
	vkCmdBindPipeline(frame.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_bezierPipeline);

	vkCmdDraw(frame.cmdBuffer, 1, 1, 0, 0);

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
	vkDestroySampler(m_device.device, m_shadowSampler, nullptr);
	for (const auto& texture : m_textures) {
		vkDestroyImageView(m_device.device, texture.imageView, nullptr);
		vmaDestroyImage(m_allocator, texture.image, texture.allocation);
	}

	vmaDestroyBuffer(m_allocator, m_vertexBuffer.buffer, m_vertexBuffer.allocation);
	vmaDestroyBuffer(m_allocator, m_indexBuffer.buffer, m_indexBuffer.allocation);
	vmaDestroyBuffer(m_allocator, m_globalsBuffer.buffer, m_globalsBuffer.allocation);
	vmaDestroyBuffer(m_allocator, m_textureIndicesBuffer.buffer, m_textureIndicesBuffer.allocation);

	vkDestroyFence(m_device.device, m_immFence, nullptr);

	for (const auto& frame : m_frames) {

		for (const auto& cubeShadowMap : frame.cubeShadowMaps) {
			vkDestroyImageView(m_device.device, cubeShadowMap.colorImage.imageView, nullptr);
			vmaDestroyImage(m_allocator, cubeShadowMap.colorImage.image, cubeShadowMap.colorImage.allocation);

			vkDestroyImageView(m_device.device, cubeShadowMap.depthImage.imageView, nullptr);
			vmaDestroyImage(m_allocator, cubeShadowMap.depthImage.image, cubeShadowMap.depthImage.allocation);
		}

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

	vkDestroyPipeline(m_device.device, m_bezierPipeline, nullptr);
	vkDestroyPipeline(m_device.device, m_lightShadowPipeline, nullptr);
	vkDestroyPipeline(m_device.device, m_lightCubeShadowPipeline, nullptr);
	vkDestroyPipeline(m_device.device, m_lightCubeShadowPCSSPipeline, nullptr);
	vkDestroyPipeline(m_device.device, m_lightPipeline, nullptr);
	vkDestroyPipeline(m_device.device, m_shadowPipeline, nullptr);
	vkDestroyPipeline(m_device.device, m_cubeShadowPipeline, nullptr);
	vkDestroyPipeline(m_device.device, m_skyboxPipeline, nullptr);
	vkDestroyPipeline(m_device.device, m_reflectPipeline, nullptr);
	vkDestroyPipeline(m_device.device, m_refractPipeline, nullptr);

	vkDestroyPipeline(m_device.device, m_lightAlphaPipeline, nullptr);
	vkDestroyPipeline(m_device.device, m_lightShadowAlphaPipeline, nullptr);
	vkDestroyPipeline(m_device.device, m_lightCubeShadowAlphaPipeline, nullptr);
	vkDestroyPipeline(m_device.device, m_lightCubeShadowPCSSAlphaPipeline, nullptr);

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