#include "camera.hpp"

Camera::Camera(const Mulcan::Descriptor::DoubleBufferedDescriptorCtx &ctx, const VmaAllocator *allocator) : mDescriptor(ctx), mAllocator(allocator)
{
    // Inital Camera Settings
    mCamPosition = {0.f, -0.f, -5.f};
    mView = glm::mat4(1.0f);
    mView = glm::translate(mView, mCamPosition);

    mProjection = glm::perspective(mFov, mAspect, mNear, mFar);
    mProjection[1][1] *= -1;

    // Descritpor set
}

VkDescriptorSet &Camera::getSet(int frameIndex)
{
    return this->mDescriptor.set[frameIndex];
}

GPUCameraData &Camera::getCameraData()
{
    return mCameraData;
}

void Camera::updateCamera(int frameIndex)
{
    mCameraData.proj = mProjection;
    mCameraData.view = mView;
    mCameraData.viewproj = mProjection * mView;
    Mulcan::Descriptor::updateData(*mAllocator, mDescriptor.buffers[frameIndex].allocation, &mCameraData, sizeof(GPUCameraData));
}

void Camera::changeCameraPos(const glm::vec3 &position)
{
    if (mCamPosition != position)
    {
        mView = glm::translate(mView, position);
        mCamPosition = position;
    }
}

void Camera::changeProjection(float fov, float aspect, float near, float end)
{
    if (mFov != fov || mAspect != aspect || mNear != near || mFar != end)
    {
        mProjection = glm::perspective(fov, aspect, near, end);
        mProjection[1][1] *= -1;

        mFov = fov;
        mAspect = aspect;
        mNear = near;
        mFar = end;
    }
}

void Camera::changeAspect(float width, float height)
{
    mAspect = width / height;
}

void Camera::cleanup()
{
}
