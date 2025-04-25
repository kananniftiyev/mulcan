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
    float mFar = 100.f;

    const VmaAllocator *mAllocator;
    Mulcan::Descriptor::DoubleBufferedDescriptorCtx mDescriptor;

public:
    Camera(const Mulcan::Descriptor::DoubleBufferedDescriptorCtx &ctx, const VmaAllocator *allocator);

    Camera(const Camera &other) = delete;
    Camera operator=(const Camera &other) = delete;

    void updateCamera(int frameIndex);

    /**
     * @brief Sets the position of the camera in world space.
     *
     * @param position A 3D vector representing the new camera position.
     */
    void changeCameraPos(const glm::vec3 &position);

    /**
     * @brief Updates the camera's projection matrix using the specified perspective parameters.
     *
     * @param fov    Field of view in degrees (vertical).
     * @param aspect Aspect ratio of the viewport (width / height).
     * @param near   Distance to the near clipping plane.
     * @param end    Distance to the far clipping plane.
     */
    void changeProjection(float fov, float aspect, float near, float end);

    /**
     * @brief Updates the aspect ratio of the camera projection based on new viewport dimensions.
     *
     * @param width  Width of the viewport.
     * @param height Height of the viewport.
     */
    void changeAspect(float width, float height);

    /// @brief Returns the Descriptor set for a certain frame.
    /// @param frameIndex Frame scene currently is in.
    /// @return VkDescriptorSet
    VkDescriptorSet &getSet(int frameIndex);

    /// @brief Returns the Camera data such as view and projection.
    /// @return GPUCameraData
    GPUCameraData &getCameraData();

    void cleanup();
};
