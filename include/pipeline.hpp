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
        static size_t mPipelineCount;
        static size_t mPipelineLayoutCount;

        // TODO: Caching support.
        VkPipelineCache mPipelineCache;
        VkPipelineLayout mPipelineLayout;
        VkPipeline mPipeline;
        VkRenderPass &mRenderPass;

        VkDevice &mDevice;

        VkShaderModule createShaderModule(const char *pShaderPath);

    public:
        Pipeline(const VkDevice &pDevice, const VkRenderPass &pRenderPass);

        void CreatePipelineLayout(const std::vector<VkPushConstantRange> &pPushConstants, const std::vector<VkDescriptorSetLayout> &pSetLayouts);
        void CreatePipeline(const char *pVertexShaderPath, const char *pFragmentShaderPath);

        VkPipeline &GetPipeline() { return this->mPipeline; }

        void DestroyPipeline();
    };
} // namespace Mulcan
