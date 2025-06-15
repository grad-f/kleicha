#ifndef DEVICE_H
#define DEVICE_H

#include "Utils.h"
#include "Types.h"
#include <vector>

class DeviceBuilder {
public:
	VkDevice build(VkInstance instance);
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

	bool are_extensions_supported(VkPhysicalDevice device) const;
	VkPhysicalDevice select_physical_device(VkInstance instance) const;
};

VkDevice DeviceBuilder::build(VkInstance instance) {

	[[maybe_unused]]VkPhysicalDevice physicalDevice{ select_physical_device(instance) };

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
		if (deviceProperties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			// check device extension support
			if (!are_extensions_supported(device))
				continue;

			// check device feature support


			chosenDevice = device;
			break;
		}
	}
	if (chosenDevice == nullptr)
		throw std::runtime_error{ "Failed to find a compatible physical device." };

	return chosenDevice;
}
#endif
