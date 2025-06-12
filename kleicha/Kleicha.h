#ifndef KLEICHA_H
#define KLEICHA_H
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vulkan/vulkan.h"
#include "Types.h"

class Kleicha {
public:
	void init();

	// initial cleanup procedure
	void cleanup() const;
private:
	GLFWwindow* m_window{};
	VkExtent2D m_windowExtent{ 1600,900 };
	vkt::Instance m_instance{};
	void init_glfw();
	void init_create_instance();

};

#endif // !KLEICHA_H
