#ifndef DEVICE_H
#define DEVICE_H

#include "Utils.h"
#include "Types.h"
#include <vector>

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
	bool are_features_supported(VkPhysicalDevice device);
	bool check_features_struct(VkBool32* p_reqFeaturesStart, VkBool32* p_reqFeaturesEnd, VkBool32* p_DeviceFeaturesStart);
	VkPhysicalDevice select_physical_device(VkInstance instance);
};

VkDevice DeviceBuilder::build() {

	[[maybe_unused]]VkPhysicalDevice physicalDevice{ select_physical_device(m_instance) };

	return VK_NULL_HANDLE;
}

// checks if the device supports the requested extensions
bool DeviceBuilder::are_extensions_supported(VkPhysicalDevice device) const {

	if (m_extensions.size() <= 0)
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

bool DeviceBuilder::check_features_struct(VkBool32* p_reqFeaturesStart, VkBool32* p_reqFeaturesEnd, VkBool32* p_DeviceFeaturesStart) {
	uint32_t offset{ 0 };
	for (const VkBool32* i{ p_reqFeaturesStart }; i <= p_reqFeaturesEnd; ++i) {
		if (*i && !p_DeviceFeaturesStart[offset]) {
			return false;
		}
		++offset;
	}
	return true;
}

// This is fine because VkBool32 is a typdef of uint32_t that provides memory width guarantee. Only an issue on 32 bit.
bool DeviceBuilder::are_features_supported(VkPhysicalDevice device) {
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

VkPhysicalDevice DeviceBuilder::select_physical_device(VkInstance instance) {

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

		if (deviceProperties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			// check device extension support
			if (!are_extensions_supported(device))
				continue;

			// check device feature support
			if (!are_features_supported(device))
				continue;

			chosenDevice = device;
			break;
		}
	}
	if (chosenDevice == nullptr)
		throw std::runtime_error{ "Failed to find a compatible physical device." };

	return chosenDevice;
}
#endif
