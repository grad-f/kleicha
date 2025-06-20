#ifndef PIPELINEBUILDER
#define PIPELINEBUILDER

#include <vector>
#include <vulkan/vulkan.h>

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

	void set_shaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);

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

void PipelineBuilder::set_shaders(VkShaderModule vertexShader, VkShaderModule fragmentShader) {
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

VkPipeline PipelineBuilder::build(VkDevice device) {
	VkGraphicsPipelineCreateInfo pipelineInfo{ .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineInfo.stageCount = static_cast<uint32_t>(m_shaderInfos.size());
	pipelineInfo.pStages = m_shaderInfos.data();
	pipelineInfo.pVertexInputState = &m_vertInputInfo; // we will be using buffer device address, no need to describe to the pipeline how to read the vertex attribute data.

	// set this always to triangle list topology for now
	m_inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
	m_inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	pipelineInfo.pInputAssemblyState = &m_inputAssemblyInfo;

}

#endif // !PIPELINEBUILDER
