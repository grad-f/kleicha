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
#include<vulkan/vk_enum_string_helper.h>


namespace vkt {
	// need not have alignment of 16 bytes as we won't be creating arrays of this type.
	// TODO: Global constants should be updated per frame and much of what is currently provided via push constants can be moved.
	struct alignas(16) PushConstants {
		glm::mat4 m_m4ViewProjection{};
		uint32_t drawId{};
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
		VkSurfaceFormatKHR imageFormat{};
	};

	struct Image {
		VkImage image{};
		VkImageView imageView{};
		VmaAllocation allocation{};
		VmaAllocationInfo allocationInfo{};
		uint32_t mipLevels{ 1 };
	};

	struct CubeImage {
		Image colorImage{};
		Image depthImage{};
	};

	struct Buffer {
		VkBuffer buffer{};
		VmaAllocation allocation{};
		VmaAllocationInfo allocationInfo{};
		VkDeviceSize deviceAddress{};
	};

	struct Vertex {
		alignas(16)glm::vec3 m_v3Position{};
		alignas(16)glm::vec2 m_v2UV{};
		alignas(16)glm::vec3 m_v3Normal{};
		alignas(16)glm::vec4 m_v4Tangent{};
		alignas(16)glm::vec3 m_v3Bitangent{};
		bool operator==(const Vertex& other) const {
			return m_v3Position == other.m_v3Position && m_v2UV == other.m_v2UV && m_v3Normal == other.m_v3Normal;
		}
	};

	struct DrawData {
		uint32_t m_uiMaterialIndex{};
		uint32_t m_uiTransformIndex{};
	};

	struct HostDrawData {
		uint32_t m_uiDrawId{};
		uint32_t m_uiIndicesCount{};
		uint32_t m_uiIndicesOffset{};
		int32_t m_iVertexOffset{};
	};

	struct Mesh {
		// triangle indices
		std::vector<glm::uvec3> tInd{};
		std::vector<Vertex> verts{};
		bool bUseAlphaTest{ false };
		bool bComputeTangent{ false };
	};

	struct GlobalData {
		glm::vec3 m_v3CameraPosition{};
		uint32_t m_uiNumPointLights{};
		uint32_t m_uiUseEmissive{};
	};

	struct Transform {
		glm::mat4 m_m4Model{};
		glm::mat4 m_m4ModelInvTr{};
	};

	enum class TextureType {
		ALBEDO,
		NORMAL,
		SPECULAR,
		ROUGHNESS,
	};

	struct Texture {
		std::string path{};
		TextureType type{};
	};

	struct PointLight {
		glm::vec3 m_v3Position{};
		alignas(16)glm::vec3 m_v3Color{};
		alignas(16)glm::vec3 m_fFalloff{};
	};

	struct alignas(16) Material {
		glm::vec3 m_v3Diffuse{};
		alignas(16)glm::vec3 m_v3Specular{};
		// emissive factor
		float m_fEmissive{};
		// controls specular contribution fall-off
		float m_fRoughness{};
		uint32_t m_uiAlbedoTexture{};
		uint32_t m_uiNormalTexture{};
		uint32_t m_uiSpecularTexture{};
		uint32_t m_uiRoughnessTexture{};

		// surface material helpers
		static Material none() {
			return {
				.m_v3Diffuse = {1.0f, 0.0f, 0.0f},
				.m_v3Specular = {0.1f, 0.1f, 0.1f},
				.m_fRoughness = 0.35f
			};
		}
	};

	// encapsulates data needed for each frame
	struct Frame {
		VkCommandBuffer cmdBuffer{};
		VkFence inFlightFence{};
		VkSemaphore acquiredSemaphore{};
		VkDescriptorSet descriptorSet{};

		vkt::Buffer transformBuffer{};
		vkt::Buffer materialBuffer{};
		vkt::Buffer lightBuffer{};

		std::vector<vkt::Image> shadowMaps{};
		std::vector<vkt::CubeImage> cubeShadowMaps{};
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