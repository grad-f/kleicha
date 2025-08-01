#ifndef KLEICHA_H
#define KLEICHA_H
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <functional>
#include <stack>

#include "vulkan/vulkan.h"
#include "Types.h"
#include "vk_mem_alloc.h"
#include "Camera.h"

constexpr uint32_t MAX_FRAMES_IN_FLIGHT{ 2 };
constexpr VkFormat INTERMEDIATE_IMAGE_FORMAT{ VK_FORMAT_R16G16B16A16_SFLOAT };
constexpr VkFormat DEPTH_IMAGE_FORMAT{ VK_FORMAT_D32_SFLOAT };
constexpr VkExtent2D INIT_WINDOW_EXTENT{ .width = 1600, .height = 900 };

class Kleicha {
public:
	GLFWwindow* m_window{};
	Camera m_camera{ glm::vec3{0.0f, 0.0f, 4.0f}, INIT_WINDOW_EXTENT };

	void init();
	void start();
	void cleanup() const;

private:
	VkSurfaceKHR m_surface{};
	VkExtent2D m_windowExtent{ INIT_WINDOW_EXTENT };
	vkt::Instance m_instance{};
	vkt::Device m_device{};
	vkt::Swapchain m_swapchain{};
	VkCommandPool m_commandPool{};
	VkCommandBuffer m_immCmdBuffer{};
	VkFence m_immFence{};
	VkPipelineLayout m_dummyPipelineLayout{};
	VkPipeline m_graphicsPipeline{};
	VmaAllocator m_allocator{};

	//global descriptor resources
	VkDescriptorSetLayout m_globDescSetLayout;
	VkDescriptorPool m_descPool{};
	VkDescriptorPool m_imguiDescPool{};
	VkDescriptorSet m_globalDescSet{};
	// per frame descriptor resources
	VkDescriptorSetLayout m_frameDescSetLayout{};

	vkt::Frame m_frames[MAX_FRAMES_IN_FLIGHT]{};
	vkt::Image rasterImage{};
	vkt::Image depthImage{};
	std::vector<VkSemaphore> m_renderedSemaphores{};

	VkSampler m_textureSampler{};
	std::vector<vkt::Image> m_textures{};

	vkt::Buffer m_vertexBuffer{};
	vkt::Buffer m_indexBuffer{};
	// this buffer specifies indicies and offsets to the other buffers available in the shader
	vkt::Buffer m_drawBuffer{};
	vkt::Buffer m_globalsBuffer{};

	std::vector<vkt::MeshDrawData> m_meshDrawData{};
	std::vector<vkt::Transform> m_meshTransforms{};
	std::vector<vkt::Material> m_materials{};
	std::vector<vkt::Light> m_lights{};

	glm::mat4 m_perspProj{ utils::orthographicProj(glm::radians(60.0f),
		static_cast<float>(m_windowExtent.width) / m_windowExtent.height, 1000.0f, 0.1f) * utils::perspective(1000.0f, 0.1f) };

	void init_vulkan();
	void init_swapchain();
	void init_command_buffers();
	void init_sync_primitives();
	void init_graphics_pipelines();
	void init_descriptors();
	void init_vma();
	void init_imgui();
	void init_draw_data();
	void init_lights();
	void init_materials();
	void init_image_buffers();
	void init_dynamic_buffers();
	void init_write_descriptor_sets();
	
	vkt::DrawData create_draw(const std::vector<vkt::MeshDrawData>& canonicalMeshes, vkt::MeshType meshType, vkt::MaterialType materialType, vkt::TextureType textureType);
	std::vector<vkt::MeshDrawData> load_mesh_data();

	vkt::PushConstants m_pushConstants{};
	std::stack<glm::mat4> m_mStack{};

	vkt::Image upload_texture_image(const char* filePath);
	vkt::Buffer upload_data(void* data, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage, VkBool32 bdaUsage = VK_FALSE);

	void draw(float currentTime);
	void draw_imgui(VkCommandBuffer frameCmdBuffer, VkImageView swapchainImage) const;
	void recreate_swapchain();
	void deallocate_frame_images() const;
	// must be r-value reference as we'll be supplying lambdas
	void immediate_submit(std::function<void(VkCommandBuffer cmdBuffer)>&& func) const;
	void process_inputs();

	uint32_t m_framesRendered{};
	const vkt::Frame& get_current_frame() const {
		return m_frames[m_framesRendered % 2];
	}
	void set_window_extent(VkExtent2D extent) {
		m_windowExtent = extent;
	}

	float m_deltaTime{};
	float m_lastFrame{};
};

#endif // !KLEICHA_H
