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
	PipelineBuilder& set_input_assembly_state(VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	PipelineBuilder& set_shaders(VkShaderModule* vertexShader, VkSpecializationInfo* vertSpecializationInfo = nullptr, VkShaderModule* fragmentShader = nullptr, VkSpecializationInfo* fragSpecializationInfo = nullptr, VkShaderModule* tessControlShader = nullptr, VkShaderModule* tessEvalShader = nullptr);
	PipelineBuilder& set_rasterizer_state(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace, float depthBiasConstant = 0.0f, float depthBiasSlope = 0.0f);
	PipelineBuilder& set_color_blend_state(VkColorComponentFlags colorComponentFlags, VkBool32 blendEnable = false);


	PipelineBuilder& set_depth_stencil_state(VkBool32 depthTestEnable, VkCompareOp depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL);
	PipelineBuilder& set_color_attachment_format(VkFormat format);
	PipelineBuilder& set_depth_attachment_format(VkFormat format);


	PipelineBuilder& enable_color_output() {
		m_color_output_disabled = false;
		return *this;
	}

	PipelineBuilder& disable_color_output() {
		m_color_output_disabled = true;
		return *this;
	}

	PipelineBuilder& set_view_mask(uint32_t viewMask) {
		m_renderingInfo.viewMask = viewMask;
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
	
	VkPipelineColorBlendAttachmentState				m_colorBlendAttachmentState{};
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
