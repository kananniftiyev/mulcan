#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include "mulcan_infos.hpp"
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "mulkan_macros.hpp"

namespace Mulcan
{
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 color;
        glm::vec2 texCoords;
        glm::vec3 normals;
    };

    class Pipeline
    {
    private:
        // TODO: Caching support.
        VkPipelineCache mPipelineCache;
        VkPipelineLayout mPipelineLayout;
        VkPipeline mPipeline;
        const VkRenderPass &mRenderPass;

        const VkDevice &mDevice;

        VkShaderModule createShaderModule(const char *pShaderPath);

    public:
        static size_t mPipelineCount;
        static size_t mPipelineLayoutCount;
        Pipeline(VkDevice &pDevice, VkRenderPass &pRenderPass);

        void CreatePipelineLayout(const std::vector<VkPushConstantRange> &pPushConstants, const std::vector<VkDescriptorSetLayout> &pSetLayouts);
        void CreatePipeline(const char *pVertexShaderPath, const char *pFragmentShaderPath);

        VkPipeline &GetPipeline() { return this->mPipeline; }
        VkPipelineLayout &GetPipelineLayout() { return this->mPipelineLayout; }

        void DestroyPipeline();
    };
} // namespace Mulcan
