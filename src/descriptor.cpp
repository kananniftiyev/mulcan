#include "descriptor.hpp"

void Mulcan::Descriptor::addBindingToSetDB(const VkDescriptorSetLayoutBinding &binding, DoubleBufferedDescriptorBuildCtx *ctx)
{
    VkDescriptorSetLayoutCreateInfo setLayoutInfo{};
    setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutInfo.bindingCount = 1;
    setLayoutInfo.pBindings = &binding;

    ctx->layoutCreateInfo = setLayoutInfo;
}

void Mulcan::Descriptor::addBufferToSetDB(size_t allocationSize, DoubleBufferedDescriptorBuildCtx *ctx)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = allocationSize;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    ctx->allocSize = allocationSize;
    ctx->bufferCreateInfo = bufferInfo;
}

void Mulcan::Descriptor::updateData(VmaAllocator &allocator, VmaAllocation allocation, const void *pData, size_t size)
{
    void *data;
    vmaMapMemory(allocator, allocation, &data);

    memcpy(data, pData, size);

    vmaUnmapMemory(allocator, allocation);
}

Mulcan::Descriptor::DoubleBufferedDescriptorCtx Mulcan::Descriptor::buildSetDB(VkDevice &device, VmaAllocator &allocator, DoubleBufferedDescriptorBuildCtx *ctx)
{
    Mulcan::Descriptor::DoubleBufferedDescriptorCtx outCtx{};

    std::vector<VkDescriptorPoolSize> globalPoolSizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10},
    };

    VkDescriptorPoolCreateInfo staticPoolInfo{};
    staticPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    staticPoolInfo.flags = 0;
    staticPoolInfo.maxSets = 10;
    staticPoolInfo.poolSizeCount = static_cast<uint32_t>(globalPoolSizes.size());
    staticPoolInfo.pPoolSizes = globalPoolSizes.data();

    CHECK_VK_LOG(vkCreateDescriptorPool(device, &staticPoolInfo, nullptr, &ctx->pool));

    CHECK_VK_LOG(vkCreateDescriptorSetLayout(device, &ctx->layoutCreateInfo, nullptr, &outCtx.layout));

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    for (size_t i = 0; i < 2; i++)
    {
        VkDescriptorSetAllocateInfo setAllocInfo{};
        setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        setAllocInfo.descriptorPool = ctx->pool;
        setAllocInfo.pSetLayouts = &outCtx.layout;
        setAllocInfo.descriptorSetCount = 1;

        CHECK_VK_LOG(vkAllocateDescriptorSets(device, &setAllocInfo, &outCtx.set[i]));

        CHECK_VK_LOG(vmaCreateBuffer(allocator, &ctx->bufferCreateInfo, &allocInfo, &outCtx.buffers[i].buffer, &outCtx.buffers[i].allocation, nullptr));

        VkDescriptorBufferInfo cameraBufferInfo{};
        cameraBufferInfo.buffer = outCtx.buffers[i].buffer;
        cameraBufferInfo.range = ctx->allocSize;
        cameraBufferInfo.offset = 0;

        VkWriteDescriptorSet setWrite{};
        setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        setWrite.descriptorCount = 1;
        setWrite.dstSet = outCtx.set[i];
        setWrite.pBufferInfo = &cameraBufferInfo;
        setWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        setWrite.dstBinding = 0;

        vkUpdateDescriptorSets(device, 1, &setWrite, 0, nullptr);
    }

    return outCtx;
}
