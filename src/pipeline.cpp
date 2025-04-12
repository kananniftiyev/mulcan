#include "pipeline.hpp"

void Mulcan::Pipeline::CreatePipeline(const char *pVertexShaderPath, const char *pFragmentShaderPath)
{

    auto vertexShaderModule = Pipeline::createShaderModule(pVertexShaderPath);
    auto fragmentShaderModule = Pipeline::createShaderModule(pFragmentShaderPath);

    VkVertexInputBindingDescription vertex_input_binding{};
    vertex_input_binding.binding = 0;
    vertex_input_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vertex_input_binding.stride = sizeof(Mulcan::Vertex);

    // TODO: better handling
    std::array<VkVertexInputAttributeDescription, 4> input_attributes;

    // position binding
    input_attributes[0].binding = 0;
    input_attributes[0].location = 0;
    input_attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    input_attributes[0].offset = offsetof(Mulcan::Vertex, Mulcan::Vertex::position);

    // color bindigs
    input_attributes[1].binding = 0;
    input_attributes[1].location = 1;
    input_attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    input_attributes[1].offset = offsetof(Mulcan::Vertex, Mulcan::Vertex::color);

    // texCords
    input_attributes[2].binding = 0;
    input_attributes[2].location = 2;
    input_attributes[2].format = VK_FORMAT_R32G32_SFLOAT;
    input_attributes[2].offset = offsetof(Mulcan::Vertex, Mulcan::Vertex::texCoords);

    // normals
    input_attributes[3].binding = 0;
    input_attributes[3].location = 3;
    input_attributes[3].format = VK_FORMAT_R32G32B32_SFLOAT;
    input_attributes[3].offset = offsetof(Mulcan::Vertex, Mulcan::Vertex::normals);

    std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;

    shader_stages[0] = MulcanInfos::createShaderStageInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule);
    shader_stages[1] = MulcanInfos::createShaderStageInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderModule);

    auto pipeline_vertex_state = MulcanInfos::createPipelineVertexInputState(vertex_input_binding, input_attributes);

    auto input_assembly_state = MulcanInfos::createInputAssemblyStateInfo();

    VkPipelineViewportStateCreateInfo vp_info{};
    vp_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp_info.scissorCount = 1;
    vp_info.viewportCount = 1;

    auto raster_state_info = MulcanInfos::createRasterizationStateInfo();

    auto multisample_state_info = MulcanInfos::createMultisampleStateInfo(VK_FALSE, VK_SAMPLE_COUNT_1_BIT);

    std::vector<VkDynamicState> dynamic_states_enables;
    dynamic_states_enables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    dynamic_states_enables.push_back(VK_DYNAMIC_STATE_SCISSOR);

    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.pDynamicStates = dynamic_states_enables.data();
    dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states_enables.size());

    VkPipelineColorBlendAttachmentState blend_attachment_state{};
    blend_attachment_state.colorWriteMask = 0xf;
    blend_attachment_state.blendEnable = VK_FALSE;
    VkPipelineColorBlendStateCreateInfo color_blend_state{};
    color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state.attachmentCount = 1;
    color_blend_state.pAttachments = &blend_attachment_state;

    VkPipelineDepthStencilStateCreateInfo depth_state{};
    depth_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_state.depthTestEnable = VK_TRUE;
    depth_state.depthWriteEnable = VK_TRUE;
    depth_state.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_state.depthBoundsTestEnable = VK_FALSE;
    depth_state.stencilTestEnable = VK_FALSE;

    VkGraphicsPipelineCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.stageCount = static_cast<uint32_t>(shader_stages.size());
    info.pStages = shader_stages.data();
    info.pVertexInputState = &pipeline_vertex_state;
    info.pInputAssemblyState = &input_assembly_state;
    info.pViewportState = &vp_info;
    info.pRasterizationState = &raster_state_info;
    info.pMultisampleState = &multisample_state_info;
    info.pDynamicState = &dynamic_state;
    info.layout = this->mPipelineLayout;
    info.renderPass = this->mRenderPass;
    info.pNext = nullptr;
    info.pColorBlendState = &color_blend_state;
    info.pDepthStencilState = &depth_state;

    CHECK_VK_LOG(vkCreateGraphicsPipelines(this->mDevice, nullptr, 1, &info, nullptr, &this->mPipeline));

    vkDestroyShaderModule(this->mDevice, vertexShaderModule, nullptr);
    vkDestroyShaderModule(this->mDevice, fragmentShaderModule, nullptr);

    this->mPipelineCount++;
}

void Mulcan::Pipeline::CreatePipelineLayout(const std::vector<VkPushConstantRange> &pPushConstants, const std::vector<VkDescriptorSetLayout> &pSetLayouts)
{
    VkPipelineLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.pushConstantRangeCount = pPushConstants.size();
    info.pPushConstantRanges = pPushConstants.data();
    info.setLayoutCount = pSetLayouts.size();
    info.pSetLayouts = pSetLayouts.data();

    CHECK_VK_LOG(vkCreatePipelineLayout(this->mDevice, &info, nullptr, &this->mPipelineLayout));

    this->mPipelineLayoutCount++;
}

VkShaderModule Mulcan::Pipeline::createShaderModule(const char *pShaderPath)
{
    std::ifstream file(pShaderPath, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        return VK_NULL_HANDLE;
    }

    size_t fileSize = (size_t)file.tellg();

    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    file.seekg(0);

    file.read((char *)buffer.data(), fileSize);

    file.close();

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = nullptr;

    createInfo.codeSize = buffer.size() * sizeof(uint32_t);
    createInfo.pCode = buffer.data();

    VkShaderModule shaderModule;
    vkCreateShaderModule(this->mDevice, &createInfo, nullptr, &shaderModule);

    return shaderModule;
}

void Mulcan::Pipeline::DestroyPipeline()
{
    vkDestroyPipeline(this->mDevice, this->mPipeline, nullptr);
}
