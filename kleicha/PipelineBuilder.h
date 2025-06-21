#ifndef PIPELINEBUILDER
#define PIPELINEBUILDER

#include <vector>
#include <vulkan/vulkan.h>

#include "Utils.h"

// pipeline builder will alter its internal structures through the provided public interface
class PipelineBuilder {
public:
	PipelineBuilder() {
		reset();
	}

	// aggregates the descriptors and push constants to be used with the pipeline being built
	VkPipelineLayout pipelineLayout{};
	void reset();
	VkPipeline build(VkDevice device);

	PipelineBuilder& set_shaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
	PipelineBuilder& set_rasterizer_state(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace);
	PipelineBuilder& set_depth_stencil_state(VkBool32 depthTestEnable);
	PipelineBuilder& set_color_attachment_format(VkFormat format);
	PipelineBuilder& set_depth_attachment_format(VkFormat format);

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

	// description of attachments to be used by the pipeline
	VkPipelineRenderingCreateInfo m_renderingInfo{};
	VkFormat m_colorAttachmentFormat{};
	VkFormat m_depthAttachmentFormat{};
};

void PipelineBuilder::reset() {
	// deallocates
	m_shaderInfos.clear();
	m_vertInputInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	m_inputAssemblyInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	m_rasterizationInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	m_multisampleInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	m_depthStencilInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	m_colorBlendInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	m_dynamicStateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	m_renderingInfo = { .sType = VK_STRUCTURE_TYPE_RENDERING_INFO };
}

PipelineBuilder& PipelineBuilder::set_shaders(VkShaderModule vertexShader, VkShaderModule fragmentShader) {
	VkPipelineShaderStageCreateInfo shaderInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	shaderInfo.pNext = nullptr;
	shaderInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderInfo.module = vertexShader;
	shaderInfo.pName = "main()";

	m_shaderInfos.emplace_back(shaderInfo);

	shaderInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderInfo.module = fragmentShader;

	m_shaderInfos.emplace_back(shaderInfo);
}

PipelineBuilder& PipelineBuilder::set_rasterizer_state(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace) {
	m_rasterizationInfo.pNext = nullptr;
	m_rasterizationInfo.depthClampEnable = VK_FALSE;
	m_rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
	m_rasterizationInfo.polygonMode = polygonMode;
	m_rasterizationInfo.cullMode = cullMode;
	m_rasterizationInfo.frontFace = frontFace;
	m_rasterizationInfo.depthBiasEnable = VK_FALSE;
	// specifies the number of pixels covered by a line per column.
	m_rasterizationInfo.lineWidth = 1.0f;
	return *this;
}

// TO-DO: We will eventually switch to reverse depth, reflect that here
PipelineBuilder& PipelineBuilder::set_depth_stencil_state(VkBool32 depthTestEnable) {
	m_depthStencilInfo.depthTestEnable = depthTestEnable;
	m_depthStencilInfo.depthWriteEnable = VK_TRUE;
	m_depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	// sample depth should fall between [0,1] NDC, if it doesn't it shouldn't be reflected in the fragment's coverage mask.
	m_depthStencilInfo.depthBoundsTestEnable = VK_TRUE;
	m_depthStencilInfo.stencilTestEnable = VK_FALSE;
	m_depthStencilInfo.minDepthBounds = 0.0f;
	m_depthStencilInfo.maxDepthBounds = 1.0f;
	return *this;
}

PipelineBuilder& PipelineBuilder::set_color_attachment_format(VkFormat format) {
	m_colorAttachmentFormat = format;
	m_renderingInfo.colorAttachmentCount = 1;
	m_renderingInfo.pColorAttachmentFormats = &m_colorAttachmentFormat;
	return *this;
}

PipelineBuilder& PipelineBuilder::set_depth_attachment_format(VkFormat format) {
	m_depthAttachmentFormat = format;
	m_renderingInfo.depthAttachmentFormat = m_depthAttachmentFormat;
	return *this;
}

VkPipeline PipelineBuilder::build(VkDevice device) {
	VkGraphicsPipelineCreateInfo pipelineInfo{ .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineInfo.pNext = &m_renderingInfo;
	pipelineInfo.stageCount = static_cast<uint32_t>(m_shaderInfos.size());
	pipelineInfo.pStages = m_shaderInfos.data();
	pipelineInfo.pVertexInputState = &m_vertInputInfo; // we will be using buffer device address, no need to describe to the pipeline how to read the vertex attribute data.

	// set this always to triangle list topology for now
	m_inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
	m_inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	pipelineInfo.pInputAssemblyState = &m_inputAssemblyInfo;

	pipelineInfo.pRasterizationState = &m_rasterizationInfo;

	// use point sampling for now
	m_multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	m_multisampleInfo.sampleShadingEnable = VK_FALSE;
	m_multisampleInfo.alphaToCoverageEnable = VK_FALSE;

	pipelineInfo.pMultisampleState = &m_multisampleInfo;

	pipelineInfo.pDepthStencilState = &m_depthStencilInfo;

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState{ .blendEnable = VK_FALSE };

	m_colorBlendInfo.logicOpEnable = VK_TRUE;
	m_colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
	m_colorBlendInfo.attachmentCount = 1;
	m_colorBlendInfo.pAttachments = &colorBlendAttachmentState;

	pipelineInfo.pColorBlendState = &m_colorBlendInfo;
	
	VkPipeline pipeline{};
	VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));

	return pipeline;
}

#endif // !PIPELINEBUILDER
