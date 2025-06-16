#ifndef DEVICEBUILDER_H
#define DEVICEBUILDER_H

#include "Utils.h"
#include "Types.h"
#include <vector>
#include <optional>

class DeviceBuilder {
public:

	DeviceBuilder(VkInstance instance, VkSurfaceKHR surface)
		: m_instance{ instance }, m_surface{ surface } {}

	VkDevice build();
	DeviceBuilder& request_extensions(std::vector<const char*>& extensions) {
		m_extensions = extensions;
		return *this;
	}

	DeviceBuilder& request_features(const vkt::DeviceFeatures& deviceFeatures) {
		m_requestedFeatures = deviceFeatures;
		return *this;
	}
private:
	std::vector<const char*> m_extensions{};
	vkt::DeviceFeatures m_requestedFeatures{};
	VkInstance m_instance{};
	VkSurfaceKHR m_surface{};
	bool are_extensions_supported(VkPhysicalDevice device) const;
	bool are_features_supported(VkPhysicalDevice device) const;
	std::optional<uint32_t> get_queue_family(VkPhysicalDevice device) const;
	std::optional<vkt::SurfaceSupportDetails> get_surface_support_details(VkPhysicalDevice device) const;
	bool check_features_struct(const VkBool32* p_reqFeaturesStart, const VkBool32* p_reqFeaturesEnd, const VkBool32* p_DeviceFeaturesStart) const;
	VkPhysicalDevice select_physical_device(VkInstance instance) const;
};

VkDevice DeviceBuilder::build() {

	[[maybe_unused]]VkPhysicalDevice physicalDevice{ select_physical_device(m_instance) };

	return VK_NULL_HANDLE;
}

// checks if the device supports the requested extensions
bool DeviceBuilder::are_extensions_supported(VkPhysicalDevice device) const {

	if (m_extensions.size() == 0)
		return true;

	uint32_t extensionsCount{};
	VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, nullptr));
	std::vector<VkExtensionProperties> extensionProperties(extensionsCount);
	VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, extensionProperties.data()));

	for (const auto& requestedExtension : m_extensions) {
		bool extensionFound{ false };
		for (const auto& extensionProperty : extensionProperties) {
			if (strcmp(requestedExtension, extensionProperty.extensionName) == 0) {
				extensionFound = true;
				break; 
			}
		}
		if (!extensionFound)
			return false;
	}
	return true;
}

/* Traverses the features object in memory to avoid checking individual fields of the struct.
This is fine because VkBool32 is a typdef of uint32_t that provides memory width guarantee.Only an issue on 32 bit.*/
bool DeviceBuilder::check_features_struct(const VkBool32* p_reqFeaturesStart, const VkBool32* p_reqFeaturesEnd, const VkBool32* p_DeviceFeaturesStart) const {
	uint32_t offset{ 0 }; // this is used to index the candidate physical device features field at its corresponding address

	// traverse the requested features object memory 4 bytes at a time
	for (const VkBool32* i{ p_reqFeaturesStart }; i <= p_reqFeaturesEnd; ++i) {
		// if requested feature is set at the field, check if is supported in the candidate physical device.
		if (*i && !p_DeviceFeaturesStart[offset]) {
			return false;
		}
		++offset;
	}
	return true;
}

bool DeviceBuilder::are_features_supported(VkPhysicalDevice device) const {
	vkt::DeviceFeatures deviceFeatures{};
	vkGetPhysicalDeviceFeatures2(device, &deviceFeatures.VkFeatures);

	if (!deviceFeatures.VkFeatures.features.geometryShader || !deviceFeatures.VkFeatures.features.tessellationShader)
		return false;
	if (!check_features_struct(&m_requestedFeatures.Vk11Features.storageBuffer16BitAccess, &m_requestedFeatures.Vk11Features.shaderDrawParameters, &deviceFeatures.Vk11Features.storageBuffer16BitAccess))
		return false;

	if (!check_features_struct(&m_requestedFeatures.Vk12Features.samplerMirrorClampToEdge, &m_requestedFeatures.Vk12Features.subgroupBroadcastDynamicId, &deviceFeatures.Vk12Features.samplerMirrorClampToEdge))
		return false;

	if (!check_features_struct(&m_requestedFeatures.Vk13Features.robustImageAccess, &m_requestedFeatures.Vk13Features.maintenance4, &deviceFeatures.Vk13Features.robustImageAccess))
		return false;

	if (!check_features_struct(&m_requestedFeatures.Vk14Features.globalPriorityQuery, &m_requestedFeatures.Vk14Features.pushDescriptor, &deviceFeatures.Vk14Features.globalPriorityQuery))
		return false;

	return true;
}

