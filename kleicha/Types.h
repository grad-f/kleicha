#ifndef TYPES_H
#define TYPES_H

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <ext/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <gtx/hash.hpp>
#include <vector>
#include "vk_mem_alloc.h"
#include "vulkan/vulkan.h"


namespace vkt {

	struct PushConstants {
		VkDeviceAddress vertexBufferAddress{};
		glm::mat4 perspectiveProjection{};
		glm::mat4 modelView{};
		glm::mat4 mvInvTr{};
		int texID{};
	};

	struct Instance {
		VkInstance instance{};
		VkDebugUtilsMessengerEXT debugMessenger{};
		PFN_vkCreateDebugUtilsMessengerEXT pfnCreateMessenger{};
		PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroyMessenger{};
	};

	// stores the surface support for our chosen physical device
	struct SurfaceSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities{};
		std::vector<VkSurfaceFormatKHR> formats{};
		std::vector<VkPresentModeKHR> presentModes{};
	};

	struct PhysicalDevice {
		VkPhysicalDevice device{};
		VkPhysicalDeviceProperties2 deviceProperties{};
		// supports graphics, compute, transfer, and presentation
		uint32_t queueFamilyIndex{};
		vkt::SurfaceSupportDetails surfaceSupportDetails{};
	};

	struct Device {
		PhysicalDevice physicalDevice{};
		VkDevice device{};
		VkQueue queue{};
	};

	struct Swapchain {
		VkSwapchainKHR swapchain{};
		std::vector<VkImage> images{};
		std::size_t imageCount{};
		std::vector<VkImageView> imageViews{};
		VkExtent2D imageExtent{};
	};

	struct Image {
		VkImage image{};
		VkImageView imageView{};
		VmaAllocation allocation{};
		VmaAllocationInfo allocationInfo{};
		uint32_t mipLevels{ 1 };
	};

	struct Buffer {
		VkBuffer buffer{};
		VmaAllocation allocation{};
		VmaAllocationInfo allocationInfo{};
		VkDeviceSize deviceAddress{};
	};

	struct GPUMeshAllocation {
		Buffer vertBuffer{};
		Buffer indexBuffer{};
		uint32_t indexCount{};
	};

	struct Vertex {
		glm::vec3 position{};
		glm::vec2 UV{};
		glm::vec3 normal{};
		glm::vec3 tangent{};
		glm::vec3 bitangent{};

		bool operator==(const Vertex& other) const {
			return position == other.position && UV == other.UV && normal == other.normal;
		}
	};

	struct IndexedMesh {
		// triangle indices
		std::vector<glm::uvec3> tInd{};
		std::vector<Vertex> verts{};

		VkDeviceSize getIndBufferSize() const { return tInd.size() * sizeof(glm::uvec3); }
		VkDeviceSize getvertsBufferSize() const { return verts.size() * sizeof(Vertex); }
		uint32_t getIndexCount() const { return static_cast<uint32_t>(tInd.size() * glm::uvec3::length()); }
	};

	struct Light {
		glm::vec4 ambient{};
		glm::vec4 diffuse{};
		glm::vec4 specular{};
		glm::vec3 position{};
	};

	struct Material {
		glm::vec4 ambient{};
		glm::vec4 diffuse{};
		glm::vec4 specular{};
		// controls specular contribution fall-off
		float shininess{};

		// surface material helpers
		static Material gold_material() {
			return {
				.ambient = {0.2473f, 0.1995f, 0.0745f, 1.0f},
				.diffuse = {0.7516f, 0.6065f, 0.2265f, 1.0f},
				.specular = {0.6283f, 0.5558f, 0.3661f, 1.0f},
				.shininess = 51.2f
			};
		}
	};

	// encapsulates data needed for each frame
	struct Frame {
		VkCommandBuffer cmdBuffer{};
		VkFence inFlightFence{};
		VkSemaphore acquiredSemaphore{};
		VkDescriptorSet descriptorSet{};
	};

	// chained and encapsulated device features struct
	struct DeviceFeatures {
		VkPhysicalDeviceFeatures2 VkFeatures{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &Vk11Features };
		VkPhysicalDeviceVulkan11Features Vk11Features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES, .pNext = &Vk12Features };
		VkPhysicalDeviceVulkan12Features Vk12Features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, .pNext = &Vk13Features };
		VkPhysicalDeviceVulkan13Features Vk13Features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES, .pNext = &Vk14Features };
		VkPhysicalDeviceVulkan14Features Vk14Features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES };
	};
}
#endif // !TYPES_H