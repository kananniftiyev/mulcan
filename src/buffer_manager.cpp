#include "buffer_manager.hpp"

BufferManager::BufferManager(const VkDevice *device, const VmaAllocator *allocator) : mDevice(device), mAllocator(allocator)
{
}

[[nodiscard]]
AllocatedBuffer BufferManager::createCPUOnlyBuffer(const void *data, size_t allocSize, VkBufferUsageFlags flag, VkBufferUsageFlagBits usage)
{
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.flags = flag;
    bufferCreateInfo.size = allocSize;
    bufferCreateInfo.usage = usage;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

    AllocatedBuffer buffer;
    CHECK_VK_LOG(vmaCreateBuffer(*this->mAllocator, &bufferCreateInfo, &allocInfo, &buffer.buffer, &buffer.allocation, nullptr));

    // this->mDeletionBuffers.push(buffer.buffer);

    this->mBufferCount++;

    return buffer;
}

[[nodiscard]]
AllocatedBuffer BufferManager::createStagingBuffer(const void *data, size_t allocSize, VkBufferUsageFlags flag)
{
    // CPU side
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = allocSize;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo vma_alloc_info{};
    vma_alloc_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    vma_alloc_info.memoryTypeBits = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    AllocatedBuffer staging_buffer;
    CHECK_VK_LOG(vmaCreateBuffer(*this->mAllocator, &buffer_info, &vma_alloc_info, &staging_buffer.buffer, &staging_buffer.allocation, nullptr));

    void *lData;
    vmaMapMemory(*this->mAllocator, staging_buffer.allocation, &lData);

    memcpy(lData, data, allocSize);

    vmaUnmapMemory(*this->mAllocator, staging_buffer.allocation);

    // GPU side
    VkBufferCreateInfo gpu_buffer_info{};
    gpu_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    gpu_buffer_info.size = allocSize;
    gpu_buffer_info.usage = flag | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    vma_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vma_alloc_info.memoryTypeBits = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    AllocatedBuffer gpu_buffer;

    CHECK_VK_LOG(vmaCreateBuffer(*this->mAllocator, &gpu_buffer_info, &vma_alloc_info, &gpu_buffer.buffer, &gpu_buffer.allocation, nullptr));

    TransferBuffer tb{};
    tb.src = staging_buffer;
    tb.dst = gpu_buffer;
    tb.buffer_size = allocSize;

    if (tb.buffer_size == 0 || tb.src.buffer == VK_NULL_HANDLE || tb.dst.buffer == VK_NULL_HANDLE)
    {
        spdlog::error("FALSE BUFFER!!!");
        abort();
    }
    this->mStagingBuffers.push_back(tb);

    this->mDeletionBuffers.push(gpu_buffer);

    this->mBufferCount++;

    return gpu_buffer;
}

[[nodiscard]]
AllocatedBuffer BufferManager::createCPUToGPUBuffer(const void *data, size_t allocSize, VkBufferUsageFlags flag, VkBufferUsageFlagBits usage)
{
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.flags = flag;
    bufferCreateInfo.size = allocSize;
    bufferCreateInfo.usage = usage;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    AllocatedBuffer buffer;
    CHECK_VK_LOG(vmaCreateBuffer(*this->mAllocator, &bufferCreateInfo, &allocInfo, &buffer.buffer, &buffer.allocation, nullptr));

    // this->mDeletionBuffers.push(buffer.buffer);

    this->mBufferCount++;
    return buffer;
}

void BufferManager::releaseNow(VkBuffer &buffer)
{
    vkDestroyBuffer(*this->mDevice, buffer, nullptr);
    this->mBufferCount--;
}

void BufferManager::releaseAllResources()
{
    LOG("BufferManager: Realeasing all Resources");
    spdlog::info("Buffer count {}", this->mBufferCount);
    while (this->mDeletionBuffers.empty())
    {
        vmaDestroyBuffer(*this->mAllocator, mDeletionBuffers.front().buffer, mDeletionBuffers.front().allocation);
        mDeletionBuffers.pop();
    }
}

std::uint32_t BufferManager::getBufferCount() const
{
    return this->mBufferCount;
}

std::vector<TransferBuffer> &BufferManager::getStaginBuffers()
{
    return this->mStagingBuffers;
}
