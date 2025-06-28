#ifndef KLEICHA_H
#define KLEICHA_H
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vulkan/vulkan.h"
#include "Types.h"
#include "vk_mem_alloc.h"

constexpr uint32_t MAX_FRAMES_IN_FLIGHT{ 2 };
constexpr VkFormat INTERMEDIATE_IMAGE_FORMAT{ VK_FORMAT_R16G16B16A16_SFLOAT };

class Kleicha {
public:
	GLFWwindow* m_window{};

	void init();
	void start();
	void cleanup() const;

	vkt::Frame get_current_frame() const	{ 
		return m_frames[m_framesRendered % 2];
	}
	uint32_t m_framesRendered{};
	void set_window_extent(VkExtent2D extent) {
		m_windowExtent = extent;
	}
private:
	VkSurfaceKHR m_surface{};
	VkExtent2D m_windowExtent{ 1600,900 };
	vkt::Instance m_instance{};
	vkt::Device m_device{};
	vkt::Swapchain m_swapchain{};
	VkCommandPool m_commandPool{};
	VkCommandBuffer m_immCmdBuffer{};
	VkFence m_immFence{};
	VkPipelineLayout m_dummyPipelineLayout{};
	VkPipeline m_graphicsPipeline{};
	VmaAllocator m_allocator{};

	vkt::Frame m_frames[MAX_FRAMES_IN_FLIGHT]{};
	std::vector<VkSemaphore> m_renderedSemaphores{};

	vkt::GPUMeshAllocation m_cubeAllocation{};

	void init_vulkan();
	void init_swapchain();
	void init_command_buffers();
	void init_sync_primitives();
	void init_graphics_pipelines();
	void init_vma();
	void init_intermediate_images();

	void init_meshes();

	vkt::GPUMeshAllocation upload_mesh_data(const vkt::IndexedMesh& mesh);

	void draw();
	void recreate_swapchain();
};

#endif // !KLEICHA_H