std::optional<uint32_t> DeviceBuilder::get_queue_family(VkPhysicalDevice device) const {
	uint32_t queueFamilyCount{};
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
	// queue family properties will be copied into our vector in their respective queue family index order
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProperties.data());

	// we're going to avoid using separate queue families for graphics, transfer, and presentation. Compute support is guaranteed if graphics commands are supported
	for (uint32_t i{ 0 }; i < queueFamilyProperties.size(); ++i) {
		if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
			// check for presentation support
			VkBool32 presentSupported{ VK_FALSE };
			VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupported));
			if (presentSupported)
				return i;
		}
	}
	return {};
}

std::optional<vkt::SurfaceSupportDetails> DeviceBuilder::get_surface_support_details(VkPhysicalDevice device) const {

	VkSurfaceCapabilitiesKHR surfaceCapabilities{};
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &surfaceCapabilities));

	uint32_t surfaceFormatCount{ 0 };
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &surfaceFormatCount, nullptr));
	//early return if no formats found
	if(surfaceFormatCount == 0)
		return {};

	// get supported present formats between physical device and surface
	std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &surfaceFormatCount, surfaceFormats.data()));

	// get supported present modes between physical device and surface
	uint32_t presentModeCount{ 0 };
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr));
	//early return if no formats found
	if (presentModeCount == 0)
		return {};
	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, presentModes.data()));

	return vkt::SurfaceSupportDetails{ .capabilities = surfaceCapabilities, .formats = surfaceFormats, .presentModes = presentModes };
}

VkPhysicalDevice DeviceBuilder::select_physical_device(VkInstance instance) const {

	// get all physical devices supported by the implementation
	uint32_t deviceCount{};
	VK_CHECK(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
	if (deviceCount == 0)
		throw std::runtime_error("[PhysicalDeviceSelector] Failed to find any physical devices.");

	fmt::println("[PhysicalDeviceSelector] Found {0} physical device(s).", deviceCount);
	std::vector<VkPhysicalDevice> devices(deviceCount);
	VK_CHECK(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()));

	VkPhysicalDevice chosenDevice{};
	// traverse physical devices and find one that is discrete and supports the requested extensions and features
	for (const auto& device : devices) {
		// get device properties
		VkPhysicalDeviceProperties2 deviceProperties{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
		vkGetPhysicalDeviceProperties2(device, &deviceProperties);

		// attempt to get a queue family index that supports graphics, compute, transfer, and presentation
		std::optional<uint32_t> queueFamilyIndex{ get_queue_family(device) };
		// attempt to get physical device surface support details
		std::optional<vkt::SurfaceSupportDetails> surfaceSupportDetails{ get_surface_support_details(device) };

		if (deviceProperties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			// check for graphics, transfer, compute, and presentation queue family support
			if (!queueFamilyIndex.has_value())
				continue;

			// check for surface support
			if (!surfaceSupportDetails.has_value())
				continue;

			// check device extension support
			if (!are_extensions_supported(device))
				continue;

			// check device feature support
			if (!are_features_supported(device))
				continue;

			// device passed all checks, encapsulate all data and return to caller as this is the device we'll be using

			chosenDevice = device;
			break;
		}
	}
	if (chosenDevice == nullptr)
		throw std::runtime_error{ "[DeviceBuilder] Failed to find a compatible physical device." };

	return chosenDevice;
}
#endif
