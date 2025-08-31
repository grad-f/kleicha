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
		glm::mat4 perspectiveProjection{};
		uint32_t drawId{};
		uint32_t lightId{};
		alignas(16)glm::vec3 viewWorldPos{};
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
		alignas(16)glm::vec3 position{};
		alignas(16)glm::vec2 UV{};
		alignas(16)glm::vec3 normal{};
		alignas(16)glm::vec3 tangent{};
		alignas(16)glm::vec3 bitangent{};
		bool operator==(const Vertex& other) const {
			return position == other.position && UV == other.UV && normal == other.normal;
		}
	};

	enum class MeshType {
		PYRAMID,
		SPHERE,
		TORUS,
		SHUTTLE,
		ICOSPHERE,
		DOLPHIN,
		PLANE,
		CUBE,
		SPONZA,
	};

	enum class MaterialType {
		NONE,
		GOLD,
		JADE,
		PEARL,
		SILVER
	};

	enum class TextureType {
		NONE,
		BRICK,
		EARTH,
		CONCRETE,
		SHUTTLE,
		DOLPHIN,
		SKYBOX_NIGHT,
		SKYBOX_DAY,
	};



	struct DrawRequest {
		vkt::MeshType meshType{};
		vkt::MaterialType materialType{};
		vkt::TextureType textureType{};
		bool isLight{ false };
		bool isSkybox{ false };
		bool isReflective{ false };
		bool isRefractive{ false };
	};

	struct DrawData {
		uint32_t materialIndex{};
		uint32_t textureIndex{};
		uint32_t transformIndex{};
	};

	struct HostDrawData {
		uint32_t drawId{};
		uint32_t transformIndex{};
		uint32_t indicesCount{};
		uint32_t indicesOffset{};
		int32_t vertexOffset{};
	};

	// this will be used to build the host draw data.
	struct GPUMesh {
		vkt::MeshType meshType{};
		uint32_t indicesCount{};
		// an offset in the index buffer in which this meshes index data begins
		uint32_t indicesOffset{};
		// an offset in the vertex buffer in which this meshes vertex data begins
		int32_t vertexOffset{};
	};

	struct Mesh {
		MeshType meshType{};
		// triangle indices
		std::vector<glm::uvec3> tInd{};
		std::vector<Vertex> verts{};
	};

	struct GlobalData {
		glm::vec4 ambientLight{};
		// Bias matrix maps NDC to texture space [0,1]
		glm::mat4 bias{};
		alignas(16) uint32_t lightCount{};
	};

	struct Transform {
		glm::mat4 model{};
		glm::mat4 modelView{};
		glm::mat4 modelViewInvTr{};
	};

	struct Light {
		glm::vec4 ambient{};
		glm::vec4 diffuse{};
		glm::vec4 specular{};
		float lightSize{};
		alignas(16)glm::vec3 attenuationFactors{};
		float frustumWidth{};
		alignas(16)glm::vec3 mPos{};
		alignas(16)glm::vec3 mvPos{};
		alignas(16)glm::mat4 viewProj{};
		alignas(16)glm::mat4 cubeViewProjs[6]{};
	};

	struct Material {
		glm::vec4 ambient{};
		glm::vec4 diffuse{};
		glm::vec4 specular{};
		// controls specular contribution fall-off
		alignas(16)float shininess{};

		// surface material helpers
		static Material none() {
			return {
				.ambient = {0.05f, 0.05f, 0.05f, 1.0f},
				.diffuse = {0.2f, 0.2f, 0.2f, 1.0f},
				.specular = {1.0f, 1.0f, 1.0f, 1.0f},
				.shininess = 51.2f
			};
		}
		
		static Material gold() {
			return {
				.ambient = {0.050f, 0.033f, 0.007f, 1.0f},
				.diffuse = {0.525f, 0.326f, 0.042f, 1.0f},
				.specular = {0.353f, 0.269f, 0.110f, 1.0f},
				.shininess = 51.2f
			};
		}

		static Material jade() {
			return {
				.ambient = {0.016f, 0.041f, 0.021f, .95f},
				.diffuse = {0.253f, 0.768f, 0.355f, .95f},
				.specular = {0.082f, 0.082f, 0.082f, .95f},
				.shininess = 12.8f
			};
		}

		static Material pearl() {
			return {
				.ambient = {0.051f, 0.035f, 0.035f, .922f},
				.diffuse = {1.0f, 0.654f, 0.654f, .922f},
				.specular = {0.072f, 0.072f, 0.072f, .922f},
				.shininess = 11.264f
			};
		}

		static Material silver() {
			return {
				.ambient = {0.031f, 0.031f, 0.031f, 1.0f},
				.diffuse = {0.221f, 0.221f, 0.221f, 1.0f},
				.specular = {0.222f, 0.222f, 0.222f, 1.0f},
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