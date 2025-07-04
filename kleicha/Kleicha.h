#ifndef KLEICHA_H
#define KLEICHA_H
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <functional>

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

	vkt::Frame m_frames[MAX_FRAMES_IN_FLIGHT]{};
	std::vector<VkSemaphore> m_renderedSemaphores{};

	vkt::GPUMeshAllocation m_cubeAllocation{};

	void init_vulkan();
	void init_swapchain();
	void init_command_buffers();
	void init_sync_primitives();
	void init_graphics_pipelines();
	void init_vma();
	void init_image_buffers();
	void init_meshes();

	vkt::GPUMeshAllocation upload_mesh_data(const vkt::IndexedMesh& mesh);

	void draw();
	void recreate_swapchain();
	void deallocate_frame_images() const;
	// must be r-value reference as we'll be supplying lambdas
	void immediate_submit(std::function<void(VkCommandBuffer cmdBuffer)>&& func) const;
	void processInputs();

	uint32_t m_framesRendered{};
	vkt::Frame get_current_frame() const {
		return m_frames[m_framesRendered % 2];
	}
	void set_window_extent(VkExtent2D extent) {
		m_windowExtent = extent;
	}

	float m_deltaTime{};
	float m_lastFrame{};
};

#endif // !KLEICHA_H
