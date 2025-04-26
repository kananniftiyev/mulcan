#pragma once

#include <iostream>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "common_types.hpp"

namespace Mulcan::Components
{
    struct MeshComponent
    {
        std::string name;
        AllocatedBuffer vertexBuffer;
        AllocatedBuffer indexBuffer;
        uint32_t indexCount;
        bool isVisible = false;
    };

    struct TransformComponent
    {
        glm::vec3 position;
        glm::vec3 rotation;
        glm::vec3 scale;
        bool isChanged;
    };
} // namespace Mulcan::Components
