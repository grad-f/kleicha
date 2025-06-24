#ifndef DEVICEBUILDER_H
#define DEVICEBUILDER_H

#include "Types.h"
#include <vector>
#include <optional>

class DeviceBuilder {
public:

	DeviceBuilder(VkInstance instance, VkSurfaceKHR surface)
		: m_instance{ instance }, m_surface{ surface } {}

	vkt::Device build();
	DeviceBuilder& request_extensions(std::vector<const char*>& extensions) {
		m_extensions = extensions;
		return *this;
	}

	DeviceBuilder& request_features(const vkt::DeviceFeatures& deviceFeatures) {
		m_requestedFeatures = deviceFeatures;
		return *this;
	}

	std::optional<vkt::SurfaceSupportDetails> get_surface_support_details(VkPhysicalDevice device) const;
private:
	std::vector<const char*> m_extensions{};
	vkt::DeviceFeatures m_requestedFeatures{};
	VkInstance m_instance{};
	VkSurfaceKHR m_surface{};
	bool are_extensions_supported(VkPhysicalDevice device) const;
	bool are_features_supported(VkPhysicalDevice device) const;
	std::optional<uint32_t> get_queue_family(VkPhysicalDevice device) const;
	bool check_features_struct(const VkBool32* p_reqFeaturesStart, const VkBool32* p_reqFeaturesEnd, const VkBool32* p_DeviceFeaturesStart) const;
	vkt::PhysicalDevice select_physical_device(VkInstance instance) const;
};
#endif
