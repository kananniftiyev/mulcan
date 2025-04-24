#include "camera.hpp"

Camera::Camera(const Mulcan::Descriptor::DoubleDescriptorCtx &ctx, VmaAllocator &allocator) : mCtx(ctx), mAllocator(allocator)
{
    // Inital Camera Settings
    mCamPosition = {0.f, -0.f, -5.f};
    mView = glm::mat4(1.0f);
    mView = glm::translate(mView, mCamPosition);

    mProjection = glm::perspective(mFov, mAspect, mNear, mEnd);
    mProjection[1][1] *= -1;

    // Descritpor set
}

GPUCameraData &Camera::GetCameraData()
{
    return mCameraData;
}

void Camera::UpdateCamera(int frameIndex)
{
    mCameraData.proj = mProjection;
    mCameraData.view = mView;
    mCameraData.viewproj = mProjection * mView;
    Mulcan::Descriptor::updateData(mAllocator, mCtx.buffers[frameIndex].allocation, &mCameraData, sizeof(GPUCameraData));
}

void Camera::ChangeCameraPos(const glm::vec3 &position)
{
    if (mCamPosition != position)
    {
        mView = glm::translate(mView, position);
        mCamPosition = position;
    }
}

void Camera::ChangeProjection(float fov, float aspect, float near, float end)
{
    if (mFov != fov || mAspect != aspect || mNear != near || mEnd != end)
    {
        mProjection = glm::perspective(fov, aspect, near, end);
        mProjection[1][1] *= -1;

        mFov = fov;
        mAspect = aspect;
        mNear = near;
        mEnd = end;
    }
}

void Camera::changeAspect(float width, float height)
{
    mAspect = width / height;
}

void Camera::Cleanup()
{
}
