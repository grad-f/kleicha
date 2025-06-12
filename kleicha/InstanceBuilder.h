#ifndef INSTANCEBUILDER_H
#define INSTANCEBUILDER_H
#include <vector>
#include "Types.h"

class InstanceBuilder {
public:
	vkt::Instance build();
	InstanceBuilder& add_layers(const std::vector<const char*>& layers) {
		m_layers = layers;
		return *this;
	}

	InstanceBuilder& add_extensions(const std::vector<const char*>& extensions) {
		m_extensions = extensions;
		return *this;
	}

	InstanceBuilder& use_validation_layer() {
		m_useValidationLayer = true;
		return *this;
	}

private:
	std::vector<const char*> m_layers{};
	std::vector<const char*> m_extensions{};
	bool m_useValidationLayer{ false };

	void check_layers_support() const;
	void add_glfw_instance_exts();
	void check_extensions_support() const;
};

#endif // !INSTANCEBUILDER_H
