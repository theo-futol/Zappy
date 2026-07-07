#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Zappy
{

/// @brief 4x4 float matrix (alias over GLM, kept as a swap seam).
using Mat4 = glm::mat4;

/// @brief 3x3 float matrix (alias over GLM, kept as a swap seam).
using Mat3 = glm::mat3;

/// @brief Float quaternion (alias over GLM), used for VR head/controller orientations.
using Quat = glm::quat;

} // namespace Zappy
