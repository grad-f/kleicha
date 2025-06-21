#ifndef KLEICHA_H
#define KLEICHA_H
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vulkan/vulkan.h"
#include "Types.h"

constexpr uint32_t MAX_FRAMES_IN_FLIGHT{ 2 };

class Kleicha {
public:
	void init();

	// initial cleanup procedure
	void cleanup() const;
private:
	GLFWwindow* m_window{};
	VkSurfaceKHR m_surface{};
	VkExtent2D m_windowExtent{ 1600,900 };
	vkt::Instance m_instance{};
	vkt::Device m_device{};
	vkt::Swapchain m_swapchain{};
	VkCommandPool m_commandPool{};
	VkCommandBuffer m_immCmdBuffer{};
	VkPipelineLayout m_dummyPipelineLayout{};
	VkPipeline m_graphicsPipeline{};

	vkt::Frame m_frames[MAX_FRAMES_IN_FLIGHT]{};

	void init_vulkan();
	void init_swapchain();
	void init_command_buffers();
	void init_sync_primitives();
	void init_graphics_pipelines();
};

#endif // !KLEICHA_H
