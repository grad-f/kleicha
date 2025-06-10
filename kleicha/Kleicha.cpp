#include "Kleicha.h"
#include "InstanceBuilder.h"
#include "vulkan/vulkan.h"
#include <iostream>


// init calls the required functions to initialize vulkan
void Kleicha::init() const {
	init_create_instance();
}

// creates a vulkan instance
void Kleicha::init_create_instance() const {
	InstanceBuilder instanceBuilder{};
	std::vector<const char*> layers{};
	instanceBuilder.add_layers(layers);
	instanceBuilder.use_validation_layer();
	instanceBuilder.build_instance();
}