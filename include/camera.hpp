#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <array>
#include <descriptor.hpp>
#include <memory>

struct GPUCameraData
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewproj;
};

class Camera final
{
private:
    glm::vec3 mCamPosition;
    glm::vec3 mCamRotation = glm::vec3(0, 0, 0);
    GPUCameraData mCameraData;

    glm::mat4 mProjection;
    glm::mat4 mView = glm::mat4(1.0f);

    // Projection Values
    float mFov = glm::radians(70.f);
    float mAspect = 1920.f / 1080.f;
    float mNear = 0.1f;
    float mEnd = 100.f;

    void Initialize();

    VmaAllocator &mAllocator;

public:
    Mulcan::Descriptor::DoubleDescriptorCtx mCtx;

    Camera(const Mulcan::Descriptor::DoubleDescriptorCtx &ctx, VmaAllocator &allocator);

    Camera(const Camera &other) = delete;
    Camera operator=(const Camera &other) = delete;

    GPUCameraData &GetCameraData();
    void UpdateCamera(int frameIndex);

    void ChangeCameraPos(const glm::vec3 &position);
    void ChangeProjection(float fov, float aspect, float near, float end);
    void changeAspect(float width, float height);

    void Cleanup();
};
