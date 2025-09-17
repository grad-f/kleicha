#include "PipelineBuilder.h"

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
	m_renderingInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
}

PipelineBuilder& PipelineBuilder::set_shaders(VkShaderModule* vertexShader, VkSpecializationInfo* vertSpecializationInfo, VkShaderModule* fragmentShader, VkSpecializationInfo* fragSpecializationInfo, VkShaderModule* tessControlShader, VkShaderModule* tessEvalShader) {

	if (!m_shaderInfos.empty())
		m_shaderInfos.clear();

	VkPipelineShaderStageCreateInfo shaderInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	shaderInfo.pNext = nullptr;
	shaderInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderInfo.module = *vertexShader;
	shaderInfo.pName = "main";
	if (vertSpecializationInfo)
		shaderInfo.pSpecializationInfo = vertSpecializationInfo;
	m_shaderInfos.emplace_back(shaderInfo);

	
	
	if (fragmentShader) {
		shaderInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderInfo.module = *fragmentShader;
		if (fragSpecializationInfo)
			shaderInfo.pSpecializationInfo = fragSpecializationInfo;
		m_shaderInfos.emplace_back(shaderInfo);
	}

	if (tessControlShader) {
		shaderInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		shaderInfo.module = *tessControlShader;
		m_shaderInfos.emplace_back(shaderInfo);
	}

	if (tessControlShader) {
		shaderInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		shaderInfo.module = *tessEvalShader;
		m_shaderInfos.emplace_back(shaderInfo);
	}

	return *this;
}

PipelineBuilder& PipelineBuilder::set_rasterizer_state(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace, float depthBiasConstant, float depthBiasSlope) {
	m_rasterizationInfo.pNext = nullptr;
	m_rasterizationInfo.depthClampEnable = VK_FALSE;
	m_rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
	m_rasterizationInfo.polygonMode = polygonMode;
	m_rasterizationInfo.cullMode = cullMode;
	m_rasterizationInfo.frontFace = frontFace;
	m_rasterizationInfo.depthBiasEnable = depthBiasConstant || depthBiasSlope ? VK_TRUE : VK_FALSE;
	m_rasterizationInfo.depthBiasConstantFactor = depthBiasConstant;
	m_rasterizationInfo.depthBiasSlopeFactor = depthBiasSlope;
	// specifies the number of pixels covered by a line per column.
	m_rasterizationInfo.lineWidth = 1.0f;
	return *this;
}

PipelineBuilder& PipelineBuilder::set_color_blend_state(VkColorComponentFlags colorComponentFlags, VkBool32 blendEnable)
{
	m_colorBlendAttachmentState.colorWriteMask = colorComponentFlags;
	m_colorBlendAttachmentState.blendEnable = blendEnable;
	m_colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	m_colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	m_colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	m_colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	m_colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	m_colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	m_colorBlendInfo.logicOpEnable = VK_FALSE;
	m_colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;

	return *this;
}

PipelineBuilder& PipelineBuilder::set_depth_stencil_state(VkBool32 depthTestEnable, VkCompareOp depthCompareOp) {
	m_depthStencilInfo.depthTestEnable = depthTestEnable;
	m_depthStencilInfo.depthWriteEnable = VK_TRUE;
	m_depthStencilInfo.depthCompareOp = depthCompareOp;
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


VkPipeline PipelineBuilder::build() {

	if (m_color_output_disabled) {
		m_renderingInfo.colorAttachmentCount = 0;
		m_renderingInfo.pColorAttachmentFormats = nullptr;
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{ .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineInfo.pNext = &m_renderingInfo;
	pipelineInfo.stageCount = static_cast<uint32_t>(m_shaderInfos.size());
	pipelineInfo.pStages = m_shaderInfos.data();
	pipelineInfo.pVertexInputState = &m_vertInputInfo; // we will be using buffer device address, no need to describe to the pipeline how to read the vertex attribute data.

	// set this always to triangle list topology for now
	m_inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
	m_inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	pipelineInfo.pInputAssemblyState = &m_inputAssemblyInfo;

	// viewport and scissor state will be set dynamically during command buffer recording
	VkDynamicState dynamicStates[]{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicStateInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(std::size(dynamicStates));
	dynamicStateInfo.pDynamicStates = dynamicStates;
	pipelineInfo.pDynamicState = &dynamicStateInfo;

	// we still need to provide the viewport state info
	VkPipelineViewportStateCreateInfo viewportStateInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportStateInfo.viewportCount = 1;
	viewportStateInfo.scissorCount = 1;

	pipelineInfo.pViewportState = &viewportStateInfo;

	pipelineInfo.pRasterizationState = &m_rasterizationInfo;

	// use point sampling for now
	m_multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	m_multisampleInfo.sampleShadingEnable = VK_FALSE;
	m_multisampleInfo.alphaToCoverageEnable = VK_FALSE;

	pipelineInfo.pMultisampleState = &m_multisampleInfo;

	pipelineInfo.pDepthStencilState = &m_depthStencilInfo;

	m_colorBlendInfo.attachmentCount = 1;
	m_colorBlendInfo.pAttachments = &m_colorBlendAttachmentState;

	pipelineInfo.pColorBlendState = &m_colorBlendInfo;

	pipelineInfo.layout = pipelineLayout;

	VkPipeline pipeline{};
	VK_CHECK(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));

	return pipeline;
}