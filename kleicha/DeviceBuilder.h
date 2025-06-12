#ifndef DEVICE_H
#define DEVICE_H

#include "Utils.h"
#include "Types.h"
#include <vector>

class DeviceBuilder {
public:
	VkDevice build(VkInstance instance);
private:
	VkPhysicalDevice select_physical_device(VkInstance instance) const;
};

VkDevice DeviceBuilder::build(VkInstance instance) {

	[[maybe_unused]]VkPhysicalDevice physicalDevice{ select_physical_device(instance) };

	return VK_NULL_HANDLE;
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
	// traverse physical devices and determine which is best
	for (const auto& device : devices) {
		// get device properties
		VkPhysicalDeviceProperties2 deviceProperties{};
		vkGetPhysicalDeviceProperties2(device, &deviceProperties);
		if (deviceProperties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			//TO-DO: Check if the physical device supports the requested device features

			chosenDevice = device;
			break;
		}
	}

	return chosenDevice;
}
#endif
