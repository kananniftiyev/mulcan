#include "descriptor.hpp"

Mulcan::DescriptorManager::DescriptorManager(VmaAllocator &pAllocator, VkDevice &pDevice) : mAllocator(pAllocator), mDevice(pDevice)
{
    std::vector<VkDescriptorPoolSize> staticPoolSizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10},
    };

    VkDescriptorPoolCreateInfo staticPoolInfo{};
    staticPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    staticPoolInfo.flags = 0;
    staticPoolInfo.maxSets = 10;
    staticPoolInfo.poolSizeCount = static_cast<uint32_t>(staticPoolSizes.size());
    staticPoolInfo.pPoolSizes = staticPoolSizes.data();

    CHECK_VK_LOG(vkCreateDescriptorPool(this->mDevice, &staticPoolInfo, nullptr, &this->mStaticObjectPool));
}

void Mulcan::DescriptorManager::CreateDescriptorSet(int id, VkBuffer &pBuffer, size_t pSize)
{

    VkDescriptorSetAllocateInfo setAllocInfo{};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = this->mStaticObjectPool;
    setAllocInfo.pSetLayouts = &this->mDescriptorLayoutMap[id];
    setAllocInfo.descriptorSetCount = 1;

    VkDescriptorSet set;
    CHECK_VK_LOG(vkAllocateDescriptorSets(this->mDevice, &setAllocInfo, &set));

    this->mDescriptorSetMap[id] = set;

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = pBuffer;
    bufferInfo.range = pSize;
    bufferInfo.offset = 0;

    VkWriteDescriptorSet setWrite{};
    setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    setWrite.descriptorCount = 1;
    setWrite.dstSet = set;
    setWrite.pBufferInfo = &bufferInfo;
    setWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    vkUpdateDescriptorSets(this->mDevice, 1, &setWrite, 0, nullptr);
}

void Mulcan::DescriptorManager::CreateDescriptorLayout(int id, const std::vector<VkDescriptorSetLayoutBinding> &pBindings)
{
    VkDescriptorSetLayoutCreateInfo setLayoutInfo{};
    setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutInfo.bindingCount = static_cast<uint32_t>(pBindings.size());
    setLayoutInfo.pBindings = pBindings.data();

    VkDescriptorSetLayout setLayout;
    CHECK_VK_LOG(vkCreateDescriptorSetLayout(this->mDevice, &setLayoutInfo, nullptr, &setLayout));

    this->mDescriptorLayoutMap[id] = setLayout;
}

[[nodiscard]]
Mulcan::AllocatedBuffer Mulcan::DescriptorManager::CreateUniformBuffer(size_t pAllocSize)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = pAllocSize;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    AllocatedBuffer allocatedBuffer{};

    CHECK_VK_LOG(vmaCreateBuffer(this->mAllocator, &bufferInfo, &allocInfo, &allocatedBuffer.buffer, &allocatedBuffer.allocation, nullptr));

    return allocatedBuffer;
}

void Mulcan::DescriptorManager::Cleanup()
{
    vkDestroyDescriptorPool(this->mDevice, this->mStaticObjectPool, nullptr);
    vkDestroyDescriptorPool(this->mDevice, this->mDynamicObjectPool, nullptr);
}
