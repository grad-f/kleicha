#ifndef PIPELINEBUILDER
#define PIPELINEBUILDER

#include <vector>
#include <vulkan/vulkan.h>

#include "Utils.h"

// pipeline builder will alter its internal structures through the provided public interface
class PipelineBuilder {
public:
	PipelineBuilder(VkDevice device)
		: m_device{device}
	{
		reset();
	}

	// aggregates the descriptors and push constants to be used with the pipeline being built
	VkPipelineLayout pipelineLayout{};
	void reset();
	VkPipeline build();

	PipelineBuilder& set_shaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
	PipelineBuilder& set_rasterizer_state(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace);
	PipelineBuilder& set_depth_stencil_state(VkBool32 depthTestEnable);
	PipelineBuilder& set_color_attachment_format(VkFormat format);
	PipelineBuilder& set_depth_attachment_format(VkFormat format);

	PipelineBuilder& disable_color_output() {
		m_color_output_disabled = true;

		return *this;
	}
private:
	std::vector<VkPipelineShaderStageCreateInfo>	m_shaderInfos{};
	VkPipelineVertexInputStateCreateInfo			m_vertInputInfo{};
	VkPipelineInputAssemblyStateCreateInfo			m_inputAssemblyInfo{};
	VkPipelineRasterizationStateCreateInfo			m_rasterizationInfo{};
	VkPipelineMultisampleStateCreateInfo			m_multisampleInfo{};
	VkPipelineDepthStencilStateCreateInfo			m_depthStencilInfo{};
	VkPipelineColorBlendStateCreateInfo				m_colorBlendInfo{};
	// viewport and scissor will be dynamic
	VkPipelineDynamicStateCreateInfo				m_dynamicStateInfo{};

	bool											m_color_output_disabled{ false };

	// description of attachments to be used by the pipeline
	VkPipelineRenderingCreateInfo m_renderingInfo{};
	VkFormat m_colorAttachmentFormat{};
	VkFormat m_depthAttachmentFormat{};

	VkDevice m_device{};
};
#endif // !PIPELINEBUILDER
