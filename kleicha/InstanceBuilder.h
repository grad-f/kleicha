#ifndef INSTANCEBUILDER_H
#define INSTANCEBUILDER_H
#include <vector>
#include "vulkan/vulkan.h"

class InstanceBuilder {
public:
	VkInstance build_instance();

	void add_layers(const std::vector<const char*>& layers) {
		m_layers = layers;
	}

	void add_extensions(const std::vector<const char*>& extensions) {
		m_extensions = extensions;
	}

	void use_validation_layer() {
		m_useValidationLayer = true;
	}

private:
	std::vector<const char*> m_layers{};
	std::vector<const char*> m_extensions{};
	bool m_useValidationLayer{ false };


	void check_layers_supported() const;
};

#endif // !INSTANCEBUILDER_H
